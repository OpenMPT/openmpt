/*
 * view_gen.cpp
 * ------------
 * Purpose: General tab, lower panel.
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
#include "ctrl_gen.h"
#include "view_gen.h"
#include "vstplug.h"
#include "EffectVis.h"
#include "movefxslotdialog.h"
#include "ChannelManagerDlg.h"
#include "SelectPluginDialog.h"
#include "../common/StringFixer.h"


IMPLEMENT_SERIAL(CViewGlobals, CFormView, 0)

BEGIN_MESSAGE_MAP(CViewGlobals, CFormView)
	//{{AFX_MSG_MAP(CViewGlobals)
	ON_WM_SIZE()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_DESTROY()
	ON_WM_CTLCOLOR()

// -> CODE#0015
// -> DESC="channels management dlg"
	ON_WM_ACTIVATE()
// -! NEW_FEATURE#0015
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
	ON_COMMAND(IDC_MOVEFXSLOT,  OnMovePlugToSlot)
	ON_COMMAND(IDC_INSERTFXSLOT,OnInsertSlot)
	ON_COMMAND(IDC_CLONEPLUG,   OnClonePlug)


// -> CODE#0002
// -> DESC="VST plugins presets"
	ON_COMMAND(IDC_BUTTON6,		OnLoadParam)
	ON_COMMAND(IDC_BUTTON8,		OnSaveParam)
// -! NEW_FEATURE#0002

// -> CODE#0014
// -> DESC="vst wet/dry slider"
	ON_COMMAND(IDC_BUTTON7,		OnSetWetDry)
// -! NEW_FEATURE#0014
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
	ON_CBN_SETFOCUS(IDC_COMBO6, OnFillParamCombo)

	ON_CBN_SELCHANGE(IDC_COMBO7, OnOutputRoutingChanged)

// -> CODE#0002
// -> DESC="VST plugins presets"
	ON_CBN_SELCHANGE(IDC_COMBO8, OnProgramChanged)
	ON_CBN_SETFOCUS(IDC_COMBO8, OnFillProgramCombo)
// -! NEW_FEATURE#0002

// -> CODE#0028
// -> DESC="effect plugin mixing mode combo"
	ON_COMMAND(IDC_CHECK12,		 OnWetDryExpandChanged)
	ON_CBN_SELCHANGE(IDC_COMBO9, OnSpecialMixProcessingChanged)
// -! BEHAVIOUR_CHANGE#0028

	ON_COMMAND_RANGE(ID_FXCOMMANDS_BASE, ID_FXCOMMANDS_BASE+10, OnFxCommands)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TABCTRL1,	OnTabSelchange)
	ON_MESSAGE(WM_MOD_UNLOCKCONTROLS,		OnUnlockControls)
	ON_MESSAGE(WM_MOD_VIEWMSG,	OnModViewMsg)
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

// -> CODE#0002
// -> DESC="VST plugins presets"
	DDX_Control(pDX, IDC_COMBO8,	m_CbnPreset);
// -! NEW_FEATURE#0002

// -> CODE#0028
// -> DESC="effect plugin mixing mode combo"
	DDX_Control(pDX, IDC_COMBO9,	m_CbnSpecialMixProcessing);
	DDX_Control(pDX, IDC_SPIN10,	m_SpinMixGain);					// update#02
// -! BEHAVIOUR_CHANGE#0028

	DDX_Control(pDX, IDC_SLIDER1,	m_sbVolume[0]);
	DDX_Control(pDX, IDC_SLIDER2,	m_sbPan[0]);
	DDX_Control(pDX, IDC_SLIDER3,	m_sbVolume[1]);
	DDX_Control(pDX, IDC_SLIDER4,	m_sbPan[1]);
	DDX_Control(pDX, IDC_SLIDER5,	m_sbVolume[2]);
	DDX_Control(pDX, IDC_SLIDER6,	m_sbPan[2]);
	DDX_Control(pDX, IDC_SLIDER7,	m_sbVolume[3]);
	DDX_Control(pDX, IDC_SLIDER8,	m_sbPan[3]);
	DDX_Control(pDX, IDC_SLIDER9,	m_sbValue);
	DDX_Control(pDX, IDC_SLIDER10,  m_sbDryRatio);	//rewbs.VSTdrywet
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
// -> CODE#0002
// -> DESC="VST plugins presets"
	m_nCurrentPreset = 0;
// -! NEW_FEATURE#0002
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

	m_sbValue.SetPos(0);			// rewbs.dryRatio 20040122
	m_sbValue.SetRange(0, 100);		// rewbs.dryRatio 20040122

// -> CODE#0028
// -> DESC="effect plugin mixing mode combo"
	m_CbnSpecialMixProcessing.AddString("Default");
	m_CbnSpecialMixProcessing.AddString("Wet subtract");
	m_CbnSpecialMixProcessing.AddString("Dry subtract");
	m_CbnSpecialMixProcessing.AddString("Mix subtract");
	m_CbnSpecialMixProcessing.AddString("Middle subtract");
	m_CbnSpecialMixProcessing.AddString("LR balance");
	m_SpinMixGain.SetRange(0,80);		// update#02
	m_SpinMixGain.SetPos(10);			// update#02
	SetDlgItemText(IDC_STATIC2, "Gain: x 1.0");	// update#02
// -! BEHAVIOUR_CHANGE#0028

	UpdateView(HINT_MODTYPE);
	OnParamChanged();
// -> CODE#0014
// -> DESC="vst wet/dry slider"
	//OnWetDryChanged();
// -! NEW_FEATURE#0014
	m_nLockCount = 0;

	
}


void CViewGlobals::OnDestroy()
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


// -> CODE#0015
// -> DESC="channels management dlg"
void CViewGlobals::OnDraw(CDC* pDC)
//---------------------------------
{
	CView::OnDraw(pDC);

	CMainFrame * pMainFrm = CMainFrame::GetMainFrame();
	BOOL activeDoc = pMainFrm ? pMainFrm->GetActiveDoc() == GetDocument() : FALSE;

	if(activeDoc && CChannelManagerDlg::sharedInstance(FALSE) && CChannelManagerDlg::sharedInstance()->IsDisplayed())
		CChannelManagerDlg::sharedInstance()->SetDocument((void*)this);
}
// -! NEW_FEATURE#0015


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
	if ((HintFlagPart(lHint) == HINT_MODCHANNELS) && ((lHint >> HINT_SHIFT_CHNTAB) != m_nActiveTab)) return;
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
			const int lastItem = min(iItem * 4 + 4, MAX_BASECHANNELS);
			wsprintf(s, "%d - %d", iItem * 4 + 1, lastItem);
			tci.mask = TCIF_TEXT | TCIF_PARAM;
			tci.pszText = s;
			tci.lParam = iItem * 4;
			m_TabCtrl.InsertItem(iItem, &tci);
		}
		if (nOldSel >= (UINT)nTabCount) nOldSel = 0;
		
		m_TabCtrl.SetRedraw(TRUE);
		m_TabCtrl.SetCurSel(nOldSel);
		
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
			const UINT nChn = nTabIndex*4+ichn;
			const BOOL bEnable = (nChn < pSndFile->GetNumChannels()) ? TRUE : FALSE;
			if(nChn < MAX_BASECHANNELS)
			{
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

				// Channel name
				memcpy(s, pSndFile->ChnSettings[nChn].szName, MAX_CHANNELNAME);
				s[MAX_CHANNELNAME - 1] = 0;
				SetDlgItemText(IDC_EDIT9 + ichn, s);
				((CEdit*)(GetDlgItem(IDC_EDIT9 + ichn)))->LimitText(MAX_CHANNELNAME - 1);

				// Channel effect
				m_CbnEffects[ichn].SetRedraw(FALSE);
				m_CbnEffects[ichn].ResetContent();
				m_CbnEffects[ichn].SetItemData(m_CbnEffects[ichn].AddString("No plugin"), 0);
				int fxsel = 0;
				for (UINT ifx=0; ifx<MAX_MIXPLUGINS; ifx++)
				{
					if (pSndFile->m_MixPlugins[ifx].IsValidPlugin()
						|| (strcmp(pSndFile->m_MixPlugins[ifx].GetName(), "")
						|| (pSndFile->ChnSettings[nChn].nMixPlugin == ifx + 1)))
					{
						wsprintf(s, "FX%d: %s", ifx + 1, pSndFile->m_MixPlugins[ifx].GetName());
						int n = m_CbnEffects[ichn].AddString(s);
						m_CbnEffects[ichn].SetItemData(n, ifx+1);
						if (pSndFile->ChnSettings[nChn].nMixPlugin == ifx+1) fxsel = n;
					}
				}
				m_CbnEffects[ichn].SetRedraw(TRUE);
				m_CbnEffects[ichn].SetCurSel(fxsel);
			}
			else
				SetDlgItemText(IDC_TEXT1+ichn, "");

			// Enable/Disable controls for this channel
			BOOL bIT = ((bEnable) && (pSndFile->m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT)));
			::EnableWindow(::GetDlgItem(m_hWnd, IDC_CHECK1+ichn*2), bEnable);
			::EnableWindow(::GetDlgItem(m_hWnd, IDC_CHECK2+ichn*2), bIT);

			::EnableWindow(m_sbVolume[ichn].m_hWnd, bIT);
			::EnableWindow(m_spinVolume[ichn], bIT);

			::EnableWindow(m_sbPan[ichn].m_hWnd, bEnable && !(pSndFile->GetType() & (MOD_TYPE_XM|MOD_TYPE_MOD)));
			::EnableWindow(m_spinPan[ichn], bEnable && !(pSndFile->GetType() & (MOD_TYPE_XM|MOD_TYPE_MOD)));
			::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT1 + ichn*2), bIT);	// channel vol
			::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT2 + ichn*2), bEnable && !(pSndFile->GetType() & (MOD_TYPE_XM|MOD_TYPE_MOD)));	// channel pan
			::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT9 + ichn), ((bEnable) && (pSndFile->m_nType & (MOD_TYPE_XM|MOD_TYPE_IT|MOD_TYPE_MPT))));	// channel name
			m_CbnEffects[ichn].EnableWindow(bEnable & (pSndFile->GetModSpecifications().supportsPlugins ? TRUE : FALSE));
		}
		UnlockControls();
	}
	// Update plugins
	if (dwHintMask & (HINT_MIXPLUGINS|HINT_MODTYPE))
	{
		m_CbnPlugin.SetRedraw(FALSE);
		m_CbnPlugin.ResetContent();
		AddPluginNamesToCombobox(m_CbnPlugin, pSndFile->m_MixPlugins, true);
		m_CbnPlugin.SetRedraw(TRUE);
		m_CbnPlugin.SetCurSel(m_nCurrentPlugin);
		if (m_nCurrentPlugin >= MAX_MIXPLUGINS) m_nCurrentPlugin = 0;
		SNDMIXPLUGIN *pPlugin = &(pSndFile->m_MixPlugins[m_nCurrentPlugin]);
		SetDlgItemText(IDC_EDIT13, pPlugin->GetName());
		CheckDlgButton(IDC_CHECK9, pPlugin->IsMasterEffect() ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(IDC_CHECK10, pPlugin->IsBypassed() ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(IDC_CHECK11, pPlugin->IsWetMix() ? BST_CHECKED : BST_UNCHECKED);
		CVstPlugin *pVstPlugin = (pPlugin->pMixPlugin) ? (CVstPlugin *)pPlugin->pMixPlugin : NULL;
		m_BtnEdit.EnableWindow(((pVstPlugin) && ((pVstPlugin->HasEditor()) || (pVstPlugin->GetNumCommands()))) ? TRUE : FALSE);
		::EnableWindow(::GetDlgItem(m_hWnd, IDC_MOVEFXSLOT), (pVstPlugin) ? TRUE : FALSE);
		::EnableWindow(::GetDlgItem(m_hWnd, IDC_INSERTFXSLOT), (pVstPlugin) ? TRUE : FALSE);
		::EnableWindow(::GetDlgItem(m_hWnd, IDC_CLONEPLUG), (pVstPlugin) ? TRUE : FALSE);
		//rewbs.DryRatio
		int n = static_cast<int>(pPlugin->fDryRatio*100);
		wsprintf(s, "(%d%% wet, %d%% dry)", 100-n, n);
		SetDlgItemText(IDC_STATIC8, s);
	m_sbDryRatio.SetPos(n);
		//end rewbs.DryRatio
		
// -> CODE#0028
// -> DESC="effect plugin mixing mode combo"
		if(pVstPlugin && pVstPlugin->isInstrument())
		{
			// ericus 18/02/2005 : disable mix mode for VSTi
			::EnableWindow(::GetDlgItem(m_hWnd, IDC_COMBO9), FALSE);
			::EnableWindow(::GetDlgItem(m_hWnd, IDC_CHECK12), FALSE);
		}
		else{
			::EnableWindow(::GetDlgItem(m_hWnd, IDC_COMBO9), TRUE);
			::EnableWindow(::GetDlgItem(m_hWnd, IDC_CHECK12), TRUE);
			m_CbnSpecialMixProcessing.SetCurSel(pPlugin->GetMixMode()); // update#02 (fix)
			CheckDlgButton(IDC_CHECK12, pPlugin->IsExpandedMix() ? BST_CHECKED : BST_UNCHECKED);
		}
		// update#02
		int gain = pPlugin->GetGain();
		if(gain == 0) gain = 10;
		float value = 0.1f * (float)gain;
		sprintf(s,"Gain: x %1.1f", value);
		SetDlgItemText(IDC_STATIC2, s);
		m_SpinMixGain.SetPos(gain);
// -! BEHAVIOUR_CHANGE#0028

		if (pVstPlugin)
		{
			const PlugParamIndex nParams = pVstPlugin->GetNumParameters();
			m_CbnParam.SetRedraw(FALSE);
			m_CbnParam.ResetContent();
			if (m_nCurrentParam >= nParams) m_nCurrentParam = 0;

			if(nParams)
			{
				m_CbnParam.SetItemData(m_CbnParam.AddString(pVstPlugin->GetFormattedParamName(m_nCurrentParam)), m_nCurrentParam);
			}

			m_CbnParam.SetCurSel(0);
			m_CbnParam.SetRedraw(TRUE);
			m_CbnParam.Invalidate();
			OnParamChanged();

			// Input / Output type
			pVstPlugin->GetPluginType(s);

			// For now, only display the "current" preset.
			// This prevents the program from hanging when switching between plugin slots or
			// switching to the general tab and the first plugin in the list has a lot of presets.
			// Some plugins like Synth1 have so many presets that this *does* indeed make a difference,
			// even on fairly modern CPUs. The rest of the presets are just added when the combo box
			// gets the focus, i.e. just when they're needed.
			m_CbnPreset.SetRedraw(FALSE);
			m_CbnPreset.ResetContent();
			m_CbnPreset.SetItemData(m_CbnPreset.AddString(_T("current")), 0);
			m_CbnPreset.SetRedraw(TRUE);
			m_CbnPreset.SetCurSel(0);

			m_sbValue.EnableWindow(TRUE);
			m_sbDryRatio.EnableWindow(TRUE);
			::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT14), TRUE);
			::EnableWindow(::GetDlgItem(m_hWnd, IDC_BUTTON3), TRUE);

		} else
		{
			s[0] = 0;
			if (m_CbnParam.GetCount() > 0) m_CbnParam.ResetContent();
			m_nCurrentParam = 0;

			CHAR s2[16];
			m_CbnPreset.SetRedraw(FALSE);
			m_CbnPreset.ResetContent();
			wsprintf(s2, "none");
			m_CbnPreset.SetItemData(m_CbnPreset.AddString(s2), 0);
			m_nCurrentPreset = 0;
			m_CbnPreset.SetRedraw(TRUE);
			m_CbnPreset.SetCurSel(0);
			m_sbValue.EnableWindow(FALSE);
			m_sbDryRatio.EnableWindow(FALSE);
			::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT14), FALSE);
			::EnableWindow(::GetDlgItem(m_hWnd, IDC_BUTTON3), FALSE);
		}
		SetDlgItemText(IDC_TEXT6, s);
		int outputsel = 0;
		m_CbnOutput.SetRedraw(FALSE);
		m_CbnOutput.ResetContent();
		m_CbnOutput.SetItemData(m_CbnOutput.AddString("Default"), 0);
		for (PLUGINDEX iOut = m_nCurrentPlugin + 1; iOut < MAX_MIXPLUGINS; iOut++)
		{
			const SNDMIXPLUGIN &plugin = pSndFile->m_MixPlugins[iOut];
			if(plugin.IsValidPlugin())
			{
				if(!strcmp(plugin.GetLibraryName(), plugin.GetName()) || !strcmp(plugin.GetName(), ""))
				{
					wsprintf(s, "FX%d: %s", iOut + 1, plugin.GetLibraryName());
				} else
				{
					wsprintf(s, "FX%d: %s (%s)", iOut + 1, plugin.GetLibraryName(), plugin.GetName());
				}

				int n = m_CbnOutput.AddString(s);
				m_CbnOutput.SetItemData(n, 0x80 + iOut);
				if (!pSndFile->m_MixPlugins[m_nCurrentPlugin].IsOutputToMaster()
					&& (pSndFile->m_MixPlugins[m_nCurrentPlugin].GetOutputPlugin() == iOut))
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

void CViewGlobals::OnMute(const CHANNELINDEX chnMod4, const UINT itemID)
//----------------------------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	
	if (pModDoc)
	{
		const bool b = (IsDlgButtonChecked(itemID) != FALSE);
		const CHANNELINDEX nChn = (CHANNELINDEX)(m_nActiveTab * 4) + chnMod4;
		pModDoc->MuteChannel(nChn, b);
		pModDoc->UpdateAllViews(this, HINT_MODCHANNELS | (m_nActiveTab << HINT_SHIFT_CHNTAB));
	}
}

void CViewGlobals::OnMute1() {OnMute(0, IDC_CHECK1);}
void CViewGlobals::OnMute2() {OnMute(1, IDC_CHECK3);}
void CViewGlobals::OnMute3() {OnMute(2, IDC_CHECK5);}
void CViewGlobals::OnMute4() {OnMute(3, IDC_CHECK7);}


void CViewGlobals::OnSurround(const CHANNELINDEX chnMod4, const UINT itemID)
//--------------------------------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	
	if (pModDoc)
	{
		const bool b = (IsDlgButtonChecked(itemID) != FALSE);
		const CHANNELINDEX nChn = (CHANNELINDEX)(m_nActiveTab * 4) + chnMod4;
		pModDoc->SurroundChannel(nChn, b);
		pModDoc->UpdateAllViews(this, HINT_MODCHANNELS | (m_nActiveTab << HINT_SHIFT_CHNTAB));
		UpdateView(HINT_MODCHANNELS);
	}
}

void CViewGlobals::OnSurround1() {OnSurround(0, IDC_CHECK2);}
void CViewGlobals::OnSurround2() {OnSurround(1, IDC_CHECK4);}
void CViewGlobals::OnSurround3() {OnSurround(2, IDC_CHECK6);}
void CViewGlobals::OnSurround4() {OnSurround(3, IDC_CHECK8);}

void CViewGlobals::OnEditVol(const CHANNELINDEX chnMod4, const UINT itemID)
//-------------------------------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	const CHANNELINDEX nChn = (CHANNELINDEX)(m_nActiveTab * 4) + chnMod4;
	const int vol = GetDlgItemIntEx(itemID);
	if ((pModDoc) && (vol >= 0) && (vol <= 64) && (!m_nLockCount))
	{
		if (pModDoc->SetChannelGlobalVolume(nChn, vol))
		{
			m_sbVolume[chnMod4].SetPos(vol);
			pModDoc->UpdateAllViews(this, HINT_MODCHANNELS | (m_nActiveTab << HINT_SHIFT_CHNTAB));
		}
	}
}

void CViewGlobals::OnEditVol1() {OnEditVol(0, IDC_EDIT1);}
void CViewGlobals::OnEditVol2() {OnEditVol(1, IDC_EDIT3);}
void CViewGlobals::OnEditVol3() {OnEditVol(2, IDC_EDIT5);}
void CViewGlobals::OnEditVol4() {OnEditVol(3, IDC_EDIT7);}


void CViewGlobals::OnEditPan(const CHANNELINDEX chnMod4, const UINT itemID)
//-------------------------------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	const CHANNELINDEX nChn = (CHANNELINDEX)(m_nActiveTab * 4) + chnMod4;
	const int pan = GetDlgItemIntEx(itemID);
	if ((pModDoc) && (pan >= 0) && (pan <= 256) && (!m_nLockCount))
	{
		if (pModDoc->SetChannelDefaultPan(nChn, pan))
		{
			m_sbPan[chnMod4].SetPos(pan/4);
			pModDoc->UpdateAllViews(this, HINT_MODCHANNELS | (m_nActiveTab << HINT_SHIFT_CHNTAB));
			// Surround is forced off when changing pan, so uncheck the checkbox.
			CheckDlgButton(IDC_CHECK2 + chnMod4 * 2, BST_UNCHECKED);
		}
	}
}


void CViewGlobals::OnEditPan1() {OnEditPan(0, IDC_EDIT2);}
void CViewGlobals::OnEditPan2() {OnEditPan(1, IDC_EDIT4);}
void CViewGlobals::OnEditPan3() {OnEditPan(2, IDC_EDIT6);}
void CViewGlobals::OnEditPan4() {OnEditPan(3, IDC_EDIT8);}


void CViewGlobals::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
//---------------------------------------------------------------------------
{
	CHAR s[64];
	CModDoc *pModDoc;
	CHANNELINDEX nChn;

	CFormView::OnHScroll(nSBCode, nPos, pScrollBar);

	pModDoc = GetDocument();
	nChn = (CHANNELINDEX)(m_nActiveTab * 4);
	if ((pModDoc) && (!IsLocked()) && (nChn < MAX_BASECHANNELS))
	{
		BOOL bUpdate = FALSE;
		short int pos;
		
		LockControls();
		const CHANNELINDEX nLoopLimit = min(4, pModDoc->GetSoundFile()->GetNumChannels() - nChn);
		for (CHANNELINDEX iCh = 0; iCh < nLoopLimit; iCh++)
		{
			// Volume sliders
			pos = (short int)m_sbVolume[iCh].GetPos();
			if ((pos >= 0) && (pos <= 64))
			{
				if (pModDoc->SetChannelGlobalVolume(nChn + iCh, pos))
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
					CheckDlgButton(IDC_CHECK2 + iCh * 2, BST_UNCHECKED);
					bUpdate = TRUE;
				}
			}
		}


		//rewbs.dryRatio
		if ((pScrollBar) && (pScrollBar->m_hWnd == m_sbDryRatio.m_hWnd))
		{
			int n = m_sbDryRatio.GetPos();
			if ((n >= 0) && (n <= 100) && (m_nCurrentPlugin < MAX_MIXPLUGINS))
			{				
				CSoundFile *pSndFile = pModDoc->GetSoundFile();
				SNDMIXPLUGIN &plugin = pSndFile->m_MixPlugins[m_nCurrentPlugin];

				if (plugin.pMixPlugin)
				{	
					wsprintf(s, "(%d%% wet, %d%% dry)", 100 - n, n);
					SetDlgItemText(IDC_STATIC8, s);
					plugin.fDryRatio = static_cast<float>(n) / 100.0f;
					if(pSndFile->GetModSpecifications().supportsPlugins)
						pModDoc->SetModified();
				}
			}
		}
		//end rewbs.dryRatio

		if (bUpdate) pModDoc->UpdateAllViews(this, HINT_MODCHANNELS | (m_nActiveTab << HINT_SHIFT_CHNTAB));
		UnlockControls();

		if ((pScrollBar) && (pScrollBar->m_hWnd == m_sbValue.m_hWnd))
		{
			int n = (short int)m_sbValue.GetPos();
			if ((n >= 0) && (n <= 100) && (m_nCurrentPlugin < MAX_MIXPLUGINS))
			{
				CSoundFile *pSndFile = pModDoc->GetSoundFile();
				SNDMIXPLUGIN &plugin = pSndFile->m_MixPlugins[m_nCurrentPlugin];

				if (plugin.pMixPlugin)
				{
					CVstPlugin *pVstPlugin = dynamic_cast<CVstPlugin *>(plugin.pMixPlugin);
					const PlugParamIndex nParams = pVstPlugin->GetNumParameters();
					if (m_nCurrentParam < nParams)
					{
						FLOAT fValue = 0.01f * n;
						wsprintf(s, "%d.%02d", n/100, n%100);
						SetDlgItemText(IDC_EDIT14, s);
						if ((nSBCode == SB_THUMBPOSITION) || (nSBCode == SB_ENDSCROLL))
						{
							pVstPlugin->SetParameter(m_nCurrentParam, fValue);
							OnParamChanged();
							if(pSndFile->GetModSpecifications().supportsPlugins)
								pModDoc->SetModified();
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
// -> CODE#0028	update#02
// -> DESC="effect plugin mixing mode combo"
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile = pModDoc->GetSoundFile();
	CHAR s[32];

	if((m_nCurrentPlugin >= MAX_MIXPLUGINS) || (!pModDoc)) return;

	if(nSBCode != SB_ENDSCROLL && pScrollBar && pScrollBar == (CScrollBar*)&m_SpinMixGain)
	{

		SNDMIXPLUGIN &plugin = pSndFile->m_MixPlugins[m_nCurrentPlugin];

		if(plugin.pMixPlugin)
		{
			DWORD gain = nPos;
			if(gain == 0) gain = 1;

			plugin.SetGain(gain);
			
			float fValue = 0.1f * (float)gain;
			sprintf(s,"Gain: x %1.1f",fValue);
			SetDlgItemText(IDC_STATIC2, s);

			if(pSndFile->GetModSpecifications().supportsPlugins)
				pModDoc->SetModified();
		}
	}
// -! BEHAVIOUR_CHANGE#0028

	CFormView::OnVScroll(nSBCode, nPos, pScrollBar);
}


void CViewGlobals::OnEditName(const CHANNELINDEX chnMod4, const UINT itemID)
//----------------------------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();

	if ((pModDoc) && (!m_nLockCount))
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		CHAR s[MAX_CHANNELNAME+4];
		const UINT nChn = m_nActiveTab * 4 + chnMod4;
		
		memset(s, 0, sizeof(s));
		GetDlgItemText(itemID, s, sizeof(s));
		s[MAX_CHANNELNAME+1] = 0;
		if ((pSndFile->m_nType & (MOD_TYPE_XM|MOD_TYPE_IT|MOD_TYPE_MPT)) && (nChn < pSndFile->m_nChannels) && (strncmp(s, pSndFile->ChnSettings[nChn].szName, MAX_CHANNELNAME)))
		{
			memcpy(pSndFile->ChnSettings[nChn].szName, s, MAX_CHANNELNAME);
			pSndFile->ChnSettings[nChn].szName[CountOf(pSndFile->ChnSettings[nChn].szName)-1] = 0;
			pModDoc->SetModified();
			pModDoc->UpdateAllViews(this, HINT_MODCHANNELS | (m_nActiveTab << HINT_SHIFT_CHNTAB));
		}
	}
}
void CViewGlobals::OnEditName1() {OnEditName(0, IDC_EDIT9);}
void CViewGlobals::OnEditName2() {OnEditName(1, IDC_EDIT10);}
void CViewGlobals::OnEditName3() {OnEditName(2, IDC_EDIT11);}
void CViewGlobals::OnEditName4() {OnEditName(3, IDC_EDIT12);}


void CViewGlobals::OnFxChanged(const CHANNELINDEX chnMod4)
//--------------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();

	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		UINT nChn = m_nActiveTab * 4 + chnMod4;
		int nfx = m_CbnEffects[chnMod4].GetItemData(m_CbnEffects[chnMod4].GetCurSel());
		if ((nfx >= 0) && (nfx <= MAX_MIXPLUGINS) && (nChn < pSndFile->m_nChannels)
		 && (pSndFile->ChnSettings[nChn].nMixPlugin != (UINT)nfx))
		{
			pSndFile->ChnSettings[nChn].nMixPlugin = (PLUGINDEX)nfx;
			if(pSndFile->GetModSpecifications().supportsPlugins)
				pModDoc->SetModified();
			pModDoc->UpdateAllViews(this, HINT_MODCHANNELS | (m_nActiveTab << HINT_SHIFT_CHNTAB));
		}
	}
}


void CViewGlobals::OnFx1Changed() {OnFxChanged(0);}
void CViewGlobals::OnFx2Changed() {OnFxChanged(1);}
void CViewGlobals::OnFx3Changed() {OnFxChanged(2);}
void CViewGlobals::OnFx4Changed() {OnFxChanged(3);}


void CViewGlobals::OnPluginNameChanged()
//--------------------------------------
{
	CHAR s[32];
	CModDoc *pModDoc = GetDocument();

	if ((pModDoc) && (m_nCurrentPlugin < MAX_MIXPLUGINS))
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();

		GetDlgItemText(IDC_EDIT13, s, 32);
		StringFixer::SetNullTerminator(s);
		if (strcmp(s, pSndFile->m_MixPlugins[m_nCurrentPlugin].GetName()))
		{
			lstrcpyn(pSndFile->m_MixPlugins[m_nCurrentPlugin].Info.szName, s, 32);
			if(pSndFile->GetModSpecifications().supportsPlugins)
				pModDoc->SetModified();
			pModDoc->UpdateAllViews(NULL, HINT_MODCHANNELS | (m_nActiveTab << HINT_SHIFT_CHNTAB));
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
		pModDoc->UpdateAllViews(NULL, HINT_MIXPLUGINS | HINT_MODCHANNELS | (m_nActiveTab << HINT_SHIFT_CHNTAB));
// -> CODE#0014
// -> DESC="vst wet/dry slider"
		//OnWetDryChanged();

	}
}


void CViewGlobals::OnNextPlugin()
//-------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if ((m_nCurrentPlugin < MAX_MIXPLUGINS-1) && (pModDoc))
	{
		m_nCurrentPlugin++;
		pModDoc->UpdateAllViews(NULL, HINT_MIXPLUGINS | HINT_MODCHANNELS | (m_nActiveTab << HINT_SHIFT_CHNTAB));
// -> CODE#0014
// -> DESC="vst wet/dry slider"
		//OnWetDryChanged();

	}
}


void CViewGlobals::OnPluginChanged()
//----------------------------------
{
	CModDoc *pModDoc = GetDocument();
	int nPlugin = m_CbnPlugin.GetCurSel();
	if ((pModDoc) && (nPlugin >= 0) && (nPlugin < MAX_MIXPLUGINS))
	{
		m_nCurrentPlugin = (PLUGINDEX)nPlugin;
		pModDoc->UpdateAllViews(NULL, HINT_MIXPLUGINS | HINT_MODCHANNELS | (m_nActiveTab << HINT_SHIFT_CHNTAB));
	}
// -> CODE#0002
// -> DESC="VST plugins presets"
	m_nCurrentPreset = 0;
	m_CbnPreset.SetCurSel(0);
// -! NEW_FEATURE#0002

// -> CODE#0014
// -> DESC="vst wet/dry slider"
	//OnWetDryChanged();
// -! NEW_FEATURE#0014
}


void CViewGlobals::OnSelectPlugin()
//---------------------------------
{
#ifndef NO_VST
	CModDoc *pModDoc = GetDocument();

	if ((pModDoc) && (m_nCurrentPlugin < MAX_MIXPLUGINS))
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		CSelectPluginDlg dlg(pModDoc, m_nCurrentPlugin, this); 
		if (dlg.DoModal() == IDOK)
		{
			if(pSndFile->GetModSpecifications().supportsPlugins)
				pModDoc->SetModified();
		}
		OnPluginChanged();
		OnParamChanged();
// -> CODE#0014
// -> DESC="vst wet/dry slider"
		//OnWetDryChanged();
// -! NEW_FEATURE#0014
	}
#endif // NO_VST
}


void CViewGlobals::OnParamChanged()
//---------------------------------
{
	int cursel = m_CbnParam.GetItemData(m_CbnParam.GetCurSel());
	CModDoc *pModDoc = GetDocument();
	CHAR s[256];
	CSoundFile *pSndFile;
	
	if ((m_nCurrentPlugin >= MAX_MIXPLUGINS) || (!pModDoc)) return;
	pSndFile = pModDoc->GetSoundFile();
	SNDMIXPLUGIN &plugin = pSndFile->m_MixPlugins[m_nCurrentPlugin];

	if (plugin.pMixPlugin && cursel != CB_ERR)
	{
		CVstPlugin *pVstPlugin = dynamic_cast<CVstPlugin *>(plugin.pMixPlugin);
		const PlugParamIndex nParams = pVstPlugin->GetNumParameters();
		if ((cursel >= 0) && (cursel < nParams)) m_nCurrentParam = cursel;
		if (m_nCurrentParam < nParams)
		{
			wsprintf(s, "Value: %s", pVstPlugin->GetFormattedParamValue(m_nCurrentParam));
			SetDlgItemText(IDC_TEXT5, s);
			float fValue = pVstPlugin->GetParameter(m_nCurrentParam);
			int nValue = (int)(fValue * 100.0f + 0.5f);
			sprintf(s, "%f", fValue); //wsprintf(s, "%d.%02d", nValue/100, nValue%100); // ericus 25/01/2005
			SetDlgItemText(IDC_EDIT14, s);
			m_sbValue.SetPos(nValue);
			return;
		}
	}
	SetDlgItemText(IDC_TEXT5, "Value:");
	SetDlgItemText(IDC_EDIT14, "");
	m_sbValue.SetPos(0);
}

// -> CODE#0002
// -> DESC="VST plugins presets"
void CViewGlobals::OnProgramChanged()
//-----------------------------------
{
	int cursel = m_CbnPreset.GetCurSel();
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;
	
	if ((m_nCurrentPlugin >= MAX_MIXPLUGINS) || (!pModDoc)) return;
	pSndFile = pModDoc->GetSoundFile();
	SNDMIXPLUGIN &plugin = pSndFile->m_MixPlugins[m_nCurrentPlugin];

	if (plugin.pMixPlugin)
	{
		CVstPlugin *pVstPlugin = dynamic_cast<CVstPlugin *>(plugin.pMixPlugin);
		UINT nParams = pVstPlugin->GetNumPrograms();
		if ((cursel > 0) && (cursel <= (int)nParams)) m_nCurrentPreset = cursel;
		if (m_nCurrentPreset > 0 && m_nCurrentPreset <= nParams)
		{
			pVstPlugin->SetCurrentProgram(m_nCurrentPreset - 1);
			// Update parameter display
			OnParamChanged();
		}
		if(pSndFile->GetModSpecifications().supportsPlugins)
			pModDoc->SetModified();
	}
}

void CViewGlobals::OnLoadParam()
//------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile = pModDoc ? pModDoc->GetSoundFile() : nullptr;
	if(pSndFile == nullptr)
	{
		return;
	}

	CVstPlugin *pVstPlugin = dynamic_cast<CVstPlugin *>(pSndFile->m_MixPlugins[m_nCurrentPlugin].pMixPlugin);

	if(pVstPlugin == nullptr)
	{
		return;
	}

	FileDlgResult files = CTrackApp::ShowOpenSaveFileDialog(true, "fxp", "",
		"VST FX Program (*.fxp)|*.fxp||",
		CMainFrame::GetSettings().GetDefaultDirectory(DIR_PLUGINPRESETS));
	if(files.abort) return;
    
	//TODO: exception handling
	if (!(pVstPlugin->LoadProgram(files.first_file.c_str())))
	{
		Reporting::Error("Error loading preset. Are you sure it is for this plugin?");
	} else 
	{
		if(pSndFile->GetModSpecifications().supportsPlugins)
			pModDoc->SetModified();
	}

	//end rewbs.fxpPresets
}

void CViewGlobals::OnSaveParam()
//------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile = pModDoc ? pModDoc->GetSoundFile() : nullptr;
	if(pSndFile == nullptr)
	{
		return;
	}

	CVstPlugin *pVstPlugin = dynamic_cast<CVstPlugin *>(pSndFile->m_MixPlugins[m_nCurrentPlugin].pMixPlugin);

	if(pVstPlugin == nullptr)
	{
		return;
	}
	FileDlgResult files = CTrackApp::ShowOpenSaveFileDialog(false, "fxp", "",
		"VST Program (*.fxp)|*.fxp||",
		CMainFrame::GetSettings().GetDefaultDirectory(DIR_PLUGINPRESETS));
	if(files.abort) return;

	//TODO: exception handling
	if (!(pVstPlugin->SaveProgram(files.first_file.c_str())))
		Reporting::Error("Error saving preset.");
	//end rewbs.fxpPresets

}
// -! NEW_FEATURE#0002


void CViewGlobals::OnSetParameter()
//---------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;
	
	if ((m_nCurrentPlugin >= MAX_MIXPLUGINS) || (!pModDoc)) return;
	pSndFile = pModDoc->GetSoundFile();
	SNDMIXPLUGIN &plugin = pSndFile->m_MixPlugins[m_nCurrentPlugin];

	if (plugin.pMixPlugin != nullptr)
	{
		CVstPlugin *pVstPlugin = dynamic_cast<CVstPlugin *>(plugin.pMixPlugin);
		const PlugParamIndex nParams = pVstPlugin->GetNumParameters();
		CHAR s[32];
		GetDlgItemText(IDC_EDIT14, s, sizeof(s));
		if ((m_nCurrentParam < nParams) && (s[0]))
		{
			FLOAT fValue = (FLOAT)atof(s);
			pVstPlugin->SetParameter(m_nCurrentParam, fValue);
            OnParamChanged();
			if(pSndFile->GetModSpecifications().supportsPlugins)
				pModDoc->SetModified();
		}
	}
}


// -> CODE#0014
// -> DESC="vst wet/dry slider"
void CViewGlobals::OnSetWetDry()
//------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;
	
	if ((m_nCurrentPlugin >= MAX_MIXPLUGINS) || (!pModDoc)) return;
	pSndFile = pModDoc->GetSoundFile();
	SNDMIXPLUGIN &plugin = pSndFile->m_MixPlugins[m_nCurrentPlugin];

	if (plugin.pMixPlugin != nullptr)
	{
		//CVstPlugin *pVstPlugin = (CVstPlugin *)pPlugin->pMixPlugin;
		UINT value = GetDlgItemIntEx(IDC_EDIT15);
		plugin.fDryRatio = (float)value / 100.0f;
		if(pSndFile->GetModSpecifications().supportsPlugins)
			pModDoc->SetModified();
		//OnWetDryChanged();
	}
}


void CViewGlobals::OnMixModeChanged()
//-----------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;

	if ((m_nCurrentPlugin >= MAX_MIXPLUGINS) || (!pModDoc)) return;
	pSndFile = pModDoc->GetSoundFile();

	pSndFile->m_MixPlugins[m_nCurrentPlugin].SetMasterEffect(IsDlgButtonChecked(IDC_CHECK9) != BST_UNCHECKED);
	
	if(pSndFile->GetModSpecifications().supportsPlugins)
		pModDoc->SetModified();
}


void CViewGlobals::OnBypassChanged()
//----------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;

	if ((m_nCurrentPlugin >= MAX_MIXPLUGINS) || (!pModDoc)) return;
	pSndFile = pModDoc->GetSoundFile();

	pSndFile->m_MixPlugins[m_nCurrentPlugin].SetBypass(IsDlgButtonChecked(IDC_CHECK10) != BST_UNCHECKED);

	if(pSndFile->GetModSpecifications().supportsPlugins)
		pModDoc->SetModified();
}


// -> CODE#0028
// -> DESC="effect plugin mixing mode combo"
void CViewGlobals::OnWetDryExpandChanged()
//----------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;

	if ((m_nCurrentPlugin >= MAX_MIXPLUGINS) || (!pModDoc)) return;
	pSndFile = pModDoc->GetSoundFile();
	
	pSndFile->m_MixPlugins[m_nCurrentPlugin].SetExpandedMix(IsDlgButtonChecked(IDC_CHECK12) != BST_UNCHECKED);
	
	if(pSndFile->GetModSpecifications().supportsPlugins)
		pModDoc->SetModified();
}


void CViewGlobals::OnSpecialMixProcessingChanged()
//------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;

	if ((m_nCurrentPlugin >= MAX_MIXPLUGINS) || (!pModDoc)) return;
	pSndFile = pModDoc->GetSoundFile();

	pSndFile->m_MixPlugins[m_nCurrentPlugin].SetMixMode(m_CbnSpecialMixProcessing.GetCurSel());	// update#02 (fix)
	if(pSndFile->GetModSpecifications().supportsPlugins)
		pModDoc->SetModified();
}
// -! BEHAVIOUR_CHANGE#0028


void CViewGlobals::OnDryMixChanged()
//----------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;

	if ((m_nCurrentPlugin >= MAX_MIXPLUGINS) || (!pModDoc)) return;
	pSndFile = pModDoc->GetSoundFile();

	pSndFile->m_MixPlugins[m_nCurrentPlugin].SetWetMix(IsDlgButtonChecked(IDC_CHECK11) != BST_UNCHECKED);

	if(pSndFile->GetModSpecifications().supportsPlugins)
		pModDoc->SetModified();
}


void CViewGlobals::OnEditPlugin()
//-------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if ((m_nCurrentPlugin >= MAX_MIXPLUGINS) || (!pModDoc)) return;
	pModDoc->TogglePluginEditor(m_nCurrentPlugin);
	return;	
}


void CViewGlobals::OnFxCommands(UINT id)
//--------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;
	UINT nIndex = id - ID_FXCOMMANDS_BASE;

	if ((m_nCurrentPlugin >= MAX_MIXPLUGINS) || (!pModDoc)) return;
	pSndFile = pModDoc->GetSoundFile();
	SNDMIXPLUGIN &plugin = pSndFile->m_MixPlugins[m_nCurrentPlugin];

	if (plugin.pMixPlugin != nullptr)
	{
		CVstPlugin *pVstPlugin = dynamic_cast<CVstPlugin *>(plugin.pMixPlugin);
		pVstPlugin->ExecuteCommand(nIndex);
		if(pSndFile->GetModSpecifications().supportsPlugins)
			pModDoc->SetModified();
	}
}


void CViewGlobals::OnOutputRoutingChanged()
//-----------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;
	int nroute;

	if ((m_nCurrentPlugin >= MAX_MIXPLUGINS) || (!pModDoc)) return;
	pSndFile = pModDoc->GetSoundFile();
	SNDMIXPLUGIN &plugin = pSndFile->m_MixPlugins[m_nCurrentPlugin];
	nroute = m_CbnOutput.GetItemData(m_CbnOutput.GetCurSel());

	if(!nroute)
		plugin.SetOutputToMaster();
	else
		plugin.SetOutputPlugin(static_cast<PLUGINDEX>(nroute - 0x80));

	if(pSndFile->GetModSpecifications().supportsPlugins)
		pModDoc->SetModified();
}



LRESULT CViewGlobals::OnModViewMsg(WPARAM wParam, LPARAM /*lParam*/)
//-----------------------------------------------------------------
{
	switch(wParam)
	{
		case VIEWMSG_SETFOCUS:
		case VIEWMSG_SETACTIVE:
			GetParentFrame()->SetActiveView(this);
			SetFocus();
			return 0;
		default:
			return 0;
	}
}

