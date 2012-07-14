/*
 * Load_xm.cpp
 * -----------
 * Purpose: XM (FastTracker II) module loader / saver
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          Adam Goode (endian and char fixes for PPC)
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"
#include "../mptrack/version.h"
#include "../common/misc_util.h"
#include "XMTools.h"
#include <algorithm>

////////////////////////////////////////////////////////
// FastTracker II XM file support

#pragma warning(disable:4244) //conversion from 'type1' to 'type2', possible loss of data

#define str_tooMuchPatternData	(GetStrI18N((_TEXT("Warning: File format limit was reached. Some pattern data may not get written to file."))))
#define str_pattern				(GetStrI18N((_TEXT("pattern"))))


// Allocate samples for an instrument
vector<SAMPLEINDEX> AllocateXMSamples(CSoundFile &sndFile, SAMPLEINDEX numSamples)
//--------------------------------------------------------------------------------
{
	LimitMax(numSamples, SAMPLEINDEX(32));

	vector<SAMPLEINDEX> foundSlots;

	for(SAMPLEINDEX i = 0; i < numSamples; i++)
	{
		SAMPLEINDEX candidateSlot = sndFile.GetNumSamples() + 1;

		if(candidateSlot >= MAX_SAMPLES)
		{
			// If too many sample slots are needed, try to fill some empty slots first.
			for(SAMPLEINDEX j = 1; j <= sndFile.GetNumSamples(); j++)
			{
				if(sndFile.GetSample(j).pSample != nullptr)
				{
					continue;
				}

				if(std::find(foundSlots.begin(), foundSlots.end(), j) == foundSlots.end())
				{
					// Empty sample slot that is not occupied by the current instrument. Yay!
					candidateSlot = j;

					// Remove unused sample from instrument sample assignments
					for(INSTRUMENTINDEX ins = 1; ins <= sndFile.GetNumInstruments(); ins++)
					{
						if(sndFile.Instruments[ins] == nullptr)
						{
							continue;
						}
						for(size_t k = 0; k < CountOf(sndFile.Instruments[ins]->Keyboard); k++)
						{
							if(sndFile.Instruments[ins]->Keyboard[k] == candidateSlot)
							{
								sndFile.Instruments[ins]->Keyboard[k] = 0;
							}
						}
					}
					break;
				}
			}
		}

		if(candidateSlot >= MAX_SAMPLES)
		{
			// Still couldn't find any empty sample slots, so look out for existing but unused samples.
			vector<bool> usedSamples;
			SAMPLEINDEX unusedSampleCount = sndFile.DetectUnusedSamples(usedSamples);

			if(unusedSampleCount > 0)
			{
				sndFile.RemoveSelectedSamples(usedSamples);
				// Remove unused samples from instrument sample assignments
				for(INSTRUMENTINDEX ins = 1; ins <= sndFile.GetNumInstruments(); ins++)
				{
					if(sndFile.Instruments[ins] == nullptr)
					{
						continue;
					}
					for(size_t k = 0; k < CountOf(sndFile.Instruments[ins]->Keyboard); k++)
					{
						if(sndFile.Instruments[ins]->Keyboard[k] < usedSamples.size() && !usedSamples[sndFile.Instruments[ins]->Keyboard[k]])
						{
							sndFile.Instruments[ins]->Keyboard[k] = 0;
						}
					}
				}

				// New candidate slot is first unused sample slot.
				candidateSlot = std::find(usedSamples.begin() + 1, usedSamples.end(), false) - usedSamples.begin();
			} else
			{
				// No unused sampel slots: Give up :(
				break;
			}
		}

		if(candidateSlot < MAX_SAMPLES)
		{
			foundSlots.push_back(candidateSlot);
			if(candidateSlot > sndFile.GetNumSamples())
			{
				sndFile.m_nSamples = candidateSlot;
			}
		}
	}

	return foundSlots;
}


// Read .XM patterns
DWORD ReadXMPatterns(const BYTE *lpStream, DWORD dwMemLength, DWORD dwMemPos, XMFileHeader *xmheader, CSoundFile &sndFile)
//-------------------------------------------------------------------------------------------------------------------------
{
	vector<BYTE> patterns_used(256, 0);
	vector<BYTE> pattern_map(256, 0);

	if(xmheader->patterns > MAX_PATTERNS)
	{
		UINT i, j;
		for (i = 0; i < xmheader->orders; i++)
		{
			if (sndFile.Order[i] < xmheader->patterns) patterns_used[sndFile.Order[i]] = true;
		}
		j = 0;
		for (i = 0; i < 256; i++)
		{
			if (patterns_used[i]) pattern_map[i] = j++;
		}
		for (i = 0; i < 256; i++)
		{
			if (!patterns_used[i])
			{
				pattern_map[i] = (j < MAX_PATTERNS) ? j : sndFile.Order.GetIgnoreIndex();
				j++;
			}
		}
		for (i = 0; i < xmheader->orders; i++)
		{
			sndFile.Order[i] = pattern_map[sndFile.Order[i]];
		}
	} else
	{
		for (UINT i = 0; i < 256; i++) pattern_map[i] = i;
	}
	if(dwMemPos + 8 >= dwMemLength) return dwMemPos;
	// Reading patterns
	for(UINT ipat = 0; ipat < xmheader->patterns; ipat++)
	{
		UINT ipatmap = pattern_map[ipat];
		DWORD dwSize = 0;
		WORD rows = 64, packsize = 0;
		dwSize = LittleEndian(*((DWORD *)(lpStream + dwMemPos)));

		if(xmheader->version == 0x0102)
		{
			rows = *((BYTE *)(lpStream + dwMemPos + 5)) + 1;
			packsize = LittleEndianW(*((WORD *)(lpStream + dwMemPos + 6)));
		}
		else
		{
			rows = LittleEndianW(*((WORD *)(lpStream + dwMemPos + 5)));
			packsize = LittleEndianW(*((WORD *)(lpStream + dwMemPos + 7)));
		}

		if ((!rows) || (rows > MAX_PATTERN_ROWS)) rows = 64;
		if (dwMemPos + dwSize + 4 > dwMemLength) return 0;
		dwMemPos += dwSize;
		if (dwMemPos + packsize > dwMemLength) return 0;
		ModCommand *p;
		if (ipatmap < MAX_PATTERNS)
		{
			if(sndFile.Patterns.Insert(ipatmap, rows))
				return true;

			if (!packsize) continue;
			p = sndFile.Patterns[ipatmap];
		} else p = NULL;
		const BYTE *src = lpStream+dwMemPos;
		UINT j=0;
		for (UINT row=0; row<rows; row++)
		{
			for (UINT chn=0; chn < xmheader->channels; chn++)
			{
				if ((p) && (j < packsize))
				{
					BYTE b = src[j++];
					UINT vol = 0;
					if (b & 0x80)
					{
						if (b & 1) p->note = src[j++];
						if (b & 2) p->instr = src[j++];
						if (b & 4) vol = src[j++];
						if (b & 8) p->command = src[j++];
						if (b & 16) p->param = src[j++];
					} else
					{
						p->note = b;
						p->instr = src[j++];
						vol = src[j++];
						p->command = src[j++];
						p->param = src[j++];
					}
					if (p->note == 97) p->note = NOTE_KEYOFF;
					else if ((p->note) && (p->note < 97)) p->note += 12;
					if (p->command | p->param) sndFile.ConvertModCommand(*p);
					if (p->instr == 0xff) p->instr = 0;
					if ((vol >= 0x10) && (vol <= 0x50))
					{
						p->volcmd = VOLCMD_VOLUME;
						p->vol = vol - 0x10;
					} else
					if (vol >= 0x60)
					{
						UINT v = vol & 0xF0;
						vol &= 0x0F;
						p->vol = vol;
						switch(v)
						{
						// 60-6F: Volume Slide Down
						case 0x60:	p->volcmd = VOLCMD_VOLSLIDEDOWN; break;
						// 70-7F: Volume Slide Up:
						case 0x70:	p->volcmd = VOLCMD_VOLSLIDEUP; break;
						// 80-8F: Fine Volume Slide Down
						case 0x80:	p->volcmd = VOLCMD_FINEVOLDOWN; break;
						// 90-9F: Fine Volume Slide Up
						case 0x90:	p->volcmd = VOLCMD_FINEVOLUP; break;
						// A0-AF: Set Vibrato Speed
						case 0xA0:	p->volcmd = VOLCMD_VIBRATOSPEED; break;
						// B0-BF: Vibrato
						case 0xB0:	p->volcmd = VOLCMD_VIBRATODEPTH; break;
						// C0-CF: Set Panning
						case 0xC0:	p->volcmd = VOLCMD_PANNING; p->vol = ((vol * 64 + 8) / 15); break;
						// D0-DF: Panning Slide Left
						case 0xD0:	p->volcmd = VOLCMD_PANSLIDELEFT; break;
						// E0-EF: Panning Slide Right
						case 0xE0:	p->volcmd = VOLCMD_PANSLIDERIGHT; break;
						// F0-FF: Tone Portamento
						case 0xF0:	p->volcmd = VOLCMD_TONEPORTAMENTO; break;
						}
					}
					p++;
				} else
				if (j < packsize)
				{
					BYTE b = src[j++];
					if (b & 0x80)
					{
						if (b & 1) j++;
						if (b & 2) j++;
						if (b & 4) j++;
						if (b & 8) j++;
						if (b & 16) j++;
					} else j += 4;
				} else break;
			}
		}
		dwMemPos += packsize;
	}
	return dwMemPos;
}

bool CSoundFile::ReadXM(const BYTE *lpStream, const DWORD dwMemLength)
//--------------------------------------------------------------------
{
	if(lpStream == nullptr || dwMemLength < sizeof(XMFileHeader))
	{
		return false;
	}

	XMFileHeader xmheader;
	DWORD dwMemPos;
	bool madeWithModPlug = false, probablyMadeWithModPlug = false, probablyMPT109 = false, isFT2 = false;

	// Load and convert header
	memcpy(&xmheader, lpStream, sizeof(XMFileHeader));
	xmheader.ConvertEndianness();

	if(xmheader.channels == 0
		|| xmheader.channels > MAX_BASECHANNELS
		|| _strnicmp(xmheader.signature, "Extended Module: ", 17))
	{
		return false;
	}

	if(xmheader.channels > 32)
	{
		// Not entirely true, some other trackers also allow more channels.
		madeWithModPlug = true;
	}

	StringFixer::ReadString<StringFixer::spacePadded>(m_szNames[0], xmheader.songName);

	// Look for null-terminated song name - that's most likely a tune made with modplug
	if(memchr(xmheader.songName, '\0', sizeof(xmheader.songName)) != nullptr)
	{
		probablyMadeWithModPlug = true;
	}

	m_nType = MOD_TYPE_XM;
	m_nMinPeriod = 27;
	m_nMaxPeriod = 54784;

	m_nRestartPos = xmheader.restartpos;
	m_nChannels = xmheader.channels;
	m_nInstruments = min(xmheader.instruments, MAX_INSTRUMENTS - 1);
	m_nSamples = 0;
	m_nDefaultSpeed = CLAMP(xmheader.speed, 1, 31);
	m_nDefaultTempo = CLAMP(xmheader.tempo, 32, 512);

	m_SongFlags.set(SONG_LINEARSLIDES, (xmheader.flags & XMFileHeader::linearSlides) != 0);
	m_SongFlags.set(SONG_EXFILTERRANGE, (xmheader.flags & XMFileHeader::extendedFilterRange) != 0);

	Order.ReadAsByte(lpStream + 80, min(xmheader.orders, MAX_ORDERS), dwMemLength - 80);

	dwMemPos = xmheader.size + 60;

	// set this here already because XMs compressed with BoobieSqueezer will exit the function early
	SetModFlag(MSF_COMPATIBLE_PLAY, true);

	if(xmheader.version >= 0x0104)
	{
		if (dwMemPos + 8 >= dwMemLength) return true;
		dwMemPos = ReadXMPatterns(lpStream, dwMemLength, dwMemPos, &xmheader, *this);
		if(dwMemPos == 0) return true;
	}

	// In case of XM versions < 1.04, we need to memorize the sample flags for all samples, as they are not stored immediately after the sample headers.
	vector<SampleIO> sampleFlags;

	// Reading instruments
	for(INSTRUMENTINDEX instr = 1; instr <= m_nInstruments; instr++)
	{
		XMInstrumentHeader insHeader;
		MemsetZero(insHeader);

		// First, try to read instrument header length...
		if(dwMemPos > dwMemLength || sizeof(uint32) > dwMemLength - dwMemPos)
		{
			return true;
		}
		size_t insHeaderSize = LittleEndian(*reinterpret_cast<const uint32 *>(lpStream + dwMemPos));
		if(insHeaderSize == 0)
		{
			insHeaderSize = sizeof(XMInstrumentHeader);
		}
		LimitMax(insHeaderSize, size_t(dwMemLength - dwMemPos));

		memcpy(&insHeader, lpStream + dwMemPos, min(sizeof(XMInstrumentHeader), insHeaderSize));
		dwMemPos += insHeaderSize;

		if(AllocateInstrument(instr) == nullptr)
		{
			continue;
		}

		insHeader.ConvertEndianness();
		insHeader.ConvertToMPT(*Instruments[instr]);

		if(insHeader.numSamples > 0)
		{
			// Yep, there are some samples associated with this instrument.
			if((insHeader.instrument.midiChannel | insHeader.instrument.midiEnabled | insHeader.instrument.midiProgram | insHeader.instrument.muteComputer | insHeader.instrument.pitchWheelRange) != 0)
			{
				// Definitely not MPT. (or any other tracker)
				isFT2 = true;
			}

			// Read sample headers
			vector<SAMPLEINDEX> sampleSlots = AllocateXMSamples(*this, insHeader.numSamples);

			// Update sample assignment map
			for(size_t k = 0 + 12; k < 96 + 12; k++)
			{
				if(Instruments[instr]->Keyboard[k] < sampleSlots.size())
				{
					Instruments[instr]->Keyboard[k] = sampleSlots[Instruments[instr]->Keyboard[k]];
				}
			}

			if(xmheader.version >= 0x0104)
			{
				sampleFlags.clear();
			}
			// Need to memorize those if we're going to skip any samples...
			vector<uint32> sampleSize(insHeader.numSamples);

			XMSample xmSample;
			MemsetZero(xmSample);

			for(SAMPLEINDEX sample = 0; sample < insHeader.numSamples; sample++)
			{
				// Early versions of Sk@le Tracker didn't set sampleHeaderSize (this fixes IFULOVE.XM)
				const size_t copyBytes = (insHeader.sampleHeaderSize > 0) ? insHeader.sampleHeaderSize : sizeof(XMSample);

				if(dwMemPos > dwMemLength || copyBytes > dwMemLength - dwMemPos)
				{
					return true;
				}

				memcpy(&xmSample, lpStream + dwMemPos, min(copyBytes, sizeof(XMSample)));
				dwMemPos += copyBytes;

				sampleFlags.push_back(xmSample.GetSampleFormat());
				sampleSize[sample] = xmSample.length;

				if(sample < sampleSlots.size())
				{
					SAMPLEINDEX mptSample = sampleSlots[sample];

					xmSample.ConvertToMPT(Samples[mptSample]);
					insHeader.instrument.ApplyAutoVibratoToMPT(Samples[mptSample]);

					StringFixer::ReadString<StringFixer::spacePadded>(m_szNames[mptSample], xmSample.name);

					if((xmSample.flags & 3) == 3)
					{
						// MPT 1.09 and maybe newer / older versions set both flags for bidi loops
						probablyMPT109 = true;
					}
				}
			}

			// Read samples
			if(xmheader.version >= 0x0104)
			{
				for(SAMPLEINDEX sample = 0; sample < insHeader.numSamples; sample++)
				{
					if(sample < sampleSlots.size() && dwMemPos < dwMemLength)
					{
						sampleFlags[sample].ReadSample(Samples[sampleSlots[sample]], (LPSTR)(lpStream + dwMemPos), dwMemLength - dwMemPos);
					}
					dwMemPos += sampleSize[sample];
				}
			}
		}
	}

	if(xmheader.version < 0x0104)
	{
		// Load Patterns and Samples (Version 1.02 and 1.03)
		if (dwMemPos + 8 >= dwMemLength) return true;
		dwMemPos = ReadXMPatterns(lpStream, dwMemLength, dwMemPos, &xmheader, *this);
		if(dwMemPos == 0) return true;

		for(SAMPLEINDEX sample = 1; sample <= GetNumSamples(); sample++)
		{
			if(dwMemPos < dwMemLength)
			{
				dwMemPos += sampleFlags[sample - 1].ReadSample(Samples[sample], (LPSTR)(lpStream + dwMemPos), dwMemLength - dwMemPos);
			}
		}
	}

	// Read song comments: "TEXT"
	if ((dwMemPos + 8 < dwMemLength) && (LittleEndian(*((DWORD *)(lpStream+dwMemPos))) == 0x74786574))
	{
		UINT len = *((DWORD *)(lpStream+dwMemPos+4));
		dwMemPos += 8;
		if ((dwMemPos + len <= dwMemLength) && (len < 16384))
		{
			ReadMessage(lpStream + dwMemPos, len, leCR);
			dwMemPos += len;
		}
		madeWithModPlug = true;
	}
	// Read midi config: "MIDI"
	if ((dwMemPos + 8 < dwMemLength) && (LittleEndian(*((DWORD *)(lpStream + dwMemPos))) == 0x4944494D))
	{
		UINT len = *((DWORD *)(lpStream+dwMemPos+4));
		dwMemPos += 8;
		if (len == sizeof(MIDIMacroConfig))
		{
			memcpy(&m_MidiCfg, lpStream + dwMemPos, len);
			m_MidiCfg.Sanitize();
			m_SongFlags.set(SONG_EMBEDMIDICFG);
			dwMemPos += len;	//rewbs.fix36946
		}
		madeWithModPlug = true;
	}
	// Read pattern names: "PNAM"
	if ((dwMemPos + 8 < dwMemLength) && (LittleEndian(*((DWORD *)(lpStream + dwMemPos))) == 0x4d414e50))
	{
		UINT len = *((DWORD *)(lpStream + dwMemPos + 4));
		dwMemPos += 8;
		if ((dwMemPos + len <= dwMemLength) && (len <= MAX_PATTERNS * MAX_PATTERNNAME) && (len >= MAX_PATTERNNAME))
		{
			DWORD pos = 0;
			PATTERNINDEX nPat = 0;
			for(pos = 0; pos < len; pos += MAX_PATTERNNAME, nPat++)
			{
				Patterns[nPat].SetName((char *)(lpStream + dwMemPos + pos), min(MAX_PATTERNNAME, len - pos));
			}
			dwMemPos += len;
		}
		madeWithModPlug = true;
	}
	// Read channel names: "CNAM"
	if ((dwMemPos + 8 < dwMemLength) && (LittleEndian(*((DWORD *)(lpStream + dwMemPos))) == 0x4d414e43))
	{
		UINT len = *((DWORD *)(lpStream+dwMemPos+4));
		dwMemPos += 8;
		if ((dwMemPos + len <= dwMemLength) && (len <= MAX_BASECHANNELS*MAX_CHANNELNAME))
		{
			UINT n = len / MAX_CHANNELNAME;
			for (UINT i=0; i<n; i++)
			{
				memcpy(ChnSettings[i].szName, (lpStream+dwMemPos + i * MAX_CHANNELNAME), MAX_CHANNELNAME);
				StringFixer::SetNullTerminator(ChnSettings[i].szName);
			}
			dwMemPos += len;
		}
		madeWithModPlug = true;
	}
	// Read mix plugins information
	if (dwMemPos + 8 < dwMemLength)
	{
		DWORD dwOldPos = dwMemPos;
		dwMemPos += LoadMixPlugins(lpStream+dwMemPos, dwMemLength-dwMemPos);
		if(dwMemPos != dwOldPos)
			madeWithModPlug = true;
	}

	// Check various things to find out whether this has been made with MPT.
	// Null chars in names -> most likely made with MPT, which disguises as FT2
	if (!memcmp((LPCSTR)lpStream + 0x26, "FastTracker v2.00   ", 20) && probablyMadeWithModPlug && !isFT2) madeWithModPlug = true;
	if (memcmp((LPCSTR)lpStream + 0x26, "FastTracker v2.00   ", 20)) madeWithModPlug = false;	// this could happen e.g. with (early?) versions of Sk@le
	if (!memcmp((LPCSTR)lpStream + 0x26, "FastTracker v 2.00  ", 20))
	{
		// Early MPT 1.0 alpha/beta versions
		madeWithModPlug = true;
		m_dwLastSavedWithVersion = MAKE_VERSION_NUMERIC(1, 00, 00, 00);
	}

	if (!memcmp((LPCSTR)lpStream + 0x26, "OpenMPT ", 8))
	{
		CHAR sVersion[13];
		memcpy(sVersion, lpStream + 0x26 + 8, 12);
		StringFixer::SetNullTerminator(sVersion);
		m_dwLastSavedWithVersion = MptVersion::ToNum(sVersion);
	}

	if(madeWithModPlug)
	{
		m_nMixLevels = mixLevels_original;
		SetModFlag(MSF_COMPATIBLE_PLAY, false);
		if(!m_dwLastSavedWithVersion)
		{
			if(probablyMPT109)
				m_dwLastSavedWithVersion = MAKE_VERSION_NUMERIC(1, 09, 00, 00);
			else
				m_dwLastSavedWithVersion = MAKE_VERSION_NUMERIC(1, 16, 00, 00);
		}
	}

// -> CODE#0027
// -> DESC="per-instrument volume ramping setup (refered as attack)"

	// Leave if no extra instrument settings are available (end of file reached)
	if(dwMemPos >= dwMemLength) return true;

	bool bInterpretOpenMPTMade = false; // specific for OpenMPT 1.17+ (bMadeWithModPlug is also for MPT 1.16)
	LPCBYTE ptr = lpStream + dwMemPos;
	if(m_nInstruments)
		ptr = LoadExtendedInstrumentProperties(ptr, lpStream+dwMemLength, &bInterpretOpenMPTMade);

	LoadExtendedSongProperties(GetType(), ptr, lpStream, dwMemLength, &bInterpretOpenMPTMade);

	if(bInterpretOpenMPTMade)
	{
		UpgradeModFlags();
	}

	if(bInterpretOpenMPTMade && m_dwLastSavedWithVersion == 0)
		m_dwLastSavedWithVersion = MAKE_VERSION_NUMERIC(1, 17, 01, 00);	// early versions of OpenMPT had no version indication.

	return true;
}


#ifndef MODPLUG_NO_FILESAVE
#include "../mptrack/Moddoc.h"	// for logging errors

bool CSoundFile::SaveXM(LPCSTR lpszFileName, bool compatibilityExport)
//--------------------------------------------------------------------
{
	#define ASSERT_CAN_WRITE(x) \
	if(len > s.size() - x) /*Buffer running out? Make it larger.*/ \
		s.resize(s.size() + 10*1024, 0); \
	\
	if((len > uint16_max - (UINT)x) && GetpModDoc()) /*Reaching the limits of file format?*/ \
	{ \
 		CString str; str.Format("%s (%s %u)\n", str_tooMuchPatternData, str_pattern, pat); \
		GetpModDoc()->AddToLog(str); \
		break; \
	}

	FILE *f;
	if(lpszFileName == nullptr || (f = fopen(lpszFileName, "wb")) == nullptr)
	{
		return false;
	}

	//BYTE s[64*64*5];
	vector<BYTE> s(64*64*5, 0);
	BYTE xmph[9];
	bool addChannel = false; // avoid odd channel count for FT2 compatibility

	XMFileHeader xmheader;
	MemsetZero(xmheader);

	memcpy(xmheader.signature, "Extended Module: ", 17);
	StringFixer::WriteString<StringFixer::spacePadded>(xmheader.songName, m_szNames[0]);
	xmheader.eof = 0x1A;
	memcpy(xmheader.trackerName, "OpenMPT " MPT_VERSION_STR "  ", 20);

	// Writing song header
	xmheader.version = 0x0104;					// XM Format v1.04
	xmheader.size = sizeof(XMFileHeader) - 60; // minus everything before this field
	xmheader.restartpos = m_nRestartPos;

	xmheader.channels = (m_nChannels + 1) & 0xFFFE; // avoid odd channel count for FT2 compatibility
	if((m_nChannels & 1) && m_nChannels < MAX_BASECHANNELS) addChannel = true;
	if(compatibilityExport && xmheader.channels > 32)
		xmheader.channels = 32;
	if(xmheader.channels > MAX_BASECHANNELS) xmheader.channels = MAX_BASECHANNELS;
	xmheader.channels = xmheader.channels;

	// Find out number of orders and patterns used.
	// +++ and --- patterns are not taken into consideration as FastTracker does not support them.
	ORDERINDEX nMaxOrds = 0;
	PATTERNINDEX nPatterns = 0;
	for(ORDERINDEX nOrd = 0; nOrd < Order.GetLengthTailTrimmed(); nOrd++)
	{
		if(Patterns.IsValidIndex(Order[nOrd]))
		{
			nMaxOrds++;
			if(Order[nOrd] >= nPatterns) nPatterns = Order[nOrd] + 1;
		}
	}
	if(!compatibilityExport) nMaxOrds = Order.GetLengthTailTrimmed(); // should really be removed at some point

	xmheader.orders = nMaxOrds;
	xmheader.patterns = nPatterns;
	xmheader.size = xmheader.size + nMaxOrds;

	uint16 writeInstruments;
	if(m_nInstruments > 0)
		xmheader.instruments = writeInstruments = m_nInstruments;
	else
		xmheader.instruments = writeInstruments = m_nSamples;

	if(m_SongFlags[SONG_LINEARSLIDES]) xmheader.flags |= XMFileHeader::linearSlides;
	if(m_SongFlags[SONG_EXFILTERRANGE] && !compatibilityExport) xmheader.flags |= XMFileHeader::extendedFilterRange;
	xmheader.flags = xmheader.flags;

	if(compatibilityExport)
		xmheader.tempo = CLAMP(m_nDefaultTempo, 32, 255);
	else
		xmheader.tempo = CLAMP(m_nDefaultTempo, 32, 512);
	xmheader.speed = CLAMP(m_nDefaultSpeed, 1, 31);

	xmheader.ConvertEndianness();

	fwrite(&xmheader, 1, sizeof(xmheader), f);
	// write order list (wihout +++ and ---, explained above)
	for(ORDERINDEX nOrd = 0; nOrd < Order.GetLengthTailTrimmed(); nOrd++)
	{
		if(Patterns.IsValidIndex(Order[nOrd]) || !compatibilityExport)
		{
			BYTE nOrdval = static_cast<BYTE>(Order[nOrd]);
			fwrite(&nOrdval, 1, sizeof(BYTE), f);
		}
	}

	// Writing patterns
	for(PATTERNINDEX pat = 0; pat < nPatterns; pat++) if (Patterns[pat])
	{
		ModCommand *p = Patterns[pat];
		UINT len = 0;
		// Empty patterns are always loaded as 64-row patterns in FT2, regardless of their real size...
		bool emptyPatNeedsFixing = (Patterns[pat].GetNumRows() != 64);

		MemsetZero(xmph);
		xmph[0] = 9;
		xmph[5] = (BYTE)(Patterns[pat].GetNumRows() & 0xFF);
		xmph[6] = (BYTE)(Patterns[pat].GetNumRows() >> 8);

		for (UINT j = m_nChannels * Patterns[pat].GetNumRows(); j > 0; j--, p++)
		{
			// Don't write more than 32 channels
			if(compatibilityExport && m_nChannels - ((j - 1) % m_nChannels) > 32) continue;

			uint8 note = p->note;
			uint8 command = p->command, param = p->param;
			ModSaveCommand(command, param, true, compatibilityExport);

			if (note >= NOTE_MIN_SPECIAL) note = 97; else
			if ((note <= 12) || (note > 96+12)) note = 0; else
			note -= 12;
			uint8 vol = 0;
			if (p->volcmd)
			{
				switch(p->volcmd)
				{
				case VOLCMD_VOLUME:			vol = 0x10 + p->vol; break;
				case VOLCMD_VOLSLIDEDOWN:	vol = 0x60 + (p->vol & 0x0F); break;
				case VOLCMD_VOLSLIDEUP:		vol = 0x70 + (p->vol & 0x0F); break;
				case VOLCMD_FINEVOLDOWN:	vol = 0x80 + (p->vol & 0x0F); break;
				case VOLCMD_FINEVOLUP:		vol = 0x90 + (p->vol & 0x0F); break;
				case VOLCMD_VIBRATOSPEED:	vol = 0xA0 + (p->vol & 0x0F); break;
				case VOLCMD_VIBRATODEPTH:	vol = 0xB0 + (p->vol & 0x0F); break;
				case VOLCMD_PANNING:		vol = 0xC0 + ((p->vol * 15 + 32) / 64); if (vol > 0xCF) vol = 0xCF; break;
				case VOLCMD_PANSLIDELEFT:	vol = 0xD0 + (p->vol & 0x0F); break;
				case VOLCMD_PANSLIDERIGHT:	vol = 0xE0 + (p->vol & 0x0F); break;
				case VOLCMD_TONEPORTAMENTO:	vol = 0xF0 + (p->vol & 0x0F); break;
				}
				// Those values are ignored in FT2. Don't save them, also to avoid possible problems with other trackers (or MPT itself)
				if(compatibilityExport && p->vol == 0)
				{
					switch(p->volcmd)
					{
					case VOLCMD_VOLUME:
					case VOLCMD_PANNING:
					case VOLCMD_VIBRATODEPTH:
					case VOLCMD_TONEPORTAMENTO:
						break;
					default:
						// no memory here.
						vol = 0;
					}
				}
			}

			// no need to fix non-empty patterns
			if(!p->IsEmpty())
				emptyPatNeedsFixing = false;

			// apparently, completely empty patterns are loaded as empty 64-row patterns in FT2, regardless of their original size.
			// We have to avoid this, so we add a "break to row 0" command in the last row.
			if(j == 1 && emptyPatNeedsFixing)
			{
				command = 0x0D;
				param = 0;
			}

			if ((note) && (p->instr) && (vol > 0x0F) && (command) && (param))
			{
				s[len++] = note;
				s[len++] = p->instr;
				s[len++] = vol;
				s[len++] = command;
				s[len++] = param;
			} else
			{
				BYTE b = 0x80;
				if (note) b |= 0x01;
				if (p->instr) b |= 0x02;
				if (vol >= 0x10) b |= 0x04;
				if (command) b |= 0x08;
				if (param) b |= 0x10;
				s[len++] = b;
				if (b & 1) s[len++] = note;
				if (b & 2) s[len++] = p->instr;
				if (b & 4) s[len++] = vol;
				if (b & 8) s[len++] = command;
				if (b & 16) s[len++] = param;
			}

			if(addChannel && (j % m_nChannels == 1 || m_nChannels == 1))
			{
				ASSERT_CAN_WRITE(1);
				s[len++] = 0x80;
			}

			ASSERT_CAN_WRITE(5);

		}
		xmph[7] = (BYTE)(len & 0xFF);
		xmph[8] = (BYTE)(len >> 8);
		fwrite(xmph, 1, 9, f);
		fwrite(&s[0], 1, len, f);
	} else
	{
		MemsetZero(xmph);
		xmph[0] = 9;
		xmph[5] = (BYTE)(Patterns[pat].GetNumRows() & 0xFF);
		xmph[6] = (BYTE)(Patterns[pat].GetNumRows() >> 8);
		fwrite(xmph, 1, 9, f);
	}

	// Check which samples are referenced by which instruments (for assigning unreferenced samples to instruments)
	vector<bool> sampleAssigned(GetNumSamples() + 1, false);
	for(INSTRUMENTINDEX ins = 1; ins <= GetNumInstruments(); ins++)
	{
		if(Instruments[ins] != nullptr)
		{
			Instruments[ins]->GetSamples(sampleAssigned);
		}
	}

	// Writing instruments
	for(INSTRUMENTINDEX ins = 1; ins <= writeInstruments; ins++)
	{
		XMInstrumentHeader insHeader;
		vector<SAMPLEINDEX> samples;

		if(GetNumInstruments())
		{
			if(Instruments[ins] != nullptr)
			{
				// Convert instrument
				insHeader.ConvertToXM(*Instruments[ins], compatibilityExport);

				samples = insHeader.instrument.GetSampleList(*Instruments[ins], compatibilityExport);
				if(samples.size() > 0 && samples[0] <= GetNumSamples())
				{
					// Copy over auto-vibrato settings of first sample
					insHeader.instrument.ApplyAutoVibratoToXM(Samples[samples[0]], GetType());
				}

				vector<SAMPLEINDEX> additionalSamples;

				// Try to save "instrument-less" samples as well by adding those after the "normal" samples of our sample.
				// We look for unassigned samples directly after the samples assigned to our current instrument, so if
				// e.g. sample 1 is assigned to instrument 1 and samples 2 to 10 aren't assigned to any instrument,
				// we will assign those to sample 1. Any samples before the first referenced sample are going to be lost,
				// but hey, I wrote this mostly for preserving instrument texts in existing modules, where we shouldn't encounter this situation...
				for(vector<SAMPLEINDEX>::const_iterator sample = samples.begin(); sample != samples.end(); sample++)
				{
					SAMPLEINDEX smp = *sample;
					while(++smp <= GetNumSamples()
						&& !sampleAssigned[smp]
						&& insHeader.numSamples < (compatibilityExport ? 16 : 32))
					{
						sampleAssigned[smp] = true;			// Don't want to add this sample again.
						additionalSamples.push_back(smp);
						insHeader.numSamples++;
					}
				}

				samples.insert(samples.end(), additionalSamples.begin(), additionalSamples.end());
			} else
			{
				MemsetZero(insHeader);
			}
		} else
		{
			// Convert samples to instruments
			MemsetZero(insHeader);
			insHeader.numSamples = 1;
			insHeader.instrument.ApplyAutoVibratoToXM(Samples[ins], GetType());
			samples.push_back(ins);
		}

		insHeader.Finalise();
		size_t insHeaderSize = insHeader.size;
		insHeader.ConvertEndianness();
		fwrite(&insHeader, 1, insHeaderSize, f);

		vector<SampleIO> sampleFlags(samples.size());

		// Write Sample Headers
		for(SAMPLEINDEX smp = 0; smp < samples.size(); smp++)
		{
			XMSample xmSample;
			if(samples[smp] <= GetNumSamples())
			{
				xmSample.ConvertToXM(Samples[samples[smp]], GetType(), compatibilityExport);
			} else
			{
				MemsetZero(xmSample);
			}
			sampleFlags[smp] = xmSample.GetSampleFormat();

			StringFixer::WriteString<StringFixer::spacePadded>(xmSample.name, m_szNames[samples[smp]]);

			fwrite(&xmSample, 1, sizeof(xmSample), f);
		}

		// Write Sample Data
		for(SAMPLEINDEX smp = 0; smp < samples.size(); smp++)
		{
			if(samples[smp] <= GetNumSamples())
			{
				sampleFlags[smp].WriteSample(f, Samples[samples[smp]]);
			}
		}
	}

	if(!compatibilityExport)
	{
		// Writing song comments
		if ((m_lpszSongComments) && (m_lpszSongComments[0]))
		{
			DWORD d = 0x74786574;
			fwrite(&d, 1, 4, f);
			d = strlen(m_lpszSongComments);
			fwrite(&d, 1, 4, f);
			fwrite(m_lpszSongComments, 1, d, f);
		}
		// Writing midi cfg
		if (m_SongFlags[SONG_EMBEDMIDICFG])
		{
			DWORD d = 0x4944494D;
			fwrite(&d, 1, 4, f);
			d = sizeof(MIDIMacroConfig);
			fwrite(&d, 1, 4, f);
			fwrite(&m_MidiCfg, 1, sizeof(MIDIMacroConfig), f);
		}
		// Writing Pattern Names
		const PATTERNINDEX numNamedPats = Patterns.GetNumNamedPatterns();
		if (numNamedPats > 0)
		{
			DWORD dwLen = numNamedPats * MAX_PATTERNNAME;
			DWORD d = 0x4d414e50;
			fwrite(&d, 1, 4, f);
			fwrite(&dwLen, 1, 4, f);

			for(PATTERNINDEX nPat = 0; nPat < numNamedPats; nPat++)
			{
				char name[MAX_PATTERNNAME];
				MemsetZero(name);
				Patterns[nPat].GetName(name);
				fwrite(name, 1, MAX_PATTERNNAME, f);
			}
		}
		// Writing Channel Names
		{
			UINT nChnNames = 0;
			for (UINT inam=0; inam<m_nChannels; inam++)
			{
				if (ChnSettings[inam].szName[0]) nChnNames = inam+1;
			}
			// Do it!
			if (nChnNames)
			{
				DWORD dwLen = nChnNames * MAX_CHANNELNAME;
				DWORD d = 0x4d414e43;
				fwrite(&d, 1, 4, f);
				fwrite(&dwLen, 1, 4, f);
				for (UINT inam=0; inam<nChnNames; inam++)
				{
					fwrite(ChnSettings[inam].szName, 1, MAX_CHANNELNAME, f);
				}
			}
		}

		//Save hacked-on extra info
		SaveMixPlugins(f);
		if(m_nInstruments)
		{
			SaveExtendedInstrumentProperties(min(GetNumInstruments(), writeInstruments), f);
		}
		SaveExtendedSongProperties(f);
	}

	fclose(f);
	return true;
}

#endif // MODPLUG_NO_FILESAVE