/*
 * Load_itp.cpp
 * ------------
 * Purpose: Impulse Tracker Project (ITP) module loader / saver
 * Notes  : Despite its name, ITP is not a format supported by Impulse Tracker.
 *          In fact, it's a format invented by the OpenMPT team to allow people to work
 *          with the IT format, but keeping the instrument files with big samples separate
 *          from the pattern data, to keep the work files small and handy.
 *          The current design of the format is quite flawed, though, so expect this to
 *          change in the (far?) future.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#ifdef MODPLUG_TRACKER
#include "../mptrack/mptrack.h"
#endif
#include "../common/version.h"
#include "Loaders.h"
#include "ITTools.h"


#define ITP_VERSION 0x00000102	// v1.02
#define ITP_FILE_ID 0x2E697470	// .itp ASCII


// Read variable-length ITP string.
template<size_t destSize>
bool ReadITPString(char (&destBuffer)[destSize], FileReader &file)
//----------------------------------------------------------------
{
	return file.ReadString<mpt::String::maybeNullTerminated>(destBuffer, file.ReadUint32LE());
}


bool CSoundFile::ReadITProject(FileReader &file, ModLoadingFlags loadFlags)
//-------------------------------------------------------------------------
{
#ifndef MODPLUG_TRACKER
	UNREFERENCED_PARAMETER(file);
	UNREFERENCED_PARAMETER(loadFlags);
	return false;
#else // MODPLUG_TRACKER
	
	uint32 version;
	FileReader::off_t size;

	file.Rewind();

	// Check file ID
	if(!file.CanRead(12 + 4 + 24 + 4)
		|| file.ReadUint32LE() != ITP_FILE_ID				// Magic bytes
		|| (version = file.ReadUint32LE()) > ITP_VERSION	// Format version
		|| !ReadITPString(m_szNames[0], file))				// Song name
	{
		return false;
	} else if(loadFlags == onlyVerifyHeader)
	{
		return true;
	}

	InitializeGlobals();

	// Song comments
	songMessage.Read(file, file.ReadUint32LE(), SongMessage::leCR);

	// Song global config
	m_SongFlags = static_cast<SongFlags>(file.ReadUint32LE() & SONG_FILE_FLAGS);
	if(!m_SongFlags[SONG_ITPROJECT])
	{
		return false;
	}

	m_nDefaultGlobalVolume = file.ReadUint32LE();
	m_nSamplePreAmp = file.ReadUint32LE();
	m_nDefaultSpeed = file.ReadUint32LE();
	m_nDefaultTempo = file.ReadUint32LE();
	m_nChannels = static_cast<CHANNELINDEX>(file.ReadUint32LE());
	if(m_nChannels == 0 || m_nChannels > MAX_BASECHANNELS)
	{
		return false;
	}

	// channel name string length (=MAX_CHANNELNAME)
	size = file.ReadUint32LE();

	// Channels' data
	for(CHANNELINDEX chn = 0; chn < m_nChannels; chn++)
	{
		ChnSettings[chn].nPan = static_cast<uint16>(file.ReadUint32LE());
		ChnSettings[chn].dwFlags = static_cast<ChannelFlags>(file.ReadUint32LE());
		ChnSettings[chn].nVolume = static_cast<uint16>(file.ReadUint32LE());
		file.ReadString<mpt::String::maybeNullTerminated>(ChnSettings[chn].szName, size);
	}

	// Song mix plugins
	{
		FileReader plugChunk = file.GetChunk(file.ReadUint32LE());
		LoadMixPlugins(plugChunk);
	}

	// MIDI Macro config
	file.ReadStructPartial(m_MidiCfg, file.ReadUint32LE());
	if(m_SongFlags[SONG_EMBEDMIDICFG])
	{
		m_MidiCfg.Sanitize();
	} else
	{
		m_MidiCfg.Reset();
	}

	// Song Instruments
	m_nInstruments = static_cast<INSTRUMENTINDEX>(file.ReadUint32LE());
	if(m_nInstruments >= MAX_INSTRUMENTS)
	{
		return false;
	}

	// Instruments' paths
	size = file.ReadUint32LE();	// path string length
	for(INSTRUMENTINDEX ins = 0; ins < GetNumInstruments(); ins++)
	{
		char path[_MAX_PATH];
		file.ReadString<mpt::String::maybeNullTerminated>(path, size);
		m_szInstrumentPath[ins] = path;
	}

	// Song Orders
	Order.ReadAsByte(file, file.ReadUint32LE());

	// Song Patterns
	const PATTERNINDEX numPats = static_cast<PATTERNINDEX>(file.ReadUint32LE());
	const PATTERNINDEX numNamedPats = static_cast<PATTERNINDEX>(file.ReadUint32LE());
	size_t patNameLen = file.ReadUint32LE();	// Size of each pattern name
	FileReader pattNames = file.GetChunk(numNamedPats * patNameLen);


	// modcommand data length
	size = file.ReadUint32LE();
	if(size != 6)
	{
		return false;
	}

	for(PATTERNINDEX pat = 0; pat < numPats; pat++)
	{
		// Patterns[npat].GetNumRows()
		const ROWINDEX numRows = file.ReadUint32LE();
		FileReader patternChunk = file.GetChunk(numRows * size * GetNumChannels());

		// Allocate pattern
		if(!(loadFlags & loadPatternData) || numRows == 0 || numRows > MAX_PATTERN_ROWS || Patterns.Insert(pat, numRows))
		{
			pattNames.Skip(patNameLen);
			continue;
		}

		if(pat < numNamedPats)
		{
			char patName[MAX_PATTERNNAME];
			pattNames.ReadString<mpt::String::maybeNullTerminated>(patName, patNameLen);
			Patterns[pat].SetName(patName);
		}

		// Pattern data
		size_t numCommands = GetNumChannels() * numRows;

		if(patternChunk.CanRead(sizeof(MODCOMMAND_ORIGINAL) * numCommands))
		{
			ModCommand *target = Patterns[pat].GetpModCommand(0, 0);
			while(numCommands-- != 0)
			{
				MODCOMMAND_ORIGINAL data;
				patternChunk.Read(data);
				*(target++) = data;
			}
		}
	}

	// Load embeded samples

	// Read original number of samples
	m_nSamples = static_cast<SAMPLEINDEX>(file.ReadUint32LE());
	LimitMax(m_nSamples, SAMPLEINDEX(MAX_SAMPLES - 1));

	// Read number of embeded samples
	uint32 embeddedSamples = file.ReadUint32LE();

	// Read samples
	for(uint32 smp = 0; smp < embeddedSamples; smp++)
	{
		SAMPLEINDEX realSample = static_cast<SAMPLEINDEX>(file.ReadUint32LE());
		ITSample sampleHeader;
		file.ReadConvertEndianness(sampleHeader);
		size = file.ReadUint32LE();

		if(realSample >= 1 && realSample < MAX_SAMPLES && sampleHeader.id == ITSample::magic)
		{
			sampleHeader.ConvertToMPT(Samples[realSample]);
			mpt::String::Read<mpt::String::nullTerminated>(m_szNames[realSample], sampleHeader.name);

			// Read sample data
			sampleHeader.GetSampleFormat().ReadSample(Samples[realSample], file);
		} else
		{
			file.Skip(size);
		}
	}

	// Load instruments
	CMappedFile f;

	for(INSTRUMENTINDEX ins = 0; ins < GetNumInstruments(); ins++)
	{
		if(m_szInstrumentPath[ins].empty() || !f.Open(m_szInstrumentPath[ins].c_str())) continue;

		size = f.GetLength();
		LPBYTE lpFile = f.Lock(size);
		if(!lpFile) { f.Close(); continue; }

		ReadInstrumentFromFile(ins + 1, lpFile, size);
		f.Unlock();
		f.Close();
	}

	// Extra info data
	uint32 code = file.ReadUint32LE();

	// Embed instruments' header [v1.01]
	if(version >= 0x00000101 && m_SongFlags[SONG_ITPEMBEDIH] && code == 'EBIH')
	{
		code = file.ReadUint32LE();

		INSTRUMENTINDEX ins = 1;
		while(ins <= GetNumInstruments() && file.AreBytesLeft())
		{
			if(code == 'MPTS')
			{
				break;
			} else if(code == 'SEP@' || code == 'MPTX')
			{
				// jump code - switch to next instrument
				ins++;
			} else
			{
				ReadExtendedInstrumentProperty(Instruments[ins], code, file);
			}

			code = file.ReadUint32LE();
		}
	}

	// Song extensions
	if(code == 'MPTS')
	{
		LoadExtendedSongProperties(MOD_TYPE_IT, file);
	}

	m_nType = MOD_TYPE_IT;
	m_nMaxPeriod = 0xF000;
	m_nMinPeriod = 8;

	UpgradeModFlags();

	madeWithTracker = "OpenMPT " + MptVersion::ToStr(m_dwLastSavedWithVersion);

	return true;
#endif
}


#ifndef MODPLUG_NO_FILESAVE

bool CSoundFile::SaveITProject(LPCSTR lpszFileName)
//-------------------------------------------------
{
	// Check song type

	if(!m_SongFlags[SONG_ITPROJECT]) return false;

	UINT i,j = 0;
	for(i = 0 ; i < m_nInstruments ; i++) { if(!m_szInstrumentPath[i].empty() || !Instruments[i+1]) j++; }
	if(m_nInstruments && j != m_nInstruments) return false;

	// Open file

	FILE *f;

	if((!lpszFileName) || ((f = fopen(lpszFileName, "wb")) == NULL)) return false;


	// File ID

	DWORD id = ITP_FILE_ID;
	fwrite(&id, 1, sizeof(id), f);

	id = ITP_VERSION;
	fwrite(&id, 1, sizeof(id), f);

	// Song name

	// name string length
	id = 27;
	fwrite(&id, 1, sizeof(id), f);

	// song name
	fwrite(&m_szNames[0], 1, 27, f);

	// Song comments

	// comment string length
	{
		const size_t msgLength = songMessage.empty() ? 0 : songMessage.length() + 1;
		id = msgLength;
		fwrite(&id, 1, sizeof(id), f);

		// comment string
		if(msgLength) fwrite(songMessage.c_str(), 1, msgLength, f);
	}

	// Song global config

	id = (m_SongFlags & SONG_FILE_FLAGS);
	fwrite(&id, 1, sizeof(id), f);
	id = m_nDefaultGlobalVolume;
	fwrite(&id, 1, sizeof(id), f);
	id = m_nSamplePreAmp;
	fwrite(&id, 1, sizeof(id), f);
	id = m_nDefaultSpeed;
	fwrite(&id, 1, sizeof(id), f);
	id = m_nDefaultTempo;
	fwrite(&id, 1, sizeof(id), f);

	// Song channels data

	// number of channels
	id = m_nChannels;
	fwrite(&id, 1, sizeof(id), f);

	// channel name string length
	id = MAX_CHANNELNAME;
	fwrite(&id, 1, sizeof(id), f);

	// channel config data
	for(i=0; i<m_nChannels; i++)
	{
		id = ChnSettings[i].nPan;
		fwrite(&id, 1, sizeof(id), f);
		id = ChnSettings[i].dwFlags;
		fwrite(&id, 1, sizeof(id), f);
		id = ChnSettings[i].nVolume;
		fwrite(&id, 1, sizeof(id), f);
		fwrite(&ChnSettings[i].szName[0], 1, MAX_CHANNELNAME, f);
	}

	// Song mix plugins

	// mix plugins data length
	id = SaveMixPlugins(NULL, TRUE);
	fwrite(&id, 1, sizeof(id), f);

	// mix plugins data
	SaveMixPlugins(f, FALSE);

	// Song midi config

	// midi cfg data length
	id = m_SongFlags[SONG_EMBEDMIDICFG] ? sizeof(MIDIMacroConfigData) : 0;
	fwrite(&id, 1, sizeof(id), f);

	// midi cfg
	fwrite(&m_MidiCfg, 1, id, f);

	// Song Instruments

	// number of instruments
	id = m_nInstruments;
	fwrite(&id, 1, sizeof(id), f);

	// path name string length
	id = _MAX_PATH;
	fwrite(&id, 1, sizeof(id), f);

	// instruments' path
	for(i = 0; i < m_nInstruments; i++)
	{
		char path[_MAX_PATH];
		MemsetZero(path);
		strncpy(path, m_szInstrumentPath[i].c_str(), _MAX_PATH);
		fwrite(path, 1, _MAX_PATH, f);
	}

	// Song Orders

	// order array size
	id = Order.size();
	fwrite(&id, 1, sizeof(id), f);

	// order array
	Order.WriteAsByte(f, static_cast<uint16>(id));

	// Song Patterns

	// number of patterns
	id = MAX_PATTERNS;
	fwrite(&id, 1, sizeof(id), f);

	// number of pattern name strings
	PATTERNINDEX numNamedPats = Patterns.GetNumNamedPatterns();
	numNamedPats = MIN(numNamedPats, MAX_PATTERNS);
	id = numNamedPats;
	fwrite(&id, 1, sizeof(id), f);

	// length of a pattern name string
	id = MAX_PATTERNNAME;
	fwrite(&id, 1, sizeof(id), f);
	// pattern name string
	for(PATTERNINDEX nPat = 0; nPat < numNamedPats; nPat++)
	{
		char name[MAX_PATTERNNAME];
		MemsetZero(name);
		Patterns[nPat].GetName(name);
		fwrite(name, 1, MAX_PATTERNNAME, f);
	}

	// modcommand data length
	id = sizeof(MODCOMMAND_ORIGINAL);
	fwrite(&id, 1, sizeof(id), f);

	// patterns data content
	for(PATTERNINDEX npat = 0; npat < MAX_PATTERNS; npat++)
	{
		// pattern size (number of rows)
		id = Patterns[npat] ? Patterns[npat].GetNumRows() : 0;
		fwrite(&id, 1, sizeof(id), f);
		// pattern data
		if(Patterns[npat] && Patterns[npat].GetNumRows()) Patterns[npat].WriteITPdata(f);
		//fwrite(Patterns[npat], 1, m_nChannels * Patterns[npat].GetNumRows() * sizeof(MODCOMMAND_ORIGINAL), f);
	}

	// Song lonely (instrument-less) samples

	// Write original number of samples
	id = m_nSamples;
	fwrite(&id, 1, sizeof(id), f);

	vector<bool> sampleUsed(GetNumSamples() + 1, false);

	// Mark samples used in instruments
	for(i = 0; i < m_nInstruments; i++)
	{
		if(Instruments[i + 1] != nullptr)
		{
			Instruments[i + 1]->GetSamples(sampleUsed);
		}
	}

	// Count samples not used in any instrument
	i = 0;
	for(j = 1; j <= m_nSamples; j++)
		if(!sampleUsed[j] && Samples[j].pSample) i++;

	id = i;
	fwrite(&id, 1, sizeof(id), f);

	// Write samples not used in any instrument (help, this looks like duplicate code!)
	for(UINT nsmp=1; nsmp<=m_nSamples; nsmp++)
	{
		if(!sampleUsed[nsmp] && Samples[nsmp].pSample)
		{
			ITSample itss;
			itss.ConvertToIT(Samples[nsmp], GetType(), false, false);

			mpt::String::Write<mpt::String::nullTerminated>(itss.name, m_szNames[nsmp]);

			id = nsmp;
			fwrite(&id, 1, sizeof(id), f);

			itss.samplepointer = 0;
			itss.ConvertEndianness();
			fwrite(&itss, 1, sizeof(itss), f);

			id = Samples[nsmp].GetSampleSizeInBytes();
			fwrite(&id, 1, sizeof(id), f);
			itss.GetSampleFormat().WriteSample(f, Samples[nsmp]);
		}
	}

	// Embed instruments' header [v1.01]

	if(m_SongFlags[SONG_ITPEMBEDIH])
	{
		// embeded instrument header tag
		uint32 code = 'EBIH';
		fwrite(&code, 1, sizeof(uint32), f);

		// instruments' header
		for(i=0; i<m_nInstruments; i++)
		{
			if(Instruments[i+1]) WriteInstrumentHeaderStruct(Instruments[i+1], f);
			// write separator tag
			code = 'SEP@';
			fwrite(&code, 1, sizeof(uint32), f);
		}
	}

	SaveExtendedSongProperties(f);

	// Close file
	fclose(f);
	return true;
}

#endif
