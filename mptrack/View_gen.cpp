#include "stdafx.h"
#include "mptrack.h"
#include "mainfrm.h"
#include "childfrm.h"
#include "moddoc.h"
#include "globals.h"
#include "ctrl_gen.h"
#include "view_gen.h"
#include "vstplug.h"

#define ID_FXCOMMANDS_BASE	41000


IMPLEMENT_SERIAL(CViewGlobals, CFormView, 0)

BEGIN_MESSAGE_MAP(CViewGlobals, CFormView)
	//{{AFX_MSG_MAP(CViewGlobals)
	ON_WM_SIZE()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_DESTROY()
	ON_COMMAND(IDC_CHECK1,		OnMute1)
	ON_COMMAND(IDC_CHECK3,		OnMute2)
	ON_COMMAND(IDC_CHECK5,		OnMute3)
	ON_COMMAND(IDC_CHECK7,		OnMute4)
	ON_COMMAND(IDC_CHECK2,		OnSurround1)
	ON_COMMAND(IDC_CHECK4,		OnSurround2)
	ON_COMMAND(IDC_CHECK6,		OnSurround3)
	ON_COMMAND(IDC_CHECK8,		OnSurround4)
	ON_COMMAND(IDC_CHECK9,		OnMixModeChanged)
	ON_COMMAND(IDC_CHECK10,		OnBypassChanged)
	ON_COMMAND(IDC_CHECK11,		OnDryMixChanged)
	ON_COMMAND(IDC_BUTTON1,		OnSelectPlugin)
	ON_COMMAND(IDC_BUTTON2,		OnEditPlugin)
	ON_COMMAND(IDC_BUTTON3,		OnSetParameter)
	ON_COMMAND(IDC_BUTTON4,		OnNextPlugin)
	ON_COMMAND(IDC_BUTTON5,		OnPrevPlugin)
	ON_EN_UPDATE(IDC_EDIT1,		OnEditVol1)
	ON_EN_UPDATE(IDC_EDIT3,		OnEditVol2)
	ON_EN_UPDATE(IDC_EDIT5,		OnEditVol3)
	ON_EN_UPDATE(IDC_EDIT7,		OnEditVol4)
	ON_EN_UPDATE(IDC_EDIT2,		OnEditPan1)
	ON_EN_UPDATE(IDC_EDIT4,		OnEditPan2)
	ON_EN_UPDATE(IDC_EDIT6,		OnEditPan3)
	ON_EN_UPDATE(IDC_EDIT8,		OnEditPan4)
	ON_EN_UPDATE(IDC_EDIT9,		OnEditName1)
	ON_EN_UPDATE(IDC_EDIT10,	OnEditName2)
	ON_EN_UPDATE(IDC_EDIT11,	OnEditName3)
	ON_EN_UPDATE(IDC_EDIT12,	OnEditName4)
	ON_EN_UPDATE(IDC_EDIT13,	OnPluginNameChanged)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnFx1Changed)
	ON_CBN_SELCHANGE(IDC_COMBO2, OnFx2Changed)
	ON_CBN_SELCHANGE(IDC_COMBO3, OnFx3Changed)
	ON_CBN_SELCHANGE(IDC_COMBO4, OnFx4Changed)
	ON_CBN_SELCHANGE(IDC_COMBO5, OnPluginChanged)
	ON_CBN_SELCHANGE(IDC_COMBO6, OnParamChanged)
	ON_CBN_SELCHANGE(IDC_COMBO7, OnOutputRoutingChanged)
	ON_COMMAND_RANGE(ID_FXCOMMANDS_BASE, ID_FXCOMMANDS_BASE+10, OnFxCommands)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TABCTRL1,	OnTabSelchange)
	ON_MESSAGE(WM_MOD_UNLOCKCONTROLS,		OnUnlockControls)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CViewGlobals::DoDataExchange(CDataExchange* pDX)
//---------------------------------------------------
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CViewGlobals)
	DDX_Control(pDX, IDC_TABCTRL1,	m_TabCtrl);
	DDX_Control(pDX, IDC_COMBO1,	m_CbnEffects[0]);
	DDX_Control(pDX, IDC_COMBO2,	m_CbnEffects[1]);
	DDX_Control(pDX, IDC_COMBO3,	m_CbnEffects[2]);
	DDX_Control(pDX, IDC_COMBO4,	m_CbnEffects[3]);
	DDX_Control(pDX, IDC_COMBO5,	m_CbnPlugin);
	DDX_Control(pDX, IDC_COMBO6,	m_CbnParam);
	DDX_Control(pDX, IDC_COMBO7,	m_CbnOutput);
	DDX_Control(pDX, IDC_SLIDER1,	m_sbVolume[0]);
	DDX_Control(pDX, IDC_SLIDER2,	m_sbPan[0]);
	DDX_Control(pDX, IDC_SLIDER3,	m_sbVolume[1]);
	DDX_Control(pDX, IDC_SLIDER4,	m_sbPan[1]);
	DDX_Control(pDX, IDC_SLIDER5,	m_sbVolume[2]);
	DDX_Control(pDX, IDC_SLIDER6,	m_sbPan[2]);
	DDX_Control(pDX, IDC_SLIDER7,	m_sbVolume[3]);
	DDX_Control(pDX, IDC_SLIDER8,	m_sbPan[3]);
	DDX_Control(pDX, IDC_SLIDER9,	m_sbValue);
	DDX_Control(pDX, IDC_SPIN1,		m_spinVolume[0]);
	DDX_Control(pDX, IDC_SPIN2,		m_spinPan[0]);
	DDX_Control(pDX, IDC_SPIN3,		m_spinVolume[1]);
	DDX_Control(pDX, IDC_SPIN4,		m_spinPan[1]);
	DDX_Control(pDX, IDC_SPIN5,		m_spinVolume[2]);
	DDX_Control(pDX, IDC_SPIN6,		m_spinPan[2]);
	DDX_Control(pDX, IDC_SPIN7,		m_spinVolume[3]);
	DDX_Control(pDX, IDC_SPIN8,		m_spinPan[3]);
	DDX_Control(pDX, IDC_BUTTON1,	m_BtnSelect);
	DDX_Control(pDX, IDC_BUTTON2,	m_BtnEdit);
	//}}AFX_DATA_MAP
}


