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
#include "../mptrack/mptrack.h"
#include "../mptrack/version.h"
#include "Loaders.h"
#include "ITTools.h"


#define ITP_VERSION 0x00000102	// v1.02
#define ITP_FILE_ID 0x2e697470	// .itp ASCII


bool CSoundFile::ReadITProject(LPCBYTE lpStream, const DWORD dwMemLength)
//-----------------------------------------------------------------------
{
	UINT i,n,nsmp;
	DWORD id,len,size;
	DWORD dwMemPos = 0;
	DWORD version;

	ASSERT_CAN_READ(12);

	// Check file ID

	memcpy(&id,lpStream+dwMemPos,sizeof(DWORD));
	if(id != ITP_FILE_ID) return false;
	dwMemPos += sizeof(DWORD);

	memcpy(&id,lpStream+dwMemPos,sizeof(DWORD));
	version = id;
	dwMemPos += sizeof(DWORD);

	// max supported version
	if(version > ITP_VERSION)
	{
		return false;
	}

	m_nType = MOD_TYPE_IT;

	// Song name

	// name string length
	memcpy(&id,lpStream+dwMemPos,sizeof(DWORD));
	len = id;
	dwMemPos += sizeof(DWORD);

	// name string
	ASSERT_CAN_READ(len);
	if (len<=sizeof(m_szNames[0]))
	{
		memcpy(m_szNames[0],lpStream+dwMemPos,len);
		dwMemPos += len;
		StringFixer::SetNullTerminator(m_szNames[0]);
	}
	else return false;

	// Song comments

	// comment string length
	ASSERT_CAN_READ(4);
	memcpy(&id,lpStream+dwMemPos,sizeof(DWORD));
	dwMemPos += sizeof(DWORD);
	if(id > uint16_max) return false;

	// allocate and copy comment string
	ASSERT_CAN_READ(id);
	if(id > 0)
	{
		ReadMessage(lpStream + dwMemPos, id - 1, leCR);
	}
	dwMemPos += id;

	// Song global config
	ASSERT_CAN_READ(5*4);

	// m_dwSongFlags
	memcpy(&id,lpStream+dwMemPos,sizeof(DWORD));
	m_dwSongFlags = (id & SONG_FILE_FLAGS);
	dwMemPos += sizeof(DWORD);

	if(!(m_dwSongFlags & SONG_ITPROJECT)) return false;

	// m_nDefaultGlobalVolume
	memcpy(&id,lpStream+dwMemPos,sizeof(DWORD));
	m_nDefaultGlobalVolume = id;
	dwMemPos += sizeof(DWORD);

	// m_nSamplePreAmp
	memcpy(&id,lpStream+dwMemPos,sizeof(DWORD));
	m_nSamplePreAmp = id;
	dwMemPos += sizeof(DWORD);

	// m_nDefaultSpeed
	memcpy(&id,lpStream+dwMemPos,sizeof(DWORD));
	m_nDefaultSpeed = id;
	dwMemPos += sizeof(DWORD);

	// m_nDefaultTempo
	memcpy(&id,lpStream+dwMemPos,sizeof(DWORD));
	m_nDefaultTempo = id;
	dwMemPos += sizeof(DWORD);

	// Song channels data
	ASSERT_CAN_READ(2*4);

	// m_nChannels
	memcpy(&id,lpStream+dwMemPos,sizeof(DWORD));
	m_nChannels = (CHANNELINDEX)id;
	dwMemPos += sizeof(DWORD);
	if(m_nChannels > 127) return false;

	// channel name string length (=MAX_CHANNELNAME)
	memcpy(&id,lpStream+dwMemPos,sizeof(DWORD));
	len = id;
	dwMemPos += sizeof(DWORD);
	if(len > MAX_CHANNELNAME) return false;

	// Channels' data
	for(i=0; i<m_nChannels; i++){
		ASSERT_CAN_READ(3*4 + len);

		// ChnSettings[i].nPan
		memcpy(&id,lpStream+dwMemPos,sizeof(DWORD));
		ChnSettings[i].nPan = id;
		dwMemPos += sizeof(DWORD);

		// ChnSettings[i].dwFlags
		memcpy(&id,lpStream+dwMemPos,sizeof(DWORD));
		ChnSettings[i].dwFlags = id;
		dwMemPos += sizeof(DWORD);

		// ChnSettings[i].nVolume
		memcpy(&id,lpStream+dwMemPos,sizeof(DWORD));
		ChnSettings[i].nVolume = id;
		dwMemPos += sizeof(DWORD);

		// ChnSettings[i].szName
		memcpy(&ChnSettings[i].szName[0],lpStream+dwMemPos,len);
		StringFixer::SetNullTerminator(ChnSettings[i].szName);
		dwMemPos += len;
	}

	// Song mix plugins
	// size of mix plugins data
	ASSERT_CAN_READ(4);
	memcpy(&id,lpStream+dwMemPos,sizeof(DWORD));
	dwMemPos += sizeof(DWORD);

	// mix plugins
	ASSERT_CAN_READ(id);
	dwMemPos += LoadMixPlugins(lpStream+dwMemPos, id);

	// Song midi config

	// midi cfg data length
	ASSERT_CAN_READ(4);
	memcpy(&id,lpStream+dwMemPos,sizeof(DWORD));
	dwMemPos += sizeof(DWORD);

	// midi cfg
	ASSERT_CAN_READ(id);
	if (id <= sizeof(m_MidiCfg))
	{
		memcpy(&m_MidiCfg, lpStream + dwMemPos, id);
		m_MidiCfg.Sanitize();
		dwMemPos += id;
	}

	// Song Instruments

	// m_nInstruments
	ASSERT_CAN_READ(4);
	memcpy(&id,lpStream+dwMemPos,sizeof(DWORD));
	m_nInstruments = (INSTRUMENTINDEX)id;
	if(m_nInstruments > MAX_INSTRUMENTS) return false;
	dwMemPos += sizeof(DWORD);

	// path string length (=_MAX_PATH)
	ASSERT_CAN_READ(4);
	memcpy(&id,lpStream+dwMemPos,sizeof(DWORD));
	len = id;
	if(len > _MAX_PATH) return false;
	dwMemPos += sizeof(DWORD);

	// instruments' paths
	for(i=0; i<m_nInstruments; i++){
		ASSERT_CAN_READ(len);
		memcpy(&m_szInstrumentPath[i][0],lpStream+dwMemPos,len);
		StringFixer::SetNullTerminator(m_szInstrumentPath[i]);
		dwMemPos += len;
	}

	// Song Orders

	// size of order array (=MAX_ORDERS)
	ASSERT_CAN_READ(4);
	memcpy(&id,lpStream+dwMemPos,sizeof(DWORD));
	size = id;
	if(size > MAX_ORDERS) return false;
	dwMemPos += sizeof(DWORD);

	// order data
	ASSERT_CAN_READ(size);
	Order.ReadAsByte(lpStream+dwMemPos, size, dwMemLength-dwMemPos);
	dwMemPos += size;



	// Song Patterns

	ASSERT_CAN_READ(3*4);
	// number of patterns (=MAX_PATTERNS)
	memcpy(&id,lpStream+dwMemPos,sizeof(DWORD));
	size = id;
	dwMemPos += sizeof(DWORD);
	if(size > MAX_PATTERNS) return false;

	// m_nPatternNames
	memcpy(&id,lpStream+dwMemPos,sizeof(DWORD));
	const PATTERNINDEX numNamedPats = static_cast<PATTERNINDEX>(id);
	dwMemPos += sizeof(DWORD);

	// pattern name string length (=MAX_PATTERNNAME)
	memcpy(&id,lpStream+dwMemPos,sizeof(DWORD));
	const DWORD patNameLen = id;
	dwMemPos += sizeof(DWORD);

	// m_lpszPatternNames
	ASSERT_CAN_READ(numNamedPats * patNameLen);
	char *patNames = (char *)(lpStream + dwMemPos);
	dwMemPos += numNamedPats * patNameLen;

	// modcommand data length
	ASSERT_CAN_READ(4);
	memcpy(&id,lpStream+dwMemPos,sizeof(DWORD));
	n = id;
	if(n != 6) return false;
	dwMemPos += sizeof(DWORD);

	for(PATTERNINDEX npat=0; npat<size; npat++)
	{
		// Patterns[npat].GetNumRows()
		ASSERT_CAN_READ(4);
		memcpy(&id,lpStream+dwMemPos,sizeof(DWORD));
		if(id > MAX_PATTERN_ROWS) return false;
		const ROWINDEX nRows = id;
		dwMemPos += sizeof(DWORD);

		// Try to allocate & read only sized patterns
		if(nRows)
		{

			// Allocate pattern
			if(Patterns.Insert(npat, nRows))
			{
				dwMemPos += m_nChannels * Patterns[npat].GetNumRows() * n;
				continue;
			}
			if(npat < numNamedPats && patNameLen > 0)
			{
				Patterns[npat].SetName(patNames, patNameLen);
				patNames += patNameLen;
			}

			// Pattern data
			long datasize = m_nChannels * Patterns[npat].GetNumRows() * n;
			//if (streamPos+datasize<=dwMemLength) {
			if(Patterns[npat].ReadITPdata(lpStream, dwMemPos, datasize, dwMemLength))
			{
				ErrorBox(IDS_ERR_FILEOPEN, NULL);
				return false;
			}
			//memcpy(Patterns[npat],lpStream+streamPos,datasize);
			//streamPos += datasize;
			//}
		}
	}

	// Load embeded samples

	// Read original number of samples
	ASSERT_CAN_READ(4);
	memcpy(&id,lpStream+dwMemPos,sizeof(DWORD));
	if(id >= MAX_SAMPLES) return false;
	m_nSamples = (SAMPLEINDEX)id;
	dwMemPos += sizeof(DWORD);

	// Read number of embeded samples
	ASSERT_CAN_READ(4);
	memcpy(&id,lpStream+dwMemPos,sizeof(DWORD));
	if(id > MAX_SAMPLES) return false;
	n = id;
	dwMemPos += sizeof(DWORD);

	// Read samples
	for(i=0; i<n; i++)
	{

		ASSERT_CAN_READ(4 + sizeof(ITSample) + 4);

		// Sample id number
		memcpy(&id,lpStream+dwMemPos,sizeof(DWORD));
		nsmp = id;
		dwMemPos += sizeof(DWORD);

		if(nsmp < 1 || nsmp >= MAX_SAMPLES)
			return false;

		// Sample struct
		ITSample pis;
		memcpy(&pis, lpStream + dwMemPos, sizeof(ITSample));
		dwMemPos += sizeof(ITSample);

		// Sample length
		memcpy(&id,lpStream+dwMemPos,sizeof(DWORD));
		len = id;
		dwMemPos += sizeof(DWORD);
		if(dwMemPos >= dwMemLength || len > dwMemLength - dwMemPos) return false;

		if(pis.id == LittleEndian(ITSample::magic))
		{
			pis.ConvertToMPT(Samples[nsmp]);

			StringFixer::ReadString<StringFixer::nullTerminated>(m_szNames[nsmp], pis.name);

			// Read sample data
			ReadSample(&Samples[nsmp], pis.GetSampleFormat(), (LPSTR)(lpStream + dwMemPos), len);
			dwMemPos += len;
		}
	}

	// Load instruments

	CMappedFile f;
	LPBYTE lpFile;

	for(INSTRUMENTINDEX i = 0; i < m_nInstruments; i++)
	{

		if(m_szInstrumentPath[i][0] == '\0' || !f.Open(m_szInstrumentPath[i])) continue;

		len = f.GetLength();
		lpFile = f.Lock(len);
		if(!lpFile) { f.Close(); continue; }

		ReadInstrumentFromFile(i+1, lpFile, len);
		f.Unlock();
		f.Close();	
	}

	// Extra info data

	__int32 fcode = 0;
	LPCBYTE ptr = lpStream + min(dwMemPos, dwMemLength);

	if (dwMemPos <= dwMemLength - 4) {
		fcode = (*((__int32 *)ptr));
	}

	// Embed instruments' header [v1.01]
	if(version >= 0x00000101 && m_dwSongFlags & SONG_ITPEMBEDIH && fcode == 'EBIH')
	{
		// jump embeded instrument header tag
		ptr += sizeof(__int32);

		// set first instrument's header as current
		i = 1;

		// parse file
		while( uintptr_t(ptr - lpStream) <= dwMemLength - 4 && i <= m_nInstruments )
		{

			fcode = (*((__int32 *)ptr));			// read field code

			switch( fcode )
			{
			case 'MPTS': goto mpts; //:)		// reached end of instrument headers 
			case 'SEP@': case 'MPTX':
				ptr += sizeof(__int32);			// jump code
				i++;							// switch to next instrument
				break;

			default:
				ptr += sizeof(__int32);			// jump field code
				ReadExtendedInstrumentProperty(Instruments[i], fcode, ptr, lpStream + dwMemLength);
				break;
			}
		}
	}

	//HACK: if we fail on i <= m_nInstruments above, arrive here without having set fcode as appropriate,
	//      hence the code duplication.
	if ( (uintptr_t)(ptr - lpStream) <= dwMemLength - 4 )
	{
		fcode = (*((__int32 *)ptr));
	}

	// Song extensions
mpts:
	if( fcode == 'MPTS' )
		LoadExtendedSongProperties(MOD_TYPE_IT, ptr, lpStream, dwMemLength);

	m_nMaxPeriod = 0xF000;
	m_nMinPeriod = 8;

	UpgradeModFlags();

	return true;
}