void CViewGlobals::OnMovePlugToSlot() 
//-----------------------------------
{
	CMoveFXSlotDialog dlg((CWnd*)this);
	CArray<UINT, UINT> emptySlots;
	BuildEmptySlotList(emptySlots);

	// If any plugin routes its output to the current plugin, we shouldn't try to move it before that plugin...
	PLUGINDEX defaultIndex = 0;
	const CSoundFile *pSndFile = GetDocument() ? (GetDocument()->GetSoundFile()) : nullptr;
	if(pSndFile)
	{
		for(PLUGINDEX i = 0; i < m_nCurrentPlugin; i++)
		{
			if(pSndFile->m_MixPlugins[i].GetOutputPlugin() == m_nCurrentPlugin)
			{
				defaultIndex = i + 1;
			}
		}
	}

	dlg.SetupMove(m_nCurrentPlugin, emptySlots, defaultIndex);

	if (dlg.DoModal() == IDOK)
	{ 
		MovePlug(m_nCurrentPlugin, dlg.m_nToSlot);
		m_CbnPlugin.SetCurSel(dlg.m_nToSlot);
		OnPluginChanged();
	}

}


// Functor for adjusting plug indexes in modcommands. Adjusts all instrument column values in 
// range [m_nInstrMin, m_nInstrMax] by m_nDiff.
struct PlugIndexModifier
//======================
{
	PlugIndexModifier(PLUGINDEX nMin, PLUGINDEX nMax, int nDiff) :
		m_nInstrMin(nMin), m_nInstrMax(nMax), m_nDiff(nDiff) {}
	void operator()(ModCommand& m)
	{
		if (m.IsInstrPlug() && m.instr >= m_nInstrMin && m.instr <= m_nInstrMax)
			m.instr = (ModCommand::INSTR)((int)m.instr + m_nDiff);
	}
	int m_nDiff;
	ModCommand::INSTR m_nInstrMin;
	ModCommand::INSTR m_nInstrMax;
};


