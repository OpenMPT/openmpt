/*
 * View_gen.cpp
 * ------------
 * Purpose: General tab, lower panel.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "View_gen.h"
#include "AbstractVstEditor.h"
#include "Childfrm.h"
#include "Ctrl_gen.h"
#include "EffectVis.h"
#include "Globals.h"
#include "InputHandler.h"
#include "Mainfrm.h"
#include "Moddoc.h"
#include "MoveFXSlotDialog.h"
#include "Reporting.h"
#include "resource.h"
#include "SelectPluginDialog.h"
#include "WindowMessages.h"
#include "../common/mptStringBuffer.h"
#include "../soundlib/mod_specifications.h"
#include "../soundlib/plugins/PlugInterface.h"

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
	
	ON_MESSAGE(WM_MOD_MDIACTIVATE,   &CViewGlobals::OnMDIDeactivate)
	ON_MESSAGE(WM_MOD_MDIDEACTIVATE, &CViewGlobals::OnMDIDeactivate)

	ON_COMMAND(IDC_CHECK1,		&CViewGlobals::OnMute1)
	ON_COMMAND(IDC_CHECK3,		&CViewGlobals::OnMute2)
	ON_COMMAND(IDC_CHECK5,		&CViewGlobals::OnMute3)
	ON_COMMAND(IDC_CHECK7,		&CViewGlobals::OnMute4)
	ON_COMMAND(IDC_CHECK2,		&CViewGlobals::OnSurround1)
	ON_COMMAND(IDC_CHECK4,		&CViewGlobals::OnSurround2)
	ON_COMMAND(IDC_CHECK6,		&CViewGlobals::OnSurround3)
	ON_COMMAND(IDC_CHECK8,		&CViewGlobals::OnSurround4)

	ON_COMMAND(IDC_BUTTON9,		&CViewGlobals::OnEditColor1)
	ON_COMMAND(IDC_BUTTON10,	&CViewGlobals::OnEditColor2)
	ON_COMMAND(IDC_BUTTON11,	&CViewGlobals::OnEditColor3)
	ON_COMMAND(IDC_BUTTON12,	&CViewGlobals::OnEditColor4)

	ON_EN_UPDATE(IDC_EDIT1,		&CViewGlobals::OnEditVol1)
	ON_EN_UPDATE(IDC_EDIT3,		&CViewGlobals::OnEditVol2)
	ON_EN_UPDATE(IDC_EDIT5,		&CViewGlobals::OnEditVol3)
	ON_EN_UPDATE(IDC_EDIT7,		&CViewGlobals::OnEditVol4)
	ON_EN_UPDATE(IDC_EDIT2,		&CViewGlobals::OnEditPan1)
	ON_EN_UPDATE(IDC_EDIT4,		&CViewGlobals::OnEditPan2)
	ON_EN_UPDATE(IDC_EDIT6,		&CViewGlobals::OnEditPan3)
	ON_EN_UPDATE(IDC_EDIT8,		&CViewGlobals::OnEditPan4)
	ON_EN_UPDATE(IDC_EDIT9,		&CViewGlobals::OnEditName1)
	ON_EN_UPDATE(IDC_EDIT10,	&CViewGlobals::OnEditName2)
	ON_EN_UPDATE(IDC_EDIT11,	&CViewGlobals::OnEditName3)
	ON_EN_UPDATE(IDC_EDIT12,	&CViewGlobals::OnEditName4)

	ON_CBN_SELCHANGE(IDC_COMBO1, &CViewGlobals::OnFx1Changed)
	ON_CBN_SELCHANGE(IDC_COMBO2, &CViewGlobals::OnFx2Changed)
	ON_CBN_SELCHANGE(IDC_COMBO3, &CViewGlobals::OnFx3Changed)
	ON_CBN_SELCHANGE(IDC_COMBO4, &CViewGlobals::OnFx4Changed)

	// Plugins
	ON_COMMAND(IDC_CHECK9,		&CViewGlobals::OnMixModeChanged)
	ON_COMMAND(IDC_CHECK10,		&CViewGlobals::OnBypassChanged)
	ON_COMMAND(IDC_CHECK11,		&CViewGlobals::OnDryMixChanged)
	ON_COMMAND(IDC_BUTTON1,		&CViewGlobals::OnSelectPlugin)
	ON_COMMAND(IDC_DELPLUGIN,	&CViewGlobals::OnRemovePlugin)
	ON_COMMAND(IDC_BUTTON2,		&CViewGlobals::OnEditPlugin)
	ON_COMMAND(IDC_BUTTON4,		&CViewGlobals::OnNextPlugin)
	ON_COMMAND(IDC_BUTTON5,		&CViewGlobals::OnPrevPlugin)
	ON_COMMAND(IDC_MOVEFXSLOT,	&CViewGlobals::OnMovePlugToSlot)
	ON_COMMAND(IDC_INSERTFXSLOT,&CViewGlobals::OnInsertSlot)
	ON_COMMAND(IDC_CLONEPLUG,	&CViewGlobals::OnClonePlug)

	ON_COMMAND(IDC_BUTTON6,		&CViewGlobals::OnLoadParam)
	ON_COMMAND(IDC_BUTTON8,		&CViewGlobals::OnSaveParam)

	ON_EN_UPDATE(IDC_EDIT13,	&CViewGlobals::OnPluginNameChanged)
	ON_EN_UPDATE(IDC_EDIT14,	&CViewGlobals::OnSetParameter)
	ON_EN_SETFOCUS(IDC_EDIT14,	&CViewGlobals::OnFocusParam)
	ON_EN_KILLFOCUS(IDC_EDIT14,	&CViewGlobals::OnParamChanged)
	ON_CBN_SELCHANGE(IDC_COMBO5, &CViewGlobals::OnPluginChanged)

	ON_CBN_SELCHANGE(IDC_COMBO6, &CViewGlobals::OnParamChanged)
	ON_CBN_SETFOCUS(IDC_COMBO6, &CViewGlobals::OnFillParamCombo)

	ON_CBN_SELCHANGE(IDC_COMBO7, &CViewGlobals::OnOutputRoutingChanged)

	ON_CBN_SELCHANGE(IDC_COMBO8, &CViewGlobals::OnProgramChanged)
	ON_CBN_SETFOCUS(IDC_COMBO8, &CViewGlobals::OnFillProgramCombo)

	ON_COMMAND(IDC_CHECK12, &CViewGlobals::OnWetDryExpandChanged)
	ON_COMMAND(IDC_CHECK13, &CViewGlobals::OnAutoSuspendChanged)
	ON_CBN_SELCHANGE(IDC_COMBO9, &CViewGlobals::OnSpecialMixProcessingChanged)

	ON_NOTIFY(TCN_SELCHANGE, IDC_TABCTRL1,	&CViewGlobals::OnTabSelchange)
	ON_MESSAGE(WM_MOD_UNLOCKCONTROLS,		&CViewGlobals::OnUnlockControls)
	ON_MESSAGE(WM_MOD_VIEWMSG,	&CViewGlobals::OnModViewMsg)
	ON_MESSAGE(WM_MOD_MIDIMSG,	&CViewGlobals::OnMidiMsg)
	ON_MESSAGE(WM_MOD_PLUGPARAMAUTOMATE,	&CViewGlobals::OnParamAutomated)
	ON_MESSAGE(WM_MOD_PLUGINDRYWETRATIOCHANGED, &CViewGlobals::OnDryWetRatioChangedFromPlayer)

	ON_COMMAND(ID_EDIT_UNDO, &CViewGlobals::OnEditUndo)
	ON_COMMAND(ID_EDIT_REDO, &CViewGlobals::OnEditRedo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, &CViewGlobals::OnUpdateUndo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, &CViewGlobals::OnUpdateRedo)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXT, 0, 0xFFFF, &CViewGlobals::OnToolTipText)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CViewGlobals::DoDataExchange(CDataExchange* pDX)
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
	DDX_Control(pDX, IDC_BUTTON4, m_nextPluginButton);
	DDX_Control(pDX, IDC_BUTTON5, m_prevPluginButton);
	//}}AFX_DATA_MAP
}


CViewGlobals::CViewGlobals() : CFormView{IDD_VIEW_GLOBALS}
{
	m_prevPluginButton.SetAccessibleText(_T("Previous Plugin"));
	m_nextPluginButton.SetAccessibleText(_T("Next Plugin"));
}


CModDoc* CViewGlobals::GetDocument() const { return static_cast<CModDoc *>(m_pDocument); }

void CViewGlobals::OnInitialUpdate()
{
	CChildFrame *pFrame = (CChildFrame *)GetParentFrame();
	int nMapMode = MM_TEXT;
	SIZE sizeTotal, sizePage, sizeLine;

	m_nActiveTab = CHANNELINDEX(-1);
	m_nCurrentPlugin = 0;
	m_nCurrentParam = 0;
	CFormView::OnInitialUpdate();
	EnableToolTips();

	if (pFrame)
	{
		GeneralViewState &generalState = pFrame->GetGeneralViewState();
		if (generalState.initialized)
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
	for(int ichn = 0; ichn < CHANNELS_IN_TAB; ichn++)
	{
		// Color select
		m_channelColor[ichn].SubclassDlgItem(IDC_BUTTON9 + ichn, this);
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

	m_CbnSpecialMixProcessing.AddString(_T("Default"));
	m_CbnSpecialMixProcessing.AddString(_T("Wet Subtract"));
	m_CbnSpecialMixProcessing.AddString(_T("Dry Subtract"));
	m_CbnSpecialMixProcessing.AddString(_T("Mix Subtract"));
	m_CbnSpecialMixProcessing.AddString(_T("Middle Subtract"));
	m_CbnSpecialMixProcessing.AddString(_T("LR Balance"));
	m_CbnSpecialMixProcessing.AddString(_T("Instrument"));
	m_SpinMixGain.SetRange(0, 80);
	m_SpinMixGain.SetPos(10);
	SetDlgItemText(IDC_EDIT16, _T("Gain: x1.0"));

	UpdateView(UpdateHint().ModType());
	OnParamChanged();
	m_nLockCount = 0;

	// Use tab background color rather than regular dialog background color (required for Aero/etc. where they are not the same color)
	EnableThemeDialogTexture(m_hWnd, ETDT_ENABLETAB);
}


void CViewGlobals::OnDestroy()
{
	CChildFrame *pFrame = (CChildFrame *)GetParentFrame();
	if (pFrame)
	{
		GeneralViewState &generalState = pFrame->GetGeneralViewState();
		generalState.initialized = true;
		generalState.nTab = m_nActiveTab;
		generalState.nPlugin = m_nCurrentPlugin;
		generalState.nParam = m_nCurrentParam;
	}
	CFormView::OnDestroy();
}


LRESULT CViewGlobals::OnMDIDeactivate(WPARAM, LPARAM)
{
	// Create new undo point if we switch to / from other window
	m_lastEdit = CHANNELINDEX_INVALID;
	return 0;
}


LRESULT CViewGlobals::OnMidiMsg(WPARAM midiData_, LPARAM)
{
	uint32 midiData = static_cast<uint32>(midiData_);
	// Handle MIDI messages assigned to shortcuts
	CInputHandler *ih = CMainFrame::GetInputHandler();
	if(ih->HandleMIDIMessage(kCtxViewGeneral, midiData) == kcNull)
		ih->HandleMIDIMessage(kCtxAllContexts, midiData);
	return 1;
}


void CViewGlobals::RecalcLayout()
{
	if (m_TabCtrl.m_hWnd != NULL)
	{
		CRect rect;
		GetClientRect(&rect);
		if (rect.right < m_rcClient.right) rect.right = m_rcClient.right;
		if (rect.bottom < m_rcClient.bottom) rect.bottom = m_rcClient.bottom;
		m_TabCtrl.SetWindowPos(&CWnd::wndBottom, 0,0, rect.right, rect.bottom, SWP_NOMOVE);
	}
}


void CViewGlobals::UnlockControls() { PostMessage(WM_MOD_UNLOCKCONTROLS); }


int CViewGlobals::GetDlgItemIntEx(UINT nID)
{
	CString s;
	GetDlgItemText(nID, s);
	if(s.GetLength() < 1 || s[0] < _T('0') || s[0] > _T('9')) return -1;
	return _ttoi(s);
}


void CViewGlobals::OnSize(UINT nType, int cx, int cy)
{
	CFormView::OnSize(nType, cx, cy);
	if (((nType == SIZE_RESTORED) || (nType == SIZE_MAXIMIZED)) && (cx > 0) && (cy > 0) && (m_hWnd))
	{
		RecalcLayout();
	}
}


void CViewGlobals::OnUpdate(CView *pView, LPARAM lHint, CObject *pHint)
{
	if (pView != this) UpdateView(UpdateHint::FromLPARAM(lHint), pHint);
}


void CViewGlobals::UpdateView(UpdateHint hint, CObject *pObject)
{
	const CModDoc *pModDoc = GetDocument();
	int nTabCount, nTabIndex;

	if(!pModDoc || pObject == this)
		return;
	const CSoundFile &sndFile = pModDoc->GetSoundFile();
	const GeneralHint genHint = hint.ToType<GeneralHint>();
	const PluginHint plugHint = hint.ToType<PluginHint>();
	if(!genHint.GetType()[HINT_MODTYPE | HINT_MODCHANNELS]
		&& !plugHint.GetType()[HINT_MIXPLUGINS | HINT_PLUGINNAMES | HINT_PLUGINPARAM])
	{
		return;
	}
	FlagSet<HintType> hintType = hint.GetType();
	const bool updateAll = hintType[HINT_MODTYPE];
	const auto updateChannel = genHint.GetChannel();
	const int updateTab = (updateChannel < sndFile.GetNumChannels()) ? (updateChannel / CHANNELS_IN_TAB) : m_nActiveTab;
	if(genHint.GetType()[HINT_MODCHANNELS] && updateTab != m_nActiveTab)
		return;

	CString s;
	nTabCount = (sndFile.GetNumChannels() + (CHANNELS_IN_TAB - 1)) / CHANNELS_IN_TAB;
	if (nTabCount != m_TabCtrl.GetItemCount())
	{
		UINT nOldSel = m_TabCtrl.GetCurSel();
		if (!m_TabCtrl.GetItemCount()) nOldSel = m_nActiveTab;
		m_TabCtrl.SetRedraw(FALSE);
		m_TabCtrl.DeleteAllItems();
		for (int iItem=0; iItem<nTabCount; iItem++)
		{
			const int lastItem = std::min(iItem * CHANNELS_IN_TAB + CHANNELS_IN_TAB, static_cast<int>(MAX_BASECHANNELS));
			s = MPT_CFORMAT("{} - {}")(iItem * CHANNELS_IN_TAB + 1, lastItem);
			TC_ITEM tci;
			tci.mask = TCIF_TEXT | TCIF_PARAM;
			tci.pszText = const_cast<TCHAR *>(s.GetString());
			tci.lParam = iItem * CHANNELS_IN_TAB;
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
		for (CHANNELINDEX ichn = 0; ichn < CHANNELS_IN_TAB; ichn++)
		{
			const CHANNELINDEX nChn = m_nActiveTab * CHANNELS_IN_TAB + ichn;
			const BOOL bEnable = (nChn < sndFile.GetNumChannels()) ? TRUE : FALSE;
			if(nChn < sndFile.GetNumChannels())
			{
				const auto &chnSettings = sndFile.ChnSettings[nChn];
				// Text
				if(bEnable)
					s = MPT_CFORMAT("Channel {}")(nChn + 1);
				else
					s = _T("");
				SetDlgItemText(IDC_TEXT1 + ichn, s);
				// Channel color
				m_channelColor[ichn].SetColor(chnSettings.color);
				m_channelColor[ichn].EnableWindow(bEnable);
				// Mute
				CheckDlgButton(IDC_CHECK1 + ichn * 2, chnSettings.dwFlags[CHN_MUTE] ? TRUE : FALSE);
				// Surround
				CheckDlgButton(IDC_CHECK2 + ichn * 2, chnSettings.dwFlags[CHN_SURROUND] ? TRUE : FALSE);
				// Volume
				int vol = chnSettings.nVolume;
				m_sbVolume[ichn].SetPos(vol);
				m_sbVolume[ichn].Invalidate(FALSE);
				SetDlgItemInt(IDC_EDIT1+ichn*2, vol);
				// Pan
				int pan = chnSettings.nPan;
				m_sbPan[ichn].SetPos(pan/4);
				m_sbPan[ichn].Invalidate(FALSE);
				SetDlgItemInt(IDC_EDIT2+ichn*2, pan);

				// Channel name
				s = mpt::ToCString(sndFile.GetCharsetInternal(), chnSettings.szName);
				SetDlgItemText(IDC_EDIT9 + ichn, s);
				((CEdit*)(GetDlgItem(IDC_EDIT9 + ichn)))->LimitText(MAX_CHANNELNAME - 1);
			} else
			{
				SetDlgItemText(IDC_TEXT1 + ichn, _T(""));
				SetDlgItemText(IDC_EDIT9 + ichn, _T(""));
				m_channelColor[ichn].EnableWindow(FALSE);
			}

			// Enable/Disable controls for this channel
			BOOL bIT = ((bEnable) && (sndFile.GetType() & (MOD_TYPE_IT|MOD_TYPE_MPT)));
			GetDlgItem(IDC_CHECK1 + ichn * 2)->EnableWindow(bEnable);
			GetDlgItem(IDC_CHECK2 + ichn * 2)->EnableWindow(bIT);

			m_sbVolume[ichn].EnableWindow(bIT);
			m_spinVolume[ichn].EnableWindow(bIT);

			m_sbPan[ichn].EnableWindow(bEnable && !(sndFile.GetType() & (MOD_TYPE_XM|MOD_TYPE_MOD)));
			m_spinPan[ichn].EnableWindow(bEnable && !(sndFile.GetType() & (MOD_TYPE_XM|MOD_TYPE_MOD)));
			GetDlgItem(IDC_EDIT1 + ichn * 2)->EnableWindow(bIT);	// channel vol
			GetDlgItem(IDC_EDIT2 + ichn * 2)->EnableWindow(bEnable && !(sndFile.GetType() & (MOD_TYPE_XM|MOD_TYPE_MOD)));	// channel pan
			GetDlgItem(IDC_EDIT9 + ichn)->EnableWindow(((bEnable) && (sndFile.GetType() & (MOD_TYPE_XM|MOD_TYPE_IT|MOD_TYPE_MPT))));	// channel name
			m_CbnEffects[ichn].EnableWindow(bEnable & (sndFile.GetModSpecifications().supportsPlugins ? TRUE : FALSE));
		}
		UnlockControls();
	}

	// Update plugin names
	if(genHint.GetType()[HINT_MODTYPE | HINT_MODCHANNELS] || plugHint.GetType()[HINT_PLUGINNAMES])
	{
		PopulateChannelPlugins(hint, pObject);
		if(plugHint.GetType()[HINT_MODTYPE | HINT_PLUGINNAMES])
		{
			m_CbnPlugin.Update(PluginComboBox::Config{PluginComboBox::ShowEmptySlots | PluginComboBox::ShowLibraryNames}.Hint(plugHint, pObject).CurrentSelection(m_nCurrentPlugin), sndFile);
			SetDlgItemText(IDC_EDIT13, mpt::ToCString(sndFile.m_MixPlugins[m_nCurrentPlugin].GetName()));
		}
	}
	// Update plugin info
	const SNDMIXPLUGIN &plugin = sndFile.m_MixPlugins[m_nCurrentPlugin];
	const bool updatePlug = (plugHint.GetPlugin() == 0 || plugHint.GetPlugin() == m_nCurrentPlugin + 1);
	const bool updateWholePluginView = updateAll || (plugHint.GetType()[HINT_MIXPLUGINS] && updatePlug);
	if(updateWholePluginView)
	{
		if(m_nCurrentPlugin >= MAX_MIXPLUGINS)
			m_nCurrentPlugin = 0;
		m_CbnPlugin.SetSelection(m_nCurrentPlugin);
		SetDlgItemText(IDC_EDIT13, mpt::ToCString(plugin.GetName()));
		CheckDlgButton(IDC_CHECK9, plugin.IsMasterEffect() ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(IDC_CHECK10, plugin.IsBypassed() ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(IDC_CHECK11, plugin.IsDryMix() ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(IDC_CHECK13, plugin.IsAutoSuspendable() ? BST_CHECKED : BST_UNCHECKED);
		IMixPlugin *pPlugin = plugin.pMixPlugin;
		m_BtnEdit.EnableWindow((pPlugin != nullptr && (pPlugin->HasEditor() || pPlugin->GetNumVisibleParameters())) ? TRUE : FALSE);
		GetDlgItem(IDC_MOVEFXSLOT)->EnableWindow((pPlugin) ? TRUE : FALSE);
		GetDlgItem(IDC_INSERTFXSLOT)->EnableWindow((pPlugin) ? TRUE : FALSE);
		GetDlgItem(IDC_CLONEPLUG)->EnableWindow((pPlugin) ? TRUE : FALSE);
		GetDlgItem(IDC_DELPLUGIN)->EnableWindow((plugin.IsValidPlugin() || !plugin.Info.szLibraryName.empty() || !plugin.Info.szName.empty()) ? TRUE : FALSE);
		UpdateDryWetDisplay();

		m_CbnSpecialMixProcessing.SetCurSel(static_cast<int>(plugin.GetMixMode()));
		CheckDlgButton(IDC_CHECK12, plugin.IsExpandedMix() ? BST_CHECKED : BST_UNCHECKED);
		int gain = plugin.GetGain();
		if(gain == 0) gain = 10;
		float value = 0.1f * (float)gain;
		s.Format(_T("Gain: x%1.1f"), value);
		SetDlgItemText(IDC_EDIT16, s);
		m_SpinMixGain.SetPos(gain);

		if (pPlugin)
		{
			const PlugParamIndex nParams = pPlugin->GetNumVisibleParameters();
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
			if (in < 1) s = _T("No input");
			else if (in == 1) s = _T("Mono-In");
			else s = _T("Stereo-In");
			s += _T(", ");
			if (out < 1) s += _T("No output");
			else if (out == 1) s += _T("Mono-Out");
			else s += _T("Stereo-Out");

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
			s.Empty();
			if (m_CbnParam.GetCount() > 0) m_CbnParam.ResetContent();
			m_nCurrentParam = 0;

			m_CbnPreset.SetRedraw(FALSE);
			m_CbnPreset.ResetContent();
			m_CbnPreset.SetItemData(m_CbnPreset.AddString(_T("none")), 0);
			m_CbnPreset.SetRedraw(TRUE);
			m_CbnPreset.SetCurSel(0);
			m_sbValue.EnableWindow(FALSE);
			m_sbDryRatio.EnableWindow(FALSE);
			GetDlgItem(IDC_EDIT14)->EnableWindow(FALSE);
		}
		SetDlgItemText(IDC_TEXT6, s);
	}
	
	if(updateWholePluginView || plugHint.GetPlugin() > m_nCurrentPlugin)
	{
		int insertAt = 1;
		m_CbnOutput.SetRedraw(FALSE);
		if(updateWholePluginView)
		{
			m_CbnOutput.ResetContent();
			m_CbnOutput.SetItemData(m_CbnOutput.AddString(_T("Default")), 0);

			for(PLUGINDEX i = m_nCurrentPlugin + 1; i < MAX_MIXPLUGINS; i++)
			{
				if(!sndFile.m_MixPlugins[i].IsValidPlugin())
				{
					m_CbnOutput.SetItemData(m_CbnOutput.AddString(_T("New Plugin...")), 1);
					insertAt = 2;
					break;
				}
			}
		} else
		{
			const DWORD_PTR changedPlugin = 0x80 + (plugHint.GetPlugin() - 1);
			const int items = m_CbnOutput.GetCount();
			for(insertAt = 1; insertAt < items; insertAt++)
			{
				DWORD_PTR thisPlugin = m_CbnOutput.GetItemData(insertAt);
				if(thisPlugin == changedPlugin)
					m_CbnOutput.DeleteString(insertAt);
				if(thisPlugin >= changedPlugin)
					break;
			}
		}

		int outputSel = plugin.IsOutputToMaster() ? 0 : -1;
		for(PLUGINDEX iOut = m_nCurrentPlugin + 1; iOut < MAX_MIXPLUGINS; iOut++)
		{
			if(!updateWholePluginView && (iOut + 1) != plugHint.GetPlugin())
				continue;
			const SNDMIXPLUGIN &outPlug = sndFile.m_MixPlugins[iOut];
			if(outPlug.IsValidPlugin())
			{
				const auto name = outPlug.GetName(), libName = outPlug.GetLibraryName();
				s.Format(_T("FX%d: "), iOut + 1);
				s += mpt::ToCString(name.empty() ? libName : name);
				if(!name.empty() && libName != name)
				{
					s += _T(" (");
					s += mpt::ToCString(libName);
					s += _T(")");
				}

				insertAt = m_CbnOutput.InsertString(insertAt, s);
				m_CbnOutput.SetItemData(insertAt, 0x80 + iOut);
				if(!plugin.IsOutputToMaster() && (plugin.GetOutputPlugin() == iOut))
				{
					outputSel = insertAt;
				}
				insertAt++;
			}
		}
		m_CbnOutput.SetRedraw(TRUE);
		if(outputSel >= 0)
			m_CbnOutput.SetCurSel(outputSel);
	}
	if(plugHint.GetType()[HINT_PLUGINPARAM] && updatePlug)
	{
		OnParamChanged();
	}

	m_CbnPlugin.Invalidate(FALSE);
	m_CbnParam.Invalidate(FALSE);
	m_CbnPreset.Invalidate(FALSE);
	m_CbnSpecialMixProcessing.Invalidate(FALSE);
	m_CbnOutput.Invalidate(FALSE);
}


void CViewGlobals::PopulateChannelPlugins(UpdateHint hint, const CObject *pObj)
{
	const CSoundFile &sndFile = GetDocument()->GetSoundFile();
	for(CHANNELINDEX ichn = 0; ichn < CHANNELS_IN_TAB; ichn++)
	{
		if(const CHANNELINDEX nChn = m_nActiveTab * CHANNELS_IN_TAB + ichn; nChn < sndFile.GetNumChannels())
		{
			PLUGINDEX sel = sndFile.ChnSettings[nChn].nMixPlugin ? sndFile.ChnSettings[nChn].nMixPlugin - 1 : PLUGINDEX_INVALID;
			m_CbnEffects[ichn].Update(PluginComboBox::Config{PluginComboBox::ShowNoPlugin}.Hint(hint, pObj).CurrentSelection(sel), sndFile);
		}
	}
}


IMixPlugin *CViewGlobals::GetCurrentPlugin() const
{
	if(GetDocument() == nullptr || m_nCurrentPlugin >= MAX_MIXPLUGINS)
	{
		return nullptr;
	}

	return GetDocument()->GetSoundFile().m_MixPlugins[m_nCurrentPlugin].pMixPlugin;
}


void CViewGlobals::OnTabSelchange(NMHDR*, LRESULT* pResult)
{
	UpdateView(GeneralHint().Channels());
	if (pResult) *pResult = 0;
}


void CViewGlobals::OnUpdateUndo(CCmdUI *pCmdUI)
{
	CModDoc *pModDoc = GetDocument();
	if((pCmdUI) && (pModDoc))
	{
		pCmdUI->Enable(pModDoc->GetPatternUndo().CanUndoChannelSettings());
		pCmdUI->SetText(CMainFrame::GetInputHandler()->GetKeyTextFromCommand(kcEditUndo, _T("Undo ") + pModDoc->GetPatternUndo().GetUndoName()));
	}
}


void CViewGlobals::OnUpdateRedo(CCmdUI *pCmdUI)
{
	CModDoc *pModDoc = GetDocument();
	if((pCmdUI) && (pModDoc))
	{
		pCmdUI->Enable(pModDoc->GetPatternUndo().CanRedoChannelSettings());
		pCmdUI->SetText(CMainFrame::GetInputHandler()->GetKeyTextFromCommand(kcEditRedo, _T("Redo ") + pModDoc->GetPatternUndo().GetRedoName()));
	}
}


void CViewGlobals::OnEditUndo()
{
	UndoRedo(true);
}


void CViewGlobals::OnEditRedo()
{
	UndoRedo(false);
}


void CViewGlobals::UndoRedo(bool undo)
{
	CModDoc *pModDoc = GetDocument();
	if(!pModDoc)
		return;
	if(undo && pModDoc->GetPatternUndo().CanUndoChannelSettings())
		pModDoc->GetPatternUndo().Undo();
	else if(!undo && pModDoc->GetPatternUndo().CanRedoChannelSettings())
		pModDoc->GetPatternUndo().Redo();
}


void CViewGlobals::PrepareUndo(CHANNELINDEX chnMod4)
{
	if(m_lastEdit != chnMod4)
	{
		// Backup old channel settings through pattern undo.
		m_lastEdit = chnMod4;
		const CHANNELINDEX chn = static_cast<CHANNELINDEX>(m_nActiveTab * CHANNELS_IN_TAB) + chnMod4;
		GetDocument()->GetPatternUndo().PrepareChannelUndo(chn, 1, "Channel Settings");
	}
}


void CViewGlobals::OnEditColor(const CHANNELINDEX chnMod4)
{
	auto *modDoc = GetDocument();
	auto &sndFile = modDoc->GetSoundFile();
	const CHANNELINDEX chn = static_cast<CHANNELINDEX>(m_nActiveTab * CHANNELS_IN_TAB) + chnMod4;
	if(auto color = m_channelColor[chnMod4].PickColor(sndFile, chn); color.has_value())
	{
		PrepareUndo(chnMod4);
		sndFile.ChnSettings[chn].color = *color;
		if(modDoc->SupportsChannelColors())
			modDoc->SetModified();
		modDoc->UpdateAllViews(nullptr, GeneralHint(chn).Channels());
	}
}


void CViewGlobals::OnEditColor1() { OnEditColor(0); }
void CViewGlobals::OnEditColor2() { OnEditColor(1); }
void CViewGlobals::OnEditColor3() { OnEditColor(2); }
void CViewGlobals::OnEditColor4() { OnEditColor(3); }


void CViewGlobals::OnMute(const CHANNELINDEX chnMod4, const UINT itemID)
{
	CModDoc *pModDoc = GetDocument();

	if (pModDoc)
	{
		const bool b = (IsDlgButtonChecked(itemID) != FALSE);
		const CHANNELINDEX nChn = (CHANNELINDEX)(m_nActiveTab * CHANNELS_IN_TAB) + chnMod4;
		pModDoc->MuteChannel(nChn, b);
		pModDoc->UpdateAllViews(this, GeneralHint(nChn).Channels());
	}
}

void CViewGlobals::OnMute1() {OnMute(0, IDC_CHECK1);}
void CViewGlobals::OnMute2() {OnMute(1, IDC_CHECK3);}
void CViewGlobals::OnMute3() {OnMute(2, IDC_CHECK5);}
void CViewGlobals::OnMute4() {OnMute(3, IDC_CHECK7);}


void CViewGlobals::OnSurround(const CHANNELINDEX chnMod4, const UINT itemID)
{
	CModDoc *pModDoc = GetDocument();

	if (pModDoc)
	{
		PrepareUndo(chnMod4);
		const bool b = (IsDlgButtonChecked(itemID) != FALSE);
		const CHANNELINDEX nChn = (CHANNELINDEX)(m_nActiveTab * CHANNELS_IN_TAB) + chnMod4;
		pModDoc->SurroundChannel(nChn, b);
		pModDoc->UpdateAllViews(nullptr, GeneralHint(nChn).Channels());
	}
}

void CViewGlobals::OnSurround1() {OnSurround(0, IDC_CHECK2);}
void CViewGlobals::OnSurround2() {OnSurround(1, IDC_CHECK4);}
void CViewGlobals::OnSurround3() {OnSurround(2, IDC_CHECK6);}
void CViewGlobals::OnSurround4() {OnSurround(3, IDC_CHECK8);}

void CViewGlobals::OnEditVol(const CHANNELINDEX chnMod4, const UINT itemID)
{
	CModDoc *pModDoc = GetDocument();
	const CHANNELINDEX nChn = (CHANNELINDEX)(m_nActiveTab * CHANNELS_IN_TAB) + chnMod4;
	const int vol = GetDlgItemIntEx(itemID);
	if ((pModDoc) && (vol >= 0) && (vol <= 64) && (!m_nLockCount))
	{
		PrepareUndo(chnMod4);
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
{
	CModDoc *pModDoc = GetDocument();
	const CHANNELINDEX nChn = (CHANNELINDEX)(m_nActiveTab * CHANNELS_IN_TAB) + chnMod4;
	const int pan = GetDlgItemIntEx(itemID);
	if ((pModDoc) && (pan >= 0) && (pan <= 256) && (!m_nLockCount))
	{
		PrepareUndo(chnMod4);
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
{
	CFormView::OnHScroll(nSBCode, nPos, pScrollBar);

	CModDoc *pModDoc = GetDocument();
	const CHANNELINDEX nChn = (CHANNELINDEX)(m_nActiveTab * CHANNELS_IN_TAB);
	if(pModDoc && !IsLocked() && nChn < pModDoc->GetNumChannels())
	{
		BOOL bUpdate = FALSE;
		short int pos;

		LockControls();
		const CHANNELINDEX nLoopLimit = std::min(static_cast<CHANNELINDEX>(CHANNELS_IN_TAB), static_cast<CHANNELINDEX>(pModDoc->GetSoundFile().GetNumChannels() - nChn));
		for (CHANNELINDEX iCh = 0; iCh < nLoopLimit; iCh++)
		{
			if(static_cast<CWnd*>(pScrollBar) == &m_sbVolume[iCh])
			{
				// Volume sliders
				pos = (short int)m_sbVolume[iCh].GetPos();
				if ((pos >= 0) && (pos <= 64))
				{
					PrepareUndo(iCh);
					if (pModDoc->SetChannelGlobalVolume(nChn + iCh, pos))
					{
						SetDlgItemInt(IDC_EDIT1 + iCh * 2, pos);
						bUpdate = TRUE;
					}
				}
			} else if(static_cast<CWnd*>(pScrollBar) == &m_sbPan[iCh])
			{
				// Pan sliders
				pos = (short int)m_sbPan[iCh].GetPos();
				if(pos >= 0 && pos <= 64 && (static_cast<uint16>(pos) != pModDoc->GetSoundFile().ChnSettings[nChn+iCh].nPan / 4u))
				{
					PrepareUndo(iCh);
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
				SNDMIXPLUGIN &plugin = pModDoc->GetSoundFile().m_MixPlugins[m_nCurrentPlugin];

				if(plugin.pMixPlugin)
				{
					plugin.fDryRatio = static_cast<float>(n) / 100.0f;
					SetPluginModified();
				}
				UpdateDryWetDisplay();
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
					const PlugParamIndex nParams = pPlugin->GetNumVisibleParameters();
					if(m_nCurrentParam < nParams)
					{
						if (nSBCode == SB_THUMBPOSITION || nSBCode == SB_THUMBTRACK || nSBCode == SB_ENDSCROLL)
						{
							pPlugin->SetScaledUIParam(m_nCurrentParam, 0.01f * n);
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
{
	CModDoc *pModDoc = GetDocument();

	if((m_nCurrentPlugin >= MAX_MIXPLUGINS) || (!pModDoc)) return;
	CSoundFile &sndFile = pModDoc->GetSoundFile();
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
{
	CModDoc *pModDoc = GetDocument();

	if ((pModDoc) && (!m_nLockCount))
	{
		CSoundFile &sndFile = pModDoc->GetSoundFile();
		const CHANNELINDEX nChn = m_nActiveTab * CHANNELS_IN_TAB + chnMod4;
		CString tmp;
		GetDlgItemText(itemID, tmp);
		const std::string s = mpt::ToCharset(sndFile.GetCharsetInternal(), tmp);
		if ((sndFile.GetType() & (MOD_TYPE_XM|MOD_TYPE_IT|MOD_TYPE_MPT)) && (nChn < sndFile.GetNumChannels()) && (s != sndFile.ChnSettings[nChn].szName))
		{
			PrepareUndo(chnMod4);
			sndFile.ChnSettings[nChn].szName = s;
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
{
	CModDoc *pModDoc = GetDocument();

	if (pModDoc)
	{
		CSoundFile &sndFile = pModDoc->GetSoundFile();
		CHANNELINDEX nChn = m_nActiveTab * CHANNELS_IN_TAB + chnMod4;
		PLUGINDEX nfx = m_CbnEffects[chnMod4].GetSelection().value_or(PLUGINDEX_INVALID);
		if(nfx == PLUGINDEX_INVALID)
			nfx = 0;
		else
			nfx++;
		if(nChn < sndFile.GetNumChannels() && sndFile.ChnSettings[nChn].nMixPlugin != nfx)
		{
			PrepareUndo(chnMod4);
			sndFile.ChnSettings[nChn].nMixPlugin = nfx;
			if(sndFile.GetModSpecifications().supportsPlugins)
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
{
	CModDoc *pModDoc = GetDocument();

	if ((pModDoc) && (m_nCurrentPlugin < MAX_MIXPLUGINS))
	{
		CSoundFile &sndFile = pModDoc->GetSoundFile();
		SNDMIXPLUGIN &plugin = sndFile.m_MixPlugins[m_nCurrentPlugin];

		CString s;
		GetDlgItemText(IDC_EDIT13, s);
		if (s != mpt::ToCString(plugin.GetName()))
		{
			plugin.Info.szName = mpt::ToCharset(mpt::Charset::Locale, s);
			if(sndFile.GetModSpecifications().supportsPlugins)
				pModDoc->SetModified();
			const auto updateHint = PluginHint(m_nCurrentPlugin + 1).Names();
			pModDoc->UpdateAllViews(this, updateHint, this);

			IMixPlugin *pPlugin = plugin.pMixPlugin;
			if(pPlugin != nullptr && pPlugin->GetEditor() != nullptr)
			{
				pPlugin->GetEditor()->SetTitle();
			}
			m_CbnPlugin.Update(PluginComboBox::Config{updateHint}, sndFile);
			PopulateChannelPlugins(updateHint);
		}
	}
}


void CViewGlobals::OnPrevPlugin()
{
	CModDoc *pModDoc = GetDocument();
	if ((m_nCurrentPlugin > 0) && (pModDoc))
	{
		m_nCurrentPlugin--;
		UpdateView(PluginHint(m_nCurrentPlugin + 1).Info());
	}
}


void CViewGlobals::OnNextPlugin()
{
	CModDoc *pModDoc = GetDocument();
	if ((m_nCurrentPlugin < MAX_MIXPLUGINS-1) && (pModDoc))
	{
		m_nCurrentPlugin++;
		UpdateView(PluginHint(m_nCurrentPlugin + 1).Info());

	}
}


void CViewGlobals::OnPluginChanged()
{
	const PLUGINDEX plugin = m_CbnPlugin.GetSelection().value_or(PLUGINDEX_INVALID);
	if(plugin != PLUGINDEX_INVALID)
	{
		m_nCurrentPlugin = plugin;
		UpdateView(PluginHint(m_nCurrentPlugin + 1).Info());
	}
	m_CbnPreset.SetCurSel(0);
}


void CViewGlobals::OnSelectPlugin()
{
#ifndef NO_PLUGINS
	CModDoc *pModDoc = GetDocument();

	if ((pModDoc) && (m_nCurrentPlugin < MAX_MIXPLUGINS))
	{
		CSelectPluginDlg dlg(pModDoc, m_nCurrentPlugin, this);
		if (dlg.DoModal() == IDOK)
		{
			if(pModDoc->GetSoundFile().GetModSpecifications().supportsPlugins)
				pModDoc->SetModified();
		}
		OnPluginChanged();
		OnParamChanged();
	}
#endif // NO_PLUGINS
}


void CViewGlobals::OnRemovePlugin()
{
#ifndef NO_PLUGINS
	CModDoc *pModDoc = GetDocument();

	if(pModDoc && m_nCurrentPlugin < MAX_MIXPLUGINS && Reporting::Confirm(MPT_UFORMAT("Remove plugin FX{}: {}?")(m_nCurrentPlugin + 1, pModDoc->GetSoundFile().m_MixPlugins[m_nCurrentPlugin].GetName()), false, true) == cnfYes)
	{
		if(pModDoc->RemovePlugin(m_nCurrentPlugin))
		{
			OnPluginChanged();
			OnParamChanged();
		}
	}
#endif  // NO_PLUGINS
}


LRESULT CViewGlobals::OnParamAutomated(WPARAM plugin, LPARAM param)
{
	if(plugin == m_nCurrentPlugin && static_cast<PlugParamIndex>(param) == m_nCurrentParam)
	{
		OnParamChanged();
	}
	return 0;
}


LRESULT CViewGlobals::OnDryWetRatioChangedFromPlayer(WPARAM plugin, LPARAM)
{
	if(plugin == m_nCurrentPlugin)
	{
		UpdateDryWetDisplay();
	}
	return 0;
}


void CViewGlobals::OnParamChanged()
{
	PlugParamIndex cursel = static_cast<PlugParamIndex>(m_CbnParam.GetItemData(m_CbnParam.GetCurSel()));

	IMixPlugin *pPlugin = GetCurrentPlugin();

	if(pPlugin != nullptr && cursel != static_cast<PlugParamIndex>(CB_ERR))
	{
		const PlugParamIndex nParams = pPlugin->GetNumVisibleParameters();
		if(cursel < nParams) m_nCurrentParam = cursel;
		if(m_nCurrentParam < nParams)
		{
			const auto value = pPlugin->GetScaledUIParam(m_nCurrentParam);
			int intValue = mpt::saturate_round<int>(value * 100.0f);
			LockControls();
			if(GetFocus() != GetDlgItem(IDC_EDIT14))
			{
				CString s = pPlugin->GetFormattedParamValue(m_nCurrentParam).Trim();
				if(s.IsEmpty())
				{
					s.Format(_T("%f"), value);
				}
				SetDlgItemText(IDC_EDIT14, s);
			}
			m_sbValue.SetPos(intValue);
			UnlockControls();
			return;
		}
	}
	SetDlgItemText(IDC_EDIT14, _T(""));
	m_sbValue.SetPos(0);
}


// When focussing the parameter value, show its real value to edit
void CViewGlobals::OnFocusParam()
{
	IMixPlugin *pPlugin = GetCurrentPlugin();
	if(pPlugin != nullptr)
	{
		const PlugParamIndex nParams = pPlugin->GetNumVisibleParameters();
		if(m_nCurrentParam < nParams)
		{
			TCHAR s[32];
			float fValue = pPlugin->GetScaledUIParam(m_nCurrentParam);
			_stprintf(s, _T("%f"), fValue);
			LockControls();
			SetDlgItemText(IDC_EDIT14, s);
			UnlockControls();
		}
	}
}


void CViewGlobals::SetPluginModified()
{
	CModDoc *pModDoc = GetDocument();
	if(pModDoc->GetSoundFile().GetModSpecifications().supportsPlugins)
		pModDoc->SetModified();
	pModDoc->UpdateAllViews(this, PluginHint(m_nCurrentPlugin + 1).Info());
}


void CViewGlobals::OnProgramChanged()
{
	int32 curProg = static_cast<int32>(m_CbnPreset.GetItemData(m_CbnPreset.GetCurSel()));

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
{
	IMixPlugin *pPlugin = GetCurrentPlugin();
	if(pPlugin != nullptr)
	{
		pPlugin->SaveProgram();
	}
}


void CViewGlobals::OnSetParameter()
{
	if(m_nCurrentPlugin >= MAX_MIXPLUGINS || IsLocked()) return;
	IMixPlugin *pPlugin = GetCurrentPlugin();

	if(pPlugin != nullptr)
	{
		const PlugParamIndex nParams = pPlugin->GetNumVisibleParameters();
		TCHAR s[32];
		GetDlgItemText(IDC_EDIT14, s, mpt::saturate_cast<int>(std::size(s)));
		if ((m_nCurrentParam < nParams) && (s[0]))
		{
			float fValue = (float)_tstof(s);
			pPlugin->SetScaledUIParam(m_nCurrentParam, fValue);
			OnParamChanged();
			SetPluginModified();
		}
	}
}


void CViewGlobals::UpdateDryWetDisplay()
{
	SNDMIXPLUGIN &plugin = GetDocument()->GetSoundFile().m_MixPlugins[m_nCurrentPlugin];
	float wetRatio = 1.0f - plugin.fDryRatio, dryRatio = plugin.fDryRatio;
	m_sbDryRatio.SetPos(mpt::saturate_round<int>(wetRatio * 100));
	if(plugin.IsExpandedMix())
	{
		wetRatio = 2.0f * wetRatio - 1.0f;
		dryRatio = -wetRatio;
	}
	int wetInt = mpt::saturate_round<int>(wetRatio * 100), dryInt = mpt::saturate_round<int>(dryRatio * 100);
	SetDlgItemText(IDC_STATIC8, MPT_TFORMAT("{}% wet, {}% dry")(wetInt, dryInt).c_str());
}


void CViewGlobals::OnMixModeChanged()
{
	CModDoc *pModDoc = GetDocument();
	if ((m_nCurrentPlugin >= MAX_MIXPLUGINS) || (!pModDoc)) return;

	pModDoc->GetSoundFile().m_MixPlugins[m_nCurrentPlugin].SetMasterEffect(IsDlgButtonChecked(IDC_CHECK9) != BST_UNCHECKED);
	SetPluginModified();
}


void CViewGlobals::OnBypassChanged()
{
	CModDoc *pModDoc = GetDocument();
	if ((m_nCurrentPlugin >= MAX_MIXPLUGINS) || (!pModDoc)) return;

	auto &mixPlugs = pModDoc->GetSoundFile().m_MixPlugins;
	auto &currentPlug = pModDoc->GetSoundFile().m_MixPlugins[m_nCurrentPlugin];
	bool bypass = IsDlgButtonChecked(IDC_CHECK10) != BST_UNCHECKED;
	const bool bypassOthers = CInputHandler::ShiftPressed();
	const bool bypassOnlyMasterPlugs = CInputHandler::CtrlPressed();
	if(bypassOthers || bypassOnlyMasterPlugs)
	{
		CheckDlgButton(IDC_CHECK10, currentPlug.IsBypassed() ? BST_CHECKED : BST_UNCHECKED);
		uint8 state = 0;
		for(const auto &plug : mixPlugs)
		{
			if(!plug.pMixPlugin || &plug == &currentPlug)
				continue;
			if(!plug.IsMasterEffect() && bypassOnlyMasterPlugs)
				continue;
			if(plug.IsBypassed())
				state |= 1;
			else
				state |= 2;
			if(state == 3)
				break;
		}
		if(!state)
			return;

		bypass = (state != 1);
		for(auto &plug : mixPlugs)
		{
			if(!plug.pMixPlugin || &plug == &currentPlug)
				continue;
			if(!plug.IsMasterEffect() && bypassOnlyMasterPlugs)
				continue;
			if(plug.IsBypassed() == bypass)
				continue;
			plug.SetBypass(bypass);
			pModDoc->UpdateAllViews(this, PluginHint(plug.pMixPlugin->GetSlot() + 1).Info());
		}
		if(pModDoc->GetSoundFile().GetModSpecifications().supportsPlugins)
			pModDoc->SetModified();
	} else
	{
		currentPlug.SetBypass(bypass);
		SetPluginModified();
	}
}


void CViewGlobals::OnWetDryExpandChanged()
{
	CModDoc *pModDoc = GetDocument();
	if ((m_nCurrentPlugin >= MAX_MIXPLUGINS) || (!pModDoc)) return;

	pModDoc->GetSoundFile().m_MixPlugins[m_nCurrentPlugin].SetExpandedMix(IsDlgButtonChecked(IDC_CHECK12) != BST_UNCHECKED);
	UpdateDryWetDisplay();
	SetPluginModified();
}


void CViewGlobals::OnAutoSuspendChanged()
{
	CModDoc *pModDoc = GetDocument();
	if(m_nCurrentPlugin >= MAX_MIXPLUGINS || !pModDoc)
		return;

	pModDoc->GetSoundFile().m_MixPlugins[m_nCurrentPlugin].SetAutoSuspend(IsDlgButtonChecked(IDC_CHECK13) != BST_UNCHECKED);
	SetPluginModified();
}


void CViewGlobals::OnSpecialMixProcessingChanged()
{
	CModDoc *pModDoc = GetDocument();
	if ((m_nCurrentPlugin >= MAX_MIXPLUGINS) || (!pModDoc)) return;

	pModDoc->GetSoundFile().m_MixPlugins[m_nCurrentPlugin].SetMixMode(static_cast<PluginMixMode>(m_CbnSpecialMixProcessing.GetCurSel()));
	SetPluginModified();
}


void CViewGlobals::OnDryMixChanged()
{
	CModDoc *pModDoc = GetDocument();
	if ((m_nCurrentPlugin >= MAX_MIXPLUGINS) || (!pModDoc)) return;

	pModDoc->GetSoundFile().m_MixPlugins[m_nCurrentPlugin].SetDryMix(IsDlgButtonChecked(IDC_CHECK11) != BST_UNCHECKED);
	SetPluginModified();
}


void CViewGlobals::OnEditPlugin()
{
	CModDoc *pModDoc = GetDocument();
	if ((m_nCurrentPlugin >= MAX_MIXPLUGINS) || (!pModDoc)) return;
	pModDoc->TogglePluginEditor(m_nCurrentPlugin, CInputHandler::ShiftPressed());
	return;
}


void CViewGlobals::OnOutputRoutingChanged()
{
	CModDoc *pModDoc = GetDocument();
	int nroute;

	if ((m_nCurrentPlugin >= MAX_MIXPLUGINS) || (!pModDoc)) return;
	CSoundFile &sndFile = pModDoc->GetSoundFile();
	SNDMIXPLUGIN &plugin = sndFile.m_MixPlugins[m_nCurrentPlugin];
	nroute = static_cast<int>(m_CbnOutput.GetItemData(m_CbnOutput.GetCurSel()));

	if(nroute == 1)
	{
		// Add new plugin
		for(PLUGINDEX i = m_nCurrentPlugin + 1; i < MAX_MIXPLUGINS; i++)
		{
			if(!sndFile.m_MixPlugins[i].IsValidPlugin())
			{
				CSelectPluginDlg dlg(pModDoc, i, this);
				if(dlg.DoModal() != IDOK)
					return;
				
				plugin.SetOutputPlugin(i);
				SetPluginModified();
				nroute = 0x80 + i;
				m_nCurrentPlugin = i;
				m_CbnPlugin.SetSelection(i);
				OnPluginChanged();
				break;
			}
		}
		if(nroute == 1)
		{
			Reporting::Error("All following plugin slots are used.", "Unable to add new plugin");
			return;
		}
	}

	if(!nroute)
		plugin.SetOutputToMaster();
	else
		plugin.SetOutputPlugin(static_cast<PLUGINDEX>(nroute - 0x80));

	SetPluginModified();
}



LRESULT CViewGlobals::OnModViewMsg(WPARAM wParam, LPARAM /*lParam*/)
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
{
	if(GetCurrentPlugin() == nullptr)
	{
		return;
	}

	// If any plugin routes its output to the current plugin, we shouldn't try to move it before that plugin...
	PLUGINDEX defaultIndex = 0;
	CSoundFile &sndFile = GetDocument()->GetSoundFile();
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

		m_CbnPlugin.SetSelection(dlg.GetSlot());
		OnPluginChanged();
		GetDocument()->UpdateAllViews(nullptr, PluginHint().Names().Info());
	}
}