void CViewGlobals::OnInitialUpdate()
//----------------------------------
{
	CChildFrame *pFrame = (CChildFrame *)GetParentFrame();
	int nMapMode = MM_TEXT;
	SIZE sizeTotal, sizePage, sizeLine;

	m_nActiveTab = -1;
	m_nCurrentPlugin = 0;
	m_nCurrentParam = 0;
	CFormView::OnInitialUpdate();
	if (pFrame)
	{
		GENERALVIEWSTATE *pState = pFrame->GetGeneralViewState();
		if (pState->cbStruct == sizeof(GENERALVIEWSTATE))
		{
			m_TabCtrl.SetCurSel(pState->nTab);
			m_nActiveTab = pState->nTab;
			m_nCurrentPlugin = pState->nPlugin;
			m_nCurrentParam = pState->nParam;
		}
	}
	GetDeviceScrollSizes(nMapMode, sizeTotal, sizePage, sizeLine);
	m_rcClient.SetRect(0, 0, sizeTotal.cx, sizeTotal.cy);
	RecalcLayout();
	// Initializing scroll ranges
	for (int ichn=0; ichn<4; ichn++)
	{
		// Volume Slider
		m_sbVolume[ichn].SetRange(0, 64);
		m_sbVolume[ichn].SetTicFreq(8);
		// Pan Slider
		m_sbPan[ichn].SetRange(0, 64);
		m_sbPan[ichn].SetTicFreq(8);
		// Volume Spin
		m_spinVolume[ichn].SetRange(0, 64);
		// Pan Spin
		m_spinPan[ichn].SetRange(0, 256);
	}
	m_sbValue.SetPos(0);
	m_sbValue.SetRange(0, 100);
	UpdateView(HINT_MODTYPE);
	OnParamChanged();
	m_nLockCount = 0;
}


VOID CViewGlobals::OnDestroy()
//----------------------------
{
	CChildFrame *pFrame = (CChildFrame *)GetParentFrame();
	if (pFrame)
	{
		GENERALVIEWSTATE *pState = pFrame->GetGeneralViewState();
		pState->cbStruct = sizeof(GENERALVIEWSTATE);
		pState->nTab = m_nActiveTab;
		pState->nPlugin = m_nCurrentPlugin;
		pState->nParam = m_nCurrentParam;
	}
	CFormView::OnDestroy();
}


void CViewGlobals::RecalcLayout()
//-------------------------------
{
	if (m_TabCtrl.m_hWnd != NULL)
	{
		CRect rect;
		GetClientRect(&rect);
		if (rect.right < m_rcClient.right) rect.right = m_rcClient.right;
		if (rect.bottom < m_rcClient.bottom) rect.bottom = m_rcClient.bottom;
		m_TabCtrl.SetWindowPos(NULL, 0,0, rect.right, rect.bottom, SWP_NOZORDER|SWP_NOMOVE);
	}
}


int CViewGlobals::GetDlgItemIntEx(UINT nID)
//-----------------------------------------
{
	CHAR s[80];
	s[0] = 0;
	GetDlgItemText(nID, s, sizeof(s));
	if ((s[0] < '0') || (s[0] > '9')) return -1;
	return atoi(s);
}


void CViewGlobals::OnUpdate(CView* pView, LPARAM lHint, CObject*pHint)
//--------------------------------------------------------------------
{
	if (((lHint & 0xFFFF) == HINT_MODCHANNELS) && ((lHint >> 24) != m_nActiveTab)) return;
	if (pView != this) UpdateView(lHint, pHint);
}


void CViewGlobals::OnSize(UINT nType, int cx, int cy)
//---------------------------------------------------
{
	CFormView::OnSize(nType, cx, cy);
	if (((nType == SIZE_RESTORED) || (nType == SIZE_MAXIMIZED)) && (cx > 0) && (cy > 0) && (m_hWnd))
	{
		RecalcLayout();
	}
}


