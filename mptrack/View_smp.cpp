#include "stdafx.h"
#include "mptrack.h"
#include "mainfrm.h"
#include "childfrm.h"
#include "moddoc.h"
#include "globals.h"
#include "ctrl_smp.h"
#include "dlsbank.h"
#include "channelManagerDlg.h"
#include "view_smp.h"
#include "midi.h"

// Non-client toolbar
#define SMP_LEFTBAR_CY			29
#define SMP_LEFTBAR_CXSEP		14
#define SMP_LEFTBAR_CXSPC		3
#define SMP_LEFTBAR_CXBTN		24
#define SMP_LEFTBAR_CYBTN		22

#define MIN_ZOOM	0
#define MAX_ZOOM	8

const UINT cLeftBarButtons[SMP_LEFTBAR_BUTTONS] = 
{
	ID_SAMPLE_ZOOMUP,
	ID_SAMPLE_ZOOMDOWN,
		ID_SEPARATOR,
};


IMPLEMENT_SERIAL(CViewSample, CModScrollView, 0)

BEGIN_MESSAGE_MAP(CViewSample, CModScrollView)
	//{{AFX_MSG_MAP(CViewSample)
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_NCCALCSIZE()
	ON_WM_NCHITTEST()
	ON_WM_NCPAINT()
	ON_WM_HSCROLL()
	ON_WM_MOUSEMOVE()
	ON_WM_NCMOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_NCLBUTTONDOWN()
	ON_WM_NCLBUTTONUP()
	ON_WM_NCLBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_CHAR()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_WM_DROPFILES()
	ON_COMMAND(ID_EDIT_SELECT_ALL,			OnEditSelectAll)
	ON_COMMAND(ID_EDIT_CUT,					OnEditCut)
	ON_COMMAND(ID_EDIT_COPY,				OnEditCopy)
	ON_COMMAND(ID_EDIT_PASTE,				OnEditPaste)
	ON_COMMAND(ID_SAMPLE_SETLOOP,			OnSetLoop)
	ON_COMMAND(ID_SAMPLE_SETSUSTAINLOOP,	OnSetSustainLoop)
	ON_COMMAND(ID_SAMPLE_8BITCONVERT,		On8BitConvert)
	ON_COMMAND(ID_SAMPLE_MONOCONVERT,		OnMonoConvert)
	ON_COMMAND(ID_SAMPLE_TRIM,				OnSampleTrim)
	ON_COMMAND(ID_PREVINSTRUMENT,			OnPrevInstrument)
	ON_COMMAND(ID_NEXTINSTRUMENT,			OnNextInstrument)
	ON_COMMAND(ID_SAMPLE_ZOOMONSEL,			OnZoomOnSel)
	ON_COMMAND(ID_SAMPLE_SETLOOPSTART,		OnSetLoopStart)
	ON_COMMAND(ID_SAMPLE_SETLOOPEND,		OnSetLoopEnd)
	ON_COMMAND(ID_SAMPLE_SETSUSTAINSTART,	OnSetSustainStart)
	ON_COMMAND(ID_SAMPLE_SETSUSTAINEND,		OnSetSustainEnd)
	ON_COMMAND(ID_SAMPLE_ZOOMUP,			OnZoomUp)
	ON_COMMAND(ID_SAMPLE_ZOOMDOWN,			OnZoomDown)
	ON_MESSAGE(WM_MOD_MIDIMSG,				OnMidiMsg)
	ON_MESSAGE(WM_MOD_KEYCOMMAND,	OnCustomKeyMsg) //rewbs.customKeys
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


///////////////////////////////////////////////////////////////
// CViewSample operations

CViewSample::CViewSample()
//------------------------
{
	m_nSample = 1;
	m_nZoom = 0;
	m_nScrollPos = 0;
	m_dwStatus = 0;
	m_nScrollFactor = 0;
	m_nBtnMouseOver = 0xFFFF;
	memset(m_dwNotifyPos, 0, sizeof(m_dwNotifyPos));
	memset(m_NcButtonState, 0, sizeof(m_NcButtonState));
	m_bmpEnvBar.Create(IDB_SMPTOOLBAR, 20, 0, RGB(192,192,192));
}


void CViewSample::OnInitialUpdate()
//---------------------------------
{
	m_dwBeginSel = m_dwEndSel = 0;
	ModifyStyleEx(0, WS_EX_ACCEPTFILES);
	CModScrollView::OnInitialUpdate();
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if (pMainFrm) 
	{
		pMainFrm->SetInfoText("");
		pMainFrm->SetXInfoText(""); //rewbs.xinfo
	}
	UpdateScrollSize();
}


void CViewSample::UpdateScrollSize()
//----------------------------------
{
	CModDoc *pModDoc = GetDocument();
	
	GetClientRect(&m_rcClient);
	if (pModDoc)
	{
		CPoint pt;
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		SIZE sizePage, sizeLine;
		DWORD dwLen = 0;

		if ((m_nSample > 0) && (m_nSample <= pSndFile->m_nSamples))
		{
			MODINSTRUMENT *pIns = &pSndFile->Ins[m_nSample];
			if (pIns->pSample) dwLen = pIns->nLength;
		}
		if (m_nZoom)
		{
			m_sizeTotal.cx = (dwLen + (1 << (m_nZoom-1)) - 1) >> (m_nZoom-1);
		} else
		{
			m_sizeTotal.cx = m_rcClient.Width();
		}
		m_nScrollFactor = 0;
		UINT cx0 = m_sizeTotal.cx;
		UINT cx = cx0;
		while (cx > 30000)
		{
			m_nScrollFactor++;
			m_sizeTotal.cx /= 2;
			cx = m_sizeTotal.cx;
			if (cx > (UINT)m_rcClient.right)
			{
				UINT wantedsize = (cx0 - m_rcClient.right) >> m_nScrollFactor;
				UINT mfcsize = cx - m_rcClient.right;
				cx += wantedsize - mfcsize;
			}
		}
		m_sizeTotal.cx = cx;
		m_sizeTotal.cy = 1;
		sizeLine.cx = (m_rcClient.right / (16 << m_nScrollFactor)) + 1;
		sizeLine.cy = 1;
		sizePage.cx = sizeLine.cx * 4;
		sizePage.cy = sizeLine.cy;
		SetScrollSizes(MM_TEXT, m_sizeTotal, sizePage, sizeLine);
		m_nScrollPos = GetScrollPos(SB_HORZ) << m_nScrollFactor;
	}
}


BOOL CViewSample::SetCurrentSample(UINT nSmp)
//-------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;

	if (!pModDoc) return FALSE;
	pSndFile = pModDoc->GetSoundFile();
	if ((nSmp < 1) || (nSmp > pSndFile->m_nSamples)) return FALSE;
	pModDoc->SetFollowWnd(m_hWnd, MPTNOTIFY_SAMPLE|nSmp);
	if (nSmp == m_nSample) return FALSE;
	m_dwBeginSel = m_dwEndSel = 0;
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if (pMainFrm) pMainFrm->SetInfoText("");
	m_nSample = nSmp;
	memset(m_dwNotifyPos, 0, sizeof(m_dwNotifyPos));
	UpdateScrollSize();
	UpdateNcButtonState();
	InvalidateRect(NULL, FALSE);
	return TRUE;
}


BOOL CViewSample::SetZoom(UINT nZoom)
//-----------------------------------
{
  
	if (nZoom == m_nZoom) 
		return TRUE;
	if (nZoom > MAX_ZOOM) 
		return FALSE;

	m_nZoom = nZoom;
	UpdateScrollSize();
	InvalidateRect(NULL, FALSE);
	return TRUE;
}


void CViewSample::SetCurSel(DWORD nBegin, DWORD nEnd)
//---------------------------------------------------
{
	if (nBegin > nEnd)
	{
		DWORD tmp = nBegin;
		nBegin = nEnd;
		nEnd = tmp;
	}
	if ((nBegin != m_dwBeginSel) || (nEnd != m_dwEndSel))
	{
		RECT rect;
		DWORD dMin = m_dwBeginSel, dMax = m_dwEndSel;
		if (m_dwBeginSel >= m_dwEndSel)
		{
			dMin = nBegin;
			dMax = nEnd;
		}
		if ((nBegin == dMin) && (dMax != nEnd))
		{
			dMin = dMax;
			if (nEnd < dMin) dMin = nEnd;
			if (nEnd > dMax) dMax = nEnd;
		} else
		if ((nEnd == dMax) && (dMin != nBegin))
		{
			dMax = dMin;
			if (nBegin < dMin) dMin = nBegin;
			if (nBegin > dMax) dMax = nBegin;
		} else
		{
			if (nBegin < dMin) dMin = nBegin;
			if (nEnd > dMax) dMax = nEnd;
		}
		m_dwBeginSel = nBegin;
		m_dwEndSel = nEnd;
		rect.top = m_rcClient.top;
		rect.bottom = m_rcClient.bottom;
		rect.left = SampleToScreen(dMin);
		rect.right = SampleToScreen(dMax) + 1;
		if (rect.left < 0) rect.left = 0;
		if (rect.right > m_rcClient.right) rect.right = m_rcClient.right;
		if (rect.right > rect.left) InvalidateRect(&rect, FALSE);
		CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
		if (pMainFrm)
		{
			CHAR s[64];
			s[0] = 0;
			if (m_dwEndSel > m_dwBeginSel)
			{
				CModDoc *pModDoc = GetDocument();
				wsprintf(s, "[%d,%d]", m_dwBeginSel, m_dwEndSel);
				if (pModDoc)
				{
					CSoundFile *pSndFile = pModDoc->GetSoundFile();
					LONG lSampleRate = pSndFile->Ins[m_nSample].nC4Speed;
					if (pSndFile->m_nType & (MOD_TYPE_MOD|MOD_TYPE_XM))
					{
						lSampleRate = CSoundFile::TransposeToFrequency(pSndFile->Ins[m_nSample].RelativeTone, pSndFile->Ins[m_nSample].nFineTune);
					}
					if (!lSampleRate) lSampleRate = 8363;
					ULONG msec = ((ULONG)(m_dwEndSel - m_dwBeginSel) * 1000) / lSampleRate;
					if (msec < 1000)
						wsprintf(s+strlen(s), " (%lums)", msec);
					else
						wsprintf(s+strlen(s), " (%lu.%lus)", msec/1000, (msec/100) % 10);
				}
			}
			pMainFrm->SetInfoText(s);
		}
	}
}


LONG CViewSample::SampleToScreen(LONG n) const
//--------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if ((pModDoc) && (m_nSample < MAX_SAMPLES))
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		UINT nLen = pSndFile->Ins[m_nSample].nLength;
		if (!nLen) return 0;
		if (m_nZoom)
		{
			return (n >> ((LONG)m_nZoom-1)) - m_nScrollPos;
		} else
		{
			return _muldiv(n, m_sizeTotal.cx, nLen);
		}
	}
	return 0;
}


