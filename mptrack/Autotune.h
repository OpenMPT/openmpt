/*
 * Autotune.h
 * ----------
 * Purpose: Class for tuning a sample to a given base note automatically.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "../soundlib/Snd_defs.h"
#include "resource.h"

OPENMPT_NAMESPACE_BEGIN

struct AutotuneThreadData;

//============
class Autotune
//============
{
protected:
	ModSample &sample;
	MODTYPE modType;

	SmpLength selectionStart, selectionEnd;

	int16 *sampleData;
	SmpLength sampleLength;

public:
	Autotune(ModSample &smp, MODTYPE type, SmpLength selStart, SmpLength selEnd) : sample(smp), modType(type), selectionStart(selStart), selectionEnd(selEnd)
	{
		sampleData = nullptr;
		sampleLength = 0;
	};

	~Autotune()
	{
		delete[] sampleData;
	}

	bool CanApply() const;
	bool Apply(double pitchReference, int targetNote);

protected:
	static double FrequencyToNote(double freq, double pitchReference);
	static double NoteToFrequency(double note, double pitchReference);
	static SmpLength NoteToShift(uint32 sampleFreq, int note, double pitchReference);

	template <class T>
	void CopySamples(const T* origSample, SmpLength sampleLoopStart, SmpLength sampleLoopEnd);

	bool PrepareSample(SmpLength maxShift);

	static void AutotuneThread(AutotuneThreadData &info);
};


//=================================
class CAutotuneDlg : public CDialog
//=================================
{
protected:
	static int pitchReference;	// Pitch reference (440Hz by default)
	static int targetNote;		// Note which the sample should be tuned to (C by default)

	CComboBox m_CbnNoteBox;

public:
	CAutotuneDlg(CWnd *parent) : CDialog(IDD_AUTOTUNE, parent)
	{ };

	int GetPitchReference() const { return pitchReference; }
	int GetTargetNote() const { return targetNote; }

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	virtual void DoDataExchange(CDataExchange* pDX);

};


OPENMPT_NAMESPACE_END
