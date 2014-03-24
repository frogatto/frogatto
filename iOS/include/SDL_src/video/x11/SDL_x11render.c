/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2010 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

#if SDL_VIDEO_RENDER_X11

#include <limits.h> /* For INT_MIN and INT_MAX */

#include "SDL_x11video.h"
#include "../SDL_rect_c.h"
#include "../SDL_pixels_c.h"
#include "../SDL_yuv_sw_c.h"

/* X11 renderer implementation */

static SDL_Renderer *X11_CreateRenderer(SDL_Window * window, Uint32 flags);
static int X11_DisplayModeChanged(SDL_Renderer * renderer);
static int X11_CreateTexture(SDL_Renderer * renderer, SDL_Texture * texture);
static int X11_QueryTexturePixels(SDL_Renderer * renderer,
                                  SDL_Texture * texture, void **pixels,
                                  int *pitch);
static int X11_SetTextureBlendMode(SDL_Renderer * renderer,
                                   SDL_Texture * texture);
static int X11_SetTextureScaleMode(SDL_Renderer * renderer,
                                   SDL_Texture * texture);
static int X11_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                             const SDL_Rect * rect, const void *pixels,
                             int pitch);
static int X11_LockTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                           const SDL_Rect * rect, int markDirty,
                           void **pixels, int *pitch);
static void X11_UnlockTexture(SDL_Renderer * renderer, SDL_Texture * texture);
static int X11_SetDrawBlendMode(SDL_Renderer * renderer);
static int X11_RenderDrawPoints(SDL_Renderer * renderer,
                                const SDL_Point * points, int count);
static int X11_RenderDrawLines(SDL_Renderer * renderer,
                               const SDL_Point * points, int count);
static int X11_RenderDrawRects(SDL_Renderer * renderer,
                               const SDL_Rect ** rects, int count);
static int X11_RenderFillRects(SDL_Renderer * renderer,
                               const SDL_Rect ** rects, int count);
static int X11_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
                          const SDL_Rect * srcrect, const SDL_Rect * dstrect);
static int X11_RenderReadPixels(SDL_Renderer * renderer, const SDL_Rect * rect,
                                Uint32 format, void * pixels, int pitch);
static int X11_RenderWritePixels(SDL_Renderer * renderer, const SDL_Rect * rect,
                                 Uint32 format, const void * pixels, int pitch);
static void X11_RenderPresent(SDL_Renderer * renderer);
static void X11_DestroyTexture(SDL_Renderer * renderer,
                               SDL_Texture * texture);
static void X11_DestroyRenderer(SDL_Renderer * renderer);


SDL_RenderDriver X11_RenderDriver = {
    X11_CreateRenderer,
    {
     "x11",
     (SDL_RENDERER_SINGLEBUFFER | SDL_RENDERER_PRESENTCOPY |
      SDL_RENDERER_PRESENTFLIP2 | SDL_RENDERER_PRESENTFLIP3 |
      SDL_RENDERER_PRESENTDISCARD | SDL_RENDERER_ACCELERATED),
     SDL_TEXTUREMODULATE_NONE,
     SDL_BLENDMODE_NONE,
     SDL_TEXTURESCALEMODE_NONE,
     0,
     {0},
     0,
     0}
};

typedef struct
{
    Display *display;
    int screen;
    Visual *visual;
    int depth;
    int scanline_pad;
    Window xwindow;
    Pixmap pixmaps[3];
    int current_pixmap;
    Drawable drawable;
    SDL_PixelFormat format;
    GC gc;
    SDL_DirtyRectList dirty;
    SDL_bool makedirty;
} X11_RenderData;

typedef struct
{
    SDL_SW_YUVTexture *yuv;
    Uint32 format;
    Pixmap pixmap;
    XImage *image;
#ifndef NO_SHARED_MEMORY
    /* MIT shared memory extension information */
    XShmSegmentInfo shminfo;
#endif
    XImage *scaling_image;
    void *pixels;
    int pitch;
} X11_TextureData;

#ifndef NO_SHARED_MEMORY
/* Shared memory error handler routine */
static int shm_error;
static int (*X_handler) (Display *, XErrorEvent *) = NULL;
static int
shm_errhandler(Display * d, XErrorEvent * e)
{
    if (e->error_code == BadAccess) {
        shm_error = True;
        return (0);
    } else {
        return (X_handler(d, e));
    }
}
#endif /* ! NO_SHARED_MEMORY */

static void
UpdateYUVTextureData(SDL_Texture * texture)
{
    X11_TextureData *data = (X11_TextureData *) texture->driverdata;
    SDL_Rect rect;

    rect.x = 0;
    rect.y = 0;
    rect.w = texture->w;
    rect.h = texture->h;
    SDL_SW_CopyYUVToRGB(data->yuv, &rect, data->format, texture->w,
                        texture->h, data->pixels, data->pitch);
}

