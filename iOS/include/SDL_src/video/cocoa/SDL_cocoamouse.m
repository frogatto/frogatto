/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2009 Sam Lantinga

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

#include "SDL_events.h"
#include "SDL_cocoavideo.h"

#include "../../events/SDL_mouse_c.h"

void
Cocoa_InitMouse(_THIS)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;
    SDL_Mouse mouse;

    SDL_zero(mouse);
    data->mouse = SDL_AddMouse(&mouse, "Mouse", 0, 0, 1);
}

static int
ConvertMouseButtonToSDL(int button)
{
    switch (button)
    {
        case 0:
            return(SDL_BUTTON_LEFT);   /* 1 */
        case 1:
            return(SDL_BUTTON_RIGHT);  /* 3 */
        case 2:
            return(SDL_BUTTON_MIDDLE); /* 2 */
    }
    return button;
}

void
Cocoa_HandleMouseEvent(_THIS, NSEvent *event)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;
    SDL_Mouse *mouse = SDL_GetMouse(data->mouse);
    int i;
    NSPoint point;
    SDL_Window *window;

    /* See if there are any fullscreen windows that might handle this event */
    window = NULL;
    for (i = 0; i < _this->num_displays; ++i) {
        SDL_VideoDisplay *display = &_this->displays[i];
        SDL_Window *candidate = display->fullscreen_window;

        if (candidate) {
            SDL_Rect bounds;

            Cocoa_GetDisplayBounds(_this, display, &bounds);
            point = [NSEvent mouseLocation];
            point.x = point.x - bounds.x;
            point.y = CGDisplayPixelsHigh(kCGDirectMainDisplay) - point.y - bounds.y;
            if (point.x < 0 || point.x >= candidate->w ||
                point.y < 0 || point.y >= candidate->h) {
                /* The mouse is out of this fullscreen display */
                if (mouse->focus == candidate) {
                    SDL_SetMouseFocus(data->mouse, 0);
                }
            } else {
                /* This is it! */
                window = candidate;
                break;
            }
        }
    }
    if (!window) {
        return;
    }

    /* Set the focus appropriately */
    if (mouse->focus != window) {
        SDL_SetMouseFocus(data->mouse, window);
    }

    switch ([event type]) {
    case NSLeftMouseDown:
    case NSOtherMouseDown:
    case NSRightMouseDown:
        SDL_SendMouseButton(data->mouse, SDL_PRESSED, ConvertMouseButtonToSDL([event buttonNumber]));
        break;
    case NSLeftMouseUp:
    case NSOtherMouseUp:
    case NSRightMouseUp:
        SDL_SendMouseButton(data->mouse, SDL_RELEASED, ConvertMouseButtonToSDL([event buttonNumber]));
        break;
    case NSLeftMouseDragged:
    case NSRightMouseDragged:
    case NSOtherMouseDragged: /* usually middle mouse dragged */
    case NSMouseMoved:
        SDL_SendMouseMotion(data->mouse, 0, (int)point.x, (int)point.y, 0);
        break;
    default: /* just to avoid compiler warnings */
        break;
    }
}

void
Cocoa_QuitMouse(_THIS)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;

    SDL_DelMouse(data->mouse);
}

/* vi: set ts=4 sw=4 expandtab: */
