/*
 * Autotune.h
 * ----------
 * Purpose: Class for tuning a sample to a given base note automatically.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "../soundlib/Snd_defs.h"
#include "resource.h"


OPENMPT_NAMESPACE_BEGIN


struct ModSample;


class Autotune
{
protected:
	ModSample &m_sample;
	MODTYPE m_modType;

	SmpLength m_selectionStart, m_selectionEnd;

	int16 *m_sampleData = nullptr;
	SmpLength m_sampleLength = 0;

public:
	Autotune(ModSample &smp, MODTYPE type, SmpLength selStart, SmpLength selEnd) : m_sample(smp), m_modType(type), m_selectionStart(selStart), m_selectionEnd(selEnd)
	{ };

	~Autotune()
	{
		delete[] m_sampleData;
	}

	bool CanApply() const;
	bool Apply(double pitchReference, int targetNote);

protected:

	template <class T>
	void CopySamples(const T* origSample, SmpLength sampleLoopStart, SmpLength sampleLoopEnd);

	bool PrepareSample(SmpLength maxShift);

};


class CAutotuneDlg : public CDialog
{
protected:
	static int m_pitchReference;	// Pitch reference (440Hz by default)
	static int m_targetNote;		// Note which the sample should be tuned to (C by default)

	CComboBox m_CbnNoteBox;

public:
	CAutotuneDlg(CWnd *parent) : CDialog(IDD_AUTOTUNE, parent)
	{ };

	int GetPitchReference() const { return m_pitchReference; }
	int GetTargetNote() const { return m_targetNote; }

protected:
	BOOL OnInitDialog() override;
	void OnOK() override;
	void DoDataExchange(CDataExchange* pDX) override;

};


OPENMPT_NAMESPACE_END