void
X11_AddRenderDriver(_THIS)
{
    SDL_RendererInfo *info = &X11_RenderDriver.info;
    SDL_DisplayMode *mode = &SDL_CurrentDisplay->desktop_mode;
    int i;

    info->texture_formats[info->num_texture_formats++] = mode->format;
    info->texture_formats[info->num_texture_formats++] = SDL_PIXELFORMAT_YV12;
    info->texture_formats[info->num_texture_formats++] = SDL_PIXELFORMAT_IYUV;
    info->texture_formats[info->num_texture_formats++] = SDL_PIXELFORMAT_YUY2;
    info->texture_formats[info->num_texture_formats++] = SDL_PIXELFORMAT_UYVY;
    info->texture_formats[info->num_texture_formats++] = SDL_PIXELFORMAT_YVYU;

    for (i = 0; i < _this->num_displays; ++i) {
        SDL_AddRenderDriver(&_this->displays[i], &X11_RenderDriver);
    }
}

SDL_Renderer *
X11_CreateRenderer(SDL_Window * window, Uint32 flags)
{
    SDL_VideoDisplay *display = window->display;
    SDL_DisplayData *displaydata = (SDL_DisplayData *) display->driverdata;
    SDL_WindowData *windowdata = (SDL_WindowData *) window->driverdata;
    SDL_Renderer *renderer;
    X11_RenderData *data;
    XGCValues gcv;
    int i, n;
    int bpp;
    Uint32 Rmask, Gmask, Bmask, Amask;

    renderer = (SDL_Renderer *) SDL_calloc(1, sizeof(*renderer));
    if (!renderer) {
        SDL_OutOfMemory();
        return NULL;
    }

    data = (X11_RenderData *) SDL_calloc(1, sizeof(*data));
    if (!data) {
        X11_DestroyRenderer(renderer);
        SDL_OutOfMemory();
        return NULL;
    }
    data->display = windowdata->videodata->display;
    data->screen = displaydata->screen;
    data->visual = displaydata->visual;
    data->depth = displaydata->depth;
    data->scanline_pad = displaydata->scanline_pad;
    data->xwindow = windowdata->xwindow;

    renderer->DisplayModeChanged = X11_DisplayModeChanged;
    renderer->CreateTexture = X11_CreateTexture;
    renderer->QueryTexturePixels = X11_QueryTexturePixels;
    renderer->SetTextureBlendMode = X11_SetTextureBlendMode;
    renderer->SetTextureScaleMode = X11_SetTextureScaleMode;
    renderer->UpdateTexture = X11_UpdateTexture;
    renderer->LockTexture = X11_LockTexture;
    renderer->UnlockTexture = X11_UnlockTexture;
    renderer->SetDrawBlendMode = X11_SetDrawBlendMode;
    renderer->RenderDrawPoints = X11_RenderDrawPoints;
    renderer->RenderDrawLines = X11_RenderDrawLines;
    renderer->RenderDrawRects = X11_RenderDrawRects;
    renderer->RenderFillRects = X11_RenderFillRects;
    renderer->RenderCopy = X11_RenderCopy;
    renderer->RenderReadPixels = X11_RenderReadPixels;
    renderer->RenderWritePixels = X11_RenderWritePixels;
    renderer->RenderPresent = X11_RenderPresent;
    renderer->DestroyTexture = X11_DestroyTexture;
    renderer->DestroyRenderer = X11_DestroyRenderer;
    renderer->info = X11_RenderDriver.info;
    renderer->window = window;
    renderer->driverdata = data;

    renderer->info.flags = SDL_RENDERER_ACCELERATED;

    if (flags & SDL_RENDERER_SINGLEBUFFER) {
        renderer->info.flags |=
            (SDL_RENDERER_SINGLEBUFFER | SDL_RENDERER_PRESENTCOPY);
        n = 0;
    } else if (flags & SDL_RENDERER_PRESENTFLIP2) {
        renderer->info.flags |= SDL_RENDERER_PRESENTFLIP2;
        n = 2;
    } else if (flags & SDL_RENDERER_PRESENTFLIP3) {
        renderer->info.flags |= SDL_RENDERER_PRESENTFLIP3;
        n = 3;
    } else {
        renderer->info.flags |= SDL_RENDERER_PRESENTCOPY;
        n = 1;
    }
    for (i = 0; i < n; ++i) {
        data->pixmaps[i] =
            XCreatePixmap(data->display, data->xwindow, window->w, window->h,
                          displaydata->depth);
        if (data->pixmaps[i] == None) {
            X11_DestroyRenderer(renderer);
            SDL_SetError("XCreatePixmap() failed");
            return NULL;
        }
    }
    if (n > 0) {
        data->drawable = data->pixmaps[0];
        data->makedirty = SDL_TRUE;
    } else {
        data->drawable = data->xwindow;
        data->makedirty = SDL_FALSE;
    }
    data->current_pixmap = 0;

    /* Get the format of the window */
    if (!SDL_PixelFormatEnumToMasks
        (display->current_mode.format, &bpp, &Rmask, &Gmask, &Bmask,
         &Amask)) {
        SDL_SetError("Unknown display format");
        X11_DestroyRenderer(renderer);
        return NULL;
    }
    SDL_InitFormat(&data->format, bpp, Rmask, Gmask, Bmask, Amask);

    /* Create the drawing context */
    gcv.graphics_exposures = False;
    data->gc =
        XCreateGC(data->display, data->xwindow, GCGraphicsExposures, &gcv);
    if (!data->gc) {
        X11_DestroyRenderer(renderer);
        SDL_SetError("XCreateGC() failed");
        return NULL;
    }

    return renderer;
}