void CViewGlobals::UpdateView(DWORD dwHintMask, CObject *)
//--------------------------------------------------------
{
	CHAR s[128];
	TC_ITEM tci;
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;
	int nTabCount, nTabIndex;
	
	if (!pModDoc) return;
	if (!(dwHintMask & (HINT_MODTYPE|HINT_MODCHANNELS|HINT_MIXPLUGINS))) return;
	pSndFile = pModDoc->GetSoundFile();
	nTabCount = (pSndFile->m_nChannels + 3) / 4;
	if (nTabCount != m_TabCtrl.GetItemCount())
	{
		UINT nOldSel = m_TabCtrl.GetCurSel();
		if (!m_TabCtrl.GetItemCount()) nOldSel = m_nActiveTab;
		m_TabCtrl.SetRedraw(FALSE);
		m_TabCtrl.DeleteAllItems();
		for (int iItem=0; iItem<nTabCount; iItem++)
		{
			wsprintf(s, "%d - %d", iItem * 4 + 1, iItem * 4 + 4);
			tci.mask = TCIF_TEXT | TCIF_PARAM;
			tci.pszText = s;
			tci.lParam = iItem * 4;
			m_TabCtrl.InsertItem(iItem, &tci);
		}
		if (nOldSel >= (UINT)nTabCount) nOldSel = 0;
		m_TabCtrl.SetCurSel(nOldSel);
		m_TabCtrl.SetRedraw(TRUE);
		InvalidateRect(NULL, FALSE);
	}
	nTabIndex = m_TabCtrl.GetCurSel();
	if ((nTabIndex < 0) || (nTabIndex >= nTabCount)) return; // ???
	if ((m_nActiveTab != nTabIndex) || (dwHintMask & (HINT_MODTYPE|HINT_MODCHANNELS)))
	{
		LockControls();
		m_nActiveTab = nTabIndex;
		for (int ichn=0; ichn<4; ichn++)
		{
			UINT nChn = nTabIndex*4+ichn;
			BOOL bEnable = (nChn < pSndFile->m_nChannels) ? TRUE : FALSE;
			// Text
			s[0] = 0;
			if (bEnable) wsprintf(s, "Channel %d", nChn+1);
			SetDlgItemText(IDC_TEXT1+ichn, s);
			// Mute
			CheckDlgButton(IDC_CHECK1+ichn*2, (pSndFile->ChnSettings[nChn].dwFlags & CHN_MUTE) ? TRUE : FALSE);
			// Surround
			CheckDlgButton(IDC_CHECK2+ichn*2, (pSndFile->ChnSettings[nChn].dwFlags & CHN_SURROUND) ? TRUE : FALSE);
			// Volume
			int vol = pSndFile->ChnSettings[nChn].nVolume;
			m_sbVolume[ichn].SetPos(vol);
			SetDlgItemInt(IDC_EDIT1+ichn*2, vol);
			// Pan
			int pan = pSndFile->ChnSettings[nChn].nPan;
			m_sbPan[ichn].SetPos(pan/4);
			SetDlgItemInt(IDC_EDIT2+ichn*2, pan);
			memcpy(s, pSndFile->ChnSettings[nChn].szName, MAX_CHANNELNAME);
			s[MAX_CHANNELNAME-1] = 0;
			SetDlgItemText(IDC_EDIT9+ichn, s);
			// Channel effect
			m_CbnEffects[ichn].SetRedraw(FALSE);
			m_CbnEffects[ichn].ResetContent();
			m_CbnEffects[ichn].SetItemData(m_CbnEffects[ichn].AddString("No effect"), 0);
			int fxsel = 0;
			for (UINT ifx=0; ifx<MAX_MIXPLUGINS; ifx++)
			{
				if ((pSndFile->m_MixPlugins[ifx].Info.dwPluginId1)
				 || (pSndFile->m_MixPlugins[ifx].Info.dwPluginId2)
				 || (pSndFile->m_MixPlugins[ifx].Info.szName[0]
				 || (pSndFile->ChnSettings[nChn].nMixPlugin == ifx+1)))
				{
					wsprintf(s, "FX%d: %s", ifx+1, pSndFile->m_MixPlugins[ifx].Info.szName);
					int n = m_CbnEffects[ichn].AddString(s);
					m_CbnEffects[ichn].SetItemData(n, ifx+1);
					if (pSndFile->ChnSettings[nChn].nMixPlugin == ifx+1) fxsel = n;
				}
			}
			m_CbnEffects[ichn].SetRedraw(TRUE);
			m_CbnEffects[ichn].SetCurSel(fxsel);
			// Enable/Disable controls for this channel
			BOOL bIT = ((bEnable) && (pSndFile->m_nType == MOD_TYPE_IT));
			::EnableWindow(::GetDlgItem(m_hWnd, IDC_CHECK1+ichn*2), bEnable);
			::EnableWindow(::GetDlgItem(m_hWnd, IDC_CHECK2+ichn*2), bIT);
			::EnableWindow(m_sbVolume[ichn].m_hWnd, bEnable);
			::EnableWindow(m_sbPan[ichn].m_hWnd, bEnable);
			::EnableWindow(m_spinVolume[ichn], bEnable);
			::EnableWindow(m_spinPan[ichn], bEnable);
			::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT1+ichn*2), bEnable);
			::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT2+ichn*2), bEnable);
			::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT9+ichn), ((bEnable) && (pSndFile->m_nType & (MOD_TYPE_XM|MOD_TYPE_IT))));
		}
		UnlockControls();
	}
	// Update plugins
	if (dwHintMask & (HINT_MIXPLUGINS|HINT_MODTYPE))
	{
		m_CbnPlugin.SetRedraw(FALSE);
		m_CbnPlugin.ResetContent();
		for (UINT iPlug=0; iPlug<MAX_MIXPLUGINS; iPlug++)
		{
			PSNDMIXPLUGIN p = &pSndFile->m_MixPlugins[iPlug];
			p->Info.szLibraryName[63] = 0;
			if (p->Info.szLibraryName[0])
				wsprintf(s, "FX%d: %s", iPlug+1, p->Info.szLibraryName);
			else
				wsprintf(s, "FX%d: undefined", iPlug+1);
			m_CbnPlugin.AddString(s);
		}
		m_CbnPlugin.SetRedraw(TRUE);
		m_CbnPlugin.SetCurSel(m_nCurrentPlugin);
		if (m_nCurrentPlugin >= MAX_MIXPLUGINS) m_nCurrentPlugin = 0;
		PSNDMIXPLUGIN pPlugin = &pSndFile->m_MixPlugins[m_nCurrentPlugin];
		SetDlgItemText(IDC_EDIT13, pPlugin->Info.szName);
		CheckDlgButton(IDC_CHECK9, (pPlugin->Info.dwInputRouting & MIXPLUG_INPUTF_MASTEREFFECT) ? TRUE : FALSE);
		CheckDlgButton(IDC_CHECK10, (pPlugin->Info.dwInputRouting & MIXPLUG_INPUTF_BYPASS) ? TRUE : FALSE);
		CheckDlgButton(IDC_CHECK11, (pPlugin->Info.dwInputRouting & MIXPLUG_INPUTF_WETMIX) ? TRUE : FALSE);
		CVstPlugin *pVstPlugin = (pPlugin->pMixPlugin) ? (CVstPlugin *)pPlugin->pMixPlugin : NULL;
		m_BtnEdit.EnableWindow(((pVstPlugin) && ((pVstPlugin->HasEditor()) || (pVstPlugin->GetNumCommands()))) ? TRUE : FALSE);
		if (pVstPlugin)
		{
			UINT nParams = pVstPlugin->GetNumParameters();
			m_CbnParam.SetRedraw(FALSE);
			m_CbnParam.ResetContent();
			for (UINT i=0; i<nParams; i++)
			{
				CHAR sname[64];
				pVstPlugin->GetParamName(i, sname, sizeof(sname));
				wsprintf(s, "%02X: %s", i|0x80, sname);
				m_CbnParam.SetItemData(m_CbnParam.AddString(s), i);
			}
			m_CbnParam.SetRedraw(TRUE);
			if (m_nCurrentParam >= nParams) m_nCurrentParam = 0;
			m_CbnParam.SetCurSel(m_nCurrentParam);
			pVstPlugin->GetPluginType(s);
		} else
		{
			s[0] = 0;
			if (m_CbnParam.GetCount() > 0) m_CbnParam.ResetContent();
			m_nCurrentParam = 0;
		}
		SetDlgItemText(IDC_TEXT6, s);
		int outputsel = 0;
		m_CbnOutput.SetRedraw(FALSE);
		m_CbnOutput.ResetContent();
		m_CbnOutput.SetItemData(m_CbnOutput.AddString("Default"), 0);
		for (UINT iOut=m_nCurrentPlugin+1; iOut<MAX_MIXPLUGINS; iOut++)
		{
			PSNDMIXPLUGIN p = &pSndFile->m_MixPlugins[iOut];
			if (p->Info.szLibraryName[0])
			{
				wsprintf(s, "FX%d: %s", iOut+1, p->Info.szLibraryName);
				int n = m_CbnOutput.AddString(s);
				m_CbnOutput.SetItemData(n, 0x80|iOut);
				if ((pSndFile->m_MixPlugins[m_nCurrentPlugin].Info.dwOutputRouting & 0x80)
				 && ((pSndFile->m_MixPlugins[m_nCurrentPlugin].Info.dwOutputRouting & 0x7f) == iOut))
				{
					outputsel = n;
				}
			}
		}
		m_CbnOutput.SetRedraw(TRUE);
		m_CbnOutput.SetCurSel(outputsel);
	}
}


