/*
 * Autotune.cpp
 * -------------
 * Purpose: Class for tuning a sample to the next C automatically.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 */


#include "stdafx.h"
#include <math.h>
#include "../common/misc_util.h"
#include "../soundlib/Sndfile.h"
#include "Autotune.h"


// The more bins, the more autocorrelations are done and the more precise the result is.
#define BINS_PER_NOTE 32
#define MIN_SAMPLE_LENGTH 2


double Autotune::FrequencyToNote(double freq) const
//-------------------------------------------------
{
	return ((12.0 * (log(freq / (440.0 / 2.0)) / log(2.0))) + 57.0);
}


double Autotune::NoteToFrequency(double note) const
//-------------------------------------------------
{
	return 440.0 * pow(2.0, (note - 69.0) / 12.0);
}


// Calculate the amount of samples for autocorrelation shifting for a given note
SmpLength Autotune::NoteToShift(uint32 sampleFreq, int note) const
//-------------------------------------------------------------------
{
	const double fundamentalFrequency = NoteToFrequency((double)note / BINS_PER_NOTE);
	return (SmpLength)max(Util::Round((double)sampleFreq / fundamentalFrequency), 1);
}


// Create an 8-Bit sample buffer with loop unrolling and mono conversion for autocorrelation.
template <class T>
void Autotune::CopySamples(const T* origSample, SmpLength sampleLoopStart, SmpLength sampleLoopEnd)
//-------------------------------------------------------------------------------------------------
{
	const uint8 channels = sample.GetNumChannels();
	sampleLoopStart *= channels;
	sampleLoopEnd *= channels;

	for(SmpLength i = 0, pos = 0; i < sampleLength; i++, pos += channels)
	{
		if(pos >= sampleLoopEnd)
		{
			pos = sampleLoopStart;
		}

		const T* sample = origSample + pos;

		int16 data = 0;	// Enough for 256 channels... :)
		for(uint8 chn = 0; chn < channels; chn++)
		{
			// We only want the MSB.
			data += static_cast<int16>(sample[chn] >> ((sizeof(T) - 1) * 8));
		}

		data /= channels;

		sampleData[i] = static_cast<int8>(data);
	}
}

	
// Prepare a sample buffer for autocorrelation
bool Autotune::PrepareSample(SmpLength maxShift)
//----------------------------------------------
{

	// Determine which parts of the sample should be examined.
	SmpLength sampleOffset = 0, sampleLoopStart = 0, sampleLoopEnd = sample.nLength;
	if(selectionEnd >= sampleLoopStart + MIN_SAMPLE_LENGTH)
	{
		// A selection has been specified: Examine selection
		sampleOffset = selectionStart;
		sampleLoopStart = 0;
		sampleLoopEnd = selectionEnd - selectionStart;
	} else if((sample.uFlags & CHN_SUSTAINLOOP) && sample.nSustainEnd >= sample.nSustainStart + MIN_SAMPLE_LENGTH)
	{
		// A sustain loop is set: Examine sample up to sustain loop and, if necessary, execute the loop several times
		sampleOffset = 0;
		sampleLoopStart = sample.nSustainStart;
		sampleLoopEnd = sample.nSustainEnd;
	} else if((sample.uFlags & CHN_LOOP) && sample.nLoopEnd >= sample.nLoopStart + MIN_SAMPLE_LENGTH)
	{
		// A normal loop is set: Examine sample up to loop and, if necessary, execute the loop several times
		sampleOffset = 0;
		sampleLoopStart = sample.nLoopStart;
		sampleLoopEnd = sample.nLoopEnd;
	}

	// We should analyse at least a one second (= GetSampleRate() samples) long sample.
	sampleLength = max(sampleLoopEnd, sample.GetSampleRate(modType)) + maxShift;

	if(sampleData != nullptr)
	{
		delete[] sampleData;
	}
	try
	{
		sampleData = new int8[sampleLength];
	} catch(MPTMemoryException)
	{
		return false;
	}

	// Copy sample over.
	switch(sample.GetElementarySampleSize())
	{
	case 1:
		CopySamples(reinterpret_cast<int8 *>(sample.pSample) + sampleOffset * sample.GetNumChannels(), sampleLoopStart, sampleLoopEnd);
		return true;

	case 2:
		CopySamples(reinterpret_cast<int16 *>(sample.pSample) + sampleOffset * sample.GetNumChannels(), sampleLoopStart, sampleLoopEnd);
		return true;
	}

	return false;

}