static int
X11_DisplayModeChanged(SDL_Renderer * renderer)
{
    X11_RenderData *data = (X11_RenderData *) renderer->driverdata;
    SDL_Window *window = renderer->window;
    int i, n;

    if (renderer->info.flags & SDL_RENDERER_SINGLEBUFFER) {
        n = 0;
    } else if (renderer->info.flags & SDL_RENDERER_PRESENTFLIP2) {
        n = 2;
    } else if (renderer->info.flags & SDL_RENDERER_PRESENTFLIP3) {
        n = 3;
    } else {
        n = 1;
    }
    for (i = 0; i < n; ++i) {
        if (data->pixmaps[i] != None) {
            XFreePixmap(data->display, data->pixmaps[i]);
            data->pixmaps[i] = None;
        }
    }
    for (i = 0; i < n; ++i) {
        data->pixmaps[i] =
            XCreatePixmap(data->display, data->xwindow, window->w, window->h,
                          data->depth);
        if (data->pixmaps[i] == None) {
            SDL_SetError("XCreatePixmap() failed");
            return -1;
        }
    }
    if (n > 0) {
        data->drawable = data->pixmaps[0];
    }
    data->current_pixmap = 0;

    return 0;
}

static int
X11_CreateTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    X11_RenderData *renderdata = (X11_RenderData *) renderer->driverdata;
    SDL_Window *window = renderer->window;
    SDL_VideoDisplay *display = window->display;
    X11_TextureData *data;
    int pitch_alignmask = ((renderdata->scanline_pad / 8) - 1);

    data = (X11_TextureData *) SDL_calloc(1, sizeof(*data));
    if (!data) {
        SDL_OutOfMemory();
        return -1;
    }

    texture->driverdata = data;

    if (SDL_ISPIXELFORMAT_FOURCC(texture->format)) {
        data->yuv =
            SDL_SW_CreateYUVTexture(texture->format, texture->w, texture->h);
        if (!data->yuv) {
            return -1;
        }
        data->format = display->current_mode.format;
    } else {
        /* The image/pixmap depth must be the same as the window or you
           get a BadMatch error when trying to putimage or copyarea.
         */
        if (texture->format != display->current_mode.format) {
            SDL_SetError("Texture format doesn't match window format");
            return -1;
        }
        data->format = texture->format;
    }
    data->pitch = texture->w * SDL_BYTESPERPIXEL(data->format);
    data->pitch = (data->pitch + pitch_alignmask) & ~pitch_alignmask;

    if (data->yuv || texture->access == SDL_TEXTUREACCESS_STREAMING) {
#ifndef NO_SHARED_MEMORY
        XShmSegmentInfo *shminfo = &data->shminfo;

        shm_error = True;

        if (SDL_X11_HAVE_SHM) {
            shminfo->shmid =
                shmget(IPC_PRIVATE, texture->h * data->pitch,
                       IPC_CREAT | 0777);
            if (shminfo->shmid >= 0) {
                shminfo->shmaddr = (char *) shmat(shminfo->shmid, 0, 0);
                shminfo->readOnly = False;
                if (shminfo->shmaddr != (char *) -1) {
                    shm_error = False;
                    X_handler = XSetErrorHandler(shm_errhandler);
                    XShmAttach(renderdata->display, shminfo);
                    XSync(renderdata->display, False);
                    XSetErrorHandler(X_handler);
                    if (shm_error) {
                        shmdt(shminfo->shmaddr);
                    }
                }
                shmctl(shminfo->shmid, IPC_RMID, NULL);
            }
        }
        if (!shm_error) {
            data->pixels = shminfo->shmaddr;

            data->image =
                XShmCreateImage(renderdata->display, renderdata->visual,
                                renderdata->depth, ZPixmap, shminfo->shmaddr,
                                shminfo, texture->w, texture->h);
            if (!data->image) {
                XShmDetach(renderdata->display, shminfo);
                XSync(renderdata->display, False);
                shmdt(shminfo->shmaddr);
                shm_error = True;
            }
        }
        if (shm_error) {
            shminfo->shmaddr = NULL;
        }
        if (!data->image)
#endif /* not NO_SHARED_MEMORY */
        {
            data->pixels = SDL_malloc(texture->h * data->pitch);
            if (!data->pixels) {
                X11_DestroyTexture(renderer, texture);
                SDL_OutOfMemory();
                return -1;
            }

            data->image =
                XCreateImage(renderdata->display, renderdata->visual,
                             renderdata->depth, ZPixmap, 0, data->pixels,
                             texture->w, texture->h,
                             SDL_BYTESPERPIXEL(data->format) * 8,
                             data->pitch);
            if (!data->image) {
                X11_DestroyTexture(renderer, texture);
                SDL_SetError("XCreateImage() failed");
                return -1;
            }
        }
    } else {
        data->pixmap =
            XCreatePixmap(renderdata->display, renderdata->xwindow, texture->w,
                          texture->h, renderdata->depth);
        if (data->pixmap == None) {
            X11_DestroyTexture(renderer, texture);
            SDL_SetError("XCreatePixmap() failed");
            return -1;
        }

        data->image =
            XCreateImage(renderdata->display, renderdata->visual,
                         renderdata->depth, ZPixmap, 0, NULL, texture->w,
                         texture->h, SDL_BYTESPERPIXEL(data->format) * 8,
                         data->pitch);
        if (!data->image) {
            X11_DestroyTexture(renderer, texture);
            SDL_SetError("XCreateImage() failed");
            return -1;
        }
    }

    return 0;
}

