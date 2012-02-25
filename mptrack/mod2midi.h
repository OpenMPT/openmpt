/*
 * mod2midi.h
 * ----------
 * Purpose: Module to MIDI conversion (dialog + conversion code).
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

struct MOD2MIDIINSTR
{
	UINT nChannel;
	uint8 nProgram;
};


//==============================
class CModToMidi: public CDialog
//==============================
{
protected:
	CComboBox m_CbnInstrument, m_CbnChannel, m_CbnProgram;
	CSpinButtonCtrl m_SpinInstrument;
	CSoundFile *m_pSndFile;
	BOOL m_bRmi, m_bPerc;
	UINT m_nCurrInstr;
	CHAR m_szFileName[_MAX_PATH];
	MOD2MIDIINSTR m_InstrMap[MAX_SAMPLES];

public:
	CModToMidi(LPCSTR pszFileName, CSoundFile *pSndFile, CWnd *pWndParent=NULL);
	~CModToMidi() {}
	BOOL DoConvert();

protected:
	virtual BOOL OnInitDialog();
	virtual VOID DoDataExchange(CDataExchange *pDX);
	virtual VOID OnOK();
	VOID FillProgramBox(BOOL bPerc);
	afx_msg VOID OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg VOID UpdateDialog();
	afx_msg VOID OnChannelChanged();
	afx_msg VOID OnProgramChanged();
	DECLARE_MESSAGE_MAP();
};
