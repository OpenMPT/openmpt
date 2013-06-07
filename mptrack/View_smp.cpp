/*
 * view_smp.cpp
 * ------------
 * Purpose: Sample tab, lower panel.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "mptrack.h"
#include "mainfrm.h"
#include "childfrm.h"
#include "moddoc.h"
#include "globals.h"
#include "ctrl_smp.h"
#include "Dlsbank.h"
#include "channelManagerDlg.h"
#include "view_smp.h"
#include "../soundlib/MIDIEvents.h"
#include "SampleEditorDialogs.h"
#include "modsmp_ctrl.h"
#include "Wav.h"

#define new DEBUG_NEW



// Non-client toolbar
#define SMP_LEFTBAR_CY			29
#define SMP_LEFTBAR_CXSEP		14
#define SMP_LEFTBAR_CXSPC		3
#define SMP_LEFTBAR_CXBTN		24
#define SMP_LEFTBAR_CYBTN		22

#define MIN_ZOOM	0
#define MAX_ZOOM	8

// Defines the minimum length for selection for which
// trimming will be done. This is the minimum value for
// selection difference, so the minimum length of result
// of trimming is nTrimLengthMin + 1.
#define MIN_TRIM_LENGTH			16

const UINT cLeftBarButtons[SMP_LEFTBAR_BUTTONS] =
{
	ID_SAMPLE_ZOOMUP,
	ID_SAMPLE_ZOOMDOWN,
		ID_SEPARATOR,
	ID_SAMPLE_DRAW,
	ID_SAMPLE_ADDSILENCE,
		ID_SEPARATOR,
	ID_SAMPLE_GRID,
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
	ON_WM_DROPFILES()
	ON_WM_MOUSEWHEEL()
	ON_COMMAND(ID_EDIT_UNDO,				OnEditUndo)
	ON_COMMAND(ID_EDIT_SELECT_ALL,			OnEditSelectAll)
	ON_COMMAND(ID_EDIT_CUT,					OnEditCut)
	ON_COMMAND(ID_EDIT_COPY,				OnEditCopy)
	ON_COMMAND(ID_EDIT_PASTE,				OnEditPaste)
	ON_COMMAND(ID_SAMPLE_SETLOOP,			OnSetLoop)
	ON_COMMAND(ID_SAMPLE_SETSUSTAINLOOP,	OnSetSustainLoop)
	ON_COMMAND(ID_SAMPLE_8BITCONVERT,		On8BitConvert)
	ON_COMMAND(ID_SAMPLE_MONOCONVERT,		OnMonoConvertMix)
	ON_COMMAND(ID_SAMPLE_MONOCONVERT_LEFT,	OnMonoConvertLeft)
	ON_COMMAND(ID_SAMPLE_MONOCONVERT_RIGHT,	OnMonoConvertRight)
	ON_COMMAND(ID_SAMPLE_MONOCONVERT_SPLIT,	OnMonoConvertSplit)
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
	ON_COMMAND(ID_SAMPLE_DRAW,				OnDrawingToggle)
	ON_COMMAND(ID_SAMPLE_ADDSILENCE,		OnAddSilence)
	ON_COMMAND(ID_SAMPLE_GRID,				OnChangeGridSize)
	ON_COMMAND(ID_SAMPLE_QUICKFADE,			OnQuickFade)
	ON_MESSAGE(WM_MOD_MIDIMSG,				OnMidiMsg)
	ON_MESSAGE(WM_MOD_KEYCOMMAND,			OnCustomKeyMsg) //rewbs.customKeys
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


///////////////////////////////////////////////////////////////
// CViewSample operations

CViewSample::CViewSample()
//------------------------
{
	m_nGridSegments = 0;
	m_nSample = 1;
	m_nZoom = 0;
	m_nScrollPos = 0;
	m_dwStatus = 0;
	m_nScrollFactor = 0;
	m_nBtnMouseOver = 0xFFFF;
	for(CHANNELINDEX i = 0; i < MAX_CHANNELS; i++)
	{
		m_dwNotifyPos[i] = Notification::PosInvalid;
	}
	MemsetZero(m_NcButtonState);
	m_bmpEnvBar.Create(IDB_SMPTOOLBAR, 20, 0, RGB(192,192,192));
	m_lastDrawPoint.SetPoint(-1, -1);
}


void CViewSample::OnInitialUpdate()
//---------------------------------
{
	m_dwBeginSel = m_dwEndSel = 0;
	m_bDrawingEnabled = false; // sample drawing
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


void CViewSample::UpdateScrollSize(const UINT nZoomOld)
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

		if ((m_nSample > 0) && (m_nSample <= pSndFile->GetNumSamples()))
		{
			const ModSample &sample = pSndFile->GetSample(m_nSample);
			if (sample.pSample != nullptr) dwLen = sample.nLength;
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
		// Limit scroll size. FIXME: For long samples, this causes the last few sampling points to be invisible! (Rounding error?)
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

		if (nZoomOld != m_nZoom) // After zoom change, keep the view position.
		{
			const UINT nOldPos = ScrollPosToSamplePos(nZoomOld);
			const float fPosFraction = (dwLen > 0) ? static_cast<float>(nOldPos) / dwLen : 0;
			m_nScrollPos = 0;
			m_nScrollPos = SampleToScreen(nOldPos);
			SetScrollPos(SB_HORZ, static_cast<int>(fPosFraction * GetScrollLimit(SB_HORZ)));
		}
		else
		{
			m_nScrollPos = GetScrollPos(SB_HORZ) << m_nScrollFactor;
		}
	}
}


BOOL CViewSample::SetCurrentSample(SAMPLEINDEX nSmp)
//--------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;

	if (!pModDoc) return FALSE;
	pSndFile = pModDoc->GetSoundFile();
	if ((nSmp < 1) || (nSmp > pSndFile->m_nSamples)) return FALSE;
	pModDoc->SetNotifications(Notification::Sample, nSmp);
	pModDoc->SetFollowWnd(m_hWnd);
	if (nSmp == m_nSample) return FALSE;
	m_dwBeginSel = m_dwEndSel = 0;
	m_bDrawingEnabled = false; // sample drawing
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if (pMainFrm) pMainFrm->SetInfoText("");
	m_nSample = nSmp;
	for(CHANNELINDEX i = 0; i < MAX_CHANNELS; i++)
	{
		m_dwNotifyPos[i] = Notification::PosInvalid;
	}
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

	const UINT nZoomOld = m_nZoom;
	m_nZoom = nZoom;
	UpdateScrollSize(nZoomOld);
	InvalidateRect(NULL, FALSE);
	return TRUE;
}


void CViewSample::SetCurSel(SmpLength nBegin, SmpLength nEnd)
//-----------------------------------------------------------
{
	CSoundFile *pSndFile = (GetDocument()) ? GetDocument()->GetSoundFile() : nullptr;
	if(pSndFile == nullptr)
		return;

	// Snap to grid
	if(m_nGridSegments > 0)
	{
		const float sampsPerSegment = (float)(pSndFile->GetSample(m_nSample).nLength / m_nGridSegments);
		nBegin = (SmpLength)(floor((float)(nBegin / sampsPerSegment) + 0.5f) * sampsPerSegment);
		nEnd = (SmpLength)(floor((float)(nEnd / sampsPerSegment) + 0.5f) * sampsPerSegment);
	}

	if (nBegin > nEnd)
	{
		std::swap(nBegin, nEnd);
	}

	if ((nBegin != m_dwBeginSel) || (nEnd != m_dwEndSel))
	{
		RECT rect;
		SmpLength dMin = m_dwBeginSel, dMax = m_dwEndSel;
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
		if(pMainFrm)
		{
			CHAR s[64];
			s[0] = 0;
			if(m_dwEndSel > m_dwBeginSel)
			{
				const SmpLength selLength = m_dwEndSel - m_dwBeginSel;
				wsprintf(s, "[%u,%u] (%u sample%s, ", m_dwBeginSel, m_dwEndSel, selLength, (selLength == 1) ? "" : "s");
				uint32 lSampleRate = pSndFile->GetSample(m_nSample).nC5Speed;
				if(pSndFile->GetType() & (MOD_TYPE_MOD|MOD_TYPE_XM))
				{
					lSampleRate = ModSample::TransposeToFrequency(pSndFile->GetSample(m_nSample).RelativeTone, pSndFile->GetSample(m_nSample).nFineTune);
				}
				if (!lSampleRate) lSampleRate = 8363;
				uint64 msec = (uint64(selLength) * 1000) / lSampleRate;
				if(msec < 1000)
					wsprintf(s + strlen(s), "%lums)", msec);
				else
					wsprintf(s + strlen(s), "%lu.%lus)", msec / 1000, (msec / 100) % 10);
			}
			pMainFrm->SetInfoText(s);
		}
	}
}


LONG CViewSample::SampleToScreen(LONG n) const
//--------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if ((pModDoc) && (m_nSample <= pModDoc->GetNumSamples()))
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		UINT nLen = pSndFile->GetSample(m_nSample).nLength;
		if (!nLen) return 0;
		if (m_nZoom)
		{
			return (n >> ((LONG)m_nZoom-1)) - m_nScrollPos;
		} else
		{
			return Util::muldiv(n, m_sizeTotal.cx, nLen);
		}
	}
	return 0;
}


DWORD CViewSample::ScreenToSample(LONG x) const
//---------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	LONG n = 0;

	if ((pModDoc) && (m_nSample <= pModDoc->GetNumSamples()))
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		UINT nLen = pSndFile->GetSample(m_nSample).nLength;
		if (!nLen) return 0;
		if (m_nZoom)
		{
			n = (m_nScrollPos + x) << (m_nZoom-1);
		} else
		{
			if (x < 0) x = 0;
			if (m_sizeTotal.cx) n = Util::muldiv(x, nLen, m_sizeTotal.cx);
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
	const SAMPLEINDEX updateSmp = (dwHintMask >> HINT_SHIFT_SMP);
	if((dwHintMask & (HINT_MPTOPTIONS | HINT_MODTYPE))
		|| ((dwHintMask & HINT_SAMPLEDATA) && (m_nSample == updateSmp || updateSmp == 0)))
	{
		UpdateScrollSize();
		UpdateNcButtonState();
		InvalidateSample();
	}

	// sample drawing
	if(dwHintMask & HINT_SAMPLEINFO)
	{
		m_bDrawingEnabled = false;
		UpdateNcButtonState();
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


#if defined(ENABLE_X86_AMD) || defined(ENABLE_SSE)

static void amdmmxext_or_sse_findminmax16(void *p, int scanlen, int smplsize, int *smin, int *smax)
//-------------------------------------------------------------------------------------------------
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


static void amdmmxext_or_sse_findminmax8(void *p, int scanlen, int smplsize, int *smin, int *smax)
//------------------------------------------------------------------------------------------------
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
		//posincr = Util::muldiv(len, 0x10000, cx);
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
#if defined(ENABLE_X86_AMD) || defined(ENABLE_SSE)
			if(GetProcSupport() & (PROCSUPPORT_AMD_MMXEXT|PROCSUPPORT_SSE))
			{
				amdmmxext_or_sse_findminmax16(p, scanlen, smplsize, &smin, &smax);
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
#if defined(ENABLE_X86_AMD) || defined(ENABLE_SSE)
			if(GetProcSupport() & (PROCSUPPORT_AMD_MMXEXT|PROCSUPPORT_SSE))
			{
				amdmmxext_or_sse_findminmax8(p, scanlen, smplsize, &smin, &smax);
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

	UINT nSmpScrollPos = ScrollPosToSamplePos();

	if ((!pModDoc) || (!pDC)) return;
	hdc = pDC->m_hDC;
	oldpen = ::SelectObject(hdc, CMainFrame::penBlack);
	pSndFile = pModDoc->GetSoundFile();
	rect = rcClient;
	if ((rcClient.bottom > rcClient.top) && (rcClient.right > rcClient.left))
	{
		const ModSample &sample = pSndFile->GetSample((m_nSample <= pSndFile->GetNumSamples()) ? m_nSample : 0);
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
		if (sample.uFlags & CHN_STEREO)
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
		if ((sample.pSample) && (yrange) && (sample.nLength > 1) && (rect.right > 1))
		{
			// Loop Start/End
			if ((sample.nLoopEnd > nSmpScrollPos) && (sample.nLoopEnd > sample.nLoopStart))
			{
				int xl = SampleToScreen(sample.nLoopStart);
				if ((xl >= 0) && (xl < rcClient.right))
				{
					::MoveToEx(hdc, xl, rect.top, NULL);
					::LineTo(hdc, xl, rect.bottom);
				}
				xl = SampleToScreen(sample.nLoopEnd);
				if ((xl >= 0) && (xl < rcClient.right))
				{
					::MoveToEx(hdc, xl, rect.top, NULL);
					::LineTo(hdc, xl, rect.bottom);
				}
			}
			// Sustain Loop Start/End
			if ((sample.nSustainEnd > nSmpScrollPos) && (sample.nSustainEnd > sample.nSustainStart))
			{
				::SelectObject(hdc, CMainFrame::penHalfDarkGray);
				int xl = SampleToScreen(sample.nSustainStart);
				if ((xl >= 0) && (xl < rcClient.right))
				{
					::MoveToEx(hdc, xl, rect.top, NULL);
					::LineTo(hdc, xl, rect.bottom);
				}
				xl = SampleToScreen(sample.nSustainEnd);
				if ((xl >= 0) && (xl < rcClient.right))
				{
					::MoveToEx(hdc, xl, rect.top, NULL);
					::LineTo(hdc, xl, rect.bottom);
				}
			}
			// Drawing Sample Data
			::SelectObject(hdc, CMainFrame::penSample);
			int smplsize = (sample.uFlags & CHN_16BIT) ? 2 : 1;
			if (sample.uFlags & CHN_STEREO) smplsize *= 2;
			if ((m_nZoom == 1) || ((!m_nZoom) && (sample.nLength <= (UINT)rect.right)))
			{
				int len = sample.nLength - nSmpScrollPos;
				signed char *psample = ((signed char *)sample.pSample) + nSmpScrollPos * smplsize;
				if (sample.uFlags & CHN_STEREO)
				{
					DrawSampleData1(hdc, ymed-yrange/2, rect.right, yrange, len, sample.uFlags, psample);
					DrawSampleData1(hdc, ymed+yrange/2, rect.right, yrange, len, sample.uFlags, psample+smplsize/2);
				} else
				{
					DrawSampleData1(hdc, ymed, rect.right, yrange*2, len, sample.uFlags, psample);
				}
			} else
			{
				int len = sample.nLength;
				int xscroll = 0;
				if (m_nZoom)
				{
					xscroll = nSmpScrollPos;
					len -= nSmpScrollPos;
				}
				signed char *psample = ((signed char *)sample.pSample) + xscroll * smplsize;
				if (sample.uFlags & CHN_STEREO)
				{
					DrawSampleData2(hdc, ymed-yrange/2, rect.right, yrange, len, sample.uFlags, psample);
					DrawSampleData2(hdc, ymed+yrange/2, rect.right, yrange, len, sample.uFlags, psample+smplsize/2);
				} else
				{
					DrawSampleData2(hdc, ymed, rect.right, yrange*2, len, sample.uFlags, psample);
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
	if(GetDocument()->GetrSoundFile().GetSample(m_nSample).pSample == nullptr)
	{
		return;
	}
	for (UINT i=0; i<MAX_CHANNELS; i++) if (m_dwNotifyPos[i] != Notification::PosInvalid)
	{
		rect.top = -2;
		rect.left = SampleToScreen(m_dwNotifyPos[i]);
		rect.right = rect.left + 1;
		rect.bottom = m_rcClient.bottom + 1;
		if ((rect.right >= 0) && (rect.right < m_rcClient.right)) InvertRect(hdc, &rect);
	}
}


LRESULT CViewSample::OnPlayerNotify(Notification *pnotify)
//--------------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if ((!pnotify) || (!pModDoc)) return 0;
	if (pnotify->type[Notification::Stop])
	{
		bool invalidate = false;
		for(CHANNELINDEX i = 0; i < MAX_CHANNELS; i++)
		{
			if(m_dwNotifyPos[i] != Notification::PosInvalid)
			{
				m_dwNotifyPos[i] = Notification::PosInvalid;
				invalidate = true;
			}
		}
		if(invalidate)
		{
			InvalidateRect(NULL, FALSE);
		}
	} else if (pnotify->type[Notification::Sample] && pnotify->item == m_nSample)
	{
		bool doUpdate = false;
		for(CHANNELINDEX i = 0; i < MAX_CHANNELS; i++)
		{
			SmpLength newpos = pnotify->pos[i];
			if (m_dwNotifyPos[i] != newpos)
			{
				doUpdate = true;
				break;
			}
		}
		if (doUpdate)
		{
			HDC hdc = ::GetDC(m_hWnd);
			DrawPositionMarks(hdc);
			for(CHANNELINDEX i = 0; i < MAX_CHANNELS; i++)
			{
				m_dwNotifyPos[i] = pnotify->pos[i];
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
		if (!(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_FLATBUTTONS))
		{
			c1 = c3 = crHi;
			c2 = crDk;
			c4 = RGB(0,0,0);
		}
		if (dwStyle & (NCBTNS_PUSHED|NCBTNS_CHECKED))
		{
			c1 = crDk;
			c2 = crHi;
			if (!(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_FLATBUTTONS))
			{
				c4 = crHi;
				c3 = (dwStyle & NCBTNS_PUSHED) ? RGB(0,0,0) : crDk;
			}
			xofs = yofs = 1;
		} else
		if ((dwStyle & NCBTNS_MOUSEOVER) && (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_FLATBUTTONS))
		{
			c1 = crHi;
			c2 = crDk;
		}
		switch(cLeftBarButtons[nBtn])
		{
		case ID_SAMPLE_ZOOMUP:		nImage = SIMAGE_ZOOMUP; break;
		case ID_SAMPLE_ZOOMDOWN:	nImage = SIMAGE_ZOOMDOWN; break;
		case ID_SAMPLE_DRAW:		nImage = (dwStyle & NCBTNS_DISABLED) ? SIMAGE_NODRAW : SIMAGE_DRAW; break;
		case ID_SAMPLE_ADDSILENCE:	nImage = SIMAGE_RESIZE; break;
		case ID_SAMPLE_GRID:		nImage = SIMAGE_GRID; break;
		}
		pDC->Draw3dRect(rect.left-1, rect.top-1, SMP_LEFTBAR_CXBTN+2, SMP_LEFTBAR_CYBTN+2, c3, c4);
		pDC->Draw3dRect(rect.left, rect.top, SMP_LEFTBAR_CXBTN, SMP_LEFTBAR_CYBTN, c1, c2);
		rect.DeflateRect(1, 1);
		pDC->FillSolidRect(&rect, crFc);
		rect.left += xofs;
		rect.top += yofs;
		if (dwStyle & NCBTNS_CHECKED) m_bmpEnvBar.Draw(pDC, SIMAGE_CHECKED, rect.TopLeft(), ILD_NORMAL);
		m_bmpEnvBar.Draw(pDC, nImage, rect.TopLeft(), ILD_NORMAL);
	} else
	{
		c1 = c2 = crFc;
		if (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_FLATBUTTONS)
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

		switch(cLeftBarButtons[i])
		{
			case ID_SAMPLE_DRAW:
				if(m_bDrawingEnabled) dwStyle |= NCBTNS_CHECKED;
				if(m_nSample > pSndFile->GetNumSamples() ||
					pSndFile->GetSample(m_nSample).GetNumChannels() > 1 ||
					pSndFile->GetSample(m_nSample).pSample == nullptr)
				{
					dwStyle |= NCBTNS_DISABLED;
				}
				break;
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


template<class T, class uT>
T CViewSample::GetSampleValueFromPoint(const CPoint &point)
//---------------------------------------------------------
{
	STATIC_ASSERT(sizeof(T) == sizeof(uT) && sizeof(T) <= 2);
	int value = (std::numeric_limits<T>::max)() - (std::numeric_limits<uT>::max)() * point.y / m_rcClient.Height();
	Limit(value, (std::numeric_limits<T>::min)(), (std::numeric_limits<T>::max)());
	return static_cast<T>(value);
}


template<class T, class uT>
void CViewSample::SetInitialDrawPoint(void *pSample, const CPoint &point)
//-----------------------------------------------------------------------
{
	T* data = static_cast<T *>(pSample);
	data[m_dwEndDrag] = GetSampleValueFromPoint<T, uT>(point);
}


template<class T, class uT>
void CViewSample::SetSampleData(void *pSample, const CPoint &point, const DWORD old)
//----------------------------------------------------------------------------------
{
	T* data = static_cast<T *>(pSample);
	const int oldvalue = data[old];
	const int value = GetSampleValueFromPoint<T, uT>(point);
	for(DWORD i=old; i != m_dwEndDrag; i += (m_dwEndDrag > old ? 1 : -1))
	{
		data[i] = static_cast<T>((float)oldvalue + (value - oldvalue) * ((float)i - old) / ((float)m_dwEndDrag - old));
	}
	data[m_dwEndDrag] = static_cast<T>(value);
}


void CViewSample::OnMouseMove(UINT, CPoint point)
//-----------------------------------------------
{
	CHAR s[64];
	CModDoc *pModDoc = GetDocument();

	if ((m_nBtnMouseOver < SMP_LEFTBAR_BUTTONS) || (m_dwStatus & SMPSTATUS_NCLBTNDOWN))
	{
		m_dwStatus &= ~SMPSTATUS_NCLBTNDOWN;
		m_nBtnMouseOver = 0xFFFF;
		UpdateNcButtonState();
		CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
		if (pMainFrm) pMainFrm->SetHelpText("");
	}
	if (!pModDoc) return;
	CSoundFile &sndFile = pModDoc->GetrSoundFile();
	if (m_rcClient.PtInRect(point))
	{
		const DWORD x = ScreenToSample(point.x);
		wsprintf(s, "Cursor: %u", x);
		UpdateIndicator(s);
		CMainFrame *pMainFrm = CMainFrame::GetMainFrame();

		if (pMainFrm && m_dwEndSel <= m_dwBeginSel)
		{
			// Show cursor position as offset effect if no selection is made.
			if(m_nSample > 0 && m_nSample <= sndFile.GetNumSamples() && x < sndFile.GetSample(m_nSample).nLength)
			{
				const DWORD xLow = (x / 0x100) % 0x100;
				const DWORD xHigh = x / 0x10000;

				const char cOffsetChar = sndFile.GetModSpecifications().GetEffectLetter(CMD_OFFSET);
				const bool bHasHighOffset = (sndFile.TypeIsS3M_IT_MPT() || (sndFile.GetType() == MOD_TYPE_XM));
				const char cHighOffsetChar = sndFile.GetModSpecifications().GetEffectLetter(static_cast<ModCommand::COMMAND>(sndFile.TypeIsS3M_IT_MPT() ? CMD_S3MCMDEX : CMD_XFINEPORTAUPDOWN));

				if(xHigh == 0)
					wsprintf(s, "Offset: %c%02X", cOffsetChar, xLow);
				else if(bHasHighOffset && xHigh < 0x10)
					wsprintf(s, "Offset: %c%02X, %cA%X", cOffsetChar, xLow, cHighOffsetChar, xHigh);
				else
					wsprintf(s, "Beyond offset range");
				pMainFrm->SetInfoText(s);
			}
			else
			{
				pMainFrm->SetInfoText("");
			}
		}
	} else UpdateIndicator(NULL);
	if (m_dwStatus & SMPSTATUS_MOUSEDRAG)
	{
		BOOL bAgain = FALSE;
		const DWORD len = sndFile.GetSample(m_nSample).nLength;
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
		m_dwEndDrag = ScreenToSample(point.x);
		if(m_bDrawingEnabled)
		{
			if(m_dwEndDrag < len)
			{
				// Shift = draw horizontal lines
				if(CMainFrame::GetInputHandler()->ShiftPressed())
				{
					if(m_lastDrawPoint.y != -1)
						point.y = m_lastDrawPoint.y;
					m_lastDrawPoint = point;
				} else
				{
					m_lastDrawPoint.SetPoint(-1, -1);
				}

				if(sndFile.GetSample(m_nSample).GetElementarySampleSize() == 2)
					SetSampleData<int16, uint16>(sndFile.GetSample(m_nSample).pSample, point, old);
				else if(sndFile.GetSample(m_nSample).GetElementarySampleSize() == 1)
					SetSampleData<int8, uint8>(sndFile.GetSample(m_nSample).pSample, point, old);

				ctrlSmp::AdjustEndOfSample(sndFile.GetSample(m_nSample), sndFile);

				InvalidateSample();
				pModDoc->SetModified();
			}
		}
		else if (old != m_dwEndDrag)
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
	ModSample &sample = pSndFile->GetSample(m_nSample);

	len = sample.nLength;
	if (!len)
		return;

	m_dwStatus |= SMPSTATUS_MOUSEDRAG;
	SetFocus();
	SetCapture();
	bool oldsel = (m_dwBeginSel != m_dwEndSel);

	// shift + click = update selection
	if(!m_bDrawingEnabled && CMainFrame::GetInputHandler()->ShiftPressed())
	{
		oldsel = true;
		m_dwEndDrag = ScreenToSample(point.x);
		SetCurSel(m_dwBeginDrag, m_dwEndDrag);
	} else
	{
		m_dwBeginDrag = ScreenToSample(point.x);
		if (m_dwBeginDrag >= len) m_dwBeginDrag = len-1;
		m_dwEndDrag = m_dwBeginDrag;
	}
	if (oldsel) SetCurSel(m_dwBeginDrag, m_dwEndDrag);
	// set initial point for sample drawing
	if (m_bDrawingEnabled)
	{
		m_lastDrawPoint = point;
		pModDoc->GetSampleUndo().PrepareUndo(m_nSample, sundo_replace);
		if(sample.GetElementarySampleSize() == 2)
			SetInitialDrawPoint<int16, uint16>(sample.pSample, point);
		else if(sample.GetElementarySampleSize() == 1)
			SetInitialDrawPoint<int8, uint8>(sample.pSample, point);

		InvalidateSample();
		pModDoc->SetModified();
	} else
	{
		// ctrl + click = play from cursor pos
		if(CMainFrame::GetInputHandler()->CtrlPressed())
			PlayNote(NOTE_MIDDLEC, ScreenToSample(point.x));
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
	m_lastDrawPoint.SetPoint(-1, -1);
}


void CViewSample::OnLButtonDblClk(UINT, CPoint)
//---------------------------------------------
{
	CModDoc *pModDoc = GetDocument();

	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		DWORD len = pSndFile->GetSample(m_nSample).nLength;
		if (len && !m_bDrawingEnabled) SetCurSel(0, len);
	}
}


void CViewSample::OnRButtonDown(UINT, CPoint pt)
//----------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		const ModSample &sample = pSndFile->GetSample(m_nSample);
		HMENU hMenu = ::CreatePopupMenu();
		CInputHandler* ih = (CMainFrame::GetMainFrame())->GetInputHandler(); //rewbs.customKeys
		if (!hMenu)	return;
		if (sample.nLength)
		{
			if (m_dwEndSel >= m_dwBeginSel + 4)
			{
				::AppendMenu(hMenu, MF_STRING | (CanZoomSelection() ? 0 : MF_GRAYED), ID_SAMPLE_ZOOMONSEL, "Zoom\t" + ih->GetKeyTextFromCommand(kcSampleZoomSelection));
				::AppendMenu(hMenu, MF_STRING, ID_SAMPLE_SETLOOP, "Set As Loop");
				if (pSndFile->GetType() & (MOD_TYPE_IT|MOD_TYPE_MPT))
					::AppendMenu(hMenu, MF_STRING, ID_SAMPLE_SETSUSTAINLOOP, "Set As Sustain Loop");
				::AppendMenu(hMenu, MF_SEPARATOR, 0, "");
			} else
			{
				CHAR s[256];
				DWORD dwPos = ScreenToSample(pt.x);
				if (dwPos <= sample.nLength)
				{
					//Set loop points
					wsprintf(s, "Set Loop Start to:\t%d", dwPos);
					::AppendMenu(hMenu, MF_STRING | (dwPos + 4 <= sample.nLoopEnd ? 0 : MF_GRAYED),
						ID_SAMPLE_SETLOOPSTART, s);
					wsprintf(s, "Set Loop End to:\t%d", dwPos);
					::AppendMenu(hMenu, MF_STRING | (dwPos >= sample.nLoopStart + 4 ? 0 : MF_GRAYED),
						ID_SAMPLE_SETLOOPEND, s);

					if (pSndFile->GetType() & (MOD_TYPE_IT|MOD_TYPE_MPT))
					{
						//Set sustain loop points
						::AppendMenu(hMenu, MF_SEPARATOR, 0, "");
						wsprintf(s, "Set Sustain Start to:\t%d", dwPos);
						::AppendMenu(hMenu, MF_STRING | (dwPos + 4 <= sample.nSustainEnd ? 0 : MF_GRAYED),
							ID_SAMPLE_SETSUSTAINSTART, s);
						wsprintf(s, "Set Sustain End to:\t%d", dwPos);
						::AppendMenu(hMenu, MF_STRING | (dwPos >= sample.nSustainStart + 4 ? 0 : MF_GRAYED),
							ID_SAMPLE_SETSUSTAINEND, s);
					}
					::AppendMenu(hMenu, MF_SEPARATOR, 0, "");
					m_dwMenuParam = dwPos;
				}
			}

			if(sample.GetElementarySampleSize() > 1) ::AppendMenu(hMenu, MF_STRING, ID_SAMPLE_8BITCONVERT, "Convert to &8-bit\t" + ih->GetKeyTextFromCommand(kcSample8Bit));
			if(sample.GetNumChannels() > 1)
			{
				HMENU hMonoMenu = ::CreatePopupMenu();
				::AppendMenu(hMonoMenu, MF_STRING, ID_SAMPLE_MONOCONVERT, "&Mix Channels\t" + ih->GetKeyTextFromCommand(kcSampleMonoMix));
				::AppendMenu(hMonoMenu, MF_STRING, ID_SAMPLE_MONOCONVERT_LEFT, "&Left Channel\t" + ih->GetKeyTextFromCommand(kcSampleMonoLeft));
				::AppendMenu(hMonoMenu, MF_STRING, ID_SAMPLE_MONOCONVERT_RIGHT, "&Right Channel\t" + ih->GetKeyTextFromCommand(kcSampleMonoRight));
				::AppendMenu(hMonoMenu, MF_STRING, ID_SAMPLE_MONOCONVERT_SPLIT, "&Split Sample\t" + ih->GetKeyTextFromCommand(kcSampleMonoSplit));
				::AppendMenu(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hMonoMenu), "Convert to &Mono");
			}

			// "Trim" menu item is responding differently if there's no selection,
			// but a loop present: "trim around loop point"! (jojo in topic 2258)
			std::string sTrimMenuText = "T&rim";
			bool bIsGrayed = ( (m_dwEndSel<=m_dwBeginSel) || (m_dwEndSel - m_dwBeginSel < MIN_TRIM_LENGTH)
								|| (m_dwEndSel - m_dwBeginSel == sample.nLength)
							  );

			if ((m_dwBeginSel == m_dwEndSel) && (sample.nLoopStart < sample.nLoopEnd))
			{
				// no selection => use loop points
				sTrimMenuText += " around loop points";
				// Check whether trim menu item can be enabled (loop not too short or long for trimming).
				if( (sample.nLoopEnd <= sample.nLength) &&
					(sample.nLoopEnd - sample.nLoopStart >= MIN_TRIM_LENGTH) &&
					(sample.nLoopEnd - sample.nLoopStart < sample.nLength) )
					bIsGrayed = false;
			}

			sTrimMenuText += "\t" + ih->GetKeyTextFromCommand(kcSampleTrim);

			::AppendMenu(hMenu, MF_STRING|(bIsGrayed) ? MF_GRAYED : 0, ID_SAMPLE_TRIM, sTrimMenuText.c_str());
			if((m_dwBeginSel == 0 && m_dwEndSel != 0) || (m_dwBeginSel < sample.nLength && m_dwEndSel == sample.nLength))
			{
				::AppendMenu(hMenu, MF_STRING, ID_SAMPLE_QUICKFADE, "Quick &fade\t" + ih->GetKeyTextFromCommand(kcSampleQuickFade));
			}
			::AppendMenu(hMenu, MF_STRING, ID_EDIT_CUT, "Cu&t\t" + ih->GetKeyTextFromCommand(kcEditCut));
			::AppendMenu(hMenu, MF_STRING, ID_EDIT_COPY, "&Copy\t" + ih->GetKeyTextFromCommand(kcEditCopy));
		}
		::AppendMenu(hMenu, MF_STRING | (IsClipboardFormatAvailable(CF_WAVE) ? 0 : MF_GRAYED), ID_EDIT_PASTE, "&Paste\t" + ih->GetKeyTextFromCommand(kcEditPaste));
		::AppendMenu(hMenu, MF_STRING | (pModDoc->GetSampleUndo().CanUndo(m_nSample) ? 0 : MF_GRAYED), ID_EDIT_UNDO, "&Undo\t" + ih->GetKeyTextFromCommand(kcEditUndo));
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
		ModSample &sample = pSndFile->GetSample(m_nSample);
		if ((m_dwEndSel > m_dwBeginSel + 15) && (m_dwEndSel <= sample.nLength))
		{
			if ((sample.nLoopStart != m_dwBeginSel) || (sample.nLoopEnd != m_dwEndSel))
			{
				sample.nLoopStart = m_dwBeginSel;
				sample.nLoopEnd = m_dwEndSel;
				sample.uFlags |= CHN_LOOP;
				pModDoc->SetModified();
				pModDoc->AdjustEndOfSample(m_nSample);
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
		ModSample &sample = pSndFile->GetSample(m_nSample);
		if ((m_dwEndSel > m_dwBeginSel + 15) && (m_dwEndSel <= sample.nLength))
		{
			if ((sample.nSustainStart != m_dwBeginSel) || (sample.nSustainEnd != m_dwEndSel))
			{
				sample.nSustainStart = m_dwBeginSel;
				sample.nSustainEnd = m_dwEndSel;
				sample.uFlags |= CHN_SUSTAINLOOP;
				pModDoc->SetModified();
				pModDoc->AdjustEndOfSample(m_nSample);
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
		DWORD len = pSndFile->GetSample(m_nSample).nLength;
		if (len) SetCurSel(0, len);
	}
}


// Update loop points after deleting a sample selection
void CViewSample::AdjustLoopPoints(UINT &loopStart, UINT &loopEnd, UINT length) const
//-----------------------------------------------------------------------------------
{
	if(m_dwBeginSel < loopStart  && m_dwEndSel < loopStart)
	{
		// cut part is before loop start
		loopStart -= m_dwEndSel - m_dwBeginSel;
		loopEnd -= m_dwEndSel - m_dwBeginSel;
	}
	else if(m_dwBeginSel < loopStart  && m_dwEndSel < loopEnd)
	{
		// cut part is partly before loop start
		loopStart = m_dwBeginSel;
		loopEnd -= m_dwEndSel - m_dwBeginSel;
	}
	else if(m_dwBeginSel >= loopStart && m_dwEndSel < loopEnd)
	{
		// cut part is in the loop
		loopEnd -= m_dwEndSel - m_dwBeginSel;
	}
	else if(m_dwBeginSel >= loopStart && m_dwBeginSel < loopEnd && m_dwEndSel > loopEnd)
	{
		// cut part is partly before loop end
		loopEnd = m_dwBeginSel;
	}

	if(loopEnd > length) loopEnd = length;
	if(loopStart + 4 >= loopEnd)
	{
		loopStart = loopEnd = 0;
	}
}


void CViewSample::OnEditDelete()
//------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;
	DWORD dwUpdateFlags = HINT_SAMPLEINFO | HINT_SAMPLEDATA;
	DWORD len;

	if (!pModDoc) return;
	pSndFile = pModDoc->GetSoundFile();
	ModSample &sample = pSndFile->GetSample(m_nSample);
	len = sample.nLength;
	if ((!sample.pSample) || (!len)) return;
	if (m_dwEndSel > len) m_dwEndSel = len;
	if ((m_dwBeginSel >= m_dwEndSel)
	 || (m_dwEndSel - m_dwBeginSel + 4 >= len))
	{
		if (Reporting::Confirm("Remove this sample?", "Remove Sample", true) != cnfYes) return;
		pModDoc->GetSampleUndo().PrepareUndo(m_nSample, sundo_replace);

		pSndFile->DestroySampleThreadsafe(m_nSample);

		dwUpdateFlags |= HINT_SMPNAMES;
	} else
	{
		pModDoc->GetSampleUndo().PrepareUndo(m_nSample, sundo_delete, m_dwBeginSel, m_dwEndSel);

		CriticalSection cs;

		UINT cutlen = (m_dwEndSel - m_dwBeginSel);
		UINT istart = m_dwBeginSel * sample.GetBytesPerSample();
		UINT iend = len * sample.GetBytesPerSample();
		sample.nLength -= cutlen;
		cutlen *= sample.GetBytesPerSample();

		char *p = static_cast<char *>(sample.pSample);
		for (UINT i=istart; i<iend; i++)
		{
			p[i] = (i+cutlen < iend) ? p[i+cutlen] : (char)0;
		}
		len = sample.nLength;


		// adjust loop points
		AdjustLoopPoints(sample.nLoopStart, sample.nLoopEnd, len);
		AdjustLoopPoints(sample.nSustainStart, sample.nSustainEnd, len);

		if(sample.nLoopEnd == 0)
		{
			sample.uFlags &= ~(CHN_LOOP | CHN_PINGPONGLOOP);
		}

		if(sample.nSustainEnd == 0)
		{
			sample.uFlags &= ~(CHN_SUSTAINLOOP | CHN_PINGPONGSUSTAIN);
		}
	}
	SetCurSel(0, 0);
	pModDoc->AdjustEndOfSample(m_nSample);
	pModDoc->SetModified();
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
	DWORD dwMemSize, dwSmpLen, dwSmpOffset;
	HGLOBAL hCpy;
	BOOL bExtra = TRUE;

	if ((!pMainFrm) || (!pModDoc)) return;
	pSndFile = pModDoc->GetSoundFile();
	const ModSample &sample = pSndFile->GetSample(m_nSample);
	if ((!sample.nLength) || (!sample.pSample)) return;
	dwMemSize = sample.nLength;
	dwSmpOffset = 0;
	if (m_dwEndSel > sample.nLength) m_dwEndSel = sample.nLength;
	if (m_dwEndSel > m_dwBeginSel) { dwMemSize = m_dwEndSel - m_dwBeginSel; dwSmpOffset = m_dwBeginSel; bExtra = FALSE; }
	if (sample.uFlags & CHN_16BIT) { dwMemSize <<= 1; dwSmpOffset <<= 1; }
	if (sample.uFlags & CHN_STEREO) { dwMemSize <<= 1; dwSmpOffset <<= 1; }
	dwSmpLen = dwMemSize;
	dwMemSize += sizeof(WAVEFILEHEADER) + sizeof(WAVEFORMATHEADER) + sizeof(WAVEDATAHEADER)
			 + sizeof(WAVEEXTRAHEADER) + sizeof(WAVESAMPLERINFO);
	// For name + fname
	dwMemSize += 32 * 2;
	BeginWaitCursor();
	if ((pMainFrm->OpenClipboard()) && ((hCpy = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, dwMemSize))!=NULL))
	{
		EmptyClipboard();
		LPBYTE p = (LPBYTE)GlobalLock(hCpy);
		WAVEFILEHEADER *phdr = (WAVEFILEHEADER *)p;
		WAVEFORMATHEADER *pfmt = (WAVEFORMATHEADER *)(p + sizeof(WAVEFILEHEADER));
		WAVEDATAHEADER *pdata = (WAVEDATAHEADER *)(p + sizeof(WAVEFILEHEADER) + sizeof(WAVEFORMATHEADER));
		phdr->id_RIFF = IFFID_RIFF;
		phdr->filesize = sizeof(WAVEFILEHEADER) + sizeof(WAVEFORMATHEADER) + sizeof(WAVEDATAHEADER) - 8;
		phdr->id_WAVE = IFFID_WAVE;
		pfmt->id_fmt = IFFID_fmt;
		pfmt->hdrlen = 16;
		pfmt->format = 1;
		pfmt->freqHz = sample.nC5Speed;
		if (pSndFile->m_nType & (MOD_TYPE_MOD|MOD_TYPE_XM))
		{
			pfmt->freqHz = ModSample::TransposeToFrequency(sample.RelativeTone, sample.nFineTune);
	}
		pfmt->channels = (sample.uFlags & CHN_STEREO) ? (WORD)2 : (WORD)1;
		pfmt->bitspersample = (sample.uFlags & CHN_16BIT) ? (WORD)16 : (WORD)8;
		pfmt->samplesize = pfmt->channels * pfmt->bitspersample / 8;
		pfmt->bytessec = pfmt->freqHz*pfmt->samplesize;
		pdata->id_data = IFFID_data;
		pdata->length = dwSmpLen;
		phdr->filesize += pdata->length;
		LPBYTE psamples = p + sizeof(WAVEFILEHEADER) + sizeof(WAVEFORMATHEADER) + sizeof(WAVEDATAHEADER);
		memcpy(psamples, static_cast<const char *>(sample.pSample) + dwSmpOffset, dwSmpLen);
		if (pfmt->bitspersample == 8)
	{
			for (UINT i = 0; i < dwSmpLen; i++) psamples[i] += 0x80;
	}
		if (bExtra)
		{
			WAVESMPLHEADER *psh = (WAVESMPLHEADER *)(psamples+dwSmpLen);
			MemsetZero(*psh);
			psh->smpl_id = 0x6C706D73;
			psh->smpl_len = sizeof(WAVESMPLHEADER) - 8;
			psh->dwSamplePeriod = 22675;
			if (sample.nC5Speed > 256) psh->dwSamplePeriod = 1000000000 / sample.nC5Speed;
			psh->dwBaseNote = 60;

			// Write loops
			WAVESAMPLERINFO *psmpl = (WAVESAMPLERINFO *)psh;
			MemsetZero(psmpl->wsiLoops);
			if((sample.uFlags & CHN_SUSTAINLOOP) != 0)
	{
				psmpl->wsiLoops[psmpl->wsiHdr.dwSampleLoops++].SetLoop(sample.nSustainStart, sample.nSustainEnd, (sample.uFlags & CHN_PINGPONGSUSTAIN) != 0);
				psmpl->wsiHdr.smpl_len += sizeof(SAMPLELOOPSTRUCT);
			}
			if((sample.uFlags & CHN_LOOP) != 0)
			{
				psmpl->wsiLoops[psmpl->wsiHdr.dwSampleLoops++].SetLoop(sample.nLoopStart, sample.nLoopEnd, (sample.uFlags & CHN_PINGPONGLOOP) != 0);
				psmpl->wsiHdr.smpl_len += sizeof(SAMPLELOOPSTRUCT);
			}

			WAVEEXTRAHEADER *pxh = (WAVEEXTRAHEADER *)(psamples+dwSmpLen+psh->smpl_len+8);
			pxh->xtra_id = IFFID_xtra;
			pxh->xtra_len = sizeof(WAVEEXTRAHEADER)-8;

			pxh->dwFlags = sample.uFlags;
			pxh->wPan = sample.nPan;
			pxh->wVolume = sample.nVolume;
			pxh->wGlobalVol = sample.nGlobalVol;

			pxh->nVibType = sample.nVibType;
			pxh->nVibSweep = sample.nVibSweep;
			pxh->nVibDepth = sample.nVibDepth;
			pxh->nVibRate = sample.nVibRate;
			if((pSndFile->GetType() & MOD_TYPE_XM) && (pxh->nVibDepth | pxh->nVibRate))
			{
				// XM vibrato is upside down
				pxh->nVibSweep = 255 - pxh->nVibSweep;
			}
		
			if ((pSndFile->m_szNames[m_nSample][0]) || (sample.filename[0]))
		{
				LPSTR pszText = (LPSTR)(pxh+1);
				memcpy(pszText, pSndFile->m_szNames[m_nSample], MAX_SAMPLENAME);
				pxh->xtra_len += MAX_SAMPLENAME;
				if (sample.filename[0])
			{
					memcpy(pszText + MAX_SAMPLENAME, sample.filename, MAX_SAMPLEFILENAME);
					pxh->xtra_len += MAX_SAMPLEFILENAME;
			}
		}
			phdr->filesize += (psh->smpl_len + 8) + (pxh->xtra_len + 8);
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
			pModDoc->GetSampleUndo().PrepareUndo(m_nSample, sundo_replace);

			CSoundFile *pSndFile = pModDoc->GetSoundFile();
			DWORD dwMemSize = GlobalSize(hCpy);

			ModSample &sample = pSndFile->GetSample(m_nSample);

			memcpy(s, pSndFile->m_szNames[m_nSample], 32);
			memcpy(s2, sample.filename, 22);
			pSndFile->ReadSampleFromFile(m_nSample, p, dwMemSize);
			if (!pSndFile->m_szNames[m_nSample][0])
{
				memcpy(pSndFile->m_szNames[m_nSample], s, 32);
}
			if (!sample.filename[0])
			{
				memcpy(sample.filename, s2, 22);
			}

			GlobalUnlock(hCpy);
			SetCurSel(0, 0);
			pModDoc->AdjustEndOfSample(m_nSample);
			pModDoc->SetModified();
			pModDoc->UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEINFO | HINT_SAMPLEDATA | HINT_SMPNAMES, NULL);
		}
		CloseClipboard();
	}
	EndWaitCursor();
}


void CViewSample::OnEditUndo()
//----------------------------
{
	CModDoc *pModDoc = GetDocument();
	if(pModDoc == nullptr) return;
	if(pModDoc->GetSampleUndo().Undo(m_nSample) == true)
		pModDoc->UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEINFO | HINT_SAMPLEDATA, NULL);
}


void CViewSample::On8BitConvert()
//-------------------------------
{
	CModDoc *pModDoc = GetDocument();
	BeginWaitCursor();
	if ((pModDoc) && (m_nSample <= pModDoc->GetNumSamples()))
	{
		CSoundFile &sndFile = pModDoc->GetrSoundFile();
		ModSample &sample = sndFile.GetSample(m_nSample);
		if(sample.uFlags[CHN_16BIT] && sample.pSample != nullptr && sample.nLength != 0)
		{
			pModDoc->GetSampleUndo().PrepareUndo(m_nSample, sundo_replace);

			CriticalSection cs;

			signed char *p = (signed char *)(sample.pSample);
			UINT len = (sample.nLength + 1) * sample.GetNumChannels();
			for (UINT i=0; i<=len; i++)
			{
				p[i] = (signed char) ((*((short int *)(p+i*2))) / 256);
			}
			sample.uFlags.reset(CHN_16BIT);
			for (UINT j=0; j<MAX_CHANNELS; j++) if (sndFile.Chn[j].pSample == sample.pSample)
			{
				sndFile.Chn[j].dwFlags.reset(CHN_16BIT);
			}

			cs.Leave();

			pModDoc->SetModified();
			pModDoc->AdjustEndOfSample(m_nSample);
			pModDoc->UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEDATA | HINT_SAMPLEINFO, NULL);
		}
	}
	EndWaitCursor();
}


void CViewSample::OnMonoConvert(ctrlSmp::StereoToMonoMode convert)
//----------------------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	BeginWaitCursor();
	if(pModDoc != nullptr && (m_nSample <= pModDoc->GetNumSamples()))
	{
		CSoundFile &sndFile = pModDoc->GetrSoundFile();
		ModSample &sample = sndFile.GetSample(m_nSample);
		if(sample.GetNumChannels() > 1 && sample.pSample != nullptr && sample.nLength != 0)
		{
			SAMPLEINDEX rightSmp = SAMPLEINDEX_INVALID;
			if(convert == ctrlSmp::splitSample)
			{
				// Split sample into two slots
				rightSmp = pModDoc->InsertSample(true);
				if(rightSmp != SAMPLEINDEX_INVALID)
				{
					sndFile.ReadSampleFromSong(rightSmp, sndFile, m_nSample);
				} else
				{
					return;
				}
			}

			pModDoc->GetSampleUndo().PrepareUndo(m_nSample, sundo_replace);

			if(ctrlSmp::ConvertToMono(sample, sndFile, convert))
			{
				if(convert == ctrlSmp::splitSample)
				{
					// Split mode: We need to convert the right channel as well!
					ModSample &right = sndFile.GetSample(rightSmp);
					ctrlSmp::ConvertToMono(right, sndFile, ctrlSmp::onlyRight);

					// Try to create a new instrument as well which maps to the right sample.
					INSTRUMENTINDEX ins = pModDoc->FindSampleParent(m_nSample);
					if(ins != INSTRUMENTINDEX_INVALID)
					{
						INSTRUMENTINDEX rightIns = pModDoc->InsertInstrument(0, ins);
						if(rightIns != INSTRUMENTINDEX_INVALID)
						{
							for(size_t i = 0; i < CountOf(sndFile.Instruments[rightIns]->Keyboard); i++)
							{
								if(sndFile.Instruments[rightIns]->Keyboard[i] == m_nSample)
								{
									sndFile.Instruments[rightIns]->Keyboard[i] = rightSmp;
								}
							}
						}
					}

					// Finally, adjust sample panning
					if(sndFile.GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT | MOD_TYPE_XM))
					{
						sample.uFlags.set(CHN_PANNING);
						sample.nPan = 0;
						right.uFlags.set(CHN_PANNING);
						right.nPan = 256;
					}
				}
				pModDoc->SetModified();
				pModDoc->UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEDATA | HINT_SAMPLEINFO | HINT_SMPNAMES | HINT_INSNAMES, NULL);
			} else
			{
				pModDoc->GetSampleUndo().RemoveLastUndoStep(m_nSample);
			}
		}
	}
	EndWaitCursor();
}


void CViewSample::OnSampleTrim()
//------------------------------
{
	CModDoc *pModDoc = GetDocument();
	//nothing loaded or invalid sample slot.
	if(!pModDoc || m_nSample > pModDoc->GetNumSamples()) return;

	CSoundFile &sndFile = pModDoc->GetrSoundFile();
	ModSample &sample = sndFile.GetSample(m_nSample);

	if(m_dwBeginSel == m_dwEndSel)
	{
		// Trim around loop points if there's no selection
		m_dwBeginSel = sample.nLoopStart;
		m_dwEndSel = sample.nLoopEnd;
	}

	if (m_dwBeginSel >= m_dwEndSel) return; // invalid selection

	BeginWaitCursor();
	UINT nStart = m_dwBeginSel;
	UINT nEnd = m_dwEndSel - m_dwBeginSel;

	if ((sample.pSample) && (nStart+nEnd <= sample.nLength) && (nEnd >= MIN_TRIM_LENGTH))
	{
		pModDoc->GetSampleUndo().PrepareUndo(m_nSample, sundo_replace);

		CriticalSection cs;


		// Note: Sample is overwritten in-place! Unused data is not deallocated!
		const UINT bend = nEnd * sample.GetBytesPerSample() , bstart = nStart * sample.GetBytesPerSample();
		signed char *p = (signed char *)sample.pSample;
		for (UINT i = 0; i < bend; i++)
		{
			p[i] = p[i + bstart];
		}

		if (sample.nLoopStart >= nStart) sample.nLoopStart -= nStart;
		if (sample.nLoopEnd >= nStart) sample.nLoopEnd -= nStart;
		if (sample.nSustainStart >= nStart) sample.nSustainStart -= nStart;
		if (sample.nSustainEnd >= nStart) sample.nSustainEnd -= nStart;
		if (sample.nLoopEnd > nEnd) sample.nLoopEnd = nEnd;
		if (sample.nSustainEnd > nEnd) sample.nSustainEnd = nEnd;
		if (sample.nLoopStart >= sample.nLoopEnd)
		{
			sample.nLoopStart = sample.nLoopEnd = 0;
			sample.uFlags.reset(CHN_LOOP|CHN_PINGPONGLOOP);
		}
		if (sample.nSustainStart >= sample.nSustainEnd)
		{
			sample.nSustainStart = sample.nSustainEnd = 0;
			sample.uFlags.reset(CHN_SUSTAINLOOP|CHN_PINGPONGSUSTAIN);
		}
		sample.nLength = nEnd;

		cs.Leave();

		pModDoc->SetModified();
		pModDoc->AdjustEndOfSample(m_nSample);
		SetCurSel(0, 0);
		pModDoc->UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEDATA | HINT_SAMPLEINFO, NULL);
	}
	EndWaitCursor();
}


void CViewSample::OnChar(UINT /*nChar*/, UINT, UINT /*nFlags*/)
//-------------------------------------------------------------
{
}