// Functor for adjusting plug indexes in modcommands. Adjusts all instrument column values in
// range [m_nInstrMin, m_nInstrMax] by m_nDiff.
struct PlugIndexModifier
{
	PlugIndexModifier(PLUGINDEX nMin, PLUGINDEX nMax, int nDiff) :
		m_nDiff(nDiff), m_nInstrMin(nMin), m_nInstrMax(nMax) {}
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
{
	if (src == dest)
		return false;
	CModDoc *pModDoc = GetDocument();
	CSoundFile &sndFile = pModDoc->GetSoundFile();

	BeginWaitCursor();

	CriticalSection cs;

	// Move plug data
	sndFile.m_MixPlugins[dest] = std::move(sndFile.m_MixPlugins[src]);
	mpt::reconstruct(sndFile.m_MixPlugins[src]);

	sndFile.GetMIDIMapper().MovePlugin(src, dest);

	// Prevent plug from pointing backwards.
	if(!sndFile.m_MixPlugins[dest].IsOutputToMaster())
	{
		PLUGINDEX nOutput = sndFile.m_MixPlugins[dest].GetOutputPlugin();
		if (nOutput <= dest && nOutput != PLUGINDEX_INVALID)
		{
			sndFile.m_MixPlugins[dest].SetOutputToMaster();
		}
	}

	// Update current plug
	IMixPlugin *pPlugin = sndFile.m_MixPlugins[dest].pMixPlugin;
	if(pPlugin != nullptr)
	{
		pPlugin->SetSlot(dest);
		if(pPlugin->GetEditor() != nullptr)
		{
			pPlugin->GetEditor()->SetTitle();
		}
	}

	// Update all other plugs' outputs
	for (PLUGINDEX nPlug = 0; nPlug < src; nPlug++)
	{
		if(!sndFile.m_MixPlugins[nPlug].IsOutputToMaster())
		{
			if(sndFile.m_MixPlugins[nPlug].GetOutputPlugin() == src)
			{
				sndFile.m_MixPlugins[nPlug].SetOutputPlugin(dest);
			}
		}
	}
	// Update channels
	for (CHANNELINDEX nChn = 0; nChn < sndFile.GetNumChannels(); nChn++)
	{
		if (sndFile.ChnSettings[nChn].nMixPlugin == src + 1u)
		{
			sndFile.ChnSettings[nChn].nMixPlugin = dest + 1u;
		}
	}

	// Update instruments
	for (INSTRUMENTINDEX nIns = 1; nIns <= sndFile.GetNumInstruments(); nIns++)
	{
		if (sndFile.Instruments[nIns] && (sndFile.Instruments[nIns]->nMixPlug == src + 1))
		{
			sndFile.Instruments[nIns]->nMixPlug = dest + 1u;
		}
	}

	// Update MODCOMMANDs so that they won't be referring to old indexes (e.g. with NOTE_PC).
	if (bAdjustPat && sndFile.GetModSpecifications().HasNote(NOTE_PC))
		sndFile.Patterns.ForEachModCommand(PlugIndexModifier(src + 1, src + 1, int(dest) - int(src)));

	cs.Leave();

	SetPluginModified();

	EndWaitCursor();

	return true;
}


void CViewGlobals::BuildEmptySlotList(std::vector<PLUGINDEX> &emptySlots)
{
	const CSoundFile &sndFile = GetDocument()->GetSoundFile();

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
{
	CString prompt;
	CSoundFile &sndFile = GetDocument()->GetSoundFile();
	prompt.Format(_T("Insert empty slot before slot FX%d?"), m_nCurrentPlugin + 1);

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
			prompt += _T("\nWarning: plugin data in last slot will be lost.");
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

		m_CbnPlugin.SetSelection(m_nCurrentPlugin);
		OnPluginChanged();
		GetDocument()->UpdateAllViews(nullptr, PluginHint().Names().Info());

		SetPluginModified();
	}

}


void CViewGlobals::OnClonePlug()
{
	if(GetCurrentPlugin() == nullptr)
	{
		return;
	}

	CSoundFile &sndFile = GetDocument()->GetSoundFile();

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
			IMixPlugin *mixPlug = newPlugin.pMixPlugin;
			if(mixPlug != nullptr && mixPlug->IsInstrument() && GetDocument()->HasInstrumentForPlugin(emptySlots[toIndex]) == INSTRUMENTINDEX_INVALID)
			{
				GetDocument()->InsertInstrumentForPlugin(emptySlots[toIndex]);
			}

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

		m_CbnPlugin.SetSelection(dlg.GetSlot());
		OnPluginChanged();
		//PopulateChannelPlugins(PluginHint().Names());
		GetDocument()->UpdateAllViews(nullptr, PluginHint().Names(), nullptr);

		SetPluginModified();
	}
}


