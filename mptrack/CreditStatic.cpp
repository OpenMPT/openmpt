// CreditStatic.cpp : implementation file
//

#include "stdafx.h"
#include "CreditStatic.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define  DISPLAY_TIMER_ID		150		// timer id
/////////////////////////////////////////////////////////////////////////////
// CCreditStatic

CCreditStatic::CCreditStatic()
{

	m_Colors[0] = RGB(0,0,0);       // Black
	m_Colors[1] = RGB(255,0,0);     // Red
	m_Colors[2] = RGB(255,255,0);   // Yellow
	m_Colors[3] = RGB(0,255,255);   // Turquoise
	m_Colors[4] = RGB(255,255,255); // White

	m_TextHeights[0] = 21;
	m_TextHeights[1] = 19;
	m_TextHeights[2] = 17;
	m_TextHeights[3] = 15;
	m_nCurrentFontHeight = m_TextHeights[NORMAL_TEXT_HEIGHT];


	m_Escapes[0] = '\t';
	m_Escapes[1] = '\n';
	m_Escapes[2] = '\r';
	m_Escapes[3] = '^';

	m_DisplaySpeed[0] = 70;
	m_DisplaySpeed[1] = 40;
	m_DisplaySpeed[2] = 10;

	m_CurrentSpeed = 1;
	m_ScrollAmount = -1;
	m_bProcessingBitmap = FALSE;

	m_ArrIndex = NULL;
	m_nCounter = 1;
	m_nClip = 0;

	m_bFirstTime = TRUE;
	m_bDrawText = FALSE;
	m_bFirstTurn = TRUE;
	m_Gradient = GRADIENT_NONE;
	m_bTransparent = FALSE;
	n_MaxWidth = 0;
	TimerOn = 0;
}

CCreditStatic::~CCreditStatic()
{
}


BEGIN_MESSAGE_MAP(CCreditStatic, CStatic)
	//{{AFX_MSG_MAP(CCreditStatic)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_TIMER()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCreditStatic message handlers

BOOL CCreditStatic::StartScrolling()
{
	if(m_ArrCredit.IsEmpty())
		return FALSE;

	if(m_BmpMain.m_hObject != NULL) {
		m_BmpMain.DeleteObject();
		m_BmpMain.m_hObject = NULL;
	}
	
	TimerOn = SetTimer(DISPLAY_TIMER_ID,m_DisplaySpeed[m_CurrentSpeed],NULL);
    ASSERT(TimerOn != 0);

	m_ArrIndex = m_ArrCredit.GetHeadPosition();
	m_nCounter = 1;
	m_nClip = 0;

	m_bFirstTime = TRUE;
	m_bDrawText = FALSE;

	return TRUE;
}

void CCreditStatic::EndScrolling()
{
	KillTimer(DISPLAY_TIMER_ID);
	TimerOn = 0;

	if(m_BmpMain.m_hObject != NULL) {
		m_BmpMain.DeleteObject();
		m_BmpMain.m_hObject = NULL;
	}
}

void CCreditStatic::SetCredits(LPCTSTR credits,char delimiter)
{
	char *str,*ptr1,*ptr2;
    
	ASSERT(credits);

	if((str = strdup(credits)) == NULL)
		return;

	m_ArrCredit.RemoveAll();

	ptr1 = str;
	while((ptr2 = strchr(ptr1,delimiter)) != NULL) {
		*ptr2 = '\0';
		m_ArrCredit.AddTail(ptr1);
		ptr1 = ptr2+1;
	}
	m_ArrCredit.AddTail(ptr1);

	free(str);

	m_ArrIndex = m_ArrCredit.GetHeadPosition();
	m_nCounter = 1;
	m_nClip = 0;

	m_bFirstTime = TRUE;
	m_bDrawText = FALSE;
}

void CCreditStatic::SetCredits(UINT nID,char delimiter)
{
	CString credits;
	if(!credits.LoadString(nID))
		return;

	SetCredits((LPCTSTR)credits, delimiter);
}

void CCreditStatic::SetSpeed(UINT index, int speed)
{
	ASSERT(index <= DISPLAY_FAST);

	if(speed)
		m_DisplaySpeed[index] = speed;

	m_CurrentSpeed = index;
}

void CCreditStatic::SetColor(UINT index, COLORREF col)
{
	ASSERT(index <= NORMAL_TEXT_COLOR);

	m_Colors[index] = col;
}

void CCreditStatic::SetTextHeight(UINT index, int height)
{
	ASSERT(index <= NORMAL_TEXT_HEIGHT);

	m_TextHeights[index] = height;
}

void CCreditStatic::SetEscape(UINT index, char escape)
{
	ASSERT(index <= DISPLAY_BITMAP);

	m_Escapes[index] = escape;
}

void CCreditStatic::SetGradient(UINT value)
{
	ASSERT(value <= GRADIENT_LEFT_LIGHT);

	m_Gradient = value;
}

void CCreditStatic::SetTransparent(BOOL bTransparent)
{
	m_bTransparent = bTransparent;
}

