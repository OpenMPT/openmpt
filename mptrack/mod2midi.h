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
#include "DialogBase.h"
#include "ProgressDialog.h"
#include "../soundlib/Snd_defs.h"

OPENMPT_NAMESPACE_BEGIN

class CModDoc;
struct SubSong;

namespace MidiExport
{
	struct Mod2MidiInstr
	{
		uint8 channel = MidiMappedChannel; // See enum MidiChannel
		uint8 program = 0;
	};
	using InstrMap = std::vector<Mod2MidiInstr>;
}


class CModToMidi : public CProgressDialog
{
protected:
	CComboBox m_CbnInstrument, m_CbnChannel, m_CbnProgram;
	CSpinButtonCtrl m_SpinInstrument;
	CModDoc &m_modDoc;
	MidiExport::InstrMap m_instrMap;
	std::vector<SubSong> m_subSongs;
	size_t m_selectedSong = 0;
	size_t m_currentInstr = 1;
	bool m_percussion = false;
	bool m_conversionRunning = false;
	bool m_locked = true;
public:
	static bool s_overlappingInstruments;

public:
	CModToMidi(CModDoc &modDoc, CWnd *parent = nullptr);
	~CModToMidi();

protected:
	void Run() override {};
	
	void UpdateSubsongName();
	void DoConversion(const mpt::PathString &fileName);

	void OnOK() override;
	void OnCancel() override;
	BOOL OnInitDialog() override;
	void DoDataExchange(CDataExchange *pDX) override;
	void FillProgramBox(bool percussion);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void UpdateDialog();
	afx_msg void OnChannelChanged();
	afx_msg void OnProgramChanged();
	afx_msg void OnOverlapChanged();
	afx_msg void OnSubsongChanged();

	DECLARE_MESSAGE_MAP();
};


OPENMPT_NAMESPACE_END

#endif // NO_PLUGINS
