/*
 * TimeStretchPitchShift.h
 * -----------------------
 * Purpose: Classes for applying different types of offline time stretching and pitch shifting to samples.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "../soundlib/Snd_defs.h"

#include <functional>

OPENMPT_NAMESPACE_BEGIN

class CSoundFile;
struct ModSample;

namespace TimeStretchPitchShift
{

enum class Result : uint8
{
	OK,
	Abort,
	StretchTooShort,
	StretchTooLong,
	InvalidGrainSize,
	SampleTooShort,
	OutOfMemory,
};


class Base
{
public:
	using UpdateProgressFunc = std::function<bool(SmpLength, SmpLength)>;
	using PrepareUndoFunc = std::function<void()>;

	Base(UpdateProgressFunc updateProgress, PrepareUndoFunc prepareUndo,
		CSoundFile &sndFile, SAMPLEINDEX sample, float pitch, float stretchRatio, SmpLength start, SmpLength end);
	virtual ~Base() = default;

	virtual Result Process() = 0;

	SmpLength NewSelectionEnd() const { return m_stretchEnd; }

protected:
	Result CheckPreconditions() const;
	void FinishProcessing(void *pNewSample);

	const UpdateProgressFunc UpdateProgress;
	const PrepareUndoFunc PrepareUndo;
	CSoundFile &m_sndFile;
	ModSample &m_sample;
	const float m_pitch, m_stretchRatio;
	const SmpLength m_start, m_end;
	const SmpLength m_selLength, m_newSelLength;
	const SmpLength m_newLength, m_stretchEnd;
};


class Signalsmith final : public Base
{
public:
	using Base::Base;

	Result Process() override;

private:
	std::pair<SmpLength, SmpLength> CalculateBufferSizes() const;
};


class LoFi final : public Base
{
public:
	LoFi(UpdateProgressFunc updateProgress, PrepareUndoFunc prepareUndo,
		CSoundFile &sndFile, SAMPLEINDEX sample, float pitch, float stretchRatio, SmpLength start, SmpLength end, int grainSize)
		: Base{std::move(updateProgress), std::move(prepareUndo), sndFile, sample, pitch, stretchRatio, start, end}
		, m_grainSize{grainSize}
	{
	}

	Result Process() override;

private:
	const int m_grainSize;
};

} // namespace TimeStretchPitchShift

OPENMPT_NAMESPACE_END