void CViewGlobals::OnTabSelchange(NMHDR*, LRESULT* pResult)
//---------------------------------------------------------
{
	UpdateView(HINT_MODCHANNELS);
	if (pResult) *pResult = 0;
}


void CViewGlobals::OnMute1()
//--------------------------
{
	CModDoc *pModDoc = GetDocument();
	BOOL b = (IsDlgButtonChecked(IDC_CHECK1)) ? TRUE : FALSE;
	UINT nChn = m_nActiveTab * 4;
	
	if (pModDoc)
	{
		pModDoc->MuteChannel(nChn, b);
		pModDoc->UpdateAllViews(this, HINT_MODCHANNELS | (m_nActiveTab << 24));
	}
}


void CViewGlobals::OnMute2()
//--------------------------
{
	CModDoc *pModDoc = GetDocument();
	BOOL b = (IsDlgButtonChecked(IDC_CHECK3)) ? TRUE : FALSE;
	UINT nChn = m_nActiveTab * 4 + 1;
	
	if (pModDoc)
	{
		pModDoc->MuteChannel(nChn, b);
		pModDoc->UpdateAllViews(this, HINT_MODCHANNELS | (m_nActiveTab << 24));
	}
}


void CViewGlobals::OnMute3()
//--------------------------
{
	CModDoc *pModDoc = GetDocument();
	BOOL b = (IsDlgButtonChecked(IDC_CHECK5)) ? TRUE : FALSE;
	UINT nChn = m_nActiveTab * 4 + 2;
	
	if (pModDoc)
	{
		pModDoc->MuteChannel(nChn, b);
		pModDoc->UpdateAllViews(this, HINT_MODCHANNELS | (m_nActiveTab << 24));
	}
}


void CViewGlobals::OnMute4()
//--------------------------
{
	CModDoc *pModDoc = GetDocument();
	BOOL b = (IsDlgButtonChecked(IDC_CHECK7)) ? TRUE : FALSE;
	UINT nChn = m_nActiveTab * 4 + 3;
	
	if (pModDoc)
	{
		pModDoc->MuteChannel(nChn, b);
		pModDoc->UpdateAllViews(this, HINT_MODCHANNELS | (m_nActiveTab << 24));
	}
}


void CViewGlobals::OnSurround1()
//------------------------------
{
	CModDoc *pModDoc = GetDocument();
	BOOL b = (IsDlgButtonChecked(IDC_CHECK2)) ? TRUE : FALSE;
	UINT nChn = m_nActiveTab * 4;
	
	if (pModDoc)
	{
		pModDoc->SurroundChannel(nChn, b);
		pModDoc->UpdateAllViews(this, HINT_MODCHANNELS | (m_nActiveTab << 24));
	}
}


void CViewGlobals::OnSurround2()
//------------------------------
{
	CModDoc *pModDoc = GetDocument();
	BOOL b = (IsDlgButtonChecked(IDC_CHECK4)) ? TRUE : FALSE;
	UINT nChn = m_nActiveTab * 4 + 1;
	
	if (pModDoc)
	{
		pModDoc->SurroundChannel(nChn, b);
		pModDoc->UpdateAllViews(this, HINT_MODCHANNELS | (m_nActiveTab << 24));
	}
}


void CViewGlobals::OnSurround3()
//------------------------------
{
	CModDoc *pModDoc = GetDocument();
	BOOL b = (IsDlgButtonChecked(IDC_CHECK6)) ? TRUE : FALSE;
	UINT nChn = m_nActiveTab * 4 + 2;
	
	if (pModDoc)
	{
		pModDoc->SurroundChannel(nChn, b);
		pModDoc->UpdateAllViews(this, HINT_MODCHANNELS | (m_nActiveTab << 24));
	}
}


void CViewGlobals::OnSurround4()
//------------------------------
{
	CModDoc *pModDoc = GetDocument();
	BOOL b = (IsDlgButtonChecked(IDC_CHECK8)) ? TRUE : FALSE;
	UINT nChn = m_nActiveTab * 4 + 3;
	
	if (pModDoc)
	{
		pModDoc->SurroundChannel(nChn, b);
		pModDoc->UpdateAllViews(this, HINT_MODCHANNELS | (m_nActiveTab << 24));
	}
}


void CViewGlobals::OnEditVol1()
//-----------------------------
{
	CModDoc *pModDoc = GetDocument();
	UINT nChn = m_nActiveTab * 4;
	int vol = GetDlgItemIntEx(IDC_EDIT1);
	if ((pModDoc) && (vol >= 0) && (vol <= 64) && (!m_nLockCount))
	{
		if (pModDoc->SetChannelGlobalVolume(nChn, vol))
		{
			m_sbVolume[0].SetPos(vol);
			pModDoc->UpdateAllViews(this, HINT_MODCHANNELS | (m_nActiveTab << 24));
		}
	}
}


void CViewGlobals::OnEditVol2()
//-----------------------------
{
	CModDoc *pModDoc = GetDocument();
	UINT nChn = m_nActiveTab * 4 + 1;
	int vol = GetDlgItemIntEx(IDC_EDIT3);
	if ((pModDoc) && (vol >= 0) && (vol <= 64) && (!m_nLockCount))
	{
		if (pModDoc->SetChannelGlobalVolume(nChn, vol))
		{
			m_sbVolume[1].SetPos(vol);
			pModDoc->UpdateAllViews(this, HINT_MODCHANNELS | (m_nActiveTab << 24));
		}
	}
}


