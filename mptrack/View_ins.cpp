#include "stdafx.h"
#include "mptrack.h"
#include "mainfrm.h"
#include "childfrm.h"
#include "moddoc.h"
#include "globals.h"
#include "ctrl_ins.h"
#include "view_ins.h"
#include "dlsbank.h"
#include "channelManagerDlg.h"
#include "ScaleEnvPointsDlg.h"
#include ".\view_ins.h"
#include "midi.h"

#define ENV_ZOOM				4
#define ENV_DRAGLOOPSTART		0x100
#define ENV_DRAGLOOPEND			0x200
#define ENV_DRAGSUSTAINSTART	0x400
#define ENV_DRAGSUSTAINEND		0x800

// Non-client toolbar
#define ENV_LEFTBAR_CY			29
#define ENV_LEFTBAR_CXSEP		14
#define ENV_LEFTBAR_CXSPC		3
#define ENV_LEFTBAR_CXBTN		24
#define ENV_LEFTBAR_CYBTN		22


const UINT cLeftBarButtons[ENV_LEFTBAR_BUTTONS] = 
{
	ID_ENVSEL_VOLUME,
	ID_ENVSEL_PANNING,
	ID_ENVSEL_PITCH,
		ID_SEPARATOR,
	ID_ENVELOPE_VOLUME,
	ID_ENVELOPE_PANNING,
	ID_ENVELOPE_PITCH,
	ID_ENVELOPE_FILTER,
		ID_SEPARATOR,
	ID_ENVELOPE_SETLOOP,
	ID_ENVELOPE_SUSTAIN,
	ID_ENVELOPE_CARRY,
		ID_SEPARATOR,
	ID_INSTRUMENT_SAMPLEMAP,
		ID_SEPARATOR,
	ID_ENVELOPE_VIEWGRID,			//rewbs.envRowGrid
		ID_SEPARATOR,
};


IMPLEMENT_SERIAL(CViewInstrument, CModScrollView, 0)

BEGIN_MESSAGE_MAP(CViewInstrument, CModScrollView)
	//{{AFX_MSG_MAP(CViewInstrument)
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_NCCALCSIZE()
	ON_WM_NCPAINT()
	ON_WM_NCHITTEST()
	ON_WM_MOUSEMOVE()
	ON_WM_NCMOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_NCLBUTTONDOWN()
	ON_WM_NCLBUTTONUP()
	ON_WM_NCLBUTTONDBLCLK()
	ON_WM_CHAR()
	ON_WM_KEYUP()
	ON_WM_DROPFILES()
	ON_COMMAND(ID_PREVINSTRUMENT,			OnPrevInstrument)
	ON_COMMAND(ID_NEXTINSTRUMENT,			OnNextInstrument)
	ON_COMMAND(ID_ENVELOPE_SETLOOP,			OnEnvLoopChanged)
	ON_COMMAND(ID_ENVELOPE_SUSTAIN,			OnEnvSustainChanged)
	ON_COMMAND(ID_ENVELOPE_CARRY,			OnEnvCarryChanged)
	ON_COMMAND(ID_ENVELOPE_INSERTPOINT,		OnEnvInsertPoint)
	ON_COMMAND(ID_ENVELOPE_REMOVEPOINT,		OnEnvRemovePoint)
	ON_COMMAND(ID_ENVELOPE_VOLUME,			OnEnvVolChanged)
	ON_COMMAND(ID_ENVELOPE_PANNING,			OnEnvPanChanged)
	ON_COMMAND(ID_ENVELOPE_PITCH,			OnEnvPitchChanged)
	ON_COMMAND(ID_ENVELOPE_FILTER,			OnEnvFilterChanged)
	ON_COMMAND(ID_ENVELOPE_VIEWGRID,		OnEnvToggleGrid) //rewbs.envRowGrid
	ON_COMMAND(ID_ENVSEL_VOLUME,			OnSelectVolumeEnv)
	ON_COMMAND(ID_ENVSEL_PANNING,			OnSelectPanningEnv)
	ON_COMMAND(ID_ENVSEL_PITCH,				OnSelectPitchEnv)
	ON_COMMAND(ID_EDIT_COPY,				OnEditCopy)
	ON_COMMAND(ID_EDIT_PASTE,				OnEditPaste)
	ON_COMMAND(ID_INSTRUMENT_SAMPLEMAP,		OnEditSampleMap)
	ON_MESSAGE(WM_MOD_MIDIMSG,				OnMidiMsg)
	ON_MESSAGE(WM_MOD_KEYCOMMAND,	OnCustomKeyMsg) //rewbs.customKeys
	ON_COMMAND(ID_ENVELOPE_TOGGLERELEASENODE, OnEnvToggleReleasNode)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_ENVELOPE_SCALEPOINTS, OnEnvelopeScalepoints)
END_MESSAGE_MAP()


///////////////////////////////////////////////////////////////
// CViewInstrument operations

CViewInstrument::CViewInstrument()
//--------------------------------
{
	m_nInstrument = 1;
	m_nEnv = ENV_VOLUME;
	m_rcClient.bottom = 2;
	m_dwStatus = 0;
	m_nBtnMouseOver = 0xFFFF;
	memset(m_dwNotifyPos, 0, sizeof(m_dwNotifyPos));
	memset(m_NcButtonState, 0, sizeof(m_NcButtonState));
	m_bmpEnvBar.Create(IDB_ENVTOOLBAR, 20, 0, RGB(192,192,192));
	memset(m_baPlayingNote, 0, sizeof(bool)*NOTE_MAX);  //rewbs.customKeys
	m_nPlayingChannel =-1;						   //rewbs.customKeys
	//rewbs.envRowGrid
	m_bGrid=true;								  
	m_bGridForceRedraw=false;
	m_GridSpeed = -1;
	m_GridScrollPos = -1;
	//end rewbs.envRowGrid

}


void CViewInstrument::OnInitialUpdate()
//-------------------------------------
{
	ModifyStyleEx(0, WS_EX_ACCEPTFILES);
	CScrollView::OnInitialUpdate();
	UpdateScrollSize();
}


void CViewInstrument::UpdateScrollSize()
//--------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	GetClientRect(&m_rcClient);
	if (m_rcClient.bottom < 2) m_rcClient.bottom = 2;
	if (pModDoc)
	{
		SIZE sizeTotal, sizePage, sizeLine;
		UINT ntickmax = EnvGetTick(EnvGetLastPoint());
		
		sizeTotal.cx = (ntickmax + 2) * ENV_ZOOM;
		sizeTotal.cy = 1;
		sizeLine.cx = ENV_ZOOM;
		sizeLine.cy = 2;
		sizePage.cx = sizeLine.cx * 4;
		sizePage.cy = sizeLine.cy;
		SetScrollSizes(MM_TEXT, sizeTotal, sizePage, sizeLine);
	}
}


BOOL CViewInstrument::SetCurrentInstrument(UINT nIns, UINT nEnv)
//--------------------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	DWORD dwNotify;

	if ((!pModDoc) || (nIns < 1) || (nIns >= MAX_INSTRUMENTS)) return FALSE;
	if (nEnv) m_nEnv = nEnv;
	m_nInstrument = nIns;
	switch(m_nEnv)
	{
	case ENV_PANNING:	dwNotify = MPTNOTIFY_PANENV; break;
	case ENV_PITCH:		dwNotify = MPTNOTIFY_PITCHENV; break;
	default:			m_nEnv = ENV_VOLUME; dwNotify = MPTNOTIFY_VOLENV; break;
	}
	pModDoc->SetFollowWnd(m_hWnd, dwNotify|m_nInstrument);
	UpdateScrollSize();
	UpdateNcButtonState();
	InvalidateRect(NULL, FALSE);
	return TRUE;
}


LRESULT CViewInstrument::OnModViewMsg(WPARAM wParam, LPARAM lParam)
//-----------------------------------------------------------------
{
	switch(wParam)
	{
	case VIEWMSG_SETCURRENTINSTRUMENT:
		SetCurrentInstrument(lParam & 0xFFFF, m_nEnv);
		break;

	case VIEWMSG_LOADSTATE:
		if (lParam)
		{
			INSTRUMENTVIEWSTATE *pState = (INSTRUMENTVIEWSTATE *)lParam;
			if (pState->cbStruct == sizeof(INSTRUMENTVIEWSTATE))
			{
				SetCurrentInstrument(m_nInstrument, pState->nEnv);
				m_bGrid = pState->bGrid;
			}
		}
		break;

	case VIEWMSG_SAVESTATE:
		if (lParam)
		{
			INSTRUMENTVIEWSTATE *pState = (INSTRUMENTVIEWSTATE *)lParam;
			pState->cbStruct = sizeof(INSTRUMENTVIEWSTATE);
			pState->nEnv = m_nEnv;
			pState->bGrid = m_bGrid;
		}
		break;

	default:
		return CModScrollView::OnModViewMsg(wParam, lParam);
	}
	return 0;
}


UINT CViewInstrument::EnvGetTick(int nPoint) const
//------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];
		if ((pIns) && (nPoint >= 0))
		{
			switch(m_nEnv)
			{
			case ENV_VOLUME:
				if (nPoint < (int)pIns->nVolEnv) return pIns->VolPoints[nPoint];
				break;
			case ENV_PANNING:
				if (nPoint < (int)pIns->nPanEnv) return pIns->PanPoints[nPoint];
				break;
			case ENV_PITCH:
				if (nPoint < (int)pIns->nPitchEnv) return pIns->PitchPoints[nPoint];
				break;
			}
		}
	}
	return 0;
}


UINT CViewInstrument::EnvGetValue(int nPoint) const
//-------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];
		if ((pIns) && (nPoint >= 0))
		{
			switch(m_nEnv)
			{
			case ENV_VOLUME:
				if (nPoint < (int)pIns->nVolEnv) return pIns->VolEnv[nPoint];
				break;
			case ENV_PANNING:
				if (nPoint < (int)pIns->nPanEnv) return pIns->PanEnv[nPoint];
				break;
			case ENV_PITCH:
				if (nPoint < (int)pIns->nPitchEnv) return pIns->PitchEnv[nPoint];
				break;
			}
		}
	}
	return 0;
}