void CViewSample::PlayNote(UINT note, const uint32 nStartPos)
//-----------------------------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = GetDocument();
	if ((pModDoc) && (pMainFrm))
	{
		if (note >= NOTE_MIN_SPECIAL)
		{
			pModDoc->NoteOff(0, (note == NOTE_NOTECUT));
		} else if(ModCommand::IsNote((ModCommand::NOTE)note))
		{
			if (m_dwStatus & SMPSTATUS_KEYDOWN)
				pModDoc->NoteOff(note, true);
			else
				pModDoc->NoteOff(0, true);

			SmpLength loopstart = m_dwBeginSel, loopend = m_dwEndSel;
			if (loopend - loopstart < (SmpLength)(4 << m_nZoom))
				loopend = loopstart = 0; // selection is too small -> no loop

			pModDoc->PlayNote(note, 0, m_nSample, false, -1, loopstart, loopend, CHANNELINDEX_INVALID, nStartPos);

			m_dwStatus |= SMPSTATUS_KEYDOWN;

			CSoundFile &sndFile = pModDoc->GetrSoundFile();
			ModSample &sample = sndFile.GetSample(m_nSample);
			uint32 freq = sndFile.GetFreqFromPeriod(sndFile.GetPeriodFromNote(note + (sndFile.GetType() == MOD_TYPE_XM ? sample.RelativeTone : 0), sample.nFineTune, sample.nC5Speed), sample.nC5Speed, 0);

			CHAR s[32];
			wsprintf(s, "%s (%d.%d Hz)", szDefaultNoteNames[note - 1], freq >> FREQ_FRACBITS, Util::muldiv(freq & ((1 << FREQ_FRACBITS) - 1), 100, 1 << FREQ_FRACBITS));
			pMainFrm->SetInfoText(s);
		}
	}

}


