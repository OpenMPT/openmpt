#include "stdafx.h"
#include "mptrack.h"
#include "moddoc.h"
#include "mainfrm.h"
#include "dlg_misc.h"
#include "dlsbank.h"
#include "ChildFrm.h"
#include "vstplug.h"
#include "ChannelManagerDlg.h"
#include "midi.h"
#include "version.h"

#pragma warning(disable:4244) //"conversion from 'type1' to 'type2', possible loss of data"


///////////////////////////////////////////////////////////////////////
// CModTypeDlg


BEGIN_MESSAGE_MAP(CModTypeDlg, CDialog)
	//{{AFX_MSG_MAP(CModTypeDlg)
	ON_COMMAND(IDC_CHECK1,		OnCheck1)
	ON_COMMAND(IDC_CHECK2,		OnCheck2)
	ON_COMMAND(IDC_CHECK3,		OnCheck3)
	ON_COMMAND(IDC_CHECK4,		OnCheck4)
	ON_COMMAND(IDC_CHECK5,		OnCheck5)
// -> CODE#0023
// -> DESC="IT project files (.itp)"
	ON_COMMAND(IDC_CHECK6,		OnCheck6)
	ON_COMMAND(IDC_CHECK_PT1X,	OnCheckPT1x)
	ON_CBN_SELCHANGE(IDC_COMBO1,UpdateDialog)
// -! NEW_FEATURE#0023

	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, &CModTypeDlg::OnToolTipNotify)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, &CModTypeDlg::OnToolTipNotify)

	//}}AFX_MSG_MAP

END_MESSAGE_MAP()


void CModTypeDlg::DoDataExchange(CDataExchange* pDX)
//--------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CModTypeDlg)
	DDX_Control(pDX, IDC_COMBO1,		m_TypeBox);
	DDX_Control(pDX, IDC_COMBO2,		m_ChannelsBox);
	DDX_Control(pDX, IDC_COMBO_TEMPOMODE,	m_TempoModeBox);
	DDX_Control(pDX, IDC_COMBO_MIXLEVELS,	m_PlugMixBox);
	DDX_Control(pDX, IDC_CHECK1,		m_CheckBox1);
	DDX_Control(pDX, IDC_CHECK2,		m_CheckBox2);
	DDX_Control(pDX, IDC_CHECK3,		m_CheckBox3);
	DDX_Control(pDX, IDC_CHECK4,		m_CheckBox4);
	DDX_Control(pDX, IDC_CHECK5,		m_CheckBox5);
// -> CODE#0023
// -> DESC="IT project files (.itp)"
	DDX_Control(pDX, IDC_CHECK6,		m_CheckBox6);
// -! NEW_FEATURE#0023
	DDX_Control(pDX, IDC_CHECK_PT1X,	m_CheckBoxPT1x);
	//}}AFX_DATA_MAP
}


BOOL CModTypeDlg::OnInitDialog()
//------------------------------
{
	CDialog::OnInitDialog();
	m_nType = m_pSndFile->m_nType;
	m_nChannels = m_pSndFile->m_nChannels;
	m_dwSongFlags = m_pSndFile->m_dwSongFlags;
	SetDlgItemInt(IDC_ROWSPERBEAT, m_pSndFile->m_nDefaultRowsPerBeat);
	SetDlgItemInt(IDC_ROWSPERMEASURE, m_pSndFile->m_nDefaultRowsPerMeasure);

	m_TypeBox.SetItemData(m_TypeBox.AddString("ProTracker MOD"), MOD_TYPE_MOD);
	m_TypeBox.SetItemData(m_TypeBox.AddString("ScreamTracker S3M"), MOD_TYPE_S3M);
	m_TypeBox.SetItemData(m_TypeBox.AddString("FastTracker XM"), MOD_TYPE_XM);
	m_TypeBox.SetItemData(m_TypeBox.AddString("Impulse Tracker IT"), MOD_TYPE_IT);
// -> CODE#0023
// -> DESC="IT project files (.itp)"
	m_TypeBox.SetItemData(m_TypeBox.AddString("Impulse Tracker Project ITP"), MOD_TYPE_IT);
	m_TypeBox.SetItemData(m_TypeBox.AddString("OpenMPT MPTM"), MOD_TYPE_MPT);
// -! NEW_FEATURE#0023
	switch(m_nType)
	{
	case MOD_TYPE_S3M:	m_TypeBox.SetCurSel(1); break;
	case MOD_TYPE_XM:	m_TypeBox.SetCurSel(2); break;
// -> CODE#0023
// -> DESC="IT project files (.itp)"
	case MOD_TYPE_IT:	m_TypeBox.SetCurSel(m_pSndFile->m_dwSongFlags & SONG_ITPROJECT ? 4 : 3); break;
// -! NEW_FEATURE#0023
	case MOD_TYPE_MPT:	m_TypeBox.SetCurSel(5); break;
	default:			m_TypeBox.SetCurSel(0); break;
	}

	UpdateChannelCBox();
	
	m_TempoModeBox.SetItemData(m_TempoModeBox.AddString("Classic"), tempo_mode_classic);
	m_TempoModeBox.SetItemData(m_TempoModeBox.AddString("Alternative"), tempo_mode_alternative);
	m_TempoModeBox.SetItemData(m_TempoModeBox.AddString("Modern (accurate)"), tempo_mode_modern);
	switch(m_pSndFile->m_nTempoMode)
	{
		case tempo_mode_alternative:	m_TempoModeBox.SetCurSel(1); break;
		case tempo_mode_modern:			m_TempoModeBox.SetCurSel(2); break;
		case tempo_mode_classic:
		default:						m_TempoModeBox.SetCurSel(0); break;
	}

	m_PlugMixBox.SetItemData(m_PlugMixBox.AddString("OpenMPT 1.17RC3"),		mixLevels_117RC3);
	m_PlugMixBox.SetItemData(m_PlugMixBox.AddString("OpenMPT 1.17RC2"),		mixLevels_117RC2);
	m_PlugMixBox.SetItemData(m_PlugMixBox.AddString("OpenMPT 1.17RC1"),		mixLevels_117RC1);
	m_PlugMixBox.SetItemData(m_PlugMixBox.AddString("Original (MPT 1.16)"),	mixLevels_original);
	m_PlugMixBox.SetItemData(m_PlugMixBox.AddString("Compatible"),			mixLevels_compatible);
	//m_PlugMixBox.SetItemData(m_PlugMixBox.AddString("Test"),				mixLevels_Test);
	switch(m_pSndFile->m_nMixLevels)
	{
		//case mixLevels_Test:		m_PlugMixBox.SetCurSel(5); break;
		case mixLevels_compatible:	m_PlugMixBox.SetCurSel(4); break;		case mixLevels_original:	m_PlugMixBox.SetCurSel(3); break;
		case mixLevels_117RC1:		m_PlugMixBox.SetCurSel(2); break;
		case mixLevels_117RC2:		m_PlugMixBox.SetCurSel(1); break;
		case mixLevels_117RC3:
		default:					m_PlugMixBox.SetCurSel(0); break;
	}

	SetDlgItemText(IDC_TEXT_CREATEDWITH, "Created with:");
	SetDlgItemText(IDC_TEXT_SAVEDWITH, "Last saved with:");

	SetDlgItemText(IDC_EDIT_CREATEDWITH, MptVersion::ToStr(m_pSndFile->m_dwCreatedWithVersion));
	SetDlgItemText(IDC_EDIT_SAVEDWITH, MptVersion::ToStr(m_pSndFile->m_dwLastSavedWithVersion));

	UpdateDialog();

	EnableToolTips(TRUE);
	return TRUE;
}


void CModTypeDlg::UpdateChannelCBox()
//-----------------------------------
{
	const MODTYPE type = m_TypeBox.GetItemData(m_TypeBox.GetCurSel());
	CHANNELINDEX currChanSel = m_ChannelsBox.GetItemData(m_ChannelsBox.GetCurSel());
	const CHANNELINDEX minChans = CSoundFile::GetModSpecifications(type).channelsMin;
	const CHANNELINDEX maxChans = CSoundFile::GetModSpecifications(type).channelsMax;
	if(m_ChannelsBox.GetCount() < 1 
		||
	   m_ChannelsBox.GetItemData(0) != minChans
		|| 
	   m_ChannelsBox.GetItemData(m_ChannelsBox.GetCount()-1) != maxChans
	  )
	{
		if(m_ChannelsBox.GetCount() < 1) currChanSel = m_nChannels;
		char s[256];
		m_ChannelsBox.ResetContent();
		for (CHANNELINDEX i=minChans; i<=maxChans; i++)
		{
			wsprintf(s, "%d Channel%s", i, (i != 1) ? "s" : "");
			m_ChannelsBox.SetItemData(m_ChannelsBox.AddString(s), i);
		}
		if(currChanSel > maxChans)
			m_ChannelsBox.SetCurSel(m_ChannelsBox.GetCount()-1);
		else
			m_ChannelsBox.SetCurSel(currChanSel-minChans);
	}
}