DWORD CViewSample::ScreenToSample(LONG x) const
//---------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	LONG n = 0;

	if ((pModDoc) && (m_nSample < MAX_SAMPLES))
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		UINT nLen = pSndFile->Ins[m_nSample].nLength;
		if (!nLen) return 0;
		if (m_nZoom)
		{
			n = (m_nScrollPos + x) << (m_nZoom-1);
		} else
		{
			if (x < 0) x = 0;
			if (m_sizeTotal.cx) n = _muldiv(x, nLen, m_sizeTotal.cx);
		}
		if (n < 0) n = 0;
		if (n > (LONG)nLen) n = nLen;
	}
	return n;
}


void CViewSample::InvalidateSample()
//----------------------------------
{
	InvalidateRect(NULL, FALSE);
}


LRESULT CViewSample::OnModViewMsg(WPARAM wParam, LPARAM lParam)
//-------------------------------------------------------------
{
	switch(wParam)
	{
	case VIEWMSG_SETCURRENTSAMPLE:
		SetZoom(lParam >> 16);
		SetCurrentSample(lParam & 0xFFFF);
		break;

	case VIEWMSG_LOADSTATE:
		if (lParam)
		{
			SAMPLEVIEWSTATE *pState = (SAMPLEVIEWSTATE *)lParam;
			if (pState->nSample == m_nSample)
			{
				SetCurSel(pState->dwBeginSel, pState->dwEndSel);
				m_nScrollPos = pState->dwScrollPos << m_nScrollFactor;
				SetScrollPos(SB_HORZ, pState->dwScrollPos);
				UpdateScrollSize();
				InvalidateSample();
			}
		}
		break;

	case VIEWMSG_SAVESTATE:
		if (lParam)
		{
			SAMPLEVIEWSTATE *pState = (SAMPLEVIEWSTATE *)lParam;
			pState->cbStruct = sizeof(SAMPLEVIEWSTATE);
			pState->dwScrollPos = GetScrollPos(SB_HORZ);
			pState->dwBeginSel = m_dwBeginSel;
			pState->dwEndSel = m_dwEndSel;
			pState->nSample = m_nSample;
		}
		break;

	default:
		return CModScrollView::OnModViewMsg(wParam, lParam);
	}
	return 0;
}


///////////////////////////////////////////////////////////////
// CViewSample drawing

void CViewSample::UpdateView(DWORD dwHintMask, CObject *)
//-------------------------------------------------------
{
	// 05/01/05 : ericus replaced ">> 24" by ">> 20" : 4000 samples -> 12bits [see Moddoc.h]
	//rewbs: replaced 0xff by 0x0fff for 4000 samples.
	if ((dwHintMask & (HINT_MPTOPTIONS|HINT_MODTYPE))
		|| ((dwHintMask & HINT_SAMPLEDATA) && (m_nSample == (dwHintMask >> HINT_SHIFT_SMP))) )
	 //|| ((dwHintMask & HINT_SAMPLEDATA) && ((m_nSample&0x0fff) == (dwHintMask >> 20))) )
	{
		UpdateScrollSize();
		UpdateNcButtonState();
		InvalidateSample();
	}
}

#define YCVT(n, bits)		(ymed - (((n) * yrange) >> (bits)))


// Draw sample data, 1:1 ratio
void CViewSample::DrawSampleData1(HDC hdc, int ymed, int cx, int cy, int len, int uFlags, PVOID pSampleData)
//----------------------------------------------------------------------------------------------------------
{	
	int smplsize;
	int yrange = cy/2;
	signed char *psample = (signed char *)pSampleData;
	int y0 = 0, looplen, loopdiv;

	smplsize = (uFlags & CHN_16BIT) ? 2 : 1;
	if (uFlags & CHN_STEREO) smplsize *= 2;
	if (uFlags & CHN_16BIT)
	{
		y0 = YCVT(*((signed short *)(psample-smplsize)),15);
	} else
	{
		y0 = YCVT(*(psample-smplsize),7);
	}
	::MoveToEx(hdc, -1, y0, NULL);
	// Linear 1:1 scale
	if (m_nZoom)
	{
		looplen = cx;
		if (looplen > len) looplen = len;
		loopdiv = cx;
	} else
	// Stretch
	{
		looplen = len;
		loopdiv = len;
	}
	// 16-Bit
	if (uFlags & CHN_16BIT)
	{
		for (int n=0; n<=looplen; n++)
		{
			int x = (n*cx) / loopdiv;
			int y = *(signed short *)psample;
			::LineTo(hdc, x, YCVT(y,15));
			psample += smplsize;
		}
	} else
	// 8-bit
	{
		for (int n=0; n<=looplen; n++)
		{
			int x = (n*cx) / loopdiv;
			int y = *psample;
			::LineTo(hdc, x, YCVT(y,7));
			psample += smplsize;
		}
	}
}


#ifdef ENABLE_MMX

static void mmxex_findminmax16(void *p, int scanlen, int smplsize, int *smin, int *smax)
//--------------------------------------------------------------------------------------
{
	_asm {
	mov ebx, p
	mov ecx, scanlen
	mov edx, smplsize
	mov esi, smin
	mov edi, smax
	movd mm0, [esi]	// mm0 = min
	movd mm1, [edi]	// mm1 = max
	shr ecx, 2
	or ecx, ecx
	punpcklwd mm0, mm0
	punpcklwd mm1, mm1
	punpckldq mm0, mm0
	punpckldq mm1, mm1
	jz done4x
mainloop4x:
	movq mm2, [ebx]
	add ebx, edx
	dec ecx
	pminsw mm0, mm2
	pmaxsw mm1, mm2
	jnz mainloop4x
	movq mm2, mm0
	movq mm3, mm1
	punpckhdq mm2, mm2
	punpckhdq mm3, mm3
	cmp edx, 2
	pminsw mm0, mm2
	pmaxsw mm1, mm3
	jg done4x
	psrad mm2, 16
	psrad mm3, 16
	pminsw mm0, mm2
	pmaxsw mm1, mm3
done4x:
	mov ecx, scanlen
	and ecx, 3
	or ecx, ecx
	jz done1x
mainloop1x:
	movzx eax, word ptr [ebx]
	add ebx, edx
	movd mm2, eax
	dec ecx
	pminsw mm0, mm2
	pmaxsw mm1, mm2
	jnz mainloop1x
done1x:
	movd eax, mm0
	movd edx, mm1
	movsx eax, ax
	movsx edx, dx
	mov [esi], eax
	mov [edi], edx
	emms
	}
}


static void mmxex_findminmax8(void *p, int scanlen, int smplsize, int *smin, int *smax)
//-------------------------------------------------------------------------------------
{
	_asm {
	mov ebx, p
	mov ecx, scanlen
	mov edx, smplsize
	mov esi, smin
	mov edi, smax
	movd mm0, [esi]	// mm0 = min
	movd mm1, [edi]	// mm1 = max
	shr ecx, 3
	mov eax, 0x80808080
	movd mm7, eax
	punpckldq mm7, mm7
	or ecx, ecx
	punpcklbw mm0, mm0
	punpcklbw mm1, mm1
	punpcklwd mm0, mm0
	punpcklwd mm1, mm1
	punpckldq mm0, mm0
	punpckldq mm1, mm1
	pxor mm0, mm7
	pxor mm1, mm7
	jz done8x
mainloop8x:
	movq mm2, [ebx]
	add ebx, edx
	dec ecx
	pxor mm2, mm7
	pminub mm0, mm2
	pmaxub mm1, mm2
	jnz mainloop8x
	movq mm2, mm0
	movq mm3, mm1
	punpckhdq mm2, mm2
	punpckhdq mm3, mm3
	pminub mm0, mm2
	pmaxub mm1, mm3
	cmp edx, 1
	psrld mm2, 16
	psrld mm3, 16
	pminub mm0, mm2
	pmaxub mm1, mm3
	jg done8x
	psrld mm2, 8
	psrld mm3, 8
	pminub mm0, mm2
	pmaxub mm1, mm3
done8x:
	mov ecx, scanlen
	and ecx, 7
	or ecx, ecx
	jz done1x
mainloop1x:
	movzx eax, byte ptr [ebx]
	add ebx, edx
	movd mm2, eax
	dec ecx
	pxor mm2, mm7
	pminub mm0, mm2
	pmaxub mm1, mm2
	jnz mainloop1x
done1x:
	pxor mm0, mm7
	pxor mm1, mm7
	movd eax, mm0
	movd edx, mm1
	movsx eax, al
	movsx edx, dl
	mov [esi], eax
	mov [edi], edx
	emms
	}
}

#endif

void CViewSample::DrawSampleData2(HDC hdc, int ymed, int cx, int cy, int len, int uFlags, PVOID pSampleData)
//----------------------------------------------------------------------------------------------------------
{
	int smplsize, oldsmin, oldsmax;
	int yrange = cy/2;
	signed char *psample = (signed char *)pSampleData;
	//int y0 = 0, xmax, posincr, posfrac, poshi;
	int32 y0 = 0, xmax, poshi;
	uint64 posincr, posfrac;

	if (len <= 0) return;
	smplsize = (uFlags & CHN_16BIT) ? 2 : 1;
	if (uFlags & CHN_STEREO) smplsize *= 2;
	if (uFlags & CHN_16BIT)
	{
		y0 = YCVT(*((signed short *)(psample-smplsize)), 15);
	} else
	{
		y0 = YCVT(*(psample-smplsize), 7);
	}
	oldsmin = oldsmax = y0;
	if (m_nZoom)
	{
		xmax = len>>(m_nZoom-1);
		if (xmax > cx) xmax = cx;
		posincr = (uint64(1) << (m_nZoom-1+16));
	} else
	{
		xmax = cx;
		//posincr = _muldiv(len, 0x10000, cx);
		posincr = uint64(len) * uint64(0x10000) / uint64(cx);
	}
	::MoveToEx(hdc, 0, ymed, NULL);
	posfrac = 0;
	poshi = 0;
	for (int x=0; x<xmax; x++)
	{
		//int smin, smax, scanlen;
		int smin, smax;
		int32 scanlen;

		posfrac += posincr;
		scanlen = static_cast<int32>((posfrac+0xffff) >> 16);
		if (poshi >= len) poshi = len-1;
		if (poshi+scanlen > len) scanlen = len-poshi;
		if (scanlen < 1) scanlen = 1;
		// 16-bit
		if (uFlags & CHN_16BIT)
		{
			signed short *p = (signed short *)(psample + poshi*smplsize);
			smin = 32767;
			smax = -32768;
#ifdef ENABLE_MMX
			if (CSoundFile::gdwSysInfo & SYSMIX_MMXEX)
			{
				mmxex_findminmax16(p, scanlen, smplsize, &smin, &smax);
			} else
#endif
			{
				for (int i=0; i<scanlen; i++)
				{
					int s = *p;
					if (s < smin) smin = s;
					if (s > smax) smax = s;
					p = (signed short *)(((signed char *)p) + smplsize);
				}
			}
			smin = YCVT(smin,15);
			smax = YCVT(smax,15);
		} else
		// 8-bit
		{
			signed char *p = psample + poshi * smplsize;
			smin = 127;
			smax = -128;
#ifdef ENABLE_MMX
			if (CSoundFile::gdwSysInfo & SYSMIX_MMXEX)
			{
				mmxex_findminmax8(p, scanlen, smplsize, &smin, &smax);
			} else
#endif
			{
				for (int i=0; i<scanlen; i++)
				{
					
					int s = *p;
					if (s < smin) smin = s;
					if (s > smax) smax = s;
					p += smplsize;
				}
			}
			smin = YCVT(smin,7);
			smax = YCVT(smax,7);
		}
		if (smin > oldsmax)
		{
			::MoveToEx(hdc, x-1, oldsmax - 1, NULL);
			::LineTo(hdc, x, smin);
		}
		if (smax < oldsmin)
		{
			::MoveToEx(hdc, x-1, oldsmin, NULL);
			::LineTo(hdc, x, smax);
		}
		::MoveToEx(hdc, x, smax-1, NULL);
		::LineTo(hdc, x, smin);
		oldsmin = smin;
		oldsmax = smax;
		poshi += static_cast<int32>(posfrac>>16);
		posfrac &= 0xffff;
	}
}


