/*
 * Autotune.h
 * ----------
 * Purpose: Header file for sample auto tuning
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 */


#pragma once
#ifndef AUTOTUNE_H
#define AUTOTUNE_H

#include "../soundlib/Snd_defs.h"

//============
class Autotune
//============
{
protected:
	MODSAMPLE &sample;
	MODTYPE modType;

	SmpLength selectionStart, selectionEnd;

	int8 *sampleData;
	SmpLength sampleLength;

public:
	Autotune(MODSAMPLE &smp, MODTYPE type, SmpLength selStart, SmpLength selEnd) : sample(smp), modType(type), selectionStart(selStart), selectionEnd(selEnd)
	{
		sampleData = 0;
		sampleLength = 0;
	};

	~Autotune()
	{
		delete[] sampleData;
	}

	bool CanApply() const;
	bool Apply();

protected:
	double FrequencyToNote(double freq) const;
	double NoteToFrequency(double note) const;
	SmpLength NoteToShift(uint32 sampleFreq, int note) const;

	template <class T>
	void CopySamples(const T* origSample, SmpLength sampleLoopStart, SmpLength sampleLoopEnd);

	bool PrepareSample(SmpLength maxShift);

};

#endif // AUTOTUNE_H