void CCreditStatic::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	if(TimerOn) return;

	CDC memDC;
	memDC.CreateCompatibleDC(&dc);

	CBitmap *pOldMemDCBitmap = NULL;
	CRect m_ScrollRect;
	GetClientRect(&m_ScrollRect);

	if(m_BmpMain.m_hObject == NULL) {

		CDC memDC2;
		CBitmap bitmap;
		memDC2.CreateCompatibleDC(&dc);
		bitmap.CreateCompatibleBitmap( &dc, m_ScrollRect.Width(), m_ScrollRect.Height() );
		CBitmap *pOldMemDC2Bitmap = (CBitmap*)memDC2.SelectObject(&bitmap);
		
		DrawCredit(&memDC2, m_ScrollRect);
		AddBackGround(&memDC2, m_ScrollRect, m_ScrollRect);

		pOldMemDCBitmap = (CBitmap*)memDC.SelectObject(&m_BmpMain);
        memDC.BitBlt( 0, 0, m_ScrollRect.Width(), m_ScrollRect.Height(), 
                                        &memDC2, 0, 0, SRCCOPY );
		memDC2.SelectObject(pOldMemDC2Bitmap);
	}
	else
		pOldMemDCBitmap = (CBitmap*)memDC.SelectObject(&m_BmpMain);
       
	dc.BitBlt( 0, 0, m_ScrollRect.Width(), m_ScrollRect.Height(), 
               &memDC, 0, 0, SRCCOPY );
}

BOOL CCreditStatic::OnEraseBkgnd(CDC* pDC) 
{
	return TRUE;
	
//	return CStatic::OnEraseBkgnd(pDC);
}

//************************************************************************
//	 OnTimer
//
//	 	On each of the display timers, scroll the window 1 unit. Each 20
//      units, fetch the next array element and load into work string. Call
//      Invalidate and UpdateWindow to invoke the OnPaint which will paint 
//      the contents of the newly updated work string.
//************************************************************************
void CCreditStatic::OnTimer(UINT nIDEvent) 
{
	if (nIDEvent != DISPLAY_TIMER_ID)
	{
		CStatic::OnTimer(nIDEvent);
		return;
	}

	BOOL bCheck = FALSE;

	if (!m_bProcessingBitmap) {
		if (m_nCounter++ % m_nCurrentFontHeight == 0)	 // every x timer events, show new line
		{
			m_nCounter=1;
			m_szWork = m_ArrCredit.GetNext(m_ArrIndex);
			if(m_bFirstTurn)
				bCheck = TRUE;
			if(m_ArrIndex == NULL) {
				m_bFirstTurn = FALSE;
				m_ArrIndex = m_ArrCredit.GetHeadPosition();
			}
			m_nClip = 0;
			m_bDrawText=TRUE;
		}
	}
	
    CClientDC dc(this);
	CRect m_ScrollRect;
	GetClientRect(&m_ScrollRect);
 
	CRect m_ClientRect(m_ScrollRect);
	m_ClientRect.left = (m_ClientRect.Width()-n_MaxWidth)/2;
	m_ClientRect.right = m_ClientRect.left + n_MaxWidth;

	MoveCredit(&dc, m_ScrollRect, m_ClientRect, bCheck);

	AddBackGround(&dc, m_ScrollRect, m_ClientRect);

	CStatic::OnTimer(nIDEvent);
}