bool Autotune::CanApply() const
	//-----------------------------
{
	return (sample.pSample != nullptr && sample.nLength >= MIN_SAMPLE_LENGTH);
}


bool Autotune::Apply()
//--------------------
{
	if(!CanApply())
	{
		return false;
	}

	const int autocorrStartNote = 24 * BINS_PER_NOTE;	// C-2
	const int autocorrEndNote = 96 * BINS_PER_NOTE;		// C-8
	const int historyBins = 12 * BINS_PER_NOTE;			// One octave

	const uint32 sampleFreq = sample.GetSampleRate(modType);
	// At the lowest frequency, we get the highest autocorrelation shift amount.
	const SmpLength maxShift = NoteToShift(sampleFreq, autocorrStartNote);
	if(!PrepareSample(maxShift))
	{
		return false;
	}
	// We don't process the autocorrelation overhead.
	const SmpLength processLength = sampleLength - maxShift;

	// Histogram for all notes.
	vector<uint64> autocorrHistogram(historyBins, 0);

	// Do autocorrelation and save results in a note histogram (restriced to one octave).
	for(int note = autocorrStartNote, noteBin = note; note < autocorrEndNote; note++, noteBin++)
	{

		if(noteBin >= historyBins)
		{
			noteBin %= historyBins;
		}

		const SmpLength autocorrShift = NoteToShift(sampleFreq, note);

		uint64 autocorrSum = 0;
		const int8 *normalData = sampleData;
		const int8 *shiftedData = sampleData + autocorrShift;
		// Add up squared differences of all values
		for(SmpLength i = processLength; i != 0; i--, normalData++, shiftedData++)
		{
			autocorrSum += (*normalData - *shiftedData) * (*normalData - *shiftedData);
		}
		autocorrHistogram[noteBin] += autocorrSum;

	}

	// Interpolate the histogram...
	vector<uint64> interpolatedHistogram(historyBins, 0);
	for(int i = 0; i < historyBins; i++)
	{
		const int kernelWidth = 4;
		for(int ki = kernelWidth; ki >= 0; ki--)
		{
			// Choose bins to interpolate with
			int left = i - ki;
			if(left < 0) left += historyBins;
			int right = i + ki;
			if(right >= historyBins) right -= historyBins;

			interpolatedHistogram[i] = interpolatedHistogram[i] / 2 + (autocorrHistogram[left] + autocorrHistogram[right]) / 2;
		}
	}

	// ...and find global minimum
	int minimumBin = 0;
	//bool decrease = false;
	for(int i = 0; i < historyBins; i++)
	{
		const int prev = (i > 0) ? (i - 1) : (historyBins - 1);
		// Are we at a minimum?
		//if(interpolatedHistogram[i] > interpolatedHistogram[prev] && decrease)
		{
			// Are we at the global minimum?
			if(interpolatedHistogram[prev] < interpolatedHistogram[minimumBin])
			{
				minimumBin = prev;
			}
		}
		//decrease = (interpolatedHistogram[i] < interpolatedHistogram[prev]);

	}

	// Center around C
	if(minimumBin >= 6 * BINS_PER_NOTE) minimumBin -= 12 * BINS_PER_NOTE;
	const double newFundamentalFreq = NoteToFrequency(69.0 + (double)minimumBin / BINS_PER_NOTE);

	sample.nC5Speed = (UINT)Util::Round(sample.nC5Speed * 440.0 / newFundamentalFreq);

	if((modType & (MOD_TYPE_XM | MOD_TYPE_MOD)) != 0)
	{
		CSoundFile::FrequencyToTranspose(&sample);
		if((modType & MOD_TYPE_MOD) != 0)
		{
			sample.RelativeTone = 0;
		}
	}

	return true;
}
