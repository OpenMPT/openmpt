/*
 * OPLInstrDlg.cpp
 * ---------------
 * Purpose: Editor for OPL-based synth instruments
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"
#include "OPLInstrDlg.h"
#include "../soundlib/OPL.h"
#include "resource.h"
#include "Mainfrm.h"

OPENMPT_NAMESPACE_BEGIN

BEGIN_MESSAGE_MAP(OPLInstrDlg, CDialog)
	ON_WM_HSCROLL()
	ON_MESSAGE(WM_MOD_DRAGONDROPPING, &OPLInstrDlg::OnDragonDropping)
	ON_COMMAND(IDC_CHECK1, &OPLInstrDlg::ParamsChanged)
	ON_COMMAND(IDC_CHECK2, &OPLInstrDlg::ParamsChanged)
	ON_COMMAND(IDC_CHECK3, &OPLInstrDlg::ParamsChanged)
	ON_COMMAND(IDC_CHECK4, &OPLInstrDlg::ParamsChanged)
	ON_COMMAND(IDC_CHECK5, &OPLInstrDlg::ParamsChanged)
	ON_COMMAND(IDC_CHECK6, &OPLInstrDlg::ParamsChanged)
	ON_COMMAND(IDC_CHECK7, &OPLInstrDlg::ParamsChanged)
	ON_COMMAND(IDC_CHECK8, &OPLInstrDlg::ParamsChanged)
	ON_COMMAND(IDC_CHECK9, &OPLInstrDlg::ParamsChanged)
	ON_CBN_SELCHANGE(IDC_COMBO1, &OPLInstrDlg::ParamsChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2, &OPLInstrDlg::ParamsChanged)
END_MESSAGE_MAP()


void OPLInstrDlg::DoDataExchange(CDataExchange *pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHECK1, m_additive);
	DDX_Control(pDX, IDC_SLIDER1, m_feedback);

	for(int op = 0; op < 2; op++)
	{
		const int slider = op * 7;
		const int check = op * 4;
		DDX_Control(pDX, IDC_SLIDER2 + slider, m_attackRate[op]);
		DDX_Control(pDX, IDC_SLIDER3 + slider, m_decayRate[op]);
		DDX_Control(pDX, IDC_SLIDER4 + slider, m_sustainLevel[op]);
		DDX_Control(pDX, IDC_SLIDER5 + slider, m_releaseRate[op]);
		DDX_Control(pDX, IDC_CHECK2  + check,  m_sustain[op]);
		DDX_Control(pDX, IDC_SLIDER6 + slider, m_volume[op]);
		DDX_Control(pDX, IDC_CHECK3  + check,  m_scaleEnv[op]);
		DDX_Control(pDX, IDC_SLIDER7 + slider, m_levelScaling[op]);
		DDX_Control(pDX, IDC_SLIDER8 + slider, m_freqMultiplier[op]);
		DDX_Control(pDX, IDC_COMBO1  + op,     m_waveform[op]);
		DDX_Control(pDX, IDC_CHECK4  + check,  m_vibrato[op]);
		DDX_Control(pDX, IDC_CHECK5  + check,  m_tremolo[op]);
	}
}


OPLInstrDlg::OPLInstrDlg(CWnd &parent)
	: m_parent(parent)
{
	Create(IDD_OPL_PARAMS, &parent);
	CRect rect;
	GetClientRect(rect);
	m_windowSize = rect.BottomRight();
	SetParent(&parent);
	ModifyStyle(WS_POPUP, WS_CHILD);
}


OPLInstrDlg::~OPLInstrDlg()
{
	DestroyWindow();
}


BOOL OPLInstrDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	m_feedback.SetRange(0, 7);
	for(int op = 0; op < 2; op++)
	{
		m_attackRate[op].SetRange(0, 15);
		m_decayRate[op].SetRange(0, 15);
		m_sustainLevel[op].SetRange(0, 15);
		m_releaseRate[op].SetRange(0, 15);
		m_volume[op].SetRange(0, 63);
		m_volume[op].SetTicFreq(4);
		m_levelScaling[op].SetRange(0, 3);
		m_freqMultiplier[op].SetRange(0, 15);
	}

	return TRUE;
}


BOOL OPLInstrDlg::PreTranslateMessage(MSG *pMsg)
{
	if(pMsg)
	{
		// Forward key presses and drag&drop support to parent editor
		if(pMsg->message == WM_SYSKEYUP || pMsg->message == WM_KEYUP ||
			pMsg->message == WM_SYSKEYDOWN || pMsg->message == WM_KEYDOWN ||
			pMsg->message == WM_DROPFILES)
		{
			if(pMsg->hwnd == m_hWnd)
			{
				pMsg->hwnd = m_parent.m_hWnd;
			}
			if(m_parent.PreTranslateMessage(pMsg))
			{
				return TRUE;
			}
		}
	}
	return CDialog::PreTranslateMessage(pMsg);
}


LRESULT OPLInstrDlg::OnDragonDropping(WPARAM wParam, LPARAM lParam)
{
	return m_parent.SendMessage(WM_MOD_DRAGONDROPPING, wParam, lParam);
}


// Swap OPL Key Scale Level bits for a "human-readable" value.
static uint8 KeyScaleLevel(uint8 kslVolume)
{
	static const uint8 KSLFix[4] = { 0x00, 0x80, 0x40, 0xC0 };
	return KSLFix[kslVolume >> 6];
}


void OPLInstrDlg::SetPatch(OPLPatch &patch)
{
	SetRedraw(FALSE);
	m_additive.SetCheck((patch[10] & OPL::CONNECTION_BIT) ? BST_CHECKED : BST_UNCHECKED);
	m_feedback.SetPos((patch[10] & OPL::FEEDBACK_MASK) >> 1);
	for(int op = 0; op < 2; op++)
	{
		m_attackRate[op].SetPos(15 - (patch[4 + op] >> 4));
		m_decayRate[op].SetPos(15 - (patch[4 + op] & 0x0F));
		m_sustainLevel[op].SetPos(15 - (patch[6 + op] >> 4));
		m_releaseRate[op].SetPos(15 - (patch[6 + op] & 0x0F));
		m_volume[op].SetPos(63 - (patch[2 + op] & OPL::TOTAL_LEVEL_MASK));
		m_levelScaling[op].SetPos(KeyScaleLevel(patch[2 + op]) >> 6);
		m_freqMultiplier[op].SetPos(patch[0 + op] & OPL::MULTIPLE_MASK);

		m_sustain[op].SetCheck((patch[0 + op] & OPL::SUSTAIN_ON) ? BST_CHECKED : BST_UNCHECKED);
		m_scaleEnv[op].SetCheck((patch[0 + op] & OPL::KSR) ? BST_CHECKED : BST_UNCHECKED);
		m_vibrato[op].SetCheck((patch[0 + op] & OPL::VIBRATO_ON) ? BST_CHECKED : BST_UNCHECKED);
		m_tremolo[op].SetCheck((patch[0 + op] & OPL::TREMOLO_ON) ? BST_CHECKED : BST_UNCHECKED);

		m_waveform[op].SetCurSel(patch[8 + op]);
	}
	SetRedraw(TRUE);
	m_patch = &patch;
}


void OPLInstrDlg::ParamsChanged()
{
	OPLPatch patch{{}};
	if(m_additive.GetCheck() != BST_UNCHECKED) patch[10] |= OPL::CONNECTION_BIT;
	patch[10] |= static_cast<uint8>(m_feedback.GetPos() << 1);
	for(int op = 0; op < 2; op++)
	{
		patch[op] = static_cast<uint8>(m_freqMultiplier[op].GetPos());
		if(m_sustain[op].GetCheck() != BST_UNCHECKED) patch[op] |= OPL::SUSTAIN_ON;
		if(m_scaleEnv[op].GetCheck() != BST_UNCHECKED) patch[op] |= OPL::KSR;
		if(m_vibrato[op].GetCheck() != BST_UNCHECKED) patch[op] |= OPL::VIBRATO_ON;
		if(m_tremolo[op].GetCheck() != BST_UNCHECKED) patch[op] |= OPL::TREMOLO_ON;
		patch[2 + op] = static_cast<uint8>((63 - m_volume[op].GetPos()) | KeyScaleLevel(static_cast<uint8>(m_levelScaling[op].GetPos() << 6)));
		patch[4 + op] = static_cast<uint8>(((15 - m_attackRate[op].GetPos()) << 4) | (15 - m_decayRate[op].GetPos()));
		patch[6 + op] = static_cast<uint8>(((15 - m_sustainLevel[op].GetPos()) << 4) | (15 - m_releaseRate[op].GetPos()));
		patch[8 + op] = static_cast<uint8>(m_waveform[op].GetCurSel());
	}

	if(*m_patch != patch)
	{
		m_parent.SendMessage(WM_MOD_VIEWMSG, VIEWMSG_PREPAREUNDO);
		*m_patch = patch;
		m_parent.SendMessage(WM_MOD_VIEWMSG, VIEWMSG_SETMODIFIED, SampleHint().Data().AsLPARAM());
	}
}

OPENMPT_NAMESPACE_END