void CCreditStatic::AddBackGround(CDC* pDC, CRect& m_ScrollRect, CRect& m_ClientRect)
{
	CDC memDC;
	memDC.CreateCompatibleDC( pDC );

    if( m_bitmap.m_hObject == NULL )
	{
        CBitmap* pOldBitmap = memDC.SelectObject( &m_BmpMain );
        pDC->BitBlt( 0, 0, m_ScrollRect.Width(), m_ScrollRect.Height(), 
                                        &memDC, 0, 0, SRCCOPY );
		memDC.SelectObject(pOldBitmap);
		return;
	}

   // Draw bitmap in the background if one has been set
                // Now create a mask
	CBitmap bitmap;
	bitmap.CreateCompatibleBitmap( pDC, m_ClientRect.Width(), m_ClientRect.Height() );
	CBitmap* pOldMemDCBitmap = memDC.SelectObject( &bitmap );
		
	CDC tempDC;
	tempDC.CreateCompatibleDC(pDC);
	CBitmap* pOldTempDCBitmap = tempDC.SelectObject( &m_BmpMain );

	memDC.BitBlt( 0, 0, m_ClientRect.Width(), m_ClientRect.Height(), &tempDC, 
                      m_ClientRect.left, m_ClientRect.top, SRCCOPY );
	CDC maskDC;
	maskDC.CreateCompatibleDC(pDC);
	CBitmap maskBitmap;

	// Create monochrome bitmap for the mask
	maskBitmap.CreateBitmap( m_ClientRect.Width(), m_ClientRect.Height(), 1, 1, NULL );
	CBitmap* pOldMaskDCBitmap = maskDC.SelectObject( &maskBitmap );
    memDC.SetBkColor(m_bTransparent? RGB(192,192,192): m_Colors[BACKGROUND_COLOR]);

	// Create the mask from the memory DC
	maskDC.BitBlt( 0, 0, m_ClientRect.Width(), m_ClientRect.Height(), &memDC, 0, 0, SRCCOPY );

	tempDC.SelectObject(pOldTempDCBitmap);
	pOldTempDCBitmap = tempDC.SelectObject( &m_bitmap );

	CDC imageDC;
	CBitmap bmpImage;
	imageDC.CreateCompatibleDC( pDC );
	bmpImage.CreateCompatibleBitmap( pDC, m_ScrollRect.Width(), m_ScrollRect.Height() );
	CBitmap* pOldImageDCBitmap = imageDC.SelectObject( &bmpImage );

	if( pDC->GetDeviceCaps(RASTERCAPS) & RC_PALETTE && m_pal.m_hObject != NULL )
	{
		pDC->SelectPalette( &m_pal, FALSE );
		pDC->RealizePalette();

		imageDC.SelectPalette( &m_pal, FALSE );
	}
	// Get x and y offset
	// Draw bitmap in tiled manner to imageDC
	for( int i = 0; i < m_ScrollRect.right; i += m_cxBitmap )
		for( int j = 0; j < m_ScrollRect.bottom; j += m_cyBitmap )
			imageDC.BitBlt( i, j, m_cxBitmap, m_cyBitmap, &tempDC, 0, 0, SRCCOPY );

	// Set the background in memDC to black. Using SRCPAINT with black and any other
	// color results in the other color, thus making black the transparent color
	memDC.SetBkColor(RGB(0,0,0));
	memDC.SetTextColor(RGB(255,255,255));
	memDC.BitBlt(0, 0, m_ClientRect.Width(), m_ClientRect.Height(), &maskDC, 0, 0, SRCAND);

	// Set the foreground to black. See comment above.
	imageDC.SetBkColor(RGB(255,255,255));
	imageDC.SetTextColor(RGB(0,0,0));
	imageDC.BitBlt(m_ClientRect.left, m_ClientRect.top, m_ClientRect.Width(), m_ClientRect.Height(), 
					&maskDC, 0, 0, SRCAND);

	// Combine the foreground with the background
    imageDC.BitBlt(m_ClientRect.left, m_ClientRect.top, m_ClientRect.Width(), m_ClientRect.Height(), 
					&memDC, 0, 0,SRCPAINT);

	// Draw the final image to the screen   
	pDC->BitBlt( 0, 0, m_ScrollRect.Width(), m_ScrollRect.Height(), 
					&imageDC, 0, 0, SRCCOPY );

	imageDC.SelectObject(pOldImageDCBitmap);
	maskDC.SelectObject(pOldMaskDCBitmap);
	tempDC.SelectObject(pOldTempDCBitmap);
	memDC.SelectObject(pOldMemDCBitmap);
}

void CCreditStatic::DrawBitmap(CDC* pDC, CDC* pDC2, CRect *rBitmap)
{
	if(!m_bTransparent || m_bitmap.m_hObject != NULL) {
    	pDC->BitBlt( rBitmap->left, rBitmap->top, rBitmap->Width(), rBitmap->Height(), 
           			pDC2, 0, 0, SRCCOPY );
		return;
	}

	CDC memDC;
	memDC.CreateCompatibleDC( pDC );

    // Now create a mask
	CBitmap bitmap;
	bitmap.CreateCompatibleBitmap( pDC, rBitmap->Width(), rBitmap->Height() );
	CBitmap* pOldMemDCBitmap = memDC.SelectObject( &bitmap );
		
	memDC.BitBlt( 0, 0, rBitmap->Width(), rBitmap->Height(), pDC2, 0, 0, SRCCOPY );

	CDC maskDC;
	maskDC.CreateCompatibleDC(pDC);

	// Create monochrome bitmap for the mask
	CBitmap maskBitmap;
	maskBitmap.CreateBitmap( rBitmap->Width(), rBitmap->Height(), 1, 1, NULL );
	CBitmap* pOldMaskDCBitmap = maskDC.SelectObject( &maskBitmap );
    memDC.SetBkColor(RGB(192,192,192));

	// Create the mask from the memory DC
	maskDC.BitBlt( 0, 0, rBitmap->Width(), rBitmap->Height(), &memDC, 0, 0, SRCCOPY );


	CDC imageDC;
	CBitmap bmpImage;
	imageDC.CreateCompatibleDC( pDC );
	bmpImage.CreateCompatibleBitmap( pDC, rBitmap->Width(), rBitmap->Height() );
	CBitmap* pOldImageDCBitmap = imageDC.SelectObject( &bmpImage );

	imageDC.BitBlt(0, 0, rBitmap->Width(), rBitmap->Height(), pDC, rBitmap->left, rBitmap->top, SRCCOPY);

	// Set the background in memDC to black. Using SRCPAINT with black and any other
	// color results in the other color, thus making black the transparent color
	memDC.SetBkColor(RGB(0,0,0));
	memDC.SetTextColor(RGB(255,255,255));
	memDC.BitBlt(0, 0, rBitmap->Width(), rBitmap->Height(), &maskDC, 0, 0, SRCAND);

	// Set the foreground to black. See comment above.
	imageDC.SetBkColor(RGB(255,255,255));
	imageDC.SetTextColor(RGB(0,0,0));
	imageDC.BitBlt(0, 0, rBitmap->Width(), rBitmap->Height(), &maskDC, 0, 0, SRCAND);

	// Combine the foreground with the background
    imageDC.BitBlt(0, 0, rBitmap->Width(), rBitmap->Height(), &memDC, 0, 0,SRCPAINT);

	// Draw the final image to the screen   
	pDC->BitBlt( rBitmap->left, rBitmap->top, rBitmap->Width(), rBitmap->Height(), 
					&imageDC, 0, 0, SRCCOPY );

	imageDC.SelectObject(pOldImageDCBitmap);
	maskDC.SelectObject(pOldMaskDCBitmap);
	memDC.SelectObject(pOldMemDCBitmap);
}