void CViewSample::OnDraw(CDC *pDC)
//--------------------------------
{	
	CRect rcClient = m_rcClient, rect, rc;
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;
	HGDIOBJ oldpen;
	HDC hdc;
	UINT nSmpScrollPos = (m_nZoom) ? (m_nScrollPos << (m_nZoom - 1)) : 0;
	
	if ((!pModDoc) || (!pDC)) return;
	hdc = pDC->m_hDC;
	oldpen = ::SelectObject(hdc, CMainFrame::penBlack);
	pSndFile = pModDoc->GetSoundFile();
	rect = rcClient;
	if ((rcClient.bottom > rcClient.top) && (rcClient.right > rcClient.left))
	{
		MODINSTRUMENT *pins = &pSndFile->Ins[(m_nSample < MAX_SAMPLES) ? m_nSample : 0];
		int ymed = (rect.top + rect.bottom) / 2;
		int yrange = (rect.bottom - rect.top) / 2;
		
		// Erase background
		if ((m_dwBeginSel < m_dwEndSel) && (m_dwEndSel > nSmpScrollPos))
		{
			rc = rect;
			if (m_dwBeginSel > nSmpScrollPos)
			{
				rc.right = SampleToScreen(m_dwBeginSel);
				if (rc.right > rcClient.right) rc.right = rcClient.right;
				if (rc.right > rc.left) ::FillRect(hdc, &rc, CMainFrame::brushBlack);
				rc.left = rc.right;
			}
			if (rc.left < 0) rc.left = 0;
			rc.right = SampleToScreen(m_dwEndSel) + 1;
			if (rc.right > rcClient.right) rc.right = rcClient.right;
			if (rc.right > rc.left) ::FillRect(hdc, &rc, CMainFrame::brushWhite);
			rc.left = rc.right;
			if (rc.left < 0) rc.left = 0;
			rc.right = rcClient.right;
			if (rc.right > rc.left) ::FillRect(hdc, &rc, CMainFrame::brushBlack);
		} else
		{
			::FillRect(hdc, &rcClient, CMainFrame::brushBlack);
		}
		::SelectObject(hdc, CMainFrame::penDarkGray);
		if (pins->uFlags & CHN_STEREO)
		{
			::MoveToEx(hdc, 0, ymed-yrange/2, NULL);
			::LineTo(hdc, rcClient.right, ymed-yrange/2);
			::MoveToEx(hdc, 0, ymed+yrange/2, NULL);
			::LineTo(hdc, rcClient.right, ymed+yrange/2);
		} else
		{
			::MoveToEx(hdc, 0, ymed, NULL);
			::LineTo(hdc, rcClient.right, ymed);
		}
		// Drawing sample
		if ((pins->pSample) && (yrange) && (pins->nLength > 1) && (rect.right > 1))
		{
			// Loop Start/End
			if ((pins->nLoopEnd > nSmpScrollPos) && (pins->nLoopEnd > pins->nLoopStart))
			{
				int xl = SampleToScreen(pins->nLoopStart);
				if ((xl >= 0) && (xl < rcClient.right))
				{
					::MoveToEx(hdc, xl, rect.top, NULL);
					::LineTo(hdc, xl, rect.bottom);
				}
				xl = SampleToScreen(pins->nLoopEnd);
				if ((xl >= 0) && (xl < rcClient.right))
				{
					::MoveToEx(hdc, xl, rect.top, NULL);
					::LineTo(hdc, xl, rect.bottom);
				}
			}
			// Sustain Loop Start/End
			if ((pins->nSustainEnd > nSmpScrollPos) && (pins->nSustainEnd > pins->nSustainStart))
			{
				::SelectObject(hdc, CMainFrame::penHalfDarkGray);
				int xl = SampleToScreen(pins->nSustainStart);
				if ((xl >= 0) && (xl < rcClient.right))
				{
					::MoveToEx(hdc, xl, rect.top, NULL);
					::LineTo(hdc, xl, rect.bottom);
				}
				xl = SampleToScreen(pins->nSustainEnd);
				if ((xl >= 0) && (xl < rcClient.right))
				{
					::MoveToEx(hdc, xl, rect.top, NULL);
					::LineTo(hdc, xl, rect.bottom);
				}
			}
			// Drawing Sample Data
			::SelectObject(hdc, CMainFrame::penSample);
			int smplsize = (pins->uFlags & CHN_16BIT) ? 2 : 1;
			if (pins->uFlags & CHN_STEREO) smplsize *= 2;
			if ((m_nZoom == 1) || ((!m_nZoom) && (pins->nLength <= (UINT)rect.right)))
			{
				int len = pins->nLength - nSmpScrollPos;
				signed char *psample = ((signed char *)pins->pSample) + nSmpScrollPos * smplsize;
				if (pins->uFlags & CHN_STEREO)
				{
					DrawSampleData1(hdc, ymed-yrange/2, rect.right, yrange, len, pins->uFlags, psample);
					DrawSampleData1(hdc, ymed+yrange/2, rect.right, yrange, len, pins->uFlags, psample+smplsize/2);
				} else
				{
					DrawSampleData1(hdc, ymed, rect.right, yrange*2, len, pins->uFlags, psample);
				}
			} else
			{
				int len = pins->nLength;
				int xscroll = 0;
				if (m_nZoom)
				{
					xscroll = nSmpScrollPos;
					len -= nSmpScrollPos;
				}
				signed char *psample = ((signed char *)pins->pSample) + xscroll * smplsize;
				if (pins->uFlags & CHN_STEREO)
				{
					DrawSampleData2(hdc, ymed-yrange/2, rect.right, yrange, len, pins->uFlags, psample);
					DrawSampleData2(hdc, ymed+yrange/2, rect.right, yrange, len, pins->uFlags, psample+smplsize/2);
				} else
				{
					DrawSampleData2(hdc, ymed, rect.right, yrange*2, len, pins->uFlags, psample);
				}
			}
		}
	}
	DrawPositionMarks(hdc);
	if (oldpen) ::SelectObject(hdc, oldpen);

// -> CODE#0015
// -> DESC="channels management dlg"
	CMainFrame * pMainFrm = CMainFrame::GetMainFrame();
	BOOL activeDoc = pMainFrm ? pMainFrm->GetActiveDoc() == GetDocument() : FALSE;

	if(activeDoc && CChannelManagerDlg::sharedInstance(FALSE) && CChannelManagerDlg::sharedInstance()->IsDisplayed())
		CChannelManagerDlg::sharedInstance()->SetDocument((void*)this);
// -! NEW_FEATURE#0015
}


void CViewSample::DrawPositionMarks(HDC hdc)
//------------------------------------------
{
	CRect rect;
	for (UINT i=0; i<MAX_CHANNELS; i++) if (m_dwNotifyPos[i] & MPTNOTIFY_POSVALID)
	{
		rect.top = -2;
		rect.left = SampleToScreen(m_dwNotifyPos[i] & 0x0FFFFFFF);
		rect.right = rect.left + 1;
		rect.bottom = m_rcClient.bottom + 1;
		if ((rect.right >= 0) && (rect.right < m_rcClient.right)) InvertRect(hdc, &rect);
	}
}


LRESULT CViewSample::OnPlayerNotify(MPTNOTIFICATION *pnotify)
//-----------------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if ((!pnotify) || (!pModDoc)) return 0;
	if (pnotify->dwType & MPTNOTIFY_STOP)
	{
		for (UINT i=0; i<MAX_CHANNELS; i++)
		{
			if (m_dwNotifyPos[i])
			{
				memset(m_dwNotifyPos, 0, sizeof(m_dwNotifyPos));
				InvalidateRect(NULL, FALSE);
				break;
			}
		}
	} else
	if ((pnotify->dwType & MPTNOTIFY_SAMPLE) && ((pnotify->dwType & 0xFFFF) == m_nSample))
	{
		//CSoundFile *pSndFile = pModDoc->GetSoundFile();
		BOOL bUpdate = FALSE;
		for (UINT i=0; i<MAX_CHANNELS; i++)
		{
			DWORD newpos = /*(pSndFile->m_dwSongFlags & SONG_PAUSED) ?*/ pnotify->dwPos[i]/* : 0*/;
			if (m_dwNotifyPos[i] != newpos)
			{
				bUpdate = TRUE;
				break;
			}
		}
		if (bUpdate)
		{
			HDC hdc = ::GetDC(m_hWnd);
			DrawPositionMarks(hdc);
			for (UINT j=0; j<MAX_CHANNELS; j++)
			{
				DWORD newpos = /*(pSndFile->m_dwSongFlags & SONG_PAUSED) ?*/ pnotify->dwPos[j] /*: 0*/;
				m_dwNotifyPos[j] = newpos;
			}
			DrawPositionMarks(hdc);
			::ReleaseDC(m_hWnd, hdc);
		}
	}
	return 0;
}


BOOL CViewSample::GetNcButtonRect(UINT nBtn, LPRECT lpRect)
//---------------------------------------------------------
{
	lpRect->left = 4;
	lpRect->top = 3;
	lpRect->bottom = lpRect->top + SMP_LEFTBAR_CYBTN;
	if (nBtn >= SMP_LEFTBAR_BUTTONS) return FALSE;
	for (UINT i=0; i<nBtn; i++)
	{
		if (cLeftBarButtons[i] == ID_SEPARATOR)
		{
			lpRect->left += SMP_LEFTBAR_CXSEP;
		} else
		{
			lpRect->left += SMP_LEFTBAR_CXBTN + SMP_LEFTBAR_CXSPC;
		}
	}
	if (cLeftBarButtons[nBtn] == ID_SEPARATOR)
	{
		lpRect->left += SMP_LEFTBAR_CXSEP/2 - 2;
		lpRect->right = lpRect->left + 2;
		return FALSE;
	} else
	{
		lpRect->right = lpRect->left + SMP_LEFTBAR_CXBTN;
	}
	return TRUE;
}