BOOL CViewInstrument::EnvSetValue(int nPoint, int nTick, int nValue)
//------------------------------------------------------------------
{
	BOOL bOk = FALSE;
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];
		if ((pIns) && (nPoint >= 0))
		{
			LPWORD pPoints = NULL;
			LPBYTE pData = NULL;
			UINT maxpoints = 0;
			switch(m_nEnv)
			{
			case ENV_VOLUME:
				maxpoints = pIns->nVolEnv;
				pPoints = pIns->VolPoints;
				pData = pIns->VolEnv;
				break;
			case ENV_PANNING:
				maxpoints = pIns->nPanEnv;
				pPoints = pIns->PanPoints;
				pData = pIns->PanEnv;
				break;
			case ENV_PITCH:
				maxpoints = pIns->nPitchEnv;
				pPoints = pIns->PitchPoints;
				pData = pIns->PitchEnv;
				break;
			}
			if (!nPoint) nTick = 0;
			if ((nPoint < (int)maxpoints) && (pPoints) && (pData))
			{
				if (nTick >= 0)
				{
					int mintick = (nPoint) ? pPoints[nPoint-1] : 0;
					int maxtick = pPoints[nPoint+1];
					if (nPoint+1 == (int)maxpoints) maxtick = 16383;
					if (nTick < mintick) nTick = mintick;
					if (nTick > maxtick) nTick = maxtick;
					if (nTick != pPoints[nPoint])
					{
						pPoints[nPoint] = (WORD)nTick;
						bOk = TRUE;
					}
				}
				if (nValue >= 0)
				{
					if (nValue > 64) nValue = 64;
					if (nValue != pData[nPoint])
					{
						pData[nPoint] = (BYTE)nValue;
						bOk = TRUE;
					}
				}
			}
		}
	}
	return bOk;
}


UINT CViewInstrument::EnvGetNumPoints() const
//-------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];
		if (pIns)
		{
			switch(m_nEnv)
			{
			case ENV_VOLUME:
				if (pIns->nVolEnv) return pIns->nVolEnv;
				break;
			case ENV_PANNING:
				if (pIns->nPanEnv) return pIns->nPanEnv;
				break;
			case ENV_PITCH:
				if (pIns->nPitchEnv) return pIns->nPitchEnv;
				break;
			}
		}
	}
	return 0;
}


UINT CViewInstrument::EnvGetLastPoint() const
//-------------------------------------------
{
	UINT nPoints = EnvGetNumPoints();
	if (nPoints > 0) return nPoints - 1;
	return 0;
}


BOOL CViewInstrument::EnvGetLoop() const
//--------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];
		if (pIns)
		{
			switch(m_nEnv)
			{
			case ENV_VOLUME:
				if (pIns->dwFlags & ENV_VOLLOOP) return TRUE;
				break;
			case ENV_PANNING:
				if (pIns->dwFlags & ENV_PANLOOP) return TRUE;
				break;
			case ENV_PITCH:
				if (pIns->dwFlags & ENV_PITCHLOOP) return TRUE;
				break;
			}
		}
	}
	return FALSE;
}


BOOL CViewInstrument::EnvGetSustain() const
//-----------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];
		if (pIns)
		{
			switch(m_nEnv)
			{
			case ENV_VOLUME:
				if (pIns->dwFlags & ENV_VOLSUSTAIN) return TRUE;
				break;
			case ENV_PANNING:
				if (pIns->dwFlags & ENV_PANSUSTAIN) return TRUE;
				break;
			case ENV_PITCH:
				if (pIns->dwFlags & ENV_PITCHSUSTAIN) return TRUE;
				break;
			}
		}
	}
	return FALSE;
}


BOOL CViewInstrument::EnvGetCarry() const
//---------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];
		if (pIns)
		{
			switch(m_nEnv)
			{
			case ENV_VOLUME:
				if (pIns->dwFlags & ENV_VOLCARRY) return TRUE;
				break;
			case ENV_PANNING:
				if (pIns->dwFlags & ENV_PANCARRY) return TRUE;
				break;
			case ENV_PITCH:
				if (pIns->dwFlags & ENV_PITCHCARRY) return TRUE;
				break;
			}
		}
	}
	return FALSE;
}


UINT CViewInstrument::EnvGetLoopStart() const
//-------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];
		if (pIns)
		{
			switch(m_nEnv)
			{
			case ENV_VOLUME:	return pIns->nVolLoopStart;
			case ENV_PANNING:	return pIns->nPanLoopStart;
			case ENV_PITCH:		return pIns->nPitchLoopStart;
			}
		}
	}
	return 0;
}


UINT CViewInstrument::EnvGetLoopEnd() const
//-----------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];
		if (pIns)
		{
			switch(m_nEnv)
			{
			case ENV_VOLUME:	return pIns->nVolLoopEnd;
			case ENV_PANNING:	return pIns->nPanLoopEnd;
			case ENV_PITCH:		return pIns->nPitchLoopEnd;
			}
		}
	}
	return 0;
}


UINT CViewInstrument::EnvGetSustainStart() const
//----------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];
		if (pIns)
		{
			switch(m_nEnv)
			{
			case ENV_VOLUME:	return pIns->nVolSustainBegin;
			case ENV_PANNING:	return pIns->nPanSustainBegin;
			case ENV_PITCH:		return pIns->nPitchSustainBegin;
			}
		}
	}
	return 0;
}


UINT CViewInstrument::EnvGetSustainEnd() const
//--------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];
		if (pIns)
		{
			switch(m_nEnv)
			{
			case ENV_VOLUME:	return pIns->nVolSustainEnd;
			case ENV_PANNING:	return pIns->nPanSustainEnd;
			case ENV_PITCH:		return pIns->nPitchSustainEnd;
			}
		}
	}
	return 0;
}


BOOL CViewInstrument::EnvGetVolEnv() const
//----------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];
		if (pIns) return (pIns->dwFlags & ENV_VOLUME) ? TRUE : FALSE;
	}
	return FALSE;
}


BOOL CViewInstrument::EnvGetPanEnv() const
//----------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];
		if (pIns) return (pIns->dwFlags & ENV_PANNING) ? TRUE : FALSE;
	}
	return FALSE;
}


BOOL CViewInstrument::EnvGetPitchEnv() const
//------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];
		if (pIns) return ((pIns->dwFlags & (ENV_PITCH|ENV_FILTER)) == ENV_PITCH) ? TRUE : FALSE;
	}
	return FALSE;
}


BOOL CViewInstrument::EnvGetFilterEnv() const
//-------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];
		if (pIns) return ((pIns->dwFlags & (ENV_PITCH|ENV_FILTER)) == (ENV_PITCH|ENV_FILTER)) ? TRUE : FALSE;
	}
	return FALSE;
}


BOOL CViewInstrument::EnvSetLoopStart(int nPoint)
//-----------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if ((pModDoc) && (nPoint >= 0) && (nPoint <= (int)EnvGetLastPoint()))
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];
		if (pIns)
		{
			switch(m_nEnv)
			{
			case ENV_VOLUME:
				if (nPoint != pIns->nVolLoopStart)
				{
					pIns->nVolLoopStart = (BYTE)nPoint;
					if (pIns->nVolLoopEnd < nPoint) pIns->nVolLoopEnd = (BYTE)nPoint;
					return TRUE;
				}
				break;
			case ENV_PANNING:
				if (nPoint != pIns->nPanLoopStart)
				{
					pIns->nPanLoopStart = (BYTE)nPoint;
					if (pIns->nPanLoopEnd < nPoint) pIns->nPanLoopEnd = (BYTE)nPoint;
					return TRUE;
				}
				break;
			case ENV_PITCH:
				if (nPoint != pIns->nPitchLoopStart)
				{
					pIns->nPitchLoopStart = (BYTE)nPoint;
					if (pIns->nPitchLoopEnd < nPoint) pIns->nPitchLoopEnd = (BYTE)nPoint;
					return TRUE;
				}
				break;
			}
		}
	}
	return FALSE;
}


BOOL CViewInstrument::EnvSetLoopEnd(int nPoint)
//---------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if ((pModDoc) && (nPoint >= 0) && (nPoint <= (int)EnvGetLastPoint()))
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];
		if (pIns)
		{
			switch(m_nEnv)
			{
			case ENV_VOLUME:
				if (nPoint != pIns->nVolLoopEnd)
				{
					pIns->nVolLoopEnd = (BYTE)nPoint;
					if (pIns->nVolLoopStart > nPoint) pIns->nVolLoopStart = (BYTE)nPoint;
					return TRUE;
				}
				break;
			case ENV_PANNING:
				if (nPoint != pIns->nPanLoopEnd)
				{
					pIns->nPanLoopEnd = (BYTE)nPoint;
					if (pIns->nPanLoopStart > nPoint) pIns->nPanLoopStart = (BYTE)nPoint;
					return TRUE;
				}
				break;
			case ENV_PITCH:
				if (nPoint != pIns->nPitchLoopEnd)
				{
					pIns->nPitchLoopEnd = (BYTE)nPoint;
					if (pIns->nPitchLoopStart > nPoint) pIns->nPitchLoopStart = (BYTE)nPoint;
					return TRUE;
				}
				break;
			}
		}
	}
	return FALSE;
}


BOOL CViewInstrument::EnvSetSustainStart(int nPoint)
//--------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if ((pModDoc) && (nPoint >= 0) && (nPoint <= (int)EnvGetLastPoint()))
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];
		if (pIns)
		{
			switch(m_nEnv)
			{
			case ENV_VOLUME:
				if (nPoint != pIns->nVolSustainBegin)
				{
					pIns->nVolSustainBegin = (BYTE)nPoint;
					if ((pIns->nVolSustainEnd < nPoint) || (pSndFile->m_nType & MOD_TYPE_XM)) pIns->nVolSustainEnd = (BYTE)nPoint;
					return TRUE;
				}
				break;
			case ENV_PANNING:
				if (nPoint != pIns->nPanSustainBegin)
				{
					pIns->nPanSustainBegin = (BYTE)nPoint;
					if ((pIns->nPanSustainEnd < nPoint) || (pSndFile->m_nType & MOD_TYPE_XM)) pIns->nPanSustainEnd = (BYTE)nPoint;
					return TRUE;
				}
				break;
			case ENV_PITCH:
				if (nPoint != pIns->nPitchSustainBegin)
				{
					pIns->nPitchSustainBegin = (BYTE)nPoint;
					if ((pIns->nPitchSustainEnd < nPoint) || (pSndFile->m_nType & MOD_TYPE_XM)) pIns->nPitchSustainEnd = (BYTE)nPoint;
					return TRUE;
				}
				break;
			}
		}
	}
	return FALSE;
}


BOOL CViewInstrument::EnvSetSustainEnd(int nPoint)
//------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if ((pModDoc) && (nPoint >= 0) && (nPoint <= (int)EnvGetLastPoint()))
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];
		if (pIns)
		{
			switch(m_nEnv)
			{
			case ENV_VOLUME:
				if (nPoint != pIns->nVolSustainEnd)
				{
					pIns->nVolSustainEnd = (BYTE)nPoint;
					if ((pIns->nVolSustainBegin > nPoint) || (pSndFile->m_nType & MOD_TYPE_XM)) pIns->nVolSustainBegin = (BYTE)nPoint;
					return TRUE;
				}
				break;
			case ENV_PANNING:
				if (nPoint != pIns->nPanSustainEnd)
				{
					pIns->nPanSustainEnd = (BYTE)nPoint;
					if ((pIns->nPanSustainBegin > nPoint) || (pSndFile->m_nType & MOD_TYPE_XM)) pIns->nPanSustainBegin = (BYTE)nPoint;
					return TRUE;
				}
				break;
			case ENV_PITCH:
				if (nPoint != pIns->nPitchSustainEnd)
				{
					pIns->nPitchSustainEnd = (BYTE)nPoint;
					if ((pIns->nPitchSustainBegin > nPoint) || (pSndFile->m_nType & MOD_TYPE_XM)) pIns->nPitchSustainBegin = (BYTE)nPoint;
					return TRUE;
				}
				break;
			}
		}
	}
	return FALSE;
}


