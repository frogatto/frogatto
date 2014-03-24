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

#if SDL_VIDEO_RENDER_D3D

#include "SDL_win32video.h"
#include "../SDL_yuv_sw_c.h"

#ifdef ASSEMBLE_SHADER
///////////////////////////////////////////////////////////////////////////
// ID3DXBuffer:
// ------------
// The buffer object is used by D3DX to return arbitrary size data.
//
// GetBufferPointer -
//    Returns a pointer to the beginning of the buffer.
//
// GetBufferSize -
//    Returns the size of the buffer, in bytes.
///////////////////////////////////////////////////////////////////////////

typedef interface ID3DXBuffer ID3DXBuffer;
typedef interface ID3DXBuffer *LPD3DXBUFFER;

// {8BA5FB08-5195-40e2-AC58-0D989C3A0102}
DEFINE_GUID(IID_ID3DXBuffer, 
0x8ba5fb08, 0x5195, 0x40e2, 0xac, 0x58, 0xd, 0x98, 0x9c, 0x3a, 0x1, 0x2);

#undef INTERFACE
#define INTERFACE ID3DXBuffer

typedef interface ID3DXBuffer {
    const struct ID3DXBufferVtbl FAR* lpVtbl;
} ID3DXBuffer;
typedef const struct ID3DXBufferVtbl ID3DXBufferVtbl;
const struct ID3DXBufferVtbl
{
    // IUnknown
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // ID3DXBuffer
    STDMETHOD_(LPVOID, GetBufferPointer)(THIS) PURE;
    STDMETHOD_(DWORD, GetBufferSize)(THIS) PURE;
};

HRESULT WINAPI
    D3DXAssembleShader(
        LPCSTR                          pSrcData,
        UINT                            SrcDataLen,
        CONST LPVOID*                   pDefines,
        LPVOID                          pInclude,
        DWORD                           Flags,
        LPD3DXBUFFER*                   ppShader,
        LPD3DXBUFFER*                   ppErrorMsgs);

#endif /* ASSEMBLE_SHADER */


/* Direct3D renderer implementation */

#if 1                           /* This takes more memory but you won't lose your texture data */
#define D3DPOOL_SDL    D3DPOOL_MANAGED
#define SDL_MEMORY_POOL_MANAGED
#else
#define D3DPOOL_SDL    D3DPOOL_DEFAULT
#define SDL_MEMORY_POOL_DEFAULT
#endif

static SDL_Renderer *D3D_CreateRenderer(SDL_Window * window, Uint32 flags);
static int D3D_DisplayModeChanged(SDL_Renderer * renderer);
static int D3D_CreateTexture(SDL_Renderer * renderer, SDL_Texture * texture);
static int D3D_QueryTexturePixels(SDL_Renderer * renderer,
                                  SDL_Texture * texture, void **pixels,
                                  int *pitch);
static int D3D_SetTexturePalette(SDL_Renderer * renderer,
                                 SDL_Texture * texture,
                                 const SDL_Color * colors, int firstcolor,
                                 int ncolors);
static int D3D_GetTexturePalette(SDL_Renderer * renderer,
                                 SDL_Texture * texture, SDL_Color * colors,
                                 int firstcolor, int ncolors);
static int D3D_SetTextureColorMod(SDL_Renderer * renderer,
                                  SDL_Texture * texture);
static int D3D_SetTextureAlphaMod(SDL_Renderer * renderer,
                                  SDL_Texture * texture);
static int D3D_SetTextureBlendMode(SDL_Renderer * renderer,
                                   SDL_Texture * texture);
static int D3D_SetTextureScaleMode(SDL_Renderer * renderer,
                                   SDL_Texture * texture);
static int D3D_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                             const SDL_Rect * rect, const void *pixels,
                             int pitch);
static int D3D_LockTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                           const SDL_Rect * rect, int markDirty,
                           void **pixels, int *pitch);
static void D3D_UnlockTexture(SDL_Renderer * renderer, SDL_Texture * texture);
static void D3D_DirtyTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                             int numrects, const SDL_Rect * rects);
static int D3D_RenderDrawPoints(SDL_Renderer * renderer,
                                const SDL_Point * points, int count);
static int D3D_RenderDrawLines(SDL_Renderer * renderer,
                               const SDL_Point * points, int count);
static int D3D_RenderDrawRects(SDL_Renderer * renderer,
                               const SDL_Rect ** rects, int count);
static int D3D_RenderFillRects(SDL_Renderer * renderer,
                               const SDL_Rect ** rects, int count);
static int D3D_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
                          const SDL_Rect * srcrect, const SDL_Rect * dstrect);
static int D3D_RenderReadPixels(SDL_Renderer * renderer, const SDL_Rect * rect,
                                Uint32 format, void * pixels, int pitch);
static int D3D_RenderWritePixels(SDL_Renderer * renderer, const SDL_Rect * rect,
                                 Uint32 format, const void * pixels, int pitch);
static void D3D_RenderPresent(SDL_Renderer * renderer);
static void D3D_DestroyTexture(SDL_Renderer * renderer,
                               SDL_Texture * texture);
static void D3D_DestroyRenderer(SDL_Renderer * renderer);


SDL_RenderDriver D3D_RenderDriver = {
    D3D_CreateRenderer,
    {
     "d3d",
     (SDL_RENDERER_SINGLEBUFFER | SDL_RENDERER_PRESENTCOPY |
      SDL_RENDERER_PRESENTFLIP2 | SDL_RENDERER_PRESENTFLIP3 |
      SDL_RENDERER_PRESENTDISCARD | SDL_RENDERER_PRESENTVSYNC |
      SDL_RENDERER_ACCELERATED),
     (SDL_TEXTUREMODULATE_NONE | SDL_TEXTUREMODULATE_COLOR |
      SDL_TEXTUREMODULATE_ALPHA),
     (SDL_BLENDMODE_NONE | SDL_BLENDMODE_MASK |
      SDL_BLENDMODE_BLEND | SDL_BLENDMODE_ADD | SDL_BLENDMODE_MOD),
     (SDL_TEXTURESCALEMODE_NONE | SDL_TEXTURESCALEMODE_FAST |
      SDL_TEXTURESCALEMODE_SLOW | SDL_TEXTURESCALEMODE_BEST),
     0,
     {0},
     0,
     0}
};

typedef struct
{
    IDirect3D9 *d3d;
    IDirect3DDevice9 *device;
    UINT adapter;
    D3DPRESENT_PARAMETERS pparams;
    LPDIRECT3DPIXELSHADER9 ps_mask;
    SDL_bool beginScene;
} D3D_RenderData;

typedef struct
{
    SDL_SW_YUVTexture *yuv;
    Uint32 format;
    IDirect3DTexture9 *texture;
} D3D_TextureData;

typedef struct
{
    float x, y, z;
    float rhw;
    DWORD color;
    float u, v;
} Vertex;