void CViewGlobals::OnEditVol3()
//-----------------------------
{
	CModDoc *pModDoc = GetDocument();
	UINT nChn = m_nActiveTab * 4 + 2;
	int vol = GetDlgItemIntEx(IDC_EDIT5);
	if ((pModDoc) && (vol >= 0) && (vol <= 64) && (!m_nLockCount))
	{
		if (pModDoc->SetChannelGlobalVolume(nChn, vol))
		{
			m_sbVolume[2].SetPos(vol);
			pModDoc->UpdateAllViews(this, HINT_MODCHANNELS | (m_nActiveTab << 24));
		}
	}
}


void CViewGlobals::OnEditVol4()
//-----------------------------
{
	CModDoc *pModDoc = GetDocument();
	UINT nChn = m_nActiveTab * 4 + 3;
	int vol = GetDlgItemIntEx(IDC_EDIT7);
	if ((pModDoc) && (vol >= 0) && (vol <= 64) && (!m_nLockCount))
	{
		if (pModDoc->SetChannelGlobalVolume(nChn, vol))
		{
			m_sbVolume[3].SetPos(vol);
			pModDoc->UpdateAllViews(this, HINT_MODCHANNELS | (m_nActiveTab << 24));
		}
	}
}


void CViewGlobals::OnEditPan1()
//-----------------------------
{
	CModDoc *pModDoc = GetDocument();
	UINT nChn = m_nActiveTab * 4;
	int pan = GetDlgItemIntEx(IDC_EDIT2);
	if ((pModDoc) && (pan >= 0) && (pan <= 256) && (!m_nLockCount))
	{
		if (pModDoc->SetChannelDefaultPan(nChn, pan))
		{
			m_sbPan[0].SetPos(pan/4);
			pModDoc->UpdateAllViews(this, HINT_MODCHANNELS | (m_nActiveTab << 24));
		}
	}
}


void CViewGlobals::OnEditPan2()
//-----------------------------
{
	CModDoc *pModDoc = GetDocument();
	UINT nChn = m_nActiveTab * 4 + 1;
	int pan = GetDlgItemIntEx(IDC_EDIT4);
	if ((pModDoc) && (pan >= 0) && (pan <= 256) && (!m_nLockCount))
	{
		if (pModDoc->SetChannelDefaultPan(nChn, pan))
		{
			m_sbPan[1].SetPos(pan/4);
			pModDoc->UpdateAllViews(this, HINT_MODCHANNELS | (m_nActiveTab << 24));
		}
	}
}


void CViewGlobals::OnEditPan3()
//-----------------------------
{
	CModDoc *pModDoc = GetDocument();
	UINT nChn = m_nActiveTab * 4 + 2;
	int pan = GetDlgItemIntEx(IDC_EDIT6);
	if ((pModDoc) && (pan >= 0) && (pan <= 256) && (!m_nLockCount))
	{
		if (pModDoc->SetChannelDefaultPan(nChn, pan))
		{
			m_sbPan[2].SetPos(pan/4);
			pModDoc->UpdateAllViews(this, HINT_MODCHANNELS | (m_nActiveTab << 24));
		}
	}
}


void CViewGlobals::OnEditPan4()
//-----------------------------
{
	CModDoc *pModDoc = GetDocument();
	UINT nChn = m_nActiveTab * 4 + 3;
	int pan = GetDlgItemIntEx(IDC_EDIT8);
	if ((pModDoc) && (pan >= 0) && (pan <= 256) && (!m_nLockCount))
	{
		if (pModDoc->SetChannelDefaultPan(nChn, pan))
		{
			m_sbPan[3].SetPos(pan/4);
			pModDoc->UpdateAllViews(this, HINT_MODCHANNELS | (m_nActiveTab << 24));
		}
	}
}


void CViewGlobals::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
//---------------------------------------------------------------------------
{
	CHAR s[64];
	CModDoc *pModDoc;
	UINT nChn;

	CFormView::OnHScroll(nSBCode, nPos, pScrollBar);
	pModDoc = GetDocument();
	nChn = m_nActiveTab * 4;
	if ((pModDoc) && (!IsLocked()) && (nChn < 64))
	{
		BOOL bUpdate = FALSE;
		short int pos;
		
		LockControls();
		for (UINT iCh=0; iCh<4; iCh++)
		{
			// Volume sliders
			pos = (short int)m_sbVolume[iCh].GetPos();
			if ((pos >= 0) && (pos <= 64))
			{
				if (pModDoc->SetChannelGlobalVolume(nChn+iCh, pos))
				{
					SetDlgItemInt(IDC_EDIT1+iCh*2, pos);
					bUpdate = TRUE;
				}
			}
			// Pan sliders
			pos = (short int)m_sbPan[iCh].GetPos();
			if ((pos >= 0) && (pos <= 64) && ((UINT)pos != pModDoc->GetSoundFile()->ChnSettings[nChn+iCh].nPan/4))
			{
				if (pModDoc->SetChannelDefaultPan(nChn+iCh, pos*4))
				{
					SetDlgItemInt(IDC_EDIT2+iCh*2, pos*4);
					bUpdate = TRUE;
				}
			}
		}
		if (bUpdate) pModDoc->UpdateAllViews(this, HINT_MODCHANNELS | (m_nActiveTab << 24));
		UnlockControls();
		if ((pScrollBar) && (pScrollBar->m_hWnd == m_sbValue.m_hWnd))
		{
			int n = (short int)m_sbValue.GetPos();
			if ((n >= 0) && (n <= 100) && (m_nCurrentPlugin < MAX_MIXPLUGINS))
			{
				CSoundFile *pSndFile = pModDoc->GetSoundFile();
				PSNDMIXPLUGIN pPlugin;

				pPlugin = &pSndFile->m_MixPlugins[m_nCurrentPlugin];
				if (pPlugin->pMixPlugin)
				{
					CVstPlugin *pVstPlugin = (CVstPlugin *)pPlugin->pMixPlugin;
					UINT nParams = pVstPlugin->GetNumParameters();
					if (m_nCurrentParam < nParams)
					{
						FLOAT fValue = 0.01f * n;
						wsprintf(s, "%d.%02d", n/100, n%100);
						SetDlgItemText(IDC_EDIT14, s);
						if ((nSBCode == SB_THUMBPOSITION) || (nSBCode == SB_ENDSCROLL))
						{
							pVstPlugin->SetParameter(m_nCurrentParam, fValue);
							OnParamChanged();
						}
					}
				}
			}
		}
	}
}