BOOL CViewInstrument::EnvSetLoop(BOOL bLoop)
//------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];
		if (pIns)
		{
			DWORD dwMask = 0;
			switch(m_nEnv)
			{
			case ENV_VOLUME:	dwMask = ENV_VOLLOOP; break;
			case ENV_PANNING:	dwMask = ENV_PANLOOP; break;
			case ENV_PITCH:		dwMask = ENV_PITCHLOOP; break;
			}
			if (!dwMask) return FALSE;
			if (bLoop)
			{
				if (!(pIns->dwFlags & dwMask))
				{
					pIns->dwFlags |= dwMask;
					return TRUE;
				}
			} else
			{
				if (pIns->dwFlags & dwMask)
				{
					pIns->dwFlags &= ~dwMask;
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}


BOOL CViewInstrument::EnvSetSustain(BOOL bSustain)
//------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];
		if (pIns)
		{
			DWORD dwMask = 0;
			switch(m_nEnv)
			{
			case ENV_VOLUME:	dwMask = ENV_VOLSUSTAIN; break;
			case ENV_PANNING:	dwMask = ENV_PANSUSTAIN; break;
			case ENV_PITCH:		dwMask = ENV_PITCHSUSTAIN; break;
			}
			if (!dwMask) return FALSE;
			if (bSustain)
			{
				if (!(pIns->dwFlags & dwMask))
				{
					pIns->dwFlags |= dwMask;
					return TRUE;
				}
			} else
			{
				if (pIns->dwFlags & dwMask)
				{
					pIns->dwFlags &= ~dwMask;
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}


BOOL CViewInstrument::EnvSetCarry(BOOL bCarry)
//--------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];
		if (pIns)
		{
			DWORD dwMask = 0;
			switch(m_nEnv)
			{
			case ENV_VOLUME:	dwMask = ENV_VOLCARRY; break;
			case ENV_PANNING:	dwMask = ENV_PANCARRY; break;
			case ENV_PITCH:		dwMask = ENV_PITCHCARRY; break;
			}
			if (!dwMask) return FALSE;
			if (bCarry)
			{
				if (!(pIns->dwFlags & dwMask))
				{
					pIns->dwFlags |= dwMask;
					return TRUE;
				}
			} else
			{
				if (pIns->dwFlags & dwMask)
				{
					pIns->dwFlags &= ~dwMask;
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}


BOOL CViewInstrument::EnvSetVolEnv(BOOL bEnable)
//----------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];
		if (pIns)
		{
			if (bEnable)
			{
				pIns->dwFlags |= ENV_VOLUME;
				if (!pIns->nVolEnv)
				{
					pIns->VolEnv[0] = 64;
					pIns->VolEnv[1] = 64;
					pIns->VolPoints[0] = 0;
					pIns->VolPoints[1] = 10;
					pIns->nVolEnv = 2;
					InvalidateRect(NULL, FALSE);
				}
			} else
			{
				pIns->dwFlags &= ~ENV_VOLUME;
			}
			return TRUE;
		}
	}
	return FALSE;
}


BOOL CViewInstrument::EnvSetPanEnv(BOOL bEnable)
//----------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];
		if (pIns)
		{
			if (bEnable)
			{
				pIns->dwFlags |= ENV_PANNING;
				if (!pIns->nPanEnv)
				{
					pIns->PanEnv[0] = 32;
					pIns->PanEnv[1] = 32;
					pIns->PanPoints[0] = 0;
					pIns->PanPoints[1] = 10;
					pIns->nPanEnv = 2;
					InvalidateRect(NULL, FALSE);
				}
			} else
			{
				pIns->dwFlags &= ~ENV_PANNING;
			}
			return TRUE;
		}
	}
	return FALSE;
}


BOOL CViewInstrument::EnvSetPitchEnv(BOOL bEnable)
//------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];
		if (pIns)
		{
			if ((bEnable) && (pSndFile->m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT)))
			{
				pIns->dwFlags |= ENV_PITCH;
				pIns->dwFlags &= ~ENV_FILTER;
				if (!pIns->nPitchEnv)
				{
					pIns->PitchEnv[0] = 32;
					pIns->PitchEnv[1] = 32;
					pIns->PitchPoints[0] = 0;
					pIns->PitchPoints[1] = 10;
					pIns->nPitchEnv = 2;
					InvalidateRect(NULL, FALSE);
				}
			} else
			{
				pIns->dwFlags &= ~(ENV_PITCH|ENV_FILTER);
			}
			return TRUE;
		}
	}
	return FALSE;
}


BOOL CViewInstrument::EnvSetFilterEnv(BOOL bEnable)
//-------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];
		if (pIns)
		{
			if ((bEnable) && (pSndFile->m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT)))
			{
				pIns->dwFlags |= (ENV_PITCH|ENV_FILTER);
				if (!pIns->nPitchEnv)
				{
					pIns->PitchEnv[0] = 64;
					pIns->PitchEnv[1] = 64;
					pIns->PitchPoints[0] = 0;
					pIns->PitchPoints[1] = 10;
					pIns->nPitchEnv = 2;
					InvalidateRect(NULL, FALSE);
				}
			} else
			{
				pIns->dwFlags &= ~(ENV_PITCH|ENV_FILTER);
			}
			return TRUE;
		}
	}
	return FALSE;
}


int CViewInstrument::TickToScreen(int nTick) const
//------------------------------------------------
{
	return ((nTick+1) * ENV_ZOOM) - GetScrollPos(SB_HORZ);
}

int CViewInstrument::PointToScreen(int nPoint) const
//--------------------------------------------------
{
	return TickToScreen(EnvGetTick(nPoint));
}


int CViewInstrument::ScreenToTick(int x) const
//--------------------------------------------
{
	return (GetScrollPos(SB_HORZ) + x + 1 - ENV_ZOOM) / ENV_ZOOM;
}


int CViewInstrument::QuickScreenToTick(int x, int cachedScrollPos) const
//----------------------------------------------------------------------
{
	return (cachedScrollPos + x + 1 - ENV_ZOOM) / ENV_ZOOM;
}

int CViewInstrument::ScreenToValue(int y) const
//---------------------------------------------
{
	if (m_rcClient.bottom < 2) return 0;
	int n = 64 - ((y * 64 + 1) / (m_rcClient.bottom - 1));
	if (n < 0) return 0;
	if (n > 64) return 64;
	return n;
}


int CViewInstrument::ScreenToPoint(int x0, int y0) const
//------------------------------------------------------
{
	int nPoint = -1;
	int ydist = 0xFFFF, xdist = 0xFFFF;
	int maxpoint = EnvGetLastPoint();
	for (int i=0; i<=maxpoint; i++)
	{
		int dx = x0 - PointToScreen(i);
		int dx2 = dx*dx;
		if (dx2 <= xdist)
		{
			int dy = y0 - ValueToScreen(EnvGetValue(i));
			int dy2 = dy*dy;
			if ((dx2 < xdist) || ((dx2 == xdist) && (dy2 < ydist)))
			{
				nPoint = i;
				xdist = dx2;
				ydist = dy2;
			}
		}
	}
	return nPoint;
}


BOOL CViewInstrument::GetNcButtonRect(UINT nBtn, LPRECT lpRect)
//-------------------------------------------------------------
{
	lpRect->left = 4;
	lpRect->top = 3;
	lpRect->bottom = lpRect->top + ENV_LEFTBAR_CYBTN;
	if (nBtn >= ENV_LEFTBAR_BUTTONS) return FALSE;
	for (UINT i=0; i<nBtn; i++)
	{
		if (cLeftBarButtons[i] == ID_SEPARATOR)
		{
			lpRect->left += ENV_LEFTBAR_CXSEP;
		} else
		{
			lpRect->left += ENV_LEFTBAR_CXBTN + ENV_LEFTBAR_CXSPC;
		}
	}
	if (cLeftBarButtons[nBtn] == ID_SEPARATOR)
	{
		lpRect->left += ENV_LEFTBAR_CXSEP/2 - 2;
		lpRect->right = lpRect->left + 2;
		return FALSE;
	} else
	{
		lpRect->right = lpRect->left + ENV_LEFTBAR_CXBTN;
	}
	return TRUE;
}


