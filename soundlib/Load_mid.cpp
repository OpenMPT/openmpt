/*
 * Load_mid.cpp
 * ------------
 * Purpose: MIDI file loader
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"
#include "Dlsbank.h"
#include "MIDIEvents.h"
#ifdef MODPLUG_TRACKER
#include "../mptrack/TrackerSettings.h"
#include "../mptrack/Moddoc.h"
#include "../mptrack/Mptrack.h"
#include "../common/mptFileIO.h"
#endif // MODPLUG_TRACKER

OPENMPT_NAMESPACE_BEGIN

#ifdef MODPLUG_TRACKER

#define MIDI_DRUMCHANNEL	10

extern const char *szMidiGroupNames[17] =
{
	"Piano",
	"Chromatic Percussion",
	"Organ",
	"Guitar",
	"Bass",
	"Strings",
	"Ensemble",
	"Brass",
	"Reed",
	"Pipe",
	"Synth Lead",
	"Synth Pad",
	"Synth Effects",
	"Ethnic",
	"Percussive",
	"Sound Effects",
	"Percussions"
};


extern const char *szMidiProgramNames[128] =
{
	// 1-8: Piano
	"Acoustic Grand Piano",
	"Bright Acoustic Piano",
	"Electric Grand Piano",
	"Honky-tonk Piano",
	"Electric Piano 1",
	"Electric Piano 2",
	"Harpsichord",
	"Clavi",
	// 9-16: Chromatic Percussion
	"Celesta",
	"Glockenspiel",
	"Music Box",
	"Vibraphone",
	"Marimba",
	"Xylophone",
	"Tubular Bells",
	"Dulcimer",
	// 17-24: Organ
	"Drawbar Organ",
	"Percussive Organ",
	"Rock Organ",
	"Church Organ",
	"Reed Organ",
	"Accordion",
	"Harmonica",
	"Tango Accordion",
	// 25-32: Guitar
	"Acoustic Guitar (nylon)",
	"Acoustic Guitar (steel)",
	"Electric Guitar (jazz)",
	"Electric Guitar (clean)",
	"Electric Guitar (muted)",
	"Overdriven Guitar",
	"Distortion Guitar",
	"Guitar harmonics",
	// 33-40   Bass
	"Acoustic Bass",
	"Electric Bass (finger)",
	"Electric Bass (pick)",
	"Fretless Bass",
	"Slap Bass 1",
	"Slap Bass 2",
	"Synth Bass 1",
	"Synth Bass 2",
	// 41-48   Strings
	"Violin",
	"Viola",
	"Cello",
	"Contrabass",
	"Tremolo Strings",
	"Pizzicato Strings",
	"Orchestral Harp",
	"Timpani",
	// 49-56   Ensemble
	"String Ensemble 1",
	"String Ensemble 2",
	"SynthStrings 1",
	"SynthStrings 2",
	"Choir Aahs",
	"Voice Oohs",
	"Synth Voice",
	"Orchestra Hit",
	// 57-64   Brass
	"Trumpet",
	"Trombone",
	"Tuba",
	"Muted Trumpet",
	"French Horn",
	"Brass Section",
	"SynthBrass 1",
	"SynthBrass 2",
	// 65-72   Reed
	"Soprano Sax",
	"Alto Sax",
	"Tenor Sax",
	"Baritone Sax",
	"Oboe",
	"English Horn",
	"Bassoon",
	"Clarinet",
	// 73-80   Pipe
	"Piccolo",
	"Flute",
	"Recorder",
	"Pan Flute",
	"Blown Bottle",
	"Shakuhachi",
	"Whistle",
	"Ocarina",
	// 81-88   Synth Lead
	"Lead 1 (square)",
	"Lead 2 (sawtooth)",
	"Lead 3 (calliope)",
	"Lead 4 (chiff)",
	"Lead 5 (charang)",
	"Lead 6 (voice)",
	"Lead 7 (fifths)",
	"Lead 8 (bass + lead)",
	// 89-96   Synth Pad
	"Pad 1 (new age)",
	"Pad 2 (warm)",
	"Pad 3 (polysynth)",
	"Pad 4 (choir)",
	"Pad 5 (bowed)",
	"Pad 6 (metallic)",
	"Pad 7 (halo)",
	"Pad 8 (sweep)",
	// 97-104  Synth Effects
	"FX 1 (rain)",
	"FX 2 (soundtrack)",
	"FX 3 (crystal)",
	"FX 4 (atmosphere)",
	"FX 5 (brightness)",
	"FX 6 (goblins)",
	"FX 7 (echoes)",
	"FX 8 (sci-fi)",
	// 105-112 Ethnic
	"Sitar",
	"Banjo",
	"Shamisen",
	"Koto",
	"Kalimba",
	"Bag pipe",
	"Fiddle",
	"Shanai",
	// 113-120 Percussive
	"Tinkle Bell",
	"Agogo",
	"Steel Drums",
	"Woodblock",
	"Taiko Drum",
	"Melodic Tom",
	"Synth Drum",
	"Reverse Cymbal",
	// 121-128 Sound Effects
	"Guitar Fret Noise",
	"Breath Noise",
	"Seashore",
	"Bird Tweet",
	"Telephone Ring",
	"Helicopter",
	"Applause",
	"Gunshot"
};


// Notes 25-85
extern const char *szMidiPercussionNames[61] =
{
	"Seq Click",
	"Brush Tap",
	"Brush Swirl",
	"Brush Slap",
	"Brush Swirl W/Attack",
	"Snare Roll",
	"Castanet",
	"Snare Lo",
	"Sticks",
	"Bass Drum Lo",
	"Open Rim Shot",
	"Acoustic Bass Drum",
	"Bass Drum 1",
	"Side Stick",
	"Acoustic Snare",
	"Hand Clap",
	"Electric Snare",
	"Low Floor Tom",
	"Closed Hi Hat",
	"High Floor Tom",
	"Pedal Hi-Hat",
	"Low Tom",
	"Open Hi-Hat",
	"Low-Mid Tom",
	"Hi Mid Tom",
	"Crash Cymbal 1",
	"High Tom",
	"Ride Cymbal 1",
	"Chinese Cymbal",
	"Ride Bell",
	"Tambourine",
	"Splash Cymbal",
	"Cowbell",
	"Crash Cymbal 2",
	"Vibraslap",
	"Ride Cymbal 2",
	"Hi Bongo",
	"Low Bongo",
	"Mute Hi Conga",
	"Open Hi Conga",
	"Low Conga",
	"High Timbale",
	"Low Timbale",
	"High Agogo",
	"Low Agogo",
	"Cabasa",
	"Maracas",
	"Short Whistle",
	"Long Whistle",
	"Short Guiro",
	"Long Guiro",
	"Claves",
	"Hi Wood Block",
	"Low Wood Block",
	"Mute Cuica",
	"Open Cuica",
	"Mute Triangle",
	"Open Triangle",
	"Shaker",
	"Jingle Bell",
	"Bell Tree",
};


////////////////////////////////////////////////////////////////////////////////
// Maps a midi instrument - returns the instrument number in the file
uint32 CSoundFile::MapMidiInstrument(uint8 program, uint16 bank, uint8 midiChannel, uint8 note)
//---------------------------------------------------------------------------------------------
{
	ModInstrument *pIns;
	program &= 0x7F;
	bank &= 0x3FFF;;
	note &= 0x7F;

	for (uint32 i = 1; i <= m_nInstruments; i++) if (Instruments[i])
	{
		ModInstrument *p = Instruments[i];
		// Drum Kit ?
		if (midiChannel == MIDI_DRUMCHANNEL)
		{
			if (note == p->nMidiDrumKey) return i;
		} else
		// Melodic Instrument
		{
			if (program + 1 == p->nMidiProgram && p->nMidiDrumKey == 0) return i;
		}
	}
	if ((m_nInstruments + 1 >= MAX_INSTRUMENTS) || (m_nSamples + 1 >= MAX_SAMPLES)) return 0;
	
	pIns = AllocateInstrument(m_nInstruments + 1);
	if(pIns == nullptr)
	{
		return 0;
	}

	m_nSamples++;
	m_nInstruments++;
	pIns->wMidiBank = bank;
	pIns->nMidiProgram = program + 1;
	pIns->nMidiChannel = midiChannel;
	if (midiChannel == MIDI_DRUMCHANNEL) pIns->nMidiDrumKey = note;
	pIns->nFadeOut = 1024;
	pIns->nNNA = NNA_NOTEOFF;
	pIns->nDCT = (midiChannel == MIDI_DRUMCHANNEL) ? DCT_SAMPLE : DCT_NOTE;
	pIns->nDNA = DNA_NOTEFADE;
	for (uint32 j=0; j<NOTE_MAX; j++)
	{
		int mapnote = j + 1;
		if (midiChannel == MIDI_DRUMCHANNEL)
		{
			mapnote = NOTE_MIDDLEC;
			/*mapnote = 61 + j - nNote;
			if (mapnote < 1) mapnote = 1;
			if (mapnote > 120) mapnote = 120;*/
		}
		pIns->Keyboard[j] = m_nSamples;
		pIns->NoteMap[j] = (uint8)mapnote;
	}
	pIns->VolEnv.dwFlags.set(ENV_ENABLED);
	if (midiChannel != MIDI_DRUMCHANNEL) pIns->VolEnv.dwFlags.set(ENV_SUSTAIN);
	pIns->VolEnv.reserve(4);
	pIns->VolEnv.push_back(EnvelopeNode(0, ENVELOPE_MAX));
	pIns->VolEnv.push_back(EnvelopeNode(10, ENVELOPE_MAX));
	pIns->VolEnv.push_back(EnvelopeNode(15, (ENVELOPE_MAX + ENVELOPE_MID) / 2));
	pIns->VolEnv.push_back(EnvelopeNode(20, ENVELOPE_MIN));
	pIns->VolEnv.nSustainStart = pIns->VolEnv.nSustainEnd = 1;
	// Sample
	Samples[m_nSamples].nPan = 128;
	Samples[m_nSamples].nVolume = 256;
	Samples[m_nSamples].nGlobalVol = 64;
	if (midiChannel != MIDI_DRUMCHANNEL)
	{
		// GM Midi Name
		strcpy(pIns->name, szMidiProgramNames[program]);
		strcpy(m_szNames[m_nSamples], szMidiProgramNames[program]);
	} else
	{
		strcpy(pIns->name, "Percussions");
		if ((note >= 24) && (note <= 84))
			strcpy(m_szNames[m_nSamples], szMidiPercussionNames[note-24]);
		else
			strcpy(m_szNames[m_nSamples], "Percussions");
	}
	return m_nInstruments;
}


