/*
 * Load_mt2.cpp
 * ------------
 * Purpose: MT2 (MadTracker 2) module loader
 * Notes  : A couple of things are not handled properly or not at all, such as internal effects and automation envelopes
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"
#ifdef MPT_EXTERNAL_SAMPLES
// For loading external samples
#include "../common/mptPathString.h"
#endif // MPT_EXTERNAL_SAMPLES
#ifndef NO_VST
#include "../mptrack/Vstplug.h"
#endif // NO_VST

OPENMPT_NAMESPACE_BEGIN

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(push, 1)
#endif

struct PACKED MT2FileHeader
{
	enum MT2HeaderFlags
	{
		packedPatterns		= 0x01,
		automation			= 0x02,
		drumsAutomation		= 0x08,
		masterAutomation	= 0x10,
	};

	char   signature[4];	// "MT20"
	uint32 userID;
	uint16 version;
	char   trackerName[32];	// "MadTracker 2.0"
	char   songName[64];
	uint16 numOrders;
	uint16 restartPos;
	uint16 numPatterns;
	uint16 numChannels;
	uint16 samplesPerTick;
	uint8  ticksPerLine;
	uint8  linesPerBeat;
	uint32 flags;			// See HeaderFlags
	uint16 numInstruments;
	uint16 numSamples;
	uint8  Orders[256];

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(userID);
		SwapBytesLE(version);
		SwapBytesLE(numOrders);
		SwapBytesLE(restartPos);
		SwapBytesLE(numPatterns);
		SwapBytesLE(numChannels);
		SwapBytesLE(samplesPerTick);
		SwapBytesLE(numInstruments);
		SwapBytesLE(numSamples);
	}
};

STATIC_ASSERT(sizeof(MT2FileHeader) == 382);


struct PACKED MT2DrumsData
{
	uint16 numDrumPatterns;
	uint16 DrumSamples[8];
	uint8  DrumPatternOrder[256];

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(numDrumPatterns);
		for(size_t i = 0; i < CountOf(DrumSamples); i++)
		{
			SwapBytesLE(DrumSamples[i]);
		}
	}
};

STATIC_ASSERT(sizeof(MT2DrumsData) == 274);


struct PACKED MT2TrackSettings
{
	uint16 volume;
	uint8  trackfx;		// Built-in effect type is used
	uint8  output;
	uint16 fxID;
	uint16 trackEffectParam[64][8];

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(volume);
		SwapBytesLE(fxID);
		for(size_t i = 0; i < CountOf(trackEffectParam); i++)
		{
			for(size_t j = 0; j < CountOf(trackEffectParam[0]); j++)
			{
				SwapBytesLE(trackEffectParam[i][j]);
			}
		}
	}
};

STATIC_ASSERT(sizeof(MT2TrackSettings) == 1030);


struct PACKED MT2Command
{
	uint8 note;	// 0=nothing, 97=note off
	uint8 instr;
	uint8 vol;
	uint8 pan;
	uint8 fxcmd;
	uint8 fxparam1;
	uint8 fxparam2;
};

STATIC_ASSERT(sizeof(MT2Command) == 7);


struct PACKED MT2EnvPoint
{
	uint16 x;
	uint16 y;

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(x);
		SwapBytesLE(y);
	}
};

STATIC_ASSERT(sizeof(MT2EnvPoint) == 4);


struct PACKED MT2Instrument
{
	enum EnvTypes
	{
		VolumeEnv	= 1,
		PanningEnv	= 2,
		PitchEnv	= 4,
		FilterEnv	= 8,
	};

	uint16 numSamples;
	uint8  groupMap[96];
	uint8  vibtype, vibsweep, vibdepth, vibrate;
	uint16 fadeout;
	uint16 nna;

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(numSamples);
		SwapBytesLE(fadeout);
		SwapBytesLE(nna);
	}
};

STATIC_ASSERT(sizeof(MT2Instrument) == 106);


struct PACKED MT2IEnvelope
{
	uint8 flags;
	uint8 numPoints;
	uint8 sustainPos;
	uint8 loopStart;
	uint8 loopEnd;
	uint8 reserved[3];
	MT2EnvPoint points[16];

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		for(size_t i = 0; i < CountOf(points); i++)
		{
			points[i].ConvertEndianness();
		}
	}
};

STATIC_ASSERT(sizeof(MT2IEnvelope) == 72);


// Note: The order of these fields differs a bit in MTIOModule_MT2.cpp - maybe just typos, I'm not sure.
// This struct follows the save format of MadTracker 2.6.1.
struct PACKED MT2InstrSynth
{
	uint8  synthID;
	uint8  effectID;	// 0 = Lowpass filter, 1 = Highpass filter
	uint16 cutoff;		// 100...11000 Hz
	uint8  resonance;	// 0...128
	uint8  attack;		// 0...128
	uint8  decay;		// 0...128
	uint8  midiChannel;	// 0...15
	int8   device;		// VST slot (positive) or MIDI device (negative)
	int8   unknown1;	// Missing in MTIOModule_MT2.cpp
	uint8  volume;		// 0...255
	int8   finetune;	// -96...96
	int8   transpose;	// -48...48
	uint8  unknown2;	// Seems to be equal to instrument number.
	uint8  unknown3;
	uint8  midiProgram;
	uint8  reserved[16];

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(cutoff);
	}
};

STATIC_ASSERT(sizeof(MT2InstrSynth) == 32);


struct PACKED MT2Sample
{
	uint32 length;
	uint32 frequency;
	uint8  depth;
	uint8  channels;
	uint8  flags;
	uint8  loopType;
	uint32 loopStart;
	uint32 loopEnd;
	uint16 volume;
	int8   panning;
	int8   note;
	int16  spb;

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(length);
		SwapBytesLE(frequency);
		SwapBytesLE(loopStart);
		SwapBytesLE(loopEnd);
		SwapBytesLE(volume);
		SwapBytesLE(spb);
	}
};

STATIC_ASSERT(sizeof(MT2Sample) == 26);


struct PACKED MT2Group
{
	uint8 sample;
	uint8 vol;		// 0...128
	int8  pitch;	// -128...127
	uint8 reserved[5];

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(pitch);
	}
};

STATIC_ASSERT(sizeof(MT2Group) == 8);


struct PACKED MT2VST
{
	char   dll[64];
	char   programName[28];
	uint32 fxID;
	uint32 fxVersion;
	uint32 programNr;
	uint8  useChunks;
	uint8  track;
	int8   pan;				// Not imported - could use pan mix mode for D/W ratio, but this is not implemented for instrument plugins!
	char   reserved[17];
	uint32 n;

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(fxID);
		SwapBytesLE(fxVersion);
		SwapBytesLE(programNr);
	}
};

STATIC_ASSERT(sizeof(MT2VST) == 128);

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(pop)
#endif


static void ConvertMT2Command(CSoundFile *that, ModCommand &m, MT2Command &p)
//---------------------------------------------------------------------------
{
	// Note
	m.note = NOTE_NONE;
	if(p.note) m.note = (p.note > 96) ? NOTE_KEYOFF : (p.note + NOTE_MIN + 11);
	// Instrument
	m.instr = p.instr;
	// Volume Column
	if(p.vol >= 0x10 && p.vol <= 0x90)
	{
		m.volcmd = VOLCMD_VOLUME;
		m.vol = (p.vol - 0x10) / 2;
	} else if(p.vol >= 0xA0 && p.vol <= 0xAF)
	{
		m.volcmd = VOLCMD_VOLSLIDEDOWN;
		m.vol = (p.vol & 0x0f);
	} else if(p.vol >= 0xB0 && p.vol <= 0xBF)
	{
		m.volcmd = VOLCMD_VOLSLIDEUP;
		m.vol = (p.vol & 0x0F);
	} else if(p.vol >= 0xC0 && p.vol <= 0xCF)
		{
		m.volcmd = VOLCMD_FINEVOLDOWN;
		m.vol = (p.vol & 0x0F);
	} else if(p.vol >= 0xD0 && p.vol <= 0xDF)
	{
		m.volcmd = VOLCMD_FINEVOLUP;
		m.vol = (p.vol & 0x0F);
	}

	// Effects
	if(p.fxcmd || p.fxparam1 || p.fxparam2)
	{
		switch(p.fxcmd)
		{
		case 0x00:	// FastTracker effect
			m.command = p.fxparam2;
			m.param = p.fxparam1;
			CSoundFile::ConvertModCommand(m);
#ifdef MODPLUG_TRACKER
			m.Convert(MOD_TYPE_XM, MOD_TYPE_IT, *that);
#else
			MPT_UNREFERENCED_PARAMETER(that);
#endif // MODPLUG_TRACKER
			break;

		case 0x08:	// Panning + Polarity (we can only import panning for now)
			m.command = CMD_PANNING8;
			m.param = p.fxparam1;
			break;

		case 0x10:	// Impulse Tracker effect
			m.command = p.fxparam2;
			m.param = p.fxparam1;
			CSoundFile::S3MConvert(m, true);
			break;

		case 0x20:	// Cutoff + Resonance (we can only import cutoff for now)
			m.command = CMD_MIDI;
			m.param = p.fxparam2 >> 1;
			break;
				
		case 0x22:	// Cutoff + Resonance + Attack + Decay (we can only import cutoff for now)
			m.command = CMD_MIDI;
			m.param = (p.fxparam1 & 0xF0) >> 1;
			break;

		case 0x24:	// Reverse
			m.command = CMD_S3MCMDEX;
			m.param = 0x9F;
			break;

		case 0x80:	// Track volume
			m.command = CMD_CHANNELVOLUME;
			m.param = p.fxparam1 / 4u;
			break;

		case 0x9D:	// Offset + delay
			m.volcmd = CMD_OFFSET;
			m.vol = p.fxparam1 >> 3;
			m.command = CMD_S3MCMDEX;
			m.param = 0xD0 | std::min(p.fxparam2, uint8(0x0F));
			break;

		case 0xCC:	// MIDI CC
			//m.command = CMD_MIDI;
			break;

			// TODO: More MT2 Effects
		}
	}

	if(p.pan)
	{
		if(m.command == CMD_NONE)
		{
			m.command = CMD_PANNING8;
			m.param = p.pan;
		} else if(m.volcmd == VOLCMD_NONE)
		{
			m.volcmd = VOLCMD_PANNING;
			m.vol = p.pan / 4;
		}
	}
}


// This doesn't really do anything but skipping the envelope chunk at the moment.
static void ReadMT2Automation(uint16 version, FileReader &file)
//-------------------------------------------------------------
{
	uint32 flags;
	uint32 trkfxid;
	if(version >= 0x203)
	{
		flags = file.ReadUint32LE();
		trkfxid = file.ReadUint32LE();
	} else
	{
		flags = file.ReadUint16LE();
		trkfxid = file.ReadUint16LE();
	}
	MPT_UNREFERENCED_PARAMETER(trkfxid);
	while(flags != 0)
	{
		if(flags & 1)
		{
			file.Skip(4 + sizeof(MT2EnvPoint) * 64);
		}
		flags >>= 1;
	}
}


bool CSoundFile::ReadMT2(FileReader &file, ModLoadingFlags loadFlags)
//-------------------------------------------------------------------
{
	file.Rewind();
	MT2FileHeader fileHeader;
	if(!file.ReadConvertEndianness(fileHeader)
		|| memcmp(fileHeader.signature, "MT20", 4)
		|| fileHeader.version < 0x200 || fileHeader.version >= 0x300
		|| fileHeader.numChannels < 1 || fileHeader.numChannels > 64
		|| fileHeader.numOrders > 256
		|| fileHeader.numInstruments >= MAX_INSTRUMENTS
		|| fileHeader.numSamples >= MAX_SAMPLES)
	{
		return false;
	} else if(loadFlags == onlyVerifyHeader)
	{
		return true;
	}

	InitializeGlobals(MOD_TYPE_MT2);
	InitializeChannels();
	mpt::String::Read<mpt::String::maybeNullTerminated>(m_madeWithTracker, fileHeader.trackerName);
	mpt::String::Read<mpt::String::maybeNullTerminated>(m_songName, fileHeader.songName);
	m_nChannels = fileHeader.numChannels;
	Order.SetRestartPos(fileHeader.restartPos);
	m_nDefaultSpeed = fileHeader.ticksPerLine;
	if(!m_nDefaultSpeed) m_nDefaultSpeed = 6;
	m_nDefaultTempo.Set(125);
	m_SongFlags = SONG_LINEARSLIDES | SONG_ITCOMPATGXX | SONG_EXFILTERRANGE;
	m_nInstruments = fileHeader.numInstruments;
	m_nSamples = fileHeader.numSamples;
	m_nDefaultRowsPerBeat = fileHeader.linesPerBeat;
	if(!m_nDefaultRowsPerBeat) m_nDefaultRowsPerBeat = 4;
	m_nDefaultRowsPerMeasure = m_nDefaultRowsPerBeat * 4;
	m_nVSTiVolume = 48;
	m_nSamplePreAmp = 48 * 2;	// Double pre-amp because we will halve the volume of all non-drum instruments, because the volume of drum samples can exceed that of normal samples

	if(fileHeader.samplesPerTick > 100 && fileHeader.samplesPerTick < 5000)
	{
		m_nDefaultTempo.SetRaw(Util::muldivr(110250, TEMPO::fractFact, fileHeader.samplesPerTick));
		//m_nDefaultTempo = 44100 * 60 / (m_nDefaultSpeed * m_nDefaultRowsPerBeat * fileHeader.samplesPerTick);
	}
	Order.ReadFromArray(fileHeader.Orders, fileHeader.numOrders);

	FileReader drumData = file.ReadChunk(file.ReadUint16LE());
	FileReader extraData = file.ReadChunk(file.ReadUint32LE());

	const CHANNELINDEX channelsWithoutDrums = m_nChannels;
	const bool hasDrumChannels = drumData.CanRead(sizeof(MT2DrumsData));
	STATIC_ASSERT(MAX_BASECHANNELS >= 64 + 8);
	if(hasDrumChannels)
	{
		m_nChannels += 8;
	}

	// Read patterns
	for(PATTERNINDEX pat = 0; pat < fileHeader.numPatterns; pat++)
	{
		ROWINDEX numRows = file.ReadUint16LE();
		FileReader chunk = file.ReadChunk((file.ReadUint32LE() + 1) & ~1);

		LimitMax(numRows, MAX_PATTERN_ROWS);
		if(!numRows
			|| !(loadFlags & loadPatternData)
			|| !Patterns.Insert(pat, numRows))
		{
			continue;
		}

		if(fileHeader.flags & MT2FileHeader::packedPatterns)
		{
			ROWINDEX row = 0;
			CHANNELINDEX chn = 0;
			while(chunk.CanRead(1))
			{
				MT2Command cmd;

				uint8 infobyte = chunk.ReadUint8();
				uint8 repeatCount = 0;
				if(infobyte == 0xFF)
				{
					repeatCount = chunk.ReadUint8();
					infobyte = chunk.ReadUint8();
				}
				if(infobyte & 0x7F)
				{
					ModCommand *m = Patterns[pat].GetpModCommand(row, chn);
					MemsetZero(cmd);
					if(infobyte & 0x01) cmd.note = chunk.ReadUint8();
					if(infobyte & 0x02) cmd.instr = chunk.ReadUint8();
					if(infobyte & 0x04) cmd.vol = chunk.ReadUint8();
					if(infobyte & 0x08) cmd.pan = chunk.ReadUint8();
					if(infobyte & 0x10) cmd.fxcmd = chunk.ReadUint8();
					if(infobyte & 0x20) cmd.fxparam1 = chunk.ReadUint8();
					if(infobyte & 0x40) cmd.fxparam2 = chunk.ReadUint8();
					ConvertMT2Command(this, *m, cmd);
					const ModCommand &orig = *m;
					const ROWINDEX fillRows = std::min((uint32)repeatCount, (uint32)numRows - (row + 1));
					for(ROWINDEX r = 0; r < fillRows; r++)
					{
						m += GetNumChannels();
						*m = orig;
					}
				}
				row += repeatCount + 1;
				while(row >= numRows) { row -= numRows; chn++; }
				if(chn >= channelsWithoutDrums) break;
			}
		} else
		{
			for(ROWINDEX row = 0; row < numRows; row++)
			{
				ModCommand *m = Patterns[pat].GetRow(row);
				for(CHANNELINDEX chn = 0; chn < channelsWithoutDrums; chn++, m++)
				{
					MT2Command cmd;
					chunk.ReadStruct(cmd);
					ConvertMT2Command(this, *m, cmd);
				}
			}
		}
	}

	// Read extra data
	uint32 numVST = 0;
	std::vector<int8> trackRouting(GetNumChannels(), 0);
	while(extraData.CanRead(8))
	{
		uint32 id = extraData.ReadUint32LE();
		FileReader chunk = extraData.ReadChunk(extraData.ReadUint32LE());

		switch(id)
		{
		case MAGIC4LE('B','P','M','+'):
			MPT_CONSTANT_IF(0)
			{
				double d = chunk.ReadDoubleLE();
				if(fileHeader.samplesPerTick != 0 && d != 0.0)
				{
					m_nDefaultTempo = TEMPO(44100.0 * 60.0 / (m_nDefaultSpeed * m_nDefaultRowsPerBeat * fileHeader.samplesPerTick * d));
				}
			}
			break;

		case MAGIC4LE('T','F','X','M'):
			break;

		case MAGIC4LE('T','R','K','S'):
			m_nSamplePreAmp = chunk.ReadUint16LE() / 256u;	// 131072 is 0dB... I think (that's how MTIOModule_MT2.cpp reads)
			m_nVSTiVolume = m_nSamplePreAmp / 2u;
			for(CHANNELINDEX c = 0; c < GetNumChannels(); c++)
			{
				MT2TrackSettings trackSettings;
				if(chunk.ReadConvertEndianness(trackSettings))
				{
					ChnSettings[c].nVolume = trackSettings.volume >> 10;	// 32768 is 0dB
					trackRouting[c] = trackSettings.output;
					LimitMax(ChnSettings[c].nVolume, uint16(64));
				}
			}
			break;

		case MAGIC4LE('T','R','K','L'):
			for(CHANNELINDEX i = 0; i < m_nChannels && chunk.CanRead(1); i++)
			{
				std::string name;
				chunk.ReadNullString(name);
				mpt::String::Read<mpt::String::spacePadded>(ChnSettings[i].szName, name.c_str(), name.length());
			}
			break;

		case MAGIC4LE('P','A','T','N'):
			for(PATTERNINDEX i = 0; i < fileHeader.numPatterns && chunk.CanRead(1) && Patterns.IsValidIndex(i); i++)
			{
				std::string name;
				chunk.ReadNullString(name);
				Patterns[i].SetName(name);
			}
			break;

		case MAGIC4LE('M','S','G','\0'):
			chunk.Skip(1);	// Show message on startup
			m_songMessage.Read(chunk, chunk.BytesLeft(), SongMessage::leCRLF);
			break;

		case MAGIC4LE('P','I','C','T'):
			break;

		case MAGIC4LE('S','U','M','\0'):
			{
				uint8 summaryMask[6];
				chunk.ReadArray(summaryMask);
				std::string artist;
				chunk.ReadNullString(artist);
				if(artist != "Unregistered")
				{
					m_songArtist = mpt::ToUnicode(mpt::CharsetWindows1252, artist);
				}
			}
			break;

		case MAGIC4LE('T','M','A','P'):
			break;
		case MAGIC4LE('M','I','D','I'):
			break;
		case MAGIC4LE('T','R','E','Q'):
			break;

		case MAGIC4LE('V','S','T','2'):
			numVST = chunk.ReadUint32LE();
#ifndef NO_VST
			if(!(loadFlags & loadPluginData))
			{
				break;
			}
			for(uint32 i = 0; i < std::min(numVST, uint32(MAX_MIXPLUGINS)); i++)
			{
				MT2VST vstHeader;
				if(chunk.ReadConvertEndianness(vstHeader))
				{
					chunk.Skip(64);	// Seems to always contain values 0-15 as uint32? Maybe some MIDI routing info?

					SNDMIXPLUGIN &mixPlug = m_MixPlugins[i];
					mixPlug.Destroy();
					mpt::String::Read<mpt::String::maybeNullTerminated>(mixPlug.Info.szLibraryName, vstHeader.dll);
					mpt::String::Read<mpt::String::maybeNullTerminated>(mixPlug.Info.szName, vstHeader.programName);
					const size_t len = strlen(mixPlug.Info.szLibraryName);
					if(len > 4 && mixPlug.Info.szLibraryName[len - 4] == '.')
					{
						// Remove ".dll" from library name
						mixPlug.Info.szLibraryName[len - 4] = '\0';
					}
					mixPlug.Info.dwPluginId1 = kEffectMagic;
					mixPlug.Info.dwPluginId2 = vstHeader.fxID;
					if(vstHeader.track >= m_nChannels)
					{
						mixPlug.SetMasterEffect(true);
					} else
					{
						if(!ChnSettings[vstHeader.track].nMixPlugin)
						{
							ChnSettings[vstHeader.track].nMixPlugin = static_cast<PLUGINDEX>(i + 1);
						} else
						{
							// Channel already has plugin assignment - chain the plugins
							PLUGINDEX outPlug = ChnSettings[vstHeader.track].nMixPlugin - 1;
							while(true)
							{
								if(m_MixPlugins[outPlug].GetOutputPlugin() == PLUGINDEX_INVALID)
								{
									m_MixPlugins[outPlug].SetOutputPlugin(static_cast<PLUGINDEX>(i));
									break;
								}
								outPlug = m_MixPlugins[outPlug].GetOutputPlugin();
							}
						}
					}

					// Read plugin settings
					if(vstHeader.useChunks)
					{
						// MT2 only ever calls effGetChunk for programs, and OpenMPT uses the defaultProgram value to determine
						// whether it should use effSetChunk for programs or banks...
						mixPlug.defaultProgram = -1;
						LimitMax(vstHeader.n, Util::MaxValueOfType(mixPlug.nPluginDataSize) - 4);
						mixPlug.nPluginDataSize = vstHeader.n + 4;
					} else
					{
						mixPlug.defaultProgram = vstHeader.programNr;
						LimitMax(vstHeader.n, (Util::MaxValueOfType(mixPlug.nPluginDataSize) / 4u) - 1);
						mixPlug.nPluginDataSize = vstHeader.n * 4 + 4;
					}
					mixPlug.pPluginData = new (std::nothrow) char[mixPlug.nPluginDataSize];
					if(mixPlug.pPluginData != nullptr)
					{
						if(vstHeader.useChunks)
						{
							memcpy(mixPlug.pPluginData, "fEvN", 4);	// 'NvEf' plugin data type
							chunk.ReadRaw(mixPlug.pPluginData + 4, vstHeader.n);
						} else
						{
							float32 *f = reinterpret_cast<float32 *>(mixPlug.pPluginData);
							*(f++) = 0;	// Plugin data type
							for(uint32 param = 0; param < vstHeader.n; param++, f++)
							{
								*f = chunk.ReadFloatLE();
							}
						}
					}
				} else
				{
					break;
				}
			}
#endif // NO_VST
			break;
		}
	}

	// Now that we have both the track settings and plugins, establish the track routing by applying the same plugins to the source track as to the target track:
	for(CHANNELINDEX c = 0; c < GetNumChannels(); c++)
	{
		int8 outTrack = trackRouting[c];
		if(outTrack > c && outTrack < GetNumChannels() && ChnSettings[outTrack].nMixPlugin != 0)
		{
			if(ChnSettings[c].nMixPlugin == 0)
			{
				ChnSettings[c].nMixPlugin = ChnSettings[outTrack].nMixPlugin;
			} else
			{
				PLUGINDEX outPlug = ChnSettings[c].nMixPlugin - 1;
				while(true)
				{
					if(m_MixPlugins[outPlug].GetOutputPlugin() == PLUGINDEX_INVALID)
					{
						m_MixPlugins[outPlug].SetOutputPlugin(ChnSettings[outTrack].nMixPlugin - 1);
						break;
					}
					outPlug = m_MixPlugins[outPlug].GetOutputPlugin();
				}
			}
		}
	}

	// Read drum channels
	INSTRUMENTINDEX drumMap[8] = { 0 };
	uint16 drumSample[8] = { 0 };
	if(hasDrumChannels)
	{
		MT2DrumsData drumHeader;
		drumData.ReadConvertEndianness(drumHeader);

		// Allocate some instruments to handle the drum samples
		for(INSTRUMENTINDEX i = 0; i < 8; i++)
		{
			drumMap[i] = GetNextFreeInstrument(m_nInstruments + 1);
			drumSample[i] = drumHeader.DrumSamples[i];
			if(drumMap[i] != INSTRUMENTINDEX_INVALID)
			{
				m_nInstruments = drumMap[i];
				ModInstrument *mptIns = AllocateInstrument(drumMap[i], drumHeader.DrumSamples[i] + 1);
				if(mptIns != nullptr)
				{
					strcpy(mptIns->name, "Drum #x");
					mptIns->name[6] = '1' + char(i);
				}
			} else
			{
				drumMap[i] = 0;
			}
		}

		// Get all the drum pattern chunks
		std::vector<FileReader> patternChunks(drumHeader.numDrumPatterns);
		for(uint32 pat = 0; pat < drumHeader.numDrumPatterns; pat++)
		{
			uint16 numRows = file.ReadUint16LE();
			patternChunks[pat] = file.ReadChunk(numRows * 32);
		}

		std::vector<PATTERNINDEX> patMapping(fileHeader.numPatterns, PATTERNINDEX_INVALID);
		for(uint32 ord = 0; ord < fileHeader.numOrders; ord++)
		{
			if(drumHeader.DrumPatternOrder[ord] >= drumHeader.numDrumPatterns || Order[ord] >= fileHeader.numPatterns)
				continue;

			// Figure out where to write this drum pattern
			PATTERNINDEX writePat = Order[ord];
			if(patMapping[writePat] == PATTERNINDEX_INVALID)
			{
				patMapping[writePat] = drumHeader.DrumPatternOrder[ord];
			} else if(patMapping[writePat] != drumHeader.DrumPatternOrder[ord])
			{
				// Damn, this pattern has previously used a different drum pattern. Duplicate it...
				PATTERNINDEX newPat = Patterns.Duplicate(writePat);
				if(newPat != PATTERNINDEX_INVALID)
				{
					writePat = newPat;
					Order[ord] = writePat;
				}
			}
			if(!Patterns.IsValidPat(writePat))
				continue;

			FileReader &chunk = patternChunks[drumHeader.DrumPatternOrder[ord]];
			chunk.Rewind();
			const ROWINDEX numRows = static_cast<ROWINDEX>(chunk.GetLength() / 32u);
			for(ROWINDEX row = 0; row < Patterns[writePat].GetNumRows(); row++)
			{
				ModCommand *m = Patterns[writePat].GetpModCommand(row, m_nChannels - 8);
				for(CHANNELINDEX chn = 0; chn < 8; chn++, m++)
				{
					*m = ModCommand::Empty();
					if(row >= numRows)
						continue;

					uint8 drums[4];
					chunk.ReadArray(drums);
					if(drums[0] & 0x80)
					{
						m->note = NOTE_MIDDLEC;
						m->instr = static_cast<ModCommand::INSTR>(drumMap[chn]);
						uint8 delay = drums[0] & 0x1F;
						if(delay)
						{
							LimitMax(delay, uint8(0x0F));
							m->command = CMD_S3MCMDEX;
							m->param = 0xD0 | delay;
						}
						m->volcmd = VOLCMD_VOLUME;
						// Volume is 0...255, but 128 is equivalent to v64 - we compensate this by halving the global volume of all non-drum instruments
						m->vol = static_cast<ModCommand::VOL>((static_cast<uint16>(drums[1]) + 3) / 4u);
					}
				}
			}
		}
	}

	// Read automation envelopes
	if(fileHeader.flags & MT2FileHeader::automation)
	{
		for(uint32 pat = 0; pat < fileHeader.numPatterns; pat++)
		{
			const uint32 numEnvelopes = ((fileHeader.flags & MT2FileHeader::drumsAutomation) ? m_nChannels : channelsWithoutDrums)
				+ numVST
				+ ((fileHeader.flags & MT2FileHeader::masterAutomation) ? 1 : 0);
			for(uint32 env = 0; env < numEnvelopes && file.CanRead(4); env++)
			{
				// TODO
				ReadMT2Automation(fileHeader.version, file);
			}
		}
	}

	// Read instruments
	std::vector<FileReader> instrChunks(255);
	for(INSTRUMENTINDEX i = 0; i < 255; i++)
	{
		char instrName[32];
		file.ReadArray(instrName);
		uint32 dataLength = file.ReadUint32LE();
		if(dataLength == 32) dataLength += 108 + sizeof(MT2IEnvelope) * 4;
		if(fileHeader.version > 0x0201 && dataLength) dataLength += 4;

		FileReader instrChunk = instrChunks[i] = file.ReadChunk(dataLength);

		ModInstrument *mptIns = nullptr;
		if(i < fileHeader.numInstruments)
		{
			mptIns = AllocateInstrument(i + 1);
		}
		if(mptIns == nullptr)
			continue;

		mpt::String::Read<mpt::String::maybeNullTerminated>(mptIns->name, instrName);

		if(!dataLength)
			continue;

		MT2Instrument insHeader;
		instrChunk.ReadConvertEndianness(insHeader);
		uint16 flags = 0;
		if(fileHeader.version >= 0x0201) flags = instrChunk.ReadUint16LE();
		uint32 envMask = MT2Instrument::VolumeEnv | MT2Instrument::PanningEnv;
		if(fileHeader.version >= 0x0202) envMask = instrChunk.ReadUint32LE();

		mptIns->nFadeOut = insHeader.fadeout;
		const uint8 NNA[4] = { NNA_NOTECUT, NNA_CONTINUE, NNA_NOTEOFF, NNA_NOTEFADE };
		const uint8 DCT[4] = { DCT_NONE, DCT_NOTE, DCT_SAMPLE, DCT_INSTRUMENT };
		const uint8 DNA[4] = { DNA_NOTECUT, DNA_NOTEFADE /* actually continue, but IT doesn't have that */, DNA_NOTEOFF, DNA_NOTEFADE };
		mptIns->nNNA = NNA[insHeader.nna & 3];
		mptIns->nDCT = DCT[(insHeader.nna >> 8) & 3];
		mptIns->nDNA = DNA[(insHeader.nna >> 12) & 3];

		// Load envelopes
		for(uint32 env = 0; env < 4; env++)
		{
			if(envMask & 1)
			{
				MT2IEnvelope mt2Env;
				instrChunk.ReadConvertEndianness(mt2Env);

				const EnvelopeType envType[4] = { ENV_VOLUME, ENV_PANNING, ENV_PITCH, ENV_PITCH };
				InstrumentEnvelope &mptEnv = mptIns->GetEnvelope(envType[env]);

				mptEnv.dwFlags.set(ENV_FILTER, (env == 3) && (mt2Env.flags & 1) != 0);
				mptEnv.dwFlags.set(ENV_ENABLED, (mt2Env.flags & 1) != 0);
				mptEnv.dwFlags.set(ENV_SUSTAIN, (mt2Env.flags & 2) != 0);
				mptEnv.dwFlags.set(ENV_LOOP, (mt2Env.flags & 4) != 0);
				mptEnv.nNodes = mt2Env.numPoints;
				LimitMax(mptEnv.nNodes, 16u);
				mptEnv.nSustainStart = mptEnv.nSustainEnd = mt2Env.sustainPos;
				mptEnv.nLoopStart = mt2Env.loopStart;
				mptEnv.nLoopEnd = mt2Env.loopEnd;

				for(uint32 i = 0; i < mptEnv.nNodes; i++)
				{
					mptEnv.Ticks[i] = mt2Env.points[i].x;
					mptEnv.Values[i] = static_cast<uint8>(Clamp(mt2Env.points[i].y, uint16(0), uint16(64)));
				}
			}
			envMask >>= 1;
		}
		if(!mptIns->VolEnv.dwFlags[ENV_ENABLED] && mptIns->nNNA != NNA_NOTEFADE)
		{
			mptIns->nFadeOut = int16_max;
		}

		mptIns->SetCutoff(0x7F, true);
		mptIns->SetResonance(0, true);

		if(flags)
		{
			MT2InstrSynth synthData;
			instrChunk.ReadConvertEndianness(synthData);

			// Translate frequency back to extended IT cutoff factor
			float cutoff = 28.8539f * std::log(0.00764451f * synthData.cutoff);
			Limit(cutoff, 0.0f, 127.0f);
			if(flags & 2)
			{
				mptIns->SetCutoff(Util::Round<uint8>(cutoff), true);
				mptIns->SetResonance(synthData.resonance, true);
			}
			mptIns->nFilterMode = synthData.effectID == 1 ? FLTMODE_HIGHPASS : FLTMODE_LOWPASS;
			if(flags & 4)
			{
				// VSTi / MIDI synth enabled
				mptIns->nMidiChannel = synthData.midiChannel + 1;
				mptIns->nMixPlug = static_cast<PLUGINDEX>(synthData.device + 1);
				if(synthData.device < 0)
				{
					// TODO: This is a MIDI device - maybe use MIDI I/O plugin to emulate those?
					mptIns->nMidiProgram = synthData.midiProgram + 1;	// MT2 only seems to use this for MIDI devices, not VSTis!
				}
				if(synthData.transpose)
				{
					for(uint32 i = 0; i < CountOf(mptIns->NoteMap); i++)
					{
						int note = NOTE_MIN + i + synthData.transpose;
						Limit(note, NOTE_MIN, NOTE_MAX);
						mptIns->NoteMap[i] = static_cast<uint8>(note);
					}
				}
			}
		}
	}

	// Read sample headers
	std::bitset<256> sampleNoInterpolation;
	std::bitset<256> sampleSynchronized;
	for(SAMPLEINDEX i = 0; i < 256; i++)
	{
		char sampleName[32];
		file.ReadArray(sampleName);
		uint32 dataLength = file.ReadUint32LE();

		FileReader sampleChunk = file.ReadChunk(dataLength);

		if(i < fileHeader.numSamples)
		{
			mpt::String::Read<mpt::String::maybeNullTerminated>(m_szNames[i + 1], sampleName);
		}

		if(dataLength && i < fileHeader.numSamples)
		{
			ModSample &mptSmp = Samples[i + 1];
			mptSmp.Initialize(MOD_TYPE_IT);
			MT2Sample sampleHeader;
			sampleChunk.ReadConvertEndianness(sampleHeader);

			mptSmp.nLength = sampleHeader.length;
			mptSmp.nC5Speed = sampleHeader.frequency;
			if(sampleHeader.depth > 1) { mptSmp.uFlags.set(CHN_16BIT); mptSmp.nLength /= 2u; }
			if(sampleHeader.channels > 1) { mptSmp.uFlags.set(CHN_STEREO); mptSmp.nLength /= 2u; }
			if(sampleHeader.loopType == 1) mptSmp.uFlags.set(CHN_LOOP);
			else if(sampleHeader.loopType == 2) mptSmp.uFlags.set(CHN_LOOP | CHN_PINGPONGLOOP);
			mptSmp.nLoopStart = sampleHeader.loopStart;
			mptSmp.nLoopEnd = sampleHeader.loopEnd;
			mptSmp.nVolume = sampleHeader.volume >> 7;
			if(sampleHeader.panning == -128)
				mptSmp.uFlags.set(CHN_SURROUND);
			else
				mptSmp.nPan = sampleHeader.panning + 128;
			mptSmp.uFlags.set(CHN_PANNING);
			mptSmp.RelativeTone = sampleHeader.note;

			if(sampleHeader.flags & 2)
			{
				// Sample is synchronized to beat
				// The synchronization part is not supported in OpenMPT, but synchronized samples also always play at the same pitch as C-5, which we CAN do!
				sampleSynchronized[i] = true;
				//mptSmp.nC5Speed = Util::muldiv(mptSmp.nC5Speed, sampleHeader.spb, 22050);
			}
			if(sampleHeader.flags & 5)
			{
				// External sample
				mptSmp.uFlags.set(SMP_KEEPONDISK);
			}
			if(sampleHeader.flags & 8)
			{
				sampleNoInterpolation[i] = true;
				for(INSTRUMENTINDEX drum = 0; drum < 8; drum++)
				{
					if(drumSample[drum] == i && Instruments[drumMap[drum]] != nullptr)
					{
						Instruments[drumMap[drum]]->nResampling = SRCMODE_NEAREST;
					}
				}
			}
		}
	}

	// Read sample groups
	for(INSTRUMENTINDEX ins = 0; ins < 255; ins++)
	{
		ModInstrument *mptIns = Instruments[ins + 1];
		if(instrChunks[ins].GetLength())
		{
			FileReader &chunk = instrChunks[ins];
			MT2Instrument insHeader;
			chunk.Rewind();
			chunk.ReadConvertEndianness(insHeader);

			std::vector<MT2Group> groups(insHeader.numSamples);
			for(uint32 grp = 0; grp < insHeader.numSamples; grp++)
			{
				file.ReadConvertEndianness(groups[grp]);
			}

			// Instruments with plugin assignments never play samples at the same time!
			if(mptIns != nullptr && mptIns->nMixPlug == 0)
			{
				mptIns->nGlobalVol = 32;	// Compensate for extended dynamic range of drum instruments
				for(uint32 note = 0; note < 96; note++)
				{
					if(insHeader.groupMap[note] < insHeader.numSamples)
					{
						const MT2Group &group = groups[insHeader.groupMap[note]];
						SAMPLEINDEX sample = group.sample + 1;
						mptIns->Keyboard[note + 11 + NOTE_MIN] = sample;
						if(sample > 0 && sample <= m_nSamples)
						{
							ModSample &mptSmp = Samples[sample];
							mptSmp.nVibType = insHeader.vibtype;
							mptSmp.nVibSweep = insHeader.vibsweep;
							mptSmp.nVibDepth = insHeader.vibdepth;
							mptSmp.nVibRate = insHeader.vibrate;
							mptSmp.nGlobalVol = uint16(group.vol) * 2;
							mptSmp.nFineTune = group.pitch;
							if(sampleNoInterpolation[sample - 1])
							{
								mptIns->nResampling = SRCMODE_NEAREST;
							}
							if(sampleSynchronized[sample - 1])
							{
								mptIns->NoteMap[note + 11 + NOTE_MIN] = NOTE_MIDDLEC;
							}
						}
						// TODO: volume, finetune for duplicated samples
					}
				}
			}
		}
	}

	if(!(loadFlags & loadSampleData))
		return true;

	// Read sample data
	for(SAMPLEINDEX i = 0; i < m_nSamples; i++)
	{
		ModSample &mptSmp = Samples[i + 1];
		const uint32 freq = Util::Round<uint32>(mptSmp.nC5Speed * std::pow(2.0, -(mptSmp.RelativeTone - 49 - (mptSmp.nFineTune / 128.0)) / 12.0));

		if(!mptSmp.uFlags[SMP_KEEPONDISK])
		{
			SampleIO(
				mptSmp.uFlags[CHN_16BIT] ? SampleIO::_16bit : SampleIO::_8bit,
				mptSmp.uFlags[CHN_STEREO] ? SampleIO::stereoSplit : SampleIO::mono,
				SampleIO::littleEndian,
				SampleIO::MT2)
				.ReadSample(mptSmp, file);
		} else
		{
			// External sample
			const uint32 filenameSize = file.ReadUint32LE();
			file.Skip(12); // Reserved
			std::string filename;
			file.ReadString<mpt::String::maybeNullTerminated>(filename, filenameSize);
			mpt::String::Copy(mptSmp.filename, filename);

#ifdef MPT_EXTERNAL_SAMPLES
			if(filename.length() >= 2
				&& filename.at(0) != '\\'	// Relative path on same drive
				&& filename.at(1) != ':')	// Absolute path
			{
				// Relative path in same folder or sub folder
				filename = ".\\" + filename;
			}
			SetSamplePath(i + 1, mpt::PathString::FromLocaleSilent(filename));
#else
			AddToLog(LogWarning, mpt::String::Print(MPT_USTRING("Loading external sample %1 ('%2') failed: External samples are not supported."), i, mpt::ToUnicode(GetCharsetLocaleOrModule(), filename)));
#endif // MPT_EXTERNAL_SAMPLES
		}
		mptSmp.nC5Speed = freq;
		mptSmp.nFineTune = 0;
		mptSmp.RelativeTone = 0;
	}

	return true;
}

OPENMPT_NAMESPACE_END
