/*
 * Load_itp.cpp
 * ------------
 * Purpose: Load and save Impulse Tracker Project (ITP) files.
 * Notes  : Despite its name, ITP is not a format supported by Impulse Tracker.
 *          In fact, it's a format invented by the OpenMPT team to allow people to work
 *          with the IT format, but keeping the instrument files with big samples separate
 *          from the pattern data, to keep the work files small and handy.
 *          The current design of the format is quite flawed, though, so expect this to
 *          change in the (far?) future.
 * Authors: OpenMPT Devs
 *
 */

#include "stdafx.h"
#include "Loaders.h"
#include "IT_DEFS.H"
#include "../mptrack/mptrack.h"
#include "../mptrack/version.h"


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
		SanitizeMacros();
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

	ITSAMPLESTRUCT pis;

	// Read original number of samples
	ASSERT_CAN_READ(4);
	memcpy(&id,lpStream+dwMemPos,sizeof(DWORD));
	if(id > MAX_SAMPLES) return false;
	m_nSamples = (SAMPLEINDEX)id;
	dwMemPos += sizeof(DWORD);

	// Read number of embeded samples
	ASSERT_CAN_READ(4);
	memcpy(&id,lpStream+dwMemPos,sizeof(DWORD));
	if(id > MAX_SAMPLES) return false;
	n = id;
	dwMemPos += sizeof(DWORD);

	// Read samples
	for(i=0; i<n; i++){

		ASSERT_CAN_READ(4 + sizeof(ITSAMPLESTRUCT) + 4);

		// Sample id number
		memcpy(&id,lpStream+dwMemPos,sizeof(DWORD));
		nsmp = id;
		dwMemPos += sizeof(DWORD);

		if(nsmp < 1 || nsmp >= MAX_SAMPLES)
			return false;

		// Sample struct
		memcpy(&pis,lpStream+dwMemPos,sizeof(ITSAMPLESTRUCT));
		dwMemPos += sizeof(ITSAMPLESTRUCT);

		// Sample length
		memcpy(&id,lpStream+dwMemPos,sizeof(DWORD));
		len = id;
		dwMemPos += sizeof(DWORD);
		if(dwMemPos >= dwMemLength || len > dwMemLength - dwMemPos) return false;

		// Copy sample struct data (ut-oh... this code looks very familiar!)
		if(pis.id == LittleEndian(IT_IMPS))
		{
			MODSAMPLE *pSmp = &Samples[nsmp];
			memcpy(pSmp->filename, pis.filename, 12);
			pSmp->uFlags = 0;
			pSmp->nLength = 0;
			pSmp->nLoopStart = pis.loopbegin;
			pSmp->nLoopEnd = pis.loopend;
			pSmp->nSustainStart = pis.susloopbegin;
			pSmp->nSustainEnd = pis.susloopend;
			pSmp->nC5Speed = pis.C5Speed;
			if(!pSmp->nC5Speed) pSmp->nC5Speed = 8363;
			if(pis.C5Speed < 256) pSmp->nC5Speed = 256;
			pSmp->nVolume = pis.vol << 2;
			if(pSmp->nVolume > 256) pSmp->nVolume = 256;
			pSmp->nGlobalVol = pis.gvl;
			if(pSmp->nGlobalVol > 64) pSmp->nGlobalVol = 64;
			if(pis.flags & 0x10) pSmp->uFlags |= CHN_LOOP;
			if(pis.flags & 0x20) pSmp->uFlags |= CHN_SUSTAINLOOP;
			if(pis.flags & 0x40) pSmp->uFlags |= CHN_PINGPONGLOOP;
			if(pis.flags & 0x80) pSmp->uFlags |= CHN_PINGPONGSUSTAIN;
			pSmp->nPan = (pis.dfp & 0x7F) << 2;
			if(pSmp->nPan > 256) pSmp->nPan = 256;
			if(pis.dfp & 0x80) pSmp->uFlags |= CHN_PANNING;
			pSmp->nVibType = autovibit2xm[pis.vit & 7];
			pSmp->nVibRate = pis.vis;
			pSmp->nVibDepth = pis.vid & 0x7F;
			pSmp->nVibSweep = pis.vir;
			if(pis.length){
				pSmp->nLength = pis.length;
				if (pSmp->nLength > MAX_SAMPLE_LENGTH) pSmp->nLength = MAX_SAMPLE_LENGTH;
				UINT flags = (pis.cvt & 1) ? RS_PCM8S : RS_PCM8U;
				if (pis.flags & 2){
					flags += 5;
					if (pis.flags & 4) flags |= RSF_STEREO;
					pSmp->uFlags |= CHN_16BIT;
				} 
				else{
					if (pis.flags & 4) flags |= RSF_STEREO;
				}
				// Read sample data
				ReadSample(&Samples[nsmp], flags, (LPSTR)(lpStream+dwMemPos), len);
				dwMemPos += len;
				memcpy(m_szNames[nsmp], pis.name, 26);
			}
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

	if(m_dwLastSavedWithVersion < MAKE_VERSION_NUMERIC(1, 17, 2, 50))
	{
		SetModFlag(MSF_COMPATIBLE_PLAY, false);
		SetModFlag(MSF_MIDICC_BUGEMULATION, true);
		SetModFlag(MSF_OLDVOLSWING, true);
	}

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
	id = sizeof(MODMIDICFG);
	fwrite(&id, 1, sizeof(id), f);

	// midi cfg
	fwrite(&m_MidiCfg, 1, sizeof(MODMIDICFG), f);

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
	for(UINT npat=0; npat<MAX_PATTERNS; npat++){
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
			MODINSTRUMENT *p = Instruments[i + 1];
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
	ITSAMPLESTRUCT itss;
	for(UINT nsmp=1; nsmp<=m_nSamples; nsmp++)
	{
		if(!sampleUsed[nsmp - 1] && Samples[nsmp].pSample)
		{

			MODSAMPLE *psmp = &Samples[nsmp];
			memset(&itss, 0, sizeof(itss));
			memcpy(itss.filename, psmp->filename, 12);
			memcpy(itss.name, m_szNames[nsmp], 26);

			itss.id = LittleEndian(IT_IMPS);
			itss.gvl = (BYTE)psmp->nGlobalVol;
			itss.flags = 0x00;

			if(psmp->uFlags & CHN_LOOP) itss.flags |= 0x10;
			if(psmp->uFlags & CHN_SUSTAINLOOP) itss.flags |= 0x20;
			if(psmp->uFlags & CHN_PINGPONGLOOP) itss.flags |= 0x40;
			if(psmp->uFlags & CHN_PINGPONGSUSTAIN) itss.flags |= 0x80;
			itss.C5Speed = psmp->nC5Speed;
			if (!itss.C5Speed) itss.C5Speed = 8363;
			itss.length = psmp->nLength;
			itss.loopbegin = psmp->nLoopStart;
			itss.loopend = psmp->nLoopEnd;
			itss.susloopbegin = psmp->nSustainStart;
			itss.susloopend = psmp->nSustainEnd;
			itss.vol = (BYTE)(psmp->nVolume >> 2);
			itss.dfp = (BYTE)(psmp->nPan >> 2);
			itss.vit = autovibxm2it[psmp->nVibType & 7];
			itss.vis = min(psmp->nVibRate, 64);
			itss.vid = min(psmp->nVibDepth, 32);
			itss.vir = min(psmp->nVibSweep, 255); //(psmp->nVibSweep < 64) ? psmp->nVibSweep * 4 : 255;
			if (psmp->uFlags & CHN_PANNING) itss.dfp |= 0x80;
			if ((psmp->pSample) && (psmp->nLength)) itss.cvt = 0x01;
			UINT flags = RS_PCM8S;

			if(psmp->uFlags & CHN_STEREO)
			{
				flags = RS_STPCM8S;
				itss.flags |= 0x04;
			}
			if(psmp->uFlags & CHN_16BIT)
			{
				itss.flags |= 0x02;
				flags = (psmp->uFlags & CHN_STEREO) ? RS_STPCM16S : RS_PCM16S;
			}

			id = nsmp;
			fwrite(&id, 1, sizeof(id), f);

			itss.samplepointer = NULL;
			fwrite(&itss, 1, sizeof(ITSAMPLESTRUCT), f);

			id = WriteSample(NULL, psmp, flags);
			fwrite(&id, 1, sizeof(id), f);
			WriteSample(f, psmp, flags);
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