static void
D3D_SetError(const char *prefix, HRESULT result)
{
    const char *error;

    switch (result) {
    case D3DERR_WRONGTEXTUREFORMAT:
        error = "WRONGTEXTUREFORMAT";
        break;
    case D3DERR_UNSUPPORTEDCOLOROPERATION:
        error = "UNSUPPORTEDCOLOROPERATION";
        break;
    case D3DERR_UNSUPPORTEDCOLORARG:
        error = "UNSUPPORTEDCOLORARG";
        break;
    case D3DERR_UNSUPPORTEDALPHAOPERATION:
        error = "UNSUPPORTEDALPHAOPERATION";
        break;
    case D3DERR_UNSUPPORTEDALPHAARG:
        error = "UNSUPPORTEDALPHAARG";
        break;
    case D3DERR_TOOMANYOPERATIONS:
        error = "TOOMANYOPERATIONS";
        break;
    case D3DERR_CONFLICTINGTEXTUREFILTER:
        error = "CONFLICTINGTEXTUREFILTER";
        break;
    case D3DERR_UNSUPPORTEDFACTORVALUE:
        error = "UNSUPPORTEDFACTORVALUE";
        break;
    case D3DERR_CONFLICTINGRENDERSTATE:
        error = "CONFLICTINGRENDERSTATE";
        break;
    case D3DERR_UNSUPPORTEDTEXTUREFILTER:
        error = "UNSUPPORTEDTEXTUREFILTER";
        break;
    case D3DERR_CONFLICTINGTEXTUREPALETTE:
        error = "CONFLICTINGTEXTUREPALETTE";
        break;
    case D3DERR_DRIVERINTERNALERROR:
        error = "DRIVERINTERNALERROR";
        break;
    case D3DERR_NOTFOUND:
        error = "NOTFOUND";
        break;
    case D3DERR_MOREDATA:
        error = "MOREDATA";
        break;
    case D3DERR_DEVICELOST:
        error = "DEVICELOST";
        break;
    case D3DERR_DEVICENOTRESET:
        error = "DEVICENOTRESET";
        break;
    case D3DERR_NOTAVAILABLE:
        error = "NOTAVAILABLE";
        break;
    case D3DERR_OUTOFVIDEOMEMORY:
        error = "OUTOFVIDEOMEMORY";
        break;
    case D3DERR_INVALIDDEVICE:
        error = "INVALIDDEVICE";
        break;
    case D3DERR_INVALIDCALL:
        error = "INVALIDCALL";
        break;
    case D3DERR_DRIVERINVALIDCALL:
        error = "DRIVERINVALIDCALL";
        break;
    case D3DERR_WASSTILLDRAWING:
        error = "WASSTILLDRAWING";
        break;
    default:
        error = "UNKNOWN";
        break;
    }
    SDL_SetError("%s: %s", prefix, error);
}

static D3DFORMAT
PixelFormatToD3DFMT(Uint32 format)
{
    switch (format) {
    case SDL_PIXELFORMAT_INDEX8:
        return D3DFMT_P8;
    case SDL_PIXELFORMAT_RGB332:
        return D3DFMT_R3G3B2;
    case SDL_PIXELFORMAT_RGB444:
        return D3DFMT_X4R4G4B4;
    case SDL_PIXELFORMAT_RGB555:
        return D3DFMT_X1R5G5B5;
    case SDL_PIXELFORMAT_ARGB4444:
        return D3DFMT_A4R4G4B4;
    case SDL_PIXELFORMAT_ARGB1555:
        return D3DFMT_A1R5G5B5;
    case SDL_PIXELFORMAT_RGB565:
        return D3DFMT_R5G6B5;
    case SDL_PIXELFORMAT_RGB888:
        return D3DFMT_X8R8G8B8;
    case SDL_PIXELFORMAT_ARGB8888:
        return D3DFMT_A8R8G8B8;
    case SDL_PIXELFORMAT_ARGB2101010:
        return D3DFMT_A2R10G10B10;
    case SDL_PIXELFORMAT_YV12:
        return MAKEFOURCC('Y','V','1','2');
    case SDL_PIXELFORMAT_IYUV:
        return MAKEFOURCC('I','4','2','0');
    case SDL_PIXELFORMAT_UYVY:
        return D3DFMT_UYVY;
    case SDL_PIXELFORMAT_YUY2:
        return D3DFMT_YUY2;
    default:
        return D3DFMT_UNKNOWN;
    }
}

static UINT D3D_FindAdapter(IDirect3D9 * d3d, SDL_VideoDisplay * display)
{
    SDL_DisplayData *displaydata = (SDL_DisplayData *) display->driverdata;
    UINT adapter, count;

    count = IDirect3D9_GetAdapterCount(d3d);
    for (adapter = 0; adapter < count; ++adapter) {
        HRESULT result;
        D3DADAPTER_IDENTIFIER9 info;
        char *name;

        result = IDirect3D9_GetAdapterIdentifier(d3d, adapter, 0, &info);
        if (FAILED(result)) {
            continue;
        }
        name = WIN_StringToUTF8(displaydata->DeviceName);
        if (SDL_strcmp(name, info.DeviceName) == 0) {
            SDL_free(name);
            return adapter;
        }
        SDL_free(name);
    }

    /* This should never happen, but just in case... */
    return D3DADAPTER_DEFAULT;
}

static SDL_bool
D3D_IsTextureFormatAvailable(IDirect3D9 * d3d, UINT adapter,
                             Uint32 display_format,
                             Uint32 texture_format)
{
    HRESULT result;

    result = IDirect3D9_CheckDeviceFormat(d3d, adapter,
                                          D3DDEVTYPE_HAL,
                                          PixelFormatToD3DFMT(display_format),
                                          0,
                                          D3DRTYPE_TEXTURE,
                                          PixelFormatToD3DFMT
                                          (texture_format));
    return FAILED(result) ? SDL_FALSE : SDL_TRUE;
}

static void
UpdateYUVTextureData(SDL_Texture * texture)
{
    D3D_TextureData *data = (D3D_TextureData *) texture->driverdata;
    SDL_Rect rect;
    RECT d3drect;
    D3DLOCKED_RECT locked;
    HRESULT result;

    d3drect.left = 0;
    d3drect.right = texture->w;
    d3drect.top = 0;
    d3drect.bottom = texture->h;

    result =
        IDirect3DTexture9_LockRect(data->texture, 0, &locked, &d3drect, 0);
    if (FAILED(result)) {
        return;
    }

    rect.x = 0;
    rect.y = 0;
    rect.w = texture->w;
    rect.h = texture->h;
    SDL_SW_CopyYUVToRGB(data->yuv, &rect, data->format, texture->w,
                        texture->h, locked.pBits, locked.Pitch);

    IDirect3DTexture9_UnlockRect(data->texture, 0);
}