void CViewSample::OnDropFiles(HDROP hDropInfo)
//--------------------------------------------
{
	const UINT nFiles = ::DragQueryFile(hDropInfo, (UINT)-1, NULL, 0);
	for(UINT f = 0; f < nFiles; f++)
	{
		TCHAR szFileName[_MAX_PATH] = "";
		CMainFrame::GetMainFrame()->SetForegroundWindow();
		::DragQueryFile(hDropInfo, f, szFileName, _MAX_PATH);
		if(szFileName[0])
		{
			if(SendCtrlMessage(CTRLMSG_SMP_OPENFILE, (LPARAM)szFileName) && f < nFiles - 1)
			{
				// Insert more sample slots
				SendCtrlMessage(IDC_SAMPLE_NEW);
			}
		}
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
		bCanDrop = ((lpDropInfo->dwDropItem < CTrackApp::gpDLSBanks.size())
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
				DLSINSTRUMENT *pDlsIns;
				UINT nIns = 0, nRgn = 0xFF;
				// Drums
				if (lpDropInfo->dwDropItem & 0x80)
				{
					UINT key = lpDropInfo->dwDropItem & 0x7F;
					pDlsIns = dlsbank.FindInstrument(TRUE, 0xFFFF, 0xFF, key, &nIns);
					if (pDlsIns) nRgn = dlsbank.GetRegionFromKey(nIns, key);
				} else
				// Melodic
				{
					pDlsIns = dlsbank.FindInstrument(FALSE, 0xFFFF, lpDropInfo->dwDropItem, 60, &nIns);
					if (pDlsIns) nRgn = dlsbank.GetRegionFromKey(nIns, 60);
				}
				bCanDrop = FALSE;
				if (pDlsIns)
				{
					CriticalSection cs;

					pModDoc->GetSampleUndo().PrepareUndo(m_nSample, sundo_replace);
					bCanDrop = dlsbank.ExtractSample(*pSndFile, m_nSample, nIns, nRgn);
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
			CriticalSection cs;

			pModDoc->GetSampleUndo().PrepareUndo(m_nSample, sundo_replace);
			bCanDrop = pDLSBank->ExtractSample(*pSndFile, m_nSample, nIns, nRgn);

			bUpdate = TRUE;
		}
		break;
	}
	if (bUpdate)
	{
		pModDoc->AdjustEndOfSample(m_nSample);
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


UINT CViewSample::GetSelectionZoomLevel() const
//---------------------------------------------
{
	UINT zoom = 0;
	while(((static_cast<UINT>(m_rcClient.right) << zoom) < m_dwEndSel - m_dwBeginSel) && (zoom < MAX_ZOOM - 1))
	{
		zoom++;
	}
	if(zoom++ < MAX_ZOOM)
		return zoom;
	else
		return 0;
}


void CViewSample::OnZoomOnSel()
//-----------------------------
{
	if ((m_dwEndSel > m_dwBeginSel) && (m_rcClient.right > 0))
	{
		DWORD dwZoom = GetSelectionZoomLevel();

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
		ModSample &sample = pSndFile->GetSample(m_nSample);
		if ((m_dwMenuParam+4 <= sample.nLoopEnd) && (sample.nLoopStart != m_dwMenuParam))
		{
			pModDoc->GetSampleUndo().PrepareUndo(m_nSample, sundo_none);
			sample.nLoopStart = m_dwMenuParam;
			sample.uFlags |= CHN_LOOP;
			pModDoc->SetModified();
			ctrlSmp::UpdateLoopPoints(sample, *pSndFile);
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
		ModSample &sample = pSndFile->GetSample(m_nSample);
		if ((m_dwMenuParam >= sample.nLoopStart+4) && (sample.nLoopEnd != m_dwMenuParam))
		{
			pModDoc->GetSampleUndo().PrepareUndo(m_nSample, sundo_none);
			sample.nLoopEnd = m_dwMenuParam;
			sample.uFlags |= CHN_LOOP;
			pModDoc->SetModified();
			ctrlSmp::UpdateLoopPoints(sample, *pSndFile);
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
		ModSample &sample = pSndFile->GetSample(m_nSample);
		if ((m_dwMenuParam+4 <= sample.nSustainEnd) && (sample.nSustainStart != m_dwMenuParam))
		{
			pModDoc->GetSampleUndo().PrepareUndo(m_nSample, sundo_none);
			sample.nSustainStart = m_dwMenuParam;
			sample.uFlags |= CHN_SUSTAINLOOP;
			pModDoc->SetModified();
			ctrlSmp::UpdateLoopPoints(sample, *pSndFile);
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
		ModSample &sample = pSndFile->GetSample(m_nSample);
		if ((m_dwMenuParam >= sample.nSustainStart+4) && (sample.nSustainEnd != m_dwMenuParam))
		{
			pModDoc->GetSampleUndo().PrepareUndo(m_nSample, sundo_none);
			sample.nSustainEnd = m_dwMenuParam;
			sample.uFlags |= CHN_SUSTAINLOOP;
			pModDoc->SetModified();
			ctrlSmp::UpdateLoopPoints(sample, *pSndFile);
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


void CViewSample::OnDrawingToggle()
//---------------------------------
{
	m_bDrawingEnabled = !m_bDrawingEnabled;
	UpdateNcButtonState();
}


void CViewSample::OnAddSilence()
//------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (!pModDoc) return;
	CSoundFile &sndFile = pModDoc->GetrSoundFile();

	ModSample &sample = sndFile.GetSample(m_nSample);

	CAddSilenceDlg dlg(this, 32, sample.nLength);
	if (dlg.DoModal() != IDOK) return;

	const SmpLength nOldLength = sample.nLength;

	if(MAX_SAMPLE_LENGTH - nOldLength < dlg.m_nSamples)
	{
		CString str; str.Format(TEXT("Can't add silence because the new sample length would exceed maximum sample length %u."), MAX_SAMPLE_LENGTH);
		Reporting::Information(str);
		return;
	}

	BeginWaitCursor();

	if(dlg.m_nEditOption == addsilence_resize)
	{
		// resize - dlg.m_nSamples = new size
		if(dlg.m_nSamples != sample.nLength)
		{
			CriticalSection cs;

			if(dlg.m_nSamples < sample.nLength)	// make it shorter!
				pModDoc->GetSampleUndo().PrepareUndo(m_nSample, sundo_delete, dlg.m_nSamples, sample.nLength);
			else	// make it longer!
				pModDoc->GetSampleUndo().PrepareUndo(m_nSample, sundo_insert, sample.nLength, dlg.m_nSamples);
			ctrlSmp::ResizeSample(sample, dlg.m_nSamples, sndFile);
		}
	} else
	{
		// add silence - dlg.m_nSamples = amount of bytes to be added
		if(dlg.m_nSamples > 0)
		{
			CriticalSection cs;

			UINT nStart = (dlg.m_nEditOption == addsilence_at_end) ? sample.nLength : 0;
			pModDoc->GetSampleUndo().PrepareUndo(m_nSample, sundo_insert, nStart, nStart + dlg.m_nSamples);
			ctrlSmp::InsertSilence(sample, dlg.m_nSamples, nStart, sndFile);
		}
	}

	EndWaitCursor();

	if(nOldLength != sample.nLength)
	{
		SetCurSel(0, 0);
		pModDoc->SetModified();
		pModDoc->UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEINFO | HINT_SAMPLEDATA | HINT_SMPNAMES, NULL);
	}
}

LRESULT CViewSample::OnMidiMsg(WPARAM dwMidiDataParam, LPARAM)
//------------------------------------------------------------
{
	const DWORD dwMidiData = dwMidiDataParam;
	static BYTE midivolume = 127;

	CModDoc *pModDoc = GetDocument();
	BYTE midibyte1 = MIDIEvents::GetDataByte1FromEvent(dwMidiData);
	BYTE midibyte2 = MIDIEvents::GetDataByte2FromEvent(dwMidiData);

	CSoundFile* pSndFile = (pModDoc) ? pModDoc->GetSoundFile() : NULL;
	if (!pSndFile) return 0;

	uint8 nNote = midibyte1 + NOTE_MIN;
	int nVol = midibyte2;
	MIDIEvents::EventType event  = MIDIEvents::GetTypeFromEvent(dwMidiData);
	if(event == MIDIEvents::evNoteOn && !nVol)
	{
		event = MIDIEvents::evNoteOff;	//Convert event to note-off if req'd
	}

	// Handle MIDI messages assigned to shortcuts
	CInputHandler *ih = CMainFrame::GetMainFrame()->GetInputHandler();
	if(ih->HandleMIDIMessage(kCtxViewSamples, dwMidiData) != kcNull
		|| ih->HandleMIDIMessage(kCtxAllContexts, dwMidiData) != kcNull)
	{
		// Mapped to a command, no need to pass message on.
		return 1;
	}

	switch(event)
	{
	case MIDIEvents::evNoteOff: // Note Off
		midibyte2 = 0;

	case MIDIEvents::evNoteOn: // Note On
		LimitMax(nNote, NOTE_MAX);
		pModDoc->NoteOff(nNote, true);
		if(midibyte2 & 0x7F)
		{
			nVol = CMainFrame::ApplyVolumeRelatedSettings(dwMidiData, midivolume);
			//pModDoc->PlayNote(nNote, 0, m_nSample, FALSE, nVol);
			PlayNote(nNote);
		}
		break;

	case MIDIEvents::evControllerChange: //Controller change
		switch(midibyte1)
		{
		case MIDIEvents::MIDICC_Volume_Coarse: //Volume
			midivolume = midibyte2;
			break;
		}
		break;
	}

	return 1;
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
//-------------------------------------------------------------------
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
		case kcSampleZoomSelection: OnZoomOnSel(); return wParam;
		case kcPrevInstrument:	OnPrevInstrument(); return wParam;
		case kcNextInstrument:	OnNextInstrument(); return wParam;
		case kcEditSelectAll:	OnEditSelectAll(); return wParam;
		case kcSampleDelete:	OnEditDelete(); return wParam;
		case kcEditCut:			OnEditCut(); return wParam;
		case kcEditCopy:		OnEditCopy(); return wParam;
		case kcEditPaste:		OnEditPaste(); return wParam;
		case kcEditUndo:		OnEditUndo(); return wParam;
		case kcSample8Bit:		On8BitConvert(); return wParam;
		case kcSampleMonoMix:	OnMonoConvertMix(); return wParam;
		case kcSampleMonoLeft:	OnMonoConvertLeft(); return wParam;
		case kcSampleMonoRight:	OnMonoConvertRight(); return wParam;
		case kcSampleMonoSplit:	OnMonoConvertSplit(); return wParam;

		case kcSampleLoad:		PostCtrlMessage(IDC_SAMPLE_OPEN); return wParam;
		case kcSampleSave:		PostCtrlMessage(IDC_SAMPLE_SAVEAS); return wParam;
		case kcSampleNew:		PostCtrlMessage(IDC_SAMPLE_NEW); return wParam;

		case kcSampleReverse:			PostCtrlMessage(IDC_SAMPLE_REVERSE); return wParam;
		case kcSampleSilence:			PostCtrlMessage(IDC_SAMPLE_SILENCE); return wParam;
		case kcSampleNormalize:			PostCtrlMessage(IDC_SAMPLE_NORMALIZE); return wParam;
		case kcSampleAmplify:			PostCtrlMessage(IDC_SAMPLE_AMPLIFY); return wParam;
		case kcSampleInvert:			PostCtrlMessage(IDC_SAMPLE_INVERT); return wParam;
		case kcSampleSignUnsign:		PostCtrlMessage(IDC_SAMPLE_SIGN_UNSIGN); return wParam;
		case kcSampleRemoveDCOffset:	PostCtrlMessage(IDC_SAMPLE_DCOFFSET); return wParam;
		case kcSampleXFade:				PostCtrlMessage(IDC_SAMPLE_XFADE); return wParam;
		case kcSampleAutotune:			PostCtrlMessage(IDC_SAMPLE_AUTOTUNE); return wParam;
		case kcSampleQuickFade:			PostCtrlMessage(IDC_SAMPLE_QUICKFADE); return wParam;

		// Those don't seem to work.
		case kcNoteOff:			PlayNote(NOTE_KEYOFF); return wParam;
		case kcNoteCut:			PlayNote(NOTE_NOTECUT); return wParam;

	}
	if (wParam >= kcSampStartNotes && wParam <= kcSampEndNotes)
	{
		PlayNote(wParam - kcSampStartNotes + 1 + pMainFrm->GetBaseOctave() * 12);
		return wParam;
	}
	if (wParam >= kcSampStartNoteStops && wParam <= kcSampEndNoteStops)
	{
		m_dwStatus &= ~SMPSTATUS_KEYDOWN;
		return wParam;
	}

	return NULL;
}


// Returns auto-zoom level compared to other zoom levels.
// If auto-zoom gives bigger zoom than zoom level N but smaller than zoom level N-1,
// return value is N. If zoom is bigger than the biggest zoom, returns MIN_ZOOM + 1 and
// if smaller than the smallest zoom, returns value >= MAX_ZOOM + 1.
UINT CViewSample::GetAutoZoomLevel(const ModSample& smp)
//------------------------------------------------------
{
	m_rcClient.NormalizeRect();
	if (m_rcClient.Width() == 0 || smp.nLength <= 0)
		return MAX_ZOOM + 1;

	// When m_nZoom > 0, 2^(m_nZoom - 1) = samplesPerPixel  [1]
	// With auto-zoom setting the whole sample is fitted to screen:
	// ViewScreenWidthInPixels * samplesPerPixel = sampleLength (approximately)  [2].
	// Solve samplesPerPixel from [2], then "m_nZoom" from [1].
	float zoom = static_cast<float>(smp.nLength) / static_cast<float>(m_rcClient.Width());
	zoom = 1 + (log10(zoom) / log10(2.0f));
	return static_cast<UINT>(std::max(zoom + 1, MIN_ZOOM + 1.0f));
}


BOOL CViewSample::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
//------------------------------------------------------------------
{
	// Ctrl + mouse wheel: zoom control.
	// One scroll direction zooms in and the other zooms out.
	// This behaviour is different from what would happen if simply scrolling
	// the zoom levels in the zoom combobox.
	if (nFlags == MK_CONTROL)
	{
		CSoundFile* const pSndFile = (GetDocument()) ? GetDocument()->GetSoundFile() : nullptr;
		if (pSndFile != nullptr)
		{
			// zoomOrder: Biggest to smallest zoom order.
			UINT zoomOrder[MAX_ZOOM + 1];
			for(size_t i = 1; i < CountOf(zoomOrder); ++i)
				zoomOrder[i-1] = i; // [0]=1, [1]=2, ...
			zoomOrder[CountOf(zoomOrder) - 1] = 0;
			UINT* const pZoomOrderEnd = zoomOrder + CountOf(zoomOrder);
			const UINT nAutoZoomLevel = GetAutoZoomLevel(pSndFile->GetSample(m_nSample));

			// If auto-zoom is not the smallest zoom, move auto-zoom index(=zero)
			// to the right position in the zoom order.
			if (nAutoZoomLevel < MAX_ZOOM + 1)
			{
				UINT* p = std::find(zoomOrder, pZoomOrderEnd, nAutoZoomLevel);
				if (p != pZoomOrderEnd)
				{
					memmove(p + 1, p, sizeof(zoomOrder[0]) * (pZoomOrderEnd - (p+1)));
					*p = 0;
				}
				else
					ASSERT(false);
			}
			const ptrdiff_t nPos = std::find(zoomOrder, pZoomOrderEnd, m_nZoom) - zoomOrder;
			if (zDelta > 0 && nPos > 0)
				SendCtrlMessage(CTRLMSG_SMP_SETZOOM, zoomOrder[nPos - 1]);
			else if (zDelta < 0 && nPos + 1 < CountOf(zoomOrder))
				SendCtrlMessage(CTRLMSG_SMP_SETZOOM, zoomOrder[nPos + 1]);
		}
	}

	return CModScrollView::OnMouseWheel(nFlags, zDelta, pt);
}


void CViewSample::OnChangeGridSize()
//----------------------------------
{
	CSampleGridDlg dlg(this, m_nGridSegments, GetDocument()->GetSoundFile()->GetSample(m_nSample).nLength);
	if(dlg.DoModal() == IDOK)
	{
		m_nGridSegments = dlg.m_nSegments;
		InvalidateSample();
	}
}