bool CViewGlobals::MovePlug(PLUGINDEX src, PLUGINDEX dest, bool bAdjustPat)
//-------------------------------------------------------------------------
{
	if (src == dest)
		return false;
	CModDoc *pModDoc = GetDocument();
	CSoundFile* pSndFile = pModDoc->GetSoundFile();

	BeginWaitCursor();
	
	CriticalSection cs;
		
	// Move plug data
	MemCopy(pSndFile->m_MixPlugins[dest], pSndFile->m_MixPlugins[src]);
	MemsetZero(pSndFile->m_MixPlugins[src]);
	
	//Prevent plug from pointing backwards.
	if(!pSndFile->m_MixPlugins[dest].IsOutputToMaster())
	{
		PLUGINDEX nOutput = pSndFile->m_MixPlugins[dest].GetOutputPlugin();
		if (nOutput <= dest && nOutput != PLUGINDEX_INVALID)
		{
			pSndFile->m_MixPlugins[dest].SetOutputToMaster();
		}
	}
	
	// Update current plug
	if(pSndFile->m_MixPlugins[dest].pMixPlugin)
	{
		((CVstPlugin*)pSndFile->m_MixPlugins[dest].pMixPlugin)->SetSlot(dest);
		((CVstPlugin*)pSndFile->m_MixPlugins[dest].pMixPlugin)->UpdateMixStructPtr(&(pSndFile->m_MixPlugins[dest]));
	}
	
	// Update all other plugs' outputs
	for (PLUGINDEX nPlug = 0; nPlug < src; nPlug++) 
	{
		if(!pSndFile->m_MixPlugins[nPlug].IsOutputToMaster())
		{
			if(pSndFile->m_MixPlugins[nPlug].GetOutputPlugin() == src)
			{
				pSndFile->m_MixPlugins[nPlug].SetOutputPlugin(dest);
			}
		}
	}
	// Update channels
	for (CHANNELINDEX nChn = 0; nChn < pSndFile->GetNumChannels(); nChn++)
	{
		if (pSndFile->ChnSettings[nChn].nMixPlugin == src + 1u)
		{
			pSndFile->ChnSettings[nChn].nMixPlugin = dest + 1u;
		}
	}

	// Update instruments
	for (INSTRUMENTINDEX nIns = 1; nIns <= pSndFile->GetNumInstruments(); nIns++)
	{
		if (pSndFile->Instruments[nIns] && (pSndFile->Instruments[nIns]->nMixPlug == src + 1))
		{
			pSndFile->Instruments[nIns]->nMixPlug = dest + 1u;
		}
	}

	// Update MODCOMMANDs so that they won't be referring to old indexes (e.g. with NOTE_PC).
	if (bAdjustPat && pSndFile->GetType() == MOD_TYPE_MPT)
		pSndFile->Patterns.ForEachModCommand(PlugIndexModifier(src + 1, src + 1, int(dest) - int(src)));

	cs.Leave();

	if(pSndFile->GetModSpecifications().supportsPlugins)
		pModDoc->SetModified();

	EndWaitCursor();

	return true;
}