void
D3D_AddRenderDriver(_THIS)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;
    SDL_RendererInfo *info = &D3D_RenderDriver.info;

    if (data->d3d) {
        int i, j;
        int formats[] = {
            SDL_PIXELFORMAT_INDEX8,
            SDL_PIXELFORMAT_RGB332,
            SDL_PIXELFORMAT_RGB444,
            SDL_PIXELFORMAT_RGB555,
            SDL_PIXELFORMAT_ARGB4444,
            SDL_PIXELFORMAT_ARGB1555,
            SDL_PIXELFORMAT_RGB565,
            SDL_PIXELFORMAT_RGB888,
            SDL_PIXELFORMAT_ARGB8888,
            SDL_PIXELFORMAT_ARGB2101010,
        };

        for (i = 0; i < _this->num_displays; ++i) {
            SDL_VideoDisplay *display = &_this->displays[i];
            SDL_DisplayMode *mode = &display->desktop_mode;
            UINT adapter = D3D_FindAdapter(data->d3d, display);

            /* Get the matching D3D adapter for this display */
            info->num_texture_formats = 0;
            for (j = 0; j < SDL_arraysize(formats); ++j) {
                if (D3D_IsTextureFormatAvailable
                    (data->d3d, adapter, mode->format, formats[j])) {
                    info->texture_formats[info->num_texture_formats++] =
                        formats[j];
                }
            }
            info->texture_formats[info->num_texture_formats++] =
                SDL_PIXELFORMAT_YV12;
            info->texture_formats[info->num_texture_formats++] =
                SDL_PIXELFORMAT_IYUV;
            info->texture_formats[info->num_texture_formats++] =
                SDL_PIXELFORMAT_YUY2;
            info->texture_formats[info->num_texture_formats++] =
                SDL_PIXELFORMAT_UYVY;
            info->texture_formats[info->num_texture_formats++] =
                SDL_PIXELFORMAT_YVYU;

            SDL_AddRenderDriver(display, &D3D_RenderDriver);
        }
    }
}