void CViewInstrument::UpdateNcButtonState()
//-----------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;
	CDC *pDC = NULL;
	
	if (!pModDoc) return;
	pSndFile = pModDoc->GetSoundFile();
	for (UINT i=0; i<ENV_LEFTBAR_BUTTONS; i++) if (cLeftBarButtons[i] != ID_SEPARATOR)
	{
		DWORD dwStyle = 0;
		
		switch(cLeftBarButtons[i])
		{
		case ID_ENVSEL_VOLUME:		if (m_nEnv == ENV_VOLUME) dwStyle |= NCBTNS_CHECKED; break;
		case ID_ENVSEL_PANNING:		if (m_nEnv == ENV_PANNING) dwStyle |= NCBTNS_CHECKED; break;
		case ID_ENVSEL_PITCH:		if (!(pSndFile->m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT))) dwStyle |= NCBTNS_DISABLED;
									else if (m_nEnv == ENV_PITCH) dwStyle |= NCBTNS_CHECKED; break;
		case ID_ENVELOPE_SETLOOP:	if (EnvGetLoop()) dwStyle |= NCBTNS_CHECKED; break;
		case ID_ENVELOPE_SUSTAIN:	if (EnvGetSustain()) dwStyle |= NCBTNS_CHECKED; break;
		case ID_ENVELOPE_CARRY:		if (!(pSndFile->m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT))) dwStyle |= NCBTNS_DISABLED;
									else if (EnvGetCarry()) dwStyle |= NCBTNS_CHECKED; break;
		case ID_ENVELOPE_VOLUME:	if (EnvGetVolEnv()) dwStyle |= NCBTNS_CHECKED; break;
		case ID_ENVELOPE_PANNING:	if (EnvGetPanEnv()) dwStyle |= NCBTNS_CHECKED; break;
		case ID_ENVELOPE_PITCH:		if (!(pSndFile->m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT))) dwStyle |= NCBTNS_DISABLED; else
									if (EnvGetPitchEnv()) dwStyle |= NCBTNS_CHECKED; break;
		case ID_ENVELOPE_FILTER:	if (!(pSndFile->m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT))) dwStyle |= NCBTNS_DISABLED; else
									if (EnvGetFilterEnv()) dwStyle |= NCBTNS_CHECKED; break;
		case ID_ENVELOPE_VIEWGRID:	if (m_bGrid) dwStyle |= NCBTNS_CHECKED; break;
		}
		if (m_nBtnMouseOver == i)
		{
			dwStyle |= NCBTNS_MOUSEOVER;
			if (m_dwStatus & INSSTATUS_NCLBTNDOWN) dwStyle |= NCBTNS_PUSHED;
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


////////////////////////////////////////////////////////////////////
// CViewInstrument drawing

void CViewInstrument::UpdateView(DWORD dwHintMask, CObject *)
//-----------------------------------------------------------
{
	if ((dwHintMask & (HINT_MPTOPTIONS|HINT_MODTYPE))
	 || ((dwHintMask & HINT_ENVELOPE) && (m_nInstrument == (dwHintMask >> HINT_SHIFT_INS)))
	 || ((dwHintMask & HINT_SPEEDCHANGE))) //rewbs.envRowGrid
	{
		UpdateScrollSize();
		UpdateNcButtonState();
		InvalidateRect(NULL, FALSE);
	}
}

//rewbs.envRowGrid
void CViewInstrument::DrawGrid(CDC *pDC, UINT speed)
//--------------------------------------------------------------------
{
	int cachedScrollPos = GetScrollPos(SB_HORZ);
	bool windowResized = false;

	if (m_dcGrid.GetSafeHdc())
	{
		m_dcGrid.SelectObject(m_pbmpOldGrid);
		m_dcGrid.DeleteDC();
		m_bmpGrid.DeleteObject();
		windowResized = true;
	}


	if (windowResized || m_bGridForceRedraw || (cachedScrollPos != m_GridScrollPos) || (speed != m_GridSpeed)) {

		m_GridSpeed = speed;
		m_GridScrollPos = cachedScrollPos;
		m_bGridForceRedraw = false;

		// create a memory based dc for drawing the grid
		m_dcGrid.CreateCompatibleDC(pDC);
		m_bmpGrid.CreateCompatibleBitmap(pDC,  m_rcClient.right-m_rcClient.left, m_rcClient.bottom-m_rcClient.top);
		m_pbmpOldGrid = m_dcGrid.SelectObject(&m_bmpGrid);

		//do draw
		int width=m_rcClient.right-m_rcClient.left;
		int nPrevTick=-1;
		int nTick, nRow;
		for (int x=3; x<width; x++)
		{
			nTick = QuickScreenToTick(x, cachedScrollPos);
			if (nTick != nPrevTick && !(nTick%speed))
			{
				nPrevTick=nTick;
				nRow=nTick/speed;

				if (nRow % max(1, CMainFrame::m_nRowSpacing) == 0)
					m_dcGrid.SelectObject(CMainFrame::penGray80);
				else if (nRow % max(1, CMainFrame::m_nRowSpacing2) == 0)
					m_dcGrid.SelectObject(CMainFrame::penGray55);
				else
					m_dcGrid.SelectObject(CMainFrame::penGray33);

				m_dcGrid.MoveTo(x+1, 0);
				m_dcGrid.LineTo(x+1, m_rcClient.bottom);
				
			}
		}
	}

	pDC->BitBlt(m_rcClient.left, m_rcClient.top, m_rcClient.right-m_rcClient.left, m_rcClient.bottom-m_rcClient.top, &m_dcGrid, 0, 0, SRCCOPY);
}
//end rewbs.envRowGrid

void CViewInstrument::OnDraw(CDC *pDC)
//------------------------------------
{
	RECT rect;
	int nScrollPos = GetScrollPos(SB_HORZ);
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;
	HGDIOBJ oldpen;
	//HDC hdc;
	UINT maxpoint;
	int ymed = (m_rcClient.bottom - 1) / 2;

//rewbs.envRowGrid
	// to avoid flicker, establish a memory dc, draw to it 
	// and then BitBlt it to the destination "pDC"
	
	//check for window resize
	if (m_dcMemMain.GetSafeHdc()) {
		m_dcMemMain.SelectObject(oldBitmap);
		m_dcMemMain.DeleteDC();
		m_bmpMemMain.DeleteObject();
	}

	m_dcMemMain.CreateCompatibleDC(pDC);
	if (!m_dcMemMain)
		return;
	m_bmpMemMain.CreateCompatibleBitmap(pDC, m_rcClient.right-m_rcClient.left, m_rcClient.bottom-m_rcClient.top);
	oldBitmap = (CBitmap *)m_dcMemMain.SelectObject(&m_bmpMemMain);

//end rewbs.envRowGrid

	if ((!pModDoc) || (!pDC)) return;
	pSndFile = pModDoc->GetSoundFile();
	//hdc = pDC->m_hDC;
	oldpen = m_dcMemMain.SelectObject(CMainFrame::penDarkGray);
	m_dcMemMain.FillRect(&m_rcClient, CBrush::FromHandle(CMainFrame::brushBlack));
	if (m_bGrid)
	{
		DrawGrid(&m_dcMemMain, pSndFile->m_nMusicSpeed);
	}
	if (m_nEnv != ENV_VOLUME)
	{
		m_dcMemMain.MoveTo(0, ymed);
		m_dcMemMain.LineTo(m_rcClient.right, ymed);
	}
	m_dcMemMain.SelectObject(CMainFrame::penDarkGray);
	// Drawing Loop Start/End
	if (EnvGetLoop())
	{
		int x1 = PointToScreen(EnvGetLoopStart()) - (ENV_ZOOM/2);
		m_dcMemMain.MoveTo(x1, 0);
		m_dcMemMain.LineTo(x1, m_rcClient.bottom);
		int x2 = PointToScreen(EnvGetLoopEnd()) + (ENV_ZOOM/2);
		m_dcMemMain.MoveTo(x2, 0);
		m_dcMemMain.LineTo(x2, m_rcClient.bottom);
	}
	// Drawing Sustain Start/End
	if (EnvGetSustain())
	{
		m_dcMemMain.SelectObject(CMainFrame::penHalfDarkGray);
		int nspace = m_rcClient.bottom/4;
		int n1 = EnvGetSustainStart();
		int x1 = PointToScreen(n1) - (ENV_ZOOM/2);
		int y1 = ValueToScreen(EnvGetValue(n1));
		m_dcMemMain.MoveTo(x1, y1 - nspace);
		m_dcMemMain.LineTo(x1, y1+nspace);
		int n2 = EnvGetSustainEnd();
		int x2 = PointToScreen(n2) + (ENV_ZOOM/2);
		int y2 = ValueToScreen(EnvGetValue(n2));
		m_dcMemMain.MoveTo(x2, y2-nspace);
		m_dcMemMain.LineTo(x2, y2+nspace);
	}
	maxpoint = EnvGetNumPoints();
	// Drawing Envelope
	if (maxpoint)
	{
		maxpoint--;
		m_dcMemMain.SelectObject(CMainFrame::penEnvelope);
		int releaseNode = EnvGetReleaseNode();	
		for (UINT i=0; i<=maxpoint; i++)
		{
			int x = (EnvGetTick(i) + 1) * ENV_ZOOM - nScrollPos;
			int y = ValueToScreen(EnvGetValue(i));
			rect.left = x - 3;
			rect.top = y - 3;
			rect.right = x + 4;
			rect.bottom = y + 4;
			if (i) {
				m_dcMemMain.LineTo(x, y);
			} else {
				m_dcMemMain.MoveTo(x, y);
			}
			if (i==releaseNode) {
				m_dcMemMain.FrameRect(&rect, CBrush::FromHandle(CMainFrame::brushHighLightRed));
				m_dcMemMain.SelectObject(CMainFrame::penEnvelopeHighlight);
			} else {
				m_dcMemMain.FrameRect(&rect, CBrush::FromHandle(CMainFrame::brushWhite));
			}

		}
	}
	DrawPositionMarks(m_dcMemMain.m_hDC);
	if (oldpen) m_dcMemMain.SelectObject(oldpen);
	
	//rewbs.envRowGrid
	pDC->BitBlt(m_rcClient.left, m_rcClient.top, m_rcClient.right-m_rcClient.left, m_rcClient.bottom-m_rcClient.top, &m_dcMemMain, 0, 0, SRCCOPY);

// -> CODE#0015
// -> DESC="channels management dlg"
	CMainFrame * pMainFrm = CMainFrame::GetMainFrame();
	BOOL activeDoc = pMainFrm ? pMainFrm->GetActiveDoc() == GetDocument() : FALSE;

	if(activeDoc && CChannelManagerDlg::sharedInstance(FALSE) && CChannelManagerDlg::sharedInstance()->IsDisplayed())
		CChannelManagerDlg::sharedInstance()->SetDocument((void*)this);

// -! NEW_FEATURE#0015
}

BYTE CViewInstrument::EnvGetReleaseNode()
//--------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc) {
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];
		if (pIns) {
			switch(m_nEnv) {
				case ENV_VOLUME:
					return pIns->nVolEnvReleaseNode;
				case ENV_PANNING:
					return pIns->nPanEnvReleaseNode;
				case ENV_PITCH:
					return pIns->nPitchEnvReleaseNode;
				default:
					return ENV_RELEASE_NODE_UNSET;
			}
		}
	}
	return ENV_RELEASE_NODE_UNSET;
}

WORD CViewInstrument::EnvGetReleaseNodeValue()
//--------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc) {
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];
		if (pIns) {
			switch(m_nEnv) {
				case ENV_VOLUME:
					return pIns->VolEnv[EnvGetReleaseNode()];
				case ENV_PANNING:
					return pIns->PanEnv[EnvGetReleaseNode()];
				case ENV_PITCH:
					return pIns->PitchEnv[EnvGetReleaseNode()];
				default:
					return 0;
			}
		}
	}
	return 0;
}

WORD CViewInstrument::EnvGetReleaseNodeTick()
//--------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc) {
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];
		if (pIns) {
			switch(m_nEnv) {
				case ENV_VOLUME:
					return pIns->VolPoints[EnvGetReleaseNode()];
				case ENV_PANNING:
					return pIns->PanPoints[EnvGetReleaseNode()];
				case ENV_PITCH:
					return pIns->PitchPoints[EnvGetReleaseNode()];
				default:
					return 0;
			}
		}
	}
	return 0;
}


void CViewInstrument::DrawPositionMarks(HDC hdc)
//----------------------------------------------
{
	CRect rect;
	for (UINT i=0; i<MAX_CHANNELS; i++) if (m_dwNotifyPos[i] & MPTNOTIFY_POSVALID)
	{
		rect.top = -2;
		rect.left = TickToScreen(m_dwNotifyPos[i] & 0xFFFF);
		rect.right = rect.left + 1;
		rect.bottom = m_rcClient.bottom + 1;
		InvertRect(hdc, &rect);
	}
}