void CViewGlobals::BuildEmptySlotList(CArray<UINT, UINT> &emptySlots) 
//-------------------------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CSoundFile* pSndFile = pModDoc->GetSoundFile();
	
	emptySlots.RemoveAll();

	for (UINT nSlot=0; nSlot<MAX_MIXPLUGINS; nSlot++)
	{
		if (pSndFile->m_MixPlugins[nSlot].pMixPlugin == nullptr)
		{
			emptySlots.Add(nSlot);	
		}
	}
	return;
}

void CViewGlobals::OnInsertSlot()
//-------------------------------
{
	CString prompt;
	CModDoc *pModDoc = GetDocument();
	CSoundFile* pSndFile = pModDoc->GetSoundFile();
	prompt.Format("Insert empty slot before slot FX%d?", m_nCurrentPlugin + 1);
	if (pSndFile->m_MixPlugins[MAX_MIXPLUGINS-1].pMixPlugin)
	{
		prompt.Append("\nWarning: plugin data in last slot will be lost."); 
	}
	if (Reporting::Confirm(prompt) == cnfYes)
	{

		//Delete last plug...
		if (pSndFile->m_MixPlugins[MAX_MIXPLUGINS-1].pMixPlugin)
		{
			pSndFile->m_MixPlugins[MAX_MIXPLUGINS-1].pMixPlugin->Release();
			memset(&(pSndFile->m_MixPlugins[MAX_MIXPLUGINS-1]), 0, sizeof(SNDMIXPLUGIN));
			//possible mem leak here...
		}

		// Update MODCOMMANDs so that they won't be referring to old indexes (e.g. with NOTE_PC).
		if (pSndFile->GetType() == MOD_TYPE_MPT)
			pSndFile->Patterns.ForEachModCommand(PlugIndexModifier(m_nCurrentPlugin + 1, MAX_MIXPLUGINS - 1, 1));


		for (PLUGINDEX nSlot = MAX_MIXPLUGINS-1; nSlot > m_nCurrentPlugin; nSlot--)
		{
			if (pSndFile->m_MixPlugins[nSlot-1].pMixPlugin)
			{
				MovePlug(nSlot-1, nSlot, NoPatternAdjust);
			}
		}

		m_CbnPlugin.SetCurSel(m_nCurrentPlugin);
		OnPluginChanged();

		if(pSndFile->GetModSpecifications().supportsPlugins)
			pModDoc->SetModified();
	}

}