void CModTypeDlg::UpdateDialog()
//------------------------------
{
	UpdateChannelCBox();

	m_CheckBox1.SetCheck((m_pSndFile->m_dwSongFlags & SONG_LINEARSLIDES) ? MF_CHECKED : 0);
	m_CheckBox2.SetCheck((m_pSndFile->m_dwSongFlags & SONG_FASTVOLSLIDES) ? MF_CHECKED : 0);
	m_CheckBox3.SetCheck((m_pSndFile->m_dwSongFlags & SONG_ITOLDEFFECTS) ? MF_CHECKED : 0);
	m_CheckBox4.SetCheck((m_pSndFile->m_dwSongFlags & SONG_ITCOMPATGXX) ? MF_CHECKED : 0);
	m_CheckBox5.SetCheck((m_pSndFile->m_dwSongFlags & SONG_EXFILTERRANGE) ? MF_CHECKED : 0);
	m_CheckBoxPT1x.SetCheck((m_pSndFile->m_dwSongFlags & SONG_PT1XMODE) ? MF_CHECKED : 0);

// -> CODE#0023
// -> DESC="IT project files (.itp)"
	m_CheckBox6.SetCheck((m_pSndFile->m_dwSongFlags & SONG_ITPEMBEDIH) ? MF_CHECKED : 0);
// -! NEW_FEATURE#0023

	m_CheckBox1.EnableWindow((m_pSndFile->m_nType & (MOD_TYPE_XM|MOD_TYPE_IT|MOD_TYPE_MPT)) ? TRUE : FALSE);
	m_CheckBox2.EnableWindow((m_pSndFile->m_nType == MOD_TYPE_S3M) ? TRUE : FALSE);
	m_CheckBox3.EnableWindow((m_pSndFile->m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT)) ? TRUE : FALSE);
	m_CheckBox4.EnableWindow((m_pSndFile->m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT)) ? TRUE : FALSE);
	m_CheckBox5.EnableWindow((m_pSndFile->m_nType & (MOD_TYPE_XM|MOD_TYPE_IT|MOD_TYPE_MPT)) ? TRUE : FALSE);
	m_CheckBoxPT1x.EnableWindow((m_pSndFile->m_nType & (MOD_TYPE_MOD)) ? TRUE : FALSE);

// -> CODE#0023
// -> DESC="IT project files (.itp)"
	m_CheckBox6.EnableWindow(m_TypeBox.GetCurSel() == 4 ? TRUE : FALSE);
// -! NEW_FEATURE#0023

	const bool XMorITorMPT = ((m_TypeBox.GetItemData(m_TypeBox.GetCurSel()) & (MOD_TYPE_XM | MOD_TYPE_IT | MOD_TYPE_MPT)) != 0);
	const bool ITorMPT = ((m_TypeBox.GetItemData(m_TypeBox.GetCurSel()) & (MOD_TYPE_IT | MOD_TYPE_MPT)) != 0);

	// Misc Flags
	if(ITorMPT)
	{
		GetDlgItem(IDC_CHK_COMPATPLAY)->SetWindowText(_T("More Impulse Tracker compatible playback"));
	} else
	{
		GetDlgItem(IDC_CHK_COMPATPLAY)->SetWindowText(_T("More Fasttracker 2 compatible playback"));
	}

	GetDlgItem(IDC_CHK_COMPATPLAY)->ShowWindow(XMorITorMPT);
	GetDlgItem(IDC_CHK_MIDICCBUG)->ShowWindow(XMorITorMPT);
	GetDlgItem(IDC_CHK_OLDRANDOM)->ShowWindow(ITorMPT);

	CheckDlgButton(IDC_CHK_COMPATPLAY, m_pSndFile->GetModFlag(MSF_COMPATIBLE_PLAY));
	CheckDlgButton(IDC_CHK_MIDICCBUG, m_pSndFile->GetModFlag(MSF_MIDICC_BUGEMULATION));
	CheckDlgButton(IDC_CHK_OLDRANDOM, m_pSndFile->GetModFlag(MSF_OLDVOLSWING));

	// Mixmode Box
	GetDlgItem(IDC_TEXT_MIXMODE)->ShowWindow(XMorITorMPT);
	m_PlugMixBox.ShowWindow(XMorITorMPT);
	
	// Tempo mode box
	m_TempoModeBox.ShowWindow(XMorITorMPT);
	GetDlgItem(IDC_ROWSPERBEAT)->ShowWindow(XMorITorMPT);
	GetDlgItem(IDC_ROWSPERMEASURE)->ShowWindow(XMorITorMPT);
	GetDlgItem(IDC_TEXT_ROWSPERBEAT)->ShowWindow(XMorITorMPT);
	GetDlgItem(IDC_TEXT_ROWSPERMEASURE)->ShowWindow(XMorITorMPT);
	GetDlgItem(IDC_TEXT_TEMPOMODE)->ShowWindow(XMorITorMPT);
	GetDlgItem(IDC_FRAME_TEMPOMODE)->ShowWindow(XMorITorMPT);

	// Version info
	GetDlgItem(IDC_FRAME_MPTVERSION)->ShowWindow(XMorITorMPT);
	GetDlgItem(IDC_TEXT_CREATEDWITH)->ShowWindow(XMorITorMPT);
	GetDlgItem(IDC_TEXT_SAVEDWITH)->ShowWindow(XMorITorMPT);
	GetDlgItem(IDC_EDIT_CREATEDWITH)->ShowWindow(XMorITorMPT);
	GetDlgItem(IDC_EDIT_SAVEDWITH)->ShowWindow(XMorITorMPT);

	// Window height - some parts of the dialog won't be visible for all formats
	RECT rWindow;
	GetWindowRect(&rWindow);
	
	UINT iHeight;
	int nItemID = (XMorITorMPT) ? IDC_FRAME_MPTVERSION : IDC_FRAME_MODFLAGS;
	RECT rFrame;
	GetDlgItem(nItemID)->GetWindowRect(&rFrame);
	iHeight = rFrame.bottom - rWindow.top + 12;
	MoveWindow(rWindow.left, rWindow.top, rWindow.right - rWindow.left, iHeight);

}


void CModTypeDlg::OnCheck1()
//--------------------------
{
	if (m_CheckBox1.GetCheck())
		m_pSndFile->m_dwSongFlags |= SONG_LINEARSLIDES;
	else
		m_pSndFile->m_dwSongFlags &= ~SONG_LINEARSLIDES;
}


void CModTypeDlg::OnCheck2()
//--------------------------
{
	if (m_CheckBox2.GetCheck())
		m_pSndFile->m_dwSongFlags |= SONG_FASTVOLSLIDES;
	else
		m_pSndFile->m_dwSongFlags &= ~SONG_FASTVOLSLIDES;
}


void CModTypeDlg::OnCheck3()
//--------------------------
{
	if (m_CheckBox3.GetCheck())
		m_pSndFile->m_dwSongFlags |= SONG_ITOLDEFFECTS;
	else
		m_pSndFile->m_dwSongFlags &= ~SONG_ITOLDEFFECTS;
}


void CModTypeDlg::OnCheck4()
//--------------------------
{
	if (m_CheckBox4.GetCheck())
		m_pSndFile->m_dwSongFlags |= SONG_ITCOMPATGXX;
	else
		m_pSndFile->m_dwSongFlags &= ~SONG_ITCOMPATGXX;
}


void CModTypeDlg::OnCheck5()
//--------------------------
{
	if (m_CheckBox5.GetCheck())
		m_pSndFile->m_dwSongFlags |= SONG_EXFILTERRANGE;
	else
		m_pSndFile->m_dwSongFlags &= ~SONG_EXFILTERRANGE;
}


void CModTypeDlg::OnCheck6()
//--------------------------
{
	if (m_CheckBox6.GetCheck())
		m_pSndFile->m_dwSongFlags |= SONG_ITPEMBEDIH;
	else
		m_pSndFile->m_dwSongFlags &= ~SONG_ITPEMBEDIH;
}

void CModTypeDlg::OnCheckPT1x()
//-----------------------------
{
	if (m_CheckBoxPT1x.GetCheck())
		m_pSndFile->m_dwSongFlags |= SONG_PT1XMODE;
	else
		m_pSndFile->m_dwSongFlags &= ~SONG_PT1XMODE;
}


