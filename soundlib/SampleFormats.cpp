/*
 * SampleFormats.cpp
 * -----------------
 * Purpose: Code for loading various more or less common sample and instrument formats.
 * Notes  : Needs a lot of rewriting.
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "sndfile.h"
#ifdef MODPLUG_TRACKER
#include "../mptrack/Moddoc.h"
#endif //MODPLUG_TRACKER
#include "../mptrack/Mainfrm.h" // For CriticalSection
#include "Wav.h"
#include "ITTools.h"
#include "XMTools.h"
#include "WAVTools.h"
#include "../common/Reporting.h"
#include "../mptrack/version.h"
#include "ChunkReader.h"


bool CSoundFile::ReadSampleFromFile(SAMPLEINDEX nSample, const LPBYTE lpMemFile, DWORD dwFileLength)
//--------------------------------------------------------------------------------------------------
{
	FileReader file(reinterpret_cast<const char *>(lpMemFile), dwFileLength);
	if(!nSample || nSample >= MAX_SAMPLES) return false;
	if(!ReadWAVSample(nSample, file)
		&& !ReadXISample(nSample, file)
		&& !ReadITISample(nSample, file)
		&& !ReadAIFFSample(nSample, file)
		&& !ReadITSSample(nSample, file)
		&& !ReadPATSample(nSample, lpMemFile, dwFileLength)
		&& !Read8SVXSample(nSample, lpMemFile, dwFileLength)
		&& !ReadS3ISample(nSample, lpMemFile, dwFileLength)
		&& !ReadFLACSample(nSample, file))
	{
		return false;
	}
	return true;
}


bool CSoundFile::ReadInstrumentFromFile(INSTRUMENTINDEX nInstr, const LPBYTE lpMemFile, DWORD dwFileLength)
//---------------------------------------------------------------------------------------------------------
{
	FileReader file(reinterpret_cast<const char *>(lpMemFile), dwFileLength);
	if ((!nInstr) || (nInstr >= MAX_INSTRUMENTS)) return false;
	if ((!ReadXIInstrument(nInstr, file))
	 && (!ReadPATInstrument(nInstr, lpMemFile, dwFileLength))
	 && (!ReadITIInstrument(nInstr, file))
	// Generic read
	 && (!ReadSampleAsInstrument(nInstr, lpMemFile, dwFileLength))) return false;
	if (nInstr > m_nInstruments) m_nInstruments = nInstr;
	return true;
}


bool CSoundFile::ReadSampleAsInstrument(INSTRUMENTINDEX nInstr, const LPBYTE lpMemFile, DWORD dwFileLength)
//---------------------------------------------------------------------------------------------------------
{
	const uint32 *psig = reinterpret_cast<const uint32 *>(lpMemFile);
	if((!lpMemFile) || (dwFileLength < 80)) return false;
	if((psig[0] == LittleEndian(0x46464952) && psig[2] == LittleEndian(0x45564157))		// RIFF....WAVE signature
	 || (psig[0] == LittleEndian(0x5453494C) && psig[2] == LittleEndian(0x65766177))	// LIST....wave
	 || psig[76/4] == LittleEndian(0x53524353)											// S3I signature
	 || (psig[0] == BigEndian(0x464F524D) && psig[2] == LittleEndian(0x46464941))		// AIFF signature
	 || (psig[0] == BigEndian(0x464F524D) && psig[2] == LittleEndian(0x43464941))		// AIFF-C signature
	 || (psig[0] == BigEndian(0x464F524D) && psig[2] == LittleEndian(0x58565338))		// 8SVX signature
	 || psig[0] == LittleEndian(ITSample::magic)										// ITS signature
	 || psig[0] == LittleEndian('CaLf')													// FLAC signature
	 || psig[0] == LittleEndian('SggO')													// OGG signature (for FLAC in OGG)
	)
	{
		// Scanning free sample
		SAMPLEINDEX nSample = GetNextFreeSample(nInstr);
		if(nSample == SAMPLEINDEX_INVALID)
		{
			return false;
		}
		
		// Loading Instrument

		ModInstrument *pIns;

		try
		{
			pIns = new ModInstrument(nSample);
		} catch(MPTMemoryException)
		{
			return false;
		}
		 
		DestroyInstrument(nInstr, deleteAssociatedSamples);
		Instruments[nInstr] = pIns;

		ReadSampleFromFile(nSample, lpMemFile, dwFileLength);

		if(nSample > GetNumSamples())
		{
			m_nSamples = nSample;
		}
		if(nInstr > GetNumInstruments())
		{
			m_nInstruments = nInstr;
		}

		return true;
	}
	return false;
}


bool CSoundFile::DestroyInstrument(INSTRUMENTINDEX nInstr, deleteInstrumentSamples removeSamples)
//-----------------------------------------------------------------------------------------------
{
	if ((!nInstr) || (nInstr > m_nInstruments)) return false;
	if (!Instruments[nInstr]) return true;

#ifdef MODPLUG_TRACKER
	if(removeSamples == askDeleteAssociatedSamples)
	{
		ConfirmAnswer result = Reporting::Confirm("Remove samples associated with an instrument if they are unused?", "Removing instrument", true);
		if(result == cnfCancel)
		{
			return false;
		}
		removeSamples = (result == cnfYes) ? deleteAssociatedSamples : doNoDeleteAssociatedSamples;
	}
#endif // MODPLUG_TRACKER
	if(removeSamples == deleteAssociatedSamples)
	{
		RemoveInstrumentSamples(nInstr);
	}

// -> CODE#0023
// -> DESC="IT project files (.itp)"
	m_szInstrumentPath[nInstr - 1][0] = '\0';
#ifdef MODPLUG_TRACKER
	if(GetpModDoc())
	{
		GetpModDoc()->m_bsInstrumentModified.reset(nInstr - 1);
	}
#endif // MODPLUG_TRACKER
// -! NEW_FEATURE#0023

	CriticalSection cs;

	ModInstrument *pIns = Instruments[nInstr];
	Instruments[nInstr] = nullptr;
	for(CHANNELINDEX i = 0; i < MAX_CHANNELS; i++)
	{
		if (Chn[i].pModInstrument == pIns)
		{
			Chn[i].pModInstrument = nullptr;
		}
	}
	delete pIns;
	return true;
}


// Removing all unused samples
bool CSoundFile::RemoveInstrumentSamples(INSTRUMENTINDEX nInstr)
//--------------------------------------------------------------
{
	if(Instruments[nInstr] == nullptr)
	{
		return false;
	}

	vector<bool> keepSamples(GetNumSamples() + 1, true);

	// Check which samples are used by the instrument we are going to nuke.
	std::set<SAMPLEINDEX> referencedSamples = Instruments[nInstr]->GetSamples();
	for(std::set<SAMPLEINDEX>::const_iterator sample = referencedSamples.begin(); sample != referencedSamples.end(); sample++)
	{
		if((*sample) <= GetNumSamples())
			keepSamples[*sample] = false;
	}

	// Check if any of those samples are referenced by other instruments as well, in which case we want to keep them of course.
	for(INSTRUMENTINDEX nIns = 1; nIns <= GetNumInstruments(); nIns++) if (Instruments[nIns] != nullptr && nIns != nInstr)
	{
		Instruments[nIns]->GetSamples(keepSamples);
	}

	// Now nuke the selected samples.
	RemoveSelectedSamples(keepSamples);
	return true;
}

////////////////////////////////////////////////////////////////////////////////
//
// I/O From another song
//

bool CSoundFile::ReadInstrumentFromSong(INSTRUMENTINDEX targetInstr, const CSoundFile *pSrcSong, INSTRUMENTINDEX sourceInstr)
//---------------------------------------------------------------------------------------------------------------------------
{
	if ((!pSrcSong) || (!sourceInstr) || (sourceInstr > pSrcSong->GetNumInstruments())
		|| (targetInstr >= MAX_INSTRUMENTS) || (!pSrcSong->Instruments[sourceInstr]))
	{
		return false;
	}
	if (m_nInstruments < targetInstr) m_nInstruments = targetInstr;

	ModInstrument *pIns;

	try
	{
		pIns = new ModInstrument();
	} catch(MPTMemoryException)
	{
		return false;
	}
		
	DestroyInstrument(targetInstr, deleteAssociatedSamples);

	Instruments[targetInstr] = pIns;
	*pIns = *pSrcSong->Instruments[sourceInstr];

	vector<SAMPLEINDEX> sourceSample;	// Sample index in source song
	vector<SAMPLEINDEX> targetSample;	// Sample index in target song
	SAMPLEINDEX targetIndex = 0;		// Next index for inserting sample

	for(size_t i = 0; i < CountOf(pIns->Keyboard); i++)
	{
		const SAMPLEINDEX sourceIndex = pIns->Keyboard[i];
		if(sourceIndex > 0 && sourceIndex <= pSrcSong->GetNumSamples())
		{
			const vector<SAMPLEINDEX>::const_iterator entry = std::find(sourceSample.begin(), sourceSample.end(), sourceIndex);
			if(entry == sourceSample.end())
			{
				// Didn't consider this sample yet, so add it to our map.
				targetIndex = GetNextFreeSample(targetInstr, targetIndex + 1);
				if(targetIndex <= GetModSpecifications().samplesMax)
				{
					sourceSample.push_back(sourceIndex);
					targetSample.push_back(targetIndex);
					pIns->Keyboard[i] = targetIndex;
				} else
				{
					pIns->Keyboard[i] = 0;
				}
			} else
			{
				// Sample reference has already been created, so only need to update the sample map.
				pIns->Keyboard[i] = *(entry - sourceSample.begin() + targetSample.begin());
			}
		} else
		{
			// Invalid or no source sample
			pIns->Keyboard[i] = 0;
		}
	}

	pIns->Convert(pSrcSong->GetType(), GetType());

	// Copy all referenced samples over
	for(size_t i = 0; i < targetSample.size(); i++)
	{
		ReadSampleFromSong(targetSample[i], pSrcSong, sourceSample[i]);
	}

	return true;
}


bool CSoundFile::ReadSampleFromSong(SAMPLEINDEX targetSample, const CSoundFile *pSrcSong, SAMPLEINDEX sourceSample)
//-----------------------------------------------------------------------------------------------------------------
{
	if(pSrcSong == nullptr || !sourceSample || sourceSample > pSrcSong->GetNumSamples() || targetSample >= GetModSpecifications().samplesMax)
	{
		return false;
	}

	CriticalSection cs;
	DestroySample(targetSample);

	const ModSample &sourceSmp = pSrcSong->GetSample(sourceSample);

	if(GetNumSamples() < targetSample) m_nSamples = targetSample;
	Samples[targetSample] = sourceSmp;
	Samples[targetSample].Convert(pSrcSong->GetType(), GetType());
	strcpy(m_szNames[targetSample], pSrcSong->m_szNames[sourceSample]);

	if(sourceSmp.pSample)
	{
		Samples[targetSample].pSample = nullptr;	// Don't want to delete the original sample!
		if(Samples[targetSample].AllocateSample())
		{
			SmpLength nSize = sourceSmp.GetSampleSizeInBytes();
			memcpy(Samples[targetSample].pSample, sourceSmp.pSample, nSize);
			AdjustSampleLoop(Samples[targetSample]);
		}
	}

	return true;
}


////////////////////////////////////////////////////////////////////////////////
// WAV Open

extern bool IMAADPCMUnpack16(int16 *target, SmpLength sampleLen, const void *source, size_t sourceSize, uint16 blockAlign);

bool CSoundFile::ReadWAVSample(SAMPLEINDEX nSample, FileReader &file, FileReader *wsmpChunk)
//------------------------------------------------------------------------------------------
{
	WAVReader wavFile(file);

	if(!wavFile.IsValid()
		|| wavFile.GetNumChannels() == 0
		|| wavFile.GetNumChannels() > 2
		|| wavFile.GetBitsPerSample() == 0
		|| wavFile.GetBitsPerSample() > 32
		|| (wavFile.GetSampleFormat() != WAVFormatChunk::fmtPCM && wavFile.GetSampleFormat() != WAVFormatChunk::fmtFloat && wavFile.GetSampleFormat() != WAVFormatChunk::fmtIMA_ADPCM))
	{
		return false;
	}

	CriticalSection cs;
	DestroySample(nSample);
	strcpy(m_szNames[nSample], "");
	ModSample &sample = Samples[nSample];
	sample.Initialize();
	sample.nLength = wavFile.GetSampleLength();
	sample.nC5Speed = wavFile.GetSampleRate();
	wavFile.ApplySampleSettings(sample, m_szNames[nSample]);
	sample.Convert(MOD_TYPE_IT, GetType());

	FileReader sampleChunk = wavFile.GetSampleData();

	if(wavFile.GetSampleFormat() == WAVFormatChunk::fmtIMA_ADPCM)
	{
		// IMA ADPCM 4:1
		LimitMax(sample.nLength, MAX_SAMPLE_LENGTH);
		sample.uFlags |= CHN_16BIT;
		if(!sample.AllocateSample())
		{
			return false;
		}
		IMAADPCMUnpack16((int16 *)sample.pSample, sample.nLength, sampleChunk.GetRawData(), sampleChunk.BytesLeft(), wavFile.GetBlockAlign());
		AdjustSampleLoop(sample);
	} else
	{
		// PCM / Float
		static const SampleIO::Bitdepth bitDepth[] = { SampleIO::_8bit, SampleIO::_16bit, SampleIO::_24bit, SampleIO::_32bit };

		SampleIO sampleIO(
			bitDepth[(wavFile.GetBitsPerSample() - 1) / 8],
			(wavFile.GetNumChannels() > 1) ? SampleIO::stereoInterleaved : SampleIO::mono,
			SampleIO::littleEndian,
			(wavFile.GetBitsPerSample() > 8) ? SampleIO::signedPCM : SampleIO::unsignedPCM);

		if(wavFile.GetSampleFormat() == WAVFormatChunk::fmtFloat)
		{
			sampleIO |= SampleIO::floatPCM;
		}

		sampleIO.ReadSample(sample, sampleChunk);
	}

	if(wsmpChunk != nullptr)
	{
		// DLS WSMP chunk
		*wsmpChunk = wavFile.GetWsmpChunk();
	}

	return true;
}


///////////////////////////////////////////////////////////////
// Save WAV


bool CSoundFile::SaveWAVSample(SAMPLEINDEX nSample, const LPCSTR lpszFileName) const
//----------------------------------------------------------------------------------
{
	char *softwareId = "OpenMPT " MPT_VERSION_STR;
	size_t softwareIdLength = strlen(softwareId) + 1;

	WAVEFILEHEADER header;
	WAVEFORMATHEADER format;
	WAVEDATAHEADER data;
	WAVESAMPLERINFO smpl;
	WAVELISTHEADER list;
	WAVEEXTRAHEADER extra;
	const ModSample &sample = Samples[nSample];
	FILE *f;

	if ((f = fopen(lpszFileName, "wb")) == NULL) return false;
	MemsetZero(extra);
	MemsetZero(smpl);
	header.id_RIFF = LittleEndian(IFFID_RIFF);
	header.filesize = sizeof(header) - 8 + sizeof(format) + sizeof(data) + sizeof(extra)
		+ sizeof(list) + 8 + softwareIdLength + 8 + 32; // LIST(INAM, ISFT)
	header.id_WAVE = LittleEndian(IFFID_WAVE);
	format.id_fmt = LittleEndian(IFFID_fmt);
	format.hdrlen = LittleEndian(16);
	format.format = LittleEndianW(1);
	if(!(GetType() & (MOD_TYPE_MOD | MOD_TYPE_XM)))
		format.freqHz = LittleEndian(sample.nC5Speed);
	else
		format.freqHz = LittleEndian(ModSample::TransposeToFrequency(sample.RelativeTone, sample.nFineTune));
	format.channels = LittleEndianW(sample.GetNumChannels());
	format.bitspersample = LittleEndianW(sample.GetElementarySampleSize() * 8);
	format.samplesize = LittleEndianW(sample.GetBytesPerSample());
	format.bytessec = LittleEndian(format.freqHz * format.samplesize);

	data.id_data = LittleEndian(IFFID_data);
	data.length = sample.GetSampleSizeInBytes();

	header.filesize += data.length;
	if((data.length % 2u) != 0)
	{
		// Write padding byte if sample size is odd.
		header.filesize++;
	}
	if((softwareIdLength % 2u) != 0)
	{
		header.filesize++;
	}

	// "smpl" field
	smpl.wsiHdr.smpl_id = LittleEndian(IFFID_smpl);
	smpl.wsiHdr.smpl_len = sizeof(WAVESMPLHEADER) - 8;
	if (sample.nC5Speed >= 256)
		smpl.wsiHdr.dwSamplePeriod = LittleEndian(1000000000 / sample.nC5Speed);
	else
		smpl.wsiHdr.dwSamplePeriod = LittleEndian(22675);	// 44100 Hz

	smpl.wsiHdr.dwBaseNote = LittleEndian(NOTE_MIDDLEC - NOTE_MIN);

	// Write loops
	if((sample.uFlags & CHN_SUSTAINLOOP) != 0)
	{
		smpl.wsiLoops[smpl.wsiHdr.dwSampleLoops++].SetLoop(sample.nSustainStart, sample.nSustainEnd, (sample.uFlags & CHN_PINGPONGSUSTAIN) != 0);
		smpl.wsiHdr.smpl_len += sizeof(SAMPLELOOPSTRUCT);
	}
	if((sample.uFlags & CHN_LOOP) != 0)
	{
		smpl.wsiLoops[smpl.wsiHdr.dwSampleLoops++].SetLoop(sample.nLoopStart, sample.nLoopEnd, (sample.uFlags & CHN_PINGPONGLOOP) != 0);
		smpl.wsiHdr.smpl_len += sizeof(SAMPLELOOPSTRUCT);
	}

	// Update file length in header
	header.filesize += smpl.wsiHdr.smpl_len + 8;

	header.filesize = LittleEndian(header.filesize);
	data.length = LittleEndian(data.length);
	smpl.wsiHdr.smpl_len = LittleEndian(smpl.wsiHdr.smpl_len);
	fwrite(&header, 1, sizeof(header), f);
	fwrite(&format, 1, sizeof(format), f);
	fwrite(&data, 1, sizeof(data), f);

	SampleIO(
		(sample.uFlags & CHN_16BIT) ? SampleIO::_16bit : SampleIO::_8bit,
		(sample.uFlags & CHN_STEREO) ? SampleIO::stereoInterleaved : SampleIO::mono,
		SampleIO::littleEndian,
		(sample.uFlags & CHN_16BIT) ? SampleIO::signedPCM : SampleIO::unsignedPCM)
		.WriteSample(f, sample);

	if((data.length % 2u) != 0)
	{
		// Write padding byte if sample size is odd.
		int8 padding = 0;
		fwrite(&padding, 1, 1, f);
	}
	fwrite(&smpl, 1, smpl.wsiHdr.smpl_len + 8, f);

	// "LIST" field
	list.list_id = LittleEndian(IFFID_LIST);
	list.list_len = LittleEndian(sizeof(list) - 8	// LIST
					+ 8 + 32						// "INAM".dwLen.szSampleName
					+ 8 + softwareIdLength);		// "ISFT".dwLen.softwareId.0
	list.info = LittleEndian(IFFID_INFO);
	fwrite(&list, 1, sizeof(list), f);

	list.list_id = LittleEndian(IFFID_INAM);		// "INAM"
	list.list_len = LittleEndian(32);
	fwrite(&list, 1, 8, f);
	fwrite(m_szNames[nSample], 1, 32, f);

	list.list_id = LittleEndian(IFFID_ISFT);		// "ISFT"
	list.list_len = LittleEndian(softwareIdLength);
	fwrite(&list, 1, 8, f);
	fwrite(softwareId, 1, list.list_len, f);
	if((softwareIdLength % 2u) != 0)
	{
		int8 padding = 0;
		fwrite(&padding, 1, 1, f);
	}

	// "xtra" field
	extra.xtra_id = LittleEndian(IFFID_xtra);
	extra.xtra_len = LittleEndian(sizeof(extra) - 8);

	extra.dwFlags = LittleEndian(sample.uFlags);
	extra.wPan = LittleEndianW(sample.nPan);
	extra.wVolume = LittleEndianW(sample.nVolume);
	extra.wGlobalVol = LittleEndianW(sample.nGlobalVol);
	extra.wReserved = 0;

	extra.nVibType = sample.nVibType;
	extra.nVibSweep = sample.nVibSweep;
	extra.nVibDepth = sample.nVibDepth;
	extra.nVibRate = sample.nVibRate;
	if((GetType() & MOD_TYPE_XM) && (extra.nVibDepth | extra.nVibRate))
	{
		// XM vibrato is upside down
		extra.nVibSweep = 255 - extra.nVibSweep;
	}

	fwrite(&extra, 1, sizeof(extra), f);
	fclose(f);
	return true;
}

///////////////////////////////////////////////////////////////
// Save RAW

bool CSoundFile::SaveRAWSample(SAMPLEINDEX nSample, const LPCSTR lpszFileName) const
//----------------------------------------------------------------------------------
{
	FILE *f;
	if ((f = fopen(lpszFileName, "wb")) == NULL) return false;

	const ModSample &sample = Samples[nSample];
	SampleIO(
		sample.uFlags[CHN_16BIT] ? SampleIO::_16bit : SampleIO::_8bit,
		sample.uFlags[CHN_STEREO] ? SampleIO::stereoInterleaved : SampleIO::mono,
		SampleIO::littleEndian,
		SampleIO::signedPCM)
		.WriteSample(f, sample);

	fclose(f);
	return true;
}

/////////////////////////////////////////////////////////////
// GUS Patches

#pragma pack(push, 1)

typedef struct GF1PATCHFILEHEADER
{
	DWORD gf1p;				// "GF1P"
	DWORD atch;				// "ATCH"
	CHAR version[4];		// "100", or "110"
	CHAR id[10];			// "ID#000002"
	CHAR copyright[60];		// Copyright
	BYTE instrum;			// Number of instruments in patch
	BYTE voices;			// Number of voices, usually 14
	BYTE channels;			// Number of wav channels that can be played concurently to the patch
	WORD samples;			// Total number of waveforms for all the .PAT
	WORD volume;			// Master volume
	DWORD data_size;
	BYTE reserved2[36];
} GF1PATCHFILEHEADER;


typedef struct GF1INSTRUMENT
{
	WORD id;				// Instrument id: 0-65535
	CHAR name[16];			// Name of instrument. Gravis doesn't seem to use it
	DWORD size;				// Number of bytes for the instrument with header. (To skip to next instrument)
	BYTE layers;			// Number of layers in instrument: 1-4
	BYTE reserved[40];
} GF1INSTRUMENT;


typedef struct GF1SAMPLEHEADER
{
	CHAR name[7];			// null terminated string. name of the wave.
	BYTE fractions;			// Start loop point fraction in 4 bits + End loop point fraction in the 4 other bits.
	DWORD length;			// total size of wavesample. limited to 65535 now by the drivers, not the card.
	DWORD loopstart;		// start loop position in the wavesample
	DWORD loopend;			// end loop position in the wavesample
	WORD freq;				// Rate at which the wavesample has been sampled
	DWORD low_freq, high_freq, root_freq;	// check note.h for the correspondance.
	SHORT finetune;			// fine tune. -512 to +512, EXCLUDING 0 cause it is a multiplier. 512 is one octave off, and 1 is a neutral value
	BYTE balance;			// Balance: 0-15. 0=full left, 15 = full right
	BYTE env_rate[6];		// attack rates
	BYTE env_volume[6];		// attack volumes
	BYTE tremolo_sweep, tremolo_rate, tremolo_depth;
	BYTE vibrato_sweep, vibrato_rate, vibrato_depth;
	BYTE flags;
	SHORT scale_frequency;
	WORD scale_factor;
	BYTE reserved[36];
} GF1SAMPLEHEADER;

// -- GF1 Envelopes --
//
//	It can be represented like this (the envelope is totally bogus, it is
//	just to show the concept):
//
//	|                               
//	|           /----`               | |
//	|   /------/      `\         | | | | |
//	|  /                 \       | | | | |
//	| /                    \     | | | | |
//	|/                       \   | | | | |
//	---------------------------- | | | | | |
//	<---> attack rate 0          0 1 2 3 4 5 amplitudes
//	     <----> attack rate 1
//		     <> attack rate 2
//			 <--> attack rate 3
//			     <> attack rate 4
//				 <-----> attack rate 5
//
// -- GF1 Flags --
//
// bit 0: 8/16 bit
// bit 1: Signed/Unsigned
// bit 2: off/on looping
// bit 3: off/on bidirectionnal looping
// bit 4: off/on backward looping
// bit 5: off/on sustaining (3rd point in env.)
// bit 6: off/on envelopes
// bit 7: off/on clamped release (6th point, env)

typedef struct GF1LAYER
{
	BYTE previous;			// If !=0 the wavesample to use is from the previous layer. The waveheader is still needed
	BYTE id;				// Layer id: 0-3
	DWORD size;				// data size in bytes in the layer, without the header. to skip to next layer for example:
	BYTE samples;			// number of wavesamples
	BYTE reserved[40];
} GF1LAYER;

#pragma pack(pop)

// returns 12*Log2(nFreq/2044)
LONG PatchFreqToNote(ULONG nFreq)
//-------------------------------
{
	const float k_base = 1.0f / 2044.0f;
	const float k_12 = 12;
	LONG result;
	if (nFreq < 1) return 0;
	_asm {
	fld k_12
	fild nFreq
	fld k_base
	fmulp ST(1), ST(0)
	fyl2x
	fistp result
	}
	return result;
}


void PatchToSample(CSoundFile *that, SAMPLEINDEX nSample, LPBYTE lpStream, DWORD dwMemLength)
//-------------------------------------------------------------------------------------------
{
	ModSample &sample = that->GetSample(nSample);
	DWORD dwMemPos = sizeof(GF1SAMPLEHEADER);
	GF1SAMPLEHEADER *psh = (GF1SAMPLEHEADER *)(lpStream);

	if(dwMemLength < sizeof(GF1SAMPLEHEADER)) return;
	if(psh->name[0])
	{
		StringFixer::ReadString<StringFixer::maybeNullTerminated>(that->m_szNames[nSample], psh->name);
	}
	sample.Initialize();
	if(psh->flags & 4) sample.uFlags |= CHN_LOOP;
	if(psh->flags & 8) sample.uFlags |= CHN_PINGPONGLOOP;
	sample.nLength = psh->length;
	sample.nLoopStart = psh->loopstart;
	sample.nLoopEnd = psh->loopend;
	sample.nC5Speed = psh->freq;
	sample.nPan = (psh->balance << 4) + 8;
	if(sample.nPan > 256) sample.nPan = 128;
	sample.nVibType = VIB_SINE;
	sample.nVibSweep = psh->vibrato_sweep;
	sample.nVibDepth = psh->vibrato_depth;
	sample.nVibRate = psh->vibrato_rate/4;
	sample.FrequencyToTranspose();
	sample.RelativeTone += static_cast<uint8>(84 - PatchFreqToNote(psh->root_freq));
	if(psh->scale_factor)
	{
		sample.RelativeTone = static_cast<uint8>(sample.RelativeTone - (psh->scale_frequency - 60));
	}
	sample.TransposeToFrequency();

	SampleIO sampleIO(
		SampleIO::_8bit,
		SampleIO::mono,
		SampleIO::littleEndian,
		(psh->flags & 2) ? SampleIO::unsignedPCM : SampleIO::signedPCM);

	if(psh->flags & 1)
	{
		sampleIO |= SampleIO::_16bit;
		sample.nLength /= 2;
		sample.nLoopStart /= 2;
		sample.nLoopEnd /= 2;
	}
	sampleIO.ReadSample(sample, (LPSTR)(lpStream+dwMemPos), dwMemLength-dwMemPos);
}


bool CSoundFile::ReadPATSample(SAMPLEINDEX nSample, LPBYTE lpStream, DWORD dwMemLength)
//-------------------------------------------------------------------------------------
{
	DWORD dwMemPos = sizeof(GF1PATCHFILEHEADER)+sizeof(GF1INSTRUMENT)+sizeof(GF1LAYER);
	GF1PATCHFILEHEADER *phdr = (GF1PATCHFILEHEADER *)lpStream;
	GF1INSTRUMENT *pinshdr = (GF1INSTRUMENT *)(lpStream+sizeof(GF1PATCHFILEHEADER));

	if ((!lpStream) || (dwMemLength < 512)
	 || (phdr->gf1p != 0x50314647) || (phdr->atch != 0x48435441)
	 || (phdr->version[3] != 0) || (phdr->id[9] != 0) || (phdr->instrum < 1)
	 || (!phdr->samples) || (!pinshdr->layers)) return false;
	
	CriticalSection cs;
	DestroySample(nSample);
	PatchToSample(this, nSample, lpStream+dwMemPos, dwMemLength-dwMemPos);
	if (pinshdr->name[0] > ' ')
	{
		memcpy(m_szNames[nSample], pinshdr->name, 16);
		m_szNames[nSample][16] = 0;
	}
	return true;
}


// PAT Instrument
bool CSoundFile::ReadPATInstrument(INSTRUMENTINDEX nInstr, LPBYTE lpStream, DWORD dwMemLength)
//--------------------------------------------------------------------------------------------
{
	GF1PATCHFILEHEADER *phdr = (GF1PATCHFILEHEADER *)lpStream;
	GF1INSTRUMENT *pih = (GF1INSTRUMENT *)(lpStream+sizeof(GF1PATCHFILEHEADER));
	GF1LAYER *plh = (GF1LAYER *)(lpStream+sizeof(GF1PATCHFILEHEADER)+sizeof(GF1INSTRUMENT));
	ModInstrument *pIns;
	DWORD dwMemPos = sizeof(GF1PATCHFILEHEADER)+sizeof(GF1INSTRUMENT)+sizeof(GF1LAYER);
	UINT nSamples;

	if ((!lpStream) || (dwMemLength < 512)
	 || (phdr->gf1p != 0x50314647) || (phdr->atch != 0x48435441)
	 || (phdr->version[3] != 0) || (phdr->id[9] != 0)
	 || (phdr->instrum < 1) || (!phdr->samples)
	 || (!pih->layers) || (!plh->samples)) return false;
	if (nInstr > m_nInstruments) m_nInstruments = nInstr;

	try
	{
		pIns = new ModInstrument();
	} catch(MPTMemoryException)
	{
		return false;
	}

	DestroyInstrument(nInstr, deleteAssociatedSamples);

	Instruments[nInstr] = pIns;
	nSamples = plh->samples;
	if (nSamples > 16) nSamples = 16;
	memcpy(pIns->name, pih->name, 16);
	pIns->name[16] = 0;
	pIns->nFadeOut = 2048;
	if (GetType() & (MOD_TYPE_IT|MOD_TYPE_MPT))
	{
		pIns->nNNA = NNA_NOTEOFF;
		pIns->nDNA = DNA_NOTEFADE;
	}
	SAMPLEINDEX nFreeSmp = 0;
	UINT nMinSmpNote = 0xff;
	SAMPLEINDEX nMinSmp = 0;
	for (UINT iSmp=0; iSmp<nSamples; iSmp++)
	{
		// Find a free sample
		nFreeSmp = GetNextFreeSample(nInstr, nFreeSmp + 1);
		if (nFreeSmp == SAMPLEINDEX_INVALID) break;
		if (m_nSamples < nFreeSmp) m_nSamples = nFreeSmp;
		if (!nMinSmp) nMinSmp = nFreeSmp;
		// Load it
		GF1SAMPLEHEADER *psh = (GF1SAMPLEHEADER *)(lpStream+dwMemPos);
		PatchToSample(this, nFreeSmp, lpStream+dwMemPos, dwMemLength-dwMemPos);
		LONG nMinNote = (psh->low_freq > 100) ? PatchFreqToNote(psh->low_freq) : 0;
		LONG nMaxNote = (psh->high_freq > 100) ? PatchFreqToNote(psh->high_freq) : NOTE_MAX;
		LONG nBaseNote = (psh->root_freq > 100) ? PatchFreqToNote(psh->root_freq) : -1;
		if ((!psh->scale_factor) && (nSamples == 1)) { nMinNote = 0; nMaxNote = NOTE_MAX; }
		// Fill Note Map
		for (UINT k=0; k<NOTE_MAX; k++)
		{
			if (((LONG)k == nBaseNote)
			 || ((!pIns->Keyboard[k])
			  && ((LONG)k >= nMinNote)
			  && ((LONG)k <= nMaxNote)))
			{
				if(!psh->scale_factor)
					pIns->NoteMap[k] = NOTE_MIDDLEC;

				pIns->Keyboard[k] = nFreeSmp;
				if (k < nMinSmpNote)
				{
					nMinSmpNote = k;
					nMinSmp = nFreeSmp;
				}
			}
		}
	/*
		// Create dummy envelope
		if (!iSmp)
		{
			pIns->dwFlags |= ENV_VOLUME;
			if (psh->flags & 32) pIns->dwFlags |= ENV_VOLSUSTAIN;
			pIns->VolEnv.Values[0] = 64;
			pIns->VolEnv.Ticks[0] = 0;
			pIns->VolEnv.Values[1] = 64;
			pIns->VolEnv.Ticks[1] = 1;
			pIns->VolEnv.Values[2] = 32;
			pIns->VolEnv.Ticks[2] = 20;
			pIns->VolEnv.Values[3] = 0;
			pIns->VolEnv.Ticks[3] = 100;
			pIns->VolEnv.nNodes = 4;
		}
	*/
		// Skip to next sample
		dwMemPos += sizeof(GF1SAMPLEHEADER)+psh->length;
		if (dwMemPos + sizeof(GF1SAMPLEHEADER) >= dwMemLength) break;
	}
	if (nMinSmp)
	{
		// Fill note map and missing samples
		for (UINT k=0; k<NOTE_MAX; k++)
		{
			if (!pIns->NoteMap[k]) pIns->NoteMap[k] = (BYTE)(k+1);
			if (!pIns->Keyboard[k])
			{
				pIns->Keyboard[k] = nMinSmp;
			} else
			{
				nMinSmp = pIns->Keyboard[k];
			}
		}
	}
	return true;
}