void CViewGlobals::OnClonePlug()
//------------------------------
{
	//Reporting::Notification("Not yet implemented.");
}


// The plugin param box is only filled when it gets the focus (done here).
void CViewGlobals::OnFillParamCombo()
//-----------------------------------
{
	// no need to fill it again.
	if(m_CbnParam.GetCount() > 1)
		return;

	if(GetDocument() ==  nullptr) return;
	CSoundFile *pSndFile = GetDocument()->GetSoundFile();
	if(pSndFile == nullptr) return;
	if (m_nCurrentPlugin >= MAX_MIXPLUGINS) m_nCurrentPlugin = 0;
	CVstPlugin *pVstPlugin = dynamic_cast<CVstPlugin *>(pSndFile->m_MixPlugins[m_nCurrentPlugin].pMixPlugin);
	if(pVstPlugin == nullptr) return;

	const PlugParamIndex nParams = pVstPlugin->GetNumParameters();
	m_CbnParam.SetRedraw(FALSE);
	m_CbnParam.ResetContent();

	for(PlugParamIndex i = 0; i < nParams; i++)
	{
		m_CbnParam.SetItemData(m_CbnParam.AddString(pVstPlugin->GetFormattedParamName(i)), i);
	}

	if (m_nCurrentParam >= nParams) m_nCurrentParam = 0;
	m_CbnParam.SetCurSel(m_nCurrentParam);
	m_CbnParam.SetRedraw(TRUE);
}


