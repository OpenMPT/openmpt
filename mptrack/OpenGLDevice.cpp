//////////////////////////////////////////////////////////////////////
// OpenGLDevice.cpp: Implementierung der Klasse OpenGLDevice.
//
// Copyright by André Stein
// E-Mail: stonemaster@steinsoft.net, andre_stein@web.de
// http://www.steinsoft.net
//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
//      Klassenname: OpenGLDevice
//      
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "OpenGLDevice.h"

//////////////////////////////////////////////////////////////////////
// Konstruktion/Destruktion
//////////////////////////////////////////////////////////////////////

OpenGLDevice::OpenGLDevice()
{
	renderContext = NULL;
	deviceContext = NULL;
}

OpenGLDevice::~OpenGLDevice()
{
	destroy();
}

OpenGLDevice::OpenGLDevice(HWND& window,int stencil)
{
	create(window,stencil);
}

OpenGLDevice::OpenGLDevice(HDC& deviceContext,int stencil)
{
	create(deviceContext,stencil);
}

//////////////////////////////////////////////////////////////////////
// Öffentliche Funktionen
//////////////////////////////////////////////////////////////////////

bool OpenGLDevice::create(HWND& window,int stencil)
{
	HDC deviceContext = ::GetDC(window);
	
	if (!create(deviceContext,stencil))
	{
		::ReleaseDC(window, deviceContext);

		return false;
	}

	::ReleaseDC(window, deviceContext);
	
	return true;
}

bool OpenGLDevice::create(HDC& deviceContext,int stencil)
{
	if (!deviceContext)
	{
		return false;
	}

	if (!setDCPixelFormat(deviceContext,stencil))
	{
		return false;
	}

	renderContext = wglCreateContext(deviceContext);
	wglMakeCurrent(deviceContext, renderContext);

	OpenGLDevice::deviceContext = deviceContext;
	
	return true;
}



void OpenGLDevice::destroy()
{
	if (renderContext != NULL)
	{
		wglMakeCurrent(NULL,NULL);
		wglDeleteContext(renderContext);
	}
}

void OpenGLDevice::makeCurrent(bool disableOther)
{
	if (renderContext != NULL)
	{
			//should all other device contexts
			//be disabled then?
			if (disableOther) 
				wglMakeCurrent(NULL,NULL);

			wglMakeCurrent(deviceContext, renderContext);
	}
}

//////////////////////////////////////////////////////////////////////
// Protected-Funktionen
//////////////////////////////////////////////////////////////////////


bool OpenGLDevice::setDCPixelFormat(HDC& deviceContext,int stencil)
{
	int pixelFormat;
	DEVMODE resolution;
	char text[256];

	//PIXELFORMAT->BPP = DESKTOP->BPP
	EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &resolution);
	
	static PIXELFORMATDESCRIPTOR pixelFormatDesc =
	{
        sizeof (PIXELFORMATDESCRIPTOR),             
        1,                                          
        PFD_DRAW_TO_WINDOW |                        
        PFD_SUPPORT_OPENGL |
        PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,                              
        resolution.dmBitsPerPel,                                         
        0, 0, 0, 0, 0, 0,                           
        0, 
		0,                                       
        0, 
		0, 0, 0, 0,                              
        16,                                         
        stencil,                                          
        0,                                          
        0,                             
        0,                                          
        0, 0, 0                                     
    };

    
    
    pixelFormat = ChoosePixelFormat (deviceContext,
									&pixelFormatDesc);

    if (!SetPixelFormat(deviceContext, pixelFormat, 
		&pixelFormatDesc)) 
	{
      return false ;
    }

    return true;
}