LRESULT CViewInstrument::OnPlayerNotify(MPTNOTIFICATION *pnotify)
//---------------------------------------------------------------
{
	DWORD dwType;
	CModDoc *pModDoc = GetDocument();
	if ((!pnotify) || (!pModDoc)) return 0;
	switch(m_nEnv)
	{
	case ENV_PANNING:	dwType = MPTNOTIFY_PANENV; break;
	case ENV_PITCH:		dwType = MPTNOTIFY_PITCHENV; break;
	default:			dwType = MPTNOTIFY_VOLENV; break;
	}
	if (pnotify->dwType & MPTNOTIFY_STOP)
	{
		for (UINT i=0; i<MAX_CHANNELS; i++)
		{
			if (m_dwNotifyPos[i])
			{
				memset(m_dwNotifyPos, 0, sizeof(m_dwNotifyPos));
				InvalidateEnvelope();
				break;
			}
			memset(m_baPlayingNote, 0, sizeof(bool)*NOTE_MAX); 	//rewbs.instViewNNA
			m_nPlayingChannel=-1;							//rewbs.instViewNNA
		}
	} else
	if ((pnotify->dwType & dwType) && ((pnotify->dwType & 0xFFFF) == m_nInstrument))
	{
		BOOL bUpdate = FALSE;
		for (UINT i=0; i<MAX_CHANNELS; i++)
		{
			//DWORD newpos = (pSndFile->m_dwSongFlags & SONG_PAUSED) ? pnotify->dwPos[i] : 0;
			DWORD newpos = pnotify->dwPos[i];
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
				//DWORD newpos = (pSndFile->m_dwSongFlags & SONG_PAUSED) ? pnotify->dwPos[j] : 0;
				DWORD newpos = pnotify->dwPos[j];
				m_dwNotifyPos[j] = newpos;
			}
			DrawPositionMarks(hdc);
			::ReleaseDC(m_hWnd, hdc);
		}
	}
	return 0;
}


void CViewInstrument::DrawNcButton(CDC *pDC, UINT nBtn)
//-----------------------------------------------------
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
		case ID_ENVSEL_VOLUME:		nImage = 1; break;
		case ID_ENVSEL_PANNING:		nImage = 2; break;
		case ID_ENVSEL_PITCH:		nImage = (dwStyle & NCBTNS_DISABLED) ? 4 : 3; break;
		case ID_ENVELOPE_SETLOOP:	nImage = 5; break;
		case ID_ENVELOPE_SUSTAIN:	nImage = 6; break;
		case ID_ENVELOPE_CARRY:		nImage = (dwStyle & NCBTNS_DISABLED) ? 8 : 7; break;
		case ID_ENVELOPE_VOLUME:	nImage = 9; break;
		case ID_ENVELOPE_PANNING:	nImage = 10; break;
		case ID_ENVELOPE_PITCH:		nImage = (dwStyle & NCBTNS_DISABLED) ? 13 : 11; break;
		case ID_ENVELOPE_FILTER:	nImage = (dwStyle & NCBTNS_DISABLED) ? 14 : 12; break;
		case ID_INSTRUMENT_SAMPLEMAP: nImage = 15; break;
		case ID_ENVELOPE_VIEWGRID:	nImage = 16; break;
		}
		pDC->Draw3dRect(rect.left-1, rect.top-1, ENV_LEFTBAR_CXBTN+2, ENV_LEFTBAR_CYBTN+2, c3, c4);
		pDC->Draw3dRect(rect.left, rect.top, ENV_LEFTBAR_CXBTN, ENV_LEFTBAR_CYBTN, c1, c2);
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
		pDC->Draw3dRect(rect.left, rect.top, 2, ENV_LEFTBAR_CYBTN, c1, c2);
	}
}


void CViewInstrument::OnNcPaint()
//-------------------------------
{
	RECT rect;

	CModScrollView::OnNcPaint();
	GetWindowRect(&rect);
	// Assumes there is no other non-client items
	rect.bottom = ENV_LEFTBAR_CY;
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
			for (UINT i=0; i<ENV_LEFTBAR_BUTTONS; i++)
			{
				DrawNcButton(pDC, i);
			}
		}
		if (oldpen) SelectObject(hdc, oldpen);
		ReleaseDC(pDC);
	}
}


////////////////////////////////////////////////////////////////////
// CViewInstrument messages


void CViewInstrument::OnSize(UINT nType, int cx, int cy)
//------------------------------------------------------
{
	CScrollView::OnSize(nType, cx, cy);
	if (((nType == SIZE_RESTORED) || (nType == SIZE_MAXIMIZED)) && (cx > 0) && (cy > 0))
	{
		UpdateScrollSize();
	}
}


void CViewInstrument::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp)
//---------------------------------------------------------------------------------
{
	CModScrollView::OnNcCalcSize(bCalcValidRects, lpncsp);
	if (lpncsp)
	{
		lpncsp->rgrc[0].top += ENV_LEFTBAR_CY;
		if (lpncsp->rgrc[0].bottom < lpncsp->rgrc[0].top) lpncsp->rgrc[0].top = lpncsp->rgrc[0].bottom;
	}
}


void CViewInstrument::OnNcMouseMove(UINT nHitTest, CPoint point)
//--------------------------------------------------------------
{
	CRect rect, rcWnd;
	UINT nBtnSel = 0xFFFF;

	GetWindowRect(&rcWnd);
	for (UINT i=0; i<ENV_LEFTBAR_BUTTONS; i++)
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
			if ((nBtnSel < ENV_LEFTBAR_BUTTONS) && (cLeftBarButtons[nBtnSel] != ID_SEPARATOR))
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


void CViewInstrument::OnNcLButtonDown(UINT uFlags, CPoint point)
//--------------------------------------------------------------
{
	if (m_nBtnMouseOver < ENV_LEFTBAR_BUTTONS)
	{
		m_dwStatus |= INSSTATUS_NCLBTNDOWN;
		if (cLeftBarButtons[m_nBtnMouseOver] != ID_SEPARATOR)
		{
			PostMessage(WM_COMMAND, cLeftBarButtons[m_nBtnMouseOver]);
			UpdateNcButtonState();
		}
	}
	CModScrollView::OnNcLButtonDown(uFlags, point);
}


void CViewInstrument::OnNcLButtonUp(UINT uFlags, CPoint point)
//------------------------------------------------------------
{
	if (m_dwStatus & INSSTATUS_NCLBTNDOWN)
	{
		m_dwStatus &= ~INSSTATUS_NCLBTNDOWN;
		UpdateNcButtonState();
	}
	CModScrollView::OnNcLButtonUp(uFlags, point);
}


void CViewInstrument::OnNcLButtonDblClk(UINT uFlags, CPoint point)
//----------------------------------------------------------------
{
	OnNcLButtonDown(uFlags, point);
}


#if _MFC_VER > 0x0710
LRESULT CViewInstrument::OnNcHitTest(CPoint point)
#else
UINT CViewInstrument::OnNcHitTest(CPoint point)
#endif 
//---------------------------------------------
{
	CRect rect;
	GetWindowRect(&rect);
	rect.bottom = rect.top + ENV_LEFTBAR_CY;
	if (rect.PtInRect(point))
	{
		return HTBORDER;
	}
	return CModScrollView::OnNcHitTest(point);
}


void CViewInstrument::OnMouseMove(UINT, CPoint pt)
//------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	BOOL bSplitCursor = FALSE;
	CHAR s[256];

	if ((m_nBtnMouseOver < ENV_LEFTBAR_BUTTONS) || (m_dwStatus & INSSTATUS_NCLBTNDOWN))
	{
		m_dwStatus &= ~INSSTATUS_NCLBTNDOWN;
		m_nBtnMouseOver = 0xFFFF;
		UpdateNcButtonState();
		CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
		if (pMainFrm) pMainFrm->SetHelpText("");
	}
	if (!pModDoc) return;
	int nTick = ScreenToTick(pt.x);
	int nVal = ScreenToValue(pt.y);
	if (nVal < 0) nVal = 0;
	if (nVal > 64) nVal = 64;
	if (nTick < 0) nTick = 0;
	if (nTick <= EnvGetReleaseNodeTick() + 1 || EnvGetReleaseNode() == ENV_RELEASE_NODE_UNSET) {
		int displayVal = (m_nEnv != ENV_VOLUME) ? nVal-32 : nVal;
		wsprintf(s, "Tick %d, [%d]", nTick, displayVal);
	} else {
		int displayVal = (nVal - EnvGetReleaseNodeValue()) * 2;
		displayVal = (m_nEnv != ENV_VOLUME) ? displayVal - 32 : displayVal;
		wsprintf(s, "Tick %d, [Rel%c%d]",  nTick, displayVal > 0 ? '+' : '-', abs(displayVal));
	}
	UpdateIndicator(s);
	if ((m_dwStatus & INSSTATUS_DRAGGING) && (m_nDragItem))
	{
		BOOL bChanged = FALSE;
		if (pt.x >= m_rcClient.right - 2) nTick++;
		if (m_nDragItem < 0x100)
		{
			bChanged = EnvSetValue(m_nDragItem-1, nTick, nVal);
		} else
		{
			int nPoint = ScreenToPoint(pt.x, pt.y);
			if (nPoint >= 0) switch(m_nDragItem)
			{
			case ENV_DRAGLOOPSTART:
				bChanged = EnvSetLoopStart(nPoint);
				bSplitCursor = TRUE;
				break;
			case ENV_DRAGLOOPEND:
				bChanged = EnvSetLoopEnd(nPoint);
				bSplitCursor = TRUE;
				break;
			case ENV_DRAGSUSTAINSTART:
				bChanged = EnvSetSustainStart(nPoint);
				bSplitCursor = TRUE;
				break;
			case ENV_DRAGSUSTAINEND:
				bChanged = EnvSetSustainEnd(nPoint);
				bSplitCursor = TRUE;
				break;
			}
		}
		if (bChanged)
		{
			if (pt.x <= 0)
			{
				UpdateScrollSize();
				OnScrollBy(CSize(pt.x-ENV_ZOOM, 0), TRUE);
			}
			if (pt.x >= m_rcClient.right-1)
			{
				UpdateScrollSize();
				OnScrollBy(CSize(ENV_ZOOM+pt.x-m_rcClient.right, 0), TRUE);
			}
			pModDoc->SetModified();
			pModDoc->UpdateAllViews(NULL, (m_nInstrument << HINT_SHIFT_INS) | HINT_ENVELOPE, NULL);
			UpdateWindow(); //rewbs: TODO - optimisation here so we don't redraw whole view.
		}
	} else
	{
		CRect rect;
		if (EnvGetSustain())
		{
			int nspace = m_rcClient.bottom/4;
			rect.top = ValueToScreen(EnvGetValue(EnvGetSustainStart())) - nspace;
			rect.bottom = rect.top + nspace * 2;
			rect.right = PointToScreen(EnvGetSustainStart()) + 1;
			rect.left = rect.right - ENV_ZOOM*2;
			if (rect.PtInRect(pt))
			{
				bSplitCursor = TRUE; // ENV_DRAGSUSTAINSTART;
			} else
			{
				rect.top = ValueToScreen(EnvGetValue(EnvGetSustainEnd())) - nspace;
				rect.bottom = rect.top + nspace * 2;
				rect.left = PointToScreen(EnvGetSustainEnd()) - 1;
				rect.right = rect.left + ENV_ZOOM*2;
				if (rect.PtInRect(pt)) bSplitCursor = TRUE; // ENV_DRAGSUSTAINEND;
			}
		}
		if (EnvGetLoop())
		{
			rect.top = m_rcClient.top;
			rect.bottom = m_rcClient.bottom;
			rect.right = PointToScreen(EnvGetLoopStart()) + 1;
			rect.left = rect.right - ENV_ZOOM*2;
			if (rect.PtInRect(pt))
			{
				bSplitCursor = TRUE; // ENV_DRAGLOOPSTART;
			} else
			{
				rect.left = PointToScreen(EnvGetLoopEnd()) - 1;
				rect.right = rect.left + ENV_ZOOM*2;
				if (rect.PtInRect(pt)) bSplitCursor = TRUE; // ENV_DRAGLOOPEND;
			}
		}
	}
	// Update the mouse cursor
	if (bSplitCursor)
	{
		if (!(m_dwStatus & INSSTATUS_SPLITCURSOR))
		{
			m_dwStatus |= INSSTATUS_SPLITCURSOR;
			if (!(m_dwStatus & INSSTATUS_DRAGGING)) SetCapture();
			SetCursor(CMainFrame::curVSplit);
		}
	} else
	{
		if (m_dwStatus & INSSTATUS_SPLITCURSOR)
		{
			m_dwStatus &= ~INSSTATUS_SPLITCURSOR;
			SetCursor(CMainFrame::curArrow);
			if (!(m_dwStatus & INSSTATUS_DRAGGING))	ReleaseCapture();
		}
	}
}