static int
X11_QueryTexturePixels(SDL_Renderer * renderer, SDL_Texture * texture,
                       void **pixels, int *pitch)
{
    X11_TextureData *data = (X11_TextureData *) texture->driverdata;

    if (data->yuv) {
        return SDL_SW_QueryYUVTexturePixels(data->yuv, pixels, pitch);
    } else {
        *pixels = data->pixels;
        *pitch = data->pitch;
        return 0;
    }
}

static int
X11_SetTextureBlendMode(SDL_Renderer * renderer, SDL_Texture * texture)
{
    switch (texture->blendMode) {
    case SDL_BLENDMODE_NONE:
        return 0;
    default:
        SDL_Unsupported();
        texture->blendMode = SDL_BLENDMODE_NONE;
        return -1;
    }
}

static int
X11_SetTextureScaleMode(SDL_Renderer * renderer, SDL_Texture * texture)
{
    X11_TextureData *data = (X11_TextureData *) texture->driverdata;

    switch (texture->scaleMode) {
    case SDL_TEXTURESCALEMODE_NONE:
        return 0;
    case SDL_TEXTURESCALEMODE_FAST:
        /* We can sort of fake it for streaming textures */
        if (data->yuv || texture->access == SDL_TEXTUREACCESS_STREAMING) {
            return 0;
        }
        /* Fall through to unsupported case */
    default:
        SDL_Unsupported();
        texture->scaleMode = SDL_TEXTURESCALEMODE_NONE;
        return -1;
    }
    return 0;
}

static int
X11_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                  const SDL_Rect * rect, const void *pixels, int pitch)
{
    X11_TextureData *data = (X11_TextureData *) texture->driverdata;

    if (data->yuv) {
        if (SDL_SW_UpdateYUVTexture(data->yuv, rect, pixels, pitch) < 0) {
            return -1;
        }
        UpdateYUVTextureData(texture);
        return 0;
    } else {
        X11_RenderData *renderdata = (X11_RenderData *) renderer->driverdata;

        if (data->pixels) {
            Uint8 *src, *dst;
            int row;
            size_t length;

            src = (Uint8 *) pixels;
            dst =
                (Uint8 *) data->pixels + rect->y * data->pitch +
                rect->x * SDL_BYTESPERPIXEL(texture->format);
            length = rect->w * SDL_BYTESPERPIXEL(texture->format);
            for (row = 0; row < rect->h; ++row) {
                SDL_memcpy(dst, src, length);
                src += pitch;
                dst += data->pitch;
            }
        } else {
            data->image->width = rect->w;
            data->image->height = rect->h;
            data->image->data = (char *) pixels;
            data->image->bytes_per_line = pitch;
            XPutImage(renderdata->display, data->pixmap, renderdata->gc,
                      data->image, 0, 0, rect->x, rect->y, rect->w, rect->h);
        }
        return 0;
    }
}