void CCreditStatic::FillGradient(CDC *pDC, CRect *m_ScrollRect, CRect *m_FillRect, COLORREF color)
{ 
	float fStep,fRStep,fGStep,fBStep;	    // How large is each band?
	int iOnBand;  // Loop index

	WORD R = GetRValue(color);
	WORD G = GetGValue(color);
	WORD B = GetBValue(color);

	// Determine how large each band should be in order to cover the
	// client with 256 bands (one for every color intensity level)
	if(m_Gradient % 2) {
		fRStep = (float)R / 255.0f;
		fGStep = (float)G / 255.0f;
		fBStep = (float)B / 255.0f;
	} else {
		fRStep = (float)(255-R) / 255.0f;
		fGStep = (float)(255-G) / 255.0f;
		fBStep = (float)(255-B) / 255.0f;
	}

	COLORREF OldCol = pDC->GetBkColor();
	// Start filling bands
	fStep = (float)m_ScrollRect->Width() / 256.0f;
	for(iOnBand = (256*m_FillRect->left)/m_ScrollRect->Width(); 
		(int)(iOnBand*fStep) < m_FillRect->right && iOnBand < 256; iOnBand++) {
		CRect r((int)(iOnBand * fStep), m_FillRect->top,
				(int)((iOnBand+1) * fStep), m_FillRect->bottom+1);
		COLORREF col;

		switch(m_Gradient) {
		case GRADIENT_RIGHT_DARK:
			col = RGB((int)(R-iOnBand*fRStep),(int)(G-iOnBand*fGStep),(int)(B-iOnBand*fBStep));
			break;
		case GRADIENT_RIGHT_LIGHT:
			col = RGB((int)(R+iOnBand*fRStep),(int)(G+iOnBand*fGStep),(int)(B+iOnBand*fBStep));
			break;
		case GRADIENT_LEFT_DARK:
			col = RGB((int)(iOnBand*fRStep),(int)(iOnBand*fGStep),(int)(iOnBand*fBStep));
			break;
		case GRADIENT_LEFT_LIGHT:
			col = RGB(255-(int)(iOnBand*fRStep),255-(int)(iOnBand*fGStep),255-(int)(iOnBand*fBStep));
			break;
		default:
			return;
		}
		pDC->FillSolidRect(&r, col);
	}
	pDC->SetBkColor(OldCol);
} 

#define SCROLLDC