void CViewInstrument::OnLButtonDown(UINT, CPoint pt)
//--------------------------------------------------
{
	if (!(m_dwStatus & INSSTATUS_DRAGGING))
	{
		CRect rect;
		// Look if dragging a point
		UINT maxpoint = EnvGetLastPoint();
		m_nDragItem = 0;
		for (UINT i=0; i<=maxpoint; i++)
		{
			int x = PointToScreen(i);
			int y = ValueToScreen(EnvGetValue(i));
			rect.SetRect(x-6, y-6, x+7, y+7);
			if (rect.PtInRect(pt))
			{
				m_nDragItem = i+1;
				break;
			}
		}
		if ((!m_nDragItem) && (EnvGetSustain()))
		{
			int nspace = m_rcClient.bottom/4;
			rect.top = ValueToScreen(EnvGetValue(EnvGetSustainStart())) - nspace;
			rect.bottom = rect.top + nspace * 2;
			rect.right = PointToScreen(EnvGetSustainStart()) + 1;
			rect.left = rect.right - ENV_ZOOM*2;
			if (rect.PtInRect(pt))
			{
				m_nDragItem = ENV_DRAGSUSTAINSTART;
			} else
			{
				rect.top = ValueToScreen(EnvGetValue(EnvGetSustainEnd())) - nspace;
				rect.bottom = rect.top + nspace * 2;
				rect.left = PointToScreen(EnvGetSustainEnd()) - 1;
				rect.right = rect.left + ENV_ZOOM*2;
				if (rect.PtInRect(pt)) m_nDragItem = ENV_DRAGSUSTAINEND;
			}
		}
		if ((!m_nDragItem) && (EnvGetLoop()))
		{
			rect.top = m_rcClient.top;
			rect.bottom = m_rcClient.bottom;
			rect.right = PointToScreen(EnvGetLoopStart()) + 1;
			rect.left = rect.right - ENV_ZOOM*2;
			if (rect.PtInRect(pt))
			{
				m_nDragItem = ENV_DRAGLOOPSTART;
			} else
			{
				rect.left = PointToScreen(EnvGetLoopEnd()) - 1;
				rect.right = rect.left + ENV_ZOOM*2;
				if (rect.PtInRect(pt)) m_nDragItem = ENV_DRAGLOOPEND;
			}
		}
		if (m_nDragItem)
		{
			SetCapture();
			m_dwStatus |= INSSTATUS_DRAGGING;
		}
	}
}


void CViewInstrument::OnLButtonUp(UINT, CPoint)
//---------------------------------------------
{
	if (m_dwStatus & INSSTATUS_SPLITCURSOR)
	{
		m_dwStatus &= ~INSSTATUS_SPLITCURSOR;
		SetCursor(CMainFrame::curArrow);
	}
	if (m_dwStatus & INSSTATUS_DRAGGING)
	{
		m_dwStatus &= ~INSSTATUS_DRAGGING;
		ReleaseCapture();
	}
}


void CViewInstrument::OnRButtonDown(UINT, CPoint pt)
//--------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CMenu Menu;
	if (m_dwStatus & INSSTATUS_DRAGGING) return;
	if ((pModDoc) && (Menu.LoadMenu(IDR_ENVELOPES)))
	{
		CMenu* pSubMenu = Menu.GetSubMenu(0);
		if (pSubMenu!=NULL)
		{
			CSoundFile *pSndFile = pModDoc->GetSoundFile();
			UINT maxpoint = (pSndFile->m_nType == MOD_TYPE_XM) ? 11 : 24;
			UINT lastpoint = EnvGetLastPoint();
			m_nDragItem = ScreenToPoint(pt.x, pt.y) + 1;
			pSubMenu->EnableMenuItem(ID_ENVELOPE_INSERTPOINT, (lastpoint < maxpoint) ? MF_ENABLED : MF_GRAYED);
			pSubMenu->EnableMenuItem(ID_ENVELOPE_REMOVEPOINT, ((m_nDragItem) && (lastpoint > 1)) ? MF_ENABLED : MF_GRAYED);
			pSubMenu->EnableMenuItem(ID_ENVELOPE_CARRY, (pSndFile->m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT)) ? MF_ENABLED : MF_GRAYED);
			pSubMenu->EnableMenuItem(ID_ENVELOPE_TOGGLERELEASENODE, (pSndFile->m_nType&(MOD_TYPE_IT|MOD_TYPE_MPT) && m_nEnv==ENV_VOLUME) ? MF_ENABLED : MF_GRAYED);
			pSubMenu->CheckMenuItem(ID_ENVELOPE_SETLOOP, (EnvGetLoop()) ? MF_CHECKED : MF_UNCHECKED);
			pSubMenu->CheckMenuItem(ID_ENVELOPE_SUSTAIN, (EnvGetSustain()) ? MF_CHECKED : MF_UNCHECKED);
			pSubMenu->CheckMenuItem(ID_ENVELOPE_CARRY, (EnvGetCarry()) ? MF_CHECKED : MF_UNCHECKED);
			pSubMenu->CheckMenuItem(ID_ENVELOPE_TOGGLERELEASENODE, (EnvGetReleaseNode()==m_nDragItem-1) ? MF_CHECKED : MF_UNCHECKED);
			m_ptMenu = pt;
			ClientToScreen(&pt);
			pSubMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON,pt.x,pt.y,this);
		}
	}
}


void CViewInstrument::OnPrevInstrument()
//--------------------------------------
{
	SendCtrlMessage(CTRLMSG_INS_PREVINSTRUMENT);
}


void CViewInstrument::OnNextInstrument()
//--------------------------------------
{
	SendCtrlMessage(CTRLMSG_INS_NEXTINSTRUMENT);
}


void CViewInstrument::OnEditSampleMap()
//-------------------------------------
{
	SendCtrlMessage(CTRLMSG_INS_SAMPLEMAP);
}


void CViewInstrument::OnSelectVolumeEnv()
//---------------------------------------
{
	if (m_nEnv != ENV_VOLUME) SetCurrentInstrument(m_nInstrument, ENV_VOLUME);
}


void CViewInstrument::OnSelectPanningEnv()
//----------------------------------------
{
	if (m_nEnv != ENV_PANNING) SetCurrentInstrument(m_nInstrument, ENV_PANNING);
}


void CViewInstrument::OnSelectPitchEnv()
//--------------------------------------
{
	if (m_nEnv != ENV_PITCH) SetCurrentInstrument(m_nInstrument, ENV_PITCH);
}


void CViewInstrument::OnEnvLoopChanged()
//--------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if ((pModDoc) && (EnvSetLoop(!EnvGetLoop())))
	{
		pModDoc->SetModified();
		pModDoc->UpdateAllViews(NULL, (m_nInstrument << HINT_SHIFT_INS) | HINT_ENVELOPE, NULL);
		UpdateNcButtonState();
	}
}


void CViewInstrument::OnEnvSustainChanged()
//-----------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if ((pModDoc) && (EnvSetSustain(!EnvGetSustain())))
	{
		pModDoc->SetModified();
		pModDoc->UpdateAllViews(NULL, (m_nInstrument << HINT_SHIFT_INS) | HINT_ENVELOPE, NULL);
		UpdateNcButtonState();
	}
}


void CViewInstrument::OnEnvCarryChanged()
//---------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if ((pModDoc) && (EnvSetCarry(!EnvGetCarry())))
	{
		pModDoc->SetModified();
		UpdateNcButtonState();
	}
}

void CViewInstrument::OnEnvToggleReleasNode() 
//---------------------------------------------------
{
	int node = m_nDragItem-1;

	CModDoc *pModDoc = GetDocument();
	if ((pModDoc) && (node>0) && (node <= EnvGetLastPoint()))
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];
		switch(m_nEnv) {
			case ENV_VOLUME:
				if (pIns->nVolEnvReleaseNode == node) {
					pIns->nVolEnvReleaseNode = ENV_RELEASE_NODE_UNSET;
				} else { 
					pIns->nVolEnvReleaseNode = node;
				}
				break;
			case ENV_PANNING:
				if (pIns->nPanEnvReleaseNode == node) {
					pIns->nPanEnvReleaseNode = ENV_RELEASE_NODE_UNSET;
				} else { 
					pIns->nPanEnvReleaseNode = node;
				}
				break;
			case ENV_PITCH:
				if (pIns->nPitchEnvReleaseNode == node) {
					pIns->nPitchEnvReleaseNode = ENV_RELEASE_NODE_UNSET;
				} else { 
					pIns->nPitchEnvReleaseNode = node;
				}
				break;			
		}
		pModDoc->SetModified();
		InvalidateRect(NULL, FALSE);
	}
}


void CViewInstrument::OnEnvVolChanged()
//-------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if ((pModDoc) && (EnvSetVolEnv(!EnvGetVolEnv())))
	{
		pModDoc->SetModified();
		UpdateNcButtonState();
	}
}