static int
X11_LockTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                const SDL_Rect * rect, int markDirty, void **pixels,
                int *pitch)
{
    X11_TextureData *data = (X11_TextureData *) texture->driverdata;

    if (data->yuv) {
        return SDL_SW_LockYUVTexture(data->yuv, rect, markDirty, pixels,
                                     pitch);
    } else if (data->pixels) {
        *pixels =
            (void *) ((Uint8 *) data->pixels + rect->y * data->pitch +
                      rect->x * SDL_BYTESPERPIXEL(texture->format));
        *pitch = data->pitch;
        return 0;
    } else {
        SDL_SetError("No pixels available");
        return -1;
    }
}

static void
X11_UnlockTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    X11_TextureData *data = (X11_TextureData *) texture->driverdata;

    if (data->yuv) {
        SDL_SW_UnlockYUVTexture(data->yuv);
        UpdateYUVTextureData(texture);
    }
}

static int
X11_SetDrawBlendMode(SDL_Renderer * renderer)
{
    switch (renderer->blendMode) {
    case SDL_BLENDMODE_NONE:
        return 0;
    default:
        SDL_Unsupported();
        renderer->blendMode = SDL_BLENDMODE_NONE;
        return -1;
    }
}

static Uint32
renderdrawcolor(SDL_Renderer * renderer, int premult)
{
    X11_RenderData *data = (X11_RenderData *) renderer->driverdata;
    Uint8 r = renderer->r;
    Uint8 g = renderer->g;
    Uint8 b = renderer->b;
    Uint8 a = renderer->a;
    if (premult)
        return SDL_MapRGBA(&data->format, ((int) r * (int) a) / 255,
                           ((int) g * (int) a) / 255,
                           ((int) b * (int) a) / 255, 255);
    else
        return SDL_MapRGBA(&data->format, r, g, b, a);
}

static int
X11_RenderDrawPoints(SDL_Renderer * renderer, const SDL_Point * points,
                     int count)
{
    X11_RenderData *data = (X11_RenderData *) renderer->driverdata;
    SDL_Window *window = renderer->window;
    unsigned long foreground;
    XPoint *xpoints, *xpoint;
    int i, xcount;

    if (data->makedirty) {
        SDL_Rect rect;

        /* Get the smallest rectangle that contains everything */
        rect.x = 0;
        rect.y = 0;
        rect.w = window->w;
        rect.h = window->h;
        if (!SDL_EnclosePoints(points, count, &rect, &rect)) {
            /* Nothing to draw */
            return 0;
        }
        SDL_AddDirtyRect(&data->dirty, &rect);
    }

    foreground = renderdrawcolor(renderer, 1);
    XSetForeground(data->display, data->gc, foreground);

    xpoint = xpoints = SDL_stack_alloc(XPoint, count);
    xcount = 0;
    for (i = 0; i < count; ++i) {
        int x = points[i].x;
        int y = points[i].y;
        if (x < 0 || x >= window->w || y < 0 || y >= window->h) {
            continue;
        }
        xpoint->x = (short)x;
        xpoint->y = (short)y;
        ++xpoint;
        ++xcount;
    }
    if (xcount > 0) {
        XDrawPoints(data->display, data->drawable, data->gc, xpoints, xcount,
                    CoordModeOrigin);
    }
    SDL_stack_free(xpoints);

    return 0;
}