// The preset box is only filled when it gets the focus (done here).
void CViewGlobals::OnFillProgramCombo()
//-------------------------------------
{
	// no need to fill it again.
	if(m_CbnPreset.GetCount() > 1)
		return;

	if(GetDocument() ==  nullptr) return;
	CSoundFile *pSndFile = GetDocument()->GetSoundFile();
	if(pSndFile == nullptr) return;
	if (m_nCurrentPlugin >= MAX_MIXPLUGINS) m_nCurrentPlugin = 0;
	CVstPlugin *pVstPlugin = dynamic_cast<CVstPlugin *>(pSndFile->m_MixPlugins[m_nCurrentPlugin].pMixPlugin);
	if(pVstPlugin == nullptr) return;

	UINT nProg = pVstPlugin->GetNumPrograms(); 
	m_CbnPreset.SetRedraw(FALSE);
	m_CbnPreset.ResetContent();
	m_CbnPreset.SetItemData(m_CbnPreset.AddString(_T("current")), 0);
	for (UINT i = 0; i < nProg; i++)
	{
		m_CbnPreset.SetItemData(m_CbnPreset.AddString(pVstPlugin->GetFormattedProgramName(i)), i + 1);
	}
	m_nCurrentPreset = 0;
	m_CbnPreset.SetRedraw(TRUE);
	m_CbnPreset.SetCurSel(0);
}


