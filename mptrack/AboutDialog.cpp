#include "stdafx.h"
#include "resource.h"
#include "AboutDialog.h"
#include "PNG.h"
#include "../common/version.h"


OPENMPT_NAMESPACE_BEGIN


CRippleBitmap *CRippleBitmap::instance = nullptr;
CAboutDlg *CAboutDlg::instance = nullptr;


BEGIN_MESSAGE_MAP(CRippleBitmap, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEHOVER()
	ON_WM_MOUSELEAVE()
END_MESSAGE_MAP()


CRippleBitmap::CRippleBitmap()
//----------------------------
{
	bitmapSrc = PNG::ReadPNG(MAKEINTRESOURCE(IDB_MPTRACK));
	bitmapTarget = new PNG::Bitmap(bitmapSrc->width, bitmapSrc->height);
	offset1.assign(bitmapSrc->GetNumPixels(), 0);
	offset2.assign(bitmapSrc->GetNumPixels(), 0);
	frontBuf = &offset2[0];
	backBuf = &offset1[0];
	activity = true;
	lastFrame = 0;
	frame = false;
	damp = true;
	showMouse = true;

	// Pre-fill first and last row of output bitmap, since those won't be touched.
	const PNG::Pixel *in1 = bitmapSrc->GetPixels(), *in2 = bitmapSrc->GetPixels() + (bitmapSrc->height - 1) * bitmapSrc->width;
	PNG::Pixel *out1 = bitmapTarget->GetPixels(), *out2 = bitmapTarget->GetPixels() + (bitmapSrc->height - 1) * bitmapSrc->width;
	for(uint32_t i = 0; i < bitmapSrc->width; i++)
	{
		*(out1++) = *(in1++);
		*(out2++) = *(in2++);
	}

	MemsetZero(bi);
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = bitmapSrc->width;
	bi.biHeight = -(int32_t)bitmapSrc->height;
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = bitmapSrc->width * bitmapSrc->height * 4;

	instance = this;
}


CRippleBitmap::~CRippleBitmap()
//-----------------------------
{
	if(!showMouse)
	{
		ShowCursor(TRUE);
	}
	delete bitmapSrc;
	delete bitmapTarget;
	if(instance == this) instance = nullptr;
}


void CRippleBitmap::OnMouseMove(UINT nFlags, CPoint point)
//--------------------------------------------------------
{
	// Initiate ripples at cursor location
	Limit(point.x, 1, int(bitmapSrc->width) - 2);
	Limit(point.y, 2, int(bitmapSrc->height) - 3);
	int32_t *p = backBuf + point.x + point.y * bitmapSrc->width;
	p[0] += (nFlags & MK_LBUTTON) ? 50 : 150;
	p[0] += (nFlags & MK_MBUTTON) ? 150 : 0;

	int32_t w = bitmapSrc->width;
	// Make the initial point of this ripple a bit "fatter".
	p[-1] += p[0] / 2;     p[1] += p[0] / 2;
	p[-w] += p[0] / 2;     p[w] += p[0] / 2;
	p[-w - 1] += p[0] / 4; p[-w + 1] += p[0] / 4;
	p[w - 1] += p[0] / 4;  p[w + 1] += p[0] / 4;

	damp = !(nFlags & MK_RBUTTON);
	activity = true;

	TRACKMOUSEEVENT me;
	me.cbSize = sizeof(TRACKMOUSEEVENT);
	me.dwFlags = TME_LEAVE | TME_HOVER;
	me.hwndTrack = m_hWnd;
	me.dwHoverTime = 1500;

	if(TrackMouseEvent(&me) && showMouse)
	{
		ShowCursor(FALSE);
		showMouse = false;
	}
}


void CRippleBitmap::OnMouseLeave()
//--------------------------------
{
	if(!showMouse)
	{
		ShowCursor(TRUE);
		showMouse = true;
	}
}


void CRippleBitmap::OnPaint()
//---------------------------
{
	CPaintDC dc(this);

	CRect rect;
	GetClientRect(rect);
	StretchDIBits(dc.m_hDC,
		0, 0, rect.Width(), rect.Height(),
		0, 0, bitmapTarget->width, bitmapTarget->height,
		bitmapTarget->GetPixels(),
		reinterpret_cast<BITMAPINFO *>(&bi), DIB_RGB_COLORS, SRCCOPY);
}


bool CRippleBitmap::Animate()
//---------------------------
{
	// Were there any pixels being moved in the last frame?
	if(!activity)
		return false;

	DWORD now = timeGetTime();
	if(now - lastFrame < 15)
		return true;
	lastFrame = now;
	activity = false;

	frontBuf = &(frame ? offset2 : offset1)[0];
	backBuf = &(frame ? offset1 : offset2)[0];

	// Spread the ripples...
	const int32_t w = bitmapSrc->width, h = bitmapSrc->height;
	const int32_t numPixels = w * (h - 2);
	const int32_t *back = backBuf + w;
	int32_t *front = frontBuf + w;
	for(int32_t i = numPixels; i != 0; i--, back++, front++)
	{
		(*front) = (back[-1] + back[1] + back[w] + back[-w]) / 2 - (*front);
		if(damp) (*front) -= (*front) >> 5;
	}

	// ...and compute the final picture.
	const int32_t *offset = frontBuf + w;
	const PNG::Pixel *pixelIn = bitmapSrc->GetPixels() + w;
	PNG::Pixel *pixelOut = bitmapTarget->GetPixels() + w;
	PNG::Pixel *limitMin = bitmapSrc->GetPixels(), *limitMax = bitmapSrc->GetPixels() + bitmapSrc->GetNumPixels() - 1;
	for(int32_t i = numPixels; i != 0; i--, pixelIn++, pixelOut++, offset++)
	{
		// Compute pixel displacement
		const int32_t xOff = offset[-1] - offset[1];
		const int32_t yOff = offset[-w] - offset[w];

		if(xOff | yOff)
		{
			const PNG::Pixel *p = pixelIn + xOff + yOff * w;
			Limit(p, limitMin, limitMax);
			// Add a bit of shading depending on how far we're displacing the pixel...
			pixelOut->r = (uint8_t)Clamp(p->r + (p->r * xOff) / 32, 0, 255);
			pixelOut->g = (uint8_t)Clamp(p->g + (p->g * xOff) / 32, 0, 255);
			pixelOut->b = (uint8_t)Clamp(p->b + (p->b * xOff) / 32, 0, 255);
			// ...and mix it with original picture
			pixelOut->r = (pixelOut->r + pixelIn->r) / 2u;
			pixelOut->g = (pixelOut->g + pixelIn->g) / 2u;
			pixelOut->b = (pixelOut->b + pixelIn->b) / 2u;
			// And now some cheap image smoothing...
			pixelOut[-1].r = (pixelOut->r + pixelOut[-1].r) / 2u;
			pixelOut[-1].g = (pixelOut->g + pixelOut[-1].g) / 2u;
			pixelOut[-1].b = (pixelOut->b + pixelOut[-1].b) / 2u;
			pixelOut[-w].r = (pixelOut->r + pixelOut[-w].r) / 2u;
			pixelOut[-w].g = (pixelOut->g + pixelOut[-w].g) / 2u;
			pixelOut[-w].b = (pixelOut->b + pixelOut[-w].b) / 2u;
			activity = true;	// Also use this to update activity status...
		} else
		{
			*pixelOut = *pixelIn;
		}
	}

	frame = !frame;

	InvalidateRect(NULL, FALSE);
	UpdateWindow();
	Sleep(10); 	//give away some CPU

	return true;
}


CAboutDlg::~CAboutDlg()
//---------------------
{
	instance = NULL;
}

void CAboutDlg::OnOK()
//--------------------
{
	instance = NULL;
	DestroyWindow();
	delete this;
}


void CAboutDlg::OnCancel()
//------------------------
{
	OnOK();
}


BOOL CAboutDlg::OnInitDialog()
//----------------------------
{
	CDialog::OnInitDialog();

	m_bmp.SubclassDlgItem(IDC_BITMAP1, this);

	SetDlgItemText(IDC_EDIT2, CString("Build Date: ") + MptVersion::GetBuildDateString().c_str());
	SetDlgItemText(IDC_EDIT3, CString("OpenMPT ") + MptVersion::GetVersionStringExtended().c_str());
	m_static.SubclassDlgItem(IDC_CREDITS,this);
	m_static.SetCredits((mpt::String::Replace(mpt::ToCharset(mpt::CharsetLocale, mpt::CharsetUTF8, MptVersion::GetFullCreditsString()), "\n", "|") + "|" + mpt::String::Replace(MptVersion::GetContactString(), "\n", "|" ) + "||||||").c_str());
	m_static.SetSpeed(DISPLAY_SLOW);
	m_static.SetColor(BACKGROUND_COLOR, RGB(138, 165, 219)); // Background Colour
	m_static.SetTransparent(); // Set parts of bitmaps with RGB(192,192,192) transparent
	m_static.SetGradient(GRADIENT_LEFT_DARK);  // Background goes from blue to black from left to right
	// m_static.SetBkImage(IDB_BITMAP1); // Background image
	m_static.StartScrolling();
	return TRUE;	// return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


OPENMPT_NAMESPACE_END