static int
X11_RenderDrawLines(SDL_Renderer * renderer, const SDL_Point * points,
                    int count)
{
    X11_RenderData *data = (X11_RenderData *) renderer->driverdata;
    SDL_Window *window = renderer->window;
    SDL_Rect clip;
    unsigned long foreground;
    XPoint *xpoints, *xpoint;
    int i, xcount;
    int minx, miny;
    int maxx, maxy;

    clip.x = 0;
    clip.y = 0;
    clip.w = window->w;
    clip.h = window->h;

    foreground = renderdrawcolor(renderer, 1);
    XSetForeground(data->display, data->gc, foreground);

    xpoint = xpoints = SDL_stack_alloc(XPoint, count);
    xcount = 0;
    minx = INT_MAX;
    miny = INT_MAX;
    maxx = INT_MIN;
    maxy = INT_MIN;
    for (i = 0; i < count; ++i) {
        int x = points[i].x;
        int y = points[i].y;

        /* If the point is inside the window, add it to the list */
        if (x >= 0 && x < window->w && y >= 0 && y < window->h) {
            if (x < minx) {
                minx = x;
            } else if (x > maxx) {
                maxx = x;
            }
            if (y < miny) {
                miny = y;
            } else if (y > maxy) {
                maxy = y;
            }
            xpoint->x = (short)x;
            xpoint->y = (short)y;
            ++xpoint;
            ++xcount;
            continue;
        }

        /* We need to clip the line segments joined by this point */
        if (xcount > 0) {
            int x1 = xpoint[-1].x;
            int y1 = xpoint[-1].y;
            int x2 = x;
            int y2 = y;
            if (SDL_IntersectRectAndLine(&clip, &x1, &y1, &x2, &y2)) {
                if (x2 < minx) {
                    minx = x2;
                } else if (x2 > maxx) {
                    maxx = x2;
                }
                if (y2 < miny) {
                    miny = y2;
                } else if (y2 > maxy) {
                    maxy = y2;
                }
                xpoint->x = (short)x2;
                xpoint->y = (short)y2;
                ++xpoint;
                ++xcount;
            }
            XDrawLines(data->display, data->drawable, data->gc,
                       xpoints, xcount, CoordModeOrigin);
            if (xpoints[0].x != x2 || xpoints[0].y != y2) {
                XDrawPoint(data->display, data->drawable, data->gc, x2, y2);
            }
            if (data->makedirty) {
                SDL_Rect rect;

                rect.x = minx;
                rect.y = miny;
                rect.w = (maxx - minx) + 1;
                rect.h = (maxy - miny) + 1;
                SDL_AddDirtyRect(&data->dirty, &rect);
            }
            xpoint = xpoints;
            xcount = 0;
            minx = INT_MAX;
            miny = INT_MAX;
            maxx = INT_MIN;
            maxy = INT_MIN;
        }
        if (i < (count-1)) {
            int x1 = x;
            int y1 = y;
            int x2 = points[i+1].x;
            int y2 = points[i+1].y;
            if (SDL_IntersectRectAndLine(&clip, &x1, &y1, &x2, &y2)) {
                if (x1 < minx) {
                    minx = x1;
                } else if (x1 > maxx) {
                    maxx = x1;
                }
                if (y1 < miny) {
                    miny = y1;
                } else if (y1 > maxy) {
                    maxy = y1;
                }
                xpoint->x = (short)x1;
                xpoint->y = (short)y1;
                ++xpoint;
                ++xcount;
            }
        }
    }
    if (xcount > 1) {
        int x2 = xpoint[-1].x;
        int y2 = xpoint[-1].y;
        XDrawLines(data->display, data->drawable, data->gc, xpoints, xcount,
                   CoordModeOrigin);
        if (xpoints[0].x != x2 || xpoints[0].y != y2) {
            XDrawPoint(data->display, data->drawable, data->gc, x2, y2);
        }
        if (data->makedirty) {
            SDL_Rect rect;

            rect.x = minx;
            rect.y = miny;
            rect.w = (maxx - minx) + 1;
            rect.h = (maxy - miny) + 1;
            SDL_AddDirtyRect(&data->dirty, &rect);
        }
    }
    SDL_stack_free(xpoints);

    return 0;
}

static int
X11_RenderDrawRects(SDL_Renderer * renderer, const SDL_Rect ** rects, int count)
{
    X11_RenderData *data = (X11_RenderData *) renderer->driverdata;
    SDL_Window *window = renderer->window;
    SDL_Rect clip, rect;
    unsigned long foreground;
    XRectangle *xrects, *xrect;
    int i, xcount;

    clip.x = 0;
    clip.y = 0;
    clip.w = window->w;
    clip.h = window->h;

    foreground = renderdrawcolor(renderer, 1);
    XSetForeground(data->display, data->gc, foreground);

    xrect = xrects = SDL_stack_alloc(XRectangle, count);
    xcount = 0;
    for (i = 0; i < count; ++i) {
        if (!SDL_IntersectRect(rects[i], &clip, &rect)) {
            continue;
        }

        xrect->x = (short)rect.x;
        xrect->y = (short)rect.y;
        xrect->width = (unsigned short)rect.w;
        xrect->height = (unsigned short)rect.h;
        ++xrect;
        ++xcount;

        if (data->makedirty) {
            SDL_AddDirtyRect(&data->dirty, &rect);
        }
    }
    if (xcount > 0) {
        XDrawRectangles(data->display, data->drawable, data->gc,
                        xrects, xcount);
    }
    SDL_stack_free(xpoints);

    return 0;
}

static int
X11_RenderFillRects(SDL_Renderer * renderer, const SDL_Rect ** rects, int count)
{
    X11_RenderData *data = (X11_RenderData *) renderer->driverdata;
    SDL_Window *window = renderer->window;
    SDL_Rect clip, rect;
    unsigned long foreground;
    XRectangle *xrects, *xrect;
    int i, xcount;

    clip.x = 0;
    clip.y = 0;
    clip.w = window->w;
    clip.h = window->h;

    foreground = renderdrawcolor(renderer, 1);
    XSetForeground(data->display, data->gc, foreground);

    xrect = xrects = SDL_stack_alloc(XRectangle, count);
    xcount = 0;
    for (i = 0; i < count; ++i) {
        if (!SDL_IntersectRect(rects[i], &clip, &rect)) {
            continue;
        }

        xrect->x = (short)rect.x;
        xrect->y = (short)rect.y;
        xrect->width = (unsigned short)rect.w;
        xrect->height = (unsigned short)rect.h;
        ++xrect;
        ++xcount;

        if (data->makedirty) {
            SDL_AddDirtyRect(&data->dirty, &rect);
        }
    }
    if (xcount > 0) {
        XFillRectangles(data->display, data->drawable, data->gc,
                        xrects, xcount);
    }
    SDL_stack_free(xpoints);

    return 0;
}

