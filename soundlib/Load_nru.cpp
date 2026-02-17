/*
 * Load_nru.cpp
 * ------------
 * Purpose: NoiseRunner module loader
 * Notes  : NoiseRunner is a modified NoiseTracker / ProTracker format for more efficient playback on Amiga.
 *          It uses the same "M.K." magic as normal ProTracker MODs, but has a completely different
 *          sample header layout and pattern data encoding.
 *          Support is mostly included to prevent such modules from being recognized as regular M.K. MODs.
 * Authors: OpenMPT Devs
 *          Based on ProWizard by Asle
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"
#include "MODTools.h"

OPENMPT_NAMESPACE_BEGIN


struct NRUSampleHeader
{
	uint16be volume;        // 0...64
	uint32be sampleAddr;    // Sample address in memory
	uint16be length;        // Sample length in words
	uint32be loopStartAddr; // Loop start address
	uint16be loopLength;    // Loop length in words
	int16be  finetune;      // May contain garbage value if positive
};

MPT_BINARY_STRUCT(NRUSampleHeader, 16)


struct NRUFileHeader
{
	std::array<NRUSampleHeader, 31> sampleHeaders;
	std::array<char, 454> unknown;  // Leftover garbage from original ProTracker module
	MODFileHeader order;
	std::array<char, 4> magic;  // "M.K." >:(

	struct ValidationResult
	{
		uint32 totalSampleSize = 0;
		uint8 numPatterns = 0;
	};

	ValidationResult IsValid() const
	{
		if(!IsMagic(magic.data(), "M.K."))
			return {};

		uint32 totalSampleSize = 0;
		for(const auto &sample : sampleHeaders)
		{
			if(sample.volume > 64)
				return {};

			const uint32 addr = sample.sampleAddr;
			const uint32 loopStartAddr = sample.loopStartAddr;
			if(addr > 0x1F'FFFF || (addr & 1))
				return {};

			if(sample.length == 0)
			{
				if(loopStartAddr != addr || sample.loopLength != 1)
					return {};
			} else
			{
				if(sample.length >= 0x8000)
					return {};
				if(loopStartAddr < addr)
					return {};
				const uint32 loopStart = loopStartAddr - addr;
				if(loopStart >= sample.length * 2u)
					return {};
				if(loopStart + sample.loopLength * 2u > sample.length * 2u)
					return {};
				totalSampleSize += sample.length;
			}

			if(const int16 finetune = sample.finetune; finetune < 0)
			{
				if(finetune < -(72 * 15) || (finetune % 72))
					return {};
			}
		}

		if(totalSampleSize < 32)
			return {};

		if(order.numOrders == 0 || order.numOrders > 127)
			return {};
		uint8 maxPattern = 0;
		for(uint8 i = 0; i < 128; i++)
		{
			const uint8 pat = order.orderList[i];
			if(i < order.numOrders)
			{
				if(pat > 63)
					return {};
				if(pat > maxPattern)
					maxPattern = pat;
			} else if(pat != 0)
			{
				return {};
			}
		}

		return {totalSampleSize * 2u, static_cast<uint8>(maxPattern + 1)};
	}
};

MPT_BINARY_STRUCT(NRUFileHeader, 1084)


CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderNRU(MemoryFileReader file, const uint64 *pfilesize)
{
	NRUFileHeader fileHeader;
	if(!file.ReadStruct(fileHeader))
		return ProbeWantMoreData;

	const auto result = fileHeader.IsValid();
	if(!result.totalSampleSize)
		return ProbeFailure;

	return ProbeAdditionalSize(file, pfilesize, result.numPatterns * 1024u);
}


bool CSoundFile::ReadNRU(FileReader &file, ModLoadingFlags loadFlags)
{
	NRUFileHeader fileHeader;
	file.Rewind();
	if(!file.ReadStruct(fileHeader))
		return false;

	const auto headerResult = fileHeader.IsValid();
	if(!headerResult.totalSampleSize)
		return false;
	if(loadFlags == onlyVerifyHeader)
		return true;

	if(!file.CanRead(headerResult.numPatterns * 1024u))
		return false;

	InitializeGlobals(MOD_TYPE_MOD, 4);

	// Reading patterns (done first to avoid doing unnecessary work if this is a real ProTracker M.K. file - which is unlikely at this point)
	if(loadFlags & loadPatternData)
		Patterns.ResizeArray(headerResult.numPatterns);

	for(PATTERNINDEX pat = 0; pat < headerResult.numPatterns; pat++)
	{
		const auto patternData = file.ReadArray<std::array<uint8, 4>, 4 * 64>();
		for(const auto &data : patternData)
		{
			// Effect: lower 2 bits must be 0, and must be a valid MOD effect
			if(data[0] & 0xC3)
				return false;
			// Must be valid ProTracker note * 2
			if(data[2] > 72 || (data[2] & 0x01))
				return false;
			// Instrument: lower 3 bits must be 0
			if(data[3] & 0x07)
				return false;
		}

		if(!(loadFlags & loadPatternData) || !Patterns.Insert(pat, 64))
			continue;

		auto m = Patterns[pat].begin();
		for(const auto &data : patternData)
		{
			if(data[2] > 0)
				m->note = static_cast<ModCommand::NOTE>(data[2] / 2u + NOTE_MIDDLEC - 13);
			m->instr = data[3] >> 3;

			uint8 modEffect;
			if(data[0] == 0x00)
				modEffect = 0x03;
			else if(data[0] == 0x0C)
				modEffect = 0x00;
			else
				modEffect = data[0] >> 2;

			if(modEffect || data[1])
				ConvertModCommand(*m, modEffect, data[1]);
			m++;
		}
	}

	// Reading samples
	if(!file.CanRead(headerResult.totalSampleSize))
		return false;
	m_nSamples = 31;
	for(SAMPLEINDEX smp = 1; smp <= 31; smp++)
	{
		const NRUSampleHeader &sample = fileHeader.sampleHeaders[smp - 1];
		ModSample &mptSmp = Samples[smp];
		mptSmp.Initialize(MOD_TYPE_MOD);
		mptSmp.nLength = sample.length * 2u;
		mptSmp.nVolume = sample.volume * 4u;
		if(sample.finetune < 0)
			mptSmp.nFineTune = MOD2XMFineTune(sample.finetune / -72);
		if(sample.loopLength > 1)
		{
			mptSmp.nLoopStart = sample.loopStartAddr - sample.sampleAddr;
			mptSmp.nLoopEnd = mptSmp.nLoopStart + sample.loopLength * 2u;
			mptSmp.uFlags.set(CHN_LOOP);
		}

		if(!(loadFlags & loadSampleData))
			continue;
		SampleIO(
			SampleIO::_8bit,
			SampleIO::mono,
			SampleIO::bigEndian,
			SampleIO::signedPCM)
			.ReadSample(Samples[smp], file);
	}

	SetupMODPanning(true);
	ReadOrderFromArray(Order(), fileHeader.order.orderList, fileHeader.order.numOrders);
	Order().SetDefaultSpeed(6);
	Order().SetDefaultTempoInt(125);
	m_nMinPeriod = 113 * 4;
	m_nMaxPeriod = 856 * 4;
	m_nSamplePreAmp = 64;
	m_SongFlags.set(SONG_PT_MODE | SONG_IMPORTED | SONG_FORMAT_NO_VOLCOL);
	m_playBehaviour.set(kMODIgnorePanning);
	m_playBehaviour.set(kMODSampleSwap);

	m_modFormat.formatName = UL_("NoiseRunner");
	m_modFormat.type = UL_("nru");
	m_modFormat.charset = mpt::Charset::Amiga_no_C1;  // No strings in this format...

	return true;
}

OPENMPT_NAMESPACE_END