void CViewSample::DrawNcButton(CDC *pDC, UINT nBtn)
//-------------------------------------------------
{
	CRect rect;
	COLORREF crHi = GetSysColor(COLOR_3DHILIGHT);
	COLORREF crDk = GetSysColor(COLOR_3DSHADOW);
	COLORREF crFc = GetSysColor(COLOR_3DFACE);
	COLORREF c1, c2;

	if (GetNcButtonRect(nBtn, &rect))
	{
		DWORD dwStyle = m_NcButtonState[nBtn];
		COLORREF c3, c4;
		int xofs = 0, yofs = 0, nImage = 0;
			
		c1 = c2 = c3 = c4 = crFc;
		if (!(CMainFrame::m_dwPatternSetup & PATTERN_FLATBUTTONS))
		{
			c1 = c3 = crHi;
			c2 = crDk;
			c4 = RGB(0,0,0);
		}
		if (dwStyle & (NCBTNS_PUSHED|NCBTNS_CHECKED))
		{
			c1 = crDk;
			c2 = crHi;
			if (!(CMainFrame::m_dwPatternSetup & PATTERN_FLATBUTTONS))
			{
				c4 = crHi;
				c3 = (dwStyle & NCBTNS_PUSHED) ? RGB(0,0,0) : crDk;
			}
			xofs = yofs = 1;
		} else
		if ((dwStyle & NCBTNS_MOUSEOVER) && (CMainFrame::m_dwPatternSetup & PATTERN_FLATBUTTONS))
		{
			c1 = crHi;
			c2 = crDk;
		}
		switch(cLeftBarButtons[nBtn])
		{
		case ID_SAMPLE_ZOOMUP:		nImage = 1; break;
		case ID_SAMPLE_ZOOMDOWN:	nImage = 2; break;
		}
		pDC->Draw3dRect(rect.left-1, rect.top-1, SMP_LEFTBAR_CXBTN+2, SMP_LEFTBAR_CYBTN+2, c3, c4);
		pDC->Draw3dRect(rect.left, rect.top, SMP_LEFTBAR_CXBTN, SMP_LEFTBAR_CYBTN, c1, c2);
		rect.DeflateRect(1, 1);
		pDC->FillSolidRect(&rect, crFc);
		rect.left += xofs;
		rect.top += yofs;
		if (dwStyle & NCBTNS_CHECKED) m_bmpEnvBar.Draw(pDC, 0, rect.TopLeft(), ILD_NORMAL);
		m_bmpEnvBar.Draw(pDC, nImage, rect.TopLeft(), ILD_NORMAL);
	} else
	{
		c1 = c2 = crFc;
		if (CMainFrame::m_dwPatternSetup & PATTERN_FLATBUTTONS)
		{
			c1 = crDk;
			c2 = crHi;
		}
		pDC->Draw3dRect(rect.left, rect.top, 2, SMP_LEFTBAR_CYBTN, c1, c2);
	}
}


void CViewSample::OnNcPaint()
//---------------------------
{
	RECT rect;

	CModScrollView::OnNcPaint();
	GetWindowRect(&rect);
	// Assumes there is no other non-client items
	rect.bottom = SMP_LEFTBAR_CY;
	rect.right -= rect.left;
	rect.left = 0;
	rect.top = 0;
	if ((rect.left < rect.right) && (rect.top < rect.bottom))
	{
		CDC *pDC = GetWindowDC();
		HDC hdc = pDC->m_hDC;
		HGDIOBJ oldpen = SelectObject(hdc, (HGDIOBJ)CMainFrame::penDarkGray);
		rect.bottom--;
		MoveToEx(hdc, rect.left, rect.bottom, NULL);
		LineTo(hdc, rect.right, rect.bottom);
		if (rect.top < rect.bottom) FillRect(hdc, &rect, CMainFrame::brushGray);
		if (rect.top + 2 < rect.bottom)
		{
			for (UINT i=0; i<SMP_LEFTBAR_BUTTONS; i++)
			{
				DrawNcButton(pDC, i);
			}
		}
		if (oldpen) SelectObject(hdc, oldpen);
		ReleaseDC(pDC);
	}
}


void CViewSample::UpdateNcButtonState()
//-------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;
	CDC *pDC = NULL;
	
	if (!pModDoc) return;
	pSndFile = pModDoc->GetSoundFile();
	for (UINT i=0; i<SMP_LEFTBAR_BUTTONS; i++) if (cLeftBarButtons[i] != ID_SEPARATOR)
	{
		DWORD dwStyle = 0;
		
		if (m_nBtnMouseOver == i)
		{
			dwStyle |= NCBTNS_MOUSEOVER;
			if (m_dwStatus & SMPSTATUS_NCLBTNDOWN) dwStyle |= NCBTNS_PUSHED;
		}
		if (dwStyle != m_NcButtonState[i])
		{
			m_NcButtonState[i] = dwStyle;
			if (!pDC) pDC = GetWindowDC();
			DrawNcButton(pDC, i);
		}
	}
	if (pDC) ReleaseDC(pDC);
}


///////////////////////////////////////////////////////////////
// CViewSample messages

void CViewSample::OnSize(UINT nType, int cx, int cy)
//--------------------------------------------------
{
	CScrollView::OnSize(nType, cx, cy);
	if (((nType == SIZE_RESTORED) || (nType == SIZE_MAXIMIZED)) && (cx > 0) && (cy > 0))
	{
		UpdateScrollSize();
	}
}


void CViewSample::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp)
//-----------------------------------------------------------------------------
{
	CModScrollView::OnNcCalcSize(bCalcValidRects, lpncsp);
	if (lpncsp)
	{
		lpncsp->rgrc[0].top += SMP_LEFTBAR_CY;
		if (lpncsp->rgrc[0].bottom < lpncsp->rgrc[0].top) lpncsp->rgrc[0].top = lpncsp->rgrc[0].bottom;
	}
}


void CViewSample::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
//--------------------------------------------------------------------------
{
	CScrollView::OnHScroll(nSBCode, nPos, pScrollBar);
	m_nScrollPos = GetScrollPos(SB_HORZ) << m_nScrollFactor;
}


BOOL CViewSample::OnScrollBy(CSize sizeScroll, BOOL bDoScroll)
//-------------------------------------------------------------
{
	int xOrig, x;

	// don't scroll if there is no valid scroll range (ie. no scroll bar)
	CScrollBar* pBar;
	DWORD dwStyle = GetStyle();
	// vertical scroll bar not enabled
	pBar = GetScrollBarCtrl(SB_HORZ);
	if ((pBar != NULL && !pBar->IsWindowEnabled()) || (pBar == NULL && !(dwStyle & WS_HSCROLL)))
	{
		// horizontal scroll bar not enabled
		sizeScroll.cx = 0;
	}

	// adjust current x position
	xOrig = x = GetScrollPos(SB_HORZ);
	int xMax = GetScrollLimit(SB_HORZ);
	x += sizeScroll.cx;
	if (x < 0)
		x = 0;
	else if (x > xMax)
		x = xMax;

	// did anything change?
	if (x == xOrig) return FALSE;

	if (bDoScroll)
	{
		// do scroll and update scroll positions:
		// CViewSample also handles the scroll factor so we can use ranges bigger than 64K
		ScrollWindow(-((x-xOrig) << m_nScrollFactor), 0);
		m_nScrollPos = x << m_nScrollFactor;
		SetScrollPos(SB_HORZ, x);
	}
	return TRUE;
}


void CViewSample::ScrollToPosition(int x)    // logical coordinates
//---------------------------------------
{
	CPoint pt;
	// now in device coordinates - limit if out of range
	int xMax = GetScrollLimit(SB_HORZ);
	pt.x = x;
	pt.y = 0;
	if (pt.x < 0)
		pt.x = 0;
	else if (pt.x > xMax)
		pt.x = xMax;
	pt.x <<= m_nScrollFactor;
	ScrollToDevicePosition(pt);
}


void CViewSample::OnMouseMove(UINT, CPoint point)
//-----------------------------------------------
{
	CHAR s[64];
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;
	
	if ((m_nBtnMouseOver < SMP_LEFTBAR_BUTTONS) || (m_dwStatus & SMPSTATUS_NCLBTNDOWN))
	{
		m_dwStatus &= ~SMPSTATUS_NCLBTNDOWN;
		m_nBtnMouseOver = 0xFFFF;
		UpdateNcButtonState();
		CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
		if (pMainFrm) pMainFrm->SetHelpText("");
	}
	if (!pModDoc) return;
	pSndFile = pModDoc->GetSoundFile();
	if (m_rcClient.PtInRect(point))
	{
		LONG x = ScreenToSample(point.x);
		wsprintf(s, "Cursor: %d", x);
		UpdateIndicator(s);
	} else UpdateIndicator(NULL);
	if (m_dwStatus & SMPSTATUS_MOUSEDRAG)
	{
		BOOL bAgain = FALSE;
		DWORD len = pSndFile->Ins[m_nSample].nLength;
		if (!len) return;
		DWORD old = m_dwEndDrag;
		if (m_nZoom)
		{
			if (point.x < 0)
			{
				CPoint pt;
				pt.x = point.x;
				pt.y = 0;
				if (OnScrollBy(pt))
				{
					UpdateWindow();
					bAgain = TRUE;
				}
				point.x = 0;
			}
			if (point.x > m_rcClient.right)
			{
				CPoint pt;
				pt.x = point.x - m_rcClient.right;
				pt.y = 0;
				if (OnScrollBy(pt))
				{
					UpdateWindow();
					bAgain = TRUE;
				}
				point.x = m_rcClient.right;
			}
		}
		LONG l = ScreenToSample(point.x);
		if (l < 0) l = 0;
		m_dwEndDrag = l;
		if (m_dwEndDrag > len) m_dwEndDrag = len;
		if (old != m_dwEndDrag)
		{
			SetCurSel(m_dwBeginDrag, m_dwEndDrag);
			UpdateWindow();
		}
	}
}


void CViewSample::OnLButtonDown(UINT, CPoint point)
//-------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;
	DWORD len;

	if ((m_dwStatus & SMPSTATUS_MOUSEDRAG) || (!pModDoc)) return;
	pSndFile = pModDoc->GetSoundFile();
	len = pSndFile->Ins[m_nSample].nLength;
	if (len)
	{
		m_dwStatus |= SMPSTATUS_MOUSEDRAG;
		SetFocus();
		SetCapture();
		BOOL oldsel = (m_dwBeginSel != m_dwEndSel) ? TRUE : FALSE;
		m_dwBeginDrag = ScreenToSample(point.x);
		if (m_dwBeginDrag >= len) m_dwBeginDrag = len-1;
		m_dwEndDrag = m_dwBeginDrag;
		if (oldsel) SetCurSel(m_dwBeginDrag, m_dwEndDrag);
	}
}