bool CModTypeDlg::VerifyData() 
//----------------------------
{

	int temp_nRPB = GetDlgItemInt(IDC_ROWSPERBEAT);
	int temp_nRPM = GetDlgItemInt(IDC_ROWSPERMEASURE);
	if ((temp_nRPB > temp_nRPM))
	{
		::AfxMessageBox("Error: Rows per measure must be greater than or equal rows per beat.", MB_OK|MB_ICONEXCLAMATION);
		GetDlgItem(IDC_ROWSPERMEASURE)->SetFocus();
		return false;
	}

	int sel = m_ChannelsBox.GetItemData(m_ChannelsBox.GetCurSel());
	int type = m_TypeBox.GetItemData(m_TypeBox.GetCurSel());

	CHANNELINDEX maxChans = CSoundFile::GetModSpecifications(type).channelsMax;

	if (sel > maxChans)
	{
		CString error;
		error.Format("Error: Max number of channels for this type is %d", maxChans);
		::AfxMessageBox(error, MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	if(maxChans < m_pSndFile->GetNumChannels())
	{
		if(MessageBox("New module type supports less channels than currently used, and reducing channel number is required. Continue?", "", MB_OKCANCEL) != IDOK)
			return false;
	}

	return true;
}

void CModTypeDlg::OnOK()
//----------------------
{
	if (!VerifyData())
		return;

	int sel = m_TypeBox.GetCurSel();
	if (sel >= 0)
	{
		m_nType = m_TypeBox.GetItemData(sel);
// -> CODE#0023
// -> DESC="IT project files (.itp)"
		if(m_pSndFile->m_dwSongFlags & SONG_ITPROJECT && sel != 4)
		{
			m_pSndFile->m_dwSongFlags &= ~SONG_ITPROJECT;
			m_pSndFile->m_dwSongFlags &= ~SONG_ITPEMBEDIH;
		}
		if(sel == 4) m_pSndFile->m_dwSongFlags |= SONG_ITPROJECT;
// -! NEW_FEATURE#0023
	}
	sel = m_ChannelsBox.GetCurSel();
	if (sel >= 0)
	{
		m_nChannels = m_ChannelsBox.GetItemData(sel);
		//if (m_nType & MOD_TYPE_XM) m_nChannels = (m_nChannels+1) & 0xFE;
	}
	
	sel = m_TempoModeBox.GetCurSel();
	if (sel >= 0)
	{
		m_pSndFile->m_nTempoMode = m_TempoModeBox.GetItemData(sel);
	}

	sel = m_PlugMixBox.GetCurSel();
	if (sel >= 0)
	{
		m_pSndFile->m_nMixLevels = m_PlugMixBox.GetItemData(sel);
		m_pSndFile->m_pConfig->SetMixLevels(m_pSndFile->m_nMixLevels);
		m_pSndFile->RecalculateGainForAllPlugs();
	}

	if(m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT | MOD_TYPE_XM))
	{
		m_pSndFile->SetModFlags(0);
		if(IsDlgButtonChecked(IDC_CHK_COMPATPLAY)) m_pSndFile->SetModFlag(MSF_COMPATIBLE_PLAY, true);
		if(IsDlgButtonChecked(IDC_CHK_MIDICCBUG)) m_pSndFile->SetModFlag(MSF_MIDICC_BUGEMULATION, true);
		if(IsDlgButtonChecked(IDC_CHK_OLDRANDOM)) m_pSndFile->SetModFlag(MSF_OLDVOLSWING, true);
	}

	m_pSndFile->m_nDefaultRowsPerBeat    = GetDlgItemInt(IDC_ROWSPERBEAT);
	m_pSndFile->m_nDefaultRowsPerMeasure = GetDlgItemInt(IDC_ROWSPERMEASURE);

	m_pSndFile->SetupITBidiMode();

	if(CChannelManagerDlg::sharedInstance(FALSE) && CChannelManagerDlg::sharedInstance()->IsDisplayed())
		CChannelManagerDlg::sharedInstance()->Update();
	CDialog::OnOK();
}

void CModTypeDlg::OnCancel()
//--------------------------
{
	// Reset mod flags
	m_pSndFile->m_dwSongFlags = m_dwSongFlags;
	CDialog::OnCancel();
}


BOOL CModTypeDlg::OnToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult)
//-------------------------------------------------------------------------
{
	UNREFERENCED_PARAMETER(id);
	UNREFERENCED_PARAMETER(pResult);

	// need to handle both ANSI and UNICODE versions of the message
	TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
	TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;
	CStringA strTipText = "";
	UINT_PTR nID = pNMHDR->idFrom;
	if (pNMHDR->code == TTN_NEEDTEXTA && (pTTTA->uFlags & TTF_IDISHWND) ||
		pNMHDR->code == TTN_NEEDTEXTW && (pTTTW->uFlags & TTF_IDISHWND))
	{
		// idFrom is actually the HWND of the tool
		nID = ::GetDlgCtrlID((HWND)nID);
	}

	switch(nID)
	{
	case IDC_CHECK1:
		strTipText = "Note slides always slide the same amount, not depending on the sample frequency.";
		break;
	case IDC_CHECK2:
		strTipText = "Old ScreamTracker 3 volume slide behaviour (not recommended).";
		break;
	case IDC_CHECK3:
		strTipText = "Play some effects like in early versions of Impulse Tracker (not recommended).";
		break;
	case IDC_CHECK4:
		strTipText = "Gxx and Exx/Fxx won't share effect memory. Gxx resets instrument envelopes.";
		break;
	case IDC_CHECK5:
		strTipText = "The resonant filter's frequency range is increased from about 4KHz to 10KHz.";
		break;
	case IDC_CHECK6:
		strTipText = "The instrument settings of the external ITI files will be ignored.";
		break;
	case IDC_CHECK_PT1X:
		strTipText = "Ignore pan fx, use on-the-fly sample swapping, enforce Amiga frequency limits.";
		break;
	case IDC_COMBO_MIXLEVELS:
		strTipText = "Mixing method of sample and VST levels.";
		break;
	case IDC_CHK_COMPATPLAY:
		strTipText = "Play commands as the original tracker would play them (recommended)";
		break;
	case IDC_CHK_MIDICCBUG:
		strTipText = "Emulate an old bug which sent wrong volume messages to VSTis (not recommended)";
		break;
	case IDC_CHK_OLDRANDOM:
		strTipText = "Use old (buggy) random volume / panning variation algorithm (not recommended)";
		break;
	}

	if (pNMHDR->code == TTN_NEEDTEXTA)
	{
		//strncpy_s(pTTTA->szText, sizeof(pTTTA->szText), strTipText, 
		//	strTipText.GetLength() + 1);
		// 80 chars max?!
		strncpy(pTTTA->szText, strTipText, min(strTipText.GetLength() + 1, ARRAYELEMCOUNT(pTTTA->szText) - 1));
	}
	else
	{
		::MultiByteToWideChar(CP_ACP , 0, strTipText, strTipText.GetLength() + 1,
			pTTTW->szText, ARRAYELEMCOUNT(pTTTW->szText));
	}

	return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
// CShowLogDlg

BEGIN_MESSAGE_MAP(CShowLogDlg, CDialog)
	//{{AFX_MSG_MAP(CShowLogDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()



void CShowLogDlg::DoDataExchange(CDataExchange* pDX)
//--------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CShowLogDlg)
	DDX_Control(pDX, IDC_EDIT_LOG,		m_EditLog);
	//}}AFX_DATA_MAP
}


BOOL CShowLogDlg::OnInitDialog()
//------------------------------
{
	CDialog::OnInitDialog();
	if (m_lpszTitle) SetWindowText(m_lpszTitle);
	m_EditLog.SetSel(0, -1);
	m_EditLog.ReplaceSel(m_lpszLog);
	m_EditLog.SetFocus();
	m_EditLog.SetSel(0, 0);
	return FALSE;
}


UINT CShowLogDlg::ShowLog(LPCSTR pszLog, LPCSTR lpszTitle)
//--------------------------------------------------------
{
	m_lpszLog = pszLog;
	m_lpszTitle = lpszTitle;
	return DoModal();
}


///////////////////////////////////////////////////////////
// CRemoveChannelsDlg

//rewbs.removeChansDlgCleanup
void CRemoveChannelsDlg::DoDataExchange(CDataExchange* pDX)
//--------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CShowLogDlg)
	DDX_Control(pDX, IDC_REMCHANSLIST,		m_RemChansList);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRemoveChannelsDlg, CDialog)
	//{{AFX_MSG_MAP(CRemoveChannelsDlg)
	ON_LBN_SELCHANGE(IDC_REMCHANSLIST,		OnChannelChanged)
	//}}AFX_MSG_MAP
//	ON_WM_SIZE()
END_MESSAGE_MAP()



BOOL CRemoveChannelsDlg::OnInitDialog()
//-------------------------------------
{
	CHAR label[max(100, 20 + MAX_CHANNELNAME)];
	CDialog::OnInitDialog();
	for (UINT n = 0; n < m_nChannels; n++)
	{
		if(m_pSndFile->ChnSettings[n].szName[0] >= 0x20)
			wsprintf(label, "Channel %d: %s", (n + 1), m_pSndFile->ChnSettings[n].szName);
		else
			wsprintf(label, "Channel %d", n + 1);

		m_RemChansList.SetItemData(m_RemChansList.AddString(label), n);
		if (!m_bKeepMask[n]) m_RemChansList.SetSel(n);
	}

	if (m_nRemove > 0) {
		wsprintf(label, "Select %d channel%s to remove:", m_nRemove, (m_nRemove != 1) ? "s" : "");
	} else {
		wsprintf(label, "Select channels to remove (the minimum number of remaining channels is %d)", m_pSndFile->GetModSpecifications().channelsMin);
	}
	
	SetDlgItemText(IDC_QUESTION1, label);
	if(GetDlgItem(IDCANCEL)) GetDlgItem(IDCANCEL)->ShowWindow(m_ShowCancel);

	OnChannelChanged();
	return TRUE;
}