/////////////////////////////////////////////////////////////
// S3I Samples

typedef struct S3ISAMPLESTRUCT
{
	BYTE id;
	char filename[12];
	BYTE reserved1;
	WORD offset;
	DWORD length;
	DWORD loopstart;
	DWORD loopend;
	BYTE volume;
	BYTE reserved2;
	BYTE pack;
	BYTE flags;
	DWORD nC5Speed;
	DWORD reserved3;
	DWORD reserved4;
	DWORD date;
	CHAR name[28];
	DWORD scrs;
} S3ISAMPLESTRUCT;

bool CSoundFile::ReadS3ISample(SAMPLEINDEX nSample, const LPBYTE lpMemFile, DWORD dwFileLength)
//---------------------------------------------------------------------------------------------
{
	// TODO: Rewrite using already existing structs from Load_s3m.cpp
	S3ISAMPLESTRUCT *pss = (S3ISAMPLESTRUCT *)lpMemFile;
	ModSample &sample = Samples[nSample];
	DWORD dwMemPos;
	SampleIO flags;

	if ((!lpMemFile) || (dwFileLength < sizeof(S3ISAMPLESTRUCT))
	 || (pss->id != 0x01) || (((DWORD)pss->offset << 4) >= dwFileLength)
	 || (pss->scrs != 0x53524353)) return false;

	CriticalSection cs;
	DestroySample(nSample);
	dwMemPos = pss->offset << 4;

	sample.Initialize();
	StringFixer::ReadString<StringFixer::maybeNullTerminated>(sample.filename, pss->filename);
	StringFixer::ReadString<StringFixer::nullTerminated>(m_szNames[nSample], pss->name);

	sample.nLength = pss->length;
	sample.nLoopStart = pss->loopstart;
	sample.nLoopEnd = pss->loopend;
	sample.nVolume = pss->volume << 2;
	sample.nC5Speed = pss->nC5Speed;
	if(pss->flags & 0x01) sample.uFlags |= CHN_LOOP;

	sample.Convert(MOD_TYPE_S3M, GetType());

	SampleIO(
		(pss->flags & 0x04) ? SampleIO::_16bit : SampleIO::_8bit,
		(pss->flags & 0x02) ? SampleIO::stereoSplit : SampleIO::mono,
		SampleIO::littleEndian,
		SampleIO::unsignedPCM)
		.ReadSample(sample, (LPSTR)(lpMemFile + dwMemPos), dwFileLength - dwMemPos);

	return true;
}