static int
X11_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
               const SDL_Rect * srcrect, const SDL_Rect * dstrect)
{
    X11_RenderData *data = (X11_RenderData *) renderer->driverdata;
    X11_TextureData *texturedata = (X11_TextureData *) texture->driverdata;

    if (data->makedirty) {
        SDL_AddDirtyRect(&data->dirty, dstrect);
    }
    if (srcrect->w == dstrect->w && srcrect->h == dstrect->h) {
#ifndef NO_SHARED_MEMORY
        if (texturedata->shminfo.shmaddr) {
            XShmPutImage(data->display, data->drawable, data->gc,
                         texturedata->image, srcrect->x, srcrect->y,
                         dstrect->x, dstrect->y, srcrect->w, srcrect->h,
                         False);
        } else
#endif
        if (texturedata->pixels) {
            XPutImage(data->display, data->drawable, data->gc,
                      texturedata->image, srcrect->x, srcrect->y, dstrect->x,
                      dstrect->y, srcrect->w, srcrect->h);
        } else {
            XCopyArea(data->display, texturedata->pixmap, data->drawable,
                      data->gc, srcrect->x, srcrect->y, dstrect->w,
                      dstrect->h, dstrect->x, dstrect->y);
        }
    } else if (texturedata->yuv
               || texture->access == SDL_TEXTUREACCESS_STREAMING) {
        SDL_Surface src, dst;
        SDL_PixelFormat fmt;
        SDL_Rect rect;
        XImage *image = texturedata->scaling_image;

        if (!image) {
            void *pixels;
            int pitch;

            pitch = dstrect->w * SDL_BYTESPERPIXEL(texturedata->format);
            pixels = SDL_malloc(dstrect->h * pitch);
            if (!pixels) {
                SDL_OutOfMemory();
                return -1;
            }

            image =
                XCreateImage(data->display, data->visual, data->depth,
                             ZPixmap, 0, pixels, dstrect->w, dstrect->h,
                             SDL_BYTESPERPIXEL(texturedata->format) * 8,
                             pitch);
            if (!image) {
                SDL_SetError("XCreateImage() failed");
                return -1;
            }
            texturedata->scaling_image = image;

        } else if (image->width != dstrect->w || image->height != dstrect->h
                   || !image->data) {
            image->width = dstrect->w;
            image->height = dstrect->h;
            image->bytes_per_line =
                image->width * SDL_BYTESPERPIXEL(texturedata->format);
            image->data =
                (char *) SDL_realloc(image->data,
                                     image->height * image->bytes_per_line);
            if (!image->data) {
                SDL_OutOfMemory();
                return -1;
            }
        }

        /* Set up fake surfaces for SDL_SoftStretch() */
        SDL_zero(src);
        src.format = &fmt;
        src.w = texture->w;
        src.h = texture->h;
#ifndef NO_SHARED_MEMORY
        if (texturedata->shminfo.shmaddr) {
            src.pixels = texturedata->shminfo.shmaddr;
        } else
#endif
            src.pixels = texturedata->pixels;
        src.pitch = texturedata->pitch;

        SDL_zero(dst);
        dst.format = &fmt;
        dst.w = image->width;
        dst.h = image->height;
        dst.pixels = image->data;
        dst.pitch = image->bytes_per_line;

        fmt.BytesPerPixel = SDL_BYTESPERPIXEL(texturedata->format);

        rect.x = 0;
        rect.y = 0;
        rect.w = dstrect->w;
        rect.h = dstrect->h;
        if (SDL_SoftStretch(&src, srcrect, &dst, &rect) < 0) {
            return -1;
        }
        XPutImage(data->display, data->drawable, data->gc, image, 0, 0,
                  dstrect->x, dstrect->y, dstrect->w, dstrect->h);
    } else {
        XCopyArea(data->display, texturedata->pixmap, data->drawable,
                  data->gc, srcrect->x, srcrect->y, dstrect->w, dstrect->h,
                  srcrect->x, srcrect->y);
    }
    return 0;
}

static int
X11_RenderReadPixels(SDL_Renderer * renderer, const SDL_Rect * rect,
                     Uint32 format, void * pixels, int pitch)
{
    X11_RenderData *data = (X11_RenderData *) renderer->driverdata;
    SDL_Window *window = renderer->window;
    SDL_VideoDisplay *display = window->display;
    Uint32 screen_format = display->current_mode.format;
    XImage *image;

    image = XGetImage(data->display, data->drawable, rect->x, rect->y,
                      rect->w, rect->h, AllPlanes, ZPixmap);

    SDL_ConvertPixels(rect->w, rect->h,
                      screen_format, image->data, image->bytes_per_line,
                      format, pixels, pitch);

    XDestroyImage(image);
    return 0;
}