void CRemoveChannelsDlg::OnOK()
//-----------------------------
{
	int nCount = m_RemChansList.GetSelCount();
	CArray<int,int> aryListBoxSel;
	aryListBoxSel.SetSize(nCount);
	m_RemChansList.GetSelItems(nCount, aryListBoxSel.GetData()); 

	m_bKeepMask.assign(m_nChannels, true);
	for (int n = 0; n < nCount; n++)
	{
		m_bKeepMask[aryListBoxSel[n]] = false;
	}
	if ((static_cast<UINT>(nCount) == m_nRemove && nCount > 0)  || (m_nRemove == 0 && (m_pSndFile->GetNumChannels() >= nCount + m_pSndFile->GetModSpecifications().channelsMin)))
		CDialog::OnOK();
	else
		CDialog::OnCancel();
}


void CRemoveChannelsDlg::OnChannelChanged()
//-----------------------------------------
{
	UINT nr = 0;
	nr = m_RemChansList.GetSelCount();
	GetDlgItem(IDOK)->EnableWindow(((nr == m_nRemove && nr >0)  || (m_nRemove == 0 && (m_pSndFile->GetNumChannels() >= nr + m_pSndFile->GetModSpecifications().channelsMin) && nr > 0)) ? TRUE : FALSE);
}
//end rewbs.removeChansDlgCleanup


////////////////////////////////////////////////////////////////////////////////
// Sound Bank Information

CSoundBankProperties::CSoundBankProperties(CDLSBank *pBank, CWnd *parent):CDialog(IDD_SOUNDBANK_INFO, parent)
//-----------------------------------------------------------------------------------------------------------
{
	SOUNDBANKINFO bi;
	
	m_szInfo[0] = 0;
	if (pBank)
	{
		UINT nType = pBank->GetBankInfo(&bi);
		wsprintf(m_szInfo, "File:\t\"%s\"\r\n", pBank->GetFileName());
		wsprintf(&m_szInfo[strlen(m_szInfo)], "Type:\t%s\r\n", (nType & SOUNDBANK_TYPE_SF2) ? "Sound Font (SF2)" : "Downloadable Sound (DLS)");
		if (bi.szBankName[0])
			wsprintf(&m_szInfo[strlen(m_szInfo)], "Name:\t\"%s\"\r\n", bi.szBankName);
		if (bi.szDescription[0])
			wsprintf(&m_szInfo[strlen(m_szInfo)], "\t\"%s\"\r\n", bi.szDescription);
		if (bi.szCopyRight[0])
			wsprintf(&m_szInfo[strlen(m_szInfo)], "Copyright:\t\"%s\"\r\n", bi.szCopyRight);
		if (bi.szEngineer[0])
			wsprintf(&m_szInfo[strlen(m_szInfo)], "Author:\t\"%s\"\r\n", bi.szEngineer);
		if (bi.szSoftware[0])
			wsprintf(&m_szInfo[strlen(m_szInfo)], "Software:\t\"%s\"\r\n", bi.szSoftware);
		// Last lines: comments
		if (bi.szComments[0])
		{
			strncat(m_szInfo, "\r\nComments:\r\n", sizeof(m_szInfo)-1);
			strncat(m_szInfo, bi.szComments, sizeof(m_szInfo)-1);
		}
	}
}


BOOL CSoundBankProperties::OnInitDialog()
//---------------------------------------
{
	CDialog::OnInitDialog();
	SetDlgItemText(IDC_EDIT1, m_szInfo);
	return TRUE;
}


////////////////////////////////////////////////////////////////////////
// Midi Macros (Zxx)

BEGIN_MESSAGE_MAP(CMidiMacroSetup, CDialog)
	ON_COMMAND(IDC_CHECK1,			OnEmbedMidiCfg)
	ON_COMMAND(IDC_BUTTON1,			OnSetAsDefault)
	ON_COMMAND(IDC_BUTTON2,			OnResetCfg)
	ON_CBN_SELCHANGE(IDC_COMBO1,	OnSFxChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2,	OnSFxPresetChanged)
	ON_CBN_SELCHANGE(IDC_COMBO3,	OnZxxPresetChanged)
	ON_CBN_SELCHANGE(IDC_COMBO4,	UpdateDialog)
	ON_CBN_SELCHANGE(IDC_MACROPLUG, OnPlugChanged)
	ON_CBN_SELCHANGE(IDC_MACROPARAM,OnPlugParamChanged)
	ON_CBN_SELCHANGE(IDC_MACROCC,	OnCCChanged)
	ON_EN_CHANGE(IDC_EDIT1,			OnSFxEditChanged)
	ON_EN_CHANGE(IDC_EDIT2,			OnZxxEditChanged)
	ON_COMMAND_RANGE(ID_PLUGSELECT, ID_PLUGSELECT+NMACROS-1, OnViewAllParams) //rewbs.patPlugName
	ON_COMMAND_RANGE(ID_PLUGSELECT+NMACROS, ID_PLUGSELECT+NMACROS+NMACROS-1, OnSetSFx) //rewbs.patPlugName
END_MESSAGE_MAP()


CMidiMacroSetup::CMidiMacroSetup(MODMIDICFG *pcfg, BOOL bEmbed, CWnd *parent):CDialog(IDD_MIDIMACRO, parent)
//----------------------------------------------------------------------------------------------------------
{
	m_bEmbed = bEmbed;
	if (pcfg) m_MidiCfg = *pcfg;
	m_pSndFile = NULL;
	m_pModDoc = NULL;
}


void CMidiMacroSetup::DoDataExchange(CDataExchange* pDX)
//------------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CModTypeDlg)
	DDX_Control(pDX, IDC_COMBO1,	m_CbnSFx);
	DDX_Control(pDX, IDC_COMBO2,	m_CbnSFxPreset);
	DDX_Control(pDX, IDC_COMBO3,	m_CbnZxxPreset);
	DDX_Control(pDX, IDC_COMBO4,	m_CbnZxx);
	DDX_Control(pDX, IDC_EDIT1,		m_EditSFx);
	DDX_Control(pDX, IDC_EDIT2,		m_EditZxx);
	DDX_Control(pDX, IDC_MACROPLUG, m_CbnMacroPlug);
	DDX_Control(pDX, IDC_MACROPARAM, m_CbnMacroParam);
	DDX_Control(pDX, IDC_MACROCC,   m_CbnMacroCC);
	//}}AFX_DATA_MAP
}


BOOL CMidiMacroSetup::OnInitDialog()
//----------------------------------
{
	m_pModDoc = CMainFrame::GetMainFrame()->GetActiveDoc();
	if (m_pModDoc) m_pSndFile = CMainFrame::GetMainFrame()->GetActiveDoc()->GetSoundFile();
	if (!m_pSndFile)
		return FALSE;

	CHAR s[128];
	CDialog::OnInitDialog();
	CheckDlgButton(IDC_CHECK1, m_bEmbed);
	m_EditSFx.SetLimitText(31);
	m_EditZxx.SetLimitText(31);

	for (UINT isfx=0; isfx<16; isfx++)
	{
		wsprintf(s, "%d (SF%X)", isfx, isfx);
		m_CbnSFx.AddString(s);
	}
	m_CbnSFx.SetCurSel(0);
	m_CbnSFxPreset.AddString("Unused");
	m_CbnSFxPreset.AddString("Set Filter Cutoff");
	m_CbnSFxPreset.AddString("Set Filter Resonance");
	m_CbnSFxPreset.AddString("Set Filter Mode");
	m_CbnSFxPreset.AddString("Plugin Dry/Wet Ratio");
	m_CbnSFxPreset.AddString("Control Plugin Param...");
	m_CbnSFxPreset.AddString("MIDI CC...");
	m_CbnSFxPreset.AddString("Custom");
	OnSFxChanged();

	for (UINT cc=MIDICC_start; cc<=MIDICC_end; cc++) {
		wsprintf(s, "CC %02d %s", cc, MidiCCNames[cc]);
		m_CbnMacroCC.SetItemData(m_CbnMacroCC.AddString(s), cc);	
	}

	for (UINT zxx=0; zxx<128; zxx++) {
		wsprintf(s, "Z%02X", zxx|0x80);
		m_CbnZxx.AddString(s);
	}
	m_CbnZxx.SetCurSel(0);
	m_CbnZxxPreset.AddString("Custom");
	m_CbnZxxPreset.AddString("Z80-Z8F controls resonance");
	m_CbnZxxPreset.AddString("Z80-ZFF controls resonance");
	m_CbnZxxPreset.AddString("Z80-ZFF controls cutoff");
	m_CbnZxxPreset.AddString("Z80-ZFF controls filter mode");
	m_CbnZxxPreset.AddString("Z80-Z9F controls resonance+mode");
	m_CbnZxxPreset.SetCurSel(m_pModDoc->GetZxxType(m_MidiCfg.szMidiZXXExt));
	UpdateDialog();

	int offsetx=108, offsety=30, separatorx=4, separatory=2, 
		height=18, widthMacro=30, widthVal=55, widthType=135, widthBtn=60;
	
	for (UINT m=0; m<NMACROS; m++)
	{
		m_EditMacro[m].Create("", /*BS_FLAT |*/ WS_CHILD | WS_VISIBLE | WS_TABSTOP /*| WS_BORDER*/,
			CRect(offsetx, offsety+m*(separatory+height), offsetx+widthMacro, offsety+m*(separatory+height)+height), this, ID_PLUGSELECT+NMACROS+m);
		m_EditMacro[m].SetFont(GetFont());
		
		m_EditMacroType[m].Create(ES_READONLY | WS_CHILD| WS_VISIBLE | WS_TABSTOP | WS_BORDER, 
			CRect(offsetx+separatorx+widthMacro, offsety+m*(separatory+height), offsetx+widthMacro+widthType, offsety+m*(separatory+height)+height), this, ID_PLUGSELECT+NMACROS+m);
		m_EditMacroType[m].SetFont(GetFont());

		m_EditMacroValue[m].Create(ES_CENTER | ES_READONLY | WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER, 
			CRect(offsetx+separatorx+widthType+widthMacro, offsety+m*(separatory+height), offsetx+widthMacro+widthType+widthVal, offsety+m*(separatory+height)+height), this, ID_PLUGSELECT+NMACROS+m);
		m_EditMacroValue[m].SetFont(GetFont());

		m_BtnMacroShowAll[m].Create("Show All...", WS_CHILD | WS_TABSTOP | WS_VISIBLE,
			CRect(offsetx+separatorx+widthType+widthMacro+widthVal, offsety+m*(separatory+height), offsetx+widthMacro+widthType+widthVal+widthBtn, offsety+m*(separatory+height)+height), this, ID_PLUGSELECT+m);
		m_BtnMacroShowAll[m].SetFont(GetFont());
	}
	UpdateMacroList();

	for (UINT plug=0; plug<MAX_MIXPLUGINS; plug++)
	{
		PSNDMIXPLUGIN p = &(m_pSndFile->m_MixPlugins[plug]);
		p->Info.szLibraryName[63] = 0;
		if (p->Info.szLibraryName[0])
		{
			wsprintf(s, "FX%d: %s", plug+1, p->Info.szName);
			m_CbnMacroPlug.SetItemData(m_CbnMacroPlug.AddString(s), plug);
		}
	}
	m_CbnMacroPlug.SetCurSel(0);
	OnPlugChanged();
	return FALSE;
}