SDL_Renderer *
D3D_CreateRenderer(SDL_Window * window, Uint32 flags)
{
    SDL_VideoDisplay *display = window->display;
    SDL_VideoData *videodata = (SDL_VideoData *) display->device->driverdata;
    SDL_WindowData *windowdata = (SDL_WindowData *) window->driverdata;
    SDL_Renderer *renderer;
    D3D_RenderData *data;
    HRESULT result;
    D3DPRESENT_PARAMETERS pparams;
    IDirect3DSwapChain9 *chain;
    D3DCAPS9 caps;

    renderer = (SDL_Renderer *) SDL_calloc(1, sizeof(*renderer));
    if (!renderer) {
        SDL_OutOfMemory();
        return NULL;
    }

    data = (D3D_RenderData *) SDL_calloc(1, sizeof(*data));
    if (!data) {
        D3D_DestroyRenderer(renderer);
        SDL_OutOfMemory();
        return NULL;
    }
    data->d3d = videodata->d3d;

    renderer->DisplayModeChanged = D3D_DisplayModeChanged;
    renderer->CreateTexture = D3D_CreateTexture;
    renderer->QueryTexturePixels = D3D_QueryTexturePixels;
    renderer->SetTexturePalette = D3D_SetTexturePalette;
    renderer->GetTexturePalette = D3D_GetTexturePalette;
    renderer->SetTextureColorMod = D3D_SetTextureColorMod;
    renderer->SetTextureAlphaMod = D3D_SetTextureAlphaMod;
    renderer->SetTextureBlendMode = D3D_SetTextureBlendMode;
    renderer->SetTextureScaleMode = D3D_SetTextureScaleMode;
    renderer->UpdateTexture = D3D_UpdateTexture;
    renderer->LockTexture = D3D_LockTexture;
    renderer->UnlockTexture = D3D_UnlockTexture;
    renderer->DirtyTexture = D3D_DirtyTexture;
    renderer->RenderDrawPoints = D3D_RenderDrawPoints;
    renderer->RenderDrawLines = D3D_RenderDrawLines;
    renderer->RenderDrawRects = D3D_RenderDrawRects;
    renderer->RenderFillRects = D3D_RenderFillRects;
    renderer->RenderCopy = D3D_RenderCopy;
    renderer->RenderReadPixels = D3D_RenderReadPixels;
    renderer->RenderWritePixels = D3D_RenderWritePixels;
    renderer->RenderPresent = D3D_RenderPresent;
    renderer->DestroyTexture = D3D_DestroyTexture;
    renderer->DestroyRenderer = D3D_DestroyRenderer;
    renderer->info = D3D_RenderDriver.info;
    renderer->window = window;
    renderer->driverdata = data;

    renderer->info.flags = SDL_RENDERER_ACCELERATED;

    SDL_zero(pparams);
    pparams.BackBufferWidth = window->w;
    pparams.BackBufferHeight = window->h;
    if (window->flags & SDL_WINDOW_FULLSCREEN) {
        pparams.BackBufferFormat =
            PixelFormatToD3DFMT(window->fullscreen_mode.format);
    } else {
        pparams.BackBufferFormat = D3DFMT_UNKNOWN;
    }
    if (flags & SDL_RENDERER_PRESENTFLIP2) {
        pparams.BackBufferCount = 2;
        pparams.SwapEffect = D3DSWAPEFFECT_FLIP;
    } else if (flags & SDL_RENDERER_PRESENTFLIP3) {
        pparams.BackBufferCount = 3;
        pparams.SwapEffect = D3DSWAPEFFECT_FLIP;
    } else if (flags & SDL_RENDERER_PRESENTCOPY) {
        pparams.BackBufferCount = 1;
        pparams.SwapEffect = D3DSWAPEFFECT_COPY;
    } else {
        pparams.BackBufferCount = 1;
        pparams.SwapEffect = D3DSWAPEFFECT_DISCARD;
    }
    if (window->flags & SDL_WINDOW_FULLSCREEN) {
        pparams.Windowed = FALSE;
        pparams.FullScreen_RefreshRateInHz =
            window->fullscreen_mode.refresh_rate;
    } else {
        pparams.Windowed = TRUE;
        pparams.FullScreen_RefreshRateInHz = 0;
    }
    if (flags & SDL_RENDERER_PRESENTVSYNC) {
        pparams.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    } else {
        pparams.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    }

    data->adapter = D3D_FindAdapter(videodata->d3d, display);
    IDirect3D9_GetDeviceCaps(videodata->d3d, data->adapter,
                             D3DDEVTYPE_HAL, &caps);

    result = IDirect3D9_CreateDevice(videodata->d3d, data->adapter,
                                     D3DDEVTYPE_HAL,
                                     windowdata->hwnd,
                                     (caps.
                                      DevCaps &
                                      D3DDEVCAPS_HWTRANSFORMANDLIGHT) ?
                                     D3DCREATE_HARDWARE_VERTEXPROCESSING :
                                     D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                     &pparams, &data->device);
    if (FAILED(result)) {
        D3D_DestroyRenderer(renderer);
        D3D_SetError("CreateDevice()", result);
        return NULL;
    }
    data->beginScene = SDL_TRUE;

    /* Get presentation parameters to fill info */
    result = IDirect3DDevice9_GetSwapChain(data->device, 0, &chain);
    if (FAILED(result)) {
        D3D_DestroyRenderer(renderer);
        D3D_SetError("GetSwapChain()", result);
        return NULL;
    }
    result = IDirect3DSwapChain9_GetPresentParameters(chain, &pparams);
    if (FAILED(result)) {
        IDirect3DSwapChain9_Release(chain);
        D3D_DestroyRenderer(renderer);
        D3D_SetError("GetPresentParameters()", result);
        return NULL;
    }
    IDirect3DSwapChain9_Release(chain);
    switch (pparams.SwapEffect) {
    case D3DSWAPEFFECT_COPY:
        renderer->info.flags |= SDL_RENDERER_PRESENTCOPY;
        break;
    case D3DSWAPEFFECT_FLIP:
        switch (pparams.BackBufferCount) {
        case 2:
            renderer->info.flags |= SDL_RENDERER_PRESENTFLIP2;
            break;
        case 3:
            renderer->info.flags |= SDL_RENDERER_PRESENTFLIP3;
            break;
        }
        break;
    case D3DSWAPEFFECT_DISCARD:
        renderer->info.flags |= SDL_RENDERER_PRESENTDISCARD;
        break;
    }
    if (pparams.PresentationInterval == D3DPRESENT_INTERVAL_ONE) {
        renderer->info.flags |= SDL_RENDERER_PRESENTVSYNC;
    }
    data->pparams = pparams;

    IDirect3DDevice9_GetDeviceCaps(data->device, &caps);
    renderer->info.max_texture_width = caps.MaxTextureWidth;
    renderer->info.max_texture_height = caps.MaxTextureHeight;

    /* Set up parameters for rendering */
    IDirect3DDevice9_SetVertexShader(data->device, NULL);
    IDirect3DDevice9_SetFVF(data->device,
                            D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
    IDirect3DDevice9_SetRenderState(data->device, D3DRS_ZENABLE, D3DZB_FALSE);
    IDirect3DDevice9_SetRenderState(data->device, D3DRS_CULLMODE,
                                    D3DCULL_NONE);
    IDirect3DDevice9_SetRenderState(data->device, D3DRS_LIGHTING, FALSE);
    /* Enable color modulation by diffuse color */
    IDirect3DDevice9_SetTextureStageState(data->device, 0, D3DTSS_COLOROP,
                                          D3DTOP_MODULATE);
    IDirect3DDevice9_SetTextureStageState(data->device, 0, D3DTSS_COLORARG1,
                                          D3DTA_TEXTURE);
    IDirect3DDevice9_SetTextureStageState(data->device, 0, D3DTSS_COLORARG2,
                                          D3DTA_DIFFUSE);
    /* Enable alpha modulation by diffuse alpha */
    IDirect3DDevice9_SetTextureStageState(data->device, 0, D3DTSS_ALPHAOP,
                                          D3DTOP_MODULATE);
    IDirect3DDevice9_SetTextureStageState(data->device, 0, D3DTSS_ALPHAARG1,
                                          D3DTA_TEXTURE);
    IDirect3DDevice9_SetTextureStageState(data->device, 0, D3DTSS_ALPHAARG2,
                                          D3DTA_DIFFUSE);
    /* Disable second texture stage, since we're done */
    IDirect3DDevice9_SetTextureStageState(data->device, 1, D3DTSS_COLOROP,
                                          D3DTOP_DISABLE);
    IDirect3DDevice9_SetTextureStageState(data->device, 1, D3DTSS_ALPHAOP,
                                          D3DTOP_DISABLE);

    {
#ifdef ASSEMBLE_SHADER
        const char *shader_text =
"ps_1_1\n"
"def c0, 0, 0, 0, 0.496\n"
"def c1, 0, 0, 0, 1\n"
"def c2, 0, 0, 0, -1\n"
"tex t0\n"
"mul r1, t0, v0\n"
"add r0, r1, c0\n"
"cnd r0, r0.a, c1, c2\n"
"add r0, r0, r1\n";
        LPD3DXBUFFER pCode;         // buffer with the assembled shader code
        LPD3DXBUFFER pErrorMsgs;    // buffer with error messages
        LPDWORD shader_data;
        DWORD   shader_size;
        result = D3DXAssembleShader( shader_text, SDL_strlen(shader_text), NULL, NULL, 0, &pCode, &pErrorMsgs );
        if (FAILED(result)) {
            D3D_SetError("D3DXAssembleShader()", result);
        }
        shader_data = (DWORD*)pCode->lpVtbl->GetBufferPointer(pCode);
        shader_size = pCode->lpVtbl->GetBufferSize(pCode);
#else
        const DWORD shader_data[] = {
            0xffff0101,0x00000051,0xa00f0000,0x00000000,0x00000000,0x00000000,
            0x3efdf3b6,0x00000051,0xa00f0001,0x00000000,0x00000000,0x00000000,
            0x3f800000,0x00000051,0xa00f0002,0x00000000,0x00000000,0x00000000,
            0xbf800000,0x00000042,0xb00f0000,0x00000005,0x800f0001,0xb0e40000,
            0x90e40000,0x00000002,0x800f0000,0x80e40001,0xa0e40000,0x00000050,
            0x800f0000,0x80ff0000,0xa0e40001,0xa0e40002,0x00000002,0x800f0000,
            0x80e40000,0x80e40001,0x0000ffff
        };
#endif
        result = IDirect3DDevice9_CreatePixelShader(data->device, shader_data, &data->ps_mask);
        if (FAILED(result)) {
            D3D_SetError("CreatePixelShader()", result);
        }
    }

    return renderer;
}

static int
D3D_Reset(SDL_Renderer * renderer)
{
    D3D_RenderData *data = (D3D_RenderData *) renderer->driverdata;
    HRESULT result;

    result = IDirect3DDevice9_Reset(data->device, &data->pparams);
    if (FAILED(result)) {
        if (result == D3DERR_DEVICELOST) {
            /* Don't worry about it, we'll reset later... */
            return 0;
        } else {
            D3D_SetError("Reset()", result);
            return -1;
        }
    }
    IDirect3DDevice9_SetVertexShader(data->device, NULL);
    IDirect3DDevice9_SetFVF(data->device,
                            D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
    IDirect3DDevice9_SetRenderState(data->device, D3DRS_CULLMODE,
                                    D3DCULL_NONE);
    IDirect3DDevice9_SetRenderState(data->device, D3DRS_LIGHTING, FALSE);
    return 0;
}

static int
D3D_DisplayModeChanged(SDL_Renderer * renderer)
{
    D3D_RenderData *data = (D3D_RenderData *) renderer->driverdata;
    SDL_Window *window = renderer->window;
    SDL_VideoDisplay *display = window->display;

    data->pparams.BackBufferWidth = window->w;
    data->pparams.BackBufferHeight = window->h;
    if (window->flags & SDL_WINDOW_FULLSCREEN) {
        data->pparams.BackBufferFormat =
            PixelFormatToD3DFMT(window->fullscreen_mode.format);
    } else {
        data->pparams.BackBufferFormat = D3DFMT_UNKNOWN;
    }
    return D3D_Reset(renderer);
}

static int
D3D_CreateTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    D3D_RenderData *renderdata = (D3D_RenderData *) renderer->driverdata;
    SDL_Window *window = renderer->window;
    SDL_VideoDisplay *display = window->display;
    Uint32 display_format = display->current_mode.format;
    D3D_TextureData *data;
    HRESULT result;

    data = (D3D_TextureData *) SDL_calloc(1, sizeof(*data));
    if (!data) {
        SDL_OutOfMemory();
        return -1;
    }

    texture->driverdata = data;

    if (SDL_ISPIXELFORMAT_FOURCC(texture->format) &&
        (texture->format != SDL_PIXELFORMAT_YUY2 ||
         !D3D_IsTextureFormatAvailable(renderdata->d3d, renderdata->adapter,
                                       display_format, texture->format))
        && (texture->format != SDL_PIXELFORMAT_YVYU
            || !D3D_IsTextureFormatAvailable(renderdata->d3d, renderdata->adapter,
                                             display_format, texture->format))) {
        data->yuv =
            SDL_SW_CreateYUVTexture(texture->format, texture->w, texture->h);
        if (!data->yuv) {
            return -1;
        }
        data->format = display->current_mode.format;
    } else {
        data->format = texture->format;
    }

    result =
        IDirect3DDevice9_CreateTexture(renderdata->device, texture->w,
                                       texture->h, 1, 0,
                                       PixelFormatToD3DFMT(data->format),
                                       D3DPOOL_SDL, &data->texture, NULL);
    if (FAILED(result)) {
        D3D_SetError("CreateTexture()", result);
        return -1;
    }

    return 0;
}

static int
D3D_QueryTexturePixels(SDL_Renderer * renderer, SDL_Texture * texture,
                       void **pixels, int *pitch)
{
    D3D_TextureData *data = (D3D_TextureData *) texture->driverdata;

    if (data->yuv) {
        return SDL_SW_QueryYUVTexturePixels(data->yuv, pixels, pitch);
    } else {
        /* D3D textures don't have their pixels hanging out */
        return -1;
    }
}

static int
D3D_SetTexturePalette(SDL_Renderer * renderer, SDL_Texture * texture,
                      const SDL_Color * colors, int firstcolor, int ncolors)
{
    D3D_RenderData *renderdata = (D3D_RenderData *) renderer->driverdata;
    D3D_TextureData *data = (D3D_TextureData *) texture->driverdata;

    return 0;
}

static int
D3D_GetTexturePalette(SDL_Renderer * renderer, SDL_Texture * texture,
                      SDL_Color * colors, int firstcolor, int ncolors)
{
    D3D_TextureData *data = (D3D_TextureData *) texture->driverdata;

    return 0;
}

static int
D3D_SetTextureColorMod(SDL_Renderer * renderer, SDL_Texture * texture)
{
    return 0;
}

static int
D3D_SetTextureAlphaMod(SDL_Renderer * renderer, SDL_Texture * texture)
{
    return 0;
}

static int
D3D_SetTextureBlendMode(SDL_Renderer * renderer, SDL_Texture * texture)
{
    switch (texture->blendMode) {
    case SDL_BLENDMODE_NONE:
    case SDL_BLENDMODE_MASK:
    case SDL_BLENDMODE_BLEND:
    case SDL_BLENDMODE_ADD:
    case SDL_BLENDMODE_MOD:
        return 0;
    default:
        SDL_Unsupported();
        texture->blendMode = SDL_BLENDMODE_NONE;
        return -1;
    }
}

static int
D3D_SetTextureScaleMode(SDL_Renderer * renderer, SDL_Texture * texture)
{
    switch (texture->scaleMode) {
    case SDL_TEXTURESCALEMODE_NONE:
    case SDL_TEXTURESCALEMODE_FAST:
    case SDL_TEXTURESCALEMODE_SLOW:
    case SDL_TEXTURESCALEMODE_BEST:
        return 0;
    default:
        SDL_Unsupported();
        texture->scaleMode = SDL_TEXTURESCALEMODE_NONE;
        return -1;
    }
    return 0;
}

static int
D3D_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                  const SDL_Rect * rect, const void *pixels, int pitch)
{
    D3D_TextureData *data = (D3D_TextureData *) texture->driverdata;
    D3D_RenderData *renderdata = (D3D_RenderData *) renderer->driverdata;

    if (data->yuv) {
        if (SDL_SW_UpdateYUVTexture(data->yuv, rect, pixels, pitch) < 0) {
            return -1;
        }
        UpdateYUVTextureData(texture);
        return 0;
    } else {
#ifdef SDL_MEMORY_POOL_DEFAULT
        IDirect3DTexture9 *temp;
        RECT d3drect;
        D3DLOCKED_RECT locked;
        const Uint8 *src;
        Uint8 *dst;
        int row, length;
        HRESULT result;

        result =
            IDirect3DDevice9_CreateTexture(renderdata->device, texture->w,
                                           texture->h, 1, 0,
                                           PixelFormatToD3DFMT(texture->
                                                               format),
                                           D3DPOOL_SYSTEMMEM, &temp, NULL);
        if (FAILED(result)) {
            D3D_SetError("CreateTexture()", result);
            return -1;
        }

        d3drect.left = rect->x;
        d3drect.right = rect->x + rect->w;
        d3drect.top = rect->y;
        d3drect.bottom = rect->y + rect->h;

        result = IDirect3DTexture9_LockRect(temp, 0, &locked, &d3drect, 0);
        if (FAILED(result)) {
            IDirect3DTexture9_Release(temp);
            D3D_SetError("LockRect()", result);
            return -1;
        }

        src = pixels;
        dst = locked.pBits;
        length = rect->w * SDL_BYTESPERPIXEL(texture->format);
        for (row = 0; row < rect->h; ++row) {
            SDL_memcpy(dst, src, length);
            src += pitch;
            dst += locked.Pitch;
        }
        IDirect3DTexture9_UnlockRect(temp, 0);

        result =
            IDirect3DDevice9_UpdateTexture(renderdata->device,
                                           (IDirect3DBaseTexture9 *) temp,
                                           (IDirect3DBaseTexture9 *)
                                           data->texture);
        IDirect3DTexture9_Release(temp);
        if (FAILED(result)) {
            D3D_SetError("UpdateTexture()", result);
            return -1;
        }
#else
        RECT d3drect;
        D3DLOCKED_RECT locked;
        const Uint8 *src;
        Uint8 *dst;
        int row, length;
        HRESULT result;

        d3drect.left = rect->x;
        d3drect.right = rect->x + rect->w;
        d3drect.top = rect->y;
        d3drect.bottom = rect->y + rect->h;

        result =
            IDirect3DTexture9_LockRect(data->texture, 0, &locked, &d3drect,
                                       0);
        if (FAILED(result)) {
            D3D_SetError("LockRect()", result);
            return -1;
        }

        src = pixels;
        dst = locked.pBits;
        length = rect->w * SDL_BYTESPERPIXEL(texture->format);
        for (row = 0; row < rect->h; ++row) {
            SDL_memcpy(dst, src, length);
            src += pitch;
            dst += locked.Pitch;
        }
        IDirect3DTexture9_UnlockRect(data->texture, 0);
#endif // SDL_MEMORY_POOL_DEFAULT

        return 0;
    }
}

