/**
*
* eglport-tegra.c
* Copyright (C) 2011 Scott Smith, Joni Valtanen
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

#include "eglport.h"

#define EGLNativeWindowType NativeWindowType
#define EGLNativeDisplayType NativeDisplayType

EGLDisplay g_eglDisplay = 0;
EGLConfig g_eglConfig = 0;
EGLContext g_eglContext = 0;
EGLSurface g_eglSurface = 0;

#define g_totalConfigsIn 20
int g_totalConfigsFound = 0;
EGLConfig g_allConfigs[g_totalConfigsIn];
Display *g_x11Display = NULL;


/*======================================================
 * Kill off any opengl specific details
  ====================================================*/
void EGL_Destroy()
{
    if( g_eglSurface || g_eglContext || g_eglDisplay )
    {
        eglMakeCurrent(g_eglDisplay, NULL, NULL, EGL_NO_CONTEXT);
        eglDestroyContext(g_eglDisplay, g_eglContext);
        eglDestroySurface(g_eglDisplay, g_eglSurface);
        eglTerminate(g_eglDisplay);
    }

    g_eglSurface = 0;
    g_eglContext = 0;
    g_eglDisplay = 0;

    if (g_x11Display)
        XCloseDisplay(g_x11Display);

    g_x11Display = NULL;

    printf( "EGL Closed\n");
}

/*===========================================================
Setup EGL context and surface
===========================================================*/
int EGL_Init( void )
{
    EGL_Open ();
    FindAppropriateEGLConfigs();

    int configIndex = 0;
    printf( "Config %d\n", configIndex );

	if (!ConfigureEGL(g_allConfigs[configIndex]))
	{
		TestEGLError();
		fprintf(stderr, "ERROR: Unable to initialise EGL. See previous error.\n");
		return 1;
	}

    return 0;
}

/*===========================================================
Swap EGL buffers and update the display
===========================================================*/
void EGL_SwapBuffers( void )
{
	eglSwapBuffers(g_eglDisplay, g_eglSurface);
}


/*========================================================
 *  Init base EGL
 * ======================================================*/
int EGL_Open( void )
{
    // use EGL to initialise GLES
    printf( "EGL Open display\n" );
    g_x11Display = XOpenDisplay(NULL);
    if (!g_x11Display)
    {
        fprintf(stderr, "ERROR: unable to get display!\n");
        return 0;
    }

    printf( "EGL Get display\n" );
    g_eglDisplay = eglGetDisplay((EGLNativeDisplayType)g_x11Display);
    if (g_eglDisplay == EGL_NO_DISPLAY)
    {
        TestEGLError();
        fprintf(stderr, "ERROR: Unable to initialise EGL display.\n");
        return 0;
    }

    // Initialise egl
    printf( "EGL Init\n" );
    if (!eglInitialize(g_eglDisplay, NULL, NULL))
    {
        TestEGLError();
        fprintf(stderr, "ERROR: Unable to initialise EGL display.\n");
        return 0;
    }

    return 1;
}

/*===========================================================
Initialise OpenGL settings
===========================================================*/
int ConfigureEGL(EGLConfig config)
{
    // Cleanup in case of a reset
    if( g_eglSurface || g_eglContext || g_eglDisplay )
    {
        eglMakeCurrent(g_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroyContext(g_eglDisplay, g_eglContext);
        eglDestroySurface(g_eglDisplay, g_eglSurface);
    }

    // Bind GLES and create the context
    printf( "EGL Bind\n" );
    eglBindAPI(EGL_OPENGL_ES_API);
	if (!TestEGLError() )
	{
		return 0;
	}

    printf( "EGL Create Context\n" );
    g_eglContext = eglCreateContext(g_eglDisplay, config, EGL_NO_CONTEXT, NULL);
    if (g_eglContext == EGL_NO_CONTEXT)
    {
        TestEGLError();
        fprintf(stderr, "ERROR: Unable to create GLES context!\n");
        return 0;
    }

    // Get the SDL window handle
    SDL_SysWMinfo sysInfo; //Will hold our Window information
    SDL_VERSION(&sysInfo.version); //Set SDL version
    if(SDL_GetWMInfo(&sysInfo) <= 0)
    {
        TestEGLError();
        fprintf( stderr, "ERROR: Unable to get window handle\n");
        return 0;
    }

    printf( "EGL Create window surface\n" );
    g_eglSurface = eglCreateWindowSurface(g_eglDisplay, config, (EGLNativeWindowType)sysInfo.info.x11.window, 0);

    if ( g_eglSurface == EGL_NO_SURFACE)
    {
        TestEGLError();
        fprintf(stderr, "ERROR: Unable to create EGL surface!\n");
        return 0;
    }

    printf( "EGL Make Current\n" );
    if (eglMakeCurrent(g_eglDisplay,  g_eglSurface,  g_eglSurface, g_eglContext) == EGL_FALSE)
    {
        TestEGLError();
        fprintf(stderr, "ERROR: Unable to make GLES context current\n");
        return 0;
    }

    printf( "EGL Done\n" );
    return 1;
}

/*=======================================================
* Detect available video resolutions
=======================================================*/
int FindAppropriateEGLConfigs( void )
{
    static const EGLint s_configAttribs[] =
    {
          EGL_RED_SIZE,        1,
          EGL_GREEN_SIZE,      1,
          EGL_BLUE_SIZE,       1,
          EGL_DEPTH_SIZE,      1,
          EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
          EGL_RENDERABLE_TYPE, 2,
          EGL_NONE
    };

    if (eglChooseConfig(g_eglDisplay, s_configAttribs, g_allConfigs, g_totalConfigsIn, &g_totalConfigsFound) != EGL_TRUE || g_totalConfigsFound == 0)
    {
        TestEGLError();
        fprintf(stderr, "ERROR: Unable to query for available configs.\n");
        return 0;
    }
    fprintf(stderr, "Found %d available configs\n", g_totalConfigsFound);
    return 1;
}

int TestEGLError( void )
{
	EGLint iErr = eglGetError();
	while (iErr != EGL_SUCCESS)
	{
		printf("EGL failed (0x%x).\n", iErr);
		return 0;
	}

	return 1;
}