/////////////////////////////////////////////////////////////
// XI Instruments


bool CSoundFile::ReadXIInstrument(INSTRUMENTINDEX nInstr, FileReader &file)
//-------------------------------------------------------------------------
{
	file.Rewind();

	XIInstrumentHeader fileHeader;
	if(!file.ReadConvertEndianness(fileHeader)
		|| memcmp(fileHeader.signature, "Extended Instrument: ", 21)
		|| fileHeader.version != XIInstrumentHeader::fileVersion
		|| fileHeader.eof != 0x1A)
	{
		return false;
	}

	ModInstrument *pIns;

	try
	{
		pIns = new ModInstrument();
	} catch(MPTMemoryException)
	{
		return false;
	}

	DestroyInstrument(nInstr, deleteAssociatedSamples);
	if(nInstr > m_nInstruments)
	{
		m_nInstruments = nInstr;
	}
	Instruments[nInstr] = pIns;

	fileHeader.ConvertToMPT(*pIns);

	// Translate sample map and find available sample slots
	vector<SAMPLEINDEX> sampleMap(fileHeader.numSamples);
	SAMPLEINDEX maxSmp = 0;

	for(size_t i = 0 + 12; i < 96 + 12; i++)
	{
		if(pIns->Keyboard[i] >= fileHeader.numSamples)
		{
			continue;
		}

		if(sampleMap[pIns->Keyboard[i]] == 0)
		{
			// Find slot for this sample
			maxSmp = GetNextFreeSample(nInstr, maxSmp + 1);
			if(maxSmp != SAMPLEINDEX_INVALID)
			{
				sampleMap[pIns->Keyboard[i]] = maxSmp;
			}
		}
		pIns->Keyboard[i] = sampleMap[pIns->Keyboard[i]];
	}

	if(m_nSamples < maxSmp)
	{
		m_nSamples = maxSmp;
	}

	vector<SampleIO> sampleFlags(fileHeader.numSamples);

	// Read sample headers
	for(SAMPLEINDEX i = 0; i < fileHeader.numSamples; i++)
	{
		XMSample sampleHeader;
		if(!file.ReadConvertEndianness(sampleHeader)
			|| !sampleMap[i])
		{
			continue;
		}

		ModSample &mptSample = Samples[sampleMap[i]];
		sampleHeader.ConvertToMPT(mptSample);
		fileHeader.instrument.ApplyAutoVibratoToMPT(mptSample);
		mptSample.Convert(MOD_TYPE_XM, GetType());
		if(GetType() != MOD_TYPE_XM && fileHeader.numSamples == 1)
		{
			// No need to pan that single sample, thank you...
			mptSample.uFlags &= ~CHN_PANNING;
		}

		StringFixer::ReadString<StringFixer::spacePadded>(mptSample.filename, sampleHeader.name);
		StringFixer::ReadString<StringFixer::spacePadded>(m_szNames[sampleMap[i]], sampleHeader.name);

		sampleFlags[i] = sampleHeader.GetSampleFormat();
	}

	// Read sample data
	for(SAMPLEINDEX i = 0; i < fileHeader.numSamples; i++)
	{
		if(sampleMap[i])
		{
			sampleFlags[i].ReadSample(Samples[sampleMap[i]], file);
		}
	}

	pIns->Convert(MOD_TYPE_XM, GetType());

	// Read MPT crap
	ReadExtendedInstrumentProperties(pIns, file);
	return true;
}