struct MThd
{
	uint32be headerLength;
	uint16be format;		// 0 = single-track, 1 = multi-track, 2 = multi-song
	uint16be numTracks;		// Number of track chunks
	int16be  division;		// Delta timing value: positive = units/beat; negative = smpte compatible units (?)
};

MPT_BINARY_STRUCT(MThd, 10);


typedef uint32 tick_t;
static const CHANNELINDEX PATTERN_CHANNELS_PER_MIDI_CHANNEL = 7;
STATIC_ASSERT(16 * PATTERN_CHANNELS_PER_MIDI_CHANNEL + 2 <= MAX_BASECHANNELS);

struct TrackState
{
	FileReader track;
	tick_t nextEvent;
	uint8 command;
	bool finished;

	TrackState()
		: nextEvent(0)
		, command(0)
		, finished(false)
	{ }
};

struct ModChannelState
{
	tick_t age;
	int32 porta;
	uint8 vol;
	uint8 pan;
	uint8 instr;
	uint8 midiCh;
	ModCommand::NOTE note;

	ModChannelState()
		: age(0)
		, porta(0)
		, vol(127)
		, pan(128)
		, instr(0)
		, midiCh(0xFF)
		, note(NOTE_NONE)
	{ }
};

struct MidiChannelState
{
	uint16 pitchbend;	// 0...16383
	uint16 bank;		// 0...16383
	uint8 program;		// 0...127
	// -- Controllers ------------- function ---------- CC# --- range  ---- init (midi) ---
	uint8 pan;             // Channel Panning           CC10    [0-255]     128  (64)
	uint8 expression;      // Channel Expression        CC11    0-128       128  (127)
	uint8 volume;          // Channel Volume            CC7     0-128       80   (100)
	uint8 modulation;      // Modulation                CC1     0-127       0
	uint8 pitchBendRange;  // Pitch Bend Range                              2
	bool  monoMode;        // Mono/Poly operation       CC126/127           Poly
	uint8 rpnState;	// Current state of RPN CCs

