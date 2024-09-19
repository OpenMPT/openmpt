/*
 * PSRatioCalc.cpp
 * ---------------
 * Purpose: Dialog for calculating sample time stretch ratios in the sample editor.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "PSRatioCalc.h"
#include "resource.h"
#include "../soundlib/Sndfile.h"


OPENMPT_NAMESPACE_BEGIN


CPSRatioCalc::CPSRatioCalc(const CSoundFile &sndFile, SAMPLEINDEX sample, double ratio, CWnd *parent)
	: DialogBase{IDD_PITCHSHIFT, parent}
	, m_ratio{ratio}
	, m_sndFile{sndFile}
	, m_sampleIndex{sample}
	, m_speed{sndFile.m_PlayState.m_nMusicSpeed}
	, m_tempo{sndFile.m_PlayState.m_nMusicTempo}
{
	const ModSample &smp = m_sndFile.GetSample(m_sampleIndex);
	uint32 sampleRate = smp.GetSampleRate(m_sndFile.GetType());
	if(sampleRate <= 0)
		sampleRate = 8363;

	// Sample rate will not change. We can calculate original duration once and disgard sampleRate.
	m_durationOrig = 1000.0 * static_cast<double>(smp.nLength) / sampleRate;
}


BEGIN_MESSAGE_MAP(CPSRatioCalc, DialogBase)
	ON_EN_UPDATE(IDC_SAMPLE_LENGTH_NEW, &CPSRatioCalc::OnChangeSampleLength)
	ON_EN_UPDATE(IDC_MS_LENGTH_NEW,     &CPSRatioCalc::OnChangeDuration)
	ON_EN_UPDATE(IDC_SPEED,             &CPSRatioCalc::OnChangeSpeed)
	ON_EN_UPDATE(IDC_TEMPO,             &CPSRatioCalc::OnChangeSpeed)
	ON_EN_UPDATE(IDC_ROW_LENGTH_NEW2,   &CPSRatioCalc::OnChangeRows)
	ON_EN_UPDATE(IDC_PSRATIO,           &CPSRatioCalc::OnChangeRatio)
END_MESSAGE_MAP()


BOOL CPSRatioCalc::OnInitDialog()
{
	DialogBase::OnInitDialog();

	LockControls();

	const std::pair<CNumberEdit &, int> NumberControls[] =
	{
		{m_EditTempo,        IDC_TEMPO},
		{m_EditRatio,        IDC_PSRATIO},
		{m_EditRowsOrig,     IDC_ROW_LENGTH_ORIGINAL},
		{m_EditRowsNew,      IDC_ROW_LENGTH_NEW2},
		{m_EditDurationOrig, IDC_MS_LENGTH_ORIGINAL2},
		{m_EditDurationNew,  IDC_MS_LENGTH_NEW},
	};
	for(auto [editCtrl, dlgID] : NumberControls)
	{
		editCtrl.SubclassDlgItem(dlgID, this);
		editCtrl.AllowNegative(false);
	}

	SetDlgItemInt(IDC_SAMPLE_LENGTH_ORIGINAL, m_sndFile.GetSample(m_sampleIndex).nLength, FALSE);
	SetDlgItemInt(IDC_SAMPLE_LENGTH_NEW, m_sampleLengthNew, FALSE);
	SetDlgItemInt(IDC_SPEED, m_speed, FALSE);
	m_EditTempo.SetTempoValue(m_tempo);
	m_EditRatio.SetDecimalValue(m_ratio);
	m_EditDurationOrig.SetDecimalValue(m_durationOrig);
	CalcSamples();
	CalcMs();
	CalcRows();

	UnlockControls();

	return TRUE;
}

void CPSRatioCalc::OnChangeSampleLength()
{
	if(IsLocked())
		return;
	m_sampleLengthNew = GetDlgItemInt(IDC_SAMPLE_LENGTH_NEW, nullptr, FALSE);
	if(m_sampleLengthNew && m_sndFile.GetSample(m_sampleIndex).nLength)
	{
		LockControls();
		m_ratio = static_cast<double>(m_sampleLengthNew) * 100.0 / static_cast<double>(m_sndFile.GetSample(m_sampleIndex).nLength);
		CalcMs();
		CalcRows();
		m_EditRatio.SetDecimalValue(m_ratio);
		UnlockControls();
	}
}

void CPSRatioCalc::OnChangeDuration()
{
	if(IsLocked())
		return;
	if(m_durationOrig > 0.0 && m_EditDurationNew.GetDecimalValue(m_durationNew))
	{
		LockControls();
		m_ratio = m_durationNew * 100.0 / m_durationOrig;
		CalcSamples();
		CalcRows();
		m_EditRatio.SetDecimalValue(m_ratio);
		UnlockControls();
	}
}

void CPSRatioCalc::OnChangeRows()
{
	if(IsLocked())
		return;
	if(m_rowsOrig && m_EditRowsNew.GetDecimalValue(m_rowsNew))
	{
		LockControls();
		m_ratio = m_rowsNew * 100.0 / m_rowsOrig;
		CalcSamples();
		CalcMs();
		m_EditRatio.SetDecimalValue(m_ratio);
		UnlockControls();
	}
}

void CPSRatioCalc::OnChangeSpeed()
{
	if(IsLocked())
		return;
	LockControls();
	m_tempo = std::max(TEMPO(1, 0), m_EditTempo.GetTempoValue());
	m_speed = std::max(uint32(1), GetDlgItemInt(IDC_SPEED, nullptr, FALSE));
	CalcRows();
	UnlockControls();
}

void CPSRatioCalc::OnChangeRatio()
{
	if(IsLocked())
		return;
	if(m_EditRatio.GetDecimalValue(m_ratio))
	{
		LockControls();
		CalcSamples();
		CalcMs();
		CalcRows();
		UnlockControls();
	}
}

// CPSRatioCalc Internal Calculations:

void CPSRatioCalc::CalcSamples()
{
	m_sampleLengthNew = mpt::saturate_round<SmpLength>(m_sndFile.GetSample(m_sampleIndex).nLength * m_ratio / 100.0);
	SetDlgItemInt(IDC_SAMPLE_LENGTH_NEW, m_sampleLengthNew, FALSE);
}

void CPSRatioCalc::CalcMs()
{
	m_durationNew = m_durationOrig * m_ratio / 100.0;
	m_EditDurationNew.SetDecimalValue(m_durationNew);
}

void CPSRatioCalc::CalcRows()
{
	m_rowsOrig = m_durationOrig / m_sndFile.GetRowDuration(m_tempo, m_speed);
	m_rowsNew = m_rowsOrig * m_ratio / 100.0;
	m_EditRowsOrig.SetDecimalValue(m_rowsOrig);
	m_EditRowsNew.SetDecimalValue(m_rowsNew);
}

OPENMPT_NAMESPACE_END