bool CSoundFile::SaveXIInstrument(INSTRUMENTINDEX nInstr, const LPCSTR lpszFileName) const
//----------------------------------------------------------------------------------------
{
	ModInstrument *pIns = Instruments[nInstr];
	if(pIns == nullptr || lpszFileName == nullptr)
	{
		return false;
	}

	FILE *f;
	if((f = fopen(lpszFileName, "wb")) == nullptr)
	{
		return false;
	}

	// Create file header
	XIInstrumentHeader header;
	header.ConvertToXM(*pIns, false);

	const vector<SAMPLEINDEX> samples = header.instrument.GetSampleList(*pIns, false);
	if(samples.size() > 0 && samples[0] <= GetNumSamples())
	{
		// Copy over auto-vibrato settings of first sample
		header.instrument.ApplyAutoVibratoToXM(Samples[samples[0]], GetType());
	}

	header.ConvertEndianness();
	fwrite(&header, 1, sizeof(XIInstrumentHeader), f);

	vector<SampleIO> sampleFlags(samples.size());

	// XI Sample Headers
	for(SAMPLEINDEX i = 0; i < samples.size(); i++)
	{
		XMSample xmSample;
		if(samples[i] <= GetNumSamples())
		{
			xmSample.ConvertToXM(Samples[samples[i]], GetType(), false);
		} else
		{
			MemsetZero(xmSample);
		}
		sampleFlags[i] = xmSample.GetSampleFormat();

		StringFixer::WriteString<StringFixer::spacePadded>(xmSample.name, m_szNames[samples[i]]);

		xmSample.ConvertEndianness();
		fwrite(&xmSample, 1, sizeof(xmSample), f);
	}

	// XI Sample Data
	for(SAMPLEINDEX i = 0; i < samples.size(); i++)
	{
		if(samples[i] <= GetNumSamples())
		{
			sampleFlags[i].WriteSample(f, Samples[samples[i]]);
		}
	}

	int32 code = 'MPTX';
	fwrite(&code, 1, sizeof(int32), f);		// Write extension tag
	WriteInstrumentHeaderStruct(pIns, f);	// Write full extended header.

	fclose(f);
	return true;
}