void CViewSample::OnLButtonUp(UINT, CPoint)
//-----------------------------------------
{
	if (m_dwStatus & SMPSTATUS_MOUSEDRAG)
	{
		m_dwStatus &= ~SMPSTATUS_MOUSEDRAG;
		ReleaseCapture();
	}
}


void CViewSample::OnLButtonDblClk(UINT, CPoint)
//---------------------------------------------
{
	CModDoc *pModDoc = GetDocument();

	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		DWORD len = pSndFile->Ins[m_nSample].nLength;
		if (len) SetCurSel(0, len);
	}
}


void CViewSample::OnRButtonDown(UINT, CPoint pt)
//----------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pins = &pSndFile->Ins[m_nSample];
		HMENU hMenu = ::CreatePopupMenu();
		CInputHandler* ih = (CMainFrame::GetMainFrame())->GetInputHandler(); //rewbs.customKeys
		if (!hMenu)	return;
		if (pins->nLength)
		{
			if (m_dwEndSel >= m_dwBeginSel + 4)
			{
				::AppendMenu(hMenu, MF_STRING, ID_SAMPLE_ZOOMONSEL, "Zoom");
				::AppendMenu(hMenu, MF_STRING, ID_SAMPLE_SETLOOP, "Set As Loop");
				if (pSndFile->m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT))
					::AppendMenu(hMenu, MF_STRING, ID_SAMPLE_SETSUSTAINLOOP, "Set As Sustain Loop");
				::AppendMenu(hMenu, MF_SEPARATOR, 0, "");
			} else
			{
				CHAR s[256];
				DWORD dwPos = ScreenToSample(pt.x);
				if (dwPos <= pins->nLength) {
					//Set loop points
					wsprintf(s, "Set Loop Start to:\t%d", dwPos);
					::AppendMenu(hMenu, MF_STRING|((dwPos+4<=pins->nLoopEnd)?0:MF_GRAYED), 
								 ID_SAMPLE_SETLOOPSTART, s);
					wsprintf(s, "Set Loop End to:\t%d", dwPos);
					::AppendMenu(hMenu, MF_STRING|((dwPos>=pins->nLoopStart+4)?0:MF_GRAYED), 
								 ID_SAMPLE_SETLOOPEND, s);
						
					if (pSndFile->m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT)) {
						//Set sustain loop points
						::AppendMenu(hMenu, MF_SEPARATOR, 0, "");
						wsprintf(s, "Set Sustain Start to:\t%d", dwPos);
						::AppendMenu(hMenu, MF_STRING|((dwPos+4<=pins->nSustainEnd)?0:MF_GRAYED), 
	  								 ID_SAMPLE_SETSUSTAINSTART, s);
						wsprintf(s, "Set Sustain End to:\t%d", dwPos);
						::AppendMenu(hMenu, MF_STRING|((dwPos>=pins->nSustainStart+4)?0:MF_GRAYED), 
								     ID_SAMPLE_SETSUSTAINEND, s);
					}
					::AppendMenu(hMenu, MF_SEPARATOR, 0, "");
					m_dwMenuParam = dwPos;
				}
			}
			if (m_dwBeginSel >= m_dwEndSel)
			{
				if (pins->uFlags & CHN_16BIT) ::AppendMenu(hMenu, MF_STRING, ID_SAMPLE_8BITCONVERT, "Convert to 8-bit");
				if (pins->uFlags & CHN_STEREO) ::AppendMenu(hMenu, MF_STRING, ID_SAMPLE_MONOCONVERT, "Convert to mono");
			}
			
			::AppendMenu(hMenu, MF_STRING|(m_dwEndSel>m_dwBeginSel)?0:MF_GRAYED, 
				ID_SAMPLE_TRIM, "Trim\t" + ih->GetKeyTextFromCommand(kcSampleTrim));
			::AppendMenu(hMenu, MF_STRING, ID_EDIT_CUT, "Cut\t" + ih->GetKeyTextFromCommand(kcEditCut));
			::AppendMenu(hMenu, MF_STRING, ID_EDIT_COPY, "Copy\t" + ih->GetKeyTextFromCommand(kcEditCopy));
		}
		::AppendMenu(hMenu, MF_STRING, ID_EDIT_PASTE, "Paste\t" + ih->GetKeyTextFromCommand(kcEditPaste));
		ClientToScreen(&pt);
		::TrackPopupMenu(hMenu, TPM_LEFTALIGN|TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hWnd, NULL);
		::DestroyMenu(hMenu);
	}
}


void CViewSample::OnNcMouseMove(UINT nHitTest, CPoint point)
//----------------------------------------------------------
{
	CRect rect, rcWnd;
	UINT nBtnSel = 0xFFFF;

	GetWindowRect(&rcWnd);
	for (UINT i=0; i<SMP_LEFTBAR_BUTTONS; i++)
	{
		if ((!(m_NcButtonState[i] & NCBTNS_DISABLED)) && (GetNcButtonRect(i, &rect)))
		{
			rect.OffsetRect(rcWnd.left, rcWnd.top);
			if (rect.PtInRect(point))
			{
				nBtnSel = i;
				break;
			}
		}
	}
	if (nBtnSel != m_nBtnMouseOver)
	{
		CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
		if (pMainFrm)
		{
			CString strText = "";
			if ((nBtnSel < SMP_LEFTBAR_BUTTONS) && (cLeftBarButtons[nBtnSel] != ID_SEPARATOR))
			{
				strText.LoadString(cLeftBarButtons[nBtnSel]);
			}
			pMainFrm->SetHelpText(strText);
		}
		m_nBtnMouseOver = nBtnSel;
		UpdateNcButtonState();
	}
	CModScrollView::OnNcMouseMove(nHitTest, point);
}


void CViewSample::OnNcLButtonDown(UINT uFlags, CPoint point)
//----------------------------------------------------------
{
	if (m_nBtnMouseOver < SMP_LEFTBAR_BUTTONS)
	{
		m_dwStatus |= SMPSTATUS_NCLBTNDOWN;
		if (cLeftBarButtons[m_nBtnMouseOver] != ID_SEPARATOR)
		{
			PostMessage(WM_COMMAND, cLeftBarButtons[m_nBtnMouseOver]);
			UpdateNcButtonState();
		}
	}
	CModScrollView::OnNcLButtonDown(uFlags, point);
}


void CViewSample::OnNcLButtonUp(UINT uFlags, CPoint point)
//--------------------------------------------------------
{
	if (m_dwStatus & SMPSTATUS_NCLBTNDOWN)
	{
		m_dwStatus &= ~SMPSTATUS_NCLBTNDOWN;
		UpdateNcButtonState();
	}
	CModScrollView::OnNcLButtonUp(uFlags, point);
}


void CViewSample::OnNcLButtonDblClk(UINT uFlags, CPoint point)
//------------------------------------------------------------
{
	OnNcLButtonDown(uFlags, point);
}


#if _MFC_VER > 0x0710
LRESULT CViewSample::OnNcHitTest(CPoint point)
#else
UINT CViewSample::OnNcHitTest(CPoint point)
#endif 
//-----------------------------------------
{
	CRect rect;
	GetWindowRect(&rect);
	rect.bottom = rect.top + SMP_LEFTBAR_CY;
	if (rect.PtInRect(point))
	{
		return HTBORDER;
	}
	return CModScrollView::OnNcHitTest(point);
}


void CViewSample::OnPrevInstrument()
//----------------------------------
{
	SendCtrlMessage(CTRLMSG_SMP_PREVINSTRUMENT);
}


void CViewSample::OnNextInstrument()
//----------------------------------
{
	SendCtrlMessage(CTRLMSG_SMP_NEXTINSTRUMENT);
}


void CViewSample::OnSetLoop()
//---------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pins = &pSndFile->Ins[m_nSample];
		if ((m_dwEndSel > m_dwBeginSel + 15) && (m_dwEndSel <= pins->nLength))
		{
			if ((pins->nLoopStart != m_dwBeginSel) || (pins->nLoopEnd != m_dwEndSel))
			{
				pins->nLoopStart = m_dwBeginSel;
				pins->nLoopEnd = m_dwEndSel;
				pins->uFlags |= CHN_LOOP;
				pModDoc->SetModified();
				pModDoc->AdjustEndOfSample(m_nSample);
				// 05/01/05 : ericus replaced "m_nSample << 24" by "m_nSample << 20" : 4000 samples -> 12bits [see Moddoc.h]
				pModDoc->UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEINFO | HINT_SAMPLEDATA, NULL);
			}
		}
	}
}


void CViewSample::OnSetSustainLoop()
//----------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pins = &pSndFile->Ins[m_nSample];
		if ((m_dwEndSel > m_dwBeginSel + 15) && (m_dwEndSel <= pins->nLength))
		{
			if ((pins->nSustainStart != m_dwBeginSel) || (pins->nSustainEnd != m_dwEndSel))
			{
				pins->nSustainStart = m_dwBeginSel;
				pins->nSustainEnd = m_dwEndSel;
				pModDoc->SetModified();
				pModDoc->AdjustEndOfSample(m_nSample);
				// 05/01/05 : ericus replaced "m_nSample << 24" by "m_nSample << 20" : 4000 samples -> 12bits [see Moddoc.h]
				pModDoc->UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEINFO | HINT_SAMPLEDATA, NULL);
			}
		}
	}
}


void CViewSample::OnEditSelectAll()
//---------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		DWORD len = pSndFile->Ins[m_nSample].nLength;
		if (len) SetCurSel(0, len);
	}
}


void CViewSample::OnEditDelete()
//------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;
	MODINSTRUMENT *pins;
	DWORD dwUpdateFlags = HINT_SAMPLEINFO | HINT_SAMPLEDATA;
	DWORD len;

	if (!pModDoc) return;
	pSndFile = pModDoc->GetSoundFile();
	pins = &pSndFile->Ins[m_nSample];
	len = pins->nLength;
	if ((!pins->pSample) || (!len)) return;
	if (m_dwEndSel > len) m_dwEndSel = len;
	if ((m_dwBeginSel >= m_dwEndSel)
	 || (m_dwEndSel - m_dwBeginSel + 4 >= len))
	{
		if (MessageBox("Remove this sample ?", "Remove Sample", MB_YESNOCANCEL | MB_ICONQUESTION) != IDYES) return;
		BEGIN_CRITICAL();
		pSndFile->DestroySample(m_nSample);
		END_CRITICAL();
		dwUpdateFlags |= HINT_SMPNAMES;
	} else
	{
		BEGIN_CRITICAL();
		UINT cutlen = m_dwEndSel - m_dwBeginSel;
		UINT istart = m_dwBeginSel;
		UINT iend = len;
		pins->nLength -= cutlen;
		if (pins->uFlags & CHN_16BIT)
		{
			cutlen <<= 1;
			istart <<= 1;
			iend <<= 1;
		}
		if (pins->uFlags & CHN_STEREO)
		{
			cutlen <<= 1;
			istart <<= 1;
			iend <<= 1;
		}
		LPSTR p = pins->pSample;
		for (UINT i=istart; i<iend; i++)
		{
			p[i] = (i+cutlen < iend) ? p[i+cutlen] : (char)0;
		}
		len = pins->nLength;
		if (pins->nLoopEnd > len) pins->nLoopEnd = len;
		if (pins->nLoopStart + 4 >= pins->nLoopEnd)
		{
			pins->nLoopStart = pins->nLoopEnd = 0;
			pins->uFlags &= ~CHN_LOOP;
		}
		END_CRITICAL();
	}
	SetCurSel(0, 0);
	pModDoc->AdjustEndOfSample(m_nSample);
	pModDoc->SetModified();
	// 05/01/05 : ericus replaced "m_nSample << 24" by "m_nSample << 20" : 4000 samples -> 12bits [see Moddoc.h]
	pModDoc->UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | dwUpdateFlags, NULL);
}


