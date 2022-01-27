/*
 * SampleTrimmer.cpp
 * -----------------
 * Purpose: Automatic trimming of unused sample parts for module size optimization.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include <numeric>
#include "InputHandler.h"
#include "Mainfrm.h"
#include "Moddoc.h"
#include "ProgressDialog.h"
#include "../tracklib/SampleEdit.h"
#include "../soundlib/OPL.h"

OPENMPT_NAMESPACE_BEGIN

class CRenderProgressDlg : public CProgressDialog
{
	CSoundFile &m_SndFile;

	class DummyAudioTarget : public IAudioTarget
	{
	public:
		void Process(mpt::audio_span_interleaved<MixSampleInt>) override { }
		void Process(mpt::audio_span_interleaved<MixSampleFloat>) override { }
	};

public:
	std::vector<SmpLength> m_SamplePlayLengths;

	CRenderProgressDlg(CWnd *parent, CSoundFile &sndFile)
		: CProgressDialog{parent}
		, m_SndFile{sndFile}
	{
		m_SndFile.m_SamplePlayLengths = &m_SamplePlayLengths;
	}

	~CRenderProgressDlg()
	{
		m_SndFile.m_SamplePlayLengths = nullptr;
	}

	void Run() override
	{
		// We're not interested in plugin rendering
		std::bitset<MAX_MIXPLUGINS> plugMuteStatus;
		for(PLUGINDEX i = 0; i < MAX_MIXPLUGINS; i++)
		{
			plugMuteStatus[i] = m_SndFile.m_MixPlugins[i].IsBypassed();
			m_SndFile.m_MixPlugins[i].SetBypass(true);
		}

		m_SamplePlayLengths.assign(m_SndFile.GetNumSamples() + 1, 0);

		const auto origSequence = m_SndFile.Order.GetCurrentSequenceIndex();
		const auto origRepeatCount = m_SndFile.GetRepeatCount();
		auto opl = std::move(m_SndFile.m_opl);
		m_SndFile.SetRepeatCount(0);
		m_SndFile.m_bIsRendering = true;

		auto prevTime = timeGetTime();
		const auto subSongs = m_SndFile.GetAllSubSongs();
		SetRange(0, mpt::saturate_round<uint64>(std::accumulate(subSongs.begin(), subSongs.end(), 0.0, [](double acc, const auto& song) { return acc + song.duration; }) * m_SndFile.GetSampleRate()));
		size_t totalSamples = 0;
		for(size_t i = 0; i < subSongs.size() && !m_abort; i++)
		{
			SetWindowText(MPT_CFORMAT("Automatic Sample Trimmer - Song {} / {}")(i + 1, subSongs.size()));

			const auto &song = subSongs[i];
			m_SndFile.ResetPlayPos();
			m_SndFile.GetLength(eAdjust, GetLengthTarget(song.startOrder, song.startRow).StartPos(song.sequence, 0, 0));
			m_SndFile.m_SongFlags.reset(SONG_PLAY_FLAGS);

			size_t subsongSamples = 0;
			DummyAudioTarget target;
			while(!m_abort)
			{
				auto count = m_SndFile.Read(MIXBUFFERSIZE, target);
				if(count == 0)
					break;

				totalSamples += count;
				subsongSamples += count;

				auto currentTime = timeGetTime();
				if(currentTime - prevTime >= 16)
				{
					prevTime = currentTime;
					auto timeSec = subsongSamples / m_SndFile.GetSampleRate();
					SetText(MPT_CFORMAT("Analyzing... {}:{}:{}")(timeSec / 3600, mpt::cfmt::dec0<2>((timeSec / 60) % 60), mpt::cfmt::dec0<2>(timeSec % 60)));
					SetProgress(totalSamples);
					ProcessMessages();
				}
			}
		}

		// Reset globals to previous values
		m_SndFile.Order.SetSequence(origSequence);
		m_SndFile.SetRepeatCount(origRepeatCount);
		m_SndFile.ResetPlayPos();
		m_SndFile.StopAllVsti();
		m_SndFile.m_bIsRendering = false;
		m_SndFile.m_opl = std::move(opl);

		for(PLUGINDEX i = 0; i < MAX_MIXPLUGINS; i++)
		{
			m_SndFile.m_MixPlugins[i].SetBypass(plugMuteStatus[i]);
		}

		EndDialog(IDOK);
	}
};


void CModDoc::OnShowSampleTrimmer()
{
	BypassInputHandler bih;
	CMainFrame::GetMainFrame()->StopMod(this);
	CRenderProgressDlg dlg(CMainFrame::GetMainFrame(), m_SndFile);
	dlg.DoModal();
	SAMPLEINDEX numTrimmed = 0;
	SmpLength numBytes = 0;
	for(SAMPLEINDEX smp = 1; smp <= GetNumSamples(); smp++)
	{
		ModSample &sample = m_SndFile.GetSample(smp);
		auto &newLength = dlg.m_SamplePlayLengths[smp];
		if(newLength == 0)
			continue;
		// Take interpolation look-ahead into account
		if((!sample.uFlags[CHN_LOOP] || newLength != sample.nLoopEnd)
		   && (!sample.uFlags[CHN_SUSTAINLOOP] || newLength != sample.nSustainEnd))
		{
			newLength = std::min(newLength + InterpolationMaxLookahead, sample.nLength);
		}
		if(sample.nLength > newLength)
		{
			numTrimmed++;
			numBytes += (sample.nLength - newLength) * sample.GetBytesPerSample();
		}
	}
	if(numTrimmed == 0)
	{
		Reporting::Information(_T("No samples can be trimmed, because all samples are played in their full length."));
		return;
	}

	mpt::ustring s = MPT_UFORMAT("{} sample{} can be trimmed, saving {} byte{}.")(numTrimmed, (numTrimmed == 1) ? U_("") : U_("s"), mpt::ufmt::dec(3, U_(","), numBytes), numBytes != 1 ? U_("s") : U_(""));
	if(dlg.m_abort)
	{
		s += U_("\n\nWARNING: Only partial results are available, possibly causing used sample parts to be trimmed.\nContinue anyway?");
	} else
	{
		s += U_(" Continue?");
	}
	if(Reporting::Confirm(s, false, dlg.m_abort) == cnfYes)
	{
		for(SAMPLEINDEX smp = 1; smp <= GetNumSamples(); smp++)
		{
			ModSample &sample = m_SndFile.GetSample(smp);
			if(dlg.m_SamplePlayLengths[smp] != 0 && sample.nLength > dlg.m_SamplePlayLengths[smp])
			{
				GetSampleUndo().PrepareUndo(smp, sundo_delete, "Automatic Sample Trimming", dlg.m_SamplePlayLengths[smp], sample.nLength);
				SampleEdit::ResizeSample(sample, dlg.m_SamplePlayLengths[smp], m_SndFile);
				sample.uFlags.set(SMP_MODIFIED);
			}
		}

		SetModified();
		UpdateAllViews(SampleHint().Data().Info());
	}
}

OPENMPT_NAMESPACE_END