// Read first sample from XI file into a sample slot
bool CSoundFile::ReadXISample(SAMPLEINDEX nSample, FileReader &file)
//------------------------------------------------------------------
{
	file.Rewind();

	XIInstrumentHeader fileHeader;
	if(!file.ReadConvertEndianness(fileHeader)
		|| !file.CanRead(sizeof(XMSample))
		|| memcmp(fileHeader.signature, "Extended Instrument: ", 21)
		|| fileHeader.version != XIInstrumentHeader::fileVersion
		|| fileHeader.eof != 0x1A
		|| fileHeader.numSamples == 0)
	{
		return false;
	}

	if(m_nSamples < nSample)
	{
		m_nSamples = nSample;
	}

	// Read first sample header
	XMSample sampleHeader;
	file.ReadConvertEndianness(sampleHeader);
	// Gotta skip 'em all!
	file.Skip(sizeof(XMSample) * (fileHeader.numSamples - 1));

	CriticalSection cs;
	DestroySample(nSample);

	ModSample &mptSample = Samples[nSample];
	sampleHeader.ConvertToMPT(mptSample);
	if(GetType() != MOD_TYPE_XM)
	{
		// No need to pan that single sample, thank you...
		mptSample.uFlags.reset(CHN_PANNING);
	}
	fileHeader.instrument.ApplyAutoVibratoToMPT(mptSample);
	mptSample.Convert(MOD_TYPE_XM, GetType());

	StringFixer::ReadString<StringFixer::spacePadded>(mptSample.filename, sampleHeader.name);
	StringFixer::ReadString<StringFixer::spacePadded>(m_szNames[nSample], sampleHeader.name);

	// Read sample data
	sampleHeader.GetSampleFormat().ReadSample(Samples[nSample], file);

	return true;
}


/////////////////////////////////////////////////////////////////////////////////////////
// AIFF File I/O


#pragma pack(push, 1)

// AIFF header
struct AIFFHeader
{
	// 32-Bit chunk identifiers
	enum AIFFMagic
	{
		idFORM	= 0x464F524D,
		idAIFF	= 0x41494646,
		idAIFC	= 0x41494643,
	};

	uint32 magic;	// FORM
	uint32 length;	// Size of the file, not including magic and length
	uint32 type;	// AIFF or AIFC

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesBE(magic);
		SwapBytesBE(type);
	}
};


// General IFF Chunk header
struct AIFFChunk
{
	// 32-Bit chunk identifiers
	enum ChunkIdentifiers
	{
		idCOMM	= 0x434F4D4D,
		idSSND	= 0x53534E44,
		idINST	= 0x494E5354,
		idMARK	= 0x4D41524B,
		idNAME	= 0x4E414D45,
	};

	typedef ChunkIdentifiers id_type;

	uint32 id;		// See ChunkIdentifiers
	uint32 length;	// Chunk size without header

	size_t GetLength() const
	{
		uint32 l = length;
		return SwapBytesBE(l);
	}

	id_type GetID() const
	{
		uint32 i = id;
		return static_cast<id_type>(SwapBytesBE(i));
	}
};


// "Common" chunk (in AIFC, a compression ID and compression name follows this header, but apart from that it's identical)
struct AIFFCommonChunk
{
	uint16 numChannels;
	uint32 numSampleFrames;
	uint16 sampleSize;
	uint8  sampleRate[10];		// Sample rate in 80-Bit floating point

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesBE(numChannels);
		SwapBytesBE(numSampleFrames);
		SwapBytesBE(sampleSize);
	}

	// Convert sample rate to integer
	uint32 GetSampleRate() const
	{
		uint32 mantissa = *reinterpret_cast<const uint32 *>(&sampleRate[2]), last = 0;
		uint8 exp = 30 - sampleRate[1];
		SwapBytesBE(mantissa);

		while(exp--)
		{
			last = mantissa;
			mantissa >>= 1;
		}
		if(last & 1) mantissa++;
		return mantissa;
	}
};


// Sound chunk
struct AIFFSoundChunk
{
	uint32 offset;
	uint32 blockSize;

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesBE(offset);
		SwapBytesBE(blockSize);
	}
};


// Marker
struct AIFFMarker
{
	uint16 id;
	uint32 position;		// Position in sample
	uint8  nameLength;		// Not counting eventually existing padding byte in name string

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesBE(id);
		SwapBytesBE(position);
	}};


// Instrument loop
struct AIFFInstrumentLoop
{
	enum PlayModes
	{
		noLoop		= 0,
		loopNormal	= 1,
		loopBidi	= 2,
	};

	uint16 playMode;
	uint16 beginLoop;		// Marker index
	uint16 endLoop;			// Marker index

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesBE(playMode);
		SwapBytesBE(beginLoop);
		SwapBytesBE(endLoop);
	}
};


struct AIFFInstrumentChunk
{
	uint8  baseNote;
	uint8  detune;
	uint8  lowNote;
	uint8  highNote;
	uint8  lowVelocity;
	uint8  highVelocity;
	uint16 gain;
	AIFFInstrumentLoop sustainLoop;
	AIFFInstrumentLoop releaseLoop;

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		sustainLoop.ConvertEndianness();
		releaseLoop.ConvertEndianness();
	}
};

#pragma pack(pop)


bool CSoundFile::ReadAIFFSample(SAMPLEINDEX nSample, FileReader &file)
//--------------------------------------------------------------------
{
	file.Rewind();
	ChunkReader chunkFile(file);

	// Verify header
	AIFFHeader fileHeader;
	if(!chunkFile.ReadConvertEndianness(fileHeader)
		|| fileHeader.magic != AIFFHeader::idFORM
		|| (fileHeader.type != AIFFHeader::idAIFF && fileHeader.type != AIFFHeader::idAIFC))
	{
		return false;
	}

	ChunkReader::ChunkList<AIFFChunk> chunks = chunkFile.ReadChunks<AIFFChunk>(2);

	// Read COMM chunk
	FileReader commChunk(chunks.GetChunk(AIFFChunk::idCOMM));
	AIFFCommonChunk sampleInfo;
	if(!commChunk.ReadConvertEndianness(sampleInfo))
	{
		return false;
	}

	// Is this a proper sample?
	if(sampleInfo.numSampleFrames == 0
		|| sampleInfo.numChannels == 0 || sampleInfo.numChannels > 2
		|| sampleInfo.sampleSize == 0 || sampleInfo.sampleSize > 32)
	{
		return false;
	}

	// Read compression type in AIFF-C files.
	uint8 compression[4] = { 'N', 'O', 'N', 'E' };
	SampleIO::Endianness endian = SampleIO::bigEndian;
	if(fileHeader.type == AIFFHeader::idAIFC)
	{
		if(!commChunk.ReadArray(compression))
		{
			return false;
		}
		if(!memcmp(compression, "twos", 4))
		{
			endian = SampleIO::littleEndian;
		}
	}

	// Read SSND chunk
	FileReader soundChunk(chunks.GetChunk(AIFFChunk::idSSND));
	AIFFSoundChunk sampleHeader;
	if(!soundChunk.ReadConvertEndianness(sampleHeader)
		|| soundChunk.BytesLeft() <= sampleHeader.offset)
	{
		return false;
	}

	static const SampleIO::Bitdepth bitDepth[] = { SampleIO::_8bit, SampleIO::_16bit, SampleIO::_24bit, SampleIO::_32bit };

	SampleIO sampleIO(bitDepth[(sampleInfo.sampleSize - 1) / 8],
		(sampleInfo.numChannels == 2) ? SampleIO::stereoInterleaved : SampleIO::mono,
		endian,
		SampleIO::signedPCM);

	if(!memcmp(compression, "fl32", 4) || !memcmp(compression, "FL32", 4))
	{
		sampleIO |= SampleIO::floatPCM;
	}

	soundChunk.Skip(sampleHeader.offset);

	CriticalSection cs;
	ModSample &mptSample = Samples[nSample];
	DestroySample(nSample);
	mptSample.Initialize();
	mptSample.nLength = sampleInfo.numSampleFrames;
	mptSample.nC5Speed = sampleInfo.GetSampleRate();

	sampleIO.ReadSample(mptSample, soundChunk);

	// Read MARK and INST chunk to extract sample loops
	FileReader markerChunk(chunks.GetChunk(AIFFChunk::idMARK));
	AIFFInstrumentChunk instrHeader;
	if(markerChunk.IsValid() && chunks.GetChunk(AIFFChunk::idINST).ReadConvertEndianness(instrHeader))
	{
		size_t numMarkers = markerChunk.ReadUint16BE();

		vector<AIFFMarker> markers;
		for(size_t i = 0; i < numMarkers; i++)
		{
			AIFFMarker marker;
			if(!markerChunk.ReadConvertEndianness(marker))
			{
				break;
			}
			markers.push_back(marker);
			markerChunk.Skip(marker.nameLength + ((marker.nameLength % 2u) == 0 ? 1 : 0));
		}

		if(instrHeader.sustainLoop.playMode != AIFFInstrumentLoop::noLoop)
		{
			mptSample.uFlags.set(CHN_SUSTAINLOOP);
			mptSample.uFlags.set(CHN_PINGPONGSUSTAIN, instrHeader.sustainLoop.playMode == AIFFInstrumentLoop::loopBidi);
		}

		if(instrHeader.releaseLoop.playMode != AIFFInstrumentLoop::noLoop)
		{
			mptSample.uFlags.set(CHN_LOOP);
			mptSample.uFlags.set(CHN_PINGPONGLOOP, instrHeader.releaseLoop.playMode == AIFFInstrumentLoop::loopBidi);
		}

		// Read markers
		for(vector<AIFFMarker>::iterator iter = markers.begin(); iter != markers.end(); iter++)
		{
			if(iter->id == instrHeader.sustainLoop.beginLoop)
			{
				mptSample.nSustainStart = iter->position;
			}
			if(iter->id == instrHeader.sustainLoop.endLoop)
			{
				mptSample.nSustainEnd = iter->position;
			}
			if(iter->id == instrHeader.releaseLoop.beginLoop)
			{
				mptSample.nLoopStart = iter->position;
			}
			if(iter->id == instrHeader.releaseLoop.endLoop)
			{
				mptSample.nLoopEnd = iter->position;
			}
		}
		mptSample.SanitizeLoops();
	}

	// Extract sample name
	FileReader nameChunk(chunks.GetChunk(AIFFChunk::idNAME));
	if(nameChunk.IsValid())
	{
		nameChunk.ReadString<StringFixer::spacePadded>(m_szNames[nSample], nameChunk.GetLength());
	} else
	{
		strcpy(m_szNames[nSample], "");
	}

	mptSample.Convert(MOD_TYPE_IT, GetType());
	return true;
}