static int
D3D_LockTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                const SDL_Rect * rect, int markDirty, void **pixels,
                int *pitch)
{
    D3D_TextureData *data = (D3D_TextureData *) texture->driverdata;

    if (data->yuv) {
        return SDL_SW_LockYUVTexture(data->yuv, rect, markDirty, pixels,
                                     pitch);
    } else {
        RECT d3drect;
        D3DLOCKED_RECT locked;
        HRESULT result;

        d3drect.left = rect->x;
        d3drect.right = rect->x + rect->w;
        d3drect.top = rect->y;
        d3drect.bottom = rect->y + rect->h;

        result =
            IDirect3DTexture9_LockRect(data->texture, 0, &locked, &d3drect,
                                       markDirty ? 0 :
                                       D3DLOCK_NO_DIRTY_UPDATE);
        if (FAILED(result)) {
            D3D_SetError("LockRect()", result);
            return -1;
        }
        *pixels = locked.pBits;
        *pitch = locked.Pitch;
        return 0;
    }
}

static void
D3D_UnlockTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    D3D_TextureData *data = (D3D_TextureData *) texture->driverdata;

    if (data->yuv) {
        SDL_SW_UnlockYUVTexture(data->yuv);
        UpdateYUVTextureData(texture);
    } else {
        IDirect3DTexture9_UnlockRect(data->texture, 0);
    }
}

