// OpenGLControl.cpp: Implementierungsdatei
//

#include "stdafx.h"
#include "OpenGLControl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COpenGLControl

COpenGLControl::COpenGLControl()
{
	dc = NULL;
	rotation = 0.0f;
}

COpenGLControl::~COpenGLControl()
{
	if (dc)
	{
		delete dc;
	}
}


BEGIN_MESSAGE_MAP(COpenGLControl, CWnd)
	//{{AFX_MSG_MAP(COpenGLControl)
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Behandlungsroutinen für Nachrichten COpenGLControl 


void COpenGLControl::InitGL()
{
	glShadeModel(GL_SMOOTH);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0f);							
	//glEnable(GL_DEPTH_TEST);					
	glDepthFunc(GL_LEQUAL);	
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
}

void COpenGLControl::DrawGLScene()
{
	glClear(GL_COLOR_BUFFER_BIT
		 |  GL_DEPTH_BUFFER_BIT);


	glLoadIdentity();

	//***************************
	// DRAWING CODE
	//***************************

	glTranslatef(0.0f,0.0f,0.0f);
	glRotatef(rotation,1.0f,1.0f,0.0f);

	glBegin(GL_TRIANGLES);
		glColor3f(1.0f,0.0f,0.0f);
		glVertex3f(1.0f,-1.0f,0.0f);
		glColor3f(0.0f,1.0f,0.0f);
		glVertex3f(-1.0f,-1.0f,0.0f);
		glColor3f(0.0f,0.0f,1.0f);
		glVertex3f(0.0f,1.0f,0.0f);
	glEnd();
/*
	glBegin(GL_QUADS);
		glColor3f(1.0f,0.0f,0.0f);
		glVertex3f(-1.1f,-1.1,0.0f);
		glVertex3f(1.1f,-1.1f,0.0f);
		glVertex3f(1.1f,1.1f,0.0f);
		glVertex3f(-1.1f,1.1f,0.0f);
	glEnd();
*/
	SwapBuffers(dc->m_hDC);
}



void COpenGLControl::Create(CRect rect, CWnd *parent)
{
	CString className = AfxRegisterWndClass(
		CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
		NULL,
		(HBRUSH)GetStockObject(BLACK_BRUSH),
		NULL);

	CreateEx(
		0,
		className,
		"OpenGL",
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		rect,
		parent,
		0);

}

void COpenGLControl::OnPaint() 
{
	rotation += 0.01f;

	if (rotation >= 360.0f)
	{
		rotation -= 360.0f;
	}

	/** OpenGL section **/

	openGLDevice.makeCurrent();

	DrawGLScene();
}

void COpenGLControl::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize(nType, cx, cy);
	
	if (cy == 0)								
	{
		cy = 1;						
	}


	glViewport(0,0,cx,cy);	

	glMatrixMode(GL_PROJECTION);						
	glLoadIdentity();						

	
	glOrtho(-1.0f,1.0f,-1.0f,1.0f,1.0f,-1.0f);
//	gluPerspective(45.0f,cx/cy,0.1f,100.0f);

	glMatrixMode(GL_MODELVIEW);						
	glLoadIdentity();
}


int COpenGLControl::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	dc = new CClientDC(this);

	openGLDevice.create(dc->m_hDC);
	InitGL();

	return 0;
}

BOOL COpenGLControl::OnEraseBkgnd(CDC* pDC) 
{
	return TRUE;
}