static int
X11_RenderWritePixels(SDL_Renderer * renderer, const SDL_Rect * rect,
                      Uint32 format, const void * pixels, int pitch)
{
    X11_RenderData *data = (X11_RenderData *) renderer->driverdata;
    SDL_Window *window = renderer->window;
    SDL_VideoDisplay *display = window->display;
    Uint32 screen_format = display->current_mode.format;
    XImage *image;
    void *image_pixels;
    int image_pitch;

    image_pitch = rect->w * SDL_BYTESPERPIXEL(screen_format);
    image_pixels = SDL_malloc(rect->h * image_pitch);
    if (!image_pixels) {
        SDL_OutOfMemory();
        return -1;
    }

    image = XCreateImage(data->display, data->visual,
                         data->depth, ZPixmap, 0, image_pixels,
                         rect->w, rect->h,
                         SDL_BYTESPERPIXEL(screen_format) * 8,
                         image_pitch);
    if (!image) {
        SDL_SetError("XCreateImage() failed");
        return -1;
    }

    SDL_ConvertPixels(rect->w, rect->h,
                      format, pixels, pitch,
                      screen_format, image->data, image->bytes_per_line);

    XPutImage(data->display, data->drawable, data->gc,
              image, 0, 0, rect->x, rect->y, rect->w, rect->h);

    image->data = NULL;
    XDestroyImage(image);

    SDL_free(image_pixels);
    return 0;
}

static void
X11_RenderPresent(SDL_Renderer * renderer)
{
    X11_RenderData *data = (X11_RenderData *) renderer->driverdata;
    SDL_DirtyRect *dirty;

    /* Send the data to the display */
    if (!(renderer->info.flags & SDL_RENDERER_SINGLEBUFFER)) {
        for (dirty = data->dirty.list; dirty; dirty = dirty->next) {
            const SDL_Rect *rect = &dirty->rect;
            XCopyArea(data->display, data->drawable, data->xwindow,
                      data->gc, rect->x, rect->y, rect->w, rect->h,
                      rect->x, rect->y);
        }
        SDL_ClearDirtyRects(&data->dirty);
    }
    XSync(data->display, False);

    /* Update the flipping chain, if any */
    if (renderer->info.flags & SDL_RENDERER_PRESENTFLIP2) {
        data->current_pixmap = (data->current_pixmap + 1) % 2;
        data->drawable = data->pixmaps[data->current_pixmap];
    } else if (renderer->info.flags & SDL_RENDERER_PRESENTFLIP3) {
        data->current_pixmap = (data->current_pixmap + 1) % 3;
        data->drawable = data->pixmaps[data->current_pixmap];
    }
}

static void
X11_DestroyTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    X11_RenderData *renderdata = (X11_RenderData *) renderer->driverdata;
    X11_TextureData *data = (X11_TextureData *) texture->driverdata;

    if (!data) {
        return;
    }
    if (data->yuv) {
        SDL_SW_DestroyYUVTexture(data->yuv);
    }
    if (data->pixmap != None) {
        XFreePixmap(renderdata->display, data->pixmap);
    }
    if (data->image) {
        data->image->data = NULL;
        XDestroyImage(data->image);
    }
#ifndef NO_SHARED_MEMORY
    if (data->shminfo.shmaddr) {
        XShmDetach(renderdata->display, &data->shminfo);
        XSync(renderdata->display, False);
        shmdt(data->shminfo.shmaddr);
        data->pixels = NULL;
    }
#endif
    if (data->scaling_image) {
        SDL_free(data->scaling_image->data);
        data->scaling_image->data = NULL;
        XDestroyImage(data->scaling_image);
    }
    if (data->pixels) {
        SDL_free(data->pixels);
    }
    SDL_free(data);
    texture->driverdata = NULL;
}

static void
X11_DestroyRenderer(SDL_Renderer * renderer)
{
    X11_RenderData *data = (X11_RenderData *) renderer->driverdata;
    int i;

    if (data) {
        for (i = 0; i < SDL_arraysize(data->pixmaps); ++i) {
            if (data->pixmaps[i] != None) {
                XFreePixmap(data->display, data->pixmaps[i]);
            }
        }
        if (data->gc) {
            XFreeGC(data->display, data->gc);
        }
        SDL_FreeDirtyRects(&data->dirty);
        SDL_free(data);
    }
    SDL_free(renderer);
}

#endif /* SDL_VIDEO_RENDER_X11 */

/* vi: set ts=4 sw=4 expandtab: */