void CViewInstrument::OnEnvPanChanged()
//-------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if ((pModDoc) && (EnvSetPanEnv(!EnvGetPanEnv())))
	{
		pModDoc->SetModified();
		UpdateNcButtonState();
	}
}


void CViewInstrument::OnEnvPitchChanged()
//---------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if ((pModDoc) && (EnvSetPitchEnv(!EnvGetPitchEnv())))
	{
		pModDoc->SetModified();
		UpdateNcButtonState();
	}
}


void CViewInstrument::OnEnvFilterChanged()
//----------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if ((pModDoc) && (EnvSetFilterEnv(!EnvGetFilterEnv())))
	{
		pModDoc->SetModified();
		UpdateNcButtonState();
	}
}

//rewbs.envRowGrid
void CViewInstrument::OnEnvToggleGrid()
//----------------------------------------
{
	m_bGrid = !m_bGrid;
	if (m_bGrid)
		m_bGridForceRedraw;
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
		pModDoc->UpdateAllViews(NULL, (m_nInstrument << HINT_SHIFT_INS) | HINT_ENVELOPE, NULL);
	
}
//end rewbs.envRowGrid

void CViewInstrument::OnEnvRemovePoint()
//--------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if ((pModDoc) && (m_nDragItem) && (m_nDragItem-1 <= EnvGetLastPoint()))
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];
		if (pIns)
		{
			BOOL bOk = FALSE;
			UINT nPoint = m_nDragItem - 1;
			switch(m_nEnv)
			{
			case ENV_VOLUME:
				if (pIns->nVolEnv > 1)
				{
					pIns->nVolEnv--;
					for (UINT i=nPoint; i<pIns->nVolEnv; i++)
					{
						pIns->VolPoints[i] = pIns->VolPoints[i+1];
						pIns->VolEnv[i] = pIns->VolEnv[i+1];
					}
					if (nPoint >= pIns->nVolEnv) nPoint = pIns->nVolEnv-1;
					if (pIns->nVolLoopStart > nPoint) pIns->nVolLoopStart--;
					if (pIns->nVolLoopEnd > nPoint) pIns->nVolLoopEnd--;
					if (pIns->nVolSustainBegin > nPoint) pIns->nVolSustainBegin--;
					if (pIns->nVolSustainEnd > nPoint) pIns->nVolSustainEnd--;
					if (pIns->nVolEnvReleaseNode>nPoint && pIns->nVolEnvReleaseNode!=ENV_RELEASE_NODE_UNSET) pIns->nVolEnvReleaseNode--;
					pIns->VolPoints[0] = 0;
					bOk = TRUE;
				}
				break;
			case ENV_PANNING:
				if (pIns->nPanEnv > 1)
				{
					pIns->nPanEnv--;
					for (UINT i=nPoint; i<pIns->nPanEnv; i++)
					{
						pIns->PanPoints[i] = pIns->PanPoints[i+1];
						pIns->PanEnv[i] = pIns->PanEnv[i+1];
					}
					if (nPoint >= pIns->nPanEnv) nPoint = pIns->nPanEnv-1;
					if (pIns->nPanLoopStart > nPoint) pIns->nPanLoopStart--;
					if (pIns->nPanLoopEnd > nPoint) pIns->nPanLoopEnd--;
					if (pIns->nPanSustainBegin > nPoint) pIns->nPanSustainBegin--;
					if (pIns->nPanSustainEnd > nPoint) pIns->nPanSustainEnd--;
					if (pIns->nPanEnvReleaseNode>nPoint && pIns->nPanEnvReleaseNode!=ENV_RELEASE_NODE_UNSET) pIns->nPanEnvReleaseNode--;
					pIns->PanPoints[0] = 0;
					bOk = TRUE;
				}
				break;
			case ENV_PITCH:
				if (pIns->nPitchEnv > 1)
				{
					pIns->nPitchEnv--;
					for (UINT i=nPoint; i<pIns->nPitchEnv; i++)
					{
						pIns->PitchPoints[i] = pIns->PitchPoints[i+1];
						pIns->PitchEnv[i] = pIns->PitchEnv[i+1];
					}
					if (nPoint >= pIns->nPitchEnv) nPoint = pIns->nPitchEnv-1;
					if (pIns->nPitchLoopStart > nPoint) pIns->nPitchLoopStart--;
					if (pIns->nPitchLoopEnd > nPoint) pIns->nPitchLoopEnd--;
					if (pIns->nPitchSustainBegin > nPoint) pIns->nPitchSustainBegin--;
					if (pIns->nPitchSustainEnd > nPoint) pIns->nPitchSustainEnd--;
					if (pIns->nPitchEnvReleaseNode>nPoint && pIns->nPitchEnvReleaseNode!=ENV_RELEASE_NODE_UNSET) pIns->nPitchEnvReleaseNode--;
					pIns->PitchPoints[0] = 0;
					bOk = TRUE;
				}
				break;
			}
			if (bOk)
			{
				pModDoc->SetModified();
				pModDoc->UpdateAllViews(NULL, (m_nInstrument << HINT_SHIFT_INS) | HINT_ENVELOPE, NULL);
			}
		}
	}
}


void CViewInstrument::OnEnvInsertPoint()
//--------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];
		if (pIns)
		{
			BOOL bOk = FALSE;
			int nTick = ScreenToTick(m_ptMenu.x);
			int nValue = ScreenToValue(m_ptMenu.y);
			UINT maxpoints = (pSndFile->m_nType == MOD_TYPE_XM) ? 12 : 25;
			//To check: Should there be MAX_ENVPOINTS?

			if (nValue < 0) nValue = 0;
			if (nValue > 64) nValue = 64;
			if (nTick >= 0) switch(m_nEnv)
			{
			case ENV_VOLUME:
				if (pIns->nVolEnv < maxpoints)
				{
					if (!pIns->nVolEnv)
					{
						pIns->VolPoints[0] = 0;
						pIns->VolEnv[0] = 64;
						pIns->nVolEnv = 1;
					}
					UINT i = 0;
					for (i=0; i<pIns->nVolEnv; i++) if (nTick <= pIns->VolPoints[i]) break;
					for (UINT j=pIns->nVolEnv; j>i; j--)
					{
						pIns->VolPoints[j] = pIns->VolPoints[j-1];
						pIns->VolEnv[j] = pIns->VolEnv[j-1];
					}
					pIns->VolPoints[i] = (WORD)nTick;
					pIns->VolEnv[i] = (BYTE)nValue;
					pIns->nVolEnv++;
					if (pIns->nVolLoopStart >= i) pIns->nVolLoopStart++;
					if (pIns->nVolLoopEnd >= i) pIns->nVolLoopEnd++;
					if (pIns->nVolSustainBegin >= i) pIns->nVolSustainBegin++;
					if (pIns->nVolSustainEnd >= i) pIns->nVolSustainEnd++;
					if (pIns->nVolEnvReleaseNode>=i && pIns->nVolEnvReleaseNode!=ENV_RELEASE_NODE_UNSET) pIns->nVolEnvReleaseNode++;
					bOk = TRUE;
				}
				break;
			case ENV_PANNING:
				if (pIns->nPanEnv < maxpoints)
				{
					if (!pIns->nPanEnv)
					{
						pIns->PanPoints[0] = 0;
						pIns->PanEnv[0] = 32;
						pIns->nPanEnv = 1;
					}
					UINT i = 0;
					for (i=0; i<pIns->nPanEnv; i++) if (nTick <= pIns->PanPoints[i]) break;
					for (UINT j=pIns->nPanEnv; j>i; j--)
					{
						pIns->PanPoints[j] = pIns->PanPoints[j-1];
						pIns->PanEnv[j] = pIns->PanEnv[j-1];
					}
					pIns->PanPoints[i] = (WORD)nTick;
					pIns->PanEnv[i] =(BYTE)nValue;
					pIns->nPanEnv++;
					if (pIns->nPanLoopStart >= i) pIns->nPanLoopStart++;
					if (pIns->nPanLoopEnd >= i) pIns->nPanLoopEnd++;
					if (pIns->nPanSustainBegin >= i) pIns->nPanSustainBegin++;
					if (pIns->nPanSustainEnd >= i) pIns->nPanSustainEnd++;
					if (pIns->nPanEnvReleaseNode>=i && pIns->nPanEnvReleaseNode!=ENV_RELEASE_NODE_UNSET) pIns->nPanEnvReleaseNode++;
					bOk = TRUE;
				}
				break;
			case ENV_PITCH:
				if (pIns->nPitchEnv < maxpoints)
				{
					if (!pIns->nPitchEnv)
					{
						pIns->PitchPoints[0] = 0;
						pIns->PitchEnv[0] = 32;
						pIns->nPitchEnv = 1;
					}
					UINT i = 0;
					for (i=0; i<pIns->nPitchEnv; i++) if (nTick <= pIns->PitchPoints[i]) break;
					for (UINT j=pIns->nPitchEnv; j>i; j--)
					{
						pIns->PitchPoints[j] = pIns->PitchPoints[j-1];
						pIns->PitchEnv[j] = pIns->PitchEnv[j-1];
					}
					pIns->PitchPoints[i] = (WORD)nTick;
					pIns->PitchEnv[i] = (BYTE)nValue;
					pIns->nPitchEnv++;
					if (pIns->nPitchLoopStart >= i) pIns->nPitchLoopStart++;
					if (pIns->nPitchLoopEnd >= i) pIns->nPitchLoopEnd++;
					if (pIns->nPitchSustainBegin >= i) pIns->nPitchSustainBegin++;
					if (pIns->nPitchSustainEnd >= i) pIns->nPitchSustainEnd++;
					if (pIns->nPitchEnvReleaseNode>=i && pIns->nPitchEnvReleaseNode!=ENV_RELEASE_NODE_UNSET) pIns->nPitchEnvReleaseNode++;
					bOk = TRUE;
				}
				break;
			}
			if (bOk)
			{
				pModDoc->SetModified();
				pModDoc->UpdateAllViews(NULL, (m_nInstrument << HINT_SHIFT_INS) | HINT_ENVELOPE, NULL);
			}
		}
	}
}


void CViewInstrument::OnEditCopy()
//--------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc) pModDoc->CopyEnvelope(m_nInstrument, m_nEnv);
}


void CViewInstrument::OnEditPaste()
//---------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc) pModDoc->PasteEnvelope(m_nInstrument, m_nEnv);
}

static DWORD nLastNotePlayed = 0;
static DWORD nLastScanCode = 0;


