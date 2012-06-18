/*
 * Load_ams.cpp
 * ------------
 * Purpose: AMS (Extreme's Tracker) module loader
 * Notes  : Extreme was renamed to Velvet Development at some point,
 *          and thus they also renamed their tracker from
 *          "Extreme's Tracker" to "Velvet Studio".
 *          While the two programs look rather similiar, the structure of both
 *          programs' "AMS" format is significantly different - Velvet Studio is a
 *          rather advanced tracker in comparison to Extreme's Tracker.
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"

#pragma warning(disable:4244) //"conversion from 'type1' to 'type2', possible loss of data"

#pragma pack(push, 1)

typedef struct AMSFILEHEADER
{
	char szHeader[7];	// "Extreme"
	BYTE verlo, verhi;	// 0x??,0x01
	BYTE chncfg;
	BYTE samples;
	WORD patterns;
	WORD orders;
	BYTE vmidi;
	WORD extra;
} AMSFILEHEADER;

typedef struct AMSSAMPLEHEADER
{
	DWORD length;
	DWORD loopstart;
	DWORD loopend;
	BYTE finetune_and_pan;
	WORD samplerate;	// C-2 = 8363
	BYTE volume;		// 0-127
	BYTE infobyte;
} AMSSAMPLEHEADER;


#pragma pack(pop)


// Callback function for reading text
void ConvertAMSTextChars(char &c)
//-------------------------------
{
	switch((unsigned char)c)
	{
	case 0x00:
	case 0x81: c = ' '; break;
	case 0x14: c = 'ö'; break;
	case 0x19: c = 'Ö'; break;
	case 0x04: c = 'ä'; break;
	case 0x0E: c = 'Ä'; break;
	case 0x06: c = 'å'; break;
	case 0x0F: c = 'Å'; break;
	default:
		if((unsigned char)c > 0x81)
			c = '\r';
		break;
	}
}


bool CSoundFile::ReadAMS(const LPCBYTE lpStream, const DWORD dwMemLength)
//-----------------------------------------------------------------------
{
	BYTE pkinf[MAX_SAMPLES];
	AMSFILEHEADER *pfh = (AMSFILEHEADER *)lpStream;
	DWORD dwMemPos;
	UINT tmp, tmp2;

	if ((!lpStream) || (dwMemLength < 126)) return false;
	if ((pfh->verhi != 0x01) || (strncmp(pfh->szHeader, "Extreme", 7))
	 || (!pfh->patterns) || (!pfh->orders) || (!pfh->samples) || (pfh->samples >= MAX_SAMPLES)
	 || (pfh->patterns > MAX_PATTERNS) || (pfh->orders > MAX_ORDERS))
	{
		return false;
	}
	dwMemPos = sizeof(AMSFILEHEADER) + pfh->extra;
	if (dwMemPos + pfh->samples * sizeof(AMSSAMPLEHEADER) >= dwMemLength) return false;
	m_nType = MOD_TYPE_AMS;
	m_nInstruments = 0;
	m_nChannels = (pfh->chncfg & 0x1F) + 1;
	m_nSamples = pfh->samples;
	for (UINT nSmp=1; nSmp <= m_nSamples; nSmp++, dwMemPos += sizeof(AMSSAMPLEHEADER))
	{
		AMSSAMPLEHEADER *psh = (AMSSAMPLEHEADER *)(lpStream + dwMemPos);
		ModSample *pSmp = &Samples[nSmp];
		pSmp->nLength = psh->length;
		pSmp->nLoopStart = psh->loopstart;
		pSmp->nLoopEnd = psh->loopend;
		pSmp->nGlobalVol = 64;
		pSmp->nVolume = psh->volume << 1;
		pSmp->nC5Speed = psh->samplerate;
		pSmp->nPan = (psh->finetune_and_pan & 0xF0);
		if (pSmp->nPan < 0x80) pSmp->nPan += 0x10;
		pSmp->nFineTune = MOD2XMFineTune(psh->finetune_and_pan & 0x0F);
		pSmp->uFlags = (psh->infobyte & 0x80) ? CHN_16BIT : 0;
		if ((pSmp->nLoopEnd <= pSmp->nLength) && (pSmp->nLoopStart+4 <= pSmp->nLoopEnd)) pSmp->uFlags |= CHN_LOOP;
		pkinf[nSmp] = psh->infobyte;
	}

	// Read Song Name
	if (dwMemPos + 1 >= dwMemLength) return true;
	tmp = lpStream[dwMemPos++];
	if (dwMemPos + tmp >= dwMemLength) return true;
	if(tmp)
	{
		StringFixer::ReadString<StringFixer::maybeNullTerminated>(m_szNames[0], reinterpret_cast<const char *>(lpStream + dwMemPos), tmp);
		dwMemPos += tmp;
	}


	// Read sample names
	for (UINT sNam=1; sNam<=m_nSamples; sNam++)
	{
		if (dwMemPos + 1 >= dwMemLength) return true;
		tmp = lpStream[dwMemPos++];
		if (dwMemPos + tmp >= dwMemLength) return true;
		if(tmp)
		{
			StringFixer::ReadString<StringFixer::maybeNullTerminated>(m_szNames[sNam], reinterpret_cast<const char *>(lpStream + dwMemPos), tmp);
			dwMemPos += tmp;
		}
	}

	// Read Channel names
	for (UINT cNam=0; cNam<m_nChannels; cNam++)
	{
		if (dwMemPos + 1 >= dwMemLength) return true;
		uint8 chnnamlen = lpStream[dwMemPos++];
		if (dwMemPos + tmp >= dwMemLength) return true;
		if(chnnamlen)
		{
			StringFixer::ReadString<StringFixer::maybeNullTerminated>(ChnSettings[cNam].szName, reinterpret_cast<const char *>(lpStream + dwMemPos), chnnamlen);
			dwMemPos += chnnamlen;
		}
	}

	// Read Pattern Names
	for (UINT pNam = 0; pNam < pfh->patterns; pNam++)
	{
		if (dwMemPos + 1 >= dwMemLength) return true;
		tmp = lpStream[dwMemPos++];
		tmp2 = min(tmp, MAX_PATTERNNAME - 1);		// not counting null char
		if (dwMemPos + tmp >= dwMemLength) return true;
		Patterns.Insert(pNam, 64);	// Create pattern now, so that the name won't be overwritten later.
		if(tmp2)
		{
			Patterns[pNam].SetName((char *)(lpStream + dwMemPos), tmp2 + 1);
		}
		dwMemPos += tmp;
	}

	// Read Song Comments
	tmp = *((WORD *)(lpStream+dwMemPos));
	dwMemPos += 2;
	if (dwMemPos + tmp >= dwMemLength) return true;
	if (tmp)
	{
		ReadMessage(lpStream + dwMemPos, tmp, leCR, &ConvertAMSTextChars);
	}
	dwMemPos += tmp;

	// Read Order List
	Order.resize(pfh->orders, Order.GetInvalidPatIndex());
	for (UINT iOrd=0; iOrd < pfh->orders; iOrd++, dwMemPos += 2)
	{
		Order[iOrd] = (PATTERNINDEX)*((WORD *)(lpStream + dwMemPos));
	}

	// Read Patterns
	for (UINT iPat=0; iPat<pfh->patterns; iPat++)
	{
		if (dwMemPos + 4 >= dwMemLength) return true;
		UINT len = *((DWORD *)(lpStream + dwMemPos));
		dwMemPos += 4;
		if ((len >= dwMemLength) || (dwMemPos + len > dwMemLength)) return true;
		// Pattern has been inserted when reading pattern names
		ModCommand* m = Patterns[iPat];
		if (!m) return true;
		const BYTE *p = lpStream + dwMemPos;
		UINT row = 0, i = 0;
		while ((row < Patterns[iPat].GetNumRows()) && (i+2 < len))
		{
			BYTE b0 = p[i++];
			BYTE b1 = p[i++];
			BYTE b2 = 0;
			UINT ch = b0 & 0x3F;
			// Note+Instr
			if (!(b0 & 0x40))
			{
				b2 = p[i++];
				if (ch < m_nChannels)
				{
					if (b1 & 0x7F) m[ch].note = (b1 & 0x7F) + 25;
					m[ch].instr = b2;
				}
				if (b1 & 0x80)
				{
					b0 |= 0x40;
					b1 = p[i++];
				}
			}
			// Effect
			if (b0 & 0x40)
			{
			anothercommand:
				if (b1 & 0x40)
				{
					if (ch < m_nChannels)
					{
						m[ch].volcmd = VOLCMD_VOLUME;
						m[ch].vol = b1 & 0x3F;
					}
				} else
				{
					b2 = p[i++];
					if (ch < m_nChannels)
					{
						UINT cmd = b1 & 0x3F;
						if (cmd == 0x0C)
						{
							m[ch].volcmd = VOLCMD_VOLUME;
							m[ch].vol = b2 >> 1;
						} else
						if (cmd == 0x0E)
						{
							if (!m[ch].command)
							{
								UINT command = CMD_S3MCMDEX;
								UINT param = b2;
								switch(param & 0xF0)
								{
								case 0x00:	if (param & 0x08) { param &= 0x07; param |= 0x90; } else {command=param=0;} break;
								case 0x10:	command = CMD_PORTAMENTOUP; param |= 0xF0; break;
								case 0x20:	command = CMD_PORTAMENTODOWN; param |= 0xF0; break;
								case 0x30:	param = (param & 0x0F) | 0x10; break;
								case 0x40:	param = (param & 0x0F) | 0x30; break;
								case 0x50:	param = (param & 0x0F) | 0x20; break;
								case 0x60:	param = (param & 0x0F) | 0xB0; break;
								case 0x70:	param = (param & 0x0F) | 0x40; break;
								case 0x90:	command = CMD_RETRIG; param &= 0x0F; break;
								case 0xA0:	if (param & 0x0F) { command = CMD_VOLUMESLIDE; param = (param << 4) | 0x0F; } else command=param=0; break;
								case 0xB0:	if (param & 0x0F) { command = CMD_VOLUMESLIDE; param |= 0xF0; } else command=param=0; break;
								}
								m[ch].command = command;
								m[ch].param = param;
							}
						} else
						{
							m[ch].command = cmd;
							m[ch].param = b2;
							ConvertModCommand(m[ch]);
						}
					}
				}
				if (b1 & 0x80)
				{
					b1 = p[i++];
					if (i <= len) goto anothercommand;
				}
			}
			if (b0 & 0x80)
			{
				row++;
				m += m_nChannels;
			}
		}
		dwMemPos += len;
	}

	// Read Samples
	for (UINT iSmp=1; iSmp<=m_nSamples; iSmp++) if (Samples[iSmp].nLength)
	{
		if (dwMemPos >= dwMemLength - 9) return true;
		dwMemPos += SampleIO(
			(Samples[iSmp].uFlags & CHN_16BIT) ? SampleIO::_16bit : SampleIO::_8bit,
			SampleIO::mono,
			SampleIO::littleEndian,
			SampleIO::AMS)
			.ReadSample(Samples[iSmp], (LPSTR)(lpStream+dwMemPos), dwMemLength-dwMemPos);
	}
	return true;
}


/////////////////////////////////////////////////////////////////////
// AMS (Velvet Studio) 2.1 / 2.2 loader

#pragma pack(1)

// AMS2 File Header
struct AMS2FileHeader
{
	enum FileFlags
	{
		linearSlides	= 0x40,
	};

	uint16 format;			// Version of format (Hi = MainVer, Low = SubVer e.g. 0202 = 2.2)
	uint8  numIns;			// Nr of Instruments (0-255)
	uint16 numPats;			// Nr of Patterns (1-1024)
	uint16 numOrds;			// Nr of Positions (1-65535)
	// Rest of header differs between format revision 2.01 and 2.02

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(format);
		SwapBytesLE(numPats);
		SwapBytesLE(numOrds);
	};
};


// AMS2 Instument Envelope 
struct AMS2Envelope
{
	uint8 speed;		// Envelope speed
	uint8 sustainPoint;	// Envelope sustain point
	uint8 loopStart;	// Envelope loop Start
	uint8 loopEnd;		// Envelope loop End
	uint8 numPoints;	// Envelope length

	// Read envelope and do partial conversion.
	void ConvertToMPT(InstrumentEnvelope &mptEnv, FileReader &file)
	{
		file.Read(*this);

		// Read envelope points
		struct
		{
			uint8 data[64][3];
		} env;
		file.ReadStructPartial(env, numPoints * 3);

		if(numPoints <= 1)
		{
			// This is not an envelope.
			return;
		}

		STATIC_ASSERT(MAX_ENVPOINTS >= CountOf(env.data));
		mptEnv.nNodes = Util::Min(numPoints, uint8(CountOf(env.data)));
		mptEnv.nLoopStart = loopStart;
		mptEnv.nLoopEnd = loopEnd;
		mptEnv.nSustainStart = mptEnv.nSustainEnd = sustainPoint;

		for(size_t i = 0; i < mptEnv.nNodes; i++)
		{
			if(i != 0)
			{
				mptEnv.Ticks[i] = mptEnv.Ticks[i - 1] + Util::Max(1, env.data[i][0] | ((env.data[i][1] & 0x01) << 8));
			}
			mptEnv.Values[i] = env.data[i][2];
		}
	}
};


// AMS2 Instrument Data
struct AMS2Instrument
{
	enum EnvelopeFlags
	{
		envLoop		= 0x01,
		envSustain	= 0x02,
		envEnabled	= 0x04,
		
		// Flag shift amounts
		volEnvShift	= 0,
		panEnvShift	= 1,
		vibEnvShift	= 2,

		vibAmpMask	= 0x3000,
		vibAmpShift	= 12,
		fadeOutMask	= 0xFFF,
	};

	uint8  shadowInstr;		// Shadow Instrument. If non-zero, the value=the shadowed inst.
	uint16 vibampFadeout;	// Vib.Amplify + Volume fadeout in one variable!
	uint16 envFlags;		// See EnvelopeFlags

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(vibampFadeout);
		SwapBytesLE(envFlags);
	}

	void ApplyFlags(InstrumentEnvelope &mptEnv, EnvelopeFlags shift) const
	{
		const uint8 flags = envFlags >> (shift * 3);
		if(flags & envEnabled) mptEnv.dwFlags |= ENV_ENABLED;
		if(flags & envLoop) mptEnv.dwFlags |= ENV_LOOP;
		if(flags & envSustain) mptEnv.dwFlags |= ENV_SUSTAIN;

		// "Break envelope" should stop the envelope loop when encountering a note-off... We can only use the sustain loop to emulate this behaviour.
		if(!(flags & envSustain) && (flags & envLoop) != 0 && (flags & (1 << (9 - shift * 2))) != 0)
		{
			mptEnv.nSustainStart = mptEnv.nLoopStart;
			mptEnv.nSustainEnd = mptEnv.nLoopEnd;
			mptEnv.dwFlags |= ENV_SUSTAIN;
			mptEnv.dwFlags &= ~ENV_LOOP;
		}
	}

};


// AMS2 Sample Header
struct AMS2SampleHeader
{
	enum SampleFlags
	{
		smpPacked	= 0x03,
		smp16Bit	= 0x04,
		smpLoop		= 0x08,
		smpBidiLoop	= 0x10,
	};

	uint32 length;
	uint32 loopStart;
	uint32 loopEnd;
	uint16 sampledRate;		// Whyyyy?
	uint8  panFinetune;		// High nibble = pan position, low nibble = finetune value
	uint16 c4speed;			// Why is all of this so redundant?
	int8   relativeTone;	// q.e.d.
	uint8  volume;			// 0...127
	uint8  flags;			// See SampleFlags

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(length);
		SwapBytesLE(loopStart);
		SwapBytesLE(loopEnd);
		SwapBytesLE(sampledRate);
		SwapBytesLE(c4speed);
	}

	// Convert sample header to OpenMPT's internal format.
	void ConvertToMPT(ModSample &mptSmp) const
	{
		mptSmp.Initialize();

		mptSmp.nLength = length;
		mptSmp.nLoopStart = Util::Min(loopStart, length);
		mptSmp.nLoopEnd = Util::Min(loopEnd, length);

		mptSmp.nC5Speed = c4speed * 2;
		if(!mptSmp.nC5Speed)
		{
			mptSmp.nC5Speed = 8363 * 2;
		}
		// Why, oh why, does this format need a c5speed and transpose/finetune at the same time...
		uint32 newC4speed = ModSample::TransposeToFrequency(relativeTone, MOD2XMFineTune(panFinetune & 0x0F));
		mptSmp.nC5Speed = (mptSmp.nC5Speed * newC4speed) / 8363;

		mptSmp.nVolume = (Util::Min(uint8(127), volume) * 256 + 64) / 127;
		if(panFinetune & 0xF0)
		{
			mptSmp.nPan = (panFinetune & 0xF0);
			mptSmp.uFlags = CHN_PANNING;
		}

		if(flags & smp16Bit)
		{
			mptSmp.uFlags |= CHN_16BIT;
		}
		if((flags & smpLoop) && mptSmp.nLoopStart < mptSmp.nLoopEnd)
		{
			mptSmp.uFlags |= CHN_LOOP;
			if(flags & smpBidiLoop)
			{
				mptSmp.uFlags |= CHN_PINGPONGLOOP;
			}
		}
	}
};


// AMS2 Song Description Header
struct AMS2Description
{
	uint32 packedLen;		// Including header
	uint32 unpackedLen;
	uint8  packRoutine;		// 01
	uint8  preProcessing;	// None!
	uint8  packingMethod;	// RLE

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(packedLen);
		SwapBytesLE(unpackedLen);
	}
};


#pragma pack()


// Read variable-length AMS string (we ignore the maximum text length specified by the AMS specs and accept any length).
template<size_t destSize>
bool ReadAMSString(char (&destBuffer)[destSize], FileReader &file)
//----------------------------------------------------------------
{
	const size_t length = file.ReadUint8();
	return file.ReadString<StringFixer::spacePadded>(destBuffer, length);
}


bool CSoundFile::ReadAMS2(FileReader &file)
//-----------------------------------------
{
	file.Rewind();

	AMS2FileHeader fileHeader;
	if(!file.ReadMagic("AMShdr\x1A")
		|| !ReadAMSString(m_szNames[0], file)
		|| !file.ReadConvertEndianness(fileHeader))
	{
		return false;
	}
	
	uint16 headerFlags;
	if(fileHeader.format == 0x202)
	{
		m_nDefaultTempo = Util::Max(uint8(32), static_cast<uint8>(file.ReadUint16LE() >> 8));	// 16.16 Tempo
		m_nDefaultSpeed = Util::Max(uint8(1), file.ReadUint8());
		file.Skip(3);	// Default values for pattern editor
		headerFlags = file.ReadUint16LE();
	} else if(fileHeader.format == 0x201)
	{
		m_nDefaultTempo = Util::Max(uint8(32), file.ReadUint8());
		m_nDefaultSpeed = Util::Max(uint8(1), file.ReadUint8());
		headerFlags = file.ReadUint8();
	} else
	{
		return false;
	}

	m_nType = MOD_TYPE_AMS2;
	m_dwSongFlags = SONG_ITCOMPATGXX | SONG_ITOLDEFFECTS | ((headerFlags & AMS2FileHeader::linearSlides) ? SONG_LINEARSLIDES : 0);
	m_nDefaultGlobalVolume = MAX_GLOBAL_VOLUME;
	m_nSamplePreAmp = m_nVSTiVolume = 48;
	m_nInstruments = fileHeader.numIns;
	m_nChannels = 32;

	// Instruments
	vector<SAMPLEINDEX> firstSample;	// First sample of instrument
	vector<uint16> sampleSettings;		// Shadow sample map... Lo byte = Instrument, Hi byte, lo nibble = Sample index in instrument, Hi byte, hi nibble = Sample pack status
	enum
	{
		instrIndexMask		= 0xFF,		// Shadow instrument
		sampleIndexMask		= 0x7F00,	// Sample index in instrument
		sampleIndexShift	= 8,
		packStatusMask		= 0x8000,	// If bit is set, sample is packed
	};

	for(INSTRUMENTINDEX ins = 1; ins <= m_nInstruments; ins++)
	{
		ModInstrument *instrument = AllocateInstrument(ins);
		if(instrument == nullptr
			|| !ReadAMSString(instrument->name, file))
		{
			return false;
		}

		uint8 numSamples = file.ReadUint8();
		uint8 sampleAssignment[120];

		if(numSamples == 0
			|| !file.ReadArray(sampleAssignment))
		{
			continue;
		}

		STATIC_ASSERT(CountOf(instrument->Keyboard) >= CountOf(sampleAssignment));
		for(size_t i = 0; i < 120; i++)
		{
			instrument->Keyboard[i] = sampleAssignment[i] + GetNumSamples() + 1;
		}

		AMS2Envelope volEnv, panEnv, vibratoEnv;
		volEnv.ConvertToMPT(instrument->VolEnv, file);
		panEnv.ConvertToMPT(instrument->PanEnv, file);
		vibratoEnv.ConvertToMPT(instrument->PitchEnv, file);

		AMS2Instrument instrHeader;
		file.ReadConvertEndianness(instrHeader);
		instrument->nFadeOut = (instrHeader.vibampFadeout & AMS2Instrument::fadeOutMask) * 2;
		const uint8 vibAmp = 1 + ((instrHeader.vibampFadeout & AMS2Instrument::vibAmpMask) >> AMS2Instrument::vibAmpShift);		// "Close enough"

		instrHeader.ApplyFlags(instrument->VolEnv, AMS2Instrument::volEnvShift);
		instrHeader.ApplyFlags(instrument->PanEnv, AMS2Instrument::panEnvShift);
		instrHeader.ApplyFlags(instrument->PitchEnv, AMS2Instrument::vibEnvShift);

		// Scale envelopes to correct range
		for(size_t i = 0; i < MAX_ENVPOINTS; i++)
		{
			instrument->VolEnv.Values[i] = Util::Min(uint8(ENVELOPE_MAX), static_cast<uint8>((instrument->VolEnv.Values[i] * ENVELOPE_MAX + 64u) / 127u));
			instrument->PanEnv.Values[i] = Util::Min(uint8(ENVELOPE_MAX), static_cast<uint8>((instrument->PanEnv.Values[i] * ENVELOPE_MAX + 128u) / 255u));
			instrument->PitchEnv.Values[i] = Util::Min(uint8(ENVELOPE_MAX), static_cast<uint8>(32 + (static_cast<int8>(instrument->PitchEnv.Values[i] - 128) * vibAmp) / 255));
		}

		// Sample headers - we will have to read them even for shadow samples, and we will have to load them several times,
		// as it is possible that shadow samples use different sample settings like base frequency or panning.
		const SAMPLEINDEX firstSmp = GetNumSamples() + 1;
		for(SAMPLEINDEX smp = 0; smp < numSamples; smp++)
		{
			if(firstSmp + smp >= MAX_SAMPLES)
			{
				file.Skip(sizeof(AMS2SampleHeader));
				break;
			}
			ReadAMSString(m_szNames[firstSmp + smp], file);

			AMS2SampleHeader sampleHeader;
			file.ReadConvertEndianness(sampleHeader);
			sampleHeader.ConvertToMPT(Samples[firstSmp + smp]);

			uint16 settings = (instrHeader.shadowInstr & instrIndexMask) | ((smp << sampleIndexShift) & sampleIndexMask) | ((sampleHeader.flags & AMS2SampleHeader::smpPacked) ? packStatusMask : 0);
			sampleSettings.push_back(settings);
		}

		firstSample.push_back(firstSmp);
		m_nSamples = Util::Min(MAX_SAMPLES - 1, GetNumSamples() + numSamples);
	}

	// Text

	// Read composer name
	uint8 composerLength = file.ReadUint8();
	if(composerLength)
	{
		ReadMessage(file, composerLength, leAutodetect, ConvertAMSTextChars);
	}

	// Channel names
	for(CHANNELINDEX chn = 0; chn < 32; chn++)
	{
		ReadAMSString(ChnSettings[chn].szName, file);
	}

	// RLE-Packed description text
	AMS2Description descriptionHeader;
	if(!file.ReadConvertEndianness(descriptionHeader))
	{
		return true;
	}
	if(descriptionHeader.packedLen)
	{
		const size_t textLength = descriptionHeader.packedLen - sizeof(descriptionHeader);
		vector<uint8> textIn, textOut(descriptionHeader.unpackedLen);
		file.ReadVector(textIn, textLength);

		size_t readLen = 0, writeLen = 0;
		while(readLen < textLength && writeLen < descriptionHeader.unpackedLen)
		{
			uint8 c = textIn[readLen++];
			if(c == 0xFF && textLength - readLen >= 2)
			{
				c = textIn[readLen++];
				uint32 count = textIn[readLen++];
				for(size_t i = Util::Min(descriptionHeader.unpackedLen - writeLen, count); i != 0; i--)
				{
					textOut[writeLen++] = c;
				}
			} else
			{
				textOut[writeLen++] = c;
			}
		}
		// Packed text doesn't include any line breaks!
		ReadFixedLineLengthMessage(&textOut[0], descriptionHeader.unpackedLen, 74, 0, ConvertAMSTextChars);
	}

	// Read Order List
	vector<uint16> orders;
	if(file.ReadVector(orders, fileHeader.numOrds))
	{
		Order.resize(fileHeader.numOrds);
		for(size_t i = 0; i < fileHeader.numOrds; i++)
		{
			Order[i] = SwapBytesLE(orders[i]);
		}
	}

	// Read Patterns
	for(PATTERNINDEX pat = 0; pat < fileHeader.numPats; pat++)
	{
		uint32 patLength = file.ReadUint32LE();
		FileReader patternChunk = file.GetChunk(patLength);

		const ROWINDEX numRows = patternChunk.ReadUint8() + 1;
		// We don't need to know the number of channels or commands.
		patternChunk.Skip(1);

		if(Patterns.Insert(pat, numRows))
		{
			continue;
		}

		char patternName[11];
		ReadAMSString(patternName, patternChunk);
		Patterns[pat].SetName(patternName);

		enum
		{
			emptyRow		= 0xFF,	// No commands on row
			endOfRowMask	= 0x80,	// If set, no more commands on this row
			noteMask		= 0x40,	// If set, no note+instr in this command
			channelMask		= 0x1F,	// Mask for extracting channel

			// Note flags
			readNextCmd		= 0x80,	// One more command follows
			noteDataMask	= 0x7F,	// Extract note

			// Command flags
			volCommand		= 0x40,	// Effect is compressed volume command
			commandMask		= 0x3F,	// Command or volume mask
		};

		// Extended (non-Protracker) effects
		static const uint8 effTrans[] =
		{
			CMD_S3MCMDEX,		// Forward / Backward
			CMD_PORTAMENTOUP,	// Extra fine slide up
			CMD_PORTAMENTODOWN,	// Extra fine slide up
			CMD_RETRIG,			// Retrigger
			CMD_NONE,
			CMD_TONEPORTAVOL,	// Toneporta with fine volume slide
			CMD_VIBRATOVOL,		// Vibrato with fine volume slide
			CMD_NONE,
			CMD_PANNINGSLIDE,
			CMD_NONE,
			CMD_VOLUMESLIDE,	// Two times finder volume slide than Axx
			CMD_NONE,
			CMD_CHANNELVOLUME,	// Channel volume (0...127)
			CMD_PATTERNBREAK,	// Long pattern break (in hex)
			CMD_S3MCMDEX,		// Fine slide commands
			CMD_NONE,			// Fractional BPM
			CMD_KEYOFF,			// Key off at tick xx
			CMD_PORTAMENTOUP,	// Porta up, but uses all octaves (?)
			CMD_PORTAMENTODOWN,	// Porta down, but uses all octaves (?)
			CMD_NONE,
			CMD_NONE,
			CMD_NONE,
			CMD_NONE,
			CMD_NONE,
			CMD_NONE,
			CMD_NONE,
			CMD_GLOBALVOLSLIDE,	// Global volume slide
			CMD_NONE,
			CMD_GLOBALVOLUME,	// Global volume (0... 127)
		};

		for(ROWINDEX row = 0; row < numRows; row++)
		{
			PatternRow baseRow = Patterns[pat].GetRow(row);
			while(patternChunk.BytesLeft())
			{
				const uint8 flags = patternChunk.ReadUint8();
				if(flags == emptyRow)
				{
					break;
				}
			
				ModCommand &m = baseRow[flags & channelMask];
				bool moreCommands = true;
				if(!(flags & noteMask))
				{
					uint8 note = patternChunk.ReadUint8(), instr = patternChunk.ReadUint8();
					moreCommands = (note & readNextCmd) != 0;
					note &= noteDataMask;
					
					if(note == 1)
					{
						m.note = NOTE_KEYOFF;
					} else if(note >= 2 && note <= 121)
					{
						m.note = note - 2 + NOTE_MIN;
					}
					m.instr = instr;
				}

				while(moreCommands)
				{
					ModCommand origCmd = m;
					const uint8 command = patternChunk.ReadUint8(), effect = (command & commandMask);
					moreCommands = (command & readNextCmd) != 0;

					if(command & volCommand)
					{
						m.volcmd = VOLCMD_VOLUME;
						m.vol = effect;
					} else
					{
						m.param = patternChunk.ReadUint8();

						if(effect < 0x10)
						{
							m.command = effect;
							ConvertModCommand(m);

							switch(m.command)
							{
							case CMD_PANNING8:
								// 4-Bit panning
								m.command = CMD_PANNING8;
								m.param = (m.param & 0x0F) * 0x11;
								break;

							case CMD_VOLUME:
								m.command = CMD_NONE;
								m.volcmd = VOLCMD_VOLUME;
								m.vol = Util::Min((m.param + 1) / 2, 64);
								break;

							case CMD_MODCMDEX:
								if(m.param == 0x80)
								{
									// Break sample loop (cut after loop)
									m.command = CMD_NONE;
								} else
								{
									m.ExtendedMODtoS3MEffect();
								}
								break;
							}
						} else
						{
							if(effect - 0x10 < CountOf(effTrans))
							{
								m.command = effTrans[effect - 0x10];

								switch(effect)
								{
								case 0x10:
									// Play sample forwards / backwards
									if(m.param <= 0x01)
									{
										m.param |= 0x9E;
									} else
									{
										m.command = CMD_NONE;
									}
									break;

								case 0x11:
								case 0x12:
									// Extra fine slides
									m.param = Util::Min(uint8(0x0F), m.param) | 0xE0;
									break;

								case 0x15:
								case 0x16:
									// Fine slides
									m.param = (Util::Min(0x10, m.param + 1) / 2) | 0xF0;
									break;

								case 0x1E:
									// More fine slides
									switch(m.param >> 4)
									{
									case 0x1:
										// Fine porta up
										m.command = CMD_PORTAMENTOUP;
										m.param |= 0xF0;
										break;
									case 0x2:
										// Fine porta down
										m.command = CMD_PORTAMENTODOWN;
										m.param |= 0xF0;
										break;
									case 0xA:
										// Extra fine volume slide up
										m.command = CMD_VOLUMESLIDE;
										m.param = ((((m.param & 0x0F) + 1) / 2) << 4) | 0x0F;
										break;
									case 0xB:
										// Extra fine volume slide down
										m.command = CMD_VOLUMESLIDE;
										m.param = (((m.param & 0x0F) + 1) / 2) | 0xF0;
										break;
									default:
										m.command = CMD_NONE;
										break;
									}
									break;

								case 0x1C:
									// Adjust channel volume range
									m.param = Util::Min((m.param + 1) / 2, 64);
									break;
								}
							}
						}

						if(origCmd.command == CMD_VOLUMESLIDE && (m.command == CMD_VIBRATO || m.command == CMD_TONEPORTAVOL) && m.param == 0)
						{
							// Merge commands
							if(m.command == CMD_VIBRATO)
							{
								m.command = CMD_VIBRATOVOL;
							} else
							{
								m.command = CMD_TONEPORTAVOL;
							}
							m.param = origCmd.param;
							origCmd.command = CMD_NONE;
						}

						if(ModCommand::GetEffectWeight(origCmd.command) > ModCommand::GetEffectWeight(m.command))
						{
							if(ModCommand::ConvertVolEffect(m.command, m.param, true))
							{
								// Volume column to the rescue!
								m.volcmd = m.command;
								m.vol = m.param;
							}

							m.command = origCmd.command;
							m.param = origCmd.param;
						}
					}
				}

				if(flags & endOfRowMask)
				{
					// End of row
					break;
				}
			}
		}
	}

	// Read Samples
	for(SAMPLEINDEX smp = 0; smp < GetNumSamples(); smp++)
	{
		if((sampleSettings[smp] & instrIndexMask) == 0)
		{
			// Only load samples that aren't part of a shadow instrument
			SampleIO(
				(Samples[smp + 1].uFlags & CHN_16BIT) ? SampleIO::_16bit : SampleIO::_8bit,
				SampleIO::mono,
				SampleIO::littleEndian,
				(sampleSettings[smp] & packStatusMask) ? SampleIO::AMS : SampleIO::signedPCM)
				.ReadSample(Samples[smp + 1], file);
		}
	}

	// Copy shadow samples
	for(SAMPLEINDEX smp = 0; smp < GetNumSamples(); smp++)
	{
		INSTRUMENTINDEX sourceInstr = (sampleSettings[smp] & instrIndexMask);
		if(sourceInstr == 0
			|| --sourceInstr >= firstSample.size())
		{
				continue;
		}

		SAMPLEINDEX sourceSample = ((sampleSettings[smp] & sampleIndexMask) >> sampleIndexShift) + firstSample[sourceInstr];
		if(sourceSample > GetNumSamples())
		{
			continue;
		}

		// Copy over original sample
		ModSample &sample = Samples[smp + 1];
		ModSample &source = Samples[sourceSample];
		if(source.uFlags & CHN_16BIT)
		{
			sample.uFlags |= CHN_16BIT;
		} else
		{
			sample.uFlags &= ~CHN_16BIT;
		}
		sample.nLength = source.nLength;
		if(sample.AllocateSample())
		{
			memcpy(sample.pSample, source.pSample, source.GetSampleSizeInBytes());
			AdjustSampleLoop(sample);
		}
	}

	return true;
}



/////////////////////////////////////////////////////////////////////
// AMS Sample unpacking

void AMSUnpack(const char *psrc, UINT inputlen, char *pdest, UINT dmax, char packcharacter)
//-----------------------------------------------------------------------------------------
{
	UINT tmplen = dmax;
	signed char *amstmp = new signed char[tmplen];

	if (!amstmp) return;
	// Unpack Loop
	{
		signed char *p = amstmp;
		UINT i=0, j=0;
		while ((i < inputlen) && (j < tmplen))
		{
			signed char ch = psrc[i++];
			if (ch == packcharacter)
			{
				BYTE ch2 = psrc[i++];
				if (ch2)
				{
					ch = psrc[i++];
					while (ch2--)
					{
						p[j++] = ch;
						if (j >= tmplen) break;
					}
				} else p[j++] = packcharacter;
			} else p[j++] = ch;
		}
	}
	// Bit Unpack Loop
	{
		signed char *p = amstmp;
		UINT bitcount = 0x80, dh;
		UINT k=0;
		for (UINT i=0; i<dmax; i++)
		{
			BYTE al = *p++;
			dh = 0;
			for (UINT count=0; count<8; count++)
			{
				UINT bl = al & bitcount;
				bl = ((bl|(bl<<8)) >> ((dh+8-count) & 7)) & 0xFF;
				bitcount = ((bitcount|(bitcount<<8)) >> 1) & 0xFF;
				pdest[k++] |= bl;
				if (k >= dmax)
				{
					k = 0;
					dh++;
				}
			}
			bitcount = ((bitcount|(bitcount<<8)) >> dh) & 0xFF;
		}
	}
	// Delta Unpack
	{
		signed char old = 0;
		for (UINT i=0; i<dmax; i++)
		{
			int pos = ((LPBYTE)pdest)[i];
			if ((pos != 128) && (pos & 0x80)) pos = -(pos & 0x7F);
			old -= (signed char)pos;
			pdest[i] = old;
		}
	}
	delete[] amstmp;
}