void CViewGlobals::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
//---------------------------------------------------------------------------
{
	CFormView::OnVScroll(nSBCode, nPos, pScrollBar);
}


void CViewGlobals::OnEditName1()
//------------------------------
{
	CModDoc *pModDoc = GetDocument();

	if ((pModDoc) && (!m_nLockCount))
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		CHAR s[MAX_CHANNELNAME+4];
		UINT nChn = m_nActiveTab * 4;
		
		memset(s, 0, sizeof(s));
		GetDlgItemText(IDC_EDIT9, s, sizeof(s));
		s[MAX_CHANNELNAME+1] = 0;
		if ((pSndFile->m_nType & (MOD_TYPE_XM|MOD_TYPE_IT)) && (nChn < pSndFile->m_nChannels) && (strncmp(s, pSndFile->ChnSettings[nChn].szName, MAX_CHANNELNAME)))
		{
			memcpy(pSndFile->ChnSettings[nChn].szName, s, MAX_CHANNELNAME);
			pModDoc->SetModified();
			pModDoc->UpdateAllViews(this, HINT_MODCHANNELS | (m_nActiveTab << 24));
		}
	}
}


void CViewGlobals::OnEditName2()
//------------------------------
{
	CModDoc *pModDoc = GetDocument();

	if ((pModDoc) && (!m_nLockCount))
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		CHAR s[MAX_CHANNELNAME+4];
		UINT nChn = m_nActiveTab * 4 + 1;
		
		memset(s, 0, sizeof(s));
		GetDlgItemText(IDC_EDIT10, s, sizeof(s));
		s[MAX_CHANNELNAME+1] = 0;
		if ((pSndFile->m_nType & (MOD_TYPE_XM|MOD_TYPE_IT)) && (nChn < pSndFile->m_nChannels) && (strncmp(s, pSndFile->ChnSettings[nChn].szName, MAX_CHANNELNAME)))
		{
			memcpy(pSndFile->ChnSettings[nChn].szName, s, MAX_CHANNELNAME);
			pModDoc->SetModified();
			pModDoc->UpdateAllViews(this, HINT_MODCHANNELS | (m_nActiveTab << 24));
		}
	}
}


void CViewGlobals::OnEditName3()
//------------------------------
{
	CModDoc *pModDoc = GetDocument();

	if ((pModDoc) && (!m_nLockCount))
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		CHAR s[MAX_CHANNELNAME+4];
		UINT nChn = m_nActiveTab * 4 + 2;
		
		memset(s, 0, sizeof(s));
		GetDlgItemText(IDC_EDIT11, s, sizeof(s));
		s[MAX_CHANNELNAME+1] = 0;
		if ((pSndFile->m_nType & (MOD_TYPE_XM|MOD_TYPE_IT)) && (nChn < pSndFile->m_nChannels) && (strncmp(s, pSndFile->ChnSettings[nChn].szName, MAX_CHANNELNAME)))
		{
			memcpy(pSndFile->ChnSettings[nChn].szName, s, MAX_CHANNELNAME);
			pModDoc->SetModified();
			pModDoc->UpdateAllViews(this, HINT_MODCHANNELS | (m_nActiveTab << 24));
		}
	}
}


void CViewGlobals::OnEditName4()
//------------------------------
{
	CModDoc *pModDoc = GetDocument();

	if ((pModDoc) && (!m_nLockCount))
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		CHAR s[MAX_CHANNELNAME+4];
		UINT nChn = m_nActiveTab * 4 + 3;
		
		memset(s, 0, sizeof(s));
		GetDlgItemText(IDC_EDIT12, s, sizeof(s));
		s[MAX_CHANNELNAME+1] = 0;
		if ((pSndFile->m_nType & (MOD_TYPE_XM|MOD_TYPE_IT)) && (nChn < pSndFile->m_nChannels) && (strncmp(s, pSndFile->ChnSettings[nChn].szName, MAX_CHANNELNAME)))
		{
			memcpy(pSndFile->ChnSettings[nChn].szName, s, MAX_CHANNELNAME);
			pModDoc->SetModified();
			pModDoc->UpdateAllViews(this, HINT_MODCHANNELS | (m_nActiveTab << 24));
		}
	}
}


void CViewGlobals::OnFx1Changed()
//-------------------------------
{
	CModDoc *pModDoc = GetDocument();

	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		UINT nChn = m_nActiveTab * 4;
		int nfx = m_CbnEffects[0].GetItemData(m_CbnEffects[0].GetCurSel());
		if ((nfx >= 0) && (nfx <= MAX_MIXPLUGINS) && (nChn < pSndFile->m_nChannels)
		 && (pSndFile->ChnSettings[nChn].nMixPlugin != (UINT)nfx))
		{
			pSndFile->ChnSettings[nChn].nMixPlugin = nfx;
			if (pSndFile->m_nType & (MOD_TYPE_XM|MOD_TYPE_IT)) pModDoc->SetModified();
			pModDoc->UpdateAllViews(this, HINT_MODCHANNELS | (m_nActiveTab << 24));
		}
	}
}


void CViewGlobals::OnFx2Changed()
//-------------------------------
{
	CModDoc *pModDoc = GetDocument();

	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		UINT nChn = m_nActiveTab * 4 + 1;
		int nfx = m_CbnEffects[1].GetItemData(m_CbnEffects[1].GetCurSel());
		if ((nfx >= 0) && (nfx <= MAX_MIXPLUGINS) && (nChn < pSndFile->m_nChannels)
		 && (pSndFile->ChnSettings[nChn].nMixPlugin != (UINT)nfx))
		{
			pSndFile->ChnSettings[nChn].nMixPlugin = nfx;
			if (pSndFile->m_nType & (MOD_TYPE_XM|MOD_TYPE_IT)) pModDoc->SetModified();
			pModDoc->UpdateAllViews(this, HINT_MODCHANNELS | (m_nActiveTab << 24));
		}
	}
}