/////////////////////////////////////////////////////////////////////////////////////////
// ITS Samples


bool CSoundFile::ReadITSSample(SAMPLEINDEX nSample, FileReader &file, bool rewind)
//--------------------------------------------------------------------------------
{
	if(rewind)
	{
		file.Rewind();
	}

	ITSample sampleHeader;
	if(!file.ReadConvertEndianness(sampleHeader)
		|| sampleHeader.id != ITSample::magic)
	{
		return false;
	}
	CriticalSection cs;
	DestroySample(nSample);

	file.Seek(sampleHeader.ConvertToMPT(Samples[nSample]));
	StringFixer::ReadString<StringFixer::spacePaddedNull>(m_szNames[nSample], sampleHeader.name);
	Samples[nSample].Convert(MOD_TYPE_IT, GetType());

	sampleHeader.GetSampleFormat().ReadSample(Samples[nSample], file);
	return true;
}


bool CSoundFile::ReadITISample(SAMPLEINDEX nSample, FileReader &file)
//-------------------------------------------------------------------
{
	ITInstrument instrumentHeader;

	file.Rewind();
	if(!file.ReadConvertEndianness(instrumentHeader)
		|| instrumentHeader.id != ITInstrument::magic
		|| instrumentHeader.nos == 0)
	{
		return false;
	}
	file.Rewind();
	ModInstrument dummy;
	ITInstrToMPT(file, dummy, instrumentHeader.trkvers);
	ReadITSSample(nSample, file, false);

	return true;
}


bool CSoundFile::ReadITIInstrument(INSTRUMENTINDEX nInstr, FileReader &file)
//--------------------------------------------------------------------------
{
	ITInstrument instrumentHeader;
	SAMPLEINDEX smp = 0, nsamples;

	file.Rewind();
	if(!file.ReadConvertEndianness(instrumentHeader)
		|| instrumentHeader.id != ITInstrument::magic)
	{
		return false;
	}
	if(nInstr > GetNumInstruments()) m_nInstruments = nInstr;

	ModInstrument *pIns;

	try
	{
		pIns = new ModInstrument();
	} catch(MPTMemoryException)
	{
		return false;
	}

	DestroyInstrument(nInstr, deleteAssociatedSamples);

	Instruments[nInstr] = pIns;
	file.Rewind();
	ITInstrToMPT(file, *pIns, instrumentHeader.trkvers);
	nsamples = instrumentHeader.nos;

	// In order to properly compute the position, in file, of eventual extended settings
	// such as "attack" we need to keep the "real" size of the last sample as those extra
	// setting will follow this sample in the file
	FileReader::off_t extraOffset = file.GetPosition();

	// Reading Samples
	vector<SAMPLEINDEX> samplemap(nsamples, 0);
	for(SAMPLEINDEX i = 0; i < nsamples; i++)
	{
		smp = GetNextFreeSample(nInstr, smp + 1);
		if(smp == SAMPLEINDEX_INVALID) break;
		samplemap[i] = smp;
		const FileReader::off_t offset = file.GetPosition();
		ReadITSSample(smp, file, false);
		extraOffset = Util::Max(extraOffset, file.GetPosition());
		file.Seek(offset + sizeof(ITSample));
	}
	if(GetNumSamples() < smp) m_nSamples = smp;

	for(size_t j = 0; j < CountOf(pIns->Keyboard); j++)
	{
		if(pIns->Keyboard[j] && pIns->Keyboard[j] <= nsamples)
		{
			pIns->Keyboard[j] = samplemap[pIns->Keyboard[j] - 1];
		}
	}

	pIns->Convert(MOD_TYPE_IT, GetType());

	if(file.Seek(extraOffset))
	{
		// Read MPT crap
		ReadExtendedInstrumentProperties(pIns, file);
	}

	return true;
}


bool CSoundFile::SaveITIInstrument(INSTRUMENTINDEX nInstr, const LPCSTR lpszFileName) const
//-----------------------------------------------------------------------------------------
{
	ITInstrumentEx iti;
	ModInstrument *pIns = Instruments[nInstr];
	uint32 filePos;
	FILE *f;

	if((!pIns) || (!lpszFileName)) return false;
	if((f = fopen(lpszFileName, "wb")) == NULL) return false;
	
	size_t instSize = iti.ConvertToIT(*pIns, false, *this);

	// Create sample assignment table
	vector<SAMPLEINDEX> smptable;
	vector<SAMPLEINDEX> smpmap(GetNumSamples(), 0);
	for(size_t i = 0; i < NOTE_MAX; i++)
	{
		const SAMPLEINDEX smp = pIns->Keyboard[i];
		if(smp && smp <= GetNumSamples())
		{
			if(!smpmap[smp - 1])
			{
				// We haven't considered this sample yet.
				smptable.push_back(smp);
				smpmap[smp - 1] = static_cast<SAMPLEINDEX>(smptable.size());
			}
			iti.iti.keyboard[i * 2 + 1] = static_cast<uint8>(smpmap[smp - 1]);
		} else
		{
			iti.iti.keyboard[i * 2 + 1] = 0;
		}
	}
	smpmap.clear();

	filePos = instSize;
	iti.ConvertEndianness();
	fwrite(&iti, 1, instSize, f);

	filePos += smptable.size() * sizeof(ITSample);

	// Writing sample headers
	for(vector<SAMPLEINDEX>::iterator iter = smptable.begin(); iter != smptable.end(); iter++)
	{
		ITSample itss;
		itss.ConvertToIT(Samples[*iter], GetType());

		StringFixer::WriteString<StringFixer::nullTerminated>(itss.name, m_szNames[*iter]);

		itss.samplepointer = filePos;
		itss.ConvertEndianness();
		fwrite(&itss, 1, sizeof(itss), f);
		filePos += Samples[*iter].GetSampleSizeInBytes();
	}

	// Writing Sample Data
	for(vector<SAMPLEINDEX>::iterator iter = smptable.begin(); iter != smptable.end(); iter++)
	{
		const ModSample &sample = Samples[*iter];
		// TODO we should actually use ITSample::GetSampleFormat() instead of re-computing the flags here.
		SampleIO(
			(sample.uFlags & CHN_16BIT) ? SampleIO::_16bit : SampleIO::_8bit,
			(sample.uFlags & CHN_STEREO) ? SampleIO::stereoSplit : SampleIO::mono,
			SampleIO::littleEndian,
			SampleIO::signedPCM)
			.WriteSample(f, sample);
	}

	int32 code = 'MPTX';
	SwapBytesLE(code);
	fwrite(&code, 1, sizeof(int32), f);		// Write extension tag
	WriteInstrumentHeaderStruct(pIns, f);	// Write full extended header.

	fclose(f);
	return true;
}


void ReadInstrumentExtensionField(ModInstrument* pIns, const uint32 code, const uint16 size, FileReader &file)
//------------------------------------------------------------------------------------------------------------
{
	// get field's address in instrument's header
	char *fadr = GetInstrumentHeaderFieldPointer(pIns, code, size);
	 
	if(fadr && code != 'K[..')	// copy field data in instrument's header
		memcpy(fadr, file.GetRawData(), size);  // (except for keyboard mapping)
	file.Skip(size);

	if(code == 'dF..' && fadr != nullptr) // 'dF..' field requires additional processing.
		ConvertReadExtendedFlags(pIns);
}


void ReadExtendedInstrumentProperty(ModInstrument* pIns, const uint32 code, FileReader &file)
//-------------------------------------------------------------------------------------------
{
	uint16 size = file.ReadUint16LE();
	if(!file.CanRead(size))
	{
		return;
	}
	ReadInstrumentExtensionField(pIns, code, size, file);
}


void ReadExtendedInstrumentProperties(ModInstrument* pIns, FileReader &file)
//--------------------------------------------------------------------------
{
	if(!file.ReadMagic("XTPM"))	// 'MPTX'
	{
		return;
	}

	while(file.BytesLeft() > 6)
	{
		ReadExtendedInstrumentProperty(pIns, file.ReadUint32LE(), file);
	}
}


