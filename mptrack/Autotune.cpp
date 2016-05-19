/*
 * Autotune.cpp
 * ------------
 * Purpose: Class for tuning a sample to a given base note automatically.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include <math.h>
#include "../common/misc_util.h"
#include "../common/thread.h"
#include "../soundlib/Sndfile.h"
#include "Autotune.h"
#ifdef ENABLE_SSE2
#include <emmintrin.h>
#endif


OPENMPT_NAMESPACE_BEGIN


// The more bins, the more autocorrelations are done and the more precise the result is.
#define BINS_PER_NOTE 32
#define MIN_SAMPLE_LENGTH 2

#define START_NOTE		(24 * BINS_PER_NOTE)	// C-2
#define END_NOTE		(96 * BINS_PER_NOTE)	// C-8
#define HISTORY_BINS	(12 * BINS_PER_NOTE)	// One octave


double Autotune::FrequencyToNote(double freq, double pitchReference)
//------------------------------------------------------------------
{
	return ((12.0 * (log(freq / (pitchReference / 2.0)) / log(2.0))) + 57.0);
}


double Autotune::NoteToFrequency(double note, double pitchReference)
//------------------------------------------------------------------
{
	return pitchReference * pow(2.0, (note - 69.0) / 12.0);
}


// Calculate the amount of samples for autocorrelation shifting for a given note
SmpLength Autotune::NoteToShift(uint32 sampleFreq, int note, double pitchReference)
//---------------------------------------------------------------------------------
{
	const double fundamentalFrequency = NoteToFrequency((double)note / BINS_PER_NOTE, pitchReference);
	return std::max(Util::Round<SmpLength>((double)sampleFreq / fundamentalFrequency), SmpLength(1));
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

		const T* smp = origSample + pos;

		int32 data = 0;	// More than enough for 256 channels... :)
		for(uint8 chn = 0; chn < channels; chn++)
		{
			// We only want the MSB.
			data += static_cast<int32>(smp[chn] >> ((sizeof(T) - 1) * 8));
		}

		data /= channels;

		sampleData[i] = static_cast<int16>(data);
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
	} else if(sample.uFlags[CHN_SUSTAINLOOP] && sample.nSustainEnd >= sample.nSustainStart + MIN_SAMPLE_LENGTH)
	{
		// A sustain loop is set: Examine sample up to sustain loop and, if necessary, execute the loop several times
		sampleOffset = 0;
		sampleLoopStart = sample.nSustainStart;
		sampleLoopEnd = sample.nSustainEnd;
	} else if(sample.uFlags[CHN_LOOP] && sample.nLoopEnd >= sample.nLoopStart + MIN_SAMPLE_LENGTH)
	{
		// A normal loop is set: Examine sample up to loop and, if necessary, execute the loop several times
		sampleOffset = 0;
		sampleLoopStart = sample.nLoopStart;
		sampleLoopEnd = sample.nLoopEnd;
	}

	// We should analyse at least a one second (= GetSampleRate() samples) long sample.
	sampleLength = std::max<SmpLength>(sampleLoopEnd, sample.GetSampleRate(modType)) + maxShift;
	sampleLength = (sampleLength + 7) & ~7;

	if(sampleData != nullptr)
	{
		delete[] sampleData;
	}
	sampleData = new int16[sampleLength];
	if(sampleData == nullptr)
	{
		return false;
	}

	// Copy sample over.
	switch(sample.GetElementarySampleSize())
	{
	case 1:
		CopySamples(sample.pSample8 + sampleOffset * sample.GetNumChannels(), sampleLoopStart, sampleLoopEnd);
		return true;

	case 2:
		CopySamples(sample.pSample16 + sampleOffset * sample.GetNumChannels(), sampleLoopStart, sampleLoopEnd);
		return true;
	}

	return false;

}


bool Autotune::CanApply() const
//-----------------------------
{
	return (sample.pSample != nullptr && sample.nLength >= MIN_SAMPLE_LENGTH);
}


struct AutotuneThreadData
{
	std::vector<uint64> histogram;
	double pitchReference;
	int16 *sampleData;
	SmpLength processLength;
	uint32 sampleFreq;
	int startNote, endNote;
};


DWORD WINAPI Autotune::AutotuneThread(void *info_)
//------------------------------------------------
{
	AutotuneThreadData &info = *static_cast<AutotuneThreadData *>(info_);
	info.histogram.resize(HISTORY_BINS, 0);
#ifdef ENABLE_SSE2
	const bool useSSE = (GetProcSupport() & PROCSUPPORT_SSE2) != 0;
#endif

	// Do autocorrelation and save results in a note histogram (restriced to one octave).
	for(int note = info.startNote, noteBin = note; note < info.endNote; note++, noteBin++)
	{

		if(noteBin >= HISTORY_BINS)
		{
			noteBin %= HISTORY_BINS;
		}

		const SmpLength autocorrShift = NoteToShift(info.sampleFreq, note, info.pitchReference);

		uint64 autocorrSum = 0;

#ifdef ENABLE_SSE2
		if(useSSE)
		{
			const __m128i *normalData = reinterpret_cast<const __m128i *>(info.sampleData);
			const __m128i *shiftedData = reinterpret_cast<const __m128i *>(info.sampleData + autocorrShift);
			for(SmpLength i = info.processLength / 8; i != 0; i--)
			{
				__m128i normal = _mm_loadu_si128(normalData++);
				__m128i shifted = _mm_loadu_si128(shiftedData++);
				__m128i diff = _mm_sub_epi16(normal, shifted);		// 8 16-bit differences
				__m128i squares = _mm_madd_epi16(diff, diff);		// Multiply and add: 4 32-bit squares

				__m128i sum1 = _mm_shuffle_epi32(squares, _MM_SHUFFLE(0, 1, 2, 3));	// Move upper two integers to lower
				__m128i sum2  = _mm_add_epi32(squares, sum1);						// Now we can add the (originally) upper two and lower two integers
				__m128i sum3 = _mm_shuffle_epi32(sum2, _MM_SHUFFLE(1, 1, 1, 1));	// Move the second-lowest integer to lowest position
				__m128i sum4  = _mm_add_epi32(sum2, sum3);							// Add the two lowest positions
				autocorrSum += _mm_cvtsi128_si32(sum4);
			}
		} else
#endif
		{
			const int16 *normalData = info.sampleData;
			const int16 *shiftedData = info.sampleData + autocorrShift;
			// Add up squared differences of all values
			for(SmpLength i = info.processLength; i != 0; i--, normalData++, shiftedData++)
			{
				autocorrSum += (*normalData - *shiftedData) * (*normalData - *shiftedData);
			}
		}

		info.histogram[noteBin] += autocorrSum;
	}
	return 0;
}


bool Autotune::Apply(double pitchReference, int targetNote)
//---------------------------------------------------------
{
	if(!CanApply())
	{
		return false;
	}

	const uint32 sampleFreq = sample.GetSampleRate(modType);
	// At the lowest frequency, we get the highest autocorrelation shift amount.
	const SmpLength maxShift = NoteToShift(sampleFreq, START_NOTE, pitchReference);
	if(!PrepareSample(maxShift))
	{
		return false;
	}
	// We don't process the autocorrelation overhead.
	const SmpLength processLength = sampleLength - maxShift;

	// Set up the autocorrelation threads
	const uint32 numProcs = std::max<uint32>(mpt::thread::hardware_concurrency(), 1);
	const uint32 notesPerThread = (END_NOTE - START_NOTE + 1) / numProcs;
	std::vector<AutotuneThreadData> threadInfo(numProcs);
	std::vector<HANDLE> threadHandles(numProcs);

	for(uint32 p = 0; p < numProcs; p++)
	{
		threadInfo[p].pitchReference = pitchReference;
		threadInfo[p].sampleData = sampleData;
		threadInfo[p].processLength = processLength;
		threadInfo[p].sampleFreq = sampleFreq;
		threadInfo[p].startNote = START_NOTE + p * notesPerThread;
		threadInfo[p].endNote = START_NOTE + (p + 1) * notesPerThread;
		if(p == numProcs - 1)
			threadInfo[p].endNote = END_NOTE;

		threadHandles[p] = mpt::UnmanagedThread(AutotuneThread, &threadInfo[p]);
		ASSERT(threadHandles[p] != INVALID_HANDLE_VALUE);
	}

	WaitForMultipleObjects(numProcs, &threadHandles[0], TRUE, INFINITE);

	// Histogram for all notes.
	std::vector<uint64> autocorrHistogram(HISTORY_BINS, 0);

	for(uint32 p = 0; p < numProcs; p++)
	{
		for(int i = 0; i < HISTORY_BINS; i++)
		{
			autocorrHistogram[i] += threadInfo[p].histogram[i];
		}
		CloseHandle(threadHandles[p]);
	}

	// Interpolate the histogram...
	std::vector<uint64> interpolatedHistogram(HISTORY_BINS, 0);
	for(int i = 0; i < HISTORY_BINS; i++)
	{
		interpolatedHistogram[i] = autocorrHistogram[i];
		const int kernelWidth = 4;
		for(int ki = kernelWidth; ki >= 0; ki--)
		{
			// Choose bins to interpolate with
			int left = i - ki;
			if(left < 0) left += HISTORY_BINS;
			int right = i + ki;
			if(right >= HISTORY_BINS) right -= HISTORY_BINS;

			interpolatedHistogram[i] = interpolatedHistogram[i] / 2 + (autocorrHistogram[left] + autocorrHistogram[right]) / 2;
		}
	}

	// ...and find global minimum
	int minimumBin = 0;
	for(int i = 0; i < HISTORY_BINS; i++)
	{
		const int prev = (i > 0) ? (i - 1) : (HISTORY_BINS - 1);
		// Are we at the global minimum?
		if(interpolatedHistogram[prev] < interpolatedHistogram[minimumBin])
		{
			minimumBin = prev;
		}
	}

	// Center target notes around C
	if(targetNote >= 6)
	{
		targetNote -= 12;
	}

	// Center bins around target note
	minimumBin -= targetNote * BINS_PER_NOTE;
	if(minimumBin >= 6 * BINS_PER_NOTE)
	{
		minimumBin -= 12 * BINS_PER_NOTE;
	}
	minimumBin += targetNote * BINS_PER_NOTE;

	const double newFundamentalFreq = NoteToFrequency(static_cast<double>(69 - targetNote) + static_cast<double>(minimumBin) / BINS_PER_NOTE, pitchReference);

	sample.nC5Speed = Util::Round<uint32>(sampleFreq * pitchReference / newFundamentalFreq);

	if((modType & (MOD_TYPE_XM | MOD_TYPE_MOD)))
	{
		sample.FrequencyToTranspose();
		if((modType & MOD_TYPE_MOD))
		{
			sample.RelativeTone = 0;
		}
	}

	return true;
}


/////////////////////////////////////////////////////////////
// CAutotuneDlg

int CAutotuneDlg::pitchReference = 440;	// Pitch reference in Hz
int CAutotuneDlg::targetNote = 0;		// Target note (C- = 0, C# = 1, etc...)

void CAutotuneDlg::DoDataExchange(CDataExchange* pDX)
//---------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAutotuneDlg)
	DDX_Control(pDX, IDC_COMBO1,	m_CbnNoteBox);
	//}}AFX_DATA_MAP
}


BOOL CAutotuneDlg::OnInitDialog()
//-------------------------------
{
	CDialog::OnInitDialog();

	m_CbnNoteBox.ResetContent();
	for(int note = 0; note < 12; note++)
	{
		const int item = m_CbnNoteBox.AddString(mpt::ToCString(mpt::CharsetASCII, CSoundFile::m_NoteNames[note]));
		m_CbnNoteBox.SetItemData(item, note);
		if(note == targetNote)
		{
			m_CbnNoteBox.SetCurSel(item);
		}
	}

	SetDlgItemInt(IDC_EDIT1, pitchReference, FALSE);

	return TRUE;
}


void CAutotuneDlg::OnOK()
//-----------------------
{
	CDialog::OnOK();

	targetNote = (int)m_CbnNoteBox.GetItemData(m_CbnNoteBox.GetCurSel());
	pitchReference = GetDlgItemInt(IDC_EDIT1);
}


void CAutotuneDlg::OnCancel()
//---------------------------
{
	CDialog::OnCancel();
}


OPENMPT_NAMESPACE_END