void CViewGlobals::OnFx3Changed()
//-------------------------------
{
	CModDoc *pModDoc = GetDocument();

	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		UINT nChn = m_nActiveTab * 4 + 2;
		int nfx = m_CbnEffects[2].GetItemData(m_CbnEffects[2].GetCurSel());
		if ((nfx >= 0) && (nfx <= MAX_MIXPLUGINS) && (nChn < pSndFile->m_nChannels)
		 && (pSndFile->ChnSettings[nChn].nMixPlugin != (UINT)nfx))
		{
			pSndFile->ChnSettings[nChn].nMixPlugin = nfx;
			if (pSndFile->m_nType & (MOD_TYPE_XM|MOD_TYPE_IT)) pModDoc->SetModified();
			pModDoc->UpdateAllViews(this, HINT_MODCHANNELS | (m_nActiveTab << 24));
		}
	}
}


void CViewGlobals::OnFx4Changed()
//-------------------------------
{
	CModDoc *pModDoc = GetDocument();

	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		UINT nChn = m_nActiveTab * 4 + 3;
		int nfx = m_CbnEffects[3].GetItemData(m_CbnEffects[3].GetCurSel());
		if ((nfx >= 0) && (nfx <= MAX_MIXPLUGINS) && (nChn < pSndFile->m_nChannels)
		 && (pSndFile->ChnSettings[nChn].nMixPlugin != (UINT)nfx))
		{
			pSndFile->ChnSettings[nChn].nMixPlugin = nfx;
			if (pSndFile->m_nType & (MOD_TYPE_XM|MOD_TYPE_IT)) pModDoc->SetModified();
			pModDoc->UpdateAllViews(this, HINT_MODCHANNELS | (m_nActiveTab << 24));
		}
	}
}


void CViewGlobals::OnPluginNameChanged()
//--------------------------------------
{
	CHAR s[64];
	CModDoc *pModDoc = GetDocument();

	if ((pModDoc) && (m_nCurrentPlugin < MAX_MIXPLUGINS))
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		memset(s, 0, 32);
		GetDlgItemText(IDC_EDIT13, s, 32);
		s[31] = 0;
		if (strcmp(s, pSndFile->m_MixPlugins[m_nCurrentPlugin].Info.szName))
		{
			memcpy(pSndFile->m_MixPlugins[m_nCurrentPlugin].Info.szName, s, 32);
			pModDoc->UpdateAllViews(NULL, HINT_MODCHANNELS | (m_nActiveTab << 24));
		}
	}
}


void CViewGlobals::OnPrevPlugin()
//-------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if ((m_nCurrentPlugin > 0) && (pModDoc))
	{
		m_nCurrentPlugin--;
		pModDoc->UpdateAllViews(NULL, HINT_MIXPLUGINS | HINT_MODCHANNELS | (m_nActiveTab << 24));
	}
}


void CViewGlobals::OnNextPlugin()
//-------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if ((m_nCurrentPlugin < MAX_MIXPLUGINS-1) && (pModDoc))
	{
		m_nCurrentPlugin++;
		pModDoc->UpdateAllViews(NULL, HINT_MIXPLUGINS | HINT_MODCHANNELS | (m_nActiveTab << 24));
	}
}


void CViewGlobals::OnPluginChanged()
//----------------------------------
{
	CModDoc *pModDoc = GetDocument();
	int nPlugin = m_CbnPlugin.GetCurSel();
	if ((pModDoc) && (nPlugin >= 0) && (nPlugin < MAX_MIXPLUGINS))
	{
		m_nCurrentPlugin = nPlugin;
		pModDoc->UpdateAllViews(NULL, HINT_MIXPLUGINS | HINT_MODCHANNELS | (m_nActiveTab << 24));
	}
}


void CViewGlobals::OnSelectPlugin()
//---------------------------------
{
	CModDoc *pModDoc = GetDocument();

	if ((pModDoc) && (m_nCurrentPlugin < MAX_MIXPLUGINS))
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		PSNDMIXPLUGIN pPlugin = &pSndFile->m_MixPlugins[m_nCurrentPlugin];
		CSelectPluginDlg dlg(pPlugin, this);
		if (dlg.DoModal() == IDOK)
		{
			if (pSndFile->m_nType & (MOD_TYPE_XM|MOD_TYPE_IT))
			{
				pModDoc->SetModified();
			}
		}
		OnPluginChanged();
		OnParamChanged();
	}
}


void CViewGlobals::OnParamChanged()
//---------------------------------
{
	int cursel = m_CbnParam.GetCurSel();
	CModDoc *pModDoc = GetDocument();
	CHAR s[256];
	PSNDMIXPLUGIN pPlugin;
	CSoundFile *pSndFile;
	
	if ((m_nCurrentPlugin >= MAX_MIXPLUGINS) || (!pModDoc)) return;
	pSndFile = pModDoc->GetSoundFile();
	pPlugin = &pSndFile->m_MixPlugins[m_nCurrentPlugin];
	if (pPlugin->pMixPlugin)
	{
		CVstPlugin *pVstPlugin = (CVstPlugin *)pPlugin->pMixPlugin;
		UINT nParams = pVstPlugin->GetNumParameters();
		if ((cursel >= 0) && (cursel < (int)nParams)) m_nCurrentParam = cursel;
		if (m_nCurrentParam < nParams)
		{
			CHAR sunits[64], sdisplay[64];
			pVstPlugin->GetParamLabel(m_nCurrentParam, sunits);
			pVstPlugin->GetParamDisplay(m_nCurrentParam, sdisplay);
			wsprintf(s, "Value: %s %s", sdisplay, sunits);
			SetDlgItemText(IDC_TEXT5, s);
			float fValue = pVstPlugin->GetParameter(m_nCurrentParam);
			int nValue = (int)(fValue * 100.0f + 0.5f);
			wsprintf(s, "%d.%02d", nValue/100, nValue%100);
			SetDlgItemText(IDC_EDIT14, s);
			m_sbValue.SetPos(nValue);
			return;
		}
	}
	SetDlgItemText(IDC_TEXT5, "Value:");
	SetDlgItemText(IDC_EDIT14, "");
	m_sbValue.SetPos(0);
}