void CViewSample::OnEditCut()
//---------------------------
{
	OnEditCopy();
	OnEditDelete();
}


void CViewSample::OnEditCopy()
//----------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;
	MODINSTRUMENT *pins;
	DWORD dwMemSize, dwSmpLen, dwSmpOffset;
	HGLOBAL hCpy;
	BOOL bExtra = TRUE;

	if ((!pMainFrm) || (!pModDoc)) return;
	pSndFile = pModDoc->GetSoundFile();
	pins = &pSndFile->Ins[m_nSample];
	if ((!pins->nLength) || (!pins->pSample)) return;
	dwMemSize = pins->nLength;
	dwSmpOffset = 0;
	if (m_dwEndSel > pins->nLength) m_dwEndSel = pins->nLength;
	if (m_dwEndSel > m_dwBeginSel) { dwMemSize = m_dwEndSel - m_dwBeginSel; dwSmpOffset = m_dwBeginSel; bExtra = FALSE; }
	if (pins->uFlags & CHN_16BIT) { dwMemSize <<= 1; dwSmpOffset <<= 1; }
	if (pins->uFlags & CHN_STEREO) { dwMemSize <<= 1; dwSmpOffset <<= 1; }
	dwSmpLen = dwMemSize;
	dwMemSize += sizeof(WAVEFILEHEADER) + sizeof(WAVEFORMATHEADER) + sizeof(WAVEDATAHEADER)
			 + sizeof(WAVEEXTRAHEADER) + sizeof(WAVESAMPLERINFO);
	// For name + fname
	dwMemSize += 32*2;
	BeginWaitCursor();
	if ((pMainFrm->OpenClipboard()) && ((hCpy = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, dwMemSize))!=NULL))
	{
		EmptyClipboard();
		LPBYTE p = (LPBYTE)GlobalLock(hCpy);
		WAVEFILEHEADER *phdr = (WAVEFILEHEADER *)p;
		WAVEFORMATHEADER *pfmt = (WAVEFORMATHEADER *)(p+sizeof(WAVEFILEHEADER));
		WAVEDATAHEADER *pdata = (WAVEDATAHEADER *)(p+sizeof(WAVEFILEHEADER)+sizeof(WAVEFORMATHEADER));
		phdr->id_RIFF = IFFID_RIFF;
		phdr->filesize = sizeof(WAVEFILEHEADER)+sizeof(WAVEFORMATHEADER)+sizeof(WAVEDATAHEADER)-8;
		phdr->id_WAVE = IFFID_WAVE;
		pfmt->id_fmt = IFFID_fmt;
		pfmt->hdrlen = 16;
		pfmt->format = 1;
		pfmt->freqHz = pins->nC4Speed;
		if (pSndFile->m_nType & (MOD_TYPE_MOD|MOD_TYPE_XM))
		{
			pfmt->freqHz = CSoundFile::TransposeToFrequency(pins->RelativeTone, pins->nFineTune);
		}
		pfmt->channels = (pins->uFlags & CHN_STEREO) ? (WORD)2 : (WORD)1;
		pfmt->bitspersample = (pins->uFlags & CHN_16BIT) ? (WORD)16 : (WORD)8;
		pfmt->samplesize = pfmt->channels*pfmt->bitspersample/8;
		pfmt->bytessec = pfmt->freqHz*pfmt->samplesize;
		pdata->id_data = IFFID_data;
		pdata->length = dwSmpLen;
		phdr->filesize += pdata->length;
		LPBYTE psamples = p+sizeof(WAVEFILEHEADER)+sizeof(WAVEFORMATHEADER)+sizeof(WAVEDATAHEADER);
		memcpy(psamples, pins->pSample+dwSmpOffset, dwSmpLen);
		if (pfmt->bitspersample == 8)
		{
			for (UINT i=0; i<dwSmpLen; i++) psamples[i] += 0x80;
		}
		if (bExtra)
		{
			WAVESMPLHEADER *psh = (WAVESMPLHEADER *)(psamples+dwSmpLen);
			psh->smpl_id = 0x6C706D73;
			psh->smpl_len = sizeof(WAVESMPLHEADER) - 8;
			psh->dwSamplePeriod = 22675;
			if (pins->nC4Speed > 256) psh->dwSamplePeriod = 1000000000 / pins->nC4Speed;
			psh->dwBaseNote = 60;
			if (pins->uFlags & (CHN_LOOP|CHN_SUSTAINLOOP))
			{
				WAVESAMPLERINFO *psmpl = (WAVESAMPLERINFO *)psh;
				if (pins->uFlags & CHN_SUSTAINLOOP)
				{
					psmpl->wsiHdr.dwSampleLoops = 2;
					psmpl->wsiLoops[0].dwLoopType = (pins->uFlags & CHN_PINGPONGSUSTAIN) ? 1 : 0;
					psmpl->wsiLoops[0].dwLoopStart = pins->nSustainStart;
					psmpl->wsiLoops[0].dwLoopEnd = pins->nSustainEnd;
					psmpl->wsiLoops[1].dwLoopType = (pins->uFlags & CHN_PINGPONGLOOP) ? 1 : 0;
					psmpl->wsiLoops[1].dwLoopStart = pins->nLoopStart;
					psmpl->wsiLoops[1].dwLoopEnd = pins->nLoopEnd;
				} else
				{
					psmpl->wsiHdr.dwSampleLoops = 1;
					psmpl->wsiLoops[0].dwLoopType = (pins->uFlags & CHN_PINGPONGLOOP) ? 1 : 0;
					psmpl->wsiLoops[0].dwLoopStart = pins->nLoopStart;
					psmpl->wsiLoops[0].dwLoopEnd = pins->nLoopEnd;
				}
				psmpl->wsiHdr.smpl_len += sizeof(SAMPLELOOPSTRUCT) * psmpl->wsiHdr.dwSampleLoops;
			}
			WAVEEXTRAHEADER *pxh = (WAVEEXTRAHEADER *)(psamples+dwSmpLen+psh->smpl_len+8);
			pxh->xtra_id = IFFID_xtra;
			pxh->xtra_len = sizeof(WAVEEXTRAHEADER)-8;
			pxh->dwFlags = pins->uFlags;
			pxh->wPan = pins->nPan;
			pxh->wVolume = pins->nVolume;
			pxh->wGlobalVol = pins->nGlobalVol;
			pxh->nVibType = pins->nVibType;
			pxh->nVibSweep = pins->nVibSweep;
			pxh->nVibDepth = pins->nVibDepth;
			pxh->nVibRate = pins->nVibRate;
			if ((pSndFile->m_szNames[m_nSample][0]) || (pins->name[0]))
			{
				LPSTR pszText = (LPSTR)(pxh+1);
				memcpy(pszText, pSndFile->m_szNames[m_nSample], 32);
				pxh->xtra_len += 32;
				if (pins->name[0])
				{
					memcpy(pszText+32, pins->name, 22);
					pxh->xtra_len += 22;
				}
			}
			phdr->filesize += sizeof(WAVESMPLHEADER) + pxh->xtra_len + 8;
		}
		GlobalUnlock(hCpy);
		SetClipboardData (CF_WAVE, (HANDLE) hCpy);
		CloseClipboard();
	}
	EndWaitCursor();
}


void CViewSample::OnEditPaste()
//-----------------------------
{
	CModDoc *pModDoc = GetDocument();
	BeginWaitCursor();
	if ((pModDoc) && (OpenClipboard()))
	{
		CHAR s[32], s2[32];
		HGLOBAL hCpy = ::GetClipboardData(CF_WAVE);
		LPBYTE p;

		if ((hCpy) && ((p = (LPBYTE)GlobalLock(hCpy)) != NULL))
		{
			CSoundFile *pSndFile = pModDoc->GetSoundFile();
			DWORD dwMemSize = GlobalSize(hCpy);
			BEGIN_CRITICAL();
			memcpy(s, pSndFile->m_szNames[m_nSample], 32);
			memcpy(s2, pSndFile->Ins[m_nSample].name, 22);
			pSndFile->DestroySample(m_nSample);
			pSndFile->Ins[m_nSample].nLength = 0;
			pSndFile->Ins[m_nSample].pSample = 0;
			pSndFile->ReadSampleFromFile(m_nSample, p, dwMemSize);
			if (!pSndFile->m_szNames[m_nSample][0])
			{
				memcpy(pSndFile->m_szNames[m_nSample], s, 32);
			}
			if (!pSndFile->Ins[m_nSample].name[0])
			{
				memcpy(pSndFile->Ins[m_nSample].name, s2, 22);
			}
			END_CRITICAL();
			GlobalUnlock(hCpy);
			SetCurSel(0, 0);
			pModDoc->AdjustEndOfSample(m_nSample);
			pModDoc->SetModified();
			// 05/01/05 : ericus replaced "m_nSample << 24" by "m_nSample << 20" : 4000 samples -> 12bits [see Moddoc.h]
			pModDoc->UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEINFO | HINT_SAMPLEDATA, NULL);
		}
		CloseClipboard();
	}
	EndWaitCursor();
}


void CViewSample::On8BitConvert()
//-------------------------------
{
	CModDoc *pModDoc = GetDocument();
	BeginWaitCursor();
	if ((pModDoc) && (m_nSample < MAX_SAMPLES))
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pins = &pSndFile->Ins[m_nSample];
		if ((pins->uFlags & CHN_16BIT) && (pins->pSample) && (pins->nLength))
		{
			BEGIN_CRITICAL();
			signed char *p = (signed char *)(pins->pSample);
			UINT len = pins->nLength+1;
			if (pins->uFlags & CHN_STEREO) len *= 2;
			for (UINT i=0; i<=len; i++)
			{
				p[i] = (signed char) ((*((short int *)(p+i*2))) / 256);
			}
			pins->uFlags &= ~(CHN_16BIT);
			for (UINT j=0; j<MAX_CHANNELS; j++) if (pSndFile->Chn[j].pSample == pins->pSample)
			{
				pSndFile->Chn[j].dwFlags &= ~(CHN_16BIT);
			}
			END_CRITICAL();
			pModDoc->SetModified();
			pModDoc->AdjustEndOfSample(m_nSample);
			// 05/01/05 : ericus replaced "m_nSample << 24" by "m_nSample << 20" : 4000 samples -> 12bits [see Moddoc.h]
			pModDoc->UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEDATA | HINT_SAMPLEINFO, NULL);
		}
	}
	EndWaitCursor();
}