static void
D3D_DirtyTexture(SDL_Renderer * renderer, SDL_Texture * texture, int numrects,
                 const SDL_Rect * rects)
{
    D3D_TextureData *data = (D3D_TextureData *) texture->driverdata;
    RECT d3drect;
    int i;

    for (i = 0; i < numrects; ++i) {
        const SDL_Rect *rect = &rects[i];

        d3drect.left = rect->x;
        d3drect.right = rect->x + rect->w;
        d3drect.top = rect->y;
        d3drect.bottom = rect->y + rect->h;

        IDirect3DTexture9_AddDirtyRect(data->texture, &d3drect);
    }
}

static void
D3D_SetBlendMode(D3D_RenderData * data, int blendMode)
{
    switch (blendMode) {
    case SDL_BLENDMODE_NONE:
        IDirect3DDevice9_SetRenderState(data->device, D3DRS_ALPHABLENDENABLE,
                                        FALSE);
        break;
    case SDL_BLENDMODE_MASK:
    case SDL_BLENDMODE_BLEND:
        IDirect3DDevice9_SetRenderState(data->device, D3DRS_ALPHABLENDENABLE,
                                        TRUE);
        IDirect3DDevice9_SetRenderState(data->device, D3DRS_SRCBLEND,
                                        D3DBLEND_SRCALPHA);
        IDirect3DDevice9_SetRenderState(data->device, D3DRS_DESTBLEND,
                                        D3DBLEND_INVSRCALPHA);
        break;
    case SDL_BLENDMODE_ADD:
        IDirect3DDevice9_SetRenderState(data->device, D3DRS_ALPHABLENDENABLE,
                                        TRUE);
        IDirect3DDevice9_SetRenderState(data->device, D3DRS_SRCBLEND,
                                        D3DBLEND_SRCALPHA);
        IDirect3DDevice9_SetRenderState(data->device, D3DRS_DESTBLEND,
                                        D3DBLEND_ONE);
        break;
    case SDL_BLENDMODE_MOD:
        IDirect3DDevice9_SetRenderState(data->device, D3DRS_ALPHABLENDENABLE,
                                        TRUE);
        IDirect3DDevice9_SetRenderState(data->device, D3DRS_SRCBLEND,
                                        D3DBLEND_ZERO);
        IDirect3DDevice9_SetRenderState(data->device, D3DRS_DESTBLEND,
                                        D3DBLEND_SRCCOLOR);
        break;
    }
}

static int
D3D_RenderDrawPoints(SDL_Renderer * renderer, const SDL_Point * points,
                     int count)
{
    D3D_RenderData *data = (D3D_RenderData *) renderer->driverdata;
    DWORD color;
    Vertex *vertices;
    int i;
    HRESULT result;

    if (data->beginScene) {
        IDirect3DDevice9_BeginScene(data->device);
        data->beginScene = SDL_FALSE;
    }

    D3D_SetBlendMode(data, renderer->blendMode);

    result =
        IDirect3DDevice9_SetTexture(data->device, 0,
                                    (IDirect3DBaseTexture9 *) 0);
    if (FAILED(result)) {
        D3D_SetError("SetTexture()", result);
        return -1;
    }

    color = D3DCOLOR_ARGB(renderer->a, renderer->r, renderer->g, renderer->b);

    vertices = SDL_stack_alloc(Vertex, count);
    for (i = 0; i < count; ++i) {
        vertices[i].x = (float) points[i].x;
        vertices[i].y = (float) points[i].y;
        vertices[i].z = 0.0f;
        vertices[i].rhw = 1.0f;
        vertices[i].color = color;
        vertices[i].u = 0.0f;
        vertices[i].v = 0.0f;
    }
    result =
        IDirect3DDevice9_DrawPrimitiveUP(data->device, D3DPT_POINTLIST, count,
                                         vertices, sizeof(*vertices));
    SDL_stack_free(vertices);
    if (FAILED(result)) {
        D3D_SetError("DrawPrimitiveUP()", result);
        return -1;
    }
    return 0;
}

static int
D3D_RenderDrawLines(SDL_Renderer * renderer, const SDL_Point * points,
                    int count)
{
    D3D_RenderData *data = (D3D_RenderData *) renderer->driverdata;
    DWORD color;
    Vertex *vertices;
    int i;
    HRESULT result;

    if (data->beginScene) {
        IDirect3DDevice9_BeginScene(data->device);
        data->beginScene = SDL_FALSE;
    }

    D3D_SetBlendMode(data, renderer->blendMode);

    result =
        IDirect3DDevice9_SetTexture(data->device, 0,
                                    (IDirect3DBaseTexture9 *) 0);
    if (FAILED(result)) {
        D3D_SetError("SetTexture()", result);
        return -1;
    }

    color = D3DCOLOR_ARGB(renderer->a, renderer->r, renderer->g, renderer->b);

    vertices = SDL_stack_alloc(Vertex, count);
    for (i = 0; i < count; ++i) {
        vertices[i].x = (float) points[i].x;
        vertices[i].y = (float) points[i].y;
        vertices[i].z = 0.0f;
        vertices[i].rhw = 1.0f;
        vertices[i].color = color;
        vertices[i].u = 0.0f;
        vertices[i].v = 0.0f;
    }
    result =
        IDirect3DDevice9_DrawPrimitiveUP(data->device, D3DPT_LINESTRIP, count-1,
                                         vertices, sizeof(*vertices));

    /* DirectX 9 has the same line rasterization semantics as GDI,
       so we need to close the endpoint of the line */
    if (points[0].x != points[count-1].x || points[0].y != points[count-1].y) {
        vertices[0].x = (float) points[count-1].x;
        vertices[0].y = (float) points[count-1].y;
        result = IDirect3DDevice9_DrawPrimitiveUP(data->device, D3DPT_POINTLIST, 1, vertices, sizeof(*vertices));
    }

    SDL_stack_free(vertices);
    if (FAILED(result)) {
        D3D_SetError("DrawPrimitiveUP()", result);
        return -1;
    }
    return 0;
}