// This is used for retrieving the correct background colour for the
// frames on the general tab when using WinXP Luna or Vista/Win7 Aero.
typedef HRESULT (__stdcall * ETDT)(HWND, DWORD);
#include <uxtheme.h>

HBRUSH CViewGlobals::OnCtlColor(CDC *pDC, CWnd* pWnd, UINT nCtlColor)
//-------------------------------------------------------------------
{
	static bool bUxInited = false;
	static ETDT hETDT = NULL;

	if(!bUxInited)
	{
		// retrieve path for uxtheme.dll...
		TCHAR szPath[MAX_PATH];
		SHGetSpecialFolderPath(0, szPath, CSIDL_SYSTEM, FALSE);
		strncat(szPath, _TEXT("\\uxtheme.dll"), MAX_PATH - (_tcslen(szPath) + 1));

		// ...and try to load it
		HMODULE uxlib = LoadLibrary(szPath);
		if(uxlib)
			hETDT = (ETDT)GetProcAddress(uxlib, "EnableThemeDialogTexture");
		bUxInited = true;
	}

	switch(nCtlColor)
	{
	case CTLCOLOR_DLG:
		if(hETDT)
			hETDT(*pWnd, ETDT_ENABLETAB);
	}

	return CFormView::OnCtlColor(pDC, pWnd, nCtlColor);
}