void CCreditStatic::MoveCredit(CDC* pDC, CRect& m_ScrollRect, CRect& m_ClientRect, BOOL bCheck)
{
	CDC memDC,memDC2;
    memDC.CreateCompatibleDC(pDC);
    memDC2.CreateCompatibleDC(pDC);
    
	COLORREF BackColor = (m_bTransparent && m_bitmap.m_hObject != NULL)? RGB(192,192,192) : m_Colors[BACKGROUND_COLOR];
	CBitmap *pOldMemDCBitmap = NULL;
	CBitmap	*pOldMemDC2Bitmap = NULL;

#ifdef SCROLLDC
	CRect r1;
#endif

	if(m_BmpMain.m_hObject == NULL) {
		m_BmpMain.CreateCompatibleBitmap( pDC, m_ScrollRect.Width(), m_ScrollRect.Height() );
		pOldMemDCBitmap = (CBitmap*)memDC.SelectObject(&m_BmpMain);
		if(m_Gradient && m_bitmap.m_hObject == NULL)
			FillGradient(&memDC, &m_ScrollRect, &m_ScrollRect,m_Colors[BACKGROUND_COLOR]);
		else
			memDC.FillSolidRect(&m_ScrollRect,BackColor);
	} else 
		pOldMemDCBitmap = (CBitmap*)memDC.SelectObject(&m_BmpMain);

	if(m_ClientRect.Width() > 0) {
#ifndef SCROLLDC
		CBitmap bitmap;
		bitmap.CreateCompatibleBitmap( pDC, m_ClientRect.Width(), m_ClientRect.Height() );
		pOldMemDC2Bitmap = memDC2.SelectObject(&bitmap);

		memDC2.BitBlt( 0, 0, m_ClientRect.Width(), m_ClientRect.Height()-abs(m_ScrollAmount), 
           	 &memDC, m_ClientRect.left, abs(m_ScrollAmount), SRCCOPY );
		memDC.BitBlt( m_ClientRect.left, 0, m_ClientRect.Width(), m_ClientRect.Height(), 
           	 &memDC2, 0, 0, SRCCOPY );

		
		memDC2.SelectObject(pOldMemDC2Bitmap);
		pOldMemDC2Bitmap = NULL;
#else
		CRgn RgnUpdate;
		memDC.ScrollDC(0,m_ScrollAmount,(LPCRECT)m_ScrollRect,(LPCRECT)m_ClientRect,&RgnUpdate,
						(LPRECT)r1);
    }
	else {
		r1 = m_ScrollRect;
		r1.top = r1.bottom-abs(m_ScrollAmount);
#endif
	}

	m_nClip = m_nClip + abs(m_ScrollAmount);	
	

	//*********************************************************************
	//	FONT SELECTION
    CFont m_fntArial;
	CFont* pOldFont = NULL;
	BOOL bSuccess = FALSE;
	
	BOOL bUnderline;
	BOOL bItalic;
    int rmcode = 0;

	if (!m_szWork.IsEmpty()) {
		char c = m_szWork[m_szWork.GetLength()-1];
		if(c == m_Escapes[TOP_LEVEL_GROUP]) {
			rmcode = 1;
			bItalic = FALSE;
			bUnderline = FALSE;
			m_nCurrentFontHeight = m_TextHeights[TOP_LEVEL_GROUP_HEIGHT];
   			bSuccess = m_fntArial.CreateFont(m_TextHeights[TOP_LEVEL_GROUP_HEIGHT], 0, 0, 0, 
   								FW_BOLD, bItalic, bUnderline, 0, 
   								ANSI_CHARSET,
                   	OUT_DEFAULT_PRECIS,
                   	CLIP_DEFAULT_PRECIS,
                   	PROOF_QUALITY,
                   	VARIABLE_PITCH | 0x04 | FF_DONTCARE,
                   	(LPSTR)"Arial");
			memDC.SetTextColor(m_Colors[TOP_LEVEL_GROUP_COLOR]);
			if (pOldFont != NULL) memDC.SelectObject(pOldFont);
			pOldFont = memDC.SelectObject(&m_fntArial);
			
		}
		else if(c == m_Escapes[GROUP_TITLE]) {
			rmcode = 1;
			bItalic = FALSE;
			bUnderline = FALSE;
			m_nCurrentFontHeight = m_TextHeights[GROUP_TITLE_HEIGHT];
   			bSuccess = m_fntArial.CreateFont(m_TextHeights[GROUP_TITLE_HEIGHT], 0, 0, 0, 
   								FW_BOLD, bItalic, bUnderline, 0, 
   								ANSI_CHARSET,
                   	OUT_DEFAULT_PRECIS,
                   	CLIP_DEFAULT_PRECIS,
                   	PROOF_QUALITY,
                   	VARIABLE_PITCH | 0x04 | FF_DONTCARE,
                   	(LPSTR)"Arial");
			memDC.SetTextColor(m_Colors[GROUP_TITLE_COLOR]);
			if (pOldFont != NULL) memDC.SelectObject(pOldFont);
			pOldFont = memDC.SelectObject(&m_fntArial);
		}
		else if(c == m_Escapes[TOP_LEVEL_TITLE]) {
			rmcode = 1;
			bItalic = FALSE;
//			bUnderline = TRUE;
			bUnderline = FALSE;
			m_nCurrentFontHeight = m_TextHeights[TOP_LEVEL_TITLE_HEIGHT];
			bSuccess = m_fntArial.CreateFont(m_TextHeights[TOP_LEVEL_TITLE_HEIGHT], 0, 0, 0, 
								FW_BOLD, bItalic, bUnderline, 0, 
								ANSI_CHARSET,
	               	OUT_DEFAULT_PRECIS,
	               	CLIP_DEFAULT_PRECIS,
	               	PROOF_QUALITY,
	               	VARIABLE_PITCH | 0x04 | FF_DONTCARE,
	               	(LPSTR)"Arial");
			memDC.SetTextColor(m_Colors[TOP_LEVEL_TITLE_COLOR]);
			if (pOldFont != NULL) memDC.SelectObject(pOldFont);
			pOldFont = memDC.SelectObject(&m_fntArial);
		}
		else if(c == m_Escapes[DISPLAY_BITMAP]) {
			if (!m_bProcessingBitmap)
			{
				CString szBitmap = m_szWork.Left(m_szWork.GetLength()-1);
				if(m_bmpWork.LoadBitmap((const char *)szBitmap)) {
					BITMAP 		m_bmpInfo;

	   				m_bmpWork.GetObject(sizeof(BITMAP), &m_bmpInfo);
			
					m_size.cx = m_bmpInfo.bmWidth;	// width  of dest rect
					m_size.cy = m_bmpInfo.bmHeight;
					// upper left point of dest
					m_pt.x = (m_ClientRect.right - 
							((m_ClientRect.Width())/2) - (m_size.cx/2));
					m_pt.y = m_ClientRect.bottom;
				
					m_bProcessingBitmap = TRUE;
					if (pOldMemDC2Bitmap != NULL) memDC2.SelectObject(pOldMemDC2Bitmap);
					pOldMemDC2Bitmap = memDC2.SelectObject(&m_bmpWork);
				}
				else
					c = ' ';
			}
			else {
				if (pOldMemDC2Bitmap != NULL) memDC2.SelectObject(pOldMemDC2Bitmap);
				pOldMemDC2Bitmap = memDC2.SelectObject(&m_bmpWork);
			}
		}
		else {
			bItalic = FALSE;
			bUnderline = FALSE;
			m_nCurrentFontHeight = m_TextHeights[NORMAL_TEXT_HEIGHT];
   			bSuccess = m_fntArial.CreateFont(m_TextHeights[NORMAL_TEXT_HEIGHT], 0, 0, 0, 
   								FW_THIN, bItalic, bUnderline, 0, 
   								ANSI_CHARSET,
                   	OUT_DEFAULT_PRECIS,
                   	CLIP_DEFAULT_PRECIS,
                   	PROOF_QUALITY,
                   	VARIABLE_PITCH | 0x04 | FF_DONTCARE,
                   	(LPSTR)"Arial");
			memDC.SetTextColor(m_Colors[NORMAL_TEXT_COLOR]);
			if (pOldFont != NULL) memDC.SelectObject(pOldFont);
			pOldFont = memDC.SelectObject(&m_fntArial);
		}
	}

#ifndef SCROLLDC
	CRect r1(m_ScrollRect);
	r1.top = r1.bottom-abs(m_ScrollAmount);
#endif

	if(m_Gradient && m_bitmap.m_hObject == NULL)
		FillGradient(&memDC, &m_ScrollRect, &r1, m_Colors[BACKGROUND_COLOR]);
	else
		memDC.FillSolidRect(&r1,BackColor);
	memDC.SetBkMode(TRANSPARENT);

	if (!m_bProcessingBitmap)
	{
		if(bCheck) {
			CSize size = memDC.GetTextExtent((LPCTSTR)m_szWork,m_szWork.GetLength()-rmcode);
			if(size.cx > n_MaxWidth) {
				n_MaxWidth = (size.cx > m_ScrollRect.Width())? m_ScrollRect.Width():size.cx;
				m_ClientRect.left = (m_ScrollRect.Width()-n_MaxWidth)/2;
				m_ClientRect.right = m_ClientRect.left + n_MaxWidth;
			}
				
		}
		CRect r(m_ClientRect);
		r.top = r.bottom-m_nClip;
		int x = memDC.DrawText((const char *)m_szWork,m_szWork.GetLength()-rmcode,&r,DT_TOP|DT_CENTER|
					DT_NOPREFIX | DT_SINGLELINE);	
		m_bDrawText=FALSE;
	}
	else
	{
		if(bCheck) {
			CSize size = memDC.GetTextExtent((LPCTSTR)m_szWork,m_szWork.GetLength()-rmcode);
			if(m_size.cx > n_MaxWidth) {
				n_MaxWidth = (m_size.cx > m_ScrollRect.Width())? m_ScrollRect.Width():m_size.cx;
				m_ClientRect.left = (m_ScrollRect.Width()-n_MaxWidth)/2;
				m_ClientRect.right = m_ClientRect.left + n_MaxWidth;
			}
		}
		CRect r( m_pt.x, m_pt.y-m_nClip, m_pt.x+ m_size.cx, m_pt.y);
		DrawBitmap(&memDC, &memDC2, &r);
//    	memDC.BitBlt( m_pt.x, m_pt.y-m_nClip, m_size.cx, m_nClip, 
//           		&memDC2, 0, 0, SRCCOPY );
		if (m_nClip >= m_size.cy)
		{
			m_bmpWork.DeleteObject();
			m_bProcessingBitmap = FALSE;
			m_nClip=0;
			m_szWork.Empty();
			m_nCounter=1;
		}
	}

	if (pOldMemDC2Bitmap != NULL) memDC2.SelectObject(pOldMemDC2Bitmap);
	if (pOldFont != NULL) memDC.SelectObject(pOldFont);
	memDC.SelectObject(pOldMemDCBitmap);
}