#ifndef MODPLUG_NO_FILESAVE

bool CSoundFile::SaveITProject(LPCSTR lpszFileName)
//-------------------------------------------------
{
	// Check song type

	if(!(m_dwSongFlags & SONG_ITPROJECT)) return false;

	UINT i,j = 0;
	for(i = 0 ; i < m_nInstruments ; i++) { if(m_szInstrumentPath[i][0] != '\0' || !Instruments[i+1]) j++; }
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
	id = m_lpszSongComments ? strlen(m_lpszSongComments)+1 : 0;
	fwrite(&id, 1, sizeof(id), f);

	// comment string
	if(m_lpszSongComments) fwrite(&m_lpszSongComments[0], 1, strlen(m_lpszSongComments)+1, f);

	// Song global config

	id = (m_dwSongFlags & SONG_FILE_FLAGS);
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
	for(i=0; i<m_nChannels; i++){
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
	id = sizeof(MIDIMacroConfig);
	fwrite(&id, 1, sizeof(id), f);

	// midi cfg
	fwrite(&m_MidiCfg, 1, sizeof(MIDIMacroConfig), f);

	// Song Instruments

	// number of instruments
	id = m_nInstruments;
	fwrite(&id, 1, sizeof(id), f);

	// path name string length
	id = _MAX_PATH;
	fwrite(&id, 1, sizeof(id), f);

	// instruments' path
	for(i=0; i<m_nInstruments; i++) fwrite(&m_szInstrumentPath[i][0], 1, _MAX_PATH, f);

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
	numNamedPats = min(numNamedPats, MAX_PATTERNS);
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
		Patterns[nPat].GetName(name, MAX_PATTERNNAME);
		fwrite(name, 1, MAX_PATTERNNAME, f);
	}

	// modcommand data length
	id = sizeof(MODCOMMAND_ORIGINAL);
	fwrite(&id, 1, sizeof(id), f);

	// patterns data content
	for(UINT npat=0; npat<MAX_PATTERNS; npat++)
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

	vector<bool> sampleUsed(m_nSamples, false);

	// Mark samples used in instruments
	for(i=0; i<m_nInstruments; i++)
	{
		if(Instruments[i + 1] != nullptr)
		{
			ModInstrument *p = Instruments[i + 1];
			for(j = 0; j < 128; j++)
			{
				if(p->Keyboard[j] > 0 && p->Keyboard[j] <= m_nSamples)
					sampleUsed[p->Keyboard[j] - 1] = true;
			}
		}
	}

	// Count samples not used in any instrument
	i = 0;
	for(j = 1; j <= m_nSamples; j++)
		if(!sampleUsed[j - 1] && Samples[j].pSample) i++;

	id = i;
	fwrite(&id, 1, sizeof(id), f);

	// Write samples not used in any instrument (help, this looks like duplicate code!)
	for(UINT nsmp=1; nsmp<=m_nSamples; nsmp++)
	{
		if(!sampleUsed[nsmp - 1] && Samples[nsmp].pSample)
		{
			ITSample itss;
			itss.ConvertToIT(Samples[nsmp], GetType());

			StringFixer::WriteString<StringFixer::nullTerminated>(itss.name, m_szNames[nsmp]);

			id = nsmp;
			fwrite(&id, 1, sizeof(id), f);

			itss.samplepointer = 0;
			fwrite(&itss, 1, sizeof(itss), f);

			UINT flags = itss.GetSampleFormat();
			id = WriteSample(nullptr, &Samples[nsmp], flags);
			fwrite(&id, 1, sizeof(id), f);
			WriteSample(f, &Samples[nsmp], flags);
		}
	}

	// Embed instruments' header [v1.01]

	if(m_dwSongFlags & SONG_ITPEMBEDIH)
	{
		// embeded instrument header tag
		__int32 code = 'EBIH';
		fwrite(&code, 1, sizeof(__int32), f);

		// instruments' header
		for(i=0; i<m_nInstruments; i++)
		{
			if(Instruments[i+1]) WriteInstrumentHeaderStruct(Instruments[i+1], f);
			// write separator tag
			code = 'SEP@';
			fwrite(&code, 1, sizeof(__int32), f);
		}
	}

	SaveExtendedSongProperties(f);

	// Close file
	fclose(f);
	return true;
}

#endif