void CMidiMacroSetup::UpdateMacroList(int macro) //-1 for all macros
//----------------------------------------------
{
	if (!m_EditMacro[0])
		return; //GUI not yet initiali

	CString s, macroText;
	UINT start, end, macroType;
	int selectedMacro=m_CbnSFx.GetCurSel();

	if (macro>=0 && macro<16)	{
		start=macro;
		end=macro;
	} else {
		start=0;
		end=NMACROS;
	}

	for (int m=0; m<NMACROS; m++)  {
		//SFx
		s.Format("SF%X", m);
		m_EditMacro[m].SetWindowText(s);

		//Macro value:
		CString macroText = &m_MidiCfg.szMidiSFXExt[m*32];
		m_EditMacroValue[m].SetWindowText(macroText);
		m_EditMacroValue[m].SetBackColor(m == selectedMacro ? RGB(200,200,225) : RGB(245,245,245) );

		//Macro Type:
		macroType = m_pModDoc->GetMacroType(macroText);
		switch (macroType)
		{
			case sfx_unused: s = "Unused"; break;
			case sfx_cutoff: s = "Set Filter Cutoff"; break;
			case sfx_reso: s = "Set Filter Resonance"; break;
			case sfx_mode: s = "Set Filter Mode"; break;
			case sfx_drywet: s = "Set Plugin dry/wet ratio"; break;
			case sfx_cc:
				s.Format("MIDI CC %d", m_pModDoc->MacroToMidiCC(macroText)); 
				break;
			case sfx_plug: 
				s.Format("Control Plugin Param %d", m_pModDoc->MacroToPlugParam(macroText)); 
				break;
			case sfx_custom: 
			default: s = "Custom";
		}
		m_EditMacroType[m].SetWindowText(s);
		m_EditMacroType[m].SetBackColor(m == selectedMacro ? RGB(200,200,225) : RGB(245,245,245) );

		//Param details button:
		if (macroType == sfx_plug)
			m_BtnMacroShowAll[m].ShowWindow(SW_SHOW);
		else 
			m_BtnMacroShowAll[m].ShowWindow(SW_HIDE);
	}
}


void CMidiMacroSetup::UpdateDialog()
//----------------------------------
{
	CHAR s[32];
	UINT sfx, sfx_preset, zxx;

	sfx = m_CbnSFx.GetCurSel();
	sfx_preset = m_CbnSFxPreset.GetCurSel();
	if (sfx < 16) {
		ToggleBoxes(sfx_preset, sfx);
		memcpy(s, &m_MidiCfg.szMidiSFXExt[sfx*32], 32);
		s[31] = 0;
		m_EditSFx.SetWindowText(s);
	}

	zxx = m_CbnZxx.GetCurSel();
	if (zxx < 0x80)
	{
		memcpy(s, &m_MidiCfg.szMidiZXXExt[zxx*32], 32);
		s[31] = 0;
		m_EditZxx.SetWindowText(s);
	}
	UpdateMacroList();
}


void CMidiMacroSetup::OnSetAsDefault()
//------------------------------------
{
	theApp.SetDefaultMidiMacro(&m_MidiCfg);
}


void CMidiMacroSetup::OnResetCfg()
//--------------------------------
{
	theApp.GetDefaultMidiMacro(&m_MidiCfg);
	m_CbnZxxPreset.SetCurSel(0);
	OnSFxChanged();
}


void CMidiMacroSetup::OnEmbedMidiCfg()
//------------------------------------
{
	m_bEmbed = IsDlgButtonChecked(IDC_CHECK1);
}


void CMidiMacroSetup::OnSFxChanged()
//----------------------------------
{
	UINT sfx = m_CbnSFx.GetCurSel();
	if (sfx < 16) {
		CString macroText;
		memcpy(macroText.GetBuffer(32), &m_MidiCfg.szMidiSFXExt[sfx*32], 32);
		int preset = m_pModDoc->GetMacroType(macroText);
		m_CbnSFxPreset.SetCurSel(preset);
	}
	UpdateDialog();
}


void CMidiMacroSetup::OnSFxPresetChanged()
//----------------------------------------
{
	UINT sfx = m_CbnSFx.GetCurSel();
	UINT sfx_preset = m_CbnSFxPreset.GetCurSel();

	if (sfx < 16)
	{
		CHAR *pmacro = &m_MidiCfg.szMidiSFXExt[sfx*32];
		switch(sfx_preset)
		{
		case sfx_unused:	pmacro[0] = 0; break;				// unused
		case sfx_cutoff:	strcpy(pmacro, "F0F000z"); break;	// cutoff
		case sfx_reso:		strcpy(pmacro, "F0F001z"); break;   // reso
		case sfx_mode:		strcpy(pmacro, "F0F002z"); break;   // mode
		case sfx_drywet:	strcpy(pmacro, "F0F003z"); break;   
		case sfx_cc:		strcpy(pmacro, "BK00z"); break;		// MIDI cc - TODO: get value from other menus
		case sfx_plug:		strcpy(pmacro, "F0F080z"); break;	// plug param - TODO: get value from other menus
		case sfx_custom:	/*strcpy(pmacro, "z");*/ break;		// custom - leave as is.
		}
		UpdateDialog();
	}
}


void CMidiMacroSetup::OnZxxPresetChanged()
//----------------------------------------
{
	enmFixedMacroType zxx_preset = static_cast<enmFixedMacroType>(m_CbnZxxPreset.GetCurSel());

	if (zxx_preset != sfx_fixed_custom && m_pModDoc != nullptr)
	{
		m_pModDoc->CreateZxxFromType(m_MidiCfg.szMidiZXXExt, zxx_preset);
		UpdateDialog();
	}
}


void CMidiMacroSetup::OnSFxEditChanged()
//--------------------------------------
{
	CHAR s[32];
	UINT sfx = m_CbnSFx.GetCurSel();
	if (sfx < 16)
	{
		memset(s, 0, sizeof(s));
		m_EditSFx.GetWindowText(s, 31);
		s[31] = 0;
		//Make all uppercase
		_strupr(s);		
		//Replace Z with z.
		char* zocc = strchr(s,'Z'); 
		if (zocc) zocc[0]='z';	

		memcpy(&m_MidiCfg.szMidiSFXExt[sfx*32], s, 32);
		int sfx_preset = m_pModDoc->GetMacroType(&(m_MidiCfg.szMidiSFXExt[sfx*32]));
		//int param = m_pModDoc->MacroToPlugParam(&(m_MidiCfg.szMidiSFXExt[sfx*32]));
		
		m_CbnSFxPreset.SetCurSel(sfx_preset);
		ToggleBoxes(sfx_preset, sfx);
		UpdateMacroList(sfx);
	}
}


void CMidiMacroSetup::OnZxxEditChanged()
//--------------------------------------
{
	CHAR s[32];
	UINT zxx = m_CbnZxx.GetCurSel();
	if (zxx < 128)
	{
		memset(s, 0, sizeof(s));
		m_EditZxx.GetWindowText(s, 31);
		s[31] = 0;
		memcpy(&m_MidiCfg.szMidiZXXExt[zxx*32], s, 32);
	}
}

void CMidiMacroSetup::OnSetSFx(UINT id)
{
	m_CbnSFx.SetCurSel(id-(ID_PLUGSELECT+NMACROS));
	OnSFxChanged();
}

