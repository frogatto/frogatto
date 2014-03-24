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

#include "SDL_video.h"


int
SDL_BlendRect(SDL_Surface * dst, const SDL_Rect * rect,
              int blendMode, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    SDL_Rect full_rect;
    SDL_Point points[5];

    if (!dst) {
        SDL_SetError("Passed NULL destination surface");
        return -1;
    }

    /* If 'rect' == NULL, then outline the whole surface */
    if (!rect) {
        full_rect.x = 0;
        full_rect.y = 0;
        full_rect.w = dst->w;
        full_rect.h = dst->h;
        rect = &full_rect;
    }

    points[0].x = rect->x;
    points[0].y = rect->y;
    points[1].x = rect->x+rect->w-1;
    points[1].y = rect->y;
    points[2].x = rect->x+rect->w-1;
    points[2].y = rect->y+rect->h-1;
    points[3].x = rect->x;
    points[3].y = rect->y+rect->h-1;
    points[4].x = rect->x;
    points[4].y = rect->y;
    return SDL_BlendLines(dst, points, 5, blendMode, r, g, b, a);
}

int
SDL_BlendRects(SDL_Surface * dst, const SDL_Rect ** rects, int count,
               int blendMode, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    int i;

    for (i = 0; i < count; ++i) {
        if (SDL_BlendRect(dst, rects[i], blendMode, r, g, b, a) < 0) {
            return -1;
        }
    }
    return 0;
}

/* vi: set ts=4 sw=4 expandtab: */
