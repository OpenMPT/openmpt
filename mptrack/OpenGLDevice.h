// OpenGLDevice.h: Schnittstelle für die Klasse OpenGLDevice.
//
// Copyright by André Stein
// E-Mail: stonemaster@steinsoft.net, andre_stein@web.de
// http://www.steinsoft.net
//////////////////////////////////////////////////////////////////////

#ifndef OPENGL_DEVICE_H
#define OPENGL_DEVICE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>

class OpenGLDevice
{
public:
	OpenGLDevice(HDC& deviceContext,int stencil = 0);
	OpenGLDevice(HWND& window,int stencil = 0);
	OpenGLDevice();

	bool create(HDC& deviceContext,int  stencil = 0);
	bool create(HWND& window,int stencil = 0);

	void destroy();
	void makeCurrent(bool disableOther = true);

	
	virtual ~OpenGLDevice();

protected:
	bool setDCPixelFormat(HDC& deviceContext,int stencil);
	
	HGLRC renderContext;
	HDC deviceContext;
};

#endif // ifndef OPENGL_DEVICE_H