	CHANNELINDEX noteOn[128];	// Value != CHANNELINDEX_INVALID: Note is active and mapped to mod channel in value

	MidiChannelState()
		: pitchbend(0x2000)
		, bank(0)
		, program(0)
		, pan(128)
		, expression(128)
		, volume(80)
		, modulation(0)
		, pitchBendRange(2)
		, rpnState(0)
		, monoMode(false)
	{
		for(size_t i = 0; i < CountOf(noteOn); i++)
		{
			noteOn[i] = CHANNELINDEX_INVALID;
		}
	}
};


static CHANNELINDEX FindUnusedChannel(uint8 midiCh, ModCommand::NOTE note, const std::vector<ModChannelState> &channels, bool monoMode, PatternRow patRow)
//--------------------------------------------------------------------------------------------------------------------------------------------------------
{
	size_t anyFreeChannel = CHANNELINDEX_INVALID;

	size_t oldsetMidiCh = CHANNELINDEX_INVALID;
	tick_t oldestMidiChAge = 0;

	size_t oldestAnyCh = 0;
	tick_t oldestAnyChAge = 0;

	for(size_t i = 0; i < channels.size(); i++)
	{
		// Check if this note is already playing, or find any note of the same MIDI channel in case of mono mode
		if(channels[i].midiCh == midiCh && (channels[i].note == note || (monoMode && channels[i].note != NOTE_NONE)))
		{
			return static_cast<CHANNELINDEX>(i);
		}

		// If we can't find any free channels, look for the oldest channels
		if(channels[i].midiCh == midiCh && channels[i].age > oldestMidiChAge)
		{
			// Oldest channel matching this MIDI channel
			oldestMidiChAge = channels[i].age;
			oldsetMidiCh = i;
		}
		if(channels[i].age > oldestAnyChAge)
		{
			// Any oldest channel
			oldestAnyChAge = channels[i].age;
			oldestAnyCh = i;
		}
	}
	
	// Try grouping output into X pattern channels per MIDI channel
	size_t baseOffset = midiCh * PATTERN_CHANNELS_PER_MIDI_CHANNEL;
	for(size_t i = 0; i < channels.size(); i++)
	{
		CHANNELINDEX j = static_cast<CHANNELINDEX>((i + baseOffset) % channels.size());
		if(channels[j].note == NOTE_NONE && !patRow[j].IsNote())
		{
			// Recycle channel previously used by the same MIDI channel
			if(channels[j].midiCh == midiCh)
				return j;
			if(anyFreeChannel == CHANNELINDEX_INVALID)
				anyFreeChannel = j;
		}
	}
	if(anyFreeChannel != CHANNELINDEX_INVALID)
		return static_cast<CHANNELINDEX>(anyFreeChannel);
	if(oldsetMidiCh != CHANNELINDEX_INVALID)
		return static_cast<CHANNELINDEX>(oldsetMidiCh);
	return static_cast<CHANNELINDEX>(oldestAnyCh);
}