void CCreditStatic::DrawCredit(CDC* pDC, CRect& m_ScrollRect)
{
	if(m_BmpMain.m_hObject != NULL) return;

	CDC memDC,memDC2;
    memDC.CreateCompatibleDC(pDC);
    memDC2.CreateCompatibleDC(pDC);
	COLORREF BackColor = (m_bTransparent && m_bitmap.m_hObject != NULL)? RGB(192,192,192) : m_Colors[BACKGROUND_COLOR];
    
	CBitmap *pOldMemDCBitmap = NULL;

	m_BmpMain.CreateCompatibleBitmap( pDC, m_ScrollRect.Width(), m_ScrollRect.Height() );
	pOldMemDCBitmap = (CBitmap*)memDC.SelectObject(&m_BmpMain);

	if(m_Gradient && m_bitmap.m_hObject == NULL)
		FillGradient(&memDC, &m_ScrollRect, &m_ScrollRect, m_Colors[BACKGROUND_COLOR]);
	else
		memDC.FillSolidRect(&m_ScrollRect, BackColor);

	POSITION pos = m_ArrCredit.GetHeadPosition();
	int height = 0;
	while(pos != NULL && height <= m_ScrollRect.Height()) {
		CString m_szWork = m_ArrCredit.GetNext(pos);
		CFont	m_fntArial;
		CFont	*pOldFont = NULL;
		CBitmap	*pOldMemDC2Bitmap = NULL;
		
		CDC memDC2;
		memDC2.CreateCompatibleDC(pDC);
		//*********************************************************************
		//	FONT SELECTION
	
	
		BOOL bSuccess = FALSE;
		BOOL bIsBitmap = FALSE;
		
		BOOL bUnderline;
		BOOL bItalic;
		int rmcode = 0;
        CBitmap bitmap;

		if (!m_szWork.IsEmpty()) {
			char c = m_szWork[m_szWork.GetLength()-1];
			if(c == m_Escapes[TOP_LEVEL_GROUP]) {
				rmcode = 1;
				bItalic = FALSE;
				bUnderline = FALSE;
				m_nCurrentFontHeight = m_TextHeights[TOP_LEVEL_GROUP_HEIGHT];
   				bSuccess = m_fntArial.CreateFont(m_TextHeights[TOP_LEVEL_GROUP_HEIGHT], 0, 0, 0, 
   								FW_BOLD, bItalic, bUnderline, 0, 
   								ANSI_CHARSET,
                   	OUT_DEFAULT_PRECIS,
                   	CLIP_DEFAULT_PRECIS,
                   	PROOF_QUALITY,
                   	VARIABLE_PITCH | 0x04 | FF_DONTCARE,
                   	(LPSTR)"Arial");
				memDC.SetTextColor(m_Colors[TOP_LEVEL_GROUP_COLOR]);
				pOldFont = memDC.SelectObject(&m_fntArial);
		
			}
			else if(c == m_Escapes[GROUP_TITLE]) {
				rmcode = 1;
				bItalic = FALSE;
				bUnderline = FALSE;
				m_nCurrentFontHeight = m_TextHeights[GROUP_TITLE_HEIGHT];
   				bSuccess = m_fntArial.CreateFont(m_TextHeights[GROUP_TITLE_HEIGHT], 0, 0, 0, 
   								FW_BOLD, bItalic, bUnderline, 0, 
   								ANSI_CHARSET,
                   	OUT_DEFAULT_PRECIS,
                   	CLIP_DEFAULT_PRECIS,
                   	PROOF_QUALITY,
                   	VARIABLE_PITCH | 0x04 | FF_DONTCARE,
                   	(LPSTR)"Arial");
				memDC.SetTextColor(m_Colors[GROUP_TITLE_COLOR]);
				pOldFont = memDC.SelectObject(&m_fntArial);
			}
			else if(c == m_Escapes[TOP_LEVEL_TITLE]) {
				rmcode = 1;
				bItalic = FALSE;
	//			bUnderline = TRUE;
				bUnderline = FALSE;
				m_nCurrentFontHeight = m_TextHeights[TOP_LEVEL_TITLE_HEIGHT];
				bSuccess = m_fntArial.CreateFont(m_TextHeights[TOP_LEVEL_TITLE_HEIGHT], 0, 0, 0, 
								FW_BOLD, bItalic, bUnderline, 0, 
								ANSI_CHARSET,
	               	OUT_DEFAULT_PRECIS,
	               	CLIP_DEFAULT_PRECIS,
	               	PROOF_QUALITY,
	               	VARIABLE_PITCH | 0x04 | FF_DONTCARE,
	               	(LPSTR)"Arial");
				memDC.SetTextColor(m_Colors[TOP_LEVEL_TITLE_COLOR]);
				pOldFont = memDC.SelectObject(&m_fntArial);
			}
			else if(c == m_Escapes[DISPLAY_BITMAP]) {
				CString szBitmap = m_szWork.Left(m_szWork.GetLength()-1);
				if(bitmap.LoadBitmap((const char *)szBitmap)) {
					BITMAP 		m_bmpInfo;

	   				bitmap.GetObject(sizeof(BITMAP), &m_bmpInfo);
			
					m_size.cx = m_bmpInfo.bmWidth;	// width  of dest rect
					m_size.cy = m_bmpInfo.bmHeight;
					// upper left point of dest
					m_pt.x = (m_ScrollRect.right - 
						((m_ScrollRect.Width())/2) - (m_size.cx/2));
					m_pt.y = height;
					pOldMemDC2Bitmap = memDC2.SelectObject(&bitmap);
					bIsBitmap = TRUE;
				}
				else
						c = ' ';
			}
			else {
				bItalic = FALSE;
				bUnderline = FALSE;
				m_nCurrentFontHeight = m_TextHeights[NORMAL_TEXT_HEIGHT];
   				bSuccess = m_fntArial.CreateFont(m_TextHeights[NORMAL_TEXT_HEIGHT], 0, 0, 0, 
   								FW_THIN, bItalic, bUnderline, 0, 
   								ANSI_CHARSET,
                   	OUT_DEFAULT_PRECIS,
                   	CLIP_DEFAULT_PRECIS,
                   	PROOF_QUALITY,
                   	VARIABLE_PITCH | 0x04 | FF_DONTCARE,
                   	(LPSTR)"Arial");
				memDC.SetTextColor(m_Colors[NORMAL_TEXT_COLOR]);
				pOldFont = memDC.SelectObject(&m_fntArial);
			}
		}

		memDC.SetBkMode(TRANSPARENT);

		if (!bIsBitmap)
		{				
			CRect r(m_ScrollRect);
			r.top = height;
			CSize size;
			if(m_szWork.GetLength()-rmcode != 0) 
			{
				int x = memDC.DrawText((const char *)m_szWork,m_szWork.GetLength()-rmcode,&r,DT_TOP|DT_CENTER|
						DT_NOPREFIX | DT_SINGLELINE);	
				size = memDC.GetTextExtent((LPCTSTR)m_szWork,m_szWork.GetLength()-rmcode);
			}
			else
				size = memDC.GetTextExtent((LPCTSTR)"W",1);
			height += size.cy;
		}
		else
		{
			CRect r( m_pt.x, m_pt.y, m_pt.x + m_size.cx, m_pt.y + m_size.cy);
			DrawBitmap(&memDC, &memDC2, &r);
//    		memDC.BitBlt( m_pt.x, m_pt.y, m_size.cx, m_size.cy, &memDC2, 0, 0, SRCCOPY );
			height += m_size.cy;
		}
		if (pOldMemDC2Bitmap != NULL) memDC2.SelectObject(pOldMemDC2Bitmap);
		if (pOldFont != NULL) memDC.SelectObject(pOldFont);
	}
	memDC.SelectObject(pOldMemDCBitmap);
}

