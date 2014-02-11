/**
*
* eglport.c/.h
* Copyright (C) 2011 Scott Smith
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
*/

#ifndef EGLPORT_H
#define EGLPORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <math.h>
#include <GLES/gl.h>
#include <EGL/egl.h>
#include <SDL/SDL_syswm.h>

void EGL_Destroy( void );
int EGL_Open( void );
int EGL_Init( void );
void EGL_SwapBuffers( void );

int ConfigureEGL(EGLConfig config);
int FindAppropriateEGLConfigs( void );
int TestEGLError( void );

#ifdef __cplusplus
}
#endif

#endif // EGLPORT_H