void CMidiMacroSetup::OnViewAllParams(UINT id)
{
	if (!m_pSndFile)
		return;

	CString message, plugName, paramName, line;
	int sfx = id-ID_PLUGSELECT;
	int param = m_pModDoc->MacroToPlugParam(&(m_MidiCfg.szMidiSFXExt[sfx*32]));
	CVstPlugin *pVstPlugin; 
	char s[256];
	message.Format("These are the parameters that can be controlled by macro SF%X:\n\n",sfx);
	
	for (UINT plug=0; plug<MAX_MIXPLUGINS; plug++)
	{
		plugName = m_pSndFile->m_MixPlugins[plug].Info.szName;
		if (plugName != "")
		{
			pVstPlugin=(CVstPlugin*) m_pSndFile->m_MixPlugins[plug].pMixPlugin;
			if (pVstPlugin && pVstPlugin->GetNumParameters()>param)
			{
				pVstPlugin->GetParamName(param, s, 256);
				paramName = s;
				line.Format("FX%d: %s\t Param %d (%x): %s\n", plug + 1, plugName, param, param+80, paramName);
				message += line;
			}
		}
	}

	::MessageBox(NULL, message, "Macro -> Params", MB_OK);
}

void CMidiMacroSetup::OnPlugChanged()
//------------------------------------
{
	if (!m_pSndFile)
		return;

	int plug = m_CbnMacroPlug.GetItemData(m_CbnMacroPlug.GetCurSel());

	if (plug<0 || plug>MAX_MIXPLUGINS)
		return;

	PSNDMIXPLUGIN pPlugin = &m_pSndFile->m_MixPlugins[plug];
	CVstPlugin *pVstPlugin = (pPlugin->pMixPlugin) ? (CVstPlugin *)pPlugin->pMixPlugin : NULL;

	if (pVstPlugin)
	{
		m_CbnMacroParam.SetRedraw(FALSE);
		m_CbnMacroParam.Clear();
		m_CbnMacroParam.ResetContent();
		AddPluginParameternamesToCombobox(m_CbnMacroParam, *pVstPlugin);
		m_CbnMacroParam.SetRedraw(TRUE);
		
		int param = m_pModDoc->MacroToPlugParam(&(m_MidiCfg.szMidiSFXExt[m_CbnSFx.GetCurSel()*32]));
		m_CbnMacroParam.SetCurSel(param);
	}
	//OnPlugParamChanged();
}

void CMidiMacroSetup::OnPlugParamChanged()
{
	CString macroText;
	UINT param = m_CbnMacroParam.GetItemData(m_CbnMacroParam.GetCurSel());

	if (param<128) {
		macroText.Format("F0F0%02Xz",param+128);
		m_EditSFx.SetWindowText(macroText);
	} else if (param<384) {
		macroText.Format("F0F1%02Xz",param-128);
		m_EditSFx.SetWindowText(macroText);
	} else 	{
		::AfxMessageBox("Warning: Currently MPT can only assign macros to parameters 0 to 383");
		param = 383;
	}	
}

void CMidiMacroSetup::OnCCChanged() {
	CString macroText;
	UINT cc = m_CbnMacroCC.GetItemData(m_CbnMacroCC.GetCurSel());
	macroText.Format("BK%02Xz", cc&0xFF);
	m_EditSFx.SetWindowText(macroText);
}

void CMidiMacroSetup::ToggleBoxes(UINT sfx_preset, UINT sfx) {

	if (sfx_preset == sfx_plug)	{
		m_CbnMacroCC.ShowWindow(FALSE);
		m_CbnMacroPlug.ShowWindow(TRUE);
		m_CbnMacroPlug.ShowWindow(TRUE);
		m_CbnMacroPlug.EnableWindow(TRUE);
		m_CbnMacroParam.EnableWindow(TRUE);
		SetDlgItemText(IDC_GENMACROLABEL, "Plug/Param");
		m_CbnMacroParam.SetCurSel(m_pModDoc->MacroToPlugParam(&(m_MidiCfg.szMidiSFXExt[sfx*32])));
	} else {
		m_CbnMacroPlug.EnableWindow(FALSE);
		m_CbnMacroParam.EnableWindow(FALSE);
	}
	
	if (sfx_preset == sfx_cc) {
		m_CbnMacroCC.EnableWindow(TRUE);
		m_CbnMacroCC.ShowWindow(TRUE);
		m_CbnMacroPlug.ShowWindow(FALSE);
		m_CbnMacroPlug.ShowWindow(FALSE);
		SetDlgItemText(IDC_GENMACROLABEL, "MIDI CC");
		m_CbnMacroCC.SetCurSel(m_pModDoc->MacroToMidiCC(&(m_MidiCfg.szMidiSFXExt[sfx*32])));
	} else {
		m_CbnMacroCC.EnableWindow(FALSE);
	}

	if (sfx_preset == sfx_unused) {
		m_EditSFx.EnableWindow(FALSE);
	} else {
		m_EditSFx.EnableWindow(TRUE);
	}

}

////////////////////////////////////////////////////////////////////////////////////////////
// Keyboard Control

const BYTE whitetab[7] = {0,2,4,5,7,9,11};
const BYTE blacktab[7] = {0xff,1,3,0xff,6,8,10};

BEGIN_MESSAGE_MAP(CKeyboardControl, CWnd)
	ON_WM_PAINT()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()


void CKeyboardControl::OnPaint()
//------------------------------
{
	HGDIOBJ oldpen, oldbrush;
	CRect rcClient, rect;
	CPaintDC dc(this);
	HDC hdc = dc.m_hDC;
	HBRUSH brushRed;

	if (!m_nOctaves) m_nOctaves = 1;
	GetClientRect(&rcClient);
	rect = rcClient;
	oldpen = ::SelectObject(hdc, CMainFrame::penBlack);
	oldbrush = ::SelectObject(hdc, CMainFrame::brushWhite);
	brushRed = ::CreateSolidBrush(RGB(0xFF, 0, 0));
	// White notes
	for (UINT note=0; note<m_nOctaves*7; note++)
	{
		rect.right = ((note + 1) * rcClient.Width()) / (m_nOctaves * 7);
		int val = (note/7) * 12 + whitetab[note % 7];
		if (val == m_nSelection) ::SelectObject(hdc, CMainFrame::brushGray);
		dc.Rectangle(&rect);
		if (val == m_nSelection) ::SelectObject(hdc, CMainFrame::brushWhite);
		if ((val < NOTE_MAX) && (KeyFlags[val]))
		{
			::SelectObject(hdc, brushRed);
			dc.Ellipse(rect.left+2, rect.bottom - (rect.right-rect.left) + 2, rect.right-2, rect.bottom-2);
			::SelectObject(hdc, CMainFrame::brushWhite);
		}
		rect.left = rect.right - 1;
	}
	// Black notes
	::SelectObject(hdc, CMainFrame::brushBlack);
	rect = rcClient;
	rect.bottom -= rcClient.Height() / 3;
	for (UINT nblack=0; nblack<m_nOctaves*7; nblack++)
	{
		switch(nblack % 7)
		{
		case 1:
		case 2:
		case 4:
		case 5:
		case 6:
			{
				rect.left = (nblack * rcClient.Width()) / (m_nOctaves * 7);
				rect.right = rect.left;
				int delta = rcClient.Width() / (m_nOctaves * 7 * 3);
				rect.left -= delta;
				rect.right += delta;
				int val = (nblack/7)*12 + blacktab[nblack%7];
				if (val == m_nSelection) ::SelectObject(hdc, CMainFrame::brushGray);
				dc.Rectangle(&rect);
				if (val == m_nSelection) ::SelectObject(hdc, CMainFrame::brushBlack);
				if ((val < NOTE_MAX) && (KeyFlags[val]))
				{
					::SelectObject(hdc, brushRed);
					dc.Ellipse(rect.left, rect.bottom - (rect.right-rect.left), rect.right, rect.bottom);
					::SelectObject(hdc, CMainFrame::brushBlack);
				}
			}
			break;
		}
	}
	if (oldpen) ::SelectObject(hdc, oldpen);
	if (oldbrush) ::SelectObject(hdc, oldbrush);
}