// The plugin param box is only filled when it gets the focus (done here).
void CViewGlobals::OnFillParamCombo()
{
	// no need to fill it again.
	if(m_CbnParam.GetCount() > 1)
		return;

	IMixPlugin *pPlugin = GetCurrentPlugin();
	if(pPlugin == nullptr) return;

	const PlugParamIndex nParams = pPlugin->GetNumVisibleParameters();
	m_CbnParam.SetRedraw(FALSE);
	m_CbnParam.ResetContent();

	AddPluginParameternamesToCombobox(m_CbnParam, *pPlugin);

	if (m_nCurrentParam >= nParams) m_nCurrentParam = 0;
	m_CbnParam.SetCurSel(m_nCurrentParam);
	m_CbnParam.SetRedraw(TRUE);
	m_CbnParam.Invalidate(FALSE);
}


// The preset box is only filled when it gets the focus (done here).
void CViewGlobals::OnFillProgramCombo()
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


INT_PTR CViewGlobals::OnToolHitTest(CPoint point, TOOLINFO *pTI) const
{
	INT_PTR nHit = CFormView::OnToolHitTest(point, pTI);
	if(nHit >= 0 && pTI && (pTI->uFlags & TTF_IDISHWND))
	{
		// Workaround to get tooltips even for disabled controls inside group boxes that are positioned in the "correct" tab order position.
		// For some reason doesn't work for enabled controls (probably because pTI->hwnd then doesn't point at the active control under the cursor),
		// so we use the default code path there, which works just fine.
		HWND child = reinterpret_cast<HWND>(pTI->uId);
		if(!::IsWindowEnabled(child))
		{
			pTI->uId = nHit;
			pTI->uFlags &= ~TTF_IDISHWND;
			::GetWindowRect(child, &pTI->rect);
			ScreenToClient(&pTI->rect);
		}
	}
	return nHit;
}