VOID CViewGlobals::OnSetParameter()
//---------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CHAR s[256];
	PSNDMIXPLUGIN pPlugin;
	CSoundFile *pSndFile;
	
	if ((m_nCurrentPlugin >= MAX_MIXPLUGINS) || (!pModDoc)) return;
	pSndFile = pModDoc->GetSoundFile();
	pPlugin = &pSndFile->m_MixPlugins[m_nCurrentPlugin];
	if (pPlugin->pMixPlugin)
	{
		CVstPlugin *pVstPlugin = (CVstPlugin *)pPlugin->pMixPlugin;
		UINT nParams = pVstPlugin->GetNumParameters();
		GetDlgItemText(IDC_EDIT14, s, sizeof(s));
		if ((m_nCurrentParam < nParams) && (s[0]))
		{
			FLOAT fValue = (FLOAT)atof(s);
			pVstPlugin->SetParameter(m_nCurrentParam, fValue);
			OnParamChanged();
		}
	}
}


VOID CViewGlobals::OnMixModeChanged()
//-----------------------------------
{
	CModDoc *pModDoc = GetDocument();
	PSNDMIXPLUGIN pPlugin;
	CSoundFile *pSndFile;

	if ((m_nCurrentPlugin >= MAX_MIXPLUGINS) || (!pModDoc)) return;
	pSndFile = pModDoc->GetSoundFile();
	pPlugin = &pSndFile->m_MixPlugins[m_nCurrentPlugin];
	if (IsDlgButtonChecked(IDC_CHECK9))
	{
		pPlugin->Info.dwInputRouting |= MIXPLUG_INPUTF_MASTEREFFECT;
	} else
	{
		pPlugin->Info.dwInputRouting &= ~MIXPLUG_INPUTF_MASTEREFFECT;
	}
}


VOID CViewGlobals::OnBypassChanged()
//----------------------------------
{
	CModDoc *pModDoc = GetDocument();
	PSNDMIXPLUGIN pPlugin;
	CSoundFile *pSndFile;

	if ((m_nCurrentPlugin >= MAX_MIXPLUGINS) || (!pModDoc)) return;
	pSndFile = pModDoc->GetSoundFile();
	pPlugin = &pSndFile->m_MixPlugins[m_nCurrentPlugin];
	if (IsDlgButtonChecked(IDC_CHECK10))
	{
		pPlugin->Info.dwInputRouting |= MIXPLUG_INPUTF_BYPASS;
	} else
	{
		pPlugin->Info.dwInputRouting &= ~MIXPLUG_INPUTF_BYPASS;
	}
}


VOID CViewGlobals::OnDryMixChanged()
//----------------------------------
{
	CModDoc *pModDoc = GetDocument();
	PSNDMIXPLUGIN pPlugin;
	CSoundFile *pSndFile;

	if ((m_nCurrentPlugin >= MAX_MIXPLUGINS) || (!pModDoc)) return;
	pSndFile = pModDoc->GetSoundFile();
	pPlugin = &pSndFile->m_MixPlugins[m_nCurrentPlugin];
	if (IsDlgButtonChecked(IDC_CHECK11))
	{
		pPlugin->Info.dwInputRouting |= MIXPLUG_INPUTF_WETMIX;
	} else
	{
		pPlugin->Info.dwInputRouting &= ~MIXPLUG_INPUTF_WETMIX;
	}
}


VOID CViewGlobals::OnEditPlugin()
//-------------------------------
{
	CModDoc *pModDoc = GetDocument();
	PSNDMIXPLUGIN pPlugin;
	CSoundFile *pSndFile;
	
	if ((m_nCurrentPlugin >= MAX_MIXPLUGINS) || (!pModDoc)) return;
	pSndFile = pModDoc->GetSoundFile();
	pPlugin = &pSndFile->m_MixPlugins[m_nCurrentPlugin];
	if (pPlugin->pMixPlugin)
	{
		CVstPlugin *pVstPlugin = (CVstPlugin *)pPlugin->pMixPlugin;
		if (pVstPlugin->HasEditor())
		{
			pVstPlugin->ToggleEditor();
		} else
		{
			UINT nCommands = pVstPlugin->GetNumCommands();
			if (nCommands > 10) nCommands = 10;
			if (nCommands)
			{
				CHAR s[32];
				HMENU hMenu = ::CreatePopupMenu();
				
				if (!hMenu)	return;
				for (UINT i=0; i<nCommands; i++)
				{
					s[0] = 0;
					pVstPlugin->GetCommandName(i, s);
					if (s[0])
					{
						::AppendMenu(hMenu, MF_STRING, ID_FXCOMMANDS_BASE+i, s);
					}
				}
				CPoint pt;
				GetCursorPos(&pt);
				::TrackPopupMenu(hMenu, TPM_LEFTALIGN|TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hWnd, NULL);
				::DestroyMenu(hMenu);
			}
		}
	}
}


VOID CViewGlobals::OnFxCommands(UINT id)
//--------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	PSNDMIXPLUGIN pPlugin;
	CSoundFile *pSndFile;
	UINT nIndex = id - ID_FXCOMMANDS_BASE;

	if ((m_nCurrentPlugin >= MAX_MIXPLUGINS) || (!pModDoc)) return;
	pSndFile = pModDoc->GetSoundFile();
	pPlugin = &pSndFile->m_MixPlugins[m_nCurrentPlugin];
	if (pPlugin->pMixPlugin)
	{
		CVstPlugin *pVstPlugin = (CVstPlugin *)pPlugin->pMixPlugin;
		pVstPlugin->ExecuteCommand(nIndex);
	}
}


VOID CViewGlobals::OnOutputRoutingChanged()
//-----------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	PSNDMIXPLUGIN pPlugin;
	CSoundFile *pSndFile;
	int nroute;

	if ((m_nCurrentPlugin >= MAX_MIXPLUGINS) || (!pModDoc)) return;
	pSndFile = pModDoc->GetSoundFile();
	pPlugin = &pSndFile->m_MixPlugins[m_nCurrentPlugin];
	nroute = m_CbnOutput.GetItemData(m_CbnOutput.GetCurSel());
	pPlugin->Info.dwOutputRouting = nroute;
}