void CKeyboardControl::OnMouseMove(UINT, CPoint point)
//----------------------------------------------------
{
	int sel = -1, xmin, xmax;
	CRect rcClient, rect;
	if (!m_nOctaves) m_nOctaves = 1;
	GetClientRect(&rcClient);
	rect = rcClient;
	xmin = rcClient.right;
	xmax = rcClient.left;
	// White notes
	for (UINT note=0; note<m_nOctaves*7; note++)
	{
		int val = (note/7)*12 + whitetab[note % 7];
		rect.right = ((note + 1) * rcClient.Width()) / (m_nOctaves * 7);
		if (val == m_nSelection)
		{
			if (rect.left < xmin) xmin = rect.left;
			if (rect.right > xmax) xmax = rect.right;
		}
		if (rect.PtInRect(point))
		{
			sel = val;
			if (rect.left < xmin) xmin = rect.left;
			if (rect.right > xmax) xmax = rect.right;
		}
		rect.left = rect.right - 1;
	}
	// Black notes
	rect = rcClient;
	rect.bottom -= rcClient.Height() / 3;
	for (UINT nblack=0; nblack<m_nOctaves*7; nblack++)
	{
		switch(nblack % 7)
		{
		case 1:
		case 2:
		case 4:
		case 5:
		case 6:
			{
				int val = (nblack/7)*12 + blacktab[nblack % 7];
				rect.left = (nblack * rcClient.Width()) / (m_nOctaves * 7);
				rect.right = rect.left;
				int delta = rcClient.Width() / (m_nOctaves * 7 * 3);
				rect.left -= delta;
				rect.right += delta;
				if (val == m_nSelection)
				{
					if (rect.left < xmin) xmin = rect.left;
					if (rect.right > xmax) xmax = rect.right;
				}
				if (rect.PtInRect(point))
				{
					sel = val;
					if (rect.left < xmin) xmin = rect.left;
					if (rect.right > xmax) xmax = rect.right;
				}
			}
			break;
		}
	}
	// Check for selection change
	if (sel != m_nSelection)
	{
		m_nSelection = sel;
		rcClient.left = xmin;
		rcClient.right = xmax;
		InvalidateRect(&rcClient, FALSE);
		if ((m_bCursorNotify) && (m_hParent))
		{
			::PostMessage(m_hParent, WM_MOD_KBDNOTIFY, KBDNOTIFY_MOUSEMOVE, m_nSelection);
		}
	}
	if (sel >= 0)
	{
		if (!m_bCapture)
		{
			m_bCapture = TRUE;
			SetCapture();
		}
	} else
	{
		if (m_bCapture)
		{
			m_bCapture = FALSE;
			ReleaseCapture();
		}
	}
}


void CKeyboardControl::OnLButtonDown(UINT, CPoint)
//------------------------------------------------
{
	if ((m_nSelection != -1) && (m_hParent))
	{
		::SendMessage(m_hParent, WM_MOD_KBDNOTIFY, KBDNOTIFY_LBUTTONDOWN, m_nSelection);
	}
}


void CKeyboardControl::OnLButtonUp(UINT, CPoint)
//----------------------------------------------
{
	if ((m_nSelection != -1) && (m_hParent))
	{
		::SendMessage(m_hParent, WM_MOD_KBDNOTIFY, KBDNOTIFY_LBUTTONUP, m_nSelection);
	}
}


////////////////////////////////////////////////////////////////////////////////
//
// Sample Map
//

BEGIN_MESSAGE_MAP(CSampleMapDlg, CDialog)
	ON_MESSAGE(WM_MOD_KBDNOTIFY,	OnKeyboardNotify)
	ON_WM_HSCROLL()
	ON_COMMAND(IDC_CHECK1,			OnUpdateSamples)
	ON_CBN_SELCHANGE(IDC_COMBO1,	OnUpdateKeyboard)
END_MESSAGE_MAP()

void CSampleMapDlg::DoDataExchange(CDataExchange* pDX)
//----------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSampleMapDlg)
	DDX_Control(pDX, IDC_KEYBOARD1,		m_Keyboard);
	DDX_Control(pDX, IDC_COMBO1,		m_CbnSample);
	DDX_Control(pDX, IDC_SLIDER1,		m_SbOctave);
	//}}AFX_DATA_MAP
}


BOOL CSampleMapDlg::OnInitDialog()
//--------------------------------
{
	CDialog::OnInitDialog();
	if (m_pSndFile)
	{
		MODINSTRUMENT *pIns = m_pSndFile->Instruments[m_nInstrument];
		if (pIns)
		{
			for (UINT i=0; i<NOTE_MAX; i++)
			{
				KeyboardMap[i] = pIns->Keyboard[i];
			}
		}
	}
	m_Keyboard.Init(m_hWnd, 3, TRUE);
	m_SbOctave.SetRange(0, 7);
	m_SbOctave.SetPos(4);
	OnUpdateSamples();
	OnUpdateOctave();
	return TRUE;
}


VOID CSampleMapDlg::OnHScroll(UINT nCode, UINT nPos, CScrollBar *pBar)
//--------------------------------------------------------------------
{
	CDialog::OnHScroll(nCode, nPos, pBar);
	OnUpdateKeyboard();
	OnUpdateOctave();
}


VOID CSampleMapDlg::OnUpdateSamples()
//-----------------------------------
{
	UINT nOldPos = 0;
	UINT nNewPos = 0;
	BOOL bAll;
	
	if ((!m_pSndFile) || (m_nInstrument >= MAX_INSTRUMENTS)) return;
	if (m_CbnSample.GetCount() > 0)	{
		nOldPos = m_CbnSample.GetItemData(m_CbnSample.GetCurSel());
	}
	m_CbnSample.ResetContent();
	bAll = IsDlgButtonChecked(IDC_CHECK1);
	
	UINT nInsertPos;
	nInsertPos = m_CbnSample.AddString("0: No sample");
	m_CbnSample.SetItemData(nInsertPos, 0);

	for (UINT i=1; i<=m_pSndFile->m_nSamples; i++)	{
		BOOL bUsed = bAll;

		if (!bUsed)	{
			for (UINT j=0; j<NOTE_MAX; j++)	{
				if (KeyboardMap[j] == i) {
					bUsed = TRUE;
					break;
				}
			}
		}
		if (bUsed) {
			CString sampleName;
			sampleName.Format("%d: %s", i, m_pSndFile->GetSampleName(i));
			nInsertPos = m_CbnSample.AddString(sampleName);
			
			m_CbnSample.SetItemData(nInsertPos, i);
			if (i == nOldPos) nNewPos = nInsertPos;
		}
	}
	m_CbnSample.SetCurSel(nNewPos);
	OnUpdateKeyboard();
}


VOID CSampleMapDlg::OnUpdateOctave()
//----------------------------------
{
	CHAR s[64];

	UINT nBaseOctave = m_SbOctave.GetPos() & 7;
	wsprintf(s, "Octaves %d-%d", nBaseOctave, nBaseOctave+2);
	SetDlgItemText(IDC_TEXT1, s);
}



VOID CSampleMapDlg::OnUpdateKeyboard()
//------------------------------------
{
	UINT nSample = m_CbnSample.GetItemData(m_CbnSample.GetCurSel());
	UINT nBaseOctave = m_SbOctave.GetPos() & 7;
	BOOL bRedraw = FALSE;
	for (UINT iNote=0; iNote<3*12; iNote++)
	{
		UINT nOld = m_Keyboard.GetFlags(iNote);
		UINT ndx = nBaseOctave*12+iNote;
		UINT nNew = (KeyboardMap[ndx] == nSample) ? CKeyboardControl::KEYFLAG_REDDOT : CKeyboardControl::KEYFLAG_NORMAL;
		if (nNew != nOld)
		{
			m_Keyboard.SetFlags(iNote, nNew);
			bRedraw = TRUE;
		}
	}
	if (bRedraw) m_Keyboard.InvalidateRect(NULL, FALSE);
}


LRESULT CSampleMapDlg::OnKeyboardNotify(WPARAM wParam, LPARAM lParam)
//-------------------------------------------------------------------
{
	CHAR s[32] = "--";
	const size_t sizeofS = sizeof(s)/sizeof(s[0]);

	if ((lParam >= 0) && (lParam < 3*12) && (m_pSndFile))
	{
		UINT nSample = m_CbnSample.GetItemData(m_CbnSample.GetCurSel());
		UINT nBaseOctave = m_SbOctave.GetPos() & 7;
		
		const std::string temp = m_pSndFile->GetNoteName(lParam+1+12*nBaseOctave, m_nInstrument).c_str();
        if(temp.size() >= sizeofS)
			wsprintf(s, "%s", "...");
		else
			wsprintf(s, "%s", temp.c_str());

		MODINSTRUMENT *pIns = m_pSndFile->Instruments[m_nInstrument];
		if ((wParam == KBDNOTIFY_LBUTTONDOWN) && (nSample < MAX_SAMPLES) && (pIns))
		{
			UINT iNote = nBaseOctave*12+lParam;
			if (KeyboardMap[iNote] == nSample)
			{
				KeyboardMap[iNote] = pIns->Keyboard[iNote];
			} else
			{
				KeyboardMap[iNote] = (WORD)nSample;
			}
/* rewbs.note: I don't think we need this with cust keys.
// -> CODE#0009
// -> DESC="instrument editor note play & octave change"
			CMDIChildWnd *pMDIActive = CMainFrame::GetMainFrame() ? CMainFrame::GetMainFrame()->MDIGetActive() : NULL;
			CView *pView = pMDIActive ? pMDIActive->GetActiveView() : NULL;

			if(pView){
				CModDoc *pModDoc = (CModDoc *)pView->GetDocument();
				BOOL bNotPlaying = ((CMainFrame::GetMainFrame()->GetModPlaying() == pModDoc) && (CMainFrame::GetMainFrame()->IsPlaying())) ? FALSE : TRUE;
				pModDoc->PlayNote(iNote+1, m_nInstrument, 0, bNotPlaying);
			}
// -! BEHAVIOUR_CHANGE#0009
*/
			OnUpdateKeyboard();
		}
	}
	SetDlgItemText(IDC_TEXT2, s);
	return 0;
}


