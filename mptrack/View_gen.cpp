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
#include "Mptrack.h"
#include "Mainfrm.h"
#include "InputHandler.h"
#include "Childfrm.h"
#include "Moddoc.h"
#include "Globals.h"
#include "Ctrl_gen.h"
#include "View_gen.h"
#include "../soundlib/plugins/PlugInterface.h"
#include "EffectVis.h"
#include "MoveFXSlotDialog.h"
#include "ChannelManagerDlg.h"
#include "SelectPluginDialog.h"
#include "../soundlib/mod_specifications.h"
#include "../common/StringFixer.h"
#include "AbstractVstEditor.h"

// This is used for retrieving the correct background colour for the
// frames on the general tab when using WinXP Luna or Vista/Win7 Aero.
#include <uxtheme.h>


OPENMPT_NAMESPACE_BEGIN


IMPLEMENT_SERIAL(CViewGlobals, CFormView, 0)

BEGIN_MESSAGE_MAP(CViewGlobals, CFormView)
	//{{AFX_MSG_MAP(CViewGlobals)
	ON_WM_SIZE()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_DESTROY()
	ON_WM_CTLCOLOR()

	ON_WM_ACTIVATE()
	ON_COMMAND(IDC_CHECK1,		OnMute1)
	ON_COMMAND(IDC_CHECK3,		OnMute2)
	ON_COMMAND(IDC_CHECK5,		OnMute3)
	ON_COMMAND(IDC_CHECK7,		OnMute4)
	ON_COMMAND(IDC_CHECK2,		OnSurround1)
	ON_COMMAND(IDC_CHECK4,		OnSurround2)
	ON_COMMAND(IDC_CHECK6,		OnSurround3)
	ON_COMMAND(IDC_CHECK8,		OnSurround4)

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

	ON_CBN_SELCHANGE(IDC_COMBO1, OnFx1Changed)
	ON_CBN_SELCHANGE(IDC_COMBO2, OnFx2Changed)
	ON_CBN_SELCHANGE(IDC_COMBO3, OnFx3Changed)
	ON_CBN_SELCHANGE(IDC_COMBO4, OnFx4Changed)

	// Plugins
	ON_COMMAND(IDC_CHECK9,		OnMixModeChanged)
	ON_COMMAND(IDC_CHECK10,		OnBypassChanged)
	ON_COMMAND(IDC_CHECK11,		OnDryMixChanged)
	ON_COMMAND(IDC_BUTTON1,		OnSelectPlugin)
	ON_COMMAND(IDC_BUTTON2,		OnEditPlugin)
	ON_COMMAND(IDC_BUTTON4,		OnNextPlugin)
	ON_COMMAND(IDC_BUTTON5,		OnPrevPlugin)
	ON_COMMAND(IDC_MOVEFXSLOT,	OnMovePlugToSlot)
	ON_COMMAND(IDC_INSERTFXSLOT,OnInsertSlot)
	ON_COMMAND(IDC_CLONEPLUG,	OnClonePlug)

	ON_COMMAND(IDC_BUTTON6,		OnLoadParam)
	ON_COMMAND(IDC_BUTTON8,		OnSaveParam)

	ON_COMMAND(IDC_BUTTON7,		OnSetWetDry)
	ON_EN_UPDATE(IDC_EDIT13,	OnPluginNameChanged)
	ON_EN_UPDATE(IDC_EDIT14,	OnSetParameter)
	ON_EN_SETFOCUS(IDC_EDIT14,	OnFocusParam)
	ON_EN_KILLFOCUS(IDC_EDIT14,	OnParamChanged)
	ON_CBN_SELCHANGE(IDC_COMBO5, OnPluginChanged)

	ON_CBN_SELCHANGE(IDC_COMBO6, OnParamChanged)
	ON_CBN_SETFOCUS(IDC_COMBO6, OnFillParamCombo)

	ON_CBN_SELCHANGE(IDC_COMBO7, OnOutputRoutingChanged)

	ON_CBN_SELCHANGE(IDC_COMBO8, OnProgramChanged)
	ON_CBN_SETFOCUS(IDC_COMBO8, OnFillProgramCombo)

	ON_COMMAND(IDC_CHECK12,		 OnWetDryExpandChanged)
	ON_CBN_SELCHANGE(IDC_COMBO9, OnSpecialMixProcessingChanged)

	ON_NOTIFY(TCN_SELCHANGE, IDC_TABCTRL1,	OnTabSelchange)
	ON_MESSAGE(WM_MOD_UNLOCKCONTROLS,		OnUnlockControls)
	ON_MESSAGE(WM_MOD_VIEWMSG,	OnModViewMsg)
	ON_MESSAGE(WM_MOD_MIDIMSG,	OnMidiMsg)
	ON_MESSAGE(WM_MOD_PLUGPARAMAUTOMATE,	OnParamAutomated)
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

	DDX_Control(pDX, IDC_COMBO8,	m_CbnPreset);
	DDX_Control(pDX, IDC_COMBO9,	m_CbnSpecialMixProcessing);
	DDX_Control(pDX, IDC_SPIN10,	m_SpinMixGain);

	DDX_Control(pDX, IDC_SLIDER1,	m_sbVolume[0]);
	DDX_Control(pDX, IDC_SLIDER2,	m_sbPan[0]);
	DDX_Control(pDX, IDC_SLIDER3,	m_sbVolume[1]);
	DDX_Control(pDX, IDC_SLIDER4,	m_sbPan[1]);
	DDX_Control(pDX, IDC_SLIDER5,	m_sbVolume[2]);
	DDX_Control(pDX, IDC_SLIDER6,	m_sbPan[2]);
	DDX_Control(pDX, IDC_SLIDER7,	m_sbVolume[3]);
	DDX_Control(pDX, IDC_SLIDER8,	m_sbPan[3]);
	DDX_Control(pDX, IDC_SLIDER9,	m_sbValue);
	DDX_Control(pDX, IDC_SLIDER10,  m_sbDryRatio);
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

	m_nActiveTab = CHANNELINDEX(-1);
	m_nCurrentPlugin = 0;
	m_nCurrentParam = 0;
	CFormView::OnInitialUpdate();

	if (pFrame)
	{
		GENERALVIEWSTATE &generalState = pFrame->GetGeneralViewState();
		if (generalState.cbStruct == sizeof(GENERALVIEWSTATE))
		{
			m_TabCtrl.SetCurSel(generalState.nTab);
			m_nActiveTab = generalState.nTab;
			m_nCurrentPlugin = generalState.nPlugin;
			m_nCurrentParam = generalState.nParam;
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

	m_sbValue.SetPos(0);
	m_sbValue.SetRange(0, 100);

	m_CbnSpecialMixProcessing.AddString("Default");
	m_CbnSpecialMixProcessing.AddString("Wet subtract");
	m_CbnSpecialMixProcessing.AddString("Dry subtract");
	m_CbnSpecialMixProcessing.AddString("Mix subtract");
	m_CbnSpecialMixProcessing.AddString("Middle subtract");
	m_CbnSpecialMixProcessing.AddString("LR balance");
	m_SpinMixGain.SetRange(0,80);
	m_SpinMixGain.SetPos(10);
	SetDlgItemText(IDC_EDIT16, _T("Gain: x1.0"));

	UpdateView(UpdateHint().ModType());
	OnParamChanged();
	m_nLockCount = 0;


}


void CViewGlobals::OnDestroy()
//----------------------------
{
	CChildFrame *pFrame = (CChildFrame *)GetParentFrame();
	if (pFrame)
	{
		GENERALVIEWSTATE &generalState = pFrame->GetGeneralViewState();
		generalState.cbStruct = sizeof(GENERALVIEWSTATE);
		generalState.nTab = m_nActiveTab;
		generalState.nPlugin = m_nCurrentPlugin;
		generalState.nParam = m_nCurrentParam;
	}
	CFormView::OnDestroy();
}


void CViewGlobals::OnDraw(CDC* pDC)
//---------------------------------
{
	CView::OnDraw(pDC);

	CMainFrame * pMainFrm = CMainFrame::GetMainFrame();
	BOOL activeDoc = pMainFrm ? pMainFrm->GetActiveDoc() == GetDocument() : FALSE;

	if(activeDoc && CChannelManagerDlg::sharedInstance(FALSE) && CChannelManagerDlg::sharedInstance()->IsDisplayed())
		CChannelManagerDlg::sharedInstance()->SetDocument((void*)this);
}


LRESULT CViewGlobals::OnMidiMsg(WPARAM midiData, LPARAM)
//------------------------------------------------------
{
	// Handle MIDI messages assigned to shortcuts
	CInputHandler *ih = CMainFrame::GetMainFrame()->GetInputHandler();
	ih->HandleMIDIMessage(kCtxViewGeneral, midiData) != kcNull
		|| ih->HandleMIDIMessage(kCtxAllContexts, midiData) != kcNull;
	return 1;
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


void CViewGlobals::OnSize(UINT nType, int cx, int cy)
//---------------------------------------------------
{
	CFormView::OnSize(nType, cx, cy);
	if (((nType == SIZE_RESTORED) || (nType == SIZE_MAXIMIZED)) && (cx > 0) && (cy > 0) && (m_hWnd))
	{
		RecalcLayout();
	}
}


void CViewGlobals::OnUpdate(CView *pView, LPARAM lHint, CObject *pHint)
//---------------------------------------------------------------------
{
	if (pView != this) UpdateView(UpdateHint::FromLPARAM(lHint), pHint);
}


void CViewGlobals::UpdateView(UpdateHint hint, CObject *pObject)
//--------------------------------------------------------------
{
	const CModDoc *pModDoc = GetDocument();
	int nTabCount, nTabIndex;

	if (!pModDoc || pObject == this ) return;
	const CSoundFile &sndFile = pModDoc->GetrSoundFile();
	const GeneralHint genHint = hint.ToType<GeneralHint>();
	const PluginHint plugHint = hint.ToType<PluginHint>();
	if (!genHint.GetType()[HINT_MODTYPE | HINT_MODCHANNELS]
		&& !plugHint.GetType()[HINT_MIXPLUGINS | HINT_PLUGINNAMES | HINT_PLUGINPARAM])
	{
		return;
	}
	FlagSet<HintType> hintType = hint.GetType();
	const bool updateAll = hintType[HINT_MODTYPE];
	const int updateTab = genHint.GetChannel() / CHANNELS_IN_TAB;
	if(genHint.GetType()[HINT_MODCHANNELS] && updateTab != m_nActiveTab && updateTab != 0)
	{
		return;
	}

	TCHAR s[128];
	nTabCount = (sndFile.m_nChannels + 3) / 4;
	if (nTabCount != m_TabCtrl.GetItemCount())
	{
		UINT nOldSel = m_TabCtrl.GetCurSel();
		if (!m_TabCtrl.GetItemCount()) nOldSel = m_nActiveTab;
		m_TabCtrl.SetRedraw(FALSE);
		m_TabCtrl.DeleteAllItems();
		for (int iItem=0; iItem<nTabCount; iItem++)
		{
			const int lastItem = MIN(iItem * 4 + 4, MAX_BASECHANNELS);
			wsprintf(s, _T("%d - %d"), iItem * 4 + 1, lastItem);
			TC_ITEM tci;
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

	if (m_nActiveTab != nTabIndex || genHint.GetType()[HINT_MODTYPE | HINT_MODCHANNELS])
	{
		LockControls();
		m_nActiveTab = static_cast<CHANNELINDEX>(nTabIndex);
		for (CHANNELINDEX ichn = 0; ichn < 4; ichn++)
		{
			const CHANNELINDEX nChn = m_nActiveTab * 4 + ichn;
			const BOOL bEnable = (nChn < sndFile.GetNumChannels()) ? TRUE : FALSE;
			if(nChn < MAX_BASECHANNELS)
			{
				// Text
				s[0] = 0;
				if (bEnable) wsprintf(s, _T("Channel %u"), nChn + 1);
				SetDlgItemText(IDC_TEXT1 + ichn, s);
				// Mute
				CheckDlgButton(IDC_CHECK1 + ichn * 2, sndFile.ChnSettings[nChn].dwFlags[CHN_MUTE] ? TRUE : FALSE);
				// Surround
				CheckDlgButton(IDC_CHECK2 + ichn * 2, sndFile.ChnSettings[nChn].dwFlags[CHN_SURROUND] ? TRUE : FALSE);
				// Volume
				int vol = sndFile.ChnSettings[nChn].nVolume;
				m_sbVolume[ichn].SetPos(vol);
				m_sbVolume[ichn].Invalidate(FALSE);
				SetDlgItemInt(IDC_EDIT1+ichn*2, vol);
				// Pan
				int pan = sndFile.ChnSettings[nChn].nPan;
				m_sbPan[ichn].SetPos(pan/4);
				m_sbPan[ichn].Invalidate(FALSE);
				SetDlgItemInt(IDC_EDIT2+ichn*2, pan);

				// Channel name
				mpt::String::Copy(s, sndFile.ChnSettings[nChn].szName);
				SetDlgItemText(IDC_EDIT9 + ichn, s);
				((CEdit*)(GetDlgItem(IDC_EDIT9 + ichn)))->LimitText(MAX_CHANNELNAME - 1);
			}
			else
				SetDlgItemText(IDC_TEXT1+ichn, "");

			// Enable/Disable controls for this channel
			BOOL bIT = ((bEnable) && (sndFile.m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT)));
			GetDlgItem(IDC_CHECK1 + ichn * 2)->EnableWindow(bEnable);
			GetDlgItem(IDC_CHECK2 + ichn * 2)->EnableWindow(bIT);

			m_sbVolume[ichn].EnableWindow(bIT);
			m_spinVolume[ichn].EnableWindow(bIT);

			m_sbPan[ichn].EnableWindow(bEnable && !(sndFile.GetType() & (MOD_TYPE_XM|MOD_TYPE_MOD)));
			m_spinPan[ichn].EnableWindow(bEnable && !(sndFile.GetType() & (MOD_TYPE_XM|MOD_TYPE_MOD)));
			GetDlgItem(IDC_EDIT1 + ichn * 2)->EnableWindow(bIT);	// channel vol
			GetDlgItem(IDC_EDIT2 + ichn * 2)->EnableWindow(bEnable && !(sndFile.GetType() & (MOD_TYPE_XM|MOD_TYPE_MOD)));	// channel pan
			GetDlgItem(IDC_EDIT9 + ichn)->EnableWindow(((bEnable) && (sndFile.m_nType & (MOD_TYPE_XM|MOD_TYPE_IT|MOD_TYPE_MPT))));	// channel name
			m_CbnEffects[ichn].EnableWindow(bEnable & (sndFile.GetModSpecifications().supportsPlugins ? TRUE : FALSE));
		}
		UnlockControls();
	}
	// Update plugin names
	if (genHint.GetType()[HINT_MODTYPE | HINT_MODCHANNELS] || plugHint.GetType()[HINT_PLUGINNAMES])
	{
		PopulateChannelPlugins();
		SetDlgItemText(IDC_EDIT13, sndFile.m_MixPlugins[m_nCurrentPlugin].GetName());
	}
	// Update plugin info
	const bool updatePlug = (plugHint.GetPlugin() == 0 || plugHint.GetPlugin() == m_nCurrentPlugin + 1);
	if (updateAll || (plugHint.GetType()[HINT_MIXPLUGINS] && updatePlug))
	{
		m_CbnPlugin.SetRedraw(FALSE);
		m_CbnPlugin.ResetContent();
		AddPluginNamesToCombobox(m_CbnPlugin, sndFile.m_MixPlugins, true);
		m_CbnPlugin.SetRedraw(TRUE);
		m_CbnPlugin.SetCurSel(m_nCurrentPlugin);
		if (m_nCurrentPlugin >= MAX_MIXPLUGINS) m_nCurrentPlugin = 0;
		const SNDMIXPLUGIN &plugin = sndFile.m_MixPlugins[m_nCurrentPlugin];
		SetDlgItemText(IDC_EDIT13, plugin.GetName());
		CheckDlgButton(IDC_CHECK9, plugin.IsMasterEffect() ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(IDC_CHECK10, plugin.IsBypassed() ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(IDC_CHECK11, plugin.IsWetMix() ? BST_CHECKED : BST_UNCHECKED);
		IMixPlugin *pPlugin = plugin.pMixPlugin;
		m_BtnEdit.EnableWindow((pPlugin != nullptr && (pPlugin->HasEditor() || pPlugin->GetNumParameters())) ? TRUE : FALSE);
		GetDlgItem(IDC_MOVEFXSLOT)->EnableWindow((pPlugin) ? TRUE : FALSE);
		GetDlgItem(IDC_INSERTFXSLOT)->EnableWindow((pPlugin) ? TRUE : FALSE);
		GetDlgItem(IDC_CLONEPLUG)->EnableWindow((pPlugin) ? TRUE : FALSE);
		int n = static_cast<int>(plugin.fDryRatio * 100);
		wsprintf(s, _T("%d%% wet, %d%% dry"), 100 - n, n);
		SetDlgItemText(IDC_STATIC8, s);
		m_sbDryRatio.SetPos(100 - n);

		if(pPlugin && pPlugin->IsInstrument())
		{
			m_CbnSpecialMixProcessing.EnableWindow(FALSE);
			GetDlgItem(IDC_CHECK12)->EnableWindow(FALSE);
		} else
		{
			m_CbnSpecialMixProcessing.EnableWindow(TRUE);
			GetDlgItem(IDC_CHECK12)->EnableWindow(TRUE);
			m_CbnSpecialMixProcessing.SetCurSel(plugin.GetMixMode());
			CheckDlgButton(IDC_CHECK12, plugin.IsExpandedMix() ? BST_CHECKED : BST_UNCHECKED);
		}
		int gain = plugin.GetGain();
		if(gain == 0) gain = 10;
		float value = 0.1f * (float)gain;
		_stprintf(s, _T("Gain: x%1.1f"), value);
		SetDlgItemText(IDC_EDIT16, s);
		m_SpinMixGain.SetPos(gain);

		if (pPlugin)
		{
			const PlugParamIndex nParams = pPlugin->GetNumParameters();
			m_CbnParam.SetRedraw(FALSE);
			m_CbnParam.ResetContent();
			if (m_nCurrentParam >= nParams) m_nCurrentParam = 0;

			if(nParams)
			{
				m_CbnParam.SetItemData(m_CbnParam.AddString(pPlugin->GetFormattedParamName(m_nCurrentParam)), m_nCurrentParam);
			}

			m_CbnParam.SetCurSel(0);
			m_CbnParam.SetRedraw(TRUE);
			OnParamChanged();

			// Input / Output type
			int in = pPlugin->GetNumInputChannels(), out = pPlugin->GetNumOutputChannels();
			if (in < 1) _tcscpy(s, _T("No input"));
			else if (in == 1) _tcscpy(s, _T("Mono-In"));
			else _tcscpy(s, _T("Stereo-In"));
			_tcscat(s, _T(", "));
			if (out < 1) _tcscat(s, _T("No output"));
			else if (out == 1) _tcscat(s, _T("Mono-Out"));
			else _tcscat(s, _T("Stereo-Out"));

			// For now, only display the "current" preset.
			// This prevents the program from hanging when switching between plugin slots or
			// switching to the general tab and the first plugin in the list has a lot of presets.
			// Some plugins like Synth1 have so many presets that this *does* indeed make a difference,
			// even on fairly modern CPUs. The rest of the presets are just added when the combo box
			// gets the focus, i.e. just when they're needed.
			int32 currentProg = pPlugin->GetCurrentProgram();
			FillPluginProgramBox(currentProg, currentProg);
			m_CbnPreset.SetCurSel(0);

			m_sbValue.EnableWindow(TRUE);
			m_sbDryRatio.EnableWindow(TRUE);
			GetDlgItem(IDC_EDIT14)->EnableWindow(TRUE);
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
			m_CbnPreset.SetRedraw(TRUE);
			m_CbnPreset.SetCurSel(0);
			m_sbValue.EnableWindow(FALSE);
			m_sbDryRatio.EnableWindow(FALSE);
			GetDlgItem(IDC_EDIT14)->EnableWindow(FALSE);
		}
		SetDlgItemText(IDC_TEXT6, s);
		int outputsel = 0;
		m_CbnOutput.SetRedraw(FALSE);
		m_CbnOutput.ResetContent();
		m_CbnOutput.SetItemData(m_CbnOutput.AddString("Default"), 0);
		for (PLUGINDEX iOut = m_nCurrentPlugin + 1; iOut < MAX_MIXPLUGINS; iOut++)
		{
			const SNDMIXPLUGIN &plugin = sndFile.m_MixPlugins[iOut];
			if(plugin.IsValidPlugin())
			{
				std::string libName = mpt::ToCharset(mpt::CharsetLocale, mpt::CharsetUTF8, plugin.GetLibraryName());
				if(!strcmp(plugin.GetName(), "") || libName != plugin.GetName())
				{
					wsprintf(s, _T("FX%d: %s"), iOut + 1, libName.c_str());
				} else
				{
					wsprintf(s, _T("FX%d: %s (%s)"), iOut + 1, libName.c_str(), plugin.GetName());
				}

				int n = m_CbnOutput.AddString(s);
				m_CbnOutput.SetItemData(n, 0x80 + iOut);
				if (!sndFile.m_MixPlugins[m_nCurrentPlugin].IsOutputToMaster()
					&& (sndFile.m_MixPlugins[m_nCurrentPlugin].GetOutputPlugin() == iOut))
				{
					outputsel = n;
				}
			}
		}
		m_CbnOutput.SetRedraw(TRUE);
		m_CbnOutput.SetCurSel(outputsel);
	}
	if (plugHint.GetType()[HINT_PLUGINPARAM] && updatePlug)
	{
		OnParamChanged();
	}

	m_CbnPlugin.Invalidate(FALSE);
	m_CbnParam.Invalidate(FALSE);
	m_CbnPreset.Invalidate(FALSE);
	m_CbnSpecialMixProcessing.Invalidate(FALSE);
	m_CbnOutput.Invalidate(FALSE);
}


void CViewGlobals::PopulateChannelPlugins()
//-----------------------------------------
{
	// Channel effect lists
	CSoundFile &sndFile = GetDocument()->GetrSoundFile();
	for(CHANNELINDEX ichn = 0; ichn< 4; ichn++)
	{
		const CHANNELINDEX nChn = m_nActiveTab * 4 + ichn;
		if(nChn < MAX_BASECHANNELS)
		{
			m_CbnEffects[ichn].SetRedraw(FALSE);
			m_CbnEffects[ichn].ResetContent();
			m_CbnEffects[ichn].SetItemData(m_CbnEffects[ichn].AddString("No plugin"), 0);
			int fxsel = 0;
			for (PLUGINDEX ifx = 0; ifx < MAX_MIXPLUGINS; ifx++)
			{
				if (sndFile.m_MixPlugins[ifx].IsValidPlugin()
					|| (strcmp(sndFile.m_MixPlugins[ifx].GetName(), "")
					|| (sndFile.ChnSettings[nChn].nMixPlugin == ifx + 1)))
				{
					CHAR s[128];
					wsprintf(s, "FX%u: %s", ifx + 1, sndFile.m_MixPlugins[ifx].GetName());
					int n = m_CbnEffects[ichn].AddString(s);
					m_CbnEffects[ichn].SetItemData(n, ifx+1);
					if (sndFile.ChnSettings[nChn].nMixPlugin == ifx+1) fxsel = n;
				}
			}
			m_CbnEffects[ichn].SetRedraw(TRUE);
			m_CbnEffects[ichn].SetCurSel(fxsel);
			m_CbnEffects[ichn].Invalidate(FALSE);
		}
	}
}


IMixPlugin *CViewGlobals::GetCurrentPlugin() const
//------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile = pModDoc ? pModDoc->GetSoundFile() : nullptr;
	if(pSndFile == nullptr || m_nCurrentPlugin >= MAX_MIXPLUGINS)
	{
		return nullptr;
	}

	return pSndFile->m_MixPlugins[m_nCurrentPlugin].pMixPlugin;
}


void CViewGlobals::OnTabSelchange(NMHDR*, LRESULT* pResult)
//---------------------------------------------------------
{
	UpdateView(GeneralHint().Channels());
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
		pModDoc->UpdateAllViews(this, GeneralHint(nChn).Channels());
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
		pModDoc->UpdateAllViews(nullptr, GeneralHint(nChn).Channels());
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
		if (pModDoc->SetChannelGlobalVolume(nChn, static_cast<uint16>(vol)))
		{
			m_sbVolume[chnMod4].SetPos(vol);
			pModDoc->UpdateAllViews(this, GeneralHint(nChn).Channels());
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
		if (pModDoc->SetChannelDefaultPan(nChn, static_cast<uint16>(pan)))
		{
			m_sbPan[chnMod4].SetPos(pan / 4);
			pModDoc->UpdateAllViews(this, GeneralHint(nChn).Channels());
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
		const CHANNELINDEX nLoopLimit = std::min<CHANNELINDEX>(4, pModDoc->GetSoundFile()->GetNumChannels() - nChn);
		for (CHANNELINDEX iCh = 0; iCh < nLoopLimit; iCh++)
		{
			if(pScrollBar == (CScrollBar *) &m_sbVolume[iCh])
			{
				// Volume sliders
				pos = (short int)m_sbVolume[iCh].GetPos();
				if ((pos >= 0) && (pos <= 64))
				{
					if (pModDoc->SetChannelGlobalVolume(nChn + iCh, pos))
					{
						SetDlgItemInt(IDC_EDIT1 + iCh * 2, pos);
						bUpdate = TRUE;
					}
				}
			} else if(pScrollBar == (CScrollBar *) &m_sbPan[iCh])
			{
				// Pan sliders
				pos = (short int)m_sbPan[iCh].GetPos();
				if(pos >= 0 && pos <= 64 && (static_cast<uint16>(pos) != pModDoc->GetSoundFile()->ChnSettings[nChn+iCh].nPan / 4u))
				{
					if (pModDoc->SetChannelDefaultPan(nChn + iCh, pos * 4))
					{
						SetDlgItemInt(IDC_EDIT2 + iCh * 2, pos * 4);
						CheckDlgButton(IDC_CHECK2 + iCh * 2, BST_UNCHECKED);
						bUpdate = TRUE;
					}
				}
			}
		}


		if ((pScrollBar) && (pScrollBar->m_hWnd == m_sbDryRatio.m_hWnd))
		{
			int n = 100 - m_sbDryRatio.GetPos();
			if ((n >= 0) && (n <= 100) && (m_nCurrentPlugin < MAX_MIXPLUGINS))
			{
				CSoundFile *pSndFile = pModDoc->GetSoundFile();
				SNDMIXPLUGIN &plugin = pSndFile->m_MixPlugins[m_nCurrentPlugin];

				if(plugin.pMixPlugin)
				{
					wsprintf(s, "%d%% wet, %d%% dry", 100 - n, n);
					SetDlgItemText(IDC_STATIC8, s);
					plugin.fDryRatio = static_cast<float>(n) / 100.0f;
					SetPluginModified();
				}
			}
		}

		if (bUpdate) pModDoc->UpdateAllViews(this, GeneralHint(nChn).Channels());
		UnlockControls();

		if ((pScrollBar) && (pScrollBar->m_hWnd == m_sbValue.m_hWnd))
		{
			int n = (short int)m_sbValue.GetPos();
			if ((n >= 0) && (n <= 100) && (m_nCurrentPlugin < MAX_MIXPLUGINS))
			{
				IMixPlugin *pPlugin = GetCurrentPlugin();
				if(pPlugin != nullptr)
				{
					const PlugParamIndex nParams = pPlugin->GetNumParameters();
					if(m_nCurrentParam < nParams)
					{
						if (nSBCode == SB_THUMBPOSITION || nSBCode == SB_THUMBTRACK || nSBCode == SB_ENDSCROLL)
						{
							pPlugin->SetParameter(m_nCurrentParam, 0.01f * n);
							OnParamChanged();
							SetPluginModified();
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
	CModDoc *pModDoc = GetDocument();

	if((m_nCurrentPlugin >= MAX_MIXPLUGINS) || (!pModDoc)) return;
	CSoundFile &sndFile = pModDoc->GetrSoundFile();
	TCHAR s[32];

	if(nSBCode != SB_ENDSCROLL && pScrollBar && pScrollBar == (CScrollBar*)&m_SpinMixGain)
	{

		SNDMIXPLUGIN &plugin = sndFile.m_MixPlugins[m_nCurrentPlugin];

		if(plugin.pMixPlugin)
		{
			uint8 gain = (uint8)nPos;
			if(gain == 0) gain = 1;

			plugin.SetGain(gain);

			float fValue = 0.1f * (float)gain;
			_stprintf(s, _T("Gain: x%1.1f"), fValue);
			CEdit *gainEdit = (CEdit *)GetDlgItem(IDC_EDIT16);
			gainEdit->SetWindowText(s);

			SetPluginModified();
		}
	}

	CFormView::OnVScroll(nSBCode, nPos, pScrollBar);
}


void CViewGlobals::OnEditName(const CHANNELINDEX chnMod4, const UINT itemID)
//----------------------------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();

	if ((pModDoc) && (!m_nLockCount))
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		CHAR s[MAX_CHANNELNAME + 2];
		const UINT nChn = m_nActiveTab * 4 + chnMod4;

		MemsetZero(s);
		GetDlgItemText(itemID, s, CountOf(s));
		s[MAX_CHANNELNAME+1] = 0;
		if ((pSndFile->GetType() & (MOD_TYPE_XM|MOD_TYPE_IT|MOD_TYPE_MPT)) && (nChn < pSndFile->GetNumChannels()) && (strncmp(s, pSndFile->ChnSettings[nChn].szName, MAX_CHANNELNAME)))
		{
			mpt::String::Copy(pSndFile->ChnSettings[nChn].szName, s);
			pModDoc->SetModified();
			pModDoc->UpdateAllViews(this, GeneralHint(nChn).Channels());
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
		CHANNELINDEX nChn = m_nActiveTab * 4 + chnMod4;
		int nfx = m_CbnEffects[chnMod4].GetItemData(m_CbnEffects[chnMod4].GetCurSel());
		if ((nfx >= 0) && (nfx <= MAX_MIXPLUGINS) && (nChn < pSndFile->GetNumChannels())
		 && (pSndFile->ChnSettings[nChn].nMixPlugin != (UINT)nfx))
		{
			pSndFile->ChnSettings[nChn].nMixPlugin = (PLUGINDEX)nfx;
			if(pSndFile->GetModSpecifications().supportsPlugins)
				pModDoc->SetModified();
			pModDoc->UpdateAllViews(this, GeneralHint(nChn).Channels());
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
		CSoundFile &sndFile = pModDoc->GetrSoundFile();
		SNDMIXPLUGIN &plugin = sndFile.m_MixPlugins[m_nCurrentPlugin];

		GetDlgItemText(IDC_EDIT13, s, CountOf(s));
		mpt::String::SetNullTerminator(s);
		if (strcmp(s, plugin.GetName()))
		{
			mpt::String::Copy(plugin.Info.szName, s);
			if(sndFile.GetModSpecifications().supportsPlugins)
				pModDoc->SetModified();
			pModDoc->UpdateAllViews(this, PluginHint(m_nCurrentPlugin + 1).Info().Names(), this);

			IMixPlugin *pPlugin = plugin.pMixPlugin;
			if(pPlugin != nullptr && pPlugin->GetEditor() != nullptr)
			{
				pPlugin->GetEditor()->SetTitle();
			}
			// Update channel plugin assignments
			PopulateChannelPlugins();
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
		UpdateView(PluginHint(m_nCurrentPlugin + 1).Info());
	}
}


void CViewGlobals::OnNextPlugin()
//-------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if ((m_nCurrentPlugin < MAX_MIXPLUGINS-1) && (pModDoc))
	{
		m_nCurrentPlugin++;
		UpdateView(PluginHint(m_nCurrentPlugin + 1).Info());

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
		UpdateView(PluginHint(m_nCurrentPlugin + 1).Info());
	}
	m_CbnPreset.SetCurSel(0);
}


void CViewGlobals::OnSelectPlugin()
//---------------------------------
{
#ifndef NO_PLUGINS
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
	}
#endif // NO_PLUGINS
}


LRESULT CViewGlobals::OnParamAutomated(WPARAM plugin, LPARAM param)
//-----------------------------------------------------------------
{
	if(plugin == m_nCurrentPlugin && param == m_nCurrentParam)
	{
		OnParamChanged();
	}
	return 0;
}


void CViewGlobals::OnParamChanged()
//---------------------------------
{
	int cursel = m_CbnParam.GetItemData(m_CbnParam.GetCurSel());

	IMixPlugin *pPlugin = GetCurrentPlugin();

	if(pPlugin != nullptr && cursel != CB_ERR)
	{
		const PlugParamIndex nParams = pPlugin->GetNumParameters();
		if(cursel >= 0 && cursel < nParams) m_nCurrentParam = cursel;
		if(m_nCurrentParam < nParams)
		{
			int nValue = Util::Round<int>(pPlugin->GetParameter(m_nCurrentParam) * 100.0f);
			LockControls();
			if(GetFocus() != GetDlgItem(IDC_EDIT14))
			{
				CString s = pPlugin->GetFormattedParamValue(m_nCurrentParam).Trim();
				if(s.IsEmpty())
				{
					s.Format(_T("%f"), pPlugin->GetParameter(m_nCurrentParam));
				}
				SetDlgItemText(IDC_EDIT14, s);
			}
			m_sbValue.SetPos(nValue);
			UnlockControls();
			return;
		}
	}
	SetDlgItemText(IDC_EDIT14, "");
	m_sbValue.SetPos(0);
}


// When focussing the parameter value, show its real value to edit
void CViewGlobals::OnFocusParam()
//-------------------------------
{
	IMixPlugin *pPlugin = GetCurrentPlugin();
	if(pPlugin != nullptr)
	{
		const PlugParamIndex nParams = pPlugin->GetNumParameters();
		if(m_nCurrentParam < nParams)
		{
			TCHAR s[32];
			float fValue = pPlugin->GetParameter(m_nCurrentParam);
			sprintf(s, _T("%f"), fValue);
			LockControls();
			SetDlgItemText(IDC_EDIT14, s);
			UnlockControls();
		}
	}
}


void CViewGlobals::SetPluginModified()
//------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if(pModDoc->GetrSoundFile().GetModSpecifications().supportsPlugins)
		pModDoc->SetModified();
	pModDoc->UpdateAllViews(this, PluginHint(m_nCurrentPlugin + 1).Info());
}


void CViewGlobals::OnProgramChanged()
//-----------------------------------
{
	int32 curProg = m_CbnPreset.GetItemData(m_CbnPreset.GetCurSel());

	IMixPlugin *pPlugin = GetCurrentPlugin();
	if(pPlugin != nullptr)
	{
		const int32 numProgs = pPlugin->GetNumPrograms();
		if(curProg <= numProgs)
		{
			pPlugin->SetCurrentProgram(curProg);
			// Update parameter display
			OnParamChanged();

			SetPluginModified();
		}
	}
}


void CViewGlobals::OnLoadParam()
//------------------------------
{
	IMixPlugin *pPlugin = GetCurrentPlugin();
	if(pPlugin != nullptr && pPlugin->LoadProgram())
	{
		int32 currentProg = pPlugin->GetCurrentProgram();
		FillPluginProgramBox(currentProg, currentProg);
		m_CbnPreset.SetCurSel(0);
		SetPluginModified();
	}
}


void CViewGlobals::OnSaveParam()
//------------------------------
{
	IMixPlugin *pPlugin = GetCurrentPlugin();
	if(pPlugin != nullptr)
	{
		pPlugin->SaveProgram();
	}
}


void CViewGlobals::OnSetParameter()
//---------------------------------
{
	if(m_nCurrentPlugin >= MAX_MIXPLUGINS || IsLocked()) return;
	IMixPlugin *pPlugin = GetCurrentPlugin();

	if(pPlugin != nullptr)
	{
		const PlugParamIndex nParams = pPlugin->GetNumParameters();
		TCHAR s[32];
		GetDlgItemText(IDC_EDIT14, s, CountOf(s));
		if ((m_nCurrentParam < nParams) && (s[0]))
		{
			float fValue = (float)_tstof(s);
			pPlugin->SetParameter(m_nCurrentParam, fValue);
			OnParamChanged();
			SetPluginModified();
		}
	}
}


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
		UINT value = GetDlgItemIntEx(IDC_EDIT15);
		plugin.fDryRatio = (float)value / 100.0f;
		SetPluginModified();
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

	SetPluginModified();
}


void CViewGlobals::OnBypassChanged()
//----------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;

	if ((m_nCurrentPlugin >= MAX_MIXPLUGINS) || (!pModDoc)) return;
	pSndFile = pModDoc->GetSoundFile();

	pSndFile->m_MixPlugins[m_nCurrentPlugin].SetBypass(IsDlgButtonChecked(IDC_CHECK10) != BST_UNCHECKED);

	SetPluginModified();
}


void CViewGlobals::OnWetDryExpandChanged()
//----------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;

	if ((m_nCurrentPlugin >= MAX_MIXPLUGINS) || (!pModDoc)) return;
	pSndFile = pModDoc->GetSoundFile();

	pSndFile->m_MixPlugins[m_nCurrentPlugin].SetExpandedMix(IsDlgButtonChecked(IDC_CHECK12) != BST_UNCHECKED);

	SetPluginModified();
}


void CViewGlobals::OnSpecialMixProcessingChanged()
//------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;

	if ((m_nCurrentPlugin >= MAX_MIXPLUGINS) || (!pModDoc)) return;
	pSndFile = pModDoc->GetSoundFile();

	pSndFile->m_MixPlugins[m_nCurrentPlugin].SetMixMode((uint8)m_CbnSpecialMixProcessing.GetCurSel());
	SetPluginModified();
}


void CViewGlobals::OnDryMixChanged()
//----------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;

	if ((m_nCurrentPlugin >= MAX_MIXPLUGINS) || (!pModDoc)) return;
	pSndFile = pModDoc->GetSoundFile();

	pSndFile->m_MixPlugins[m_nCurrentPlugin].SetWetMix(IsDlgButtonChecked(IDC_CHECK11) != BST_UNCHECKED);

	SetPluginModified();
}


void CViewGlobals::OnEditPlugin()
//-------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if ((m_nCurrentPlugin >= MAX_MIXPLUGINS) || (!pModDoc)) return;
	pModDoc->TogglePluginEditor(m_nCurrentPlugin);
	return;
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

	SetPluginModified();
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
	if(GetCurrentPlugin() == nullptr)
	{
		return;
	}

	// If any plugin routes its output to the current plugin, we shouldn't try to move it before that plugin...
	PLUGINDEX defaultIndex = 0;
	CSoundFile &sndFile = GetDocument()->GetrSoundFile();
	for(PLUGINDEX i = 0; i < m_nCurrentPlugin; i++)
	{
		if(sndFile.m_MixPlugins[i].GetOutputPlugin() == m_nCurrentPlugin)
		{
			defaultIndex = i + 1;
		}
	}

	std::vector<PLUGINDEX> emptySlots;
	BuildEmptySlotList(emptySlots);

	CMoveFXSlotDialog dlg(this, m_nCurrentPlugin, emptySlots, defaultIndex, false, !sndFile.m_MixPlugins[m_nCurrentPlugin].IsOutputToMaster());

	if(dlg.DoModal() == IDOK)
	{
		size_t toIndex = dlg.GetSlotIndex();
		do
		{
			const SNDMIXPLUGIN &curPlugin = sndFile.m_MixPlugins[m_nCurrentPlugin];
			SNDMIXPLUGIN &newPlugin = sndFile.m_MixPlugins[emptySlots[toIndex]];
			const PLUGINDEX nextPlugin = curPlugin.GetOutputPlugin();

			MovePlug(m_nCurrentPlugin, emptySlots[toIndex]);

			if(nextPlugin == PLUGINDEX_INVALID || toIndex == emptySlots.size() - 1)
			{
				break;
			}

			m_nCurrentPlugin = nextPlugin;

			if(dlg.DoMoveChain())
			{
				toIndex++;
				newPlugin.SetOutputPlugin(emptySlots[toIndex]);
			}
		} while(dlg.DoMoveChain());

		m_CbnPlugin.SetCurSel(dlg.GetSlot());
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
	IMixPlugin *pPlugin = pSndFile->m_MixPlugins[dest].pMixPlugin;
	if(pPlugin != nullptr)
	{
		pPlugin->SetSlot(dest);
		pPlugin->UpdateMixStructPtr(&(pSndFile->m_MixPlugins[dest]));
		if(pPlugin->GetEditor() != nullptr)
		{
			pPlugin->GetEditor()->SetTitle();
		}
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
	if (bAdjustPat && pSndFile->GetModSpecifications().HasNote(NOTE_PC))
		pSndFile->Patterns.ForEachModCommand(PlugIndexModifier(src + 1, src + 1, int(dest) - int(src)));

	cs.Leave();

	SetPluginModified();

	EndWaitCursor();

	return true;
}


void CViewGlobals::BuildEmptySlotList(std::vector<PLUGINDEX> &emptySlots)
//-----------------------------------------------------------------------
{
	const CSoundFile &sndFile = GetDocument()->GetrSoundFile();

	emptySlots.clear();

	for(PLUGINDEX nSlot = 0; nSlot < MAX_MIXPLUGINS; nSlot++)
	{
		if(sndFile.m_MixPlugins[nSlot].pMixPlugin == nullptr)
		{
			emptySlots.push_back(nSlot);
		}
	}
	return;
}

void CViewGlobals::OnInsertSlot()
//-------------------------------
{
	CString prompt;
	CModDoc *pModDoc = GetDocument();
	CSoundFile &sndFile = pModDoc->GetrSoundFile();
	prompt.Format("Insert empty slot before slot FX%d?", m_nCurrentPlugin + 1);

	// If last plugin slot is occupied, move it so that the plugin is not lost.
	// This could certainly be improved...
	bool moveLastPlug = false;

	if(sndFile.m_MixPlugins[MAX_MIXPLUGINS - 1].pMixPlugin)
	{
		if(sndFile.m_MixPlugins[MAX_MIXPLUGINS - 2].pMixPlugin == nullptr)
		{
			moveLastPlug = true;
		} else
		{
			prompt.Append("\nWarning: plugin data in last slot will be lost.");
		}
	}
	if(Reporting::Confirm(prompt) == cnfYes)
	{

		// Delete last plug...
		if(sndFile.m_MixPlugins[MAX_MIXPLUGINS - 1].pMixPlugin)
		{
			if(moveLastPlug)
			{
				MovePlug(MAX_MIXPLUGINS - 1, MAX_MIXPLUGINS - 2, true);
			} else
			{
				sndFile.m_MixPlugins[MAX_MIXPLUGINS - 1].Destroy();
				MemsetZero(sndFile.m_MixPlugins[MAX_MIXPLUGINS - 1].Info);
			}
		}

		// Update MODCOMMANDs so that they won't be referring to old indexes (e.g. with NOTE_PC).
		if(sndFile.GetModSpecifications().HasNote(NOTE_PC))
			sndFile.Patterns.ForEachModCommand(PlugIndexModifier(m_nCurrentPlugin + 1, MAX_MIXPLUGINS - 1, 1));


		for(PLUGINDEX nSlot = MAX_MIXPLUGINS - 1; nSlot > m_nCurrentPlugin; nSlot--)
		{
			if(sndFile.m_MixPlugins[nSlot-1].pMixPlugin)
			{
				MovePlug(nSlot - 1, nSlot, NoPatternAdjust);
			}
		}

		m_CbnPlugin.SetCurSel(m_nCurrentPlugin);
		OnPluginChanged();

		SetPluginModified();
	}

}


void CViewGlobals::OnClonePlug()
//------------------------------
{
	if(GetCurrentPlugin() == nullptr)
	{
		return;
	}

	CSoundFile &sndFile = GetDocument()->GetrSoundFile();

	std::vector<PLUGINDEX> emptySlots;
	BuildEmptySlotList(emptySlots);

	CMoveFXSlotDialog dlg(this, m_nCurrentPlugin, emptySlots, 0, true, !sndFile.m_MixPlugins[m_nCurrentPlugin].IsOutputToMaster());

	if(dlg.DoModal() == IDOK)
	{
		size_t toIndex = dlg.GetSlotIndex();
		do
		{
			const SNDMIXPLUGIN &curPlugin = sndFile.m_MixPlugins[m_nCurrentPlugin];
			SNDMIXPLUGIN &newPlugin = sndFile.m_MixPlugins[emptySlots[toIndex]];

			GetDocument()->ClonePlugin(newPlugin, curPlugin);

			if(curPlugin.IsOutputToMaster() || toIndex == emptySlots.size() - 1)
			{
				break;
			}

			m_nCurrentPlugin = curPlugin.GetOutputPlugin();

			if(dlg.DoMoveChain())
			{
				toIndex++;
				newPlugin.SetOutputPlugin(emptySlots[toIndex]);
			}
		} while(dlg.DoMoveChain());

		m_CbnPlugin.SetCurSel(dlg.GetSlot());
		OnPluginChanged();

		SetPluginModified();
	}
}


// The plugin param box is only filled when it gets the focus (done here).
void CViewGlobals::OnFillParamCombo()
//-----------------------------------
{
	// no need to fill it again.
	if(m_CbnParam.GetCount() > 1)
		return;

	IMixPlugin *pPlugin = GetCurrentPlugin();
	if(pPlugin == nullptr) return;

	const PlugParamIndex nParams = pPlugin->GetNumParameters();
	m_CbnParam.SetRedraw(FALSE);
	m_CbnParam.ResetContent();

	pPlugin->CacheParameterNames(0, nParams);
	for(PlugParamIndex i = 0; i < nParams; i++)
	{
		m_CbnParam.SetItemData(m_CbnParam.AddString(pPlugin->GetFormattedParamName(i)), i);
	}

	if (m_nCurrentParam >= nParams) m_nCurrentParam = 0;
	m_CbnParam.SetCurSel(m_nCurrentParam);
	m_CbnParam.SetRedraw(TRUE);
	m_CbnParam.Invalidate(FALSE);
}


// The preset box is only filled when it gets the focus (done here).
void CViewGlobals::OnFillProgramCombo()
//-------------------------------------
{
	// no need to fill it again.
	if(m_CbnPreset.GetCount() > 1)
		return;

	IMixPlugin *pPlugin = GetCurrentPlugin();
	if(pPlugin == nullptr) return;

	FillPluginProgramBox(0, pPlugin->GetNumPrograms() - 1);
	m_CbnPreset.SetCurSel(pPlugin->GetCurrentProgram());
}


void CViewGlobals::FillPluginProgramBox(int32 firstProg, int32 lastProg)
//----------------------------------------------------------------------
{
	IMixPlugin *pPlugin = GetCurrentPlugin();

	m_CbnPreset.SetRedraw(FALSE);
	m_CbnPreset.ResetContent();

	pPlugin->CacheProgramNames(firstProg, lastProg + 1);
	for (int32 i = firstProg; i <= lastProg; i++)
	{
		m_CbnPreset.SetItemData(m_CbnPreset.AddString(pPlugin->GetFormattedProgramName(i)), i);
	}

	m_CbnPreset.SetRedraw(TRUE);
	m_CbnPreset.Invalidate(FALSE);
}


// This is used for retrieving the correct background colour for the
// frames on the general tab when using WinXP Luna or Vista/Win7 Aero.
HBRUSH CViewGlobals::OnCtlColor(CDC *pDC, CWnd* pWnd, UINT nCtlColor)
//-------------------------------------------------------------------
{
	switch(nCtlColor)
	{
	case CTLCOLOR_DLG:
		{
			ComponentHandle<ComponentUXTheme> uxtheme;
			if(IsComponentAvailable(uxtheme))
			{
				EnableThemeDialogTexture(*pWnd, ETDT_ENABLETAB);
			}
		}
		break;
	}
	return CFormView::OnCtlColor(pDC, pWnd, nCtlColor);
}


OPENMPT_NAMESPACE_END