void ConvertReadExtendedFlags(ModInstrument *pIns)
//------------------------------------------------
{
	const uint32 dwOldFlags = pIns->dwFlags;

	pIns->VolEnv.dwFlags.set(ENV_ENABLED, (dwOldFlags & dFdd_VOLUME) != 0);
	pIns->VolEnv.dwFlags.set(ENV_SUSTAIN, (dwOldFlags & dFdd_VOLSUSTAIN) != 0);
	pIns->VolEnv.dwFlags.set(ENV_LOOP, (dwOldFlags & dFdd_VOLLOOP) != 0);
	pIns->VolEnv.dwFlags.set(ENV_CARRY, (dwOldFlags & dFdd_VOLCARRY) != 0);

	pIns->PanEnv.dwFlags.set(ENV_ENABLED, (dwOldFlags & dFdd_PANNING) != 0);
	pIns->PanEnv.dwFlags.set(ENV_SUSTAIN, (dwOldFlags & dFdd_PANSUSTAIN) != 0);
	pIns->PanEnv.dwFlags.set(ENV_LOOP, (dwOldFlags & dFdd_PANLOOP) != 0);
	pIns->PanEnv.dwFlags.set(ENV_CARRY, (dwOldFlags & dFdd_PANCARRY) != 0);

	pIns->PitchEnv.dwFlags.set(ENV_ENABLED, (dwOldFlags & dFdd_PITCH) != 0);
	pIns->PitchEnv.dwFlags.set(ENV_SUSTAIN, (dwOldFlags & dFdd_PITCHSUSTAIN) != 0);
	pIns->PitchEnv.dwFlags.set(ENV_LOOP, (dwOldFlags & dFdd_PITCHLOOP) != 0);
	pIns->PitchEnv.dwFlags.set(ENV_CARRY, (dwOldFlags & dFdd_PITCHCARRY) != 0);
	pIns->PitchEnv.dwFlags.set(ENV_FILTER, (dwOldFlags & dFdd_FILTER) != 0);

	pIns->dwFlags.set(INS_SETPANNING, (dwOldFlags & dFdd_SETPANNING) != 0);
	pIns->dwFlags.set(INS_MUTE, (dwOldFlags & dFdd_MUTE) != 0);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// 8SVX Samples

#define IFFID_8SVX	0x58565338
#define IFFID_VHDR	0x52444856
#define IFFID_BODY	0x59444f42
#define IFFID_NAME	0x454d414e
#define IFFID_ANNO	0x4f4e4e41

#pragma pack(push, 1)

typedef struct IFF8SVXFILEHEADER
{
	DWORD dwFORM;	// "FORM"
	DWORD dwSize;
	DWORD dw8SVX;	// "8SVX"
} IFF8SVXFILEHEADER;

typedef struct IFFVHDR
{
	DWORD dwVHDR;	// "VHDR"
	DWORD dwSize;
	ULONG oneShotHiSamples,			/* # samples in the high octave 1-shot part */
			repeatHiSamples,		/* # samples in the high octave repeat part */
			samplesPerHiCycle;		/* # samples/cycle in high octave, else 0 */
	WORD samplesPerSec;				/* data sampling rate */
	BYTE	ctOctave,				/* # octaves of waveforms */
			sCompression;			/* data compression technique used */
	DWORD Volume;
} IFFVHDR;

typedef struct IFFBODY
{
	DWORD dwBody;
	DWORD dwSize;
} IFFBODY;


#pragma pack(pop)



bool CSoundFile::Read8SVXSample(SAMPLEINDEX nSample, LPBYTE lpMemFile, DWORD dwFileLength)
//----------------------------------------------------------------------------------------
{
	IFF8SVXFILEHEADER *pfh = (IFF8SVXFILEHEADER *)lpMemFile;
	IFFVHDR *pvh = (IFFVHDR *)(lpMemFile + 12);
	ModSample &sample = Samples[nSample];
	DWORD dwMemPos = 12;
	
	if ((!lpMemFile) || (dwFileLength < sizeof(IFFVHDR)+12) || (pfh->dwFORM != IFFID_FORM)
	 || (pfh->dw8SVX != IFFID_8SVX) || (BigEndian(pfh->dwSize) >= dwFileLength)
	 || (pvh->dwVHDR != IFFID_VHDR) || (BigEndian(pvh->dwSize) >= dwFileLength)) return false;

	CriticalSection cs;
	DestroySample(nSample);
	// Default values
	sample.Initialize();
	sample.nLoopStart = BigEndian(pvh->oneShotHiSamples);
	sample.nLoopEnd = sample.nLoopStart + BigEndian(pvh->repeatHiSamples);
	sample.nVolume = (WORD)(BigEndianW((WORD)pvh->Volume) >> 8);
	sample.nC5Speed = BigEndianW(pvh->samplesPerSec);
	if((!sample.nVolume) || (sample.nVolume > 256)) sample.nVolume = 256;
	if(!sample.nC5Speed) sample.nC5Speed = 22050;

	sample.Convert(MOD_TYPE_IT, GetType());

	dwMemPos += BigEndian(pvh->dwSize) + 8;
	while (dwMemPos + 8 < dwFileLength)
	{
		DWORD dwChunkId = *((LPDWORD)(lpMemFile+dwMemPos));
		DWORD dwChunkLen = BigEndian(*((LPDWORD)(lpMemFile+dwMemPos+4)));
		LPBYTE pChunkData = (LPBYTE)(lpMemFile+dwMemPos+8);
		// Hack for broken files: Trim claimed length if it's too long
		dwChunkLen = min(dwChunkLen, dwFileLength - dwMemPos);
		switch(dwChunkId)
		{
		case IFFID_NAME:
			{
				const UINT len = min(dwChunkLen, MAX_SAMPLENAME - 1);
				MemsetZero(m_szNames[nSample]);
				memcpy(m_szNames[nSample], pChunkData, len);
			}
			break;
		case IFFID_BODY:
			if (!sample.pSample)
			{
				UINT len = dwChunkLen;
				if (len > dwFileLength - dwMemPos - 8) len = dwFileLength - dwMemPos - 8;
				if (len > 4)
				{
					sample.nLength = len;
					if ((sample.nLoopStart + 4 < sample.nLoopEnd) && (sample.nLoopEnd <= sample.nLength)) sample.uFlags |= CHN_LOOP;

					SampleIO(
						SampleIO::_8bit,
						SampleIO::mono,
						SampleIO::bigEndian,
						SampleIO::signedPCM)
						.ReadSample(sample, (LPSTR)(pChunkData), len);
				}
			}
			break;
		}
		dwMemPos += dwChunkLen + 8;
	}
	return (sample.pSample != nullptr);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// FLAC Samples

#ifndef NO_FLAC
#define FLAC__NO_DLL
#include <flac/include/FLAC/stream_decoder.h>
#include <flac/include/FLAC/stream_encoder.h>
#include <flac/include/FLAC/metadata.h>
#include "SampleFormatConverters.h"

struct FLACDecoder
{
	FileReader &file;
	CSoundFile &sndFile;
	SAMPLEINDEX sample;
	bool ready;

	FLACDecoder(FileReader &f, CSoundFile &sf, SAMPLEINDEX smp) : file(f), sndFile(sf), sample(smp), ready(false) { }

	static FLAC__StreamDecoderReadStatus read_cb(const FLAC__StreamDecoder *, FLAC__byte buffer[], size_t *bytes, void *client_data)
	{
		FileReader &file = static_cast<FLACDecoder *>(client_data)->file;
		if(*bytes > 0)
		{
			FileReader::off_t readBytes = *bytes;
			LimitMax(readBytes, file.BytesLeft());
			memcpy(buffer, file.GetRawData(), readBytes);
			file.Skip(readBytes);
			*bytes = readBytes;
			if(*bytes == 0)
				return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
			else
				return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
		} else
		{
			return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
		}
	}

	static FLAC__StreamDecoderSeekStatus seek_cb(const FLAC__StreamDecoder *, FLAC__uint64 absolute_byte_offset, void *client_data)
	{
		FileReader &file = static_cast<FLACDecoder *>(client_data)->file;
		if(!file.Seek(static_cast<FileReader::off_t>(absolute_byte_offset)))
			return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
		else
			return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
	}

	static FLAC__StreamDecoderTellStatus tell_cb(const FLAC__StreamDecoder *, FLAC__uint64 *absolute_byte_offset, void *client_data)
	{
		FileReader &file = static_cast<FLACDecoder *>(client_data)->file;
		*absolute_byte_offset = file.GetPosition();
		return FLAC__STREAM_DECODER_TELL_STATUS_OK;
	}

	static FLAC__StreamDecoderLengthStatus length_cb(const FLAC__StreamDecoder *, FLAC__uint64 *stream_length, void *client_data)
	{
		FileReader &file = static_cast<FLACDecoder *>(client_data)->file;
		*stream_length = file.GetLength();
		return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
	}

	static FLAC__bool eof_cb(const FLAC__StreamDecoder *, void *client_data)
	{
		FileReader &file = static_cast<FLACDecoder *>(client_data)->file;
		return file.BytesLeft() == 0;
	}

	static FLAC__StreamDecoderWriteStatus write_cb(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *const buffer[], void *client_data)
	{
		FLACDecoder &client = *static_cast<FLACDecoder *>(client_data);
		ModSample &sample = client.sndFile.GetSample(client.sample);

		if(frame->header.number.sample_number >= sample.nLength || !client.ready)
		{
			// We're reading beyond the sample size already, or we aren't even ready to decode yet!
			return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
		}

		// Number of samples to be copied in this call
		const SmpLength copySamples = Util::Min(static_cast<SmpLength>(frame->header.blocksize), static_cast<SmpLength>(sample.nLength - frame->header.number.sample_number));
		// Number of target channels
		const uint8 modChannels = sample.GetNumChannels();
		// Offset (in samples) into target data
		const size_t offset = static_cast<size_t>(frame->header.number.sample_number) * modChannels;
		// Source size in bytes
		const size_t srcSize = frame->header.blocksize * 4;
		// Source bit depth
		const unsigned int bps = frame->header.bits_per_sample;

		int8 *sampleData8 = reinterpret_cast<int8 *>(sample.pSample) + offset;
		int16 *sampleData16 = reinterpret_cast<int16 *>(sample.pSample) + offset;

		ASSERT((bps <= 8 && sample.GetElementarySampleSize() == 1) || (bps > 8 && sample.GetElementarySampleSize() == 2));
		ASSERT(modChannels <= FLAC__stream_decoder_get_channels(decoder));
		ASSERT(bps == FLAC__stream_decoder_get_bits_per_sample(decoder));

		// Do the sample conversion
		for(uint8 chn = 0; chn < modChannels; chn++)
		{
			if(bps <= 8)
			{
				CopySample<ReadInt8PCM<0> >(sampleData8 + chn, copySamples, modChannels, buffer[chn], srcSize, 4);
			} else if(bps <= 16)
			{
				CopySample<ReadBigIntToInt16PCMNative<0> >(sampleData16 + chn, copySamples, modChannels, buffer[chn], srcSize, 1);
			} else if(bps <= 24)
			{
				CopySample<ReadBigIntToInt16PCMNative<8> >(sampleData16 + chn, copySamples, modChannels, buffer[chn], srcSize, 1);
			} else if(bps <= 32)
			{
				CopySample<ReadBigIntToInt16PCMNative<16> >(sampleData16 + chn, copySamples, modChannels, buffer[chn], srcSize, 1);
			}
		}

		return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
	}

	static void metadata_cb(const FLAC__StreamDecoder *, const FLAC__StreamMetadata *metadata, void *client_data)
	{
		FLACDecoder &client = *static_cast<FLACDecoder *>(client_data);
		ModSample &sample = client.sndFile.GetSample(client.sample);

		if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO && metadata->data.stream_info.total_samples != 0)
		{
			// Init sample information
			CriticalSection cs;
			client.sndFile.DestroySample(client.sample);
			strcpy(client.sndFile.m_szNames[client.sample], "");
			sample.Initialize();
			sample.uFlags.set(CHN_16BIT, metadata->data.stream_info.bits_per_sample > 8);
			sample.uFlags.set(CHN_STEREO, metadata->data.stream_info.channels > 1);
			sample.nLength = static_cast<SmpLength>(metadata->data.stream_info.total_samples);
			sample.nC5Speed = metadata->data.stream_info.sample_rate;
			client.ready = (sample.AllocateSample() != 0);
		} else if(metadata->type == FLAC__METADATA_TYPE_APPLICATION && !memcmp(metadata->data.application.id, "riff", 4) && client.ready)
		{
			// Try reading RIFF loop points and other sample information
			ChunkReader data(reinterpret_cast<const char*>(metadata->data.application.data), metadata->length);
			ChunkReader::ChunkList<RIFFChunk> chunks = data.ReadChunks<RIFFChunk>(2);

			// We're not really going to read a WAV file here because there will be only one RIFF chunk per metadata event, but we can still re-use the code for parsing RIFF metadata...
			WAVReader riffReader(data);
			riffReader.FindMetadataChunks(chunks);
			riffReader.ApplySampleSettings(sample, client.sndFile.m_szNames[client.sample]);
		} else if(metadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT && client.ready)
		{
			// Try reading Vorbis Comments for sample title
			for(FLAC__uint32 i = 0; i < metadata->data.vorbis_comment.num_comments; i++)
			{
				const char *tag = reinterpret_cast<const char *>(metadata->data.vorbis_comment.comments[i].entry);
				const FLAC__uint32 length = metadata->data.vorbis_comment.comments[i].length;
				if(length > 6 && !_strnicmp(tag, "TITLE=", 6))
				{
					StringFixer::ReadString<StringFixer::maybeNullTerminated>(client.sndFile.m_szNames[client.sample], tag + 6, length - 6);
					break;
				}
			}
		}
	}

	static void error_cb(const FLAC__StreamDecoder *, FLAC__StreamDecoderErrorStatus, void *)
	{
	}
};

#endif // NO_FLAC


bool CSoundFile::ReadFLACSample(SAMPLEINDEX sample, FileReader &file)
//-------------------------------------------------------------------
{
#ifndef NO_FLAC
	// Check if we're dealing with FLAC in an OGG container.
	// We won't check for the "fLaC" signature but let libFLAC decide whether a file is valid or not, as some FLAC files might have e.g. leading ID3v2 data.
	file.Rewind();
	const bool isOgg = FLAC_API_SUPPORTS_OGG_FLAC && file.ReadMagic("OggS") && file.Seek(29) && file.ReadMagic("FLAC");
	file.Rewind();

	FLAC__StreamDecoder *decoder = FLAC__stream_decoder_new();
	if(decoder == nullptr)
	{
		return false;
	}

	// Give me all the metadata!
	FLAC__stream_decoder_set_metadata_respond_all(decoder);

	FLACDecoder client(file, *this, sample);

	// Init decoder
	FLAC__StreamDecoderInitStatus initStatus;
	if(isOgg)
		initStatus = FLAC__stream_decoder_init_ogg_stream(decoder, FLACDecoder::read_cb, FLACDecoder::seek_cb, FLACDecoder::tell_cb, FLACDecoder::length_cb, FLACDecoder::eof_cb, FLACDecoder::write_cb, FLACDecoder::metadata_cb, FLACDecoder::error_cb, &client);
	else
		initStatus = FLAC__stream_decoder_init_stream(decoder, FLACDecoder::read_cb, FLACDecoder::seek_cb, FLACDecoder::tell_cb, FLACDecoder::length_cb, FLACDecoder::eof_cb, FLACDecoder::write_cb, FLACDecoder::metadata_cb, FLACDecoder::error_cb, &client);
	if(initStatus != FLAC__STREAM_DECODER_INIT_STATUS_OK)
	{
		return false;
	}

	// Decode file
	FLAC__stream_decoder_process_until_end_of_stream(decoder);
	FLAC__stream_decoder_finish(decoder);
	FLAC__stream_decoder_delete(decoder);

	if(client.ready && Samples[sample].pSample != nullptr)
	{
		Samples[sample].Convert(MOD_TYPE_IT, GetType());
		return true;
	}
#endif // NO_FLAC
	return false;
}


// Helper function for copying OpenMPT's sample data to FLAC's int32 buffer.
template<typename T>
inline void SampleToFLAC32(FLAC__int32 *dst, const void *src, SmpLength numSamples)
{
	const T *in = reinterpret_cast<const T *>(src);
	for(SmpLength i = 0; i < numSamples; i++)
	{
		dst[i] = in[i];
	}
};


bool CSoundFile::SaveFLACSample(SAMPLEINDEX nSample, const LPCSTR lpszFileName) const
//-----------------------------------------------------------------------------------
{
#ifndef NO_FLAC
	FLAC__StreamEncoder *encoder = FLAC__stream_encoder_new();
	if(encoder == nullptr)
	{
		return false;
	}

	const ModSample &sample = Samples[nSample];

	// First off, set up all the metadata...
	FLAC__StreamMetadata *metadata[] =
	{
		FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT),
		FLAC__metadata_object_new(FLAC__METADATA_TYPE_APPLICATION),
		FLAC__metadata_object_new(FLAC__METADATA_TYPE_APPLICATION),
	};

	const bool writeLoopData = sample.uFlags[CHN_LOOP | CHN_SUSTAINLOOP];
	if(metadata[0])
	{
		// Store sample name
		FLAC__StreamMetadata_VorbisComment_Entry entry;
		FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "TITLE", m_szNames[nSample]);
		FLAC__metadata_object_vorbiscomment_append_comment(metadata[0], entry, false);
		FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "ENCODER", "OpenMPT " MPT_VERSION_STR);
		FLAC__metadata_object_vorbiscomment_append_comment(metadata[0], entry, false);
	}
	if(metadata[1])
	{
		// Write MPT sample information
		memcpy(metadata[1]->data.application.id, "riff", 4);

		struct
		{
			RIFFChunk header;
			WAVExtraChunk mptInfo;
		} chunk;

		chunk.header.id = RIFFChunk::idxtra;
		chunk.header.length = sizeof(WAVExtraChunk);

		chunk.mptInfo.ConvertToWAV(sample, GetType());

		const uint32 length = sizeof(RIFFChunk) + sizeof(WAVExtraChunk);
		chunk.header.ConvertEndianness();
		chunk.mptInfo.ConvertEndianness();

		FLAC__metadata_object_application_set_data(metadata[1], reinterpret_cast<FLAC__byte *>(&chunk), length, true);
	}
	if(metadata[2] && writeLoopData)
	{
		// Store loop points
		memcpy(metadata[2]->data.application.id, "riff", 4);

		struct
		{
			RIFFChunk header;
			WAVSampleInfoChunk info;
			WAVSampleLoop loops[2];
		} chunk;

		chunk.header.id = RIFFChunk::idsmpl;
		chunk.header.length = sizeof(WAVSampleInfoChunk);

		chunk.info.ConvertToWAV(sample.GetSampleRate(GetType()));

		if(sample.uFlags[CHN_SUSTAINLOOP])
		{
			chunk.loops[chunk.info.numLoops++].ConvertToWAV(sample.nSustainStart, sample.nSustainEnd, sample.uFlags[CHN_PINGPONGSUSTAIN]);
			chunk.header.length += sizeof(WAVSampleLoop);
		}
		if(sample.uFlags[CHN_LOOP])
		{
			chunk.loops[chunk.info.numLoops++].ConvertToWAV(sample.nLoopStart, sample.nLoopEnd, sample.uFlags[CHN_PINGPONGLOOP]);
			chunk.header.length += sizeof(WAVSampleLoop);
		}

		const uint32 length = sizeof(RIFFChunk) + chunk.header.length;
		chunk.header.ConvertEndianness();
		chunk.info.ConvertEndianness();
		chunk.loops[0].ConvertEndianness();
		chunk.loops[1].ConvertEndianness();
		
		FLAC__metadata_object_application_set_data(metadata[2], reinterpret_cast<FLAC__byte *>(&chunk), length, true);
	}

	FLAC__stream_encoder_set_channels(encoder, sample.GetNumChannels());
	FLAC__stream_encoder_set_bits_per_sample(encoder, sample.GetElementarySampleSize() * 8);
	FLAC__stream_encoder_set_sample_rate(encoder, sample.GetSampleRate(GetType()));
	FLAC__stream_encoder_set_total_samples_estimate(encoder, sample.nLength);
	FLAC__stream_encoder_set_metadata(encoder, metadata, writeLoopData ? 3 : 2);