static int
D3D_RenderDrawRects(SDL_Renderer * renderer, const SDL_Rect ** rects,
                    int count)
{
    D3D_RenderData *data = (D3D_RenderData *) renderer->driverdata;
    DWORD color;
    int i;
    Vertex vertices[5];
    HRESULT result;

    if (data->beginScene) {
        IDirect3DDevice9_BeginScene(data->device);
        data->beginScene = SDL_FALSE;
    }

    D3D_SetBlendMode(data, renderer->blendMode);

    result =
        IDirect3DDevice9_SetTexture(data->device, 0,
                                    (IDirect3DBaseTexture9 *) 0);
    if (FAILED(result)) {
        D3D_SetError("SetTexture()", result);
        return -1;
    }

    color = D3DCOLOR_ARGB(renderer->a, renderer->r, renderer->g, renderer->b);

    for (i = 0; i < SDL_arraysize(vertices); ++i) {
        vertices[i].z = 0.0f;
        vertices[i].rhw = 1.0f;
        vertices[i].color = color;
        vertices[i].u = 0.0f;
        vertices[i].v = 0.0f;
    }

    for (i = 0; i < count; ++i) {
        const SDL_Rect *rect = rects[i];

        vertices[0].x = (float) rect->x;
        vertices[0].y = (float) rect->y;

        vertices[1].x = (float) rect->x+rect->w-1;
        vertices[1].y = (float) rect->y;

        vertices[2].x = (float) rect->x+rect->w-1;
        vertices[2].y = (float) rect->y+rect->h-1;

        vertices[3].x = (float) rect->x;
        vertices[3].y = (float) rect->y+rect->h-1;

        vertices[4].x = (float) rect->x;
        vertices[4].y = (float) rect->y;

        result =
            IDirect3DDevice9_DrawPrimitiveUP(data->device, D3DPT_LINESTRIP, 4,
                                             vertices, sizeof(*vertices));

        if (FAILED(result)) {
            D3D_SetError("DrawPrimitiveUP()", result);
            return -1;
        }
    }
    return 0;
}

static int
D3D_RenderFillRects(SDL_Renderer * renderer, const SDL_Rect ** rects,
                    int count)
{
    D3D_RenderData *data = (D3D_RenderData *) renderer->driverdata;
    DWORD color;
    int i;
    float minx, miny, maxx, maxy;
    Vertex vertices[4];
    HRESULT result;

    if (data->beginScene) {
        IDirect3DDevice9_BeginScene(data->device);
        data->beginScene = SDL_FALSE;
    }

    D3D_SetBlendMode(data, renderer->blendMode);

    result =
        IDirect3DDevice9_SetTexture(data->device, 0,
                                    (IDirect3DBaseTexture9 *) 0);
    if (FAILED(result)) {
        D3D_SetError("SetTexture()", result);
        return -1;
    }

    color = D3DCOLOR_ARGB(renderer->a, renderer->r, renderer->g, renderer->b);

    for (i = 0; i < count; ++i) {
        const SDL_Rect *rect = rects[i];

        minx = (float) rect->x;
        miny = (float) rect->y;
        maxx = (float) rect->x + rect->w;
        maxy = (float) rect->y + rect->h;

        vertices[0].x = minx;
        vertices[0].y = miny;
        vertices[0].z = 0.0f;
        vertices[0].rhw = 1.0f;
        vertices[0].color = color;
        vertices[0].u = 0.0f;
        vertices[0].v = 0.0f;

        vertices[1].x = maxx;
        vertices[1].y = miny;
        vertices[1].z = 0.0f;
        vertices[1].rhw = 1.0f;
        vertices[1].color = color;
        vertices[1].u = 0.0f;
        vertices[1].v = 0.0f;

        vertices[2].x = maxx;
        vertices[2].y = maxy;
        vertices[2].z = 0.0f;
        vertices[2].rhw = 1.0f;
        vertices[2].color = color;
        vertices[2].u = 0.0f;
        vertices[2].v = 0.0f;

        vertices[3].x = minx;
        vertices[3].y = maxy;
        vertices[3].z = 0.0f;
        vertices[3].rhw = 1.0f;
        vertices[3].color = color;
        vertices[3].u = 0.0f;
        vertices[3].v = 0.0f;

        result =
            IDirect3DDevice9_DrawPrimitiveUP(data->device, D3DPT_TRIANGLEFAN,
                                             2, vertices, sizeof(*vertices));
        if (FAILED(result)) {
            D3D_SetError("DrawPrimitiveUP()", result);
            return -1;
        }
    }
    return 0;
}