static void MIDINoteOff(MidiChannelState &midiChn, std::vector<ModChannelState> &modChnStatus, uint8 note, uint8 delay, PatternRow patRow)
//----------------------------------------------------------------------------------------------------------------------------------------
{
	CHANNELINDEX chn = midiChn.noteOn[note];
	if(chn == CHANNELINDEX_INVALID)
		return;

	uint8 midiCh = modChnStatus[chn].midiCh;
	modChnStatus[chn].note = NOTE_NONE;
	midiChn.noteOn[note] = CHANNELINDEX_INVALID;
	ModCommand &m = patRow[chn];
	if(m.note == NOTE_NONE)
	{
		m.note = NOTE_KEYOFF;
		if(delay != 0)
		{
			m.command = CMD_S3MCMDEX;
			m.param = 0xD0 | delay;
		}
	} else if(m.IsNote() && midiCh != (MIDI_DRUMCHANNEL - 1))
	{
		if(m.command == CMD_S3MCMDEX && (m.param & 0xF0) == 0xD0)
		{
			// Already have a note delay
			m.command = CMD_DELAYCUT;
			m.param = (m.param << 4) | (delay - (m.param & 0x0F));
		} else if(m.command == CMD_NONE || m.command == CMD_PANNING8)
		{
			m.command = CMD_S3MCMDEX;
			m.param = 0xC0 | delay;
		}
	}
}


static void EnterMIDIVolume(ModCommand &m, ModChannelState &modChn, const MidiChannelState &midiChn)
//--------------------------------------------------------------------------------------------------
{
	m.volcmd = VOLCMD_VOLUME;

	int32 vol = CDLSBank::DLSMidiVolumeToLinear(modChn.vol) >> 8;
	vol = (vol * midiChn.volume * midiChn.expression) >> 13;
	Limit(vol, 4, 256);
	m.vol = static_cast<ModCommand::VOL>(vol / 4);
}