void CViewSample::OnMonoConvert()
//-------------------------------
{
	CModDoc *pModDoc = GetDocument();
	BeginWaitCursor();
	if ((pModDoc) && (m_nSample < MAX_SAMPLES))
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pins = &pSndFile->Ins[m_nSample];
		if ((pins->uFlags & CHN_STEREO) && (pins->pSample) && (pins->nLength))
		{
			BEGIN_CRITICAL();
			if (pins->uFlags & CHN_16BIT)
			{
				signed short *p = (signed short *)(pins->pSample);
				for (UINT i=0; i<=pins->nLength+1; i++)
				{
					p[i] = (signed short)((p[i*2]+p[i*2+1]+1)>>1);
				}
			} else
			{
				signed char *p = (signed char *)(pins->pSample);
				for (UINT i=0; i<=pins->nLength+1; i++)
				{
					p[i] = (signed char)((p[i*2]+p[i*2+1]+1)>>1);
				}
			}
			pins->uFlags &= ~(CHN_STEREO);
			for (UINT j=0; j<MAX_CHANNELS; j++) if (pSndFile->Chn[j].pSample == pins->pSample)
			{
				pSndFile->Chn[j].dwFlags &= ~(CHN_STEREO);
			}
			END_CRITICAL();
			pModDoc->SetModified();
			pModDoc->AdjustEndOfSample(m_nSample);
			// 05/01/05 : ericus replaced "m_nSample << 24" by "m_nSample << 20" : 4000 samples -> 12bits [see Moddoc.h]
			pModDoc->UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEDATA | HINT_SAMPLEINFO, NULL);
		}
	}
	EndWaitCursor();
}


void CViewSample::OnSampleTrim()
//------------------------------
{
	CModDoc *pModDoc = GetDocument();
	BeginWaitCursor();
	if ((pModDoc) && (m_nSample < MAX_SAMPLES) && (m_dwBeginSel < m_dwEndSel))
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pins = &pSndFile->Ins[m_nSample];
		UINT nStart = m_dwBeginSel;
		UINT nEnd = m_dwEndSel - m_dwBeginSel;

		if ((pins->pSample) && (nStart+nEnd <= pins->nLength) && (nEnd >= 16))
		{
			BEGIN_CRITICAL();
			{
				UINT bend = nEnd, bstart = nStart;
				if (pins->uFlags & CHN_16BIT) { bend <<= 1; bstart <<= 1; }
				if (pins->uFlags & CHN_STEREO) { bend <<= 1; bstart <<= 1; }
				signed char *p = (signed char *)pins->pSample;
				for (UINT i=0; i<bend; i++)
				{
					p[i] = p[i+bstart];
				}
			}
			if (pins->nLoopStart >= nStart) pins->nLoopStart -= nStart;
			if (pins->nLoopEnd >= nStart) pins->nLoopEnd -= nStart;
			if (pins->nSustainStart >= nStart) pins->nSustainStart -= nStart;
			if (pins->nSustainEnd >= nStart) pins->nSustainEnd -= nStart;
			if (pins->nLoopEnd > nEnd) pins->nLoopEnd = nEnd;
			if (pins->nSustainEnd > nEnd) pins->nSustainEnd = nEnd;
			if (pins->nLoopStart >= pins->nLoopEnd)
			{
				pins->nLoopStart = pins->nLoopEnd = 0;
				pins->uFlags &= ~(CHN_LOOP|CHN_PINGPONGLOOP);
			}
			if (pins->nSustainStart >= pins->nSustainEnd)
			{
				pins->nSustainStart = pins->nSustainEnd = 0;
				pins->uFlags &= ~(CHN_SUSTAINLOOP|CHN_PINGPONGSUSTAIN);
			}
			pins->nLength = nEnd;
			END_CRITICAL();
			pModDoc->SetModified();
			pModDoc->AdjustEndOfSample(m_nSample);
			SetCurSel(0, 0);
			// 05/01/05 : ericus replaced "m_nSample << 24" by "m_nSample << 20" : 4000 samples -> 12bits [see Moddoc.h]
			pModDoc->UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEDATA | HINT_SAMPLEINFO, NULL);
		}
	}
	EndWaitCursor();
}


void CViewSample::OnChar(UINT /*nChar*/, UINT, UINT /*nFlags*/)
//-----------------------------------------------------
{
}

void CViewSample::PlayNote(UINT note)
//-----------------------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = GetDocument();
	if ((pModDoc) && (pMainFrm))
	{
		if (note >= 0xFE)
		{
			pModDoc->NoteOff(0, (note == 0xFE) ? TRUE : FALSE);
		} 
		else
		{
			CHAR s[64];
			if (m_dwStatus & SMPSTATUS_KEYDOWN)
				pModDoc->NoteOff(note, TRUE);
			else
				pModDoc->NoteOff(0, TRUE);
			DWORD loopstart = m_dwBeginSel, loopend = m_dwEndSel;
			if (loopend - loopstart < (UINT)(4 << m_nZoom)) loopend = loopstart = 0;
			pModDoc->PlayNote(note, 0, m_nSample, FALSE, -1, loopstart, loopend);
			m_dwStatus |= SMPSTATUS_KEYDOWN;
			s[0] = 0;
			if ((note) && (note <= NOTE_MAX))
				pMainFrm->SetInfoText(szDefaultNoteNames[note-1]);
			else
				pMainFrm->SetInfoText(s);
		}
	}

}


void CViewSample::OnKeyDown(UINT /*nChar*/, UINT /*nRepCnt*/, UINT /*nFlags*/)
//----------------------------------------------------------------
{
	/*
	switch(nChar)
	{
	case VK_DELETE:
		OnEditDelete();
		break;
	default:
		CModScrollView::OnKeyDown(nChar, nRepCnt, nFlags);
	}
	*/
}


void CViewSample::OnKeyUp(UINT /*nChar*/, UINT /*nRepCnt*/, UINT /*nFlags*/)
//--------------------------------------------------------------
{
	/*
	m_dwStatus &= ~SMPSTATUS_KEYDOWN;
	CModScrollView::OnKeyUp(nChar, nRepCnt, nFlags);
	*/
}


void CViewSample::OnDropFiles(HDROP hDropInfo)
//--------------------------------------------
{
	UINT nFiles = ::DragQueryFile(hDropInfo, (UINT)-1, NULL, 0);
	if (nFiles)
	{
		TCHAR szFileName[_MAX_PATH] = "";
		CMainFrame::GetMainFrame()->SetForegroundWindow();
		::DragQueryFile(hDropInfo, 0, szFileName, _MAX_PATH);
		if (szFileName[0]) SendCtrlMessage(CTRLMSG_SMP_OPENFILE, (LPARAM)szFileName);
	}
	::DragFinish(hDropInfo);
}


BOOL CViewSample::OnDragonDrop(BOOL bDoDrop, LPDRAGONDROP lpDropInfo)
//-------------------------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;
	BOOL bCanDrop = FALSE, bUpdate;

	if ((!lpDropInfo) || (!pModDoc)) return FALSE;
	pSndFile = pModDoc->GetSoundFile();
	switch(lpDropInfo->dwDropType)
	{
	case DRAGONDROP_SAMPLE:
		if (lpDropInfo->pModDoc == pModDoc)
		{
			bCanDrop = ((lpDropInfo->dwDropItem)
					 && (lpDropInfo->dwDropItem <= pSndFile->m_nSamples));
		} else
		{
			bCanDrop = ((lpDropInfo->dwDropItem)
					&& ((lpDropInfo->lDropParam) || (lpDropInfo->pModDoc)));
		}
		break;

	case DRAGONDROP_DLS:
		bCanDrop = ((lpDropInfo->dwDropItem < MAX_DLS_BANKS)
				 && (CTrackApp::gpDLSBanks[lpDropInfo->dwDropItem]));
		break;
	
	case DRAGONDROP_SOUNDFILE:
	case DRAGONDROP_MIDIINSTR:
		bCanDrop = ((lpDropInfo->lDropParam)
				 && (*((LPCSTR)lpDropInfo->lDropParam)));
		break;
	}
	if ((!bCanDrop) || (!bDoDrop)) return bCanDrop;
	// Do the drop
	BeginWaitCursor();
	bUpdate = FALSE;
	switch(lpDropInfo->dwDropType)
	{
	case DRAGONDROP_SAMPLE:
		if (lpDropInfo->pModDoc == pModDoc)
		{
			SendCtrlMessage(CTRLMSG_SETCURRENTINSTRUMENT, lpDropInfo->dwDropItem);
		} else
		{
			SendCtrlMessage(CTRLMSG_SMP_SONGDROP, (LPARAM)lpDropInfo);
		}
		break;

	case DRAGONDROP_MIDIINSTR:
		if (CDLSBank::IsDLSBank((LPCSTR)lpDropInfo->lDropParam))
		{
			CDLSBank dlsbank;
			if (dlsbank.Open((LPCSTR)lpDropInfo->lDropParam))
			{
				DLSINSTRUMENT *pins;
				UINT nIns = 0, nRgn = 0xFF;
				// Drums
				if (lpDropInfo->dwDropItem & 0x80)
				{
					UINT key = lpDropInfo->dwDropItem & 0x7F;
					pins = dlsbank.FindInstrument(TRUE, 0xFFFF, 0xFF, key, &nIns);
					if (pins) nRgn = dlsbank.GetRegionFromKey(nIns, key);
				} else
				// Melodic
				{
					pins = dlsbank.FindInstrument(FALSE, 0xFFFF, lpDropInfo->dwDropItem, 60, &nIns);
					if (pins) nRgn = dlsbank.GetRegionFromKey(nIns, 60);
				}
				bCanDrop = FALSE;
				if (pins)
				{
					BEGIN_CRITICAL();
					bCanDrop = dlsbank.ExtractSample(pSndFile, m_nSample, nIns, nRgn);
					END_CRITICAL();
				}
				bUpdate = TRUE;
				break;
			}
		}
		// Fall through
	case DRAGONDROP_SOUNDFILE:
		SendCtrlMessage(CTRLMSG_SMP_OPENFILE, (LPARAM)lpDropInfo->lDropParam);
		break;
	
	case DRAGONDROP_DLS:
		{
			CDLSBank *pDLSBank = CTrackApp::gpDLSBanks[lpDropInfo->dwDropItem];
			UINT nIns = lpDropInfo->lDropParam & 0x7FFF;
			UINT nRgn;
			// Drums:	(0x80000000) | (Region << 16) | (Instrument)
			if (lpDropInfo->lDropParam & 0x80000000)
			{
				nRgn = (lpDropInfo->lDropParam & 0xFF0000) >> 16;
			} else
			// Melodic: (MidiBank << 16) | (Instrument)
			{
				nRgn = pDLSBank->GetRegionFromKey(nIns, 60);
			}
			BEGIN_CRITICAL();
			bCanDrop = pDLSBank->ExtractSample(pSndFile, m_nSample, nIns, nRgn);
			END_CRITICAL();
			bUpdate = TRUE;
		}
		break;
	}
	if (bUpdate)
	{
		pModDoc->AdjustEndOfSample(m_nSample);
		// 05/01/05 : ericus replaced "m_nSample << 24" by "m_nSample << 20" : 4000 samples -> 12bits [see Moddoc.h]
		pModDoc->UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEDATA | HINT_SAMPLEINFO | HINT_SMPNAMES, NULL);
		pModDoc->SetModified();
	}
	CMDIChildWnd *pMDIFrame = (CMDIChildWnd *)GetParentFrame();
	if (pMDIFrame)
	{
		pMDIFrame->MDIActivate();
		pMDIFrame->SetActiveView(this);
		SetFocus();
	}
	EndWaitCursor();
	return bCanDrop;
}


