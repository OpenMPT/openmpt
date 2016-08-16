/*
 * mod2midi.h
 * ----------
 * Purpose: Module to MIDI conversion (dialog + conversion code).
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#ifndef NO_PLUGINS
#include "ProgressDialog.h"

OPENMPT_NAMESPACE_BEGIN


namespace MidiExport
{
	struct Mod2MidiInstr
	{
		uint8 nChannel; // See enum MidiChannel
		uint8 nProgram;
	};
	typedef std::vector<Mod2MidiInstr> InstrMap;
}


//==============================
class CModToMidi: public CDialog
//==============================
{
protected:
	CComboBox m_CbnInstrument, m_CbnChannel, m_CbnProgram;
	CSpinButtonCtrl m_SpinInstrument;
	CSoundFile &m_sndFile;
	UINT m_nCurrInstr;
	bool m_bPerc;
public:
	MidiExport::InstrMap m_instrMap;

public:
	CModToMidi(CSoundFile &sndFile, CWnd *pWndParent = nullptr);

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange *pDX);
	void FillProgramBox(bool percussion);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void UpdateDialog();
	afx_msg void OnChannelChanged();
	afx_msg void OnProgramChanged();
	DECLARE_MESSAGE_MAP();
};


//==========================================
class CDoMidiConvert: public CProgressDialog
//==========================================
{
public:
	CSoundFile &m_sndFile;
	const mpt::PathString m_fileName;
	const MidiExport::InstrMap &m_instrMap;

public:
	CDoMidiConvert(CSoundFile &sndFile, const mpt::PathString &filename, const MidiExport::InstrMap &instrMap, CWnd *parent = nullptr)
		: CProgressDialog(parent)
		, m_sndFile(sndFile)
		, m_fileName(filename)
		, m_instrMap(instrMap)
	{ }
	void Run();
};


OPENMPT_NAMESPACE_END

#endif // NO_PLUGINS
