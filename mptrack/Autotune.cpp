/*
 * Autotune.cpp
 * ------------
 * Purpose: Class for tuning a sample to a given base note automatically.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Autotune.h"
#include <math.h>
#include "../common/misc_util.h"
#include "../soundlib/Sndfile.h"
#include <algorithm>
#include <execution>
#include <numeric>
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


static double FrequencyToNote(double freq, double pitchReference)
{
	return ((12.0 * (log(freq / (pitchReference / 2.0)) / log(2.0))) + 57.0);
}


static double NoteToFrequency(double note, double pitchReference)
{
	return pitchReference * pow(2.0, (note - 69.0) / 12.0);
}


// Calculate the amount of samples for autocorrelation shifting for a given note
static SmpLength NoteToShift(uint32 sampleFreq, int note, double pitchReference)
{
	const double fundamentalFrequency = NoteToFrequency((double)note / BINS_PER_NOTE, pitchReference);
	return std::max(mpt::saturate_round<SmpLength>((double)sampleFreq / fundamentalFrequency), SmpLength(1));
}


// Create an 8-Bit sample buffer with loop unrolling and mono conversion for autocorrelation.
template <class T>
void Autotune::CopySamples(const T* origSample, SmpLength sampleLoopStart, SmpLength sampleLoopEnd)
{
	const uint8 channels = m_sample.GetNumChannels();
	sampleLoopStart *= channels;
	sampleLoopEnd *= channels;

	for(SmpLength i = 0, pos = 0; i < m_sampleLength; i++, pos += channels)
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

		m_sampleData[i] = static_cast<int16>(data);
	}
}


// Prepare a sample buffer for autocorrelation
bool Autotune::PrepareSample(SmpLength maxShift)
{

	// Determine which parts of the sample should be examined.
	SmpLength sampleOffset = 0, sampleLoopStart = 0, sampleLoopEnd = m_sample.nLength;
	if(m_selectionEnd >= sampleLoopStart + MIN_SAMPLE_LENGTH)
	{
		// A selection has been specified: Examine selection
		sampleOffset = m_selectionStart;
		sampleLoopStart = 0;
		sampleLoopEnd = m_selectionEnd - m_selectionStart;
	} else if(m_sample.uFlags[CHN_SUSTAINLOOP] && m_sample.nSustainEnd >= m_sample.nSustainStart + MIN_SAMPLE_LENGTH)
	{
		// A sustain loop is set: Examine sample up to sustain loop and, if necessary, execute the loop several times
		sampleOffset = 0;
		sampleLoopStart = m_sample.nSustainStart;
		sampleLoopEnd = m_sample.nSustainEnd;
	} else if(m_sample.uFlags[CHN_LOOP] && m_sample.nLoopEnd >= m_sample.nLoopStart + MIN_SAMPLE_LENGTH)
	{
		// A normal loop is set: Examine sample up to loop and, if necessary, execute the loop several times
		sampleOffset = 0;
		sampleLoopStart = m_sample.nLoopStart;
		sampleLoopEnd = m_sample.nLoopEnd;
	}

	// We should analyse at least a one second (= GetSampleRate() samples) long sample.
	m_sampleLength = std::max(sampleLoopEnd, static_cast<SmpLength>(m_sample.GetSampleRate(m_modType))) + maxShift;
	m_sampleLength = (m_sampleLength + 7) & ~7;

	if(m_sampleData != nullptr)
	{
		delete[] m_sampleData;
	}
	m_sampleData = new int16[m_sampleLength];
	if(m_sampleData == nullptr)
	{
		return false;
	}

	// Copy sample over.
	switch(m_sample.GetElementarySampleSize())
	{
	case 1:
		CopySamples(m_sample.sample8() + sampleOffset * m_sample.GetNumChannels(), sampleLoopStart, sampleLoopEnd);
		return true;

	case 2:
		CopySamples(m_sample.sample16() + sampleOffset * m_sample.GetNumChannels(), sampleLoopStart, sampleLoopEnd);
		return true;
	}

	return false;

}


bool Autotune::CanApply() const
{
	return (m_sample.HasSampleData() && m_sample.nLength >= MIN_SAMPLE_LENGTH) || m_sample.uFlags[CHN_ADLIB];
}


namespace
{


struct AutotuneHistogramEntry
{
	int index;
	uint64 sum;
};

struct AutotuneHistogram
{
	std::array<uint64, HISTORY_BINS> histogram{};
};

struct AutotuneContext
{
	const int16 *m_sampleData;
	double pitchReference;
	SmpLength processLength;
	uint32 sampleFreq;
};

#if defined(ENABLE_SSE2)

static inline AutotuneHistogramEntry CalculateNoteHistogramSSE2(int note, AutotuneContext ctx)
{
	const SmpLength autocorrShift = NoteToShift(ctx.sampleFreq, note, ctx.pitchReference);
	uint64 autocorrSum = 0;
	{
		const __m128i *normalData = reinterpret_cast<const __m128i *>(ctx.m_sampleData);
		const __m128i *shiftedData = reinterpret_cast<const __m128i *>(ctx.m_sampleData + autocorrShift);
		for(SmpLength i = ctx.processLength / 8; i != 0; i--)
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
	}
	return {note % HISTORY_BINS, autocorrSum};
}

#endif // ENABLE_SSE2

static inline AutotuneHistogramEntry CalculateNoteHistogram(int note, AutotuneContext ctx)
{
	const SmpLength autocorrShift = NoteToShift(ctx.sampleFreq, note, ctx.pitchReference);
	uint64 autocorrSum = 0;
	{
		const int16 *normalData = ctx.m_sampleData;
		const int16 *shiftedData = ctx.m_sampleData + autocorrShift;
		// Add up squared differences of all values
		for(SmpLength i = ctx.processLength; i != 0; i--, normalData++, shiftedData++)
		{
			autocorrSum += (*normalData - *shiftedData) * (*normalData - *shiftedData);
		}
	}
	return {note % HISTORY_BINS, autocorrSum};
}


static inline AutotuneHistogram operator+(AutotuneHistogram a, AutotuneHistogram b) noexcept
{
	AutotuneHistogram result;
	for(std::size_t i = 0; i < HISTORY_BINS; ++i)
	{
		result.histogram[i] = a.histogram[i] + b.histogram[i];
	}
	return result;
}


static inline AutotuneHistogram & operator+=(AutotuneHistogram &a, AutotuneHistogram b) noexcept
{
	for(std::size_t i = 0; i < HISTORY_BINS; ++i)
	{
		a.histogram[i] += b.histogram[i];
	}
	return a;
}


static inline AutotuneHistogram &operator+=(AutotuneHistogram &a, AutotuneHistogramEntry b) noexcept
{
	a.histogram[b.index] += b.sum;
	return a;
}


struct AutotuneHistogramReduce
{
	inline AutotuneHistogram operator()(AutotuneHistogram a, AutotuneHistogram b) noexcept
	{
		return a + b;
	}
	inline AutotuneHistogram operator()(AutotuneHistogramEntry a, AutotuneHistogramEntry b) noexcept
	{
		AutotuneHistogram result;
		result += a;
		result += b;
		return result;
	}
	inline AutotuneHistogram operator()(AutotuneHistogramEntry a, AutotuneHistogram b) noexcept
	{
		b += a;
		return b;
	}
	inline AutotuneHistogram operator()(AutotuneHistogram a, AutotuneHistogramEntry b) noexcept
	{
		a += b;
		return a;
	}
};


} // local



bool Autotune::Apply(double pitchReference, int targetNote)
{
	if(!CanApply())
	{
		return false;
	}

	const uint32 sampleFreq = m_sample.GetSampleRate(m_modType);
	// At the lowest frequency, we get the highest autocorrelation shift amount.
	const SmpLength maxShift = NoteToShift(sampleFreq, START_NOTE, pitchReference);
	if(!PrepareSample(maxShift))
	{
		return false;
	}
	// We don't process the autocorrelation overhead.
	const SmpLength processLength = m_sampleLength - maxShift;

	AutotuneContext ctx;
	ctx.m_sampleData = m_sampleData;
	ctx.pitchReference = pitchReference;
	ctx.processLength = processLength;
	ctx.sampleFreq = sampleFreq;
	
	// Note that we cannot use a fake integer iterator here because of the requirement on ForwardIterator to return a reference to the elements.
	std::array<int, END_NOTE - START_NOTE> notes;
	std::iota(notes.begin(), notes.end(), START_NOTE);

	AutotuneHistogram autocorr =
#ifdef ENABLE_SSE2
		(CPU::HasFeatureSet(CPU::feature::sse2)) ? std::transform_reduce(std::execution::par_unseq, std::begin(notes), std::end(notes), AutotuneHistogram{}, AutotuneHistogramReduce{}, [ctx](int note) { return CalculateNoteHistogramSSE2(note, ctx); } ) :
#endif
		std::transform_reduce(std::execution::par_unseq, std::begin(notes), std::end(notes), AutotuneHistogram{}, AutotuneHistogramReduce{}, [ctx](int note) { return CalculateNoteHistogram(note, ctx); } );
	
	// Interpolate the histogram...
	AutotuneHistogram interpolated;
	for(int i = 0; i < HISTORY_BINS; i++)
	{
		interpolated.histogram[i] = autocorr.histogram[i];
		const int kernelWidth = 4;
		for(int ki = kernelWidth; ki >= 0; ki--)
		{
			// Choose bins to interpolate with
			int left = i - ki;
			if(left < 0) left += HISTORY_BINS;
			int right = i + ki;
			if(right >= HISTORY_BINS) right -= HISTORY_BINS;

			interpolated.histogram[i] = interpolated.histogram[i] / 2 + (autocorr.histogram[left] + autocorr.histogram[right]) / 2;
		}
	}

	// ...and find global minimum
	int minimumBin = static_cast<int>(std::min_element(std::begin(interpolated.histogram), std::end(interpolated.histogram)) - std::begin(interpolated.histogram));

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

	if(const auto newFreq = mpt::saturate_round<uint32>(sampleFreq * pitchReference / newFundamentalFreq); newFreq != sampleFreq)
		m_sample.nC5Speed = newFreq;
	else
		return false;

	if((m_modType & (MOD_TYPE_XM | MOD_TYPE_MOD)))
	{
		m_sample.FrequencyToTranspose();
		if((m_modType & MOD_TYPE_MOD))
		{
			m_sample.RelativeTone = 0;
		}
	}

	return true;
}


/////////////////////////////////////////////////////////////
// CAutotuneDlg

int CAutotuneDlg::m_pitchReference = 440; // Pitch reference in Hz
int CAutotuneDlg::m_targetNote = 0;       // Target note (C- = 0, C# = 1, etc...)

void CAutotuneDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAutotuneDlg)
	DDX_Control(pDX, IDC_COMBO1,	m_CbnNoteBox);
	//}}AFX_DATA_MAP
}


BOOL CAutotuneDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_CbnNoteBox.ResetContent();
	for(int note = 0; note < 12; note++)
	{
		const int item = m_CbnNoteBox.AddString(mpt::ToCString(CSoundFile::GetDefaultNoteName(note)));
		m_CbnNoteBox.SetItemData(item, note);
		if(note == m_targetNote)
		{
			m_CbnNoteBox.SetCurSel(item);
		}
	}

	SetDlgItemInt(IDC_EDIT1, m_pitchReference, FALSE);

	return TRUE;
}


void CAutotuneDlg::OnOK()
{
	int pitch = GetDlgItemInt(IDC_EDIT1);
	if(pitch <= 0)
	{
		MessageBeep(MB_ICONWARNING);
		return;
	}

	CDialog::OnOK();
	m_targetNote = (int)m_CbnNoteBox.GetItemData(m_CbnNoteBox.GetCurSel());
	m_pitchReference = pitch;
}

OPENMPT_NAMESPACE_END
