/*
 * load_dmf.cpp
 * ------------
 * Purpose: DMF module loader (X-Tracker by D-LUSiON).
 * Notes  : If it wasn't already outdated when the tracker was released, this would be a rather interesting
 *          and in some parts even sophisticated format - effect columns are separated by effect type, an easy to
 *          understand BPM tempo mode, effect durations are always divided into a 256th row, vibrato effects are
 *			specified by period length and the same 8-Bit granularity is used for both volume and panning.
 *          Unluckily, this format does not offer any envelopes or multi-sample instruments, and bidi sample loops
 *          are missing as well, so it was already well behind FT2 and IT back then.
 * Authors: Johannes Schultz (mostly based on DMF.TXT, DMF_EFFC.TXT, trial and error and some invaluable hints by Zatzen)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"
#ifdef MODPLUG_TRACKER
#include "../mptrack/Moddoc.h"
#endif // MODPLUG_TRACKER

// 32-bit chunk identifiers
#define DMF_DDMF	0x464D4444
#define DMF_CMSG	0x47534D43
#define DMF_SEQU	0x55514553
#define DMF_PATT	0x54544150
#define DMF_SMPI	0x49504D53
#define DMF_SMPD	0x44504D53
#define DMF_SMPJ	0x4A504D53
#define DMF_ENDE	0x45444E45
#define DMF_SETT	0x9C219DE4

// Pattern flags - global track
#define DMFPAT_GLOBPACK	0x80	// Pack information for global track follows
#define DMFPAT_GLOBMASK	0x3F	// Mask for global effects
// Pattern flags - note tracks
#define DMFPAT_COUNTER	0x80	// Pack information for current channel follows
#define DMFPAT_INSTR	0x40	// Instrument number present
#define DMFPAT_NOTE		0x20	// Note present
#define DMFPAT_VOLUME	0x10	// Volume present
#define DMFPAT_INSEFF	0x08	// Instrument effect present
#define DMFPAT_NOTEEFF	0x04	// Note effect present
#define DMFPAT_VOLEFF	0x02	// Volume effect stored

// Sample flags
#define DMFSMP_LOOP		0x01
#define DMFSMP_16BIT	0x02
#define DMFSMP_COMPMASK 0x0C
#define DMFSMP_COMP1	0x04	// Compression type 1
#define DMFSMP_COMP2	0x08	// Compression type 2 (unused)
#define DMFSMP_COMP3	0x0C	// Compression type 3 (dito)
#define DMFSMP_LIBRARY	0x80	// Sample is stored in a library

#pragma pack(push, 1)

// DMF header
struct DMFHEADER
{
	uint32 signature;		// "DDMF"
	uint8  version;			// 1 - 7 are beta versions, 8 is the official thing, 10 is xtracker32
	char   tracker[8];		// "XTRACKER"
	char   songname[30];
	char   composer[20];
	uint8  creationDay;
	uint8  creationMonth;
	uint8  creationYear;
};

struct DMF_IFFCHUNK
{
	uint32 signature;	// 4-letter identifier
	uint32 chunksize;	// chunk size without header
};

// Order list
struct DMFCHUNK_SEQUENCE
{
	uint16 loopStart;
	uint16 loopEnd;
	// order list follows here ...
};

// Pattern header (global)
struct DMFCHUNK_PATTERNS
{
	uint16 numPatterns;	// 1..1024 patterns
	uint8  numTracks;	// 1..32 channels
};

// Pattern header (for each pattern)
struct DMFCHUNK_PATTERNHEADER
{
	uint8  numTracks;	// 1..32 channels
	uint8  beat;		// [hi|lo] -> hi = rows per beat, lo = reserved
	uint16 numRows;
	uint32 patternLength;
	// patttern data follows here ...
};

// Sample header
struct DMFCHUNK_SAMPLEHEADER
{
	uint32 length;
	uint32 loopStart;
	uint32 loopEnd;
	uint16 c3freq;		// 1000..45000hz
	uint8  volume;		// 0 = ignore
	uint8  flags;
};

// Sample header tail (between head and tail, there might be the library name of the sample, depending on the DMF version)
struct DMFCHUNK_SAMPLEHEADERTAIL
{
	uint16 filler;
	uint32 crc32;
};

#pragma pack(pop)

// Pattern translation memory
struct DMF_PATTERNSETTINGS
{
	uint8 beat;								// Rows per beat
	uint8 tempoTicks;						// Tick mode param
	uint8 tempoBPM;							// BPM mode param
	bool realBPMmode;						// true = BPM mode
	uint8 internalTicks;					// Ticks per row in final pattern
	vector<bool> playDir;					// Sample play direction of each channel... false = forward (default)
	vector<ModCommand::NOTE> noteBuffer;	// Note buffer
	vector<ModCommand::NOTE> lastNote;		// Last played note on channel
};


// Convert portamento value (not very accurate due to X-Tracker's higher granularity, to say the least)
uint8 DMFporta2MPT(uint8 val, const uint8 internalTicks, const bool hasFine)
//--------------------------------------------------------------------------
{
	if(val == 0)
		return 0;
	else if((val <= 0x0F || internalTicks < 2) && hasFine)
		return (val | 0xF0);
	else
		return max(1, (val / (internalTicks - 1)));	// no porta on first tick!
}


// Convert portamento / volume slide value (not very accurate due to X-Tracker's higher granularity, to say the least)
uint8 DMFslide2MPT(uint8 val, const uint8 internalTicks, const bool up)
//---------------------------------------------------------------------
{
	val = max(1, val / 4);
	const bool isFine = (val < 0x0F) || (internalTicks < 2);
	if(!isFine)
		val = max(1, (val + internalTicks - 2) / (internalTicks - 1));	// no slides on first tick! "+ internalTicks - 2" for rounding precision

	if(up)
		return (isFine ? 0x0F : 0x00) | (val << 4);
	else
		return (isFine ? 0xF0 : 0x00) | (val & 0x0F);

}


// Calculate tremor on/off param
uint8 DMFtremor2MPT(uint8 val, const uint8 internalTicks)
//-------------------------------------------------------
{
	uint8 ontime = (val >> 4);
	uint8 offtime = (val & 0x0F);
	ontime = CLAMP(ontime * internalTicks / 15, 1, 15);
	offtime = CLAMP(offtime * internalTicks / 15, 1, 15);
	return (ontime << 4) | offtime;
}


// Calculate delay parameter for note cuts / delays
uint8 DMFdelay2MPT(uint8 val, const uint8 internalTicks)
//------------------------------------------------------
{
	int newval = (int)val * (int)internalTicks / 255;
	Limit(newval, 0, 15);
	return (uint8)newval;
}


// Convert vibrato-style command parameters
uint8 DMFvibrato2MPT(uint8 val, const uint8 internalTicks)
//--------------------------------------------------------
{
	// MPT: 1 vibrato period == 64 ticks... we have internalTicks ticks per row.
	// X-Tracker: Period length specified in rows!
	const int periodInTicks = max(1, (val >> 4)) * internalTicks;
	const uint8 matchingPeriod = (uint8)CLAMP((128 / periodInTicks), 1, 15);
	return (matchingPeriod << 4) | max(1, (val & 0x0F));
}


PATTERNINDEX ConvertDMFPattern(const LPCBYTE lpStream, const DWORD dwMemLength, DMF_PATTERNSETTINGS &settings, CSoundFile *pSndFile)
//----------------------------------------------------------------------------------------------------------------------------------
{
	#define ASSERT_CAN_READ_PATTERN(x) ASSERT_CAN_READ_PROTOTYPE(dwMemPos, dwMemLength, x, return nPat);

	DWORD dwMemPos = 0;

	// ASSERT_CAN_READ_PATTERN(sizeof(DMFCHUNK_PATTERNHEADER)); -- already done in main loop
	DMFCHUNK_PATTERNHEADER *patHead = (DMFCHUNK_PATTERNHEADER *)(lpStream + dwMemPos);
	dwMemPos += sizeof(DMFCHUNK_PATTERNHEADER);

	const ROWINDEX numRows = CLAMP(LittleEndianW(patHead->numRows), 1, MAX_PATTERN_ROWS);
	const PATTERNINDEX nPat = pSndFile->Patterns.Insert(numRows);
	if(nPat == PATTERNINDEX_INVALID)
	{
		return nPat;
	}

	ModCommand *m = pSndFile->Patterns[nPat];
	const CHANNELINDEX numChannels = min(pSndFile->GetNumChannels(), patHead->numTracks);

	// When breaking to a pattern with less channels that the previous pattern,
	// all voices in the now unused channels are killed:
	for(CHANNELINDEX nChn = numChannels + 1; nChn < pSndFile->GetNumChannels(); nChn++)
	{
		m[nChn].note = NOTE_NOTECUT;
	}

	// Initialize tempo stuff
	settings.beat = (patHead->beat >> 4);
	bool tempoChange = settings.realBPMmode;
	uint8 writeDelay = 0;

	// Counters for channel packing (including global track)
	vector<uint8> channelCounter(numChannels + 1, 0);

	for(ROWINDEX nRow = 0; nRow < numRows; nRow++)
	{
		// Global track info counter reached 0 => read global track data
		if(channelCounter[0] == 0)
		{
			ASSERT_CAN_READ_PATTERN(1);
			uint8 globalInfo = lpStream[dwMemPos++];
			// 0x80: Packing counter (if not present, counter stays at 0)
			if((globalInfo & DMFPAT_GLOBPACK) != 0)
			{
				ASSERT_CAN_READ_PATTERN(1);
				channelCounter[0] = lpStream[dwMemPos++];
			}

			globalInfo &= DMFPAT_GLOBMASK;

			uint8 globalData = 0;
			if(globalInfo != 0)
			{
				ASSERT_CAN_READ_PATTERN(1);
				globalData = lpStream[dwMemPos++];
			}

			switch(globalInfo)
			{
			case 1:		// Set Tick Frame Speed
				settings.realBPMmode = false;
				settings.tempoTicks = max(1, globalData);	// Tempo in 1/4 rows per second
				settings.tempoBPM = 0;						// Automatically updated by X-Tracker
				tempoChange = true;
				break;
			case 2:		// Set BPM Speed (real BPM mode)
				if(globalData)	// DATA = 0 doesn't do anything
				{
					settings.realBPMmode = true;
					settings.tempoBPM = globalData;		// Tempo in real BPM (depends on rows per beat)
					if(settings.beat != 0)
					{
						settings.tempoTicks = (globalData * settings.beat * 15);	// Automatically updated by X-Tracker
					}
					tempoChange = true;
				}
				break;
			case 3:		// Set Beat
				settings.beat = (globalData >> 4);
				if(settings.beat != 0)
				{
					// Tempo changes only if we're in real BPM mode
					tempoChange = settings.realBPMmode;
				} else
				{
					// If beat is 0, change to tick speed mode, but keep current tempo
					settings.realBPMmode = false;
				}
				break;
			case 4:		// Tick Delay
				writeDelay = globalData;
				break;
			case 5:		// Set External Flag
				break;
			case 6:		// Slide Speed Up
				if(globalData > 0)
				{
					uint8 &tempoData = (settings.realBPMmode) ? settings.tempoBPM : settings.tempoTicks;
					if(tempoData < 256 - globalData)
					{
						tempoData += globalData;
					} else
					{
						tempoData = 255;
					}
					tempoChange = true;
				}
				break;
			case 7:		// Slide Speed Down
				if(globalData > 0)
				{
					uint8 &tempoData = (settings.realBPMmode) ? settings.tempoBPM : settings.tempoTicks;
					if(tempoData > 1 + globalData)
					{
						tempoData -= globalData;
					} else
					{
						tempoData = 1;
					}
					tempoChange = true;
				}
				break;
			}
		} else
		{
			channelCounter[0]--;
		}

		// These will eventually be written to the pattern
		int speed = 0, tempo = 0;

		if(tempoChange)
		{
			// Can't do anything if we're in BPM mode and there's no rows per beat set...
			if(!settings.realBPMmode || settings.beat)
			{
				// My approach to convert X-Tracker's "tick speed" (1/4 rows per second):
				// Tempo * 6 / Speed = Beats per Minute
				// => Tempo * 6 / (Speed * 60) = Beats per Second
				// => Tempo * 24 / (Speed * 60) = Rows per Second (4 rows per beat at tempo 6)
				// => Tempo = 60 * Rows per Second * Speed / 24
				// For some reason, using settings.tempoTicks + 1 gives more accurate results than just settings.tempoTicks... (same problem in the old libmodplug DMF loader)
				// Original unoptimized formula:
				//const int tickspeed = (tempoRealBPMmode) ? max(1, (tempoData * beat * 4) / 60) : tempoData;
				const int tickspeed = (settings.realBPMmode) ? max(1, settings.tempoBPM * settings.beat * 2) : ((settings.tempoTicks + 1) * 30);
				// Try to find matching speed - try higher speeds first, so that effects like arpeggio and tremor work better.
				for(speed = 255; speed > 1; speed--)
				{
					// Original unoptimized formula:
					// tempo = 30 * tickspeed * speed / 48;
					tempo = tickspeed * speed / 48;
					if(tempo >= 32 && tempo <= 255)
					{
						break;
					}
				}
				Limit(tempo, 32, 255);
				settings.internalTicks = (uint8)speed;
			} else
			{
				tempoChange = false;
			}
		}

		m = pSndFile->Patterns[nPat].GetpModCommand(nRow, 1);	// Reserve first channel for global effects

		for(CHANNELINDEX nChn = 1; nChn <= numChannels; nChn++, m++)
		{
			// Track info counter reached 0 => read track data
			if(channelCounter[nChn] == 0)
			{
				ASSERT_CAN_READ_PATTERN(1);
				const uint8 channelInfo = lpStream[dwMemPos++];
				////////////////////////////////////////////////////////////////
				// 0x80: Packing counter (if not present, counter stays at 0)
				if((channelInfo & DMFPAT_COUNTER) != 0)
				{
					ASSERT_CAN_READ_PATTERN(1);
					channelCounter[nChn] = lpStream[dwMemPos++];
				}

				////////////////////////////////////////////////////////////////
				// 0x40: Instrument
				bool slideNote = true;		// If there is no instrument number next to a note, the note is not retriggered!
				if((channelInfo & DMFPAT_INSTR) != 0)
				{
					ASSERT_CAN_READ_PATTERN(1);
					m->instr = lpStream[dwMemPos++];
					if(m->instr != 0)
					{
						slideNote = false;
					}
				}

				////////////////////////////////////////////////////////////////
				// 0x20: Note
				if((channelInfo & DMFPAT_NOTE) != 0)
				{
					ASSERT_CAN_READ_PATTERN(1);
					m->note = lpStream[dwMemPos++];
					if(m->note >= 1 && m->note <= 108)
					{
						m->note = CLAMP(m->note + 24, NOTE_MIN, NOTE_MAX);
						settings.lastNote[nChn] = m->note;
					} else if(m->note >= 129 && m->note <= 236)
					{
						// "Buffer notes" for portamento (and other effects?) that are actually not played, but just "queued"...
						m->note = CLAMP((m->note & 0x7F) + 24, NOTE_MIN, NOTE_MAX);
						settings.noteBuffer[nChn] = m->note;
						m->note = NOTE_NONE;
					} else if(m->note == 255)
					{
						m->note = NOTE_NOTECUT;
					}
				}

				// If there's just an instrument number, but no note, retrigger sample.
				if(m->note == NOTE_NONE && m->instr > 0)
				{
					m->note = settings.lastNote[nChn];
					m->instr = 0;
				}

				if(m->IsNote())
				{
					settings.playDir[nChn] = false;
				}

				uint8 effect1 = CMD_NONE, effect2 = CMD_NONE, effect3 = CMD_NONE;
				uint8 effectParam1 = 0, effectParam2 = 0, effectParam3 = 0;

				////////////////////////////////////////////////////////////////
				// 0x10: Volume
				if((channelInfo & DMFPAT_VOLUME) != 0)
				{
					ASSERT_CAN_READ_PATTERN(1);
					m->volcmd = VOLCMD_VOLUME;
					m->vol = (lpStream[dwMemPos++] + 3) / 4;
				}

				////////////////////////////////////////////////////////////////
				// 0x08: Instrument effect
				if((channelInfo & DMFPAT_INSEFF) != 0)
				{
					ASSERT_CAN_READ_PATTERN(2);
					effect1 = lpStream[dwMemPos++];
					effectParam1 = lpStream[dwMemPos++];

					switch(effect1)
					{
					case 1:		// Stop Sample
						m->note = NOTE_NOTECUT;
						effect1 = CMD_NONE;
						break;
					case 2:		// Stop Sample Loop
						m->note = NOTE_KEYOFF;
						effect1 = CMD_NONE;
						break;
					case 3:		// Instrument Volume Override (aka "Restart")
						m->note = settings.lastNote[nChn];
						settings.playDir[nChn] = false;
						effect1 = CMD_NONE;
						break;
					case 4:		// Sample Delay
						effectParam1 = DMFdelay2MPT(effectParam1, settings.internalTicks);
						if(effectParam1)
						{
							effect1 = CMD_S3MCMDEX;
							effectParam1 = 0xD0 | (effectParam1);
						} else
						{
							effect1 = CMD_NONE;
						}
						if(m->note == NOTE_NONE)
						{
							m->note = settings.lastNote[nChn];
							settings.playDir[nChn] = false;
						}
						break;
					case 5:		// Tremolo Retrig Sample (who invented those stupid effect names?)
						effectParam1 = max(1, DMFdelay2MPT(effectParam1, settings.internalTicks));
						effect1 = CMD_RETRIG;
						settings.playDir[nChn] = false;
						break;
					case 6:		// Offset
					case 7:		// Offset + 64k
					case 8:		// Offset + 128k
					case 9:		// Offset + 192k
						// Put high offset on previous row
						if(nRow > 0)
						{
							pSndFile->Patterns[nPat].WriteEffect(EffectWriter(CMD_S3MCMDEX, (0xA0 | (effect1 - 6))).Row(nRow - 1).Channel(nChn).Retry(EffectWriter::rmTryPreviousRow));
						}
						effect1 = CMD_OFFSET;
						settings.playDir[nChn] = false;
						break;
					case 10:	// Invert Sample play direction ("Tekkno Invert")
						effect1 = CMD_S3MCMDEX;
						if(settings.playDir[nChn] == false)
							effectParam1 = 0x9F;
						else
							effectParam1 = 0x9E;
						settings.playDir[nChn] = !settings.playDir[nChn];
						break;
					default:
						effect1 = CMD_NONE;
						break;
					}
				}

				////////////////////////////////////////////////////////////////
				// 0x04: Note effect
				if((channelInfo & DMFPAT_NOTEEFF) != 0)
				{
					ASSERT_CAN_READ_PATTERN(2);
					effect2 = lpStream[dwMemPos++];
					effectParam2 = lpStream[dwMemPos++];

					switch(effect2)
					{
					case 1:		// Note Finetune
						effect2 = static_cast<ModCommand::COMMAND>(effectParam2 < 128 ? CMD_PORTAMENTOUP : CMD_PORTAMENTODOWN);
						if(effectParam2 > 128) effectParam2 = 255 - effectParam2 + 1;
						effectParam2 = 0xF0 | min(0x0F, effectParam2);	// Well, this is not too accurate...
						break;
					case 2:		// Note Delay (wtf is the difference to Sample Delay?)
						effectParam2 = DMFdelay2MPT(effectParam2, settings.internalTicks);
						if(effectParam2)
						{
							effect2 = CMD_S3MCMDEX;
							effectParam2 = 0xD0 | (effectParam2);
						} else
						{
							effect2 = CMD_NONE;
						}
						break;
					case 3:		// Arpeggio
						effect2 = CMD_ARPEGGIO;
						break;
					case 4:		// Portamento Up
					case 5:		// Portamento Down
						effectParam2 = DMFporta2MPT(effectParam2, settings.internalTicks, true);
						effect2 = static_cast<ModCommand::COMMAND>(effect2 == 4 ? CMD_PORTAMENTOUP : CMD_PORTAMENTODOWN);
						break;
					case 6:		// Portamento to Note
						if(m->note == NOTE_NONE)
						{
							m->note = settings.noteBuffer[nChn];
						}
						effectParam2 = DMFporta2MPT(effectParam2, settings.internalTicks, false);
						effect2 = CMD_TONEPORTAMENTO;
						break;
					case 7:		// Scratch to Note (neat! but we don't have such an effect...)
						m->note = CLAMP(effectParam2 + 25, NOTE_MIN, NOTE_MAX);
						effect2 = CMD_TONEPORTAMENTO;
						effectParam2 = 0xFF;
						break;
					case 8:		// Vibrato Sine
					case 9:		// Vibrato Triangle (ramp down should be close enough)
					case 10:	// Vibrato Square
						// Put vibrato type on previous row
						if(nRow > 0)
						{
							pSndFile->Patterns[nPat].WriteEffect(EffectWriter(CMD_S3MCMDEX, (0x30 | (effect2 - 8))).Row(nRow - 1).Channel(nChn).Retry(EffectWriter::rmTryPreviousRow));
						}
						effect2 = CMD_VIBRATO;
						effectParam2 = DMFvibrato2MPT(effectParam2, settings.internalTicks);
						break;
					case 11:	 // Note Tremolo
						effectParam2 = DMFtremor2MPT(effectParam2, settings.internalTicks);
						effect2 = CMD_TREMOR;
						break;
					case 12:	// Note Cut
						effectParam2 = DMFdelay2MPT(effectParam2, settings.internalTicks);
						if(effectParam2)
						{
							effect2 = CMD_S3MCMDEX;
							effectParam2 = 0xC0 | (effectParam2);
						} else
						{
							effect2 = CMD_NONE;
							m->note = NOTE_NOTECUT;
						}
						break;
					default:
						effect2 = CMD_NONE;
						break;
					}
				}

				////////////////////////////////////////////////////////////////
				// 0x02: Volume effect
				if((channelInfo & DMFPAT_VOLEFF) != 0)
				{
					ASSERT_CAN_READ_PATTERN(2);
					effect3 = lpStream[dwMemPos++];
					effectParam3 = lpStream[dwMemPos++];

					switch(effect3)
					{
					case 1:		// Volume Slide Up
					case 2:		// Volume Slide Down
						effectParam3 = DMFslide2MPT(effectParam3, settings.internalTicks, (effect3 == 1));
						effect3 = CMD_VOLUMESLIDE;
						break;
					case 3:		// Volume Tremolo (actually this is Tremor)
						effectParam3 = DMFtremor2MPT(effectParam3, settings.internalTicks);
						effect3 = CMD_TREMOR;
						break;
					case 4:		// Tremolo Sine
					case 5:		// Tremolo Triangle (ramp down should be close enough)
					case 6:		// Tremolo Square
						// Put tremolo type on previous row
						if(nRow > 0)
						{
							pSndFile->Patterns[nPat].WriteEffect(EffectWriter(CMD_S3MCMDEX, (0x40 | (effect3 - 4))).Row(nRow - 1).Channel(nChn).Retry(EffectWriter::rmTryPreviousRow));
						}
						effect3 = CMD_TREMOLO;
						effectParam3 = DMFvibrato2MPT(effectParam3, settings.internalTicks);
						break;
					case 7:		// Set Balance
						effect3 = CMD_PANNING8;
						break;
					case 8:		// Slide Balance Left
					case 9:		// Slide Balance Right
						effectParam3 = DMFslide2MPT(effectParam3, settings.internalTicks, (effect3 == 8));
						effect3 = CMD_PANNINGSLIDE;
						break;
					case 10:	// Balance Vibrato Left/Right (always sine modulated)
						effect3 = CMD_PANBRELLO;
						effectParam3 = DMFvibrato2MPT(effectParam3, settings.internalTicks);
						break;
					default:
						effect3 = CMD_NONE;
						break;
					}
				}

				// I guess this is close enough to "not retriggering the note"
				if(slideNote && m->IsNote())
				{
					if(effect2 == CMD_NONE)
					{
						effect2 = CMD_TONEPORTAMENTO;
						effectParam2 = 0xFF;
					} else if(effect3 == CMD_NONE && effect2 != CMD_TONEPORTAMENTO)	// Tone portamentos normally go in effect #2
					{
						effect3 = CMD_TONEPORTAMENTO;
						effectParam3 = 0xFF;
					}
				}
				// If one of the effects is unused, temporarily put volume commands in there
				if(m->volcmd == VOLCMD_VOLUME)
				{
					if(effect2 == CMD_NONE)
					{
						effect2 = CMD_VOLUME;
						effectParam2 = m->vol;
						m->volcmd = VOLCMD_NONE;
					} else if(effect3 == CMD_NONE)
					{
						effect3 = CMD_VOLUME;
						effectParam3 = m->vol;
						m->volcmd = VOLCMD_NONE;
					}
				}

				// Do that dance.
				// Maybe I should quit rewriting this everywhere and make a generic version :P
				int n;
				for (n = 0; n < 4; n++)
				{
					if(ModCommand::ConvertVolEffect(effect2, effectParam2, (n >> 1) != 0))
					{
						n = 5;
						break;
					}
					std::swap(effect2, effect3);
					std::swap(effectParam2, effectParam3);
				}
				if (n < 5)
				{
					if (ModCommand::GetEffectWeight((ModCommand::COMMAND)effect2) > ModCommand::GetEffectWeight((ModCommand::COMMAND)effect3))
					{
						std::swap(effect2, effect3);
						std::swap(effectParam2, effectParam3);
					}
					effect2 = CMD_NONE;
				}
				if (!effect2)
					effectParam2 = 0;
				if (!effect3)
					effectParam3 = 0;

				if(m->volcmd == VOLCMD_NONE && effect2 != VOLCMD_NONE)
				{
					m->volcmd = effect2;
					m->vol = effectParam2;
				}
				// Prefer instrument effects over any other effects
				if(effect1 != CMD_NONE)
				{
					m->command = effect1;
					m->param = effectParam1;
				} else if(effect3 != CMD_NONE)
				{
					m->command = effect3;
					m->param = effectParam3;
				}

			} else
			{
				channelCounter[nChn]--;
			}
		}	// End for all channels

		// Now we can try to write tempo information.
		if(tempoChange)
		{
			tempoChange = false;
			
			pSndFile->Patterns[nPat].WriteEffect(EffectWriter(CMD_TEMPO, static_cast<ModCommand::PARAM>(tempo)).Row(nRow).Channel(0).Retry(EffectWriter::rmTryNextRow));
			pSndFile->Patterns[nPat].WriteEffect(EffectWriter(CMD_SPEED, static_cast<ModCommand::PARAM>(speed)).Row(nRow).Retry(EffectWriter::rmTryNextRow));
		}
		// Try to put delay effects somewhere as well
		if(writeDelay & 0xF0)
		{
			pSndFile->Patterns[nPat].WriteEffect(EffectWriter(CMD_S3MCMDEX, 0xE0 | (writeDelay >> 4)).Row(nRow).AllowMultiple().Retry(EffectWriter::rmIgnore));
		}
		if(writeDelay & 0x0F)
		{
			const uint8 param = (writeDelay & 0x0F) * settings.internalTicks / 15;
			pSndFile->Patterns[nPat].WriteEffect(EffectWriter(CMD_S3MCMDEX, 0x60u | Clamp(param, uint8(1), uint8(15))).Row(nRow).AllowMultiple().Retry(EffectWriter::rmIgnore));
		}
		writeDelay = 0;
	}	// End for all rows

	return nPat;

	#undef ASSERT_CAN_READ_PATTERN

}


DWORD ConvertDMFSample(const SAMPLEINDEX nSmp, const LPCBYTE lpStream, const DWORD dwMemLength, const bool isV8, uint8 &sampleFlags, CSoundFile *pSndFile)
//--------------------------------------------------------------------------------------------------------------------------------------------------------
{
	#define ASSERT_CAN_READ_SAMPLE(x) ASSERT_CAN_READ_PROTOTYPE(dwMemPos, dwMemLength, x, return 0);

	DWORD dwMemPos = 0;

	ASSERT_CAN_READ_SAMPLE(1);
	const size_t lenName = lpStream[dwMemPos++];
	ASSERT_CAN_READ_SAMPLE(lenName);
	StringFixer::ReadString<StringFixer::spacePadded>(pSndFile->m_szNames[nSmp], reinterpret_cast<const char *>(lpStream + dwMemPos), lenName);
	dwMemPos += lenName;

	ASSERT_CAN_READ_SAMPLE(sizeof(DMFCHUNK_SAMPLEHEADER));
	DMFCHUNK_SAMPLEHEADER *smpHead = (DMFCHUNK_SAMPLEHEADER *)(lpStream + dwMemPos);
	dwMemPos += sizeof(DMFCHUNK_SAMPLEHEADER);

	ModSample &sample = pSndFile->GetSample(nSmp);
	sample.Initialize();
	sample.nLength = LittleEndian(smpHead->length);
	sample.nSustainEnd = min(sample.nLength, LittleEndian(smpHead->loopEnd));
	sample.nSustainStart = min(sample.nSustainEnd, LittleEndian(smpHead->loopStart));
	if(sample.nSustainEnd > 0)
	{
		sample.nSustainEnd--;
	}

	sample.nC5Speed = LittleEndianW(smpHead->c3freq);
	sample.nGlobalVol = 64;
	if(smpHead->volume)
	{
		sample.nVolume = smpHead->volume + 1;
	} else
	{
		sample.nVolume = 256;
	}
	sampleFlags = smpHead->flags;
	if((sampleFlags & DMFSMP_LOOP) != 0 && sample.nSustainEnd > sample.nSustainStart)
	{
		sample.uFlags |= CHN_SUSTAINLOOP;
	}
	if((sampleFlags & DMFSMP_16BIT) != 0)
	{
		sample.uFlags |= CHN_16BIT;
		sample.nLength /= 2;
		sample.nSustainStart /= 2;
		sample.nSustainEnd /= 2;
	}

	if(isV8)
	{
		// Read library name in version 8 files
		ASSERT_CAN_READ_SAMPLE(8);
		StringFixer::ReadString<StringFixer::spacePadded>(sample.filename, reinterpret_cast<const char *>(lpStream + dwMemPos), 8);
		dwMemPos += 8;
	}

	ASSERT_CAN_READ_SAMPLE(sizeof(DMFCHUNK_SAMPLEHEADERTAIL));
	// We don't care for the checksum of the sample data...
	dwMemPos += sizeof(DMFCHUNK_SAMPLEHEADERTAIL);

	return dwMemPos;

	#undef ASSERT_CAN_READ_SAMPLE

}


bool CSoundFile::ReadDMF(const BYTE *lpStream, const DWORD dwMemLength)
//---------------------------------------------------------------------
{
	#define ASSERT_CAN_READ_CHUNK(x) ASSERT_CAN_READ_PROTOTYPE(dwMemPos, dwChunkEnd, x, break);

	DWORD dwMemPos = 0;

	ASSERT_CAN_READ(sizeof(DMFHEADER));
	DMFHEADER *pHeader = (DMFHEADER *)lpStream;
	if(pHeader->signature != LittleEndian(DMF_DDMF) || !pHeader->version || pHeader->version > 10)
	{
		return false;
	}
	dwMemPos += sizeof(DMFHEADER);

	StringFixer::ReadString<StringFixer::spacePadded>(m_szNames[0], pHeader->songname);
	m_nChannels = 0;

#ifdef MODPLUG_TRACKER
	if(GetpModDoc() != nullptr)
	{
		FileHistory mptHistory;
		MemsetZero(mptHistory);
		mptHistory.loadDate.tm_mday = CLAMP(pHeader->creationDay, 0, 31);
		mptHistory.loadDate.tm_mon = CLAMP(pHeader->creationMonth, 1, 12) - 1;
		mptHistory.loadDate.tm_year = pHeader->creationYear;
		GetpModDoc()->GetFileHistory().clear();
		GetpModDoc()->GetFileHistory().push_back(mptHistory);
	}
#endif // MODPLUG_TRACKER

	vector<uint8> sampleFlags;
	vector<DWORD> patternOffset;
	vector<DWORD> patternLength;

	ORDERINDEX loopStart = 0, loopEnd = ORDERINDEX_INVALID;

	// go through all chunks now
	while(dwMemPos < dwMemLength)
	{
		// Special case: Last 4 bytes should be "ENDE", without a size field (WTF)
		ASSERT_CAN_READ(4);
		if(*(uint32 *)(lpStream + dwMemPos) == LittleEndian(DMF_ENDE))
		{
			break;
		}

		ASSERT_CAN_READ(sizeof(DMF_IFFCHUNK));
		DMF_IFFCHUNK chunkheader = *(DMF_IFFCHUNK *)(lpStream + dwMemPos);
		dwMemPos += sizeof(DMF_IFFCHUNK);

		chunkheader.signature = LittleEndian(chunkheader.signature);
		chunkheader.chunksize = LittleEndian(chunkheader.chunksize);
		ASSERT_CAN_READ(chunkheader.chunksize);

		const DWORD dwChunkEnd = dwMemPos + chunkheader.chunksize;

		switch(chunkheader.signature)
		{
		case DMF_CMSG:	// "CMSG" - Song message
			ASSERT_CAN_READ_CHUNK(1);
			dwMemPos++;	// filler byte
			ReadFixedLineLengthMessage(lpStream + dwMemPos, chunkheader.chunksize - 1, 40, 0);
			break;

		case DMF_SEQU:	// "SEQU" - Order list
			{
				ASSERT_CAN_READ_CHUNK(sizeof(DMFCHUNK_SEQUENCE));
				DMFCHUNK_SEQUENCE *seqHead = (DMFCHUNK_SEQUENCE *)(lpStream + dwMemPos);
				dwMemPos += sizeof(DMFCHUNK_SEQUENCE);

				loopStart = LittleEndianW(seqHead->loopStart);
				loopEnd = LittleEndianW(seqHead->loopEnd);
				const ORDERINDEX numOrders = (ORDERINDEX)min(MAX_ORDERS, (chunkheader.chunksize - sizeof(DMFCHUNK_SEQUENCE)) / 2);
				Order.resize(numOrders, Order.GetInvalidPatIndex());

				for(ORDERINDEX i = 0; i < numOrders; i++, dwMemPos += 2)
				{
					uint16 orderItem = *(uint16 *)(lpStream + dwMemPos);
					Order[i] = (PATTERNINDEX)LittleEndianW(orderItem);
				}
			}
			break;

		case DMF_PATT:	// "PATT" - Pattern data
			if(m_nChannels == 0)
			{
				ASSERT_CAN_READ_CHUNK(sizeof(DMFCHUNK_PATTERNS));
				DMFCHUNK_PATTERNS *patInfo = (DMFCHUNK_PATTERNS *)(lpStream + dwMemPos);
				dwMemPos += sizeof(DMFCHUNK_PATTERNS);
				m_nChannels = CLAMP(patInfo->numTracks, 1, 32) + 1;	// + 1 for global track (used for tempo stuff)

				const PATTERNINDEX numPats = min(MAX_PATTERNS, LittleEndianW(patInfo->numPatterns));
				patternOffset.assign(numPats, 0);
				patternLength.assign(numPats, 0);

				for(PATTERNINDEX nPat = 0; nPat < numPats; nPat++)
				{
					ASSERT_CAN_READ_CHUNK(sizeof(DMFCHUNK_PATTERNHEADER));
					DMFCHUNK_PATTERNHEADER *patHead = (DMFCHUNK_PATTERNHEADER *)(lpStream + dwMemPos);

					patternOffset[nPat] = dwMemPos;
					patternLength[nPat] = sizeof(DMFCHUNK_PATTERNHEADER) + LittleEndian(patHead->patternLength);

					ASSERT_CAN_READ_CHUNK(patternLength[nPat]);
					dwMemPos += patternLength[nPat];
				}
			}
			break;

		case DMF_SMPI:	// "SMPI" - Sample headers
			if(m_nSamples == 0)
			{
				ASSERT_CAN_READ_CHUNK(1);
				m_nSamples = (SAMPLEINDEX)min(MAX_SAMPLES - 1, lpStream[dwMemPos]);
				dwMemPos++;

				sampleFlags.assign(m_nSamples, 0);
				for(SAMPLEINDEX nSmp = 0; nSmp < m_nSamples; nSmp++)
				{
					const DWORD bytesRead = ConvertDMFSample(nSmp + 1, lpStream + dwMemPos, dwChunkEnd - dwMemPos, (pHeader->version >= 8), sampleFlags[nSmp], this);
					if(bytesRead == 0)
					{
						break;
					}
					dwMemPos += bytesRead;
				}

			}
			break;

		case DMF_SMPD:	// "SMPD" - Sample data
			for(SAMPLEINDEX nSmp = 1; nSmp <= m_nSamples; nSmp++)
			{
				ASSERT_CAN_READ_CHUNK(4);
				const uint32 length = LittleEndian(*(uint32 *)(lpStream + dwMemPos));
				dwMemPos += 4;
				ASSERT_CAN_READ_CHUNK(length);

				if(length > 0)
				{
					SampleIO(
						(Samples[nSmp].uFlags & CHN_16BIT) ? SampleIO::_16bit : SampleIO::_8bit,
						SampleIO::mono,
						SampleIO::littleEndian,
						((sampleFlags[nSmp - 1] & DMFSMP_COMPMASK) == DMFSMP_COMP1) ? SampleIO::DMF : SampleIO::signedPCM)
						.ReadSample(Samples[nSmp], (LPCSTR)(lpStream + dwMemPos), length);

					dwMemPos += length;
				}
			}
			break;

		case DMF_SMPJ:	// "SMPJ" - Sample jump points (xtracker32 only)
		case DMF_SETT:	// "Unprintable ID" - those might be GUI settings and other related stuff
			break;

#ifdef DEBUG
		default:
			{
				char s[40];
				const char *sig = (char *)&chunkheader.signature;
				wsprintf(s, "Unknown chunk ID %c%c%c%c at %d\n", sig[0], sig[1], sig[2], sig[3], dwMemPos - sizeof(DMF_IFFCHUNK));
				if(GetpModDoc()) GetpModDoc()->AddToLog(s);
			}
#endif
		}

		dwMemPos = dwChunkEnd;
	}

	if(!patternOffset.empty())
	{
		DMF_PATTERNSETTINGS settings;
		settings.beat = 0;
		settings.tempoTicks = 32;
		settings.tempoBPM = 120;
		settings.realBPMmode = false;
		settings.internalTicks = 6;
		settings.playDir.assign(GetNumChannels(), false);
		settings.noteBuffer.assign(GetNumChannels(), NOTE_NONE);
		settings.lastNote.assign(GetNumChannels(), NOTE_NONE);

		for(ORDERINDEX nOrd = 0; nOrd < Order.GetLength(); nOrd++)
		{
			// Create one pattern for each order item, as the same pattern can be played with different settings
			PATTERNINDEX nPat = Order[nOrd];
			if(nPat < patternOffset.size() && patternOffset[nPat] != 0)
			{
				nPat = ConvertDMFPattern(lpStream + patternOffset[nPat], patternLength[nPat], settings, this);
				Order[nOrd] = nPat;
				// Loop end?
				if(nPat != PATTERNINDEX_INVALID && nOrd == loopEnd && (loopStart > 0 || nOrd < Order.GetLength() - 1))
				{
					Patterns[nPat].WriteEffect(EffectWriter(CMD_POSITIONJUMP, static_cast<ModCommand::PARAM>(loopStart)).Row(Patterns[nPat].GetNumRows() - 1).Retry(EffectWriter::rmTryPreviousRow));
				}
			}
		}
	}

	m_nType = MOD_TYPE_DMF;
	m_SongFlags = SONG_LINEARSLIDES | SONG_ITCOMPATGXX;	// this will be converted to IT format by MPT. SONG_ITOLDEFFECTS is not set because of tremor.
	SetModFlag(MSF_COMPATIBLE_PLAY, true);
	m_nDefaultSpeed = 6;
	m_nDefaultTempo = 120;
	m_nDefaultGlobalVolume = 256;
	m_nSamplePreAmp = m_nVSTiVolume = 48;

	return true;

	#undef ASSERT_CAN_READ_CHUNK
}


///////////////////////////////////////////////////////////////////////
// DMF Compression (from libmodplug)

typedef struct DMF_HNODE
{
	int16 left, right;
	uint8 value;
} DMF_HNODE;

typedef struct DMF_HTREE
{
	const uint8 *ibuf, *ibufmax;
	uint32 bitbuf;
	int bitnum;
	int lastnode, nodecount;
	DMF_HNODE nodes[256];
} DMF_HTREE;


// DMF Huffman ReadBits
BYTE DMFReadBits(DMF_HTREE *tree, UINT nbits)
//-------------------------------------------
{
	uint8 x = 0, bitv = 1;
	while (nbits--)
	{
		if (tree->bitnum)
		{
			tree->bitnum--;
		} else
		{
			tree->bitbuf = (tree->ibuf < tree->ibufmax) ? *(tree->ibuf++) : 0;
			tree->bitnum = 7;
		}
		if (tree->bitbuf & 1) x |= bitv;
		bitv <<= 1;
		tree->bitbuf >>= 1;
	}
	return x;
}

//
// tree: [8-bit value][12-bit index][12-bit index] = 32-bit
//

void DMFNewNode(DMF_HTREE *tree)
//------------------------------
{
	uint8 isleft, isright;
	int actnode;

	actnode = tree->nodecount;
	if (actnode > 255) return;
	tree->nodes[actnode].value = DMFReadBits(tree, 7);
	isleft = DMFReadBits(tree, 1);
	isright = DMFReadBits(tree, 1);
	actnode = tree->lastnode;
	if (actnode > 255) return;
	tree->nodecount++;
	tree->lastnode = tree->nodecount;
	if (isleft)
	{
		tree->nodes[actnode].left = (int16)tree->lastnode;
		DMFNewNode(tree);
	} else
	{
		tree->nodes[actnode].left = -1;
	}
	tree->lastnode = tree->nodecount;
	if (isright)
	{
		tree->nodes[actnode].right = (int16)tree->lastnode;
		DMFNewNode(tree);
	} else
	{
		tree->nodes[actnode].right = -1;
	}
}


int DMFUnpack(LPBYTE psample, const uint8 *ibuf, const uint8 *ibufmax, UINT maxlen)
//---------------------------------------------------------------------------------
{
	DMF_HTREE tree;
	int actnode;
	uint8 value, sign, delta = 0;

	MemsetZero(tree);
	tree.ibuf = ibuf;
	tree.ibufmax = ibufmax;
	DMFNewNode(&tree);
	value = 0;

	for (UINT i = 0; i < maxlen; i++)
	{
		actnode = 0;
		sign = DMFReadBits(&tree, 1);
		do
		{
			if (DMFReadBits(&tree, 1))
				actnode = tree.nodes[actnode].right;
			else
				actnode = tree.nodes[actnode].left;
			if (actnode > 255) break;
			delta = tree.nodes[actnode].value;
			if ((tree.ibuf >= tree.ibufmax) && (!tree.bitnum)) break;
		} while ((tree.nodes[actnode].left >= 0) && (tree.nodes[actnode].right >= 0));
		if (sign) delta ^= 0xFF;
		value += delta;
		psample[i] = (i) ? value : 0;
	}
#ifdef DMFLOG
//	Log("DMFUnpack: %d remaining bytes\n", tree.ibufmax-tree.ibuf);
#endif
	return tree.ibuf - ibuf;
}