VOID CSampleMapDlg::OnOK()
//------------------------
{
	if (m_pSndFile)
	{
		MODINSTRUMENT *pIns = m_pSndFile->Instruments[m_nInstrument];
		if (pIns)
		{
			BOOL bModified = FALSE;
			for (UINT i=0; i<NOTE_MAX; i++)
			{
				if (KeyboardMap[i] != pIns->Keyboard[i])
				{
					pIns->Keyboard[i] = KeyboardMap[i];
					bModified = TRUE;
				}
			}
			if (bModified)
			{
				CDialog::OnOK();
				return;
			}
		}
	}
	CDialog::OnCancel();
}


////////////////////////////////////////////////////////////////////////////////////////////
// Edit history dialog

BEGIN_MESSAGE_MAP(CEditHistoryDlg, CDialog)
	ON_COMMAND(IDC_BTN_CLEAR,	OnClearHistory)
END_MESSAGE_MAP()


BOOL CEditHistoryDlg::OnInitDialog()
//----------------------------------
{
	CDialog::OnInitDialog();

	if(m_pModDoc == nullptr)
		return TRUE;

	CString s;
	uint64 totalTime = 0;
	const size_t num = m_pModDoc->GetFileHistory()->size();
	
	for(size_t n = 0; n < num; n++)
	{
		FileHistory *hist = &(m_pModDoc->GetFileHistory()->at(n));
		totalTime += hist->openTime;

		// Date
		TCHAR szDate[32];
		_tcsftime(szDate, sizeof(szDate), _T("%d %b %Y, %H:%M:%S"), &hist->loadDate);
		// Time + stuff
		uint32 duration = (uint32)((double)(hist->openTime) / HISTORY_TIMER_PRECISION);
		s.AppendFormat(_T("Loaded %s, open in the editor for %luh %02lum %02lus\r\n"),
			szDate, duration / 3600, (duration / 60) % 60, duration % 60);
	}
	if(num == 0)
	{
		s = _T("No information available about the previous edit history of this module.");
	}
	SetDlgItemText(IDC_EDIT_HISTORY, s);

	// Total edit time
	s = "";
	if(totalTime)
	{
		totalTime = (uint64)((double)(totalTime) / HISTORY_TIMER_PRECISION);

		s.Format(_T("Total edit time: %lluh %02llum %02llus (%u session%s)"), totalTime / 3600, (totalTime / 60) % 60, totalTime % 60, num, (num != 1) ? _T("s") : _T(""));
		SetDlgItemText(IDC_TOTAL_EDIT_TIME, s);
		// Window title
		s.Format(_T("Edit history for %s"), m_pModDoc->GetTitle());
		SetWindowText(s);
	}
	// Enable or disable Clear button
	GetDlgItem(IDC_BTN_CLEAR)->EnableWindow((m_pModDoc->GetFileHistory()->empty()) ? FALSE : TRUE);

	return TRUE;

}


void CEditHistoryDlg::OnClearHistory()
//------------------------------------
{
	if(m_pModDoc != nullptr && !m_pModDoc->GetFileHistory()->empty())
	{
		m_pModDoc->GetFileHistory()->clear();
		m_pModDoc->SetModified();
		OnInitDialog();
	}
}


void CEditHistoryDlg::OnOK()
//--------------------------
{
	CDialog::OnOK();
}


///////////////////////////////////////////////////////////////////////////////////////
// Messagebox with 'don't show again'-option.

//===================================
class CMsgBoxHidable : public CDialog
//===================================
{
public:
	CMsgBoxHidable(LPCTSTR strMsg, bool checkStatus = true, CWnd* pParent = NULL);
	enum { IDD = IDD_MSGBOX_HIDABLE };

	int m_nCheckStatus;
	LPCTSTR m_StrMsg;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);   // DDX/DDV support
	virtual BOOL OnInitDialog();
};


struct MsgBoxHidableMessage
//=========================
{
	LPCTSTR strMsg;
	uint32 nMask;
	bool bDefaultDontShowAgainStatus; // true for don't show again, false for show again.
};

const MsgBoxHidableMessage HidableMessages[] =
{
	{TEXT("Tip: To create ProTracker compatible MOD-files, try compatibility export from File-menu."), 1, true},
	{TEXT("Tip: To create IT-files without MPT-specific extensions included, try compatibility export from File-menu."), 1 << 1, true},
	{TEXT("Press OK to apply signed/unsigned conversion\n (note: this often significantly increases volume level)"), 1 << 2, false},
	{TEXT("Tip: To create XM-files without MPT-specific extensions included, try compatibility export from File-menu."), 1 << 1, true},
};

STATIC_ASSERT(ARRAYELEMCOUNT(HidableMessages) == enMsgBoxHidableMessage_count);

// Messagebox with 'don't show this again'-checkbox. Uses parameter 'enMsg'
// to get the needed information from message array, and updates the variable that
// controls the show/don't show-flags.
void MsgBoxHidable(enMsgBoxHidableMessage enMsg)
//----------------------------------------------
{
	// Check whether the message should be shown.
	if((CMainFrame::gnMsgBoxVisiblityFlags & HidableMessages[enMsg].nMask) == 0)
		return;

	const LPCTSTR strMsg = HidableMessages[enMsg].strMsg;
	const uint32 mask = HidableMessages[enMsg].nMask;
	const bool defaulCheckStatus = HidableMessages[enMsg].bDefaultDontShowAgainStatus;

	// Show dialog.
	CMsgBoxHidable dlg(strMsg, defaulCheckStatus);
	dlg.DoModal();

	// Update visibility flags.
	if(dlg.m_nCheckStatus == BST_CHECKED)
		CMainFrame::gnMsgBoxVisiblityFlags &= ~mask;
	else
		CMainFrame::gnMsgBoxVisiblityFlags |= mask;
}


CMsgBoxHidable::CMsgBoxHidable(LPCTSTR strMsg, bool checkStatus, CWnd* pParent)
	:	CDialog(CMsgBoxHidable::IDD, pParent),
		m_StrMsg(strMsg),
		m_nCheckStatus((checkStatus) ? BST_CHECKED : BST_UNCHECKED)
//----------------------------------------------------------------------------
{}

BOOL CMsgBoxHidable::OnInitDialog()
//----------------------------------
{
	CDialog::OnInitDialog();
	SetDlgItemText(IDC_MESSAGETEXT, m_StrMsg);
	SetWindowText(AfxGetAppName());
	return TRUE;
}

void CMsgBoxHidable::DoDataExchange(CDataExchange* pDX)
//------------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_DONTSHOWAGAIN, m_nCheckStatus);
}


/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

const LPCTSTR szNullNote = TEXT("...");
const LPCTSTR szUnknownNote = TEXT("???");


LPCTSTR GetNoteStr(const MODCOMMAND::NOTE nNote)
//----------------------------------------------
{
	if(nNote == 0)
		return szNullNote;

	if(nNote >= 1 && nNote <= NOTE_MAX)
	{
		return szDefaultNoteNames[nNote-1];
	}
	else if(nNote >= NOTE_MIN_SPECIAL && nNote <= NOTE_MAX_SPECIAL)
	{
		return szSpecialNoteNames[nNote - NOTE_MIN_SPECIAL];
	}
	else
		return szUnknownNote;
}

	
void AppendNotesToControl(CComboBox& combobox, const MODCOMMAND::NOTE noteStart, const MODCOMMAND::NOTE noteEnd)
//------------------------------------------------------------------------------------------------------------------
{
	const MODCOMMAND::NOTE upperLimit = min(ARRAYELEMCOUNT(szDefaultNoteNames)-1, noteEnd);
	for(MODCOMMAND::NOTE note = noteStart; note <= upperLimit; ++note)
		combobox.SetItemData(combobox.AddString(szDefaultNoteNames[note]), note);
}


void AppendNotesToControlEx(CComboBox& combobox, const CSoundFile* const pSndFile /* = nullptr*/, const INSTRUMENTINDEX nInstr/* = MAX_INSTRUMENTS*/)
//----------------------------------------------------------------------------------------------------------------------------------
{
	const MODCOMMAND::NOTE noteStart = (pSndFile != nullptr) ? pSndFile->GetModSpecifications().noteMin : 1;
	const MODCOMMAND::NOTE noteEnd = (pSndFile != nullptr) ? pSndFile->GetModSpecifications().noteMax : NOTE_MAX;
	for(MODCOMMAND::NOTE nNote = noteStart; nNote <= noteEnd; nNote++)
	{
		if(pSndFile != nullptr && nInstr != MAX_INSTRUMENTS)
			combobox.SetItemData(combobox.AddString(pSndFile->GetNoteName(nNote, nInstr).c_str()), nNote);
		else
			combobox.SetItemData(combobox.AddString(szDefaultNoteNames[nNote-1]), nNote);
	}
	for(MODCOMMAND::NOTE nNote = NOTE_MIN_SPECIAL-1; nNote++ < NOTE_MAX_SPECIAL;)
	{
		if(pSndFile == nullptr || pSndFile->GetModSpecifications().HasNote(nNote) == true)
			combobox.SetItemData(combobox.AddString(szSpecialNoteNames[nNote-NOTE_MIN_SPECIAL]), nNote);
	}
}