void CViewSample::OnZoomOnSel()
//-----------------------------
{
	if ((m_dwEndSel > m_dwBeginSel) && (m_rcClient.right > 0))
	{
		DWORD dwZoom = 0;
		
		while ((((DWORD)m_rcClient.right << dwZoom) < (m_dwEndSel - m_dwBeginSel)) && (dwZoom < (MAX_ZOOM-1))) dwZoom++;
		if (dwZoom < (MAX_ZOOM-1)) dwZoom++; else dwZoom = 0;
		SendCtrlMessage(CTRLMSG_SMP_SETZOOM, dwZoom);
		if (dwZoom)
		{
			LONG l;
			CSize sz;
			SetZoom(dwZoom);
			UpdateScrollSize();
			sz.cx = m_dwBeginSel >> (dwZoom-1);
			l = (m_rcClient.right - ((m_dwEndSel - m_dwBeginSel) >> (dwZoom-1))) / 2;
			if (l > 0) sz.cx -= l;
			sz.cx >>= m_nScrollFactor;
			sz.cx -= GetScrollPos(SB_HORZ);
			sz.cy = 0;
			OnScrollBy(sz, TRUE);
		}
	} else
	{
		SendCtrlMessage(CTRLMSG_SMP_SETZOOM, 0);
	}
}


void CViewSample::OnSetLoopStart()
//--------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pins = &pSndFile->Ins[m_nSample];
		if ((m_dwMenuParam+4 <= pins->nLoopEnd) && (pins->nLoopStart != m_dwMenuParam))
		{
			pins->nLoopStart = m_dwMenuParam;
			pModDoc->SetModified();
			pModDoc->AdjustEndOfSample(m_nSample);
			// 05/01/05 : ericus replaced "m_nSample << 24" by "m_nSample << 20" : 4000 samples -> 12bits [see Moddoc.h]
			pModDoc->UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEINFO | HINT_SAMPLEDATA, NULL);
		}
	}
}


void CViewSample::OnSetLoopEnd()
//------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pins = &pSndFile->Ins[m_nSample];
		if ((m_dwMenuParam >= pins->nLoopStart+4) && (pins->nLoopEnd != m_dwMenuParam))
		{
			pins->nLoopEnd = m_dwMenuParam;
			pModDoc->SetModified();
			pModDoc->AdjustEndOfSample(m_nSample);
			// 05/01/05 : ericus replaced "m_nSample << 24" by "m_nSample << 20" : 4000 samples -> 12bits [see Moddoc.h]
			pModDoc->UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEINFO | HINT_SAMPLEDATA, NULL);
		}
	}
}


void CViewSample::OnSetSustainStart()
//-----------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pins = &pSndFile->Ins[m_nSample];
		if ((m_dwMenuParam+4 <= pins->nSustainEnd) && (pins->nSustainStart != m_dwMenuParam))
		{
			pins->nSustainStart = m_dwMenuParam;
			pModDoc->SetModified();
			pModDoc->AdjustEndOfSample(m_nSample);
			// 05/01/05 : ericus replaced "m_nSample << 24" by "m_nSample << 20" : 4000 samples -> 12bits [see Moddoc.h]
			pModDoc->UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEINFO | HINT_SAMPLEDATA, NULL);
		}
	}
}


void CViewSample::OnSetSustainEnd()
//---------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pins = &pSndFile->Ins[m_nSample];
		if ((m_dwMenuParam >= pins->nSustainStart+4) && (pins->nSustainEnd != m_dwMenuParam))
		{
			pins->nSustainEnd = m_dwMenuParam;
			pModDoc->SetModified();
			pModDoc->AdjustEndOfSample(m_nSample);
			// 05/01/05 : ericus replaced "m_nSample << 24" by "m_nSample << 20" : 4000 samples -> 12bits [see Moddoc.h]
			pModDoc->UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEINFO | HINT_SAMPLEDATA, NULL);
		}
	}
}


void CViewSample::OnZoomUp()
//--------------------------
{
	if (m_nZoom >= MIN_ZOOM)
		SendCtrlMessage(CTRLMSG_SMP_SETZOOM, (m_nZoom>MIN_ZOOM) ? m_nZoom-1 : MAX_ZOOM);
}


void CViewSample::OnZoomDown()
//----------------------------
{
	SendCtrlMessage(CTRLMSG_SMP_SETZOOM, (m_nZoom<MAX_ZOOM) ? m_nZoom+1 : MIN_ZOOM);
}


LRESULT CViewSample::OnMidiMsg(WPARAM dwMidiDataParam, LPARAM)
//-------------------------------------------------------
{
	const DWORD dwMidiData = dwMidiDataParam;
	static BYTE midivolume = 127;

	CModDoc *pModDoc = GetDocument();
	BYTE midibyte1 = GetFromMIDIMsg_DataByte1(dwMidiData);
	BYTE midibyte2 = GetFromMIDIMsg_DataByte2(dwMidiData);

	CSoundFile* pSndFile = (pModDoc) ? pModDoc->GetSoundFile() : NULL;
	if (!pSndFile) return 0;

	const BYTE nNote  = midibyte1 + 1;		// +1 is for MPT, where middle C is 61
	int nVol   = midibyte2;					
	BYTE event  = GetFromMIDIMsg_Event(dwMidiData);
	if ((event == 0x9) && !nVol) event = 0x8;	//Convert event to note-off if req'd

	switch(event)
	{
		case 0x8: // Note Off
			midibyte2 = 0;

		case 0x9: // Note On
			pModDoc->NoteOff(nNote, true);
			if (midibyte2 & 0x7F)
			{
				nVol = ApplyVolumeRelatedMidiSettings(dwMidiData, midivolume);
				pModDoc->PlayNote(nNote, 0, m_nSample, FALSE, nVol);
			}
		break;

		case 0xB: //Controller change
			switch(midibyte1)
			{
				case 0x7: //Volume
					midivolume = midibyte2;
				break;
			}
		break;
	}

	return 0;
}

BOOL CViewSample::PreTranslateMessage(MSG *pMsg)
//-----------------------------------------------
{
	if (pMsg)
	{
		//We handle keypresses before Windows has a chance to handle them (for alt etc..)
		if ((pMsg->message == WM_SYSKEYUP)   || (pMsg->message == WM_KEYUP) || 
			(pMsg->message == WM_SYSKEYDOWN) || (pMsg->message == WM_KEYDOWN))
		{
			CInputHandler* ih = (CMainFrame::GetMainFrame())->GetInputHandler();
			
			//Translate message manually
			UINT nChar = pMsg->wParam;
			UINT nRepCnt = LOWORD(pMsg->lParam);
			UINT nFlags = HIWORD(pMsg->lParam);
			KeyEventType kT = ih->GetKeyEventType(nFlags);
			InputTargetContext ctx = (InputTargetContext)(kCtxViewSamples);
			
			if (ih->KeyEvent(ctx, nChar, nRepCnt, nFlags, kT) != kcNull)
				return true; // Mapped to a command, no need to pass message on.
		}

	}
	
	return CModScrollView::PreTranslateMessage(pMsg);
}

LRESULT CViewSample::OnCustomKeyMsg(WPARAM wParam, LPARAM /*lParam*/)
//---------------------------------------------------------------
{
	if (wParam == kcNull)
		return NULL;
	
	CModDoc *pModDoc = GetDocument();	
	if (!pModDoc) return NULL;
	
	//CSoundFile *pSndFile = pModDoc->GetSoundFile();	
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();

	switch(wParam)
	{
		case kcSampleTrim:		OnSampleTrim() ; return wParam;
		case kcSampleZoomUp:	OnZoomUp(); return wParam;
		case kcSampleZoomDown:	OnZoomDown(); return wParam;
		case kcPrevInstrument:	OnPrevInstrument(); return wParam;
		case kcNextInstrument:	OnNextInstrument(); return wParam;
		case kcEditSelectAll:	OnEditSelectAll(); return wParam;
		case kcSampleDelete:	OnEditDelete(); return wParam;
		case kcEditCut:			OnEditCut(); return wParam;
		case kcEditCopy:		OnEditCopy(); return wParam;
		case kcEditPaste:		OnEditPaste(); return wParam;

		case kcSampleLoad:		PostCtrlMessage(IDC_SAMPLE_OPEN); return wParam;
		case kcSampleSave:		PostCtrlMessage(IDC_SAMPLE_SAVEAS); return wParam;
		case kcSampleNew:		PostCtrlMessage(IDC_SAMPLE_NEW); return wParam;
								
		case kcSampleReverse:	PostCtrlMessage(IDC_SAMPLE_REVERSE); return wParam;
		case kcSampleSilence:	PostCtrlMessage(IDC_SAMPLE_SILENCE); return wParam;
		case kcSampleNormalize:	PostCtrlMessage(IDC_SAMPLE_NORMALIZE); return wParam;
		case kcSampleAmplify:	PostCtrlMessage(IDC_SAMPLE_AMPLIFY); return wParam;

		case kcNoteOff:			PlayNote(255); return wParam;
		case kcNoteCut:			PlayNote(254); return wParam;

	}
	if (wParam>=kcSampStartNotes && wParam<=kcSampEndNotes)
	{
		PlayNote(wParam-kcSampStartNotes+1+pMainFrm->GetBaseOctave()*12);
		return wParam;
	}
	if (wParam>=kcSampStartNoteStops && wParam<=kcSampEndNoteStops)
	{
		m_dwStatus &= ~SMPSTATUS_KEYDOWN;
		return wParam;
	}

	return NULL;
}
