/*
 * Autotune.cpp
 * ------------
 * Purpose: Class for tuning a sample to a given base note automatically, using McLeod Pitch Method.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Autotune.h"
#include "resource.h"
#include "../common/misc_util.h"
#include "../soundlib/Sndfile.h"
#include "../soundlib/TinyFFT.h"
#include "../tracklib/SampleEdit.h"

#include <pitch-detection/include/pitch_detection.h>

#include <algorithm>
#include <cmath>
#include <execution>
#include <numeric>
#include <thread>


OPENMPT_NAMESPACE_BEGIN


static constexpr SmpLength MIN_SAMPLE_LENGTH = 2;


static inline double FrequencyToNote(double freq, double pitchReference)
{
	return 69.0 + 12.0 * std::log2(freq / pitchReference);
}


// Create a normalized sample buffer with loop unrolling and deinterleaving stereo channels.
template <class T>
void Autotune::CopySamples(const T *origSample, SmpLength sampleLoopStart, SmpLength sampleLoopEnd)
{
	const bool singleChannel = SampleEdit::IsSingleChannel(m_sample, m_channelSelection);
	const uint8 channels = m_sample.GetNumChannels();
	const uint8 firstChannel = singleChannel ? SampleEdit::SelectedChannel(m_channelSelection) : 0;
	const uint8 numAnalyzedChannels = singleChannel ? 1 : channels;
	const SmpLength channelLength = static_cast<SmpLength>(m_sampleData.size() / numAnalyzedChannels);
	double peak = 0.0;
	std::vector<double> dcOffsets(numAnalyzedChannels);
	sampleLoopStart *= channels;
	sampleLoopEnd *= channels;

	for(uint8 outChn = 0; outChn < numAnalyzedChannels; outChn++)
	{
		const uint8 inChn = firstChannel + outChn;
		double *out = m_sampleData.data() + outChn * channelLength;
		double sum = 0.0;
		T minValue = 0, maxValue = 0;
		for(SmpLength i = 0, pos = inChn; i < channelLength; i++, pos += channels)
		{
			if(pos >= sampleLoopEnd)
				pos = sampleLoopStart + inChn;

			const T value = origSample[pos];
			out[i] = value;
			sum += value;
			minValue = std::min(minValue, value);
			maxValue = std::max(maxValue, value);
		}
		dcOffsets[outChn] = sum / channelLength;
		peak = std::max({peak, maxValue - dcOffsets[outChn], dcOffsets[outChn] - minValue});
	}

	// Removing DC offset increases confidence with some samples
	const double gain = peak > 0.0 ? 1.0 / peak : 1.0;
	for(uint8 outChn = 0; outChn < numAnalyzedChannels; outChn++)
	{
		double *out = m_sampleData.data() + outChn * channelLength;
		for(SmpLength i = 0; i < channelLength; i++)
		{
			out[i] = (out[i] - dcOffsets[outChn]) * gain;
		}
	}
}


// Prepare a sample buffer for pitch detection
bool Autotune::PrepareSample(SmpLength windowSize)
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

	// We should analyze at least a one second (= GetSampleRate() samples) long sample.
	const uint8 numAnalyzedChannels = SampleEdit::IsSingleChannel(m_sample, m_channelSelection) ? 1 : m_sample.GetNumChannels();
	m_sampleData.resize(mpt::align_up(std::max(sampleLoopEnd, static_cast<SmpLength>(m_sample.GetSampleRate(m_modType))), windowSize) * numAnalyzedChannels);

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


Autotune::Result Autotune::Apply(double pitchReference, int targetNote)
{
	const uint32 sampleFreq = m_sample.GetSampleRate(m_modType);
	if(!sampleFreq || !CanApply())
		return Result::CannotApply;

	const uint32 windowOrder = std::max(static_cast<uint32>(mpt::bit_width(static_cast<uint32>(4.0 * sampleFreq / pitch::lower_pitch_cutoff()))), uint32(4));
	const SmpLength windowSize = SmpLength(1) << windowOrder;
	if(!PrepareSample(windowSize))
		return Result::Error;

	struct TinyFFTProvider : public pitch::FFTProvider
	{
		TinyFFTProvider(const uint32 fftSize): fft{fftSize} {}
		
		void FFT(std::vector<std::complex<double>> &A) const override { fft.FFT(A); }
		void IFFT(std::vector<std::complex<double>> &A) const override { fft.IFFT(A); }
		void Normalize(std::vector<std::complex<double>> &data) const override { TinyFFT::Normalize(data); }
		
		const TinyFFT fft;
	};
	const TinyFFTProvider fft{windowOrder + 1};

	const SmpLength numFrames = static_cast<SmpLength>(m_sampleData.size() / windowSize);
	const SmpLength numWorkers = std::min(numFrames, static_cast<SmpLength>(std::max(std::thread::hardware_concurrency(), 1u)));
	std::vector<double> frequencies(numFrames);
	std::vector<SmpLength> workers(numWorkers);
	std::iota(workers.begin(), workers.end(), SmpLength(0));

	std::for_each(std::execution::par, workers.begin(), workers.end(), [&](SmpLength worker)
	{
		const SmpLength firstFrame = static_cast<SmpLength>(Util::muldiv_unsigned(worker, numFrames, numWorkers));
		const SmpLength endFrame = static_cast<SmpLength>(Util::muldiv_unsigned(worker + 1, numFrames, numWorkers));
		pitch_alloc::Mpm<double> mpm{static_cast<long>(windowSize), fft};
		std::vector<double> frame;
		for(SmpLength i = firstFrame; i < endFrame; i++)
		{
			const auto start = m_sampleData.begin() + i * windowSize;
			frame.assign(start, start + windowSize);
			frequencies[i] = mpm.pitch(frame, static_cast<int>(sampleFreq));
		}
	});

	mpt::erase_if(frequencies, [](const double frequency) { return frequency <= 0.0; });
	if(frequencies.empty())
		return Result::NoPitchFound;

	const auto medianFreq = frequencies.begin() + frequencies.size() / 2;
	std::nth_element(frequencies.begin(), medianFreq, frequencies.end());

	// Only shift within the current octave of the sample
	double offset = FrequencyToNote(*medianFreq, pitchReference) - targetNote;
	offset -= 12.0 * std::floor((offset + 6.0) / 12.0);

	if(const auto newFreq = mpt::saturate_round<uint32>(sampleFreq * std::pow(2.0, -offset / 12.0)); newFreq != sampleFreq)
		m_sample.nC5Speed = newFreq;
	else
		return Result::NoChange;

	if((m_modType & (MOD_TYPE_XM | MOD_TYPE_MOD)))
	{
		m_sample.FrequencyToTranspose();
		if((m_modType & MOD_TYPE_MOD))
		{
			m_sample.RelativeTone = 0;
		}
	}

	return Result::OK;
}


/////////////////////////////////////////////////////////////
// CAutotuneDlg

int CAutotuneDlg::m_pitchReference = 440; // Pitch reference in Hz
int CAutotuneDlg::m_targetNote = 0;       // Target note (C- = 0, C# = 1, etc...)

void CAutotuneDlg::DoDataExchange(CDataExchange* pDX)
{
	DialogBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAutotuneDlg)
	DDX_Control(pDX, IDC_COMBO1,	m_CbnNoteBox);
	//}}AFX_DATA_MAP
}


CAutotuneDlg::CAutotuneDlg(CWnd *parent) : DialogBase(IDD_AUTOTUNE, parent) {}


BOOL CAutotuneDlg::OnInitDialog()
{
	DialogBase::OnInitDialog();

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

	DialogBase::OnOK();
	m_targetNote = (int)m_CbnNoteBox.GetItemData(m_CbnNoteBox.GetCurSel());
	m_pitchReference = pitch;
}

OPENMPT_NAMESPACE_END