void CCreditStatic::OnDestroy() 
{
	CStatic::OnDestroy();

	m_ArrCredit.RemoveAll();

	if(TimerOn)
		ASSERT(KillTimer(DISPLAY_TIMER_ID));	
}

BOOL CCreditStatic::SetBkImage(UINT nIDResource)
{
    return SetBkImage( (LPCTSTR)nIDResource );
}

BOOL CCreditStatic::SetBkImage(LPCTSTR lpszResourceName)
{

    // If this is not the first call then Delete GDI objects
    if( m_bitmap.m_hObject != NULL )
		m_bitmap.DeleteObject();
    if( m_pal.m_hObject != NULL )
		m_pal.DeleteObject();
    
    
    HBITMAP hBmp = (HBITMAP)::LoadImage( AfxGetInstanceHandle(), 
            lpszResourceName, IMAGE_BITMAP, 0,0, LR_CREATEDIBSECTION );

     if( hBmp == NULL ) 
        return FALSE;

	m_bitmap.Attach( hBmp );
    BITMAP bm;
    m_bitmap.GetBitmap( &bm );
    m_cxBitmap = bm.bmWidth;
    m_cyBitmap = bm.bmHeight;

    // Create a logical palette for the bitmap
    DIBSECTION ds;
    BITMAPINFOHEADER &bmInfo = ds.dsBmih;
    m_bitmap.GetObject( sizeof(ds), &ds );

    int nColors = bmInfo.biClrUsed ? bmInfo.biClrUsed : 1 << bmInfo.biBitCount;

    // Create a halftone palette if colors > 256. 
    CClientDC dc(NULL);             // Desktop DC
    if( nColors > 256 )
        m_pal.CreateHalftonePalette( &dc );
    else
    {
        // Create the palette

        RGBQUAD *pRGB = new RGBQUAD[nColors];
        CDC memDC;
        memDC.CreateCompatibleDC(&dc);

        CBitmap* pOldMemDCBitmap = memDC.SelectObject( &m_bitmap );
        ::GetDIBColorTable( memDC, 0, nColors, pRGB );

        UINT nSize = sizeof(LOGPALETTE) + (sizeof(PALETTEENTRY) * nColors);
        LOGPALETTE *pLP = (LOGPALETTE *) new BYTE[nSize];

        pLP->palVersion = 0x300;
        pLP->palNumEntries = nColors;

        for( int i=0; i < nColors; i++)
        {
            pLP->palPalEntry[i].peRed = pRGB[i].rgbRed;
            pLP->palPalEntry[i].peGreen = pRGB[i].rgbGreen;
            pLP->palPalEntry[i].peBlue = pRGB[i].rgbBlue;
            pLP->palPalEntry[i].peFlags = 0;
        }

        m_pal.CreatePalette( pLP );

		memDC.SelectObject(pOldMemDCBitmap);
        delete[] pLP;
        delete[] pRGB;
    }
//    Invalidate();

    return TRUE;
}