bool CSoundFile::ReadMID(FileReader &file, ModLoadingFlags loadFlags)
//-------------------------------------------------------------------
{
	file.Rewind();

	// Microsoft MIDI files
	if(file.ReadMagic("RIFF"))
	{
		file.Skip(4);
		if(!file.ReadMagic("RMID"))
		{
			return false;
		} else if(loadFlags == onlyVerifyHeader)
		{
			return true;
		}
		do
		{
			char id[4];
			file.ReadArray(id);
			uint32 length = file.ReadUint32LE();
			if(memcmp(id, "data", 4))
			{
				file.Skip(length);
			} else
			{
				break;
			}
		} while(file.BytesLeft());
	}

	if(!file.ReadMagic("MThd"))
	{
		return false;
	} else if(loadFlags == onlyVerifyHeader)
	{
		return true;
	}

	MThd fileHeader;
	if(!file.ReadStruct(fileHeader)
		|| !file.Skip(fileHeader.headerLength - 6))
	{
		return false;
	}

	InitializeGlobals(MOD_TYPE_MID);
	InitializeChannels();

#ifdef MODPLUG_TRACKER
	const uint32 quantize = Clamp(TrackerSettings::Instance().midiImportQuantize.Get(), 1u, 256u);
	const ROWINDEX patternLen = Clamp(TrackerSettings::Instance().midiImportPatternLen.Get(), ROWINDEX(1), MAX_PATTERN_ROWS);
	const uint8 ticksPerRow = Clamp(TrackerSettings::Instance().midiImportTicks.Get(), uint8(2), uint8(16));
#else
	const uint32 quantize = 32;
	const ROWINDEX patternLen = 128;
	const uint8 ticksPerRow = 16;	// Must be in range 2...16
#endif

	m_songArtist = MPT_USTRING("MIDI Conversion");
	m_madeWithTracker = "Standard MIDI File";
	m_nTempoMode = tempoModeModern;
	m_SongFlags = SONG_LINEARSLIDES;
	m_nDefaultTempo.Set(120);
	m_nDefaultSpeed = ticksPerRow;
	m_nChannels = 16u * PATTERN_CHANNELS_PER_MIDI_CHANNEL + 2; // +2 for global commands
	m_nDefaultRowsPerBeat = quantize / 4;
	m_nDefaultRowsPerMeasure = 4 * m_nDefaultRowsPerBeat;
	TEMPO tempo = m_nDefaultTempo;
	uint16 ppqn = fileHeader.division;
	if(ppqn < 0)
	{
		int frames = -(ppqn >> 8), subFrames = (ppqn & 0xFF);
		ppqn = static_cast<uint16>(frames * subFrames / 2);
	}
	if(!ppqn)
		ppqn = 96;
	Order.clear();

	MidiChannelState midiChnStatus[16];
	const CHANNELINDEX tempoChannel = m_nChannels - 2, globalVolChannel = m_nChannels - 1;
	const uint16 numTracks = fileHeader.numTracks;
	std::vector<TrackState> tracks(numTracks);
	std::vector<ModChannelState> modChnStatus(m_nChannels);
	std::map<uint8, INSTRUMENTINDEX> modInstrument;

	for(uint16 t = 0; t < numTracks; t++)
	{
		if(!file.ReadMagic("MTrk"))
			return false;
		tracks[t].track = file.ReadChunk(file.ReadUint32BE());
		tick_t delta = 0;
		tracks[t].track.ReadVarInt(delta);
		tracks[t].nextEvent = delta;
	}

	uint16 finishedTracks = 0;
	PATTERNINDEX lastPat = 0;
	ROWINDEX lastRow = 0;
	while(finishedTracks < numTracks)
	{
		uint16 t = 0;
		tick_t tick = Util::MaxValueOfType(tick);
		for(uint16 track = 0; track < numTracks; track++)
		{
			if(!tracks[track].finished && tracks[track].nextEvent < tick)
			{
				tick = tracks[track].nextEvent;
				t = track;
			}
		}
		FileReader &track = tracks[t].track;

		tick_t modTicks = Util::muldivr_unsigned(tick, quantize * ticksPerRow / 4u, ppqn);

		PATTERNINDEX pat = static_cast<PATTERNINDEX>((modTicks / ticksPerRow) / patternLen);
		ROWINDEX row = (modTicks / ticksPerRow) % patternLen;
		uint8 delay = static_cast<uint8>(modTicks % ticksPerRow);

		if(pat > lastPat)
		{
			lastPat = pat;
			lastRow = row;
		} else if(pat == lastPat)
		{
			lastRow = std::max(lastRow, row);
		}

		if(pat >= Order.size())
		{
			ORDERINDEX curSize = Order.size();
			Order.resize(pat + 1);
			for(ORDERINDEX ord = curSize;  ord <= pat; ord++)
			{
				Order[ord] = ord;
				if(!Patterns.IsValidPat(ord) && !Patterns.Insert(ord, patternLen))
				{
					break;
				}
			}
			if(!Patterns.IsValidPat(pat))
				break;
		}
		PatternRow patRow = Patterns[pat].GetRow(row);

		uint8 data1 = track.ReadUint8();
		if(data1 == 0xFF)
		{
			data1 = track.ReadUint8();
			size_t len = 0;
			track.ReadVarInt(len);
			FileReader chunk = track.ReadChunk(len);

			switch(data1)
			{
			case 1:	// Text
				break;
			case 2:	// Copyright
				m_songMessage.Read(chunk, len, SongMessage::leAutodetect);
				break;
			case 3:	// Track Name
				if(m_songName.empty())
				{
					//std::string s;
					//chunk.ReadString<mpt::String::maybeNullTerminated>(s, len);
					//m_songArtist = mpt::ToUnicode(mpt::CharsetLocaleOrUTF8, s);
					chunk.ReadString<mpt::String::maybeNullTerminated>(m_songName, len);
				}
				break;
			case 4:	// Instrument
			case 5:	// Lyric
				break;
			case 6:	// Marker
			case 7:	// Cue point
				{
					std::string s;
					chunk.ReadString<mpt::String::maybeNullTerminated>(s, len);
					Patterns[pat].SetName(s);
				}
				break;
			case 8:	// Patch name
			case 9:	// Port name
				break;
			case 0x2F:
				break;
			case 0x58: // Time Signature
				{
					/*int nn = chunk.ReadUint8();
					int dd = chunk.ReadUint8();
					int cc = chunk.ReadUint8();
					int bb = chunk.ReadUint8();
					Log("Time sig: %d:%d, metronome:%d, quarter:%d\n",nn,dd,cc,bb);*/
				}
				break;
			case 0x59: // Key Signature
				{
					/*int sf = chunk.ReadInt8();
					int mi = chunk.ReadUint8();
					Log("Key sig: %d %s, %s\n",abs(sf),sf == 0?"c":(sf < 0 ? "flat":"sharp"), mi?"minor":"major");*/
				}
				break;
			case 0x51: // Tempo
				{
					uint32 t = (chunk.ReadUint8() << 16) | (chunk.ReadUint8() << 8) | chunk.ReadUint8();
					if(t == 0)
						break;
					TEMPO newTempo(60000000.0 / t);
					if(!tick)
					{
						m_nDefaultTempo = newTempo;
					} else if(newTempo != tempo)
					{
						patRow[tempoChannel].command = CMD_TEMPO;
						patRow[tempoChannel].param = Util::Round<ModCommand::PARAM>(newTempo.ToDouble());
					}
					tempo = newTempo;
				}
				break;

			default:
				break;
			}
		} else
		{
			if(data1 & 0x80)
			{
				// Command byte (if not present, repeat previous command byte)
				tracks[t].command = data1;
				if(data1 < 0xF0)
					data1 = track.ReadUint8();
			}
			uint8 midiCh = tracks[t].command & 0x0F;

			switch(tracks[t].command & 0xF0)
			{
			case 0x80: // Note Off
			case 0x90: // Note On
				{
					data1 &= 0x7F;
					ModCommand::NOTE note = static_cast<ModCommand::NOTE>(Clamp(data1 + NOTE_MIN, NOTE_MIN, NOTE_MAX));
					uint8 data2 = track.ReadUint8();
					if(data2 > 0 && (tracks[t].command & 0xF0) == 0x90)
					{
						// Note On
						CHANNELINDEX chn = FindUnusedChannel(midiCh, note, modChnStatus, midiChnStatus[midiCh].monoMode, patRow);
						if(chn != CHANNELINDEX_INVALID)
						{
							modChnStatus[chn].note = note;
							modChnStatus[chn].midiCh = midiCh;
							modChnStatus[chn].vol = data2;
							midiChnStatus[midiCh].noteOn[data1] = chn;
							int32 pitchOffset = 0;
							if(midiChnStatus[midiCh].pitchbend != 0x2000)
							{
								pitchOffset = Util::muldivr(midiChnStatus[midiCh].pitchbend - 0x2000, midiChnStatus[midiCh].pitchBendRange, 0x2000);
								modChnStatus[chn].porta = pitchOffset * 64;
							} else
							{
								modChnStatus[chn].porta = 0;
							}
							patRow[chn].note = static_cast<ModCommand::NOTE>(Clamp(note + pitchOffset, NOTE_MIN, NOTE_MAX));
							patRow[chn].instr = mpt::saturate_cast<ModCommand::INSTR>(MapMidiInstrument(midiChnStatus[midiCh].program, midiChnStatus[midiCh].bank, midiCh + 1, data1));
							EnterMIDIVolume(patRow[chn], modChnStatus[chn], midiChnStatus[midiCh]);

							if(delay != 0)
							{
								patRow[chn].command = CMD_S3MCMDEX;
								patRow[chn].param = 0xD0 | delay;
							}
							if(modChnStatus[chn].pan != midiChnStatus[midiCh].pan && patRow[chn].command == CMD_NONE)
							{
								patRow[chn].command = CMD_PANNING8;
								patRow[chn].param = midiChnStatus[midiCh].pan;
								modChnStatus[chn].pan = midiChnStatus[midiCh].pan;
							}
						}
					} else
					{
						// Note Off
						MIDINoteOff(midiChnStatus[midiCh], modChnStatus, data1, delay, patRow);
					}
				}
				break;
			case 0xA0: // Note Aftertouch
				{
					track.Skip(1);
				}
				break;
			case 0xB0: // Controller
				{
					uint8 data2 = track.ReadUint8();
					switch(data1)
					{
					case MIDIEvents::MIDICC_Panposition_Coarse:
						midiChnStatus[midiCh].pan = data2 * 2u;
						for(size_t i = 0; i < CountOf(midiChnStatus[midiCh].noteOn); i++)
						{
							CHANNELINDEX chn = midiChnStatus[midiCh].noteOn[i];
							if(chn != CHANNELINDEX_INVALID)
							{
								if(Patterns[pat].WriteEffect(EffectWriter(CMD_PANNING8, midiChnStatus[midiCh].pan).Channel(chn).Row(row)))
								{
									modChnStatus[chn].pan = midiChnStatus[midiCh].pan;
								}
							}
						}
						break;

					case MIDIEvents::MIDICC_DataEntry_Coarse:
						if(midiChnStatus[midiCh].rpnState == 2)
						{
							midiChnStatus[midiCh].pitchBendRange = std::max(data2, uint8(1));
						}
						break;

					case MIDIEvents::MIDICC_Volume_Coarse:
						midiChnStatus[midiCh].volume = (uint8)(CDLSBank::DLSMidiVolumeToLinear(data2) >> 9);
						for(size_t i = 0; i < CountOf(midiChnStatus[midiCh].noteOn); i++)
						{
							CHANNELINDEX chn = midiChnStatus[midiCh].noteOn[i];
							if(chn != CHANNELINDEX_INVALID)
							{
								EnterMIDIVolume(patRow[chn], modChnStatus[chn], midiChnStatus[midiCh]);
							}
						}
						break;

					case MIDIEvents::MIDICC_Expression_Coarse:
						midiChnStatus[midiCh].expression = (uint8)(CDLSBank::DLSMidiVolumeToLinear(data2) >> 9);
						for(size_t i = 0; i < CountOf(midiChnStatus[midiCh].noteOn); i++)
						{
							CHANNELINDEX chn = midiChnStatus[midiCh].noteOn[i];
							if(chn != CHANNELINDEX_INVALID)
							{
								EnterMIDIVolume(patRow[chn], modChnStatus[chn], midiChnStatus[midiCh]);
							}
						}
						break;

					case MIDIEvents::MIDICC_BankSelect_Coarse:
						midiChnStatus[midiCh].bank &= 0x7F;
						midiChnStatus[midiCh].bank |= (data2 << 7);
						break;

					case MIDIEvents::MIDICC_BankSelect_Fine:
						midiChnStatus[midiCh].bank &= (0x7F << 7);
						midiChnStatus[midiCh].bank |= data2;
						break;

					case MIDIEvents::MIDICC_RegisteredParameter_Fine:
						if(data2 == 0)
						{
							midiChnStatus[midiCh].rpnState = 1;
						}
						break;
					case MIDIEvents::MIDICC_RegisteredParameter_Coarse:
						if(data2 == 0 && midiChnStatus[midiCh].rpnState == 1)
						{
							midiChnStatus[midiCh].rpnState = 2;
						}
						break;

					case MIDIEvents::MIDICC_AllControllersOff:
						midiChnStatus[midiCh].expression = 128;
						midiChnStatus[midiCh].pitchbend = 0x2000;
						midiChnStatus[midiCh].pitchBendRange = 2;
						midiChnStatus[midiCh].rpnState = 0;
						midiChnStatus[midiCh].modulation = 0;
						// Should also reset pedals (40h-43h), NRP, RPN, aftertouch
						break;

						// Bn.78.00: All Sound Off (GS)
						// Bn.7B.00: All Notes Off (GM)
					case MIDIEvents::MIDICC_AllSoundOff:
					case MIDIEvents::MIDICC_AllNotesOff:
						if(data2 == 0x00)
						{
							// All Notes Off
							for(uint8 note = 0; note < 128; note++)
							{
								MIDINoteOff(midiChnStatus[midiCh], modChnStatus, note, delay, patRow);
							}
						}
					case MIDIEvents::MIDICC_MonoOperation:
						midiChnStatus[midiCh].monoMode = true;
						break;
					case MIDIEvents::MIDICC_PolyOperation:
						midiChnStatus[midiCh].monoMode = false;
						break;
					}

					if((data1 != MIDIEvents::MIDICC_RegisteredParameter_Fine && data1 != MIDIEvents::MIDICC_RegisteredParameter_Coarse) || data2 != 0)
					{
						midiChnStatus[midiCh].rpnState = 0;
					}
				}
				break;
			case 0xC0: // Program Change
				midiChnStatus[midiCh].program = data1 & 0x7F;
				break;
			case 0xD0: // Channel aftertouch
				break;
			case 0xE0: // Pitch bend
				midiChnStatus[midiCh].pitchbend = data1 | (track.ReadUint8() << 7);
				break;
			case 0xF0: // General / Immediate
				switch(midiCh)
				{
				case MIDIEvents::sysExStart: // SysEx
					{
						uint32 len;
						track.ReadVarInt(len);
						FileReader sysex = track.ReadChunk(len);
						if(sysex.ReadMagic("\x7F\x7F\x04\x01"))
						{
							uint16 globalVol = sysex.ReadUint8() | (sysex.ReadUint8() << 7);
							patRow[globalVolChannel].command = CMD_GLOBALVOLUME;
							patRow[globalVolChannel].param = static_cast<ModCommand::PARAM>(Util::muldivr_unsigned(globalVol, 128, 16383));
						}
					}
					break;
				case MIDIEvents::sysQuarterFrame:
					track.Skip(1);
					break;
				case MIDIEvents::sysPositionPointer:
					track.Skip(2);
					break;
				case MIDIEvents::sysSongSelect:
					track.Skip(1);
					break;
				case MIDIEvents::sysTuneRequest:
				case MIDIEvents::sysMIDIClock:
				case MIDIEvents::sysMIDITick:
				case MIDIEvents::sysStart:
				case MIDIEvents::sysContinue:
				case MIDIEvents::sysStop:
				case MIDIEvents::sysActiveSense:
				case MIDIEvents::sysReset:
					break;

				default:
					break;
				}
				break;

			default:
				break;
			}
		}

		// Pitch bend any channels that haven't reached their target yet
		// TODO: This is currently not called on any rows without events!
		for(size_t chn = 0; chn < modChnStatus.size(); chn++)
		{
			ModChannelState &chnState = modChnStatus[chn];
			ModCommand &m = patRow[chn];
			uint8 midiCh = chnState.midiCh;
			if(chnState.note == NOTE_NONE || m.command == CMD_S3MCMDEX || m.command == CMD_DELAYCUT || midiCh > 0x0F)
				continue;

			int32 pitchBendRange = midiChnStatus[midiCh].pitchBendRange * 64;
			// Convert from arbitrary MIDI pitchbend to 64th of semitone
			int32 diff = Util::muldiv(midiChnStatus[midiCh].pitchbend - 0x2000, pitchBendRange, 0x2000) - chnState.porta;
			if(diff == 0)
				continue;

			if(m.command == CMD_PORTAMENTODOWN || m.command == CMD_PORTAMENTOUP)
			{
				// First, undo the effect of an existing portamento command
				int32 porta = 0;
				if(m.param < 0xE0)
					porta = m.param * 4 * (ticksPerRow - 1);
				else if(m.param < 0xF0)
					porta = (m.param & 0x0F);
				else
					porta = (m.param & 0x0F) * 4;
				if(m.command == CMD_PORTAMENTODOWN)
					diff -= porta;
				else
					diff += porta;
			}

			m.command = static_cast<ModCommand::COMMAND>(diff < 0 ? CMD_PORTAMENTODOWN : CMD_PORTAMENTOUP);
			int32 absDiff = mpt::abs(diff);
			int32 realDiff = 0;
			if(absDiff < 16)
			{
				// Extra-fine slides can do this.
				m.param = 0xE0 | static_cast<uint8>(absDiff);
				realDiff = absDiff;
			} else if(absDiff < 64)
			{
				// Fine slides can do this.
				absDiff /= 4;
				m.param = 0xF0 | static_cast<uint8>(absDiff);
				realDiff = absDiff * 4;
			} else
			{
				// Need a normal slide.
				absDiff /= 4 * (ticksPerRow - 1);
				LimitMax(absDiff, 0xDF);
				m.param = static_cast<uint8>(absDiff);
				MPT_ASSERT(m.param < 30);
				realDiff = absDiff * 4 * (ticksPerRow - 1);
			}
			chnState.porta += realDiff * sgn(diff);
		}

		tick_t delta = 0;
		if(track.ReadVarInt(delta) && track.CanRead(1))
		{
			tracks[t].nextEvent += delta;
		} else
		{
			finishedTracks++;
			tracks[t].nextEvent = Util::MaxValueOfType(delta);
			tracks[t].finished = true;
		}
	}

	if(Patterns.IsValidPat(lastPat))
	{
		Patterns[lastPat].Resize(lastRow + 1);
	}

	if(GetpModDoc() != nullptr)
	{
		std::vector<CHANNELINDEX> channels;
		channels.reserve(m_nChannels);
		for(CHANNELINDEX i = 0; i < m_nChannels; i++)
		{
			if(modChnStatus[i].midiCh != 0xFF || !GetpModDoc()->IsChannelUnused(i))
			{
				channels.push_back(i);
				if(i < tempoChannel)
					sprintf(ChnSettings[i].szName, "MIDI Ch %u", 1 + (i / PATTERN_CHANNELS_PER_MIDI_CHANNEL));
				else if(i == tempoChannel)
					strcpy(ChnSettings[i].szName, "Tempo");
				else if(i == globalVolChannel)
					strcpy(ChnSettings[i].szName, "Global Volume");
			}
		}
		GetpModDoc()->ReArrangeChannels(channels);
	}

	CDLSBank *pCachedBank = nullptr, *pEmbeddedBank = nullptr;
	mpt::PathString szCachedBankFile;

	if(CDLSBank::IsDLSBank(file.GetFileName()))
	{
		pEmbeddedBank = new CDLSBank();
		pEmbeddedBank->Open(file.GetFileName());
	}
	ChangeModTypeTo(MOD_TYPE_MPT);
	MIDILIBSTRUCT &midiLib = CTrackApp::GetMidiLibrary();
	// Scan Instruments
	for (INSTRUMENTINDEX nIns = 1; nIns <= m_nInstruments; nIns++) if (Instruments[nIns])
	{
		mpt::PathString pszMidiMapName;
		ModInstrument *pIns = Instruments[nIns];
		uint32 nMidiCode;
		bool embedded = false;

		if (pIns->nMidiChannel == MIDI_DRUMCHANNEL)
			nMidiCode = 0x80 | (pIns->nMidiDrumKey & 0x7F);
		else
			nMidiCode = pIns->nMidiProgram & 0x7F;
		pszMidiMapName = midiLib.MidiMap[nMidiCode];
		if (pEmbeddedBank)
		{
			uint32 nDlsIns = 0, nDrumRgn = 0;
			uint32 nProgram = (pIns->nMidiProgram != 0) ? pIns->nMidiProgram - 1 : 0;
			uint32 dwKey = (nMidiCode < 128) ? 0xFF : (nMidiCode & 0x7F);
			if ((pEmbeddedBank->FindInstrument(	(nMidiCode >= 128),
				(pIns->wMidiBank & 0x3FFF),
				nProgram, dwKey, &nDlsIns))
				|| (pEmbeddedBank->FindInstrument(	(nMidiCode >= 128),	0xFFFF,
				(nMidiCode >= 128) ? 0xFF : nProgram,
				dwKey, &nDlsIns)))
			{
				if (dwKey < 0x80) nDrumRgn = pEmbeddedBank->GetRegionFromKey(nDlsIns, dwKey);
				if (pEmbeddedBank->ExtractInstrument(*this, nIns, nDlsIns, nDrumRgn))
				{
					pIns = Instruments[nIns]; // Reset pIns because ExtractInstrument may delete the previous value.
					if ((dwKey >= 24) && (dwKey < 100))
					{
						mpt::String::CopyN(pIns->name, szMidiPercussionNames[dwKey - 24]);
					}
					embedded = true;
				}
				else
					pIns = Instruments[nIns]; // Reset pIns because ExtractInstrument may delete the previous value.
			}
		}
		if((!pszMidiMapName.empty()) && (!embedded))
		{
			// Load From DLS Bank
			if (CDLSBank::IsDLSBank(pszMidiMapName))
			{
				CDLSBank *pDLSBank = NULL;

				if ((pCachedBank) && (!mpt::PathString::CompareNoCase(szCachedBankFile, pszMidiMapName)))
				{
					pDLSBank = pCachedBank;
				} else
				{
					if (pCachedBank) delete pCachedBank;
					pCachedBank = new CDLSBank;
					szCachedBankFile = pszMidiMapName;
					if (pCachedBank->Open(pszMidiMapName)) pDLSBank = pCachedBank;
				}
				if (pDLSBank)
				{
					uint32 nDlsIns = 0, nDrumRgn = 0;
					uint32 nProgram = (pIns->nMidiProgram != 0) ? pIns->nMidiProgram - 1 : 0;
					uint32 dwKey = (nMidiCode < 128) ? 0xFF : (nMidiCode & 0x7F);
					if ((pDLSBank->FindInstrument(	(nMidiCode >= 128),
						(pIns->wMidiBank & 0x3FFF),
						nProgram, dwKey, &nDlsIns))
						|| (pDLSBank->FindInstrument(	(nMidiCode >= 128), 0xFFFF,
						(nMidiCode >= 128) ? 0xFF : nProgram,
						dwKey, &nDlsIns)))
					{
						if (dwKey < 0x80) nDrumRgn = pDLSBank->GetRegionFromKey(nDlsIns, dwKey);
						pDLSBank->ExtractInstrument(*this, nIns, nDlsIns, nDrumRgn);
						pIns = Instruments[nIns]; // Reset pIns because ExtractInstrument may delete the previous value.
						if ((dwKey >= 24) && (dwKey < 24+61))
						{
							mpt::String::CopyN(pIns->name, szMidiPercussionNames[dwKey-24]);
						}
					}
				}
			} else
			{
				// Load from Instrument or Sample file
				InputFile f;

				if(f.Open(pszMidiMapName))
				{
					FileReader file = GetFileReader(f);
					if(file.IsValid())
					{
						ReadInstrumentFromFile(nIns, file, false);
						mpt::PathString szName, szExt;
						pszMidiMapName.SplitPath(nullptr, nullptr, &szName, &szExt);
						szName += szExt;
						pIns = Instruments[nIns];
						if (!pIns->filename[0]) mpt::String::Copy(pIns->filename, szName.ToLocale().c_str());
						if (!pIns->name[0])
						{
							if (nMidiCode < 128)
							{
								mpt::String::CopyN(pIns->name, szMidiProgramNames[nMidiCode]);
							} else
							{
								uint32 nKey = nMidiCode & 0x7F;
								if (nKey >= 24)
									mpt::String::CopyN(pIns->name, szMidiPercussionNames[nKey - 24]);
							}
						}
					}
				}
			}
		}
	}
	if (pCachedBank) delete pCachedBank;
	if (pEmbeddedBank) delete pEmbeddedBank;

	return true;
}


#else // !MODPLUG_TRACKER

bool CSoundFile::ReadMID(FileReader &/*file*/, ModLoadingFlags /*loadFlags*/)
{
	return false;
}

#endif

OPENMPT_NAMESPACE_END