static int
D3D_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
               const SDL_Rect * srcrect, const SDL_Rect * dstrect)
{
    D3D_RenderData *data = (D3D_RenderData *) renderer->driverdata;
    D3D_TextureData *texturedata = (D3D_TextureData *) texture->driverdata;
    LPDIRECT3DPIXELSHADER9 shader = NULL;
    float minx, miny, maxx, maxy;
    float minu, maxu, minv, maxv;
    DWORD color;
    Vertex vertices[4];
    HRESULT result;

    if (data->beginScene) {
        IDirect3DDevice9_BeginScene(data->device);
        data->beginScene = SDL_FALSE;
    }

    minx = (float) dstrect->x - 0.5f;
    miny = (float) dstrect->y - 0.5f;
    maxx = (float) dstrect->x + dstrect->w - 0.5f;
    maxy = (float) dstrect->y + dstrect->h - 0.5f;

    minu = (float) srcrect->x / texture->w;
    maxu = (float) (srcrect->x + srcrect->w) / texture->w;
    minv = (float) srcrect->y / texture->h;
    maxv = (float) (srcrect->y + srcrect->h) / texture->h;

    color = D3DCOLOR_ARGB(texture->a, texture->r, texture->g, texture->b);

    vertices[0].x = minx;
    vertices[0].y = miny;
    vertices[0].z = 0.0f;
    vertices[0].rhw = 1.0f;
    vertices[0].color = color;
    vertices[0].u = minu;
    vertices[0].v = minv;

    vertices[1].x = maxx;
    vertices[1].y = miny;
    vertices[1].z = 0.0f;
    vertices[1].rhw = 1.0f;
    vertices[1].color = color;
    vertices[1].u = maxu;
    vertices[1].v = minv;

    vertices[2].x = maxx;
    vertices[2].y = maxy;
    vertices[2].z = 0.0f;
    vertices[2].rhw = 1.0f;
    vertices[2].color = color;
    vertices[2].u = maxu;
    vertices[2].v = maxv;

    vertices[3].x = minx;
    vertices[3].y = maxy;
    vertices[3].z = 0.0f;
    vertices[3].rhw = 1.0f;
    vertices[3].color = color;
    vertices[3].u = minu;
    vertices[3].v = maxv;

    D3D_SetBlendMode(data, texture->blendMode);

    if (texture->blendMode == SDL_BLENDMODE_MASK) {
        shader = data->ps_mask;
    }

    switch (texture->scaleMode) {
    case SDL_TEXTURESCALEMODE_NONE:
    case SDL_TEXTURESCALEMODE_FAST:
        IDirect3DDevice9_SetSamplerState(data->device, 0, D3DSAMP_MINFILTER,
                                         D3DTEXF_POINT);
        IDirect3DDevice9_SetSamplerState(data->device, 0, D3DSAMP_MAGFILTER,
                                         D3DTEXF_POINT);
        break;
    case SDL_TEXTURESCALEMODE_SLOW:
        IDirect3DDevice9_SetSamplerState(data->device, 0, D3DSAMP_MINFILTER,
                                         D3DTEXF_LINEAR);
        IDirect3DDevice9_SetSamplerState(data->device, 0, D3DSAMP_MAGFILTER,
                                         D3DTEXF_LINEAR);
        break;
    case SDL_TEXTURESCALEMODE_BEST:
        IDirect3DDevice9_SetSamplerState(data->device, 0, D3DSAMP_MINFILTER,
                                         D3DTEXF_GAUSSIANQUAD);
        IDirect3DDevice9_SetSamplerState(data->device, 0, D3DSAMP_MAGFILTER,
                                         D3DTEXF_GAUSSIANQUAD);
        break;
    }

    result =
        IDirect3DDevice9_SetTexture(data->device, 0, (IDirect3DBaseTexture9 *)
                                    texturedata->texture);
    if (FAILED(result)) {
        D3D_SetError("SetTexture()", result);
        return -1;
    }
    if (shader) {
        result = IDirect3DDevice9_SetPixelShader(data->device, shader);
        if (FAILED(result)) {
            D3D_SetError("SetShader()", result);
            return -1;
        }
    }
    result =
        IDirect3DDevice9_DrawPrimitiveUP(data->device, D3DPT_TRIANGLEFAN, 2,
                                         vertices, sizeof(*vertices));
    if (FAILED(result)) {
        D3D_SetError("DrawPrimitiveUP()", result);
        return -1;
    }
    if (shader) {
        result = IDirect3DDevice9_SetPixelShader(data->device, NULL);
        if (FAILED(result)) {
            D3D_SetError("SetShader()", result);
            return -1;
        }
    }
    return 0;
}

static int
D3D_RenderReadPixels(SDL_Renderer * renderer, const SDL_Rect * rect,
                     Uint32 format, void * pixels, int pitch)
{
    D3D_RenderData *data = (D3D_RenderData *) renderer->driverdata;
    SDL_Window *window = renderer->window;
    SDL_VideoDisplay *display = window->display;
    D3DSURFACE_DESC desc;
    LPDIRECT3DSURFACE9 backBuffer;
    LPDIRECT3DSURFACE9 surface;
    RECT d3drect;
    D3DLOCKED_RECT locked;
    HRESULT result;

    result = IDirect3DDevice9_GetBackBuffer(data->device, 0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer);
    if (FAILED(result)) {
        D3D_SetError("GetBackBuffer()", result);
        return -1;
    }

    result = IDirect3DSurface9_GetDesc(backBuffer, &desc);
    if (FAILED(result)) {
        D3D_SetError("GetDesc()", result);
        IDirect3DSurface9_Release(backBuffer);
        return -1;
    }

    result = IDirect3DDevice9_CreateOffscreenPlainSurface(data->device, desc.Width, desc.Height, desc.Format, D3DPOOL_SYSTEMMEM, &surface, NULL);
    if (FAILED(result)) {
        D3D_SetError("CreateOffscreenPlainSurface()", result);
        IDirect3DSurface9_Release(backBuffer);
        return -1;
    }

    result = IDirect3DDevice9_GetRenderTargetData(data->device, backBuffer, surface);
    if (FAILED(result)) {
        D3D_SetError("GetRenderTargetData()", result);
        IDirect3DSurface9_Release(surface);
        IDirect3DSurface9_Release(backBuffer);
        return -1;
    }

    d3drect.left = rect->x;
    d3drect.right = rect->x + rect->w;
    d3drect.top = rect->y;
    d3drect.bottom = rect->y + rect->h;

    result = IDirect3DSurface9_LockRect(surface, &locked, &d3drect, D3DLOCK_READONLY);
    if (FAILED(result)) {
        D3D_SetError("LockRect()", result);
        IDirect3DSurface9_Release(surface);
        IDirect3DSurface9_Release(backBuffer);
        return -1;
    }

    SDL_ConvertPixels(rect->w, rect->h,
                      display->current_mode.format, locked.pBits, locked.Pitch,
                      format, pixels, pitch);

    IDirect3DSurface9_UnlockRect(surface);

    IDirect3DSurface9_Release(surface);
    IDirect3DSurface9_Release(backBuffer);

    return 0;
}

static int
D3D_RenderWritePixels(SDL_Renderer * renderer, const SDL_Rect * rect,
                      Uint32 format, const void * pixels, int pitch)
{
    /* Work in progress */
    SDL_Unsupported();
    return -1;
}

static void
D3D_RenderPresent(SDL_Renderer * renderer)
{
    D3D_RenderData *data = (D3D_RenderData *) renderer->driverdata;
    HRESULT result;

    if (!data->beginScene) {
        IDirect3DDevice9_EndScene(data->device);
        data->beginScene = SDL_TRUE;
    }

    result = IDirect3DDevice9_TestCooperativeLevel(data->device);
    if (result == D3DERR_DEVICELOST) {
        /* We'll reset later */
        return;
    }
    if (result == D3DERR_DEVICENOTRESET) {
        D3D_Reset(renderer);
    }
    result = IDirect3DDevice9_Present(data->device, NULL, NULL, NULL, NULL);
    if (FAILED(result)) {
        D3D_SetError("Present()", result);
    }
}

static void
D3D_DestroyTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    D3D_TextureData *data = (D3D_TextureData *) texture->driverdata;

    if (!data) {
        return;
    }
    if (data->yuv) {
        SDL_SW_DestroyYUVTexture(data->yuv);
    }
    if (data->texture) {
        IDirect3DTexture9_Release(data->texture);
    }
    SDL_free(data);
    texture->driverdata = NULL;
}

static void
D3D_DestroyRenderer(SDL_Renderer * renderer)
{
    D3D_RenderData *data = (D3D_RenderData *) renderer->driverdata;

    if (data) {
        if (data->device) {
            IDirect3DDevice9_Release(data->device);
        }
        SDL_free(data);
    }
    SDL_free(renderer);
}

#endif /* SDL_VIDEO_RENDER_D3D */

/* vi: set ts=4 sw=4 expandtab: */