BOOL CViewGlobals::OnToolTipText(UINT, NMHDR *pNMHDR, LRESULT *pResult)
{
	auto pTTT = reinterpret_cast<TOOLTIPTEXT *>(pNMHDR);
	UINT_PTR id = pNMHDR->idFrom;
	if(pTTT->uFlags & TTF_IDISHWND)
	{
		// idFrom is actually the HWND of the tool
		id = static_cast<UINT_PTR>(::GetDlgCtrlID(reinterpret_cast<HWND>(id)));
	}

	mpt::tstring text;
	const auto &chnSettings = GetDocument()->GetSoundFile().ChnSettings;
	switch(id)
	{
		case IDC_EDIT1:
		case IDC_EDIT3:
		case IDC_EDIT5:
		case IDC_EDIT7:
			text = CModDoc::LinearToDecibelsString(chnSettings[m_nActiveTab * CHANNELS_IN_TAB + (id - IDC_EDIT1) / 2].nVolume, 64.0);
			break;
		case IDC_SLIDER1:
		case IDC_SLIDER3:
		case IDC_SLIDER5:
		case IDC_SLIDER7:
			text = CModDoc::LinearToDecibelsString(chnSettings[m_nActiveTab * CHANNELS_IN_TAB + (id - IDC_SLIDER1) / 2].nVolume, 64.0);
			break;
		case IDC_EDIT2:
		case IDC_EDIT4:
		case IDC_EDIT6:
		case IDC_EDIT8:
			text = CModDoc::PanningToString(chnSettings[m_nActiveTab * CHANNELS_IN_TAB + (id - IDC_EDIT2) / 2].nPan, 128);
			break;
		case IDC_SLIDER2:
		case IDC_SLIDER4:
		case IDC_SLIDER6:
		case IDC_SLIDER8:
			text = CModDoc::PanningToString(chnSettings[m_nActiveTab * CHANNELS_IN_TAB + (id - IDC_SLIDER2) / 2].nPan, 128);
			break;
		case IDC_EDIT16:
			{
				const auto gain = GetDocument()->GetSoundFile().m_MixPlugins[m_nCurrentPlugin].GetGain();
				text = CModDoc::LinearToDecibelsString(gain ? gain : 10, 10.0);
			}
			break;
		case IDC_BUTTON5:
			text = _T("Previous Plugin");
			break;
		case IDC_BUTTON4:
			text = _T("Next Plugin");
			break;
		default:
			return FALSE;
	}

	mpt::String::WriteWinBuf(pTTT->szText) = text;
	*pResult = 0;

	// bring the tooltip window above other popup windows
	::SetWindowPos(pNMHDR->hwndFrom, HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE | SWP_NOOWNERZORDER);

	return TRUE;  // message was handled
}


OPENMPT_NAMESPACE_END