void CViewInstrument::PlayNote(UINT note)
//-----------------------------------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = GetDocument();
	if ((pModDoc) && (pMainFrm) && (note > 0) && (note<128))
	{
		CHAR s[64];
		const size_t sizeofS = sizeof(s) / sizeof(s[0]);
		if (note >= 0xFE)
		{
			pModDoc->NoteOff(0, (note == NOTE_NOTECUT) ? TRUE : FALSE, m_nInstrument);
			pMainFrm->SetInfoText("");
		} else
		if (m_nInstrument && !m_baPlayingNote[note])
		{
			//rewbs.instViewNNA
		/*	CSoundFile *pSoundFile = pModDoc->GetSoundFile();
			if (pSoundFile && m_baPlayingNote>0 && m_nPlayingChannel>=0)
			{
				MODCHANNEL *pChn = &(pSoundFile->Chn[m_nPlayingChannel]); //Get pointer to channel playing last note.
				if (pChn->pHeader)	//is it valid?
				{
					DWORD tempflags = pChn->dwFlags;
					pChn->dwFlags = 0;
					pModDoc->GetSoundFile()->CheckNNA(m_nPlayingChannel, m_nInstrument, note, FALSE); //if so, apply NNA
					pChn->dwFlags = tempflags;
				}
			}
		*/
			MODINSTRUMENT *pIns = pModDoc->GetSoundFile()->Instruments[m_nInstrument];
			if ((!pIns) || (!pIns->Keyboard[note-1] && !pIns->nMixPlug)) return;
			m_baPlayingNote[note] = true;											//rewbs.instViewNNA
			m_nPlayingChannel= pModDoc->PlayNote(note, m_nInstrument, 0, FALSE); //rewbs.instViewNNA
			s[0] = 0;
			if ((note) && (note <= NOTE_MAX)) 
			{
				const std::string temp = pModDoc->GetSoundFile()->GetNoteName(note, m_nInstrument);
				if(temp.size() >= sizeofS)
					wsprintf(s, "%s", "...");
				else
					wsprintf(s, "%s", temp.c_str());
			}
			pMainFrm->SetInfoText(s);
		}
	}
}

void CViewInstrument::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
//-----------------------------------------------------------------
{
	CModScrollView::OnChar(nChar, nRepCnt, nFlags);
}


void CViewInstrument::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
//------------------------------------------------------------------
{
	CModScrollView::OnKeyUp(nChar, nRepCnt, nFlags);
}


void CViewInstrument::OnDropFiles(HDROP hDropInfo)
//------------------------------------------------
{
	UINT nFiles = ::DragQueryFile(hDropInfo, (UINT)-1, NULL, 0);
	if (nFiles)
	{
		TCHAR szFileName[_MAX_PATH] = "";
		CMainFrame::GetMainFrame()->SetForegroundWindow();
		::DragQueryFile(hDropInfo, 0, szFileName, _MAX_PATH);
		if (szFileName[0]) SendCtrlMessage(CTRLMSG_INS_OPENFILE, (LPARAM)szFileName);
	}
	::DragFinish(hDropInfo);
}


BOOL CViewInstrument::OnDragonDrop(BOOL bDoDrop, LPDRAGONDROP lpDropInfo)
//-----------------------------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;
	BOOL bCanDrop = FALSE;
	BOOL bUpdate;

	if ((!lpDropInfo) || (!pModDoc)) return FALSE;
	pSndFile = pModDoc->GetSoundFile();
	switch(lpDropInfo->dwDropType)
	{
	case DRAGONDROP_INSTRUMENT:
		if (lpDropInfo->pModDoc == pModDoc)
		{
			bCanDrop = ((lpDropInfo->dwDropItem)
					 && (lpDropInfo->dwDropItem <= pSndFile->m_nInstruments)
					 && (lpDropInfo->pModDoc == pModDoc));
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
	if ((!pSndFile->m_nInstruments) && (pSndFile->m_nType & (MOD_TYPE_XM|MOD_TYPE_IT|MOD_TYPE_MPT)))
	{
		SendCtrlMessage(CTRLMSG_INS_NEWINSTRUMENT);
	}
	if ((!m_nInstrument) || (m_nInstrument > pSndFile->m_nInstruments)) return FALSE;
	// Do the drop
	bUpdate = FALSE;
	BeginWaitCursor();
	switch(lpDropInfo->dwDropType)
	{
	case DRAGONDROP_INSTRUMENT:
		if (lpDropInfo->pModDoc == pModDoc)
		{
			SendCtrlMessage(CTRLMSG_SETCURRENTINSTRUMENT, lpDropInfo->dwDropItem);
		} else
		{
			SendCtrlMessage(CTRLMSG_INS_SONGDROP, (LPARAM)lpDropInfo);
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
					BEGIN_CRITICAL();
					bCanDrop = dlsbank.ExtractInstrument(pSndFile, m_nInstrument, nIns, nRgn);
					END_CRITICAL();
				}
				bUpdate = TRUE;
				break;
			}
		}
		// Instrument file -> fall through
	case DRAGONDROP_SOUNDFILE:
		SendCtrlMessage(CTRLMSG_INS_OPENFILE, lpDropInfo->lDropParam);
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
			// Melodic: (Instrument)
			{
				nRgn = pDLSBank->GetRegionFromKey(nIns, 60);
			}
			BEGIN_CRITICAL();
			bCanDrop = pDLSBank->ExtractInstrument(pSndFile, m_nInstrument, nIns, nRgn);
			END_CRITICAL();
			bUpdate = TRUE;
		}
		break;
	}
	if (bUpdate)
	{
		pModDoc->UpdateAllViews(NULL, (m_nInstrument << HINT_SHIFT_INS) | HINT_INSTRUMENT | HINT_ENVELOPE | HINT_INSNAMES, NULL);
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


LRESULT CViewInstrument::OnMidiMsg(WPARAM dwMidiDataParam, LPARAM)
//-----------------------------------------------------------
{
	const DWORD dwMidiData = dwMidiDataParam;
	static BYTE midivolume = 127;

	CModDoc *pModDoc = GetDocument();
	BYTE midiByte1 = GetFromMIDIMsg_DataByte1(dwMidiData);
	BYTE midiByte2 = GetFromMIDIMsg_DataByte2(dwMidiData);

	CSoundFile* pSndFile = (pModDoc) ? pModDoc->GetSoundFile() : NULL;
	if (!pSndFile) return 0;

	const BYTE nNote  = midiByte1 + 1;		// +1 is for MPT, where middle C is 61
	int nVol   = midiByte2;					
	BYTE event  = GetFromMIDIMsg_Event(dwMidiData);
	if ((event == 0x9) && !nVol) event = 0x8;	//Convert event to note-off if req'd

	BYTE mappedIndex = 0, paramValue = 0;
	uint32 paramIndex = 0;
	if(pSndFile->GetMIDIMapper().OnMIDImsg(dwMidiData, mappedIndex, paramIndex, paramValue)) 
		return 0;

	switch(event)
	{
		case 0x8: // Note Off
			midiByte2 = 0;

		case 0x9: // Note On
			pModDoc->NoteOff(nNote, FALSE, m_nInstrument);
			if (midiByte2 & 0x7F)
			{
				nVol = ApplyVolumeRelatedMidiSettings(dwMidiData, midivolume);
				pModDoc->PlayNote(nNote, m_nInstrument, 0, FALSE, nVol);
			}
		break;

		case 0xB: //Controller change
			switch(midiByte1)
			{
				case 0x7: //Volume
					midivolume = midiByte2;
				break;
			}
		default:
			if(CMainFrame::m_dwMidiSetup & MIDISETUP_MIDITOPLUG && CMainFrame::GetMainFrame()->GetModPlaying() == pModDoc)
			{
				const INSTRUMENTINDEX instr = m_nInstrument;
				IMixPlugin* plug = pSndFile->GetInstrumentPlugin(instr);
				if(plug)
				{
					plug->MidiSend(dwMidiData);
					// Sending midi may modify the plug. For now, if MIDI data
					// is not active sensing, set modified.
					if(dwMidiData != MIDISTATUS_ACTIVESENSING)
						CMainFrame::GetMainFrame()->ThreadSafeSetModified(pModDoc);
				}
			}
		break;
	}

	return 0;
}

BOOL CViewInstrument::PreTranslateMessage(MSG *pMsg)
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
			InputTargetContext ctx = (InputTargetContext)(kCtxViewInstruments);
			
			if (ih->KeyEvent(ctx, nChar, nRepCnt, nFlags, kT) != kcNull)
				return true; // Mapped to a command, no need to pass message on.
		}

	}
	
	return CModScrollView::PreTranslateMessage(pMsg);
}

LRESULT CViewInstrument::OnCustomKeyMsg(WPARAM wParam, LPARAM)
//------------------------------------------------------------
{
	if (wParam == kcNull)
		return NULL;
	
	CModDoc *pModDoc = GetDocument();	
	if (!pModDoc) return NULL;
	
	//CSoundFile *pSndFile = pModDoc->GetSoundFile();	
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();

	switch(wParam)
	{
		case kcPrevInstrument:	OnPrevInstrument(); return wParam;
		case kcNextInstrument:	OnNextInstrument(); return wParam;
		case kcEditCopy:		OnEditCopy(); return wParam;
		case kcEditPaste:		OnEditPaste(); return wParam;
		case kcNoteOffOld:
		case kcNoteOff:			PlayNote(NOTE_KEYOFF); return wParam;
		case kcNoteCutOld:
		case kcNoteCut:			PlayNote(NOTE_NOTECUT); return wParam;
		case kcInstrumentLoad:	SendCtrlMessage(IDC_INSTRUMENT_OPEN); return wParam;
		case kcInstrumentSave:	SendCtrlMessage(IDC_INSTRUMENT_SAVEAS); return wParam;
		case kcInstrumentNew:	SendCtrlMessage(IDC_INSTRUMENT_NEW); return wParam;	
	}
	if (wParam>=kcInstrumentStartNotes && wParam<=kcInstrumentEndNotes)
	{
		PlayNote(wParam-kcInstrumentStartNotes+1+pMainFrm->GetBaseOctave()*12);
		return wParam;
	}
	if (wParam>=kcInstrumentStartNoteStops && wParam<=kcInstrumentEndNoteStops)
	{	
		int note =wParam-kcInstrumentStartNoteStops+1+pMainFrm->GetBaseOctave()*12;
		m_baPlayingNote[note]=false; 
		pModDoc->NoteOff(note, FALSE, m_nInstrument);
		return wParam;
	}

	return NULL;
}
void CViewInstrument::OnEnvelopeScalepoints()
//--------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile = (pModDoc) ? pModDoc->GetSoundFile() : NULL;
	if(pSndFile == NULL)
		return;

	if(m_nInstrument >= 1 &&
	   m_nInstrument <= pSndFile->GetNumInstruments() &&
	   pSndFile->Instruments[m_nInstrument])
	{
		CScaleEnvPointsDlg dialog(this, pSndFile->Instruments[m_nInstrument], m_nEnv);
		if (dialog.DoModal() == IDOK)
		{
			pModDoc->SetModified();
			pModDoc->UpdateAllViews(NULL, (m_nInstrument << HINT_SHIFT_INS) | HINT_ENVELOPE, NULL);
		}
	}

	
}
