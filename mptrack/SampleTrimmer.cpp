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
#include "Mainfrm.h"
#include "Moddoc.h"
#include "ProgressDialog.h"
#include "../soundlib/modsmp_ctrl.h"

OPENMPT_NAMESPACE_BEGIN

class CRenderProgressDlg : public CProgressDialog
{
	CSoundFile &m_SndFile;

	class DummyAudioTarget : public IAudioReadTarget
	{
	public:
		void DataCallback(int *, std::size_t, std::size_t) override { }
	};

public:
	CRenderProgressDlg(CWnd *parent, CSoundFile &sndFile)
		: CProgressDialog(parent)
		, m_SndFile(sndFile)
	{ }

	~CRenderProgressDlg()
	{
		m_SndFile.m_SamplePlayLengths.clear();
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

		m_SndFile.m_SamplePlayLengths.assign(m_SndFile.GetNumSamples() + 1, 0);

		auto prevTime = timeGetTime();
		auto currentSeq = m_SndFile.Order.GetCurrentSequenceIndex();
		auto currentRepeatCount = m_SndFile.GetRepeatCount();
		m_SndFile.SetRepeatCount(0);
		m_SndFile.m_bIsRendering = true;
		for(SEQUENCEINDEX seq = 0; seq < m_SndFile.Order.GetNumSequences() && !m_abort; seq++)
		{
			TCHAR s[64];
			wsprintf(s, _T("Automatic Sample Trimmer - Sequence %u / %u"), seq + 1, m_SndFile.Order.GetNumSequences());
			SetWindowText(s);

			m_SndFile.Order.SetSequence(seq);
			m_SndFile.ResetPlayPos();
			const auto subSongs = m_SndFile.GetLength(eNoAdjust, GetLengthTarget(true));
			SetRange(0, Util::Round<uint32>(std::accumulate(subSongs.begin(), subSongs.end(), 0.0, [](double acc, const GetLengthType &glt) { return acc + glt.duration; }) * m_SndFile.GetSampleRate()));

			size_t totalSamples = 0;
			DummyAudioTarget target;
			for(const auto &subSong : subSongs)
			{
				m_SndFile.GetLength(eAdjust, GetLengthTarget(subSong.startOrder, subSong.startRow));
				m_SndFile.m_SongFlags.reset(SONG_PLAY_FLAGS);
				while(!m_abort)
				{
					size_t count = m_SndFile.Read(MIXBUFFERSIZE, target);
					if(count == 0)
					{
						break;
					}
					totalSamples += count;

					auto currentTime = timeGetTime();
					if(currentTime - prevTime >= 16)
					{
						prevTime = currentTime;
						auto timeSec = totalSamples / m_SndFile.GetSampleRate();
						wsprintf(s, _T("Analyzing... %u:%02u:%02u"), timeSec / 3600, (timeSec / 60) % 60, timeSec % 60);
						SetText(s);
						SetProgress(mpt::saturate_cast<uint32>(totalSamples));
						ProcessMessages();
					}
				}
			}
		}

		// Reset globals to previous values
		m_SndFile.Order.SetSequence(currentSeq);
		m_SndFile.SetRepeatCount(currentRepeatCount);
		m_SndFile.ResetPlayPos();
		m_SndFile.StopAllVsti();
		m_SndFile.m_bIsRendering = false;

		for(PLUGINDEX i = 0; i < MAX_MIXPLUGINS; i++)
		{
			m_SndFile.m_MixPlugins[i].SetBypass(plugMuteStatus[i]);
		}

		EndDialog(IDOK);
	}
};


void CModDoc::OnShowSampleTrimmer()
{
	CMainFrame::GetMainFrame()->StopMod(this);
	CRenderProgressDlg dlg(CMainFrame::GetMainFrame(), m_SndFile);
	dlg.DoModal();
	SAMPLEINDEX numTrimmed = 0;
	SmpLength numBytes = 0;
	for(SAMPLEINDEX smp = 1; smp <= GetNumSamples(); smp++)
	{
		ModSample &sample = m_SndFile.GetSample(smp);
		SmpLength length = sample.nLength;
		if(m_SndFile.m_SamplePlayLengths[smp] != 0 && length > m_SndFile.m_SamplePlayLengths[smp])
		{
			numTrimmed++;
			numBytes += (length - m_SndFile.m_SamplePlayLengths[smp]) * sample.GetBytesPerSample();
		}
	}
	if(numTrimmed == 0)
	{
		Reporting::Information(_T("No samples can be trimmed, because all samples are played in their full length."));
		return;
	}

	CString s;
	s.Format(_T("%u sample%s can be trimmed, saving %u bytes."), numTrimmed, (numTrimmed == 1) ? _T("") : _T("s"), numBytes);
	if(dlg.m_abort)
	{
		s += _T("\n\nWARNING: Only partial results are available, possibly causing used sample parts to be trimmed.\nContinue anyway?");
	} else
	{
		s += _T(" Continue?");
	}
	if(Reporting::Confirm(s, false, dlg.m_abort) == cnfYes)
	{
		for(SAMPLEINDEX smp = 1; smp <= GetNumSamples(); smp++)
		{
			ModSample &sample = m_SndFile.GetSample(smp);
			SmpLength length = sample.nLength;
			if(m_SndFile.m_SamplePlayLengths[smp] != 0 && length > m_SndFile.m_SamplePlayLengths[smp])
			{
				GetSampleUndo().PrepareUndo(smp, sundo_delete, "Automatic Sample Trimming", m_SndFile.m_SamplePlayLengths[smp], length);
				ctrlSmp::ResizeSample(sample, m_SndFile.m_SamplePlayLengths[smp], m_SndFile);
				sample.uFlags.set(SMP_MODIFIED);
			}
		}

		SetModified();
		UpdateAllViews(SampleHint().Data().Info());
	}
}

OPENMPT_NAMESPACE_END
