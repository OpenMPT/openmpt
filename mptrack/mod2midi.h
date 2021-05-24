/*
 * mod2midi.h
 * ----------
 * Purpose: Module to MIDI conversion (dialog + conversion code).
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#ifndef NO_PLUGINS
#include "ProgressDialog.h"

OPENMPT_NAMESPACE_BEGIN


namespace MidiExport
{
	struct Mod2MidiInstr
	{
		uint8 channel = MidiMappedChannel; // See enum MidiChannel
		uint8 program = 0;
	};
	using InstrMap = std::vector<Mod2MidiInstr>;
}


class CModToMidi: public CDialog
{
protected:
	CComboBox m_CbnInstrument, m_CbnChannel, m_CbnProgram;
	CSpinButtonCtrl m_SpinInstrument;
	CSoundFile &m_sndFile;
	UINT m_currentInstr;
	bool m_percussion;
public:
	MidiExport::InstrMap m_instrMap;
	static bool s_overlappingInstruments;

public:
	CModToMidi(CSoundFile &sndFile, CWnd *pWndParent = nullptr);

protected:
	void OnOK() override;
	BOOL OnInitDialog() override;
	void DoDataExchange(CDataExchange *pDX) override;
	void FillProgramBox(bool percussion);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void UpdateDialog();
	afx_msg void OnChannelChanged();
	afx_msg void OnProgramChanged();
	afx_msg void OnOverlapChanged();
	DECLARE_MESSAGE_MAP();
};


class CDoMidiConvert: public CProgressDialog
{
public:
	CSoundFile &m_sndFile;
	mpt::ofstream &m_file;
	const MidiExport::InstrMap &m_instrMap;

public:
	CDoMidiConvert(CSoundFile &sndFile, mpt::ofstream &f, const MidiExport::InstrMap &instrMap, CWnd *parent = nullptr)
		: CProgressDialog(parent)
		, m_sndFile(sndFile)
		, m_file(f)
		, m_instrMap(instrMap)
	{ }
	void Run() override;
};


OPENMPT_NAMESPACE_END

#endif // NO_PLUGINS