#ifdef MODPLUG_TRACKER
	// Custom compression level (0...8)
	static const int compression = CMainFrame::GetPrivateProfileLong("Sample Editor", "FLACCompressionLevel", 5, theApp.GetConfigFileName());
	FLAC__stream_encoder_set_compression_level(encoder, compression);
#endif // MODPLUG_TRACKER

	if(FLAC__stream_encoder_init_file(encoder, lpszFileName, nullptr, nullptr) != FLAC__STREAM_ENCODER_INIT_STATUS_OK)
	{
		return false;
	}

	// Convert sample data to signed 32-Bit integer array.
	const SmpLength numSamples = sample.nLength * sample.GetNumChannels();
	FLAC__int32 *sampleData;
	try
	{
		sampleData = new FLAC__int32[numSamples];
	} catch(MPTMemoryException)
	{
		return false;
	}

	if(sample.GetElementarySampleSize() == 1)
	{
		SampleToFLAC32<int8>(sampleData, sample.pSample, numSamples);
	} else if(sample.GetElementarySampleSize() == 2)
	{
		SampleToFLAC32<int16>(sampleData, sample.pSample, numSamples);
	} else
	{
		ASSERT(false);
	}

	// Do the actual conversion.
	FLAC__stream_encoder_process_interleaved(encoder, sampleData, sample.nLength);
	FLAC__stream_encoder_finish(encoder);

	delete[] sampleData;
	for(size_t i = 0; i < CountOf(metadata); i++)
	{
		FLAC__metadata_object_delete(metadata[i]);
	}

	FLAC__stream_encoder_delete(encoder);
	return true;
#else
	return false;
#endif // NO_FLAC
}