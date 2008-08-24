/*
 * This program is  free software; you can redistribute it  and modify it
 * under the terms of the GNU  General Public License as published by the
 * Free Software Foundation; either version 2  of the license or (at your
 * option) any later version.
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>
*/

#include "stdafx.h"
// -> CODE#0023
// -> DESC="IT project files (.itp)"
#include "../mptrack/mptrack.h"
#include "../mptrack/mainfrm.h"
// -! NEW_FEATURE#0023
#include "sndfile.h"
#include "it_defs.h"
#include "tuningcollection.h"
#include "../mptrack/moddoc.h"
#include <fstream>
#include <strstream>
#include <list>
#include "../mptrack/version.h"
using std::map;
using std::list;
using std::vector;
using std::string;
using std::ofstream;
using std::ifstream;
using std::istrstream;
using namespace srlztn; //SeRiaLiZaTioN

#define str_MBtitle				(GetStrI18N((_TEXT("Saving IT"))))
#define str_tooMuchPatternData	(GetStrI18N((_TEXT("Warning: File format limit was reached. Some pattern data may not get written to file."))))
#define str_pattern				(GetStrI18N((_TEXT("pattern"))))

static bool AreNonDefaultTuningsUsed(CSoundFile& sf)
//--------------------------------------------------
{
	const INSTRUMENTINDEX iCount = sf.GetNumInstruments();
	for(INSTRUMENTINDEX i = 1; i <= iCount; i++)
	{
		if(sf.Headers[i]->pTuning != 0)
			return true;
	}
	return false;
}





class CInstrumentTuningMapStreamer : public srlztn::ABCSerializationStreamer
//=========================================================================
{
public:
	CInstrumentTuningMapStreamer(CSoundFile& csf) : ABCSerializationStreamer(OUTFLAG|INFLAG), m_rSoundfile(csf) {}
	void ProWrite(OUTSTREAM& fout) const
	//----------------------------
	{
		if(m_rSoundfile.GetNumInstruments() > 0)
		{
			//Writing instrument tuning data: first creating
			//tuning name <-> tuning id number map,
			//and then writing the tuning id for every instrument.
			//For example if there are 6 instruments and
			//first half use tuning 'T1', and the other half 
			//tuning 'T2', the output would be something like
			//T1 1 T2 2 1 1 1 2 2 2

			//Creating the tuning address <-> tuning id number map.
			typedef map<CTuning*, uint16> TNTS_MAP;
			typedef TNTS_MAP::iterator TNTS_MAP_ITER;
			TNTS_MAP tNameToShort_Map;
			
			unsigned short figMap = 0;
			for(UINT i = 1; i <= m_rSoundfile.GetNumInstruments(); i++)
			{
				if(m_rSoundfile.Headers[i] == NULL)
				{
					MessageBox(0, "Error: 210807_2", "Error", MB_ICONERROR);
					return;
				}
					
				TNTS_MAP_ITER iter = tNameToShort_Map.find(m_rSoundfile.Headers[i]->pTuning);
				if(iter != tNameToShort_Map.end())
					continue; //Tuning already mapped.
				
				tNameToShort_Map[m_rSoundfile.Headers[i]->pTuning] = figMap;
				figMap++;
			}

			//...and write the map with tuning names replacing
			//the addresses.
			const uint16 tuningMapSize = static_cast<uint16>(tNameToShort_Map.size());
			fout.write(reinterpret_cast<const char*>(&tuningMapSize), sizeof(tuningMapSize));
			for(TNTS_MAP_ITER iter = tNameToShort_Map.begin(); iter != tNameToShort_Map.end(); iter++)
			{
				if(iter->first)
					StringToBinaryStream<uint8>(fout, iter->first->GetName());
				else //Case: Using original IT tuning.
					StringToBinaryStream<uint8>(fout, "->MPT_ORIGINAL_IT<-");

				fout.write(reinterpret_cast<const char*>(&(iter->second)), sizeof(iter->second));
			}

			//Writing tuning data for instruments.
			for(UINT i = 1; i <= m_rSoundfile.GetNumInstruments(); i++)
			{
				TNTS_MAP_ITER iter = tNameToShort_Map.find(m_rSoundfile.Headers[i]->pTuning);
				if(iter == tNameToShort_Map.end()) //Should never happen
				{
					MessageBox(0, "Error: 210807_1", 0, MB_ICONERROR);
					return;
				}
				fout.write(reinterpret_cast<const char*>(&iter->second), sizeof(iter->second));
			}
		}
	}
	void ProRead(INSTREAM& iStrm, const uint64)
	//----------------------------
	{
		ReadInstrumentTunings(m_rSoundfile, iStrm, false);
	}

	static void ReadInstrumentTunings(CSoundFile& csf, INSTREAM& fin, const bool oldTuningmapRead)
	//---------------------------------------------------------------
	{
		typedef map<WORD, string> MAP;
		typedef MAP::iterator MAP_ITER;
		MAP shortToTNameMap;
		if(oldTuningmapRead)
			ReadTuningMap<uint32, uint32>(fin, shortToTNameMap, 100);
		else
			ReadTuningMap<uint16, uint8>(fin, shortToTNameMap);

		//Read & set tunings for instruments
		list<string> notFoundTunings;
		for(UINT i = 1; i<=csf.GetNumInstruments(); i++)
		{
			uint16 ui;
			fin.read(reinterpret_cast<char*>(&ui), sizeof(ui));
			MAP_ITER iter = shortToTNameMap.find(ui);
			if(csf.Headers[i] && iter != shortToTNameMap.end())
			{
				const string str = iter->second;

				if(str == string("->MPT_ORIGINAL_IT<-"))
				{
					csf.Headers[i]->pTuning = NULL;
					continue;
				}

				csf.Headers[i]->pTuning = csf.GetTuneSpecificTunings().GetTuning(str);
				if(csf.Headers[i]->pTuning)
					continue;

				csf.Headers[i]->pTuning = csf.GetLocalTunings().GetTuning(str);
				if(csf.Headers[i]->pTuning)
					continue;

				csf.Headers[i]->pTuning = csf.GetStandardTunings().GetTuning(str);
				if(csf.Headers[i]->pTuning)
					continue;

				if(str == "TET12" && csf.GetStandardTunings().GetNumTunings() > 0)
					csf.Headers[i]->pTuning = &csf.GetStandardTunings().GetTuning(0);

				if(csf.Headers[i]->pTuning)
					continue;

				//Checking if not found tuning already noticed.
				list<string>::iterator iter;
				iter = find(notFoundTunings.begin(), notFoundTunings.end(), str);
				if(iter == notFoundTunings.end())
				{
					notFoundTunings.push_back(str);
					string erm = string("Tuning ") + str + string(" used by the module was not found.");
					MessageBox(0, erm.c_str(), 0, MB_ICONINFORMATION);
					if(csf.GetpModDoc()) //The tuning is changed so the modified flag is set.
						csf.GetpModDoc()->SetModified();
					
				}
				csf.Headers[i]->pTuning = csf.Headers[i]->s_DefaultTuning;

			}
			else //This 'else' happens probably only in case of corrupted file.
			{
				if(csf.Headers[i])
					csf.Headers[i]->pTuning = INSTRUMENTHEADER::s_DefaultTuning;
			}

		}
		//End read&set instrument tunings
	}

	template<class TUNNUMTYPE, class STRSIZETYPE>
	static bool ReadTuningMap(istream& fin, map<uint16, string>& shortToTNameMap, const size_t maxNum = 500)
	//----------------------------------------------------------------------------------------
	{
		typedef map<uint16, string> MAP;
		typedef MAP::iterator MAP_ITER;
		TUNNUMTYPE numTuning = 0;
		fin.read(reinterpret_cast<char*>(&numTuning), sizeof(numTuning));
		if(numTuning > maxNum)
			return true;

		for(size_t i = 0; i<numTuning; i++)
		{
			string temp;
			uint16 ui;
			if(StringFromBinaryStream<STRSIZETYPE>(fin, temp, 255))
				return true;

			fin.read(reinterpret_cast<char*>(&ui), sizeof(ui));
			shortToTNameMap[ui] = temp;
		}
		if(fin.good())
			return false;
		else
			return true;
	}

private:
	CSoundFile& m_rSoundfile;
};




static CSerializationInstructions CreateSerializationInstructions(CSoundFile& sf, uint8 flag)
//----------------------------------------------------------------------------
{
	CSerializationInstructions si("mptm", 1, flag, 3);
	//Adding entries always for reading, and for writing only when there is something to write.

	//0: Tunespecific tunings
	if(flag & INFLAG || sf.GetTuneSpecificTunings().GetNumTunings() > 0) 
		si.AddEntry(CSerializationentry("0", new CTuningCollectionStreamer(sf.GetTuneSpecificTunings())));

	//1: Used definable tunings?
	if(flag & INFLAG || AreNonDefaultTuningsUsed(sf))
		si.AddEntry(CSerializationentry("1", new CInstrumentTuningMapStreamer(sf)));

	//2: Extended order range?
	if(flag & INFLAG || sf.Order.NeedsExtraDatafield())
		si.AddEntry(CSerializationentry("2", sf.Order.NewReadWriteObject()));

	return si;
}




#pragma warning(disable:4244)

BYTE autovibit2xm[8] =
{ 0, 3, 1, 4, 2, 0, 0, 0 };

BYTE autovibxm2it[8] =
{ 0, 2, 4, 1, 3, 0, 0, 0 };

//////////////////////////////////////////////////////////
// Impulse Tracker IT file support


static inline UINT ConvertVolParam(UINT value)
//--------------------------------------------
{
	return (value > 9)  ? 9 : value;
}


//BOOL CSoundFile::ITInstrToMPT(const void *p, INSTRUMENTHEADER *penv, UINT trkvers)
long CSoundFile::ITInstrToMPT(const void *p, INSTRUMENTHEADER *penv, UINT trkvers) //rewbs.modularInstData
//--------------------------------------------------------------------------------
{	
	long returnVal=0;
	penv->pTuning = m_defaultInstrument.pTuning;
	penv->nPluginVelocityHandling = PLUGIN_VELOCITYHANDLING_CHANNEL;
	penv->nPluginVolumeHandling = PLUGIN_VOLUMEHANDLING_IGNORE;
	if (trkvers < 0x0200)
	{
		const ITOLDINSTRUMENT *pis = (const ITOLDINSTRUMENT *)p;
		memcpy(penv->name, pis->name, 26);
		memcpy(penv->filename, pis->filename, 12);
		penv->nFadeOut = pis->fadeout << 6;
		penv->nGlobalVol = 64;
		for (UINT j=0; j<NOTE_MAX; j++)
		{
			UINT note = pis->keyboard[j*2];
			UINT ins = pis->keyboard[j*2+1];
			if (ins < MAX_SAMPLES) penv->Keyboard[j] = ins;
			if (note < 128) penv->NoteMap[j] = note+1;
			else if (note >= 0xFE) penv->NoteMap[j] = note;
		}
		if (pis->flags & 0x01) penv->dwFlags |= ENV_VOLUME;
		if (pis->flags & 0x02) penv->dwFlags |= ENV_VOLLOOP;
		if (pis->flags & 0x04) penv->dwFlags |= ENV_VOLSUSTAIN;
		penv->nVolLoopStart = pis->vls;
		penv->nVolLoopEnd = pis->vle;
		penv->nVolSustainBegin = pis->sls;
		penv->nVolSustainEnd = pis->sle;
		penv->nVolEnv = 25;
		for (UINT ev=0; ev<25; ev++)
		{
			if ((penv->VolPoints[ev] = pis->nodes[ev*2]) == 0xFF)
			{
				penv->nVolEnv = ev;
				break;
			}
			penv->VolEnv[ev] = pis->nodes[ev*2+1];
		}
		penv->nNNA = pis->nna;
		penv->nDCT = pis->dnc;
		penv->nPan = 0x80;
	} else
	{
		const ITINSTRUMENT *pis = (const ITINSTRUMENT *)p;
		memcpy(penv->name, pis->name, 26);
		memcpy(penv->filename, pis->filename, 12);
		if (pis->mpr<=128)
			penv->nMidiProgram = pis->mpr;
		penv->nMidiChannel = pis->mch;
		if (penv->nMidiChannel > 16)	//rewbs.instroVSTi
		{								//(handle old format where midichan
										// and mixplug are 1 value)
			penv->nMixPlug = penv->nMidiChannel-128;
			penv->nMidiChannel = 0;		
		}
		if (pis->mbank<=128)
			penv->wMidiBank = pis->mbank;
		penv->nFadeOut = pis->fadeout << 5;
		penv->nGlobalVol = pis->gbv >> 1;
		if (penv->nGlobalVol > 64) penv->nGlobalVol = 64;
		for (UINT j=0; j<NOTE_MAX; j++)
		{
			UINT note = pis->keyboard[j*2];
			UINT ins = pis->keyboard[j*2+1];
			if (ins < MAX_SAMPLES) penv->Keyboard[j] = ins;
			if (note < 128) penv->NoteMap[j] = note+1;
			else if (note >= 0xFE) penv->NoteMap[j] = note;
		}
		// Olivier's MPT Instrument Extension
		if (*((int *)pis->dummy) == 'MPTX')
		{
			const ITINSTRUMENTEX *pisex = (const ITINSTRUMENTEX *)pis;
			for (UINT k=0; k<NOTE_MAX; k++)
			{
				penv->Keyboard[k] |= ((UINT)pisex->keyboardhi[k] << 8);
			}
		}
		//rewbs.modularInstData  
		//find end of standard header
		BYTE* pEndInstHeader;
		if (*((int *)pis->dummy) == 'MPTX')
			pEndInstHeader=(BYTE*)pis+sizeof(ITINSTRUMENTEX);
		else
			pEndInstHeader=(BYTE*)pis+sizeof(ITINSTRUMENT);

		//If the next piece of data is 'INSM' we have modular extensions to our instrument...
		if ( *( (UINT*)pEndInstHeader ) == 'INSM' )
		{
			//...the next piece of data must be the total size of the modular data
			long modularInstSize = *((long *)(pEndInstHeader+4));
			
			//handle chunks
			BYTE* pModularInst = (BYTE*)(pEndInstHeader+4+sizeof(modularInstSize)); //4 is for 'INSM'
			pEndInstHeader+=4+sizeof(modularInstSize)+modularInstSize;
			while  (pModularInst<pEndInstHeader) //4 is for 'INSM'
			{
				UINT chunkID = *((int *)pModularInst);
				pModularInst+=4;
				switch (chunkID)
				{
					/*case 'DMMY':
						MessageBox(NULL, "Dummy chunk identified", NULL, MB_OK|MB_ICONEXCLAMATION);
						pModularInst+=1024;
						break;*/
					case 'PLUG':
						penv->nMixPlug = *(pModularInst);
						pModularInst+=sizeof(penv->nMixPlug);
						break;
					/*How to load more chunks?  -- see also how to save chunks
					case: 'MYID':
						// handle chunk data, as pointed to by pModularInst
						// move pModularInst as appropriate
						break;
					*/

					default: pModularInst++; //move forward one byte and try to recognize again.

				}
			}
			returnVal = 4+sizeof(modularInstSize)+modularInstSize;
		}
		//end rewbs.modularInstData

			
		// Volume Envelope 
		if (pis->volenv.flags & 1) penv->dwFlags |= ENV_VOLUME;
		if (pis->volenv.flags & 2) penv->dwFlags |= ENV_VOLLOOP;
		if (pis->volenv.flags & 4) penv->dwFlags |= ENV_VOLSUSTAIN;
		if (pis->volenv.flags & 8) penv->dwFlags |= ENV_VOLCARRY;
		penv->nVolEnv = pis->volenv.num;
		if (penv->nVolEnv > 25) penv->nVolEnv = 25;
		penv->nVolLoopStart = pis->volenv.lpb;
		penv->nVolLoopEnd = pis->volenv.lpe;
		penv->nVolSustainBegin = pis->volenv.slb;
		penv->nVolSustainEnd = pis->volenv.sle;
		// Panning Envelope 
		if (pis->panenv.flags & 1) penv->dwFlags |= ENV_PANNING;
		if (pis->panenv.flags & 2) penv->dwFlags |= ENV_PANLOOP;
		if (pis->panenv.flags & 4) penv->dwFlags |= ENV_PANSUSTAIN;
		if (pis->panenv.flags & 8) penv->dwFlags |= ENV_PANCARRY;
		penv->nPanEnv = pis->panenv.num;
		if (penv->nPanEnv > 25) penv->nPanEnv = 25;
		penv->nPanLoopStart = pis->panenv.lpb;
		penv->nPanLoopEnd = pis->panenv.lpe;
		penv->nPanSustainBegin = pis->panenv.slb;
		penv->nPanSustainEnd = pis->panenv.sle;
		// Pitch Envelope 
		if (pis->pitchenv.flags & 1) penv->dwFlags |= ENV_PITCH;
		if (pis->pitchenv.flags & 2) penv->dwFlags |= ENV_PITCHLOOP;
		if (pis->pitchenv.flags & 4) penv->dwFlags |= ENV_PITCHSUSTAIN;
		if (pis->pitchenv.flags & 8) penv->dwFlags |= ENV_PITCHCARRY;
		if (pis->pitchenv.flags & 0x80) penv->dwFlags |= ENV_FILTER;
		penv->nPitchEnv = pis->pitchenv.num;
		if (penv->nPitchEnv > 25) penv->nPitchEnv = 25;
		penv->nPitchLoopStart = pis->pitchenv.lpb;
		penv->nPitchLoopEnd = pis->pitchenv.lpe;
		penv->nPitchSustainBegin = pis->pitchenv.slb;
		penv->nPitchSustainEnd = pis->pitchenv.sle;
		// Envelopes Data
		for (UINT ev=0; ev<25; ev++)
		{
			penv->VolEnv[ev] = pis->volenv.data[ev*3];
			penv->VolPoints[ev] = (pis->volenv.data[ev*3+2] << 8) | (pis->volenv.data[ev*3+1]);
			penv->PanEnv[ev] = pis->panenv.data[ev*3] + 32;
			penv->PanPoints[ev] = (pis->panenv.data[ev*3+2] << 8) | (pis->panenv.data[ev*3+1]);
			penv->PitchEnv[ev] = pis->pitchenv.data[ev*3] + 32;
			penv->PitchPoints[ev] = (pis->pitchenv.data[ev*3+2] << 8) | (pis->pitchenv.data[ev*3+1]);
		}
		penv->nNNA = pis->nna;
		penv->nDCT = pis->dct;
		penv->nDNA = pis->dca;
		penv->nPPS = pis->pps;
		penv->nPPC = pis->ppc;
		penv->nIFC = pis->ifc;
		penv->nIFR = pis->ifr;
		penv->nVolSwing = pis->rv;
		penv->nPanSwing = pis->rp;
		penv->nPan = (pis->dfp & 0x7F) << 2;
		SetDefaultInstrumentValues(penv);
		penv->nPluginVelocityHandling = PLUGIN_VELOCITYHANDLING_CHANNEL;
		penv->nPluginVolumeHandling = PLUGIN_VOLUMEHANDLING_IGNORE;
		if (penv->nPan > 256) penv->nPan = 128;
		if (pis->dfp < 0x80) penv->dwFlags |= ENV_SETPANNING;
	}
	if ((penv->nVolLoopStart >= 25) || (penv->nVolLoopEnd >= 25)) penv->dwFlags &= ~ENV_VOLLOOP;
	if ((penv->nVolSustainBegin >= 25) || (penv->nVolSustainEnd >= 25)) penv->dwFlags &= ~ENV_VOLSUSTAIN;

	return returnVal; //return offset
}

// -> CODE#0023
// -> DESC="IT project files (.itp)"
BOOL CSoundFile::ReadITProject(LPCBYTE lpStream, const DWORD dwMemLength)
//-----------------------------------------------------------------
{
	UINT i,n,nsmp;
	DWORD id,len,size;
	DWORD streamPos = 0;
	DWORD version;

	if(dwMemLength < 12) return FALSE;

// Check file ID

	memcpy(&id,lpStream+streamPos,sizeof(DWORD));
	if(id != 0x2e697470) return FALSE;	// .itp
	m_nType = MOD_TYPE_IT;
	streamPos += sizeof(DWORD);

	memcpy(&id,lpStream+streamPos,sizeof(DWORD));
	version = id;
	streamPos += sizeof(DWORD);

// Song name

	// name string length
	memcpy(&id,lpStream+streamPos,sizeof(DWORD));
	len = id;
	streamPos += sizeof(DWORD);

	// name string
	if (streamPos+len<=dwMemLength && len<=sizeof(m_szNames[0])) {
		memcpy(m_szNames[0],lpStream+streamPos,len);
		streamPos += len;
		m_szNames[0][sizeof(m_szNames[0])-1] = '\0';
	}
	else return FALSE;

// Song comments

	// comment string length
	if(streamPos + 4 > dwMemLength) return FALSE;
	memcpy(&id,lpStream+streamPos,sizeof(DWORD));
	streamPos += sizeof(DWORD);
	if(id > uint16_max) return FALSE;

	// allocate comment string
	if(m_lpszSongComments) delete[] m_lpszSongComments;
	if (id < dwMemLength && id > 0)
	{
		m_lpszSongComments = new char[id];
	}
	else
		m_lpszSongComments = NULL;

	// m_lpszSongComments
	if (m_lpszSongComments && id && streamPos + id <= dwMemLength) {
		memcpy(&m_lpszSongComments[0],lpStream+streamPos,id);
		streamPos += id;
	}

// Song global config
	if(streamPos + 5*4 > dwMemLength) return FALSE;

	// m_dwSongFlags
	memcpy(&id,lpStream+streamPos,sizeof(DWORD));
	m_dwSongFlags = id;
	streamPos += sizeof(DWORD);

	if(!(m_dwSongFlags & SONG_ITPROJECT)) return FALSE;

	// m_nDefaultGlobalVolume
	memcpy(&id,lpStream+streamPos,sizeof(DWORD));
	m_nDefaultGlobalVolume = id;
	streamPos += sizeof(DWORD);

	// m_nSamplePreAmp
	memcpy(&id,lpStream+streamPos,sizeof(DWORD));
	m_nSamplePreAmp = id;
	streamPos += sizeof(DWORD);

	// m_nDefaultSpeed
	memcpy(&id,lpStream+streamPos,sizeof(DWORD));
	m_nDefaultSpeed = id;
	streamPos += sizeof(DWORD);

	// m_nDefaultTempo
	memcpy(&id,lpStream+streamPos,sizeof(DWORD));
	m_nDefaultTempo = id;
	streamPos += sizeof(DWORD);

// Song channels data
	if(streamPos + 2*4 > dwMemLength) return FALSE;

	// m_nChannels
	memcpy(&id,lpStream+streamPos,sizeof(DWORD));
	m_nChannels = id;
	streamPos += sizeof(DWORD);
	if(m_nChannels > 127) return FALSE;

	// channel name string length (=MAX_CHANNELNAME)
	memcpy(&id,lpStream+streamPos,sizeof(DWORD));
	len = id;
	streamPos += sizeof(DWORD);
	if(len > MAX_CHANNELNAME) return FALSE;

	// Channels' data
	for(i=0; i<m_nChannels; i++){

		if(streamPos + 3*4 + len > dwMemLength) return FALSE;

		// ChnSettings[i].nPan
		memcpy(&id,lpStream+streamPos,sizeof(DWORD));
		ChnSettings[i].nPan = id;
		streamPos += sizeof(DWORD);

		// ChnSettings[i].dwFlags
		memcpy(&id,lpStream+streamPos,sizeof(DWORD));
		ChnSettings[i].dwFlags = id;
		streamPos += sizeof(DWORD);

		// ChnSettings[i].nVolume
		memcpy(&id,lpStream+streamPos,sizeof(DWORD));
		ChnSettings[i].nVolume = id;
		streamPos += sizeof(DWORD);

		// ChnSettings[i].szName
		memcpy(&ChnSettings[i].szName[0],lpStream+streamPos,len);
		streamPos += len;
	}

// Song mix plugins
	// size of mix plugins data
	if(streamPos + 4 > dwMemLength) return FALSE;
	memcpy(&id,lpStream+streamPos,sizeof(DWORD));
	streamPos += sizeof(DWORD);

	// mix plugins
	if(id > dwMemLength - streamPos) return FALSE;
	streamPos += LoadMixPlugins(lpStream+streamPos, id);

// Song midi config

	// midi cfg data length
	if(streamPos + 4 > dwMemLength) return FALSE;
	memcpy(&id,lpStream+streamPos,sizeof(DWORD));
	streamPos += sizeof(DWORD);

	// midi cfg
	if (id<=sizeof(m_MidiCfg) && id+streamPos<=dwMemLength) {
		memcpy(&m_MidiCfg,lpStream+streamPos,id);
		streamPos += id;
	}

// Song Instruments

	// m_nInstruments
	if(streamPos + 4 > dwMemLength) return FALSE;
	memcpy(&id,lpStream+streamPos,sizeof(DWORD));
	m_nInstruments = id;
	if(m_nInstruments > 256) return FALSE;
	streamPos += sizeof(DWORD);

	// path string length (=_MAX_PATH)
	if(streamPos + 4 > dwMemLength) return FALSE;
	memcpy(&id,lpStream+streamPos,sizeof(DWORD));
	len = id;
	if(len > _MAX_PATH) return FALSE;
	streamPos += sizeof(DWORD);

	// instruments' paths
	for(i=0; i<m_nInstruments; i++){
		if (len+streamPos<=dwMemLength)
		{
			memcpy(&m_szInstrumentPath[i][0],lpStream+streamPos,len);
			streamPos += len;
		}
		else return FALSE;
	}

// Song Orders

	// size of order array (=MAX_ORDERS)
	if(streamPos + 4 > dwMemLength) return FALSE;
	memcpy(&id,lpStream+streamPos,sizeof(DWORD));
	size = id;
	if(size > MAX_ORDERS) return FALSE;
	streamPos += sizeof(DWORD);

	// order data
	if (size+streamPos<=dwMemLength)
	{
		Order.ReadAsByte(lpStream+streamPos, size, dwMemLength-streamPos);
		streamPos += size;
	}
	else return FALSE;
	

// Song Patterns

	if(streamPos + 12 > dwMemLength) return FALSE;
	// number of patterns (=MAX_PATTERNS)
	memcpy(&id,lpStream+streamPos,sizeof(DWORD));
	size = id;
	streamPos += sizeof(DWORD);
	if(size > MAX_PATTERNS) return FALSE;

	// m_nPatternNames
	memcpy(&id,lpStream+streamPos,sizeof(DWORD));
	m_nPatternNames = id;
	streamPos += sizeof(DWORD);

	// pattern name string length (=MAX_PATTERNNAME)
	memcpy(&id,lpStream+streamPos,sizeof(DWORD));
	len = id;
	streamPos += sizeof(DWORD);

	// m_lpszPatternNames
	if (len<=MAX_PATTERNNAME && m_nPatternNames<=MAX_PATTERNS) 
	{
		m_lpszPatternNames = new char[m_nPatternNames * len];
		memcpy(&m_lpszPatternNames[0],lpStream+streamPos,m_nPatternNames * len);
	}
	else return FALSE;

	streamPos += m_nPatternNames * len;

	// modcommand data length
	if(streamPos + 4 > dwMemLength) return FALSE;
	memcpy(&id,lpStream+streamPos,sizeof(DWORD));
	n = id;
	streamPos += sizeof(DWORD);

	for(UINT npat=0; npat<size; npat++){

		// Free pattern if not empty
		if(Patterns[npat]) { FreePattern(Patterns[npat]); Patterns[npat] = NULL; }

		// PatternSize[npat]
		if(streamPos + 4 > dwMemLength) return FALSE;
		memcpy(&id,lpStream+streamPos,sizeof(DWORD));
		if(id > 1024) return FALSE;
		Patterns[npat].Resize(id);
		streamPos += sizeof(DWORD);

		// Try to allocate & read only sized patterns
		if(PatternSize[npat]){

			// Allocate pattern
			if(Patterns.Insert(npat, PatternSize[npat])){
				streamPos += m_nChannels * PatternSize[npat] * n;
				continue;
			}

			// Pattern data
			long datasize = m_nChannels * PatternSize[npat] * n;
			//if (streamPos+datasize<=dwMemLength) {
			if(Patterns[npat].ReadITPdata(lpStream, streamPos, datasize, dwMemLength))
			{
				ErrorBox(IDS_ERR_FILEOPEN, NULL);
				return FALSE;
			}
				//memcpy(Patterns[npat],lpStream+streamPos,datasize);
				//streamPos += datasize;
			//}
		}
	}

// Load embeded samples

	ITSAMPLESTRUCT pis;

	// Read original number of samples
	if(streamPos + 4 > dwMemLength) return FALSE;
	memcpy(&id,lpStream+streamPos,sizeof(DWORD));
	if(id > MAX_SAMPLES) return FALSE;
	m_nSamples = id;
	streamPos += sizeof(DWORD);

	// Read number of embeded samples
	if(streamPos + 4 > dwMemLength) return FALSE;
	memcpy(&id,lpStream+streamPos,sizeof(DWORD));
	if(id > MAX_SAMPLES) return FALSE;
	n = id;
	streamPos += sizeof(DWORD);

	// Read samples
	for(i=0; i<n; i++){

		if(streamPos + 4 + sizeof(ITSAMPLESTRUCT) + 4 > dwMemLength) return FALSE;

		// Sample id number
		memcpy(&id,lpStream+streamPos,sizeof(DWORD));
		nsmp = id;
		streamPos += sizeof(DWORD);

		// Sample struct
		memcpy(&pis,lpStream+streamPos,sizeof(ITSAMPLESTRUCT));
		streamPos += sizeof(ITSAMPLESTRUCT);

		// Sample length
		memcpy(&id,lpStream+streamPos,sizeof(DWORD));
		len = id;
		streamPos += sizeof(DWORD);
		if(streamPos >= dwMemLength || len > dwMemLength - streamPos) return FALSE;

		// Copy sample struct data
		if(pis.id == 0x53504D49){
			MODINSTRUMENT *pins = &Ins[nsmp];
			memcpy(pins->name, pis.filename, 12);
			pins->uFlags = 0;
			pins->nLength = 0;
			pins->nLoopStart = pis.loopbegin;
			pins->nLoopEnd = pis.loopend;
			pins->nSustainStart = pis.susloopbegin;
			pins->nSustainEnd = pis.susloopend;
			pins->nC4Speed = pis.C5Speed;
			if(!pins->nC4Speed) pins->nC4Speed = 8363;
			if(pis.C5Speed < 256) pins->nC4Speed = 256;
			pins->nVolume = pis.vol << 2;
			if(pins->nVolume > 256) pins->nVolume = 256;
			pins->nGlobalVol = pis.gvl;
			if(pins->nGlobalVol > 64) pins->nGlobalVol = 64;
			if(pis.flags & 0x10) pins->uFlags |= CHN_LOOP;
			if(pis.flags & 0x20) pins->uFlags |= CHN_SUSTAINLOOP;
			if(pis.flags & 0x40) pins->uFlags |= CHN_PINGPONGLOOP;
			if(pis.flags & 0x80) pins->uFlags |= CHN_PINGPONGSUSTAIN;
			pins->nPan = (pis.dfp & 0x7F) << 2;
			if(pins->nPan > 256) pins->nPan = 256;
			if(pis.dfp & 0x80) pins->uFlags |= CHN_PANNING;
			pins->nVibType = autovibit2xm[pis.vit & 7];
			pins->nVibRate = pis.vis;
			pins->nVibDepth = pis.vid & 0x7F;
			pins->nVibSweep = (pis.vir + 3) / 4;
			if(pis.length){
				pins->nLength = pis.length;
				if (pins->nLength > MAX_SAMPLE_LENGTH) pins->nLength = MAX_SAMPLE_LENGTH;
				UINT flags = (pis.cvt & 1) ? RS_PCM8S : RS_PCM8U;
				if (pis.flags & 2){
					flags += 5;
					if (pis.flags & 4) flags |= RSF_STEREO;
					pins->uFlags |= CHN_16BIT;
				} 
				else{
					if (pis.flags & 4) flags |= RSF_STEREO;
				}
				// Read sample data
				ReadSample(&Ins[nsmp], flags, (LPSTR)(lpStream+streamPos), len);
				streamPos += len;
				memcpy(m_szNames[nsmp], pis.name, 26);
			}
		}
	}

// Load instruments

	CMappedFile f;
	LPBYTE lpFile;

	for(i=0; i<m_nInstruments; i++){

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
	__int16 fsize = 0;
	BYTE * ptr = (BYTE *)(lpStream + streamPos);

	if (streamPos + 4 <= dwMemLength) {
		fcode = (*((__int32 *)ptr));
	}

// Embed instruments' header [v1.01]
	if(version >= 0x00000101 && m_dwSongFlags & SONG_ITPEMBEDIH && fcode == 'EBIH'){
		// jump embeded instrument header tag
		ptr += sizeof(__int32);

		// set first instrument's header as current
		i = 1;

		// parse file
		while( ptr + 4 <= (BYTE *)(lpStream + dwMemLength) && i <= m_nInstruments ){

			fcode = (*((__int32 *)ptr));			// read field code

			switch( fcode ){
				case 'MPTS': goto mpts; //:)		// reached end of instrument headers 
				case 'SEP@': case 'MPTX':
					ptr += sizeof(__int32);			// jump code
					i++;							// switch to next instrument
				break;

				default:
					ptr += sizeof(__int32);			// jump field code
					if(ptr + 2 > (BYTE *)(lpStream + dwMemLength)) return FALSE;
					fsize = (*((__int16 *)ptr));	// read field size
					ptr += sizeof(__int16);			// jump field size
					BYTE * fadr = GetInstrumentHeaderFieldPointer(Headers[i], fcode, fsize);
					if(fadr && fcode != 'K[..')		// copy field data in instrument's header
					{
						if(ptr + fsize > (BYTE *)(lpStream + dwMemLength)) return FALSE;
						memcpy(fadr,ptr,fsize);		// (except for keyboard mapping)
					}
					ptr += fsize;					// jump field
				break;
			}
		}
	}

	//HACK: if we fail on i <= m_nInstruments above, arrive here without having set fcode as appropriate,
	//      hence the code duplication.
	if (ptr + 4 <= (BYTE *)(lpStream + dwMemLength)) {
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
		SetModFlag(MSF_MIDICC_BUGEMULATION, true);
		SetModFlag(MSF_OLDVOLSWING, true);
	}

	return TRUE;
}
// -! NEW_FEATURE#0023

BOOL CSoundFile::ReadIT(const BYTE *lpStream, const DWORD dwMemLength)
//--------------------------------------------------------------
{
	ITFILEHEADER *pifh = (ITFILEHEADER *)lpStream;

	DWORD dwMemPos = sizeof(ITFILEHEADER);
	DWORD inspos[MAX_INSTRUMENTS];
	DWORD smppos[MAX_SAMPLES];
	vector<DWORD> patpos; patpos.resize(Patterns.Size(), 0);
// Using eric's code here to take care of NNAs etc..
// -> CODE#0006
// -> DESC="misc quantity changes"
//	BYTE chnmask[64], channels_used[64];
//	MODCOMMAND lastvalue[64];
	BYTE chnmask[MAX_BASECHANNELS], channels_used[MAX_BASECHANNELS];
	MODCOMMAND lastvalue[MAX_BASECHANNELS];
// -! BEHAVIOUR_CHANGE#0006

	bool interpretModplugmade = false;
	bool hasModplugExtensions = false;

	if ((!lpStream) || (dwMemLength < 0x100)) return FALSE;
	if ((pifh->id != 0x4D504D49 && pifh->id != 0x2e6D7074) || (pifh->insnum > 0xFF)
	 || (!pifh->smpnum) || (pifh->smpnum >= MAX_SAMPLES) || (!pifh->ordnum)) return FALSE;
	if (dwMemPos + pifh->ordnum + pifh->insnum*4
	 + pifh->smpnum*4 + pifh->patnum*4 > dwMemLength) return FALSE;


	DWORD mptStartPos = dwMemLength;
	memcpy(&mptStartPos, lpStream + (dwMemLength - sizeof(DWORD)), sizeof(DWORD));
	if(mptStartPos >= dwMemLength || mptStartPos < 0x100)
		mptStartPos = dwMemLength;

	if(pifh->id == 0x2e6D7074)
	{
		ChangeModTypeTo(MOD_TYPE_MPT);
	}
	else
	{
		if(mptStartPos <= dwMemLength - 3 && pifh->cwtv > 0x888)
		{
			char temp[3];
			const char ID[3] = {'2','2','8'};
			memcpy(temp, lpStream + mptStartPos, 3);
			if(!memcmp(temp, ID, 3)) ChangeModTypeTo(MOD_TYPE_MPT);
			else ChangeModTypeTo(MOD_TYPE_IT);
		}
		else ChangeModTypeTo(MOD_TYPE_IT);

		if(GetType() == MOD_TYPE_IT)
		{
			if(pifh->cmwt == 0x888 || pifh->cwtv == 0x888 || 
			(pifh->cwtv == 0x217 && pifh->cmwt == 0x200)) interpretModplugmade = true;
			//TODO: Check whether above interpretation is reasonable especially for 
			//values 0x217 and 0x200 which are the values used in 1.16. 
		}
	}
	
	if(GetType() == MOD_TYPE_IT) mptStartPos = dwMemLength;

	if (pifh->flags & 0x08) m_dwSongFlags |= SONG_LINEARSLIDES;
	if (pifh->flags & 0x10) m_dwSongFlags |= SONG_ITOLDEFFECTS;
	if (pifh->flags & 0x20) m_dwSongFlags |= SONG_ITCOMPATMODE;
	if (pifh->flags & 0x80) m_dwSongFlags |= SONG_EMBEDMIDICFG;
	if (pifh->flags & 0x1000) m_dwSongFlags |= SONG_EXFILTERRANGE;
	memcpy(m_szNames[0], pifh->songname, 26);
	m_szNames[0][26] = 0;
	// Global Volume
	if (pifh->globalvol)
	{
		m_nDefaultGlobalVolume = pifh->globalvol << 1;
		if (!m_nDefaultGlobalVolume) m_nDefaultGlobalVolume = 256;
		if (m_nDefaultGlobalVolume > 256) m_nDefaultGlobalVolume = 256;
	}
	if (pifh->speed) m_nDefaultSpeed = pifh->speed;
	if (pifh->tempo) m_nDefaultTempo = pifh->tempo;
	m_nSamplePreAmp = pifh->mv & 0x7F;
	if (m_nSamplePreAmp<0x20) {
		m_nSamplePreAmp=100;
	}
	// Reading Channels Pan Positions
	for (int ipan=0; ipan</*MAX_BASECHANNELS*/64; ipan++) if (pifh->chnpan[ipan] != 0xFF) //Header only has room for settings for 64 chans...		
	{
		ChnSettings[ipan].nVolume = pifh->chnvol[ipan];
		ChnSettings[ipan].nPan = 128;
		if (pifh->chnpan[ipan] & 0x80) ChnSettings[ipan].dwFlags |= CHN_MUTE;
		UINT n = pifh->chnpan[ipan] & 0x7F;
		if (n <= 64) ChnSettings[ipan].nPan = n << 2;
		if (n == 100) ChnSettings[ipan].dwFlags |= CHN_SURROUND;
	}
	if (m_nChannels < GetModSpecifications().channelsMin) m_nChannels = GetModSpecifications().channelsMin;

	// Reading Song Message
	if ((pifh->special & 0x01) && (pifh->msglength) && (pifh->msgoffset + pifh->msglength < dwMemLength))
	{
		m_lpszSongComments = new char[pifh->msglength+1];
		if (m_lpszSongComments)
		{
			memcpy(m_lpszSongComments, lpStream+pifh->msgoffset, pifh->msglength);
			m_lpszSongComments[pifh->msglength] = 0;
		}
	}
	// Reading orders
	UINT nordsize = pifh->ordnum;
	if(GetType() == MOD_TYPE_IT)
	{
		if(nordsize > MAX_ORDERS) nordsize = MAX_ORDERS;
		Order.ReadAsByte(lpStream + dwMemPos, nordsize, dwMemLength-dwMemPos);
		dwMemPos += pifh->ordnum;
	}
	else
	{
		if(nordsize > GetModSpecifications().ordersMax) nordsize = GetModSpecifications().ordersMax;

		if(pifh->cwtv > 0x88A && pifh->cwtv <= 0x88D)
			dwMemPos += Order.Unserialize(lpStream+dwMemPos, dwMemLength-dwMemPos);
		else
		{	
			Order.ReadAsByte(lpStream + dwMemPos, nordsize, dwMemLength - dwMemPos);
			dwMemPos += pifh->ordnum;
			//Replacing 0xFF and 0xFE with new corresponding indexes
			replace(Order.begin(), Order.end(), static_cast<PATTERNINDEX>(0xFE), Order.GetIgnoreIndex());
			replace(Order.begin(), Order.end(), static_cast<PATTERNINDEX>(0xFF), Order.GetInvalidPatIndex());
		}
	}

	// Reading Instrument Offsets
	memset(inspos, 0, sizeof(inspos));
	UINT inspossize = pifh->insnum;
	if (inspossize > MAX_INSTRUMENTS) inspossize = MAX_INSTRUMENTS;
	inspossize <<= 2;
	memcpy(inspos, lpStream+dwMemPos, inspossize);
	dwMemPos += pifh->insnum * 4;
	// Reading Samples Offsets
	memset(smppos, 0, sizeof(smppos));
	UINT smppossize = pifh->smpnum;
	if (smppossize > MAX_SAMPLES) smppossize = MAX_SAMPLES;
	smppossize <<= 2;
	memcpy(smppos, lpStream+dwMemPos, smppossize);
	dwMemPos += pifh->smpnum * 4;
	// Reading Patterns Offsets
	UINT patpossize = pifh->patnum;
	if(patpossize > GetModSpecifications().patternsMax) patpossize = GetModSpecifications().patternsMax;

	patpos.resize(patpossize);
	patpossize *= 4; // <-> patpossize *= sizeof(DWORD);
	if(patpossize > dwMemLength - dwMemPos)
		return FALSE;
	if(patpossize > 0)
		memcpy(&patpos[0], lpStream+dwMemPos, patpossize);
	
	
	dwMemPos += pifh->patnum * 4;
	// Reading IT Extra Info
	if (dwMemPos + 2 < dwMemLength)
	{
		UINT nflt = *((WORD *)(lpStream + dwMemPos));
		dwMemPos += 2;
		if (nflt * 8 < dwMemLength - dwMemPos) dwMemPos += nflt * 8;
	}
	// Reading Midi Output & Macros
	if (m_dwSongFlags & SONG_EMBEDMIDICFG)
	{
		if (dwMemPos + sizeof(MODMIDICFG) < dwMemLength)
		{
			memcpy(&m_MidiCfg, lpStream+dwMemPos, sizeof(MODMIDICFG));
			dwMemPos += sizeof(MODMIDICFG);
		}
	}
	// Read pattern names: "PNAM"
	if ((dwMemPos + 8 < dwMemLength) && (*((DWORD *)(lpStream+dwMemPos)) == 0x4d414e50))
	{
		UINT len = *((DWORD *)(lpStream+dwMemPos+4));
		dwMemPos += 8;
		if ((dwMemPos + len <= dwMemLength) && (len <= patpos.size()*MAX_PATTERNNAME) && (len >= MAX_PATTERNNAME))
		{
			m_lpszPatternNames = new char[len];
			if (m_lpszPatternNames)
			{
				m_nPatternNames = len / MAX_PATTERNNAME;
				memcpy(m_lpszPatternNames, lpStream+dwMemPos, len);
			}
			dwMemPos += len;
		}
	}

	m_nChannels = GetModSpecifications().channelsMin;
	// Read channel names: "CNAM"
	if ((dwMemPos + 8 < dwMemLength) && (*((DWORD *)(lpStream+dwMemPos)) == 0x4d414e43))
	{
		UINT len = *((DWORD *)(lpStream+dwMemPos+4));
		dwMemPos += 8;
		if ((dwMemPos + len <= dwMemLength) && (len <= MAX_BASECHANNELS*MAX_CHANNELNAME))
		{
			UINT n = len / MAX_CHANNELNAME;
			if (n > m_nChannels) m_nChannels = n;
			for (UINT i=0; i<n; i++)
			{
				memcpy(ChnSettings[i].szName, (lpStream+dwMemPos+i*MAX_CHANNELNAME), MAX_CHANNELNAME);
				ChnSettings[i].szName[MAX_CHANNELNAME-1] = 0;
			}
			dwMemPos += len;
		}
	}
	// Read mix plugins information
	if (dwMemPos + 8 < dwMemLength) 
	{
		dwMemPos += LoadMixPlugins(lpStream+dwMemPos, dwMemLength-dwMemPos);
	}
	
	//UINT npatterns = pifh->patnum;
	UINT npatterns = patpos.size();
	
	if (npatterns > GetModSpecifications().patternsMax) 
		npatterns = GetModSpecifications().patternsMax;

	// Checking for unused channels
	for (UINT patchk=0; patchk<npatterns; patchk++)
	{
		memset(chnmask, 0, sizeof(chnmask));

		if ((!patpos[patchk]) || ((DWORD)patpos[patchk] >= dwMemLength - 4)) 
			continue;

		UINT len = *((WORD *)(lpStream+patpos[patchk]));
		UINT rows = *((WORD *)(lpStream+patpos[patchk]+2));

		if(rows <= ModSpecs::it_ext.patternRowsMax && rows > ModSpecs::it.patternRowsMax)
		{
			interpretModplugmade = true;
			hasModplugExtensions = true;
		}

		if ((rows < GetModSpecifications().patternRowsMin) || (rows > GetModSpecifications().patternRowsMax)) 
			continue;

		if (patpos[patchk]+8+len > dwMemLength) 
			continue;

		UINT i = 0;
		const BYTE *p = lpStream+patpos[patchk]+8;
		UINT nrow = 0;
		
		while (nrow<rows)
		{
			if (i >= len) break;
			BYTE b = p[i++]; // p is the bytestream offset at current pattern_position
			if (!b)
			{
				nrow++;
				continue;
			}

			UINT ch = b & IT_bitmask_patternChanField_c;   // 0x7f We have some data grab a byte keeping only 127 bits
			if (ch) 
				ch = (ch - 1);// & IT_bitmask_patternChanMask_c;   // 0x3f mask of the byte again, keeping only 64 bits

			if (b & IT_bitmask_patternChanEnabled_c)            // 0x80 check if the upper bit is enabled.
			{
				if (i >= len)               
				    break;        
				chnmask[ch] = p[i++];       // set the channel mask for this channel.
			}
			// Channel used
			if (chnmask[ch] & 0x0F)         // if this channel is used set m_nChannels
			{
// -> CODE#0006
// -> DESC="misc quantity changes"
//				if ((ch >= m_nChannels) && (ch < 64)) m_nChannels = ch+1;
				if ((ch >= m_nChannels) && (ch < MAX_BASECHANNELS)) m_nChannels = ch+1;
// -! BEHAVIOUR_CHANGE#0006
			}
			// Now we actually update the pattern-row entry the note,instrument etc.
			// Note          
			if (chnmask[ch] & 1) i++;
			// Instrument
			if (chnmask[ch] & 2) i++;
			// Volume
			if (chnmask[ch] & 4) i++;
			// Effect
			if (chnmask[ch] & 8) i += 2;
			if (i >= len) break;
		}
	}
	// Reading Instruments
	m_nInstruments = 0;
	if (pifh->flags & 0x04) m_nInstruments = pifh->insnum;
	if (m_nInstruments >= MAX_INSTRUMENTS) m_nInstruments = MAX_INSTRUMENTS-1;
	for (UINT nins=0; nins<m_nInstruments; nins++)
	{
		if ((inspos[nins] > 0) && (inspos[nins] < dwMemLength - sizeof(ITOLDINSTRUMENT)))
		{
			INSTRUMENTHEADER *penv = new INSTRUMENTHEADER;
			if (!penv) continue;
			Headers[nins+1] = penv;
			memset(penv, 0, sizeof(INSTRUMENTHEADER));
			ITInstrToMPT(lpStream + inspos[nins], penv, pifh->cmwt);
		}
	}

// -> CODE#0027
// -> DESC="per-instrument volume ramping setup (refered as attack)"
	// In order to properly compute the position, in file, of eventual extended settings
	// such as "attack" we need to keep the "real" size of the last sample as those extra
	// setting will follow this sample in the file
	UINT lastSampleSize = 0;
// -! NEW_FEATURE#0027

	// Reading Samples
	m_nSamples = pifh->smpnum;
	if (m_nSamples >= MAX_SAMPLES) m_nSamples = MAX_SAMPLES-1;
	for (UINT nsmp=0; nsmp<pifh->smpnum; nsmp++) if ((smppos[nsmp]) && (smppos[nsmp] + sizeof(ITSAMPLESTRUCT) <= dwMemLength))
	{
		lastSampleSize = 0; //ensure lastsamplesize = 0 if last sample is empty, else we'll skip the MPTX stuff.
		ITSAMPLESTRUCT *pis = (ITSAMPLESTRUCT *)(lpStream+smppos[nsmp]);
		if (pis->id == 0x53504D49)
		{
			MODINSTRUMENT *pins = &Ins[nsmp+1];
			memcpy(pins->name, pis->filename, 12);
			pins->uFlags = 0;
			pins->nLength = 0;
			pins->nLoopStart = pis->loopbegin;
			pins->nLoopEnd = pis->loopend;
			pins->nSustainStart = pis->susloopbegin;
			pins->nSustainEnd = pis->susloopend;
			pins->nC4Speed = pis->C5Speed;
			if (!pins->nC4Speed) pins->nC4Speed = 8363;
			if (pis->C5Speed < 256) pins->nC4Speed = 256;
			pins->nVolume = pis->vol << 2;
			if (pins->nVolume > 256) pins->nVolume = 256;
			pins->nGlobalVol = pis->gvl;
			if (pins->nGlobalVol > 64) pins->nGlobalVol = 64;
			if (pis->flags & 0x10) pins->uFlags |= CHN_LOOP;
			if (pis->flags & 0x20) pins->uFlags |= CHN_SUSTAINLOOP;
			if (pis->flags & 0x40) pins->uFlags |= CHN_PINGPONGLOOP;
			if (pis->flags & 0x80) pins->uFlags |= CHN_PINGPONGSUSTAIN;
			pins->nPan = (pis->dfp & 0x7F) << 2;
			if (pins->nPan > 256) pins->nPan = 256;
			if (pis->dfp & 0x80) pins->uFlags |= CHN_PANNING;
			pins->nVibType = autovibit2xm[pis->vit & 7];
			pins->nVibRate = pis->vis;
			pins->nVibDepth = pis->vid & 0x7F;
			pins->nVibSweep = (pis->vir + 3) / 4;
			if ((pis->samplepointer) && (pis->samplepointer < dwMemLength) && (pis->length))
			{
				pins->nLength = pis->length;
				if (pins->nLength > MAX_SAMPLE_LENGTH) pins->nLength = MAX_SAMPLE_LENGTH;
				UINT flags = (pis->cvt & 1) ? RS_PCM8S : RS_PCM8U;
				if (pis->flags & 2)
				{
					flags += 5;
					if (pis->flags & 4) flags |= RSF_STEREO;
					pins->uFlags |= CHN_16BIT;
					// IT 2.14 16-bit packed sample ?
					if (pis->flags & 8) flags = ((pifh->cmwt >= 0x215) && (pis->cvt & 4)) ? RS_IT21516 : RS_IT21416;
				} else
				{
					if (pis->flags & 4) flags |= RSF_STEREO;
					if (pis->cvt == 0xFF) flags = RS_ADPCM4; else
					// IT 2.14 8-bit packed sample ?
					if (pis->flags & 8)	flags =	((pifh->cmwt >= 0x215) && (pis->cvt & 4)) ? RS_IT2158 : RS_IT2148;
				}
// -> CODE#0027
// -> DESC="per-instrument volume ramping setup (refered as attack)"
//				ReadSample(&Ins[nsmp+1], flags, (LPSTR)(lpStream+pis->samplepointer), dwMemLength - pis->samplepointer);
				lastSampleSize = ReadSample(&Ins[nsmp+1], flags, (LPSTR)(lpStream+pis->samplepointer), dwMemLength - pis->samplepointer);
// -! NEW_FEATURE#0027
			}
		}
		memcpy(m_szNames[nsmp+1], pis->name, 26);
	}

	for (UINT ncu=0; ncu<MAX_BASECHANNELS; ncu++)
	{
		if (ncu>=m_nChannels)
		{
			ChnSettings[ncu].nVolume = 64;
			ChnSettings[ncu].dwFlags &= ~CHN_MUTE;
		}
	}
	m_nMinPeriod = 8;
	m_nMaxPeriod = 0xF000;

// -> CODE#0027
// -> DESC="per-instrument volume ramping setup (refered as attack)"

	// Compute extra instruments settings position
	ITSAMPLESTRUCT *pis = (ITSAMPLESTRUCT *)(lpStream+smppos[pifh->smpnum-1]);
	dwMemPos = pis->samplepointer + lastSampleSize;

	// Get file pointer to match the first byte of extra settings informations	__int16 size = 0;
	__int32 code = 0;
	__int16 size = 0;
	BYTE * ptr = NULL;
	if(dwMemPos < dwMemLength) {
		ptr = (BYTE *)(lpStream + dwMemPos);
		code = (*((__int32 *)ptr));;
	}

	// Instrument extensions
	if( code == 'MPTX' ){
		interpretModplugmade = true;
		ptr += sizeof(__int32);							// jump extension header code
		while( (DWORD)(ptr - lpStream) < mptStartPos ){ //Loop 'till beginning of end of file/mpt specific looking for inst. extensions
		
			code = (*((__int32 *)ptr));			// read field code
			if (code == 'MPTS') {				//Reached song extensions, break out of this loop
				break;
			}
			
			ptr += sizeof(__int32);				// jump field code
			size = (*((__int16 *)ptr));			// read field size
			ptr += sizeof(__int16);				// jump field size

			for(UINT nins=1; nins<=m_nInstruments; nins++){
				if(Headers[nins]){
					// get field's adress in instrument's header
					BYTE * fadr = GetInstrumentHeaderFieldPointer(Headers[nins], code, size);
					// copy field data in instrument's header (except for keyboard mapping)
					if(fadr && code != 'K[..') memcpy(fadr,ptr,size);
					// jump field
					ptr += size;
				}
			}
			//end rewbs.instroVSTi
		}
	}
// -! NEW_FEATURE#0027

	// Song extensions
	if( code == 'MPTS' )
	{
		interpretModplugmade = true;
		LoadExtendedSongProperties(GetType(), ptr, lpStream, mptStartPos);
	}

	Patterns.ResizeArray(max(MAX_PATTERNS, npatterns));

	// Reading Patterns
	for (UINT npat=0; npat<npatterns; npat++)
	{
		if ((!patpos[npat]) || ((DWORD)patpos[npat] >= dwMemLength - 4))
		{
			if(Patterns.Insert(npat, 64)) 
			{
				CString s;
				s.Format("Allocating patterns failed starting from pattern %u", npat);
				MessageBox(NULL, s, "", MB_ICONERROR);
				break;
			}
			continue;
		}
		UINT len = *((WORD *)(lpStream+patpos[npat]));
		UINT rows = *((WORD *)(lpStream+patpos[npat]+2));
// -> CODE#0008
// -> DESC="#define to set pattern size"
//		if ((rows < 4) || (rows > 256)) continue;
		if ((rows < GetModSpecifications().patternRowsMin) || (rows > GetModSpecifications().patternRowsMax)) continue;
// -> BEHAVIOUR_CHANGE#0008
		if (patpos[npat]+8+len > dwMemLength) continue;

		if(Patterns.Insert(npat, rows)) continue;

		memset(lastvalue, 0, sizeof(lastvalue));
		memset(chnmask, 0, sizeof(chnmask));
		MODCOMMAND *m = Patterns[npat];
		UINT i = 0;
		const BYTE *p = lpStream+patpos[npat]+8;
		UINT nrow = 0;
		while (nrow<rows)
		{
			if (i >= len) break;
			BYTE b = p[i++];
			if (!b)
			{
				nrow++;
				m+=m_nChannels;
				continue;
			}
		
			UINT ch = b & IT_bitmask_patternChanField_c; // 0x7f
			
			if (ch)
				ch = (ch - 1); //& IT_bitmask_patternChanMask_c; // 0x3f
		
			if (b & IT_bitmask_patternChanEnabled_c)  // 0x80
			{
				if (i >= len) 
					break;
				chnmask[ch] = p[i++];
			}

			// Now we grab the data for this particular row/channel.
			
			if ((chnmask[ch] & 0x10) && (ch < m_nChannels))
			{
				m[ch].note = lastvalue[ch].note;
			}
			if ((chnmask[ch] & 0x20) && (ch < m_nChannels))
			{
				m[ch].instr = lastvalue[ch].instr;
			}
			if ((chnmask[ch] & 0x40) && (ch < m_nChannels))
			{
				m[ch].volcmd = lastvalue[ch].volcmd;
				m[ch].vol = lastvalue[ch].vol;
			}
			if ((chnmask[ch] & 0x80) && (ch < m_nChannels))
			{
				m[ch].command = lastvalue[ch].command;
				m[ch].param = lastvalue[ch].param;
			}
			if (chnmask[ch] & 1)	// Note
			{
				if (i >= len) break;
				UINT note = p[i++];
				if (ch < m_nChannels)
				{
					if (note < 0x80) note++;
					m[ch].note = note;
					lastvalue[ch].note = note;
					channels_used[ch] = TRUE;
				}
			}
			if (chnmask[ch] & 2)
			{
				if (i >= len) break;
				UINT instr = p[i++];
				if (ch < m_nChannels)
				{
					m[ch].instr = instr;
					lastvalue[ch].instr = instr;
				}
			}
			if (chnmask[ch] & 4)
			{
				if (i >= len) break;
				UINT vol = p[i++];
				if (ch < m_nChannels)
				{
					// 0-64: Set Volume
					if (vol <= 64) { m[ch].volcmd = VOLCMD_VOLUME; m[ch].vol = vol; } else
					// 128-192: Set Panning
					if ((vol >= 128) && (vol <= 192)) { m[ch].volcmd = VOLCMD_PANNING; m[ch].vol = vol - 128; } else
					// 65-74: Fine Volume Up
					if (vol < 75) { m[ch].volcmd = VOLCMD_FINEVOLUP; m[ch].vol = vol - 65; } else
					// 75-84: Fine Volume Down
					if (vol < 85) { m[ch].volcmd = VOLCMD_FINEVOLDOWN; m[ch].vol = vol - 75; } else
					// 85-94: Volume Slide Up
					if (vol < 95) { m[ch].volcmd = VOLCMD_VOLSLIDEUP; m[ch].vol = vol - 85; } else
					// 95-104: Volume Slide Down
					if (vol < 105) { m[ch].volcmd = VOLCMD_VOLSLIDEDOWN; m[ch].vol = vol - 95; } else
					// 105-114: Pitch Slide Up
					if (vol < 115) { m[ch].volcmd = VOLCMD_PORTADOWN; m[ch].vol = vol - 105; } else
					// 115-124: Pitch Slide Down
					if (vol < 125) { m[ch].volcmd = VOLCMD_PORTAUP; m[ch].vol = vol - 115; } else
					// 193-202: Portamento To
					if ((vol >= 193) && (vol <= 202)) { m[ch].volcmd = VOLCMD_TONEPORTAMENTO; m[ch].vol = vol - 193; } else
					// 203-212: Vibrato
					if ((vol >= 203) && (vol <= 212)) { m[ch].volcmd = VOLCMD_VIBRATOSPEED; m[ch].vol = vol - 203; } else
					// 213-222: Velocity //rewbs.velocity
					if ((vol >= 213) && (vol <= 222)) { m[ch].volcmd = VOLCMD_VELOCITY; m[ch].vol = vol - 213; } else	//rewbs.velocity
					// 223-232: Offset //rewbs.VolOffset
					if ((vol >= 223) && (vol <= 232)) { m[ch].volcmd = VOLCMD_OFFSET; m[ch].vol = vol - 223; } //rewbs.volOff
					lastvalue[ch].volcmd = m[ch].volcmd;
					lastvalue[ch].vol = m[ch].vol;
				}
			}
			// Reading command/param
			if (chnmask[ch] & 8)
			{
				if (i > len - 2) break;
				UINT cmd = p[i++];
				UINT param = p[i++];
				if (ch < m_nChannels)
				{
					if (cmd)
					{
						m[ch].command = cmd;
						m[ch].param = param;
						S3MConvert(&m[ch], TRUE);
						lastvalue[ch].command = m[ch].command;
						lastvalue[ch].param = m[ch].param;
					}
				}
			}
		}
	}

	if(m_dwLastSavedWithVersion < MAKE_VERSION_NUMERIC(1, 17, 2, 50))
	{
		SetModFlag(MSF_MIDICC_BUGEMULATION, true);
		SetModFlag(MSF_OLDVOLSWING, true);
	}

	static char autodetectITplaymode = -1;
	if(GetType() == MOD_TYPE_IT)
	{
		if(autodetectITplaymode == -1)
			autodetectITplaymode = CMainFrame::GetPrivateProfileLong("Misc", "AutodetectITplaystyle", 1, theApp.GetConfigFileName());

		if(autodetectITplaymode)
		{
			if(!interpretModplugmade)
			{
				SetModFlag(MSF_MIDICC_BUGEMULATION, false);
				SetModFlag(MSF_OLDVOLSWING, false);
				SetModFlag(MSF_IT_COMPATIBLE_PLAY, true);
			}
		}
		return TRUE;
	}
	else
	{
		//START - mpt specific:
		//Using member cwtv on pifh as the version number.
		const uint16 version = pifh->cwtv;
		if(version > 0x889)
		{
			const char* const cpcMPTStart = reinterpret_cast<const char*>(lpStream + mptStartPos);
			istrstream fin(cpcMPTStart, dwMemLength-mptStartPos);

			if(version >= 0x88D)
			{
				CSerializationInstructions si = CreateSerializationInstructions(*this, INFLAG);
				string msg;
				CSSBSerialization ms(si);
				ms.SetLogstring(msg);
				CReadNotification rn = ms.Unserialize(fin);
				if(msg.length() > 0) MessageBox(0, msg.c_str(), "MPTm load messages", (rn.ID & SNT_FAILURE) ? MB_ICONERROR : MB_ICONINFORMATION);
			}
			else //Loading for older files.
			{
				if(GetTuneSpecificTunings().Unserialize(fin))
					MessageBox(0, "Error occured - loading failed while trying to load tune specific tunings.", "", MB_ICONERROR);
				else
					CInstrumentTuningMapStreamer::ReadInstrumentTunings(*this, fin, (version < 0x88C) ? true : false);
			}
		} //version condition(MPT)
	}

	return TRUE;
}
//end plastiq: code readability improvements

#ifndef MODPLUG_NO_FILESAVE
//#define SAVEITTIMESTAMP
#pragma warning(disable:4100)

// -> CODE#0023
// -> DESC="IT project files (.itp)"
BOOL CSoundFile::SaveITProject(LPCSTR lpszFileName)
//-------------------------------------------------
{
// Check song type

	if(!(m_dwSongFlags & SONG_ITPROJECT)) return FALSE;

	UINT i,j = 0;
	for(i = 0 ; i < m_nInstruments ; i++) { if(m_szInstrumentPath[i][0] != '\0' || !Headers[i+1]) j++; }
	if(m_nInstruments && j != m_nInstruments) return FALSE;

// Open file

	FILE *f;

	if((!lpszFileName) || ((f = fopen(lpszFileName, "wb")) == NULL)) return FALSE;
	

// File ID

	DWORD id = 0x2e697470; // .itp ASCII
	fwrite(&id, 1, sizeof(id), f);

	id = 0x00000101; // v1.01
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

	id = m_dwSongFlags;
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
	Order.WriteAsByte(f, MAX_ORDERS);

// Song Patterns

	// number of patterns
	id = MAX_PATTERNS;
	fwrite(&id, 1, sizeof(id), f);

	// number of pattern name strings
	id = m_nPatternNames;
	fwrite(&id, 1, sizeof(id), f);

	// length of a pattern name string
	id = MAX_PATTERNNAME;
	fwrite(&id, 1, sizeof(id), f);
	fwrite(&m_lpszPatternNames[0], 1, m_nPatternNames * MAX_PATTERNNAME, f);

	// modcommand data length
	id = sizeof(MODCOMMAND_ORIGINAL);
	fwrite(&id, 1, sizeof(id), f);

	// patterns data content
	for(UINT npat=0; npat<MAX_PATTERNS; npat++){
		// pattern size (number of rows)
		id = Patterns[npat] ? PatternSize[npat] : 0;
		fwrite(&id, 1, sizeof(id), f);
		// pattern data
		if(Patterns[npat] && PatternSize[npat]) Patterns[npat].WriteITPdata(f);
			//fwrite(Patterns[npat], 1, m_nChannels * PatternSize[npat] * sizeof(MODCOMMAND_ORIGINAL), f);
	}

// Song lonely (instrument-less) samples

	// Write original number of samples
	id = m_nSamples;
	fwrite(&id, 1, sizeof(id), f);

	BOOL sampleUsed[MAX_SAMPLES];
	memset(&sampleUsed,0,MAX_SAMPLES * sizeof(BOOL));

	// Mark samples used in instruments
	for(i=0; i<m_nInstruments; i++){
		if(Headers[i+1]){
			INSTRUMENTHEADER *p = Headers[i+1];
			for(j=0; j<128; j++) if(p->Keyboard[j]) sampleUsed[p->Keyboard[j]] = TRUE;
		}
	}

	// Count samples not used in any instrument
	i = 0;
	for(j=1; j<=m_nSamples; j++) if(!sampleUsed[j] && Ins[j].pSample) i++;

	id = i;
	fwrite(&id, 1, sizeof(id), f);

	// Write samples not used in any instrument
	ITSAMPLESTRUCT itss;
	for(UINT nsmp=1; nsmp<=m_nSamples; nsmp++){
		if(!sampleUsed[nsmp] && Ins[nsmp].pSample){

			MODINSTRUMENT *psmp = &Ins[nsmp];
			memset(&itss, 0, sizeof(itss));
			memcpy(itss.filename, psmp->name, 12);
			memcpy(itss.name, m_szNames[nsmp], 26);

			itss.id = 0x53504D49;
			itss.gvl = (BYTE)psmp->nGlobalVol;
			itss.flags = 0x00;

			if(psmp->uFlags & CHN_LOOP) itss.flags |= 0x10;
			if(psmp->uFlags & CHN_SUSTAINLOOP) itss.flags |= 0x20;
			if(psmp->uFlags & CHN_PINGPONGLOOP) itss.flags |= 0x40;
			if(psmp->uFlags & CHN_PINGPONGSUSTAIN) itss.flags |= 0x80;
			itss.C5Speed = psmp->nC4Speed;
			if (!itss.C5Speed) itss.C5Speed = 8363;
			itss.length = psmp->nLength;
			itss.loopbegin = psmp->nLoopStart;
			itss.loopend = psmp->nLoopEnd;
			itss.susloopbegin = psmp->nSustainStart;
			itss.susloopend = psmp->nSustainEnd;
			itss.vol = psmp->nVolume >> 2;
			itss.dfp = psmp->nPan >> 2;
			itss.vit = autovibxm2it[psmp->nVibType & 7];
			itss.vis = psmp->nVibRate;
			itss.vid = psmp->nVibDepth;
			itss.vir = (psmp->nVibSweep < 64) ? psmp->nVibSweep * 4 : 255;
			if (psmp->uFlags & CHN_PANNING) itss.dfp |= 0x80;
			if ((psmp->pSample) && (psmp->nLength)) itss.cvt = 0x01;
			UINT flags = RS_PCM8S;

			if(psmp->uFlags & CHN_STEREO){
				flags = RS_STPCM8S;
				itss.flags |= 0x04;
			}
			if(psmp->uFlags & CHN_16BIT){
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

	if(m_dwSongFlags & SONG_ITPEMBEDIH){
		// embeded instrument header tag
		__int32 code = 'EBIH';
		fwrite(&code, 1, sizeof(__int32), f);

		// instruments' header
		for(i=0; i<m_nInstruments; i++){
			if(Headers[i+1]) WriteInstrumentHeaderStruct(Headers[i+1], f);
			// write separator tag
			code = 'SEP@';
			fwrite(&code, 1, sizeof(__int32), f);
		}
	}

	SaveExtendedSongProperties(f);

// Close file
	fclose(f);
	return TRUE;
}
// -! NEW_FEATURE#0023

BOOL CSoundFile::SaveIT(LPCSTR lpszFileName, UINT nPacking)
//-------------------------------------------------------------
{
	DWORD dwPatNamLen, dwChnNamLen;
	ITFILEHEADER header;
	ITINSTRUMENT iti;
	ITSAMPLESTRUCT itss;
	BYTE smpcount[(MAX_SAMPLES+7)/8];
	DWORD inspos[MAX_INSTRUMENTS];
	vector<DWORD> patpos;
	DWORD smppos[MAX_SAMPLES];
	DWORD dwPos = 0, dwHdrPos = 0, dwExtra = 2;
	WORD patinfo[4];
// -> CODE#0006
// -> DESC="misc quantity changes"
//	BYTE chnmask[64];
//	MODCOMMAND lastvalue[64];
	BYTE chnmask[MAX_BASECHANNELS];
	MODCOMMAND lastvalue[MAX_BASECHANNELS];
// -! BEHAVIOUR_CHANGE#0006
	BYTE buf[8 * MAX_BASECHANNELS];
	FILE *f;


	if ((!lpszFileName) || ((f = fopen(lpszFileName, "wb")) == NULL)) return FALSE;


	memset(inspos, 0, sizeof(inspos));
	memset(smppos, 0, sizeof(smppos));
	// Writing Header
	memset(&header, 0, sizeof(header));
	dwPatNamLen = 0;
	dwChnNamLen = 0;
	header.id = 0x4D504D49;
	lstrcpyn(header.songname, m_szNames[0], 26);

	header.reserved1 = 0x1004;
	header.ordnum = 0;
	//while ((header.ordnum < MAX_ORDERS) /*&& (Order[header.ordnum] < 0xFF)*/) header.ordnum++; //rewbs.AllowSaveHiddenPatterns
	//if (header.ordnum < MAX_ORDERS) Order[header.ordnum++] = 0xFF;
	//Ericus' implementation is better.
// -> CODE#0013
// -> DESC="load/save the whole pattern order list"
//	while ((header.ordnum < MAX_ORDERS) && (Order[header.ordnum] < 0xFF)) header.ordnum++;
//	if (header.ordnum < MAX_ORDERS) Order[header.ordnum++] = 0xFF;
	header.ordnum = MAX_ORDERS;
	if(Order.size() < MAX_ORDERS) Order.resize(MAX_ORDERS, Order.GetInvalidPatIndex());
	
	if(GetType() == MOD_TYPE_MPT)
	{
		if(!Order.NeedsExtraDatafield()) header.ordnum = Order.size();
		else header.ordnum = min(Order.size(), MAX_ORDERS); //Writing MAX_ORDERS at max here, and if there's more, writing them elsewhere.

		//Crop unused orders from the end.
		while(header.ordnum > 1 && Order[header.ordnum - 1] == Order.GetInvalidPatIndex()) header.ordnum--;
	}
		
	
	if(GetType() != MOD_TYPE_MPT)
		Order[MAX_ORDERS-1] = 0xFF;
// -! CODE#0013

	header.insnum = m_nInstruments;
	header.smpnum = m_nSamples;
	header.patnum = (GetType() == MOD_TYPE_MPT) ? Patterns.Size() : MAX_PATTERNS;
	if(Patterns.Size() < header.patnum) Patterns.ResizeArray(header.patnum);
	while ((header.patnum > 0) && (!Patterns[header.patnum-1])) header.patnum--;

	patpos.resize(header.patnum, 0);

	//VERSION
	if(GetType() == MOD_TYPE_MPT)
	{
		header.cwtv = 0x88E;	// Used in OMPT-hack versioning.
		header.cmwt = 0x888;
		/*
		Version history:
		0x88D(v.02.49) -> 0x88E: Changed ID to that of IT and undone the orderlist change done in
						  0x88A->0x88B. Now extended orderlist is saved as extension.
		0x88C(v.02.48) -> 0x88D: Some tuning related changes - that part fails to read on older versions.
		0x88B -> 0x88C: Changed type in which tuning number is printed
						to file: size_t -> uint16.
		0x88A -> 0x88B: Changed order-to-pattern-index table type from BYTE-array to vector<UINT>.
		*/
	}
	else //IT
	{
		header.cwtv = 0x888;	//
		header.cmwt = 0x888;	// Might come up as "Impulse Tracker 8" file in XMPlay. :)
	}

	header.flags = 0x0001;
	header.special = 0x0006;
	if (m_nInstruments) header.flags |= 0x04;
	if (m_dwSongFlags & SONG_LINEARSLIDES) header.flags |= 0x08;
	if (m_dwSongFlags & SONG_ITOLDEFFECTS) header.flags |= 0x10;
	if (m_dwSongFlags & SONG_ITCOMPATMODE) header.flags |= 0x20;
	if (m_dwSongFlags & SONG_EXFILTERRANGE) header.flags |= 0x1000;
	header.globalvol = m_nDefaultGlobalVolume >> 1;
	header.mv = m_nSamplePreAmp;
	if (header.mv < 0x20) header.mv = 0x20;
	if (header.mv > 0x7F) header.mv = 0x7F;
	header.speed = m_nDefaultSpeed;
 	header.tempo = min(m_nDefaultTempo,255);  //Limit this one to 255, we save the real one as an extension below.
	header.sep = 128;
	dwHdrPos = sizeof(header) + header.ordnum;
	// Channel Pan and Volume
	memset(header.chnpan, 0xFF, 64);
	memset(header.chnvol, 64, 64);
	for (UINT ich=0; ich</*m_nChannels*/64; ich++) //Header only has room for settings for 64 chans...
	{
		header.chnpan[ich] = ChnSettings[ich].nPan >> 2;
		if (ChnSettings[ich].dwFlags & CHN_SURROUND) header.chnpan[ich] = 100;
		header.chnvol[ich] = ChnSettings[ich].nVolume;
		if (ChnSettings[ich].dwFlags & CHN_MUTE) header.chnpan[ich] |= 0x80;
	}

	for (UINT ich=0; ich<m_nChannels; ich++)
	{
		if (ChnSettings[ich].szName[0])
		{
			dwChnNamLen = (ich+1) * MAX_CHANNELNAME;
		}
	}
	if (dwChnNamLen) dwExtra += dwChnNamLen + 8;
#ifdef SAVEITTIMESTAMP
	dwExtra += 8; // Time Stamp
#endif
	if (m_dwSongFlags & SONG_EMBEDMIDICFG)
	{
		header.flags |= 0x80;
		header.special |= 0x08;
		dwExtra += sizeof(MODMIDICFG);
	}
	// Pattern Names
	if ((m_nPatternNames) && (m_lpszPatternNames))
	{
		dwPatNamLen = m_nPatternNames * MAX_PATTERNNAME;
		while ((dwPatNamLen >= MAX_PATTERNNAME) && (!m_lpszPatternNames[dwPatNamLen-MAX_PATTERNNAME])) dwPatNamLen -= MAX_PATTERNNAME;
		if (dwPatNamLen < MAX_PATTERNNAME) dwPatNamLen = 0;
		if (dwPatNamLen) dwExtra += dwPatNamLen + 8;
	}
	// Mix Plugins
	dwExtra += SaveMixPlugins(NULL, TRUE);
	// Comments
	if (m_lpszSongComments)
	{
		header.special |= 1;
		header.msglength = strlen(m_lpszSongComments)+1;
		header.msgoffset = dwHdrPos + dwExtra + header.insnum*4 + header.patnum*4 + header.smpnum*4;
	}
	// Write file header
	fwrite(&header, 1, sizeof(header), f);
	Order.WriteAsByte(f, header.ordnum);
	if (header.insnum) fwrite(inspos, 4, header.insnum, f);
	if (header.smpnum) fwrite(smppos, 4, header.smpnum, f);
	if (header.patnum) fwrite(&patpos[0], 4, header.patnum, f);
	// Writing editor history information
	{
#ifdef SAVEITTIMESTAMP
		SYSTEMTIME systime;
		FILETIME filetime;
		WORD timestamp[4];
		WORD nInfoEx = 1;
		memset(timestamp, 0, sizeof(timestamp));
		fwrite(&nInfoEx, 1, 2, f);
		GetSystemTime(&systime);
		SystemTimeToFileTime(&systime, &filetime);
		FileTimeToDosDateTime(&filetime, &timestamp[0], &timestamp[1]);
		fwrite(timestamp, 1, 8, f);
#else
		WORD nInfoEx = 0;
		fwrite(&nInfoEx, 1, 2, f);
#endif
	}
	// Writing midi cfg
	if (header.flags & 0x80)
	{
		fwrite(&m_MidiCfg, 1, sizeof(MODMIDICFG), f);
	}
	// Writing pattern names
	if (dwPatNamLen)
	{
		DWORD d = 0x4d414e50;
		fwrite(&d, 1, 4, f);
		fwrite(&dwPatNamLen, 1, 4, f);
		fwrite(m_lpszPatternNames, 1, dwPatNamLen, f);
	}
	// Writing channel Names
	if (dwChnNamLen)
	{
		DWORD d = 0x4d414e43;
		fwrite(&d, 1, 4, f);
		fwrite(&dwChnNamLen, 1, 4, f);
		UINT nChnNames = dwChnNamLen / MAX_CHANNELNAME;
		for (UINT inam=0; inam<nChnNames; inam++)
		{
			fwrite(ChnSettings[inam].szName, 1, MAX_CHANNELNAME, f);
		}
	}
	// Writing mix plugins info
	SaveMixPlugins(f, FALSE);
	// Writing song message
	dwPos = dwHdrPos + dwExtra + (header.insnum + header.smpnum + header.patnum) * 4;
	if (header.special & 1)
	{
		dwPos += strlen(m_lpszSongComments) + 1;
		fwrite(m_lpszSongComments, 1, strlen(m_lpszSongComments)+1, f);
	}
	// Writing instruments
	for (UINT nins=1; nins<=header.insnum; nins++)
	{
		BOOL bKbdEx = FALSE;
		BYTE keyboardex[NOTE_MAX];

		memset(&iti, 0, sizeof(iti));
		iti.id = 0x49504D49;	// "IMPI"
		//iti.trkvers = 0x211;
		iti.trkvers = 0x220;	//rewbs.itVersion
		if (Headers[nins])
		{
			INSTRUMENTHEADER *penv = Headers[nins];
			memset(smpcount, 0, sizeof(smpcount));
			memcpy(iti.filename, penv->filename, 12);
			memcpy(iti.name, penv->name, 26);
			iti.mbank = penv->wMidiBank;
			iti.mpr = penv->nMidiProgram;
			iti.mch = penv->nMidiChannel;
			iti.nna = penv->nNNA;
			//if (penv->nDCT<DCT_PLUGIN) iti.dct = penv->nDCT; else iti.dct =0;	
			iti.dct = penv->nDCT; //rewbs.instroVSTi: will other apps barf if they get an unknown DCT?
			iti.dca = penv->nDNA;
			iti.fadeout = penv->nFadeOut >> 5;
			iti.pps = penv->nPPS;
			iti.ppc = penv->nPPC;
			iti.gbv = (BYTE)(penv->nGlobalVol << 1);
			iti.dfp = (BYTE)penv->nPan >> 2;
			if (!(penv->dwFlags & ENV_SETPANNING)) iti.dfp |= 0x80;
			iti.rv = penv->nVolSwing;
			iti.rp = penv->nPanSwing;
			iti.ifc = penv->nIFC;
			iti.ifr = penv->nIFR;
			iti.nos = 0;
			for (UINT i=0; i<NOTE_MAX; i++) if (penv->Keyboard[i] < MAX_SAMPLES)
			{
				UINT smp = penv->Keyboard[i];
				if ((smp) && (!(smpcount[smp>>3] & (1<<(smp&7)))))
				{
					smpcount[smp>>3] |= 1 << (smp&7);
					iti.nos++;
				}
				iti.keyboard[i*2] = penv->NoteMap[i] - 1;
				iti.keyboard[i*2+1] = smp;
				if (smp > 0xff) bKbdEx = TRUE;
				keyboardex[i] = (smp>>8);
			} else keyboardex[i] = 0;
			// Writing Volume envelope
			if (penv->dwFlags & ENV_VOLUME) iti.volenv.flags |= 0x01;
			if (penv->dwFlags & ENV_VOLLOOP) iti.volenv.flags |= 0x02;
			if (penv->dwFlags & ENV_VOLSUSTAIN) iti.volenv.flags |= 0x04;
			if (penv->dwFlags & ENV_VOLCARRY) iti.volenv.flags |= 0x08;
			iti.volenv.num = (BYTE)penv->nVolEnv;
			iti.volenv.lpb = (BYTE)penv->nVolLoopStart;
			iti.volenv.lpe = (BYTE)penv->nVolLoopEnd;
			iti.volenv.slb = penv->nVolSustainBegin;
			iti.volenv.sle = penv->nVolSustainEnd;
			// Writing Panning envelope
			if (penv->dwFlags & ENV_PANNING) iti.panenv.flags |= 0x01;
			if (penv->dwFlags & ENV_PANLOOP) iti.panenv.flags |= 0x02;
			if (penv->dwFlags & ENV_PANSUSTAIN) iti.panenv.flags |= 0x04;
			if (penv->dwFlags & ENV_PANCARRY) iti.panenv.flags |= 0x08;
			iti.panenv.num = (BYTE)penv->nPanEnv;
			iti.panenv.lpb = (BYTE)penv->nPanLoopStart;
			iti.panenv.lpe = (BYTE)penv->nPanLoopEnd;
			iti.panenv.slb = penv->nPanSustainBegin;
			iti.panenv.sle = penv->nPanSustainEnd;
			// Writing Pitch Envelope
			if (penv->dwFlags & ENV_PITCH) iti.pitchenv.flags |= 0x01;
			if (penv->dwFlags & ENV_PITCHLOOP) iti.pitchenv.flags |= 0x02;
			if (penv->dwFlags & ENV_PITCHSUSTAIN) iti.pitchenv.flags |= 0x04;
			if (penv->dwFlags & ENV_PITCHCARRY) iti.pitchenv.flags |= 0x08;
			if (penv->dwFlags & ENV_FILTER) iti.pitchenv.flags |= 0x80;
			iti.pitchenv.num = (BYTE)penv->nPitchEnv;
			iti.pitchenv.lpb = (BYTE)penv->nPitchLoopStart;
			iti.pitchenv.lpe = (BYTE)penv->nPitchLoopEnd;
			iti.pitchenv.slb = (BYTE)penv->nPitchSustainBegin;
			iti.pitchenv.sle = (BYTE)penv->nPitchSustainEnd;
			// Writing Envelopes data
			for (UINT ev=0; ev<25; ev++)
			{
				iti.volenv.data[ev*3] = penv->VolEnv[ev];
				iti.volenv.data[ev*3+1] = penv->VolPoints[ev] & 0xFF;
				iti.volenv.data[ev*3+2] = penv->VolPoints[ev] >> 8;
				iti.panenv.data[ev*3] = penv->PanEnv[ev] - 32;
				iti.panenv.data[ev*3+1] = penv->PanPoints[ev] & 0xFF;
				iti.panenv.data[ev*3+2] = penv->PanPoints[ev] >> 8;
				iti.pitchenv.data[ev*3] = penv->PitchEnv[ev] - 32;
				iti.pitchenv.data[ev*3+1] = penv->PitchPoints[ev] & 0xFF;
				iti.pitchenv.data[ev*3+2] = penv->PitchPoints[ev] >> 8;
			}
		} else
		// Save Empty Instrument
		{
			for (UINT i=0; i<NOTE_MAX; i++) iti.keyboard[i*2] = i;
			iti.ppc = 5*12;
			iti.gbv = 128;
			iti.dfp = 0x20;
			iti.ifc = 0xFF;
		}
		if (!iti.nos) iti.trkvers = 0;
		// Writing instrument
		if (bKbdEx) *((int *)iti.dummy) = 'MPTX';
		inspos[nins-1] = dwPos;
		dwPos += sizeof(ITINSTRUMENT);
		fwrite(&iti, 1, sizeof(ITINSTRUMENT), f);
		if (bKbdEx)
		{
			dwPos += NOTE_MAX;
			fwrite(keyboardex, 1, NOTE_MAX, f);
		}

		//------------ rewbs.modularInstData
		if (Headers[nins])
		{
			long modularInstSize = 0;
			UINT ModInstID = 'INSM';
			fwrite(&ModInstID, 1, sizeof(ModInstID), f);	// mark this as an instrument with modular extensions
			long sizePos = ftell(f);				// we will want to write the modular data's total size here
			fwrite(&modularInstSize, 1, sizeof(modularInstSize), f);	// write a DUMMY size, just to move file pointer by a long
			
			//Write chunks
			UINT ID;
			{	//VST Slot chunk:
				ID='PLUG';
				fwrite(&ID, 1, sizeof(int), f);
				INSTRUMENTHEADER *penv = Headers[nins];
				fwrite(&(penv->nMixPlug), 1, sizeof(BYTE), f);
				modularInstSize += sizeof(int)+sizeof(BYTE);
			}
			//How to save your own modular instrument chunk:
	/*		{
				ID='MYID';
				fwrite(&ID, 1, sizeof(int), f);
				instModularDataSize+=sizeof(int);
				
				//You can save your chunk size somwhere here if you need variable chunk size.
				fwrite(myData, 1, myDataSize, f);
				instModularDataSize+=myDataSize;
			}
	*/
			//write modular data's total size
			long curPos = ftell(f);			// remember current pos
			fseek(f, sizePos, SEEK_SET);	// go back to  sizePos
			fwrite(&modularInstSize, 1, sizeof(modularInstSize), f);	// write data
			fseek(f, curPos, SEEK_SET);		// go back to where we were.
			
			//move forward 
			dwPos+=sizeof(ModInstID)+sizeof(modularInstSize)+modularInstSize;
		}
		//------------ end rewbs.modularInstData
	}
	// Writing sample headers
	memset(&itss, 0, sizeof(itss));
	for (UINT hsmp=0; hsmp<header.smpnum; hsmp++)
	{
		smppos[hsmp] = dwPos;
		dwPos += sizeof(ITSAMPLESTRUCT);
		fwrite(&itss, 1, sizeof(ITSAMPLESTRUCT), f);
	}
	// Writing Patterns
	for (UINT npat=0; npat<header.patnum; npat++)
	{
		DWORD dwPatPos = dwPos;
		if (!Patterns[npat]) continue;
		patpos[npat] = dwPos;
		patinfo[0] = 0;
		patinfo[1] = PatternSize[npat];
		patinfo[2] = 0;
		patinfo[3] = 0;

		// Check for empty pattern
		if (PatternSize[npat] == 64)
		{
			MODCOMMAND *pzc = Patterns[npat];
			UINT nz = PatternSize[npat] * m_nChannels;
			UINT iz = 0;
			for (iz=0; iz<nz; iz++)
			{
				if ((pzc[iz].note) || (pzc[iz].instr)
				 || (pzc[iz].volcmd) || (pzc[iz].command)) break;
			}
			if (iz == nz)
			{
				patpos[npat] = 0;
				continue;
			}
		}
		fwrite(patinfo, 8, 1, f);
		dwPos += 8;
		memset(chnmask, 0xFF, sizeof(chnmask));
		memset(lastvalue, 0, sizeof(lastvalue));
		MODCOMMAND *m = Patterns[npat];
		for (UINT row=0; row<PatternSize[npat]; row++)
		{
			UINT len = 0;
			for (UINT ch=0; ch<m_nChannels; ch++, m++)
			{
				BYTE b = 0;
				UINT command = m->command;
				UINT param = m->param;
				UINT vol = 0xFF;
				UINT note = m->note;
				if (note) b |= 1;
				if ((note) && (note < 0xFE)) note--;
				if (m->instr) b |= 2;
				if (m->volcmd)
				{
					UINT volcmd = m->volcmd;
					switch(volcmd)
					{
					case VOLCMD_VOLUME:			vol = m->vol; if (vol > 64) vol = 64; break;
					case VOLCMD_PANNING:		vol = m->vol + 128; if (vol > 192) vol = 192; break;
					case VOLCMD_VOLSLIDEUP:		vol = 85 + ConvertVolParam(m->vol); break;
					case VOLCMD_VOLSLIDEDOWN:	vol = 95 + ConvertVolParam(m->vol); break;
					case VOLCMD_FINEVOLUP:		vol = 65 + ConvertVolParam(m->vol); break;
					case VOLCMD_FINEVOLDOWN:	vol = 75 + ConvertVolParam(m->vol); break;
					case VOLCMD_VIBRATO:		vol = 203; break;
					case VOLCMD_VIBRATOSPEED:	vol = 203 + ConvertVolParam(m->vol); break;
					case VOLCMD_TONEPORTAMENTO:	vol = 193 + ConvertVolParam(m->vol); break;
					case VOLCMD_PORTADOWN:		vol = 105 + ConvertVolParam(m->vol); break;
					case VOLCMD_PORTAUP:		vol = 115 + ConvertVolParam(m->vol); break;
					case VOLCMD_VELOCITY:		vol = 213 + ConvertVolParam(m->vol); break; //rewbs.velocity
					case VOLCMD_OFFSET:			vol = 223 + ConvertVolParam(m->vol); break; //rewbs.volOff
					default:					vol = 0xFF;
					}
				}
				if (vol != 0xFF) b |= 4;
				if (command)
				{
					S3MSaveConvert(&command, &param, TRUE);
					if (command) b |= 8;
				}
				// Packing information
				if (b)
				{
					// Same note ?
					if (b & 1)
					{
						if ((note == lastvalue[ch].note) && (lastvalue[ch].volcmd & 1))
						{
							b &= ~1;
							b |= 0x10;
						} else
						{
							lastvalue[ch].note = note;	
							lastvalue[ch].volcmd |= 1;
						}
					}
					// Same instrument ?
					if (b & 2)
					{
						if ((m->instr == lastvalue[ch].instr) && (lastvalue[ch].volcmd & 2))
						{
							b &= ~2;
							b |= 0x20;
						} else
						{
							lastvalue[ch].instr = m->instr;
							lastvalue[ch].volcmd |= 2;
						}
					}
					// Same volume column byte ?
					if (b & 4)
					{
						if ((vol == lastvalue[ch].vol) && (lastvalue[ch].volcmd & 4))
						{
							b &= ~4;
							b |= 0x40;
						} else
						{
							lastvalue[ch].vol = vol;
							lastvalue[ch].volcmd |= 4;
						}
					}
					// Same command / param ?
					if (b & 8)
					{
						if ((command == lastvalue[ch].command) && (param == lastvalue[ch].param) && (lastvalue[ch].volcmd & 8))
						{
							b &= ~8;
							b |= 0x80;
						} else
						{
							lastvalue[ch].command = command;
							lastvalue[ch].param = param;
							lastvalue[ch].volcmd |= 8;
						}
					}
					if (b != chnmask[ch])
					{
						chnmask[ch] = b;
						buf[len++] = (ch+1) | 0x80;
						buf[len++] = b;
					} else
					{
						buf[len++] = ch+1;
					}
					if (b & 1) buf[len++] = note;
					if (b & 2) buf[len++] = m->instr;
					if (b & 4) buf[len++] = vol;
					if (b & 8)
					{
						buf[len++] = command;
						buf[len++] = param;
					}
				}
			}
			buf[len++] = 0;
			if(patinfo[0] > uint16_max - len)
			{
				CString str; str.Format("%s (%s %u)", str_tooMuchPatternData, str_pattern, npat);
				MessageBox(0, str, str_MBtitle, MB_ICONWARNING);
				break;
			}
			else
			{
				dwPos += len;
				patinfo[0] += len;
				fwrite(buf, 1, len, f);
			}
		}
		fseek(f, dwPatPos, SEEK_SET);
		fwrite(patinfo, 8, 1, f);
		fseek(f, dwPos, SEEK_SET);
	}
	// Writing Sample Data
	for (UINT nsmp=1; nsmp<=header.smpnum; nsmp++)
	{
		MODINSTRUMENT *psmp = &Ins[nsmp];
		memset(&itss, 0, sizeof(itss));
		memcpy(itss.filename, psmp->name, 12);
		memcpy(itss.name, m_szNames[nsmp], 26);
		itss.id = 0x53504D49;
		itss.gvl = (BYTE)psmp->nGlobalVol;
		if (m_nInstruments)
		{
			for (UINT iu=1; iu<=m_nInstruments; iu++) if (Headers[iu])
			{
				INSTRUMENTHEADER *penv = Headers[iu];
				for (UINT ju=0; ju<128; ju++) if (penv->Keyboard[ju] == nsmp)
				{
					itss.flags = 0x01;
					break;
				}
			}
		} else
		{
			itss.flags = 0x01;
		}
		if (psmp->uFlags & CHN_LOOP) itss.flags |= 0x10;
		if (psmp->uFlags & CHN_SUSTAINLOOP) itss.flags |= 0x20;
		if (psmp->uFlags & CHN_PINGPONGLOOP) itss.flags |= 0x40;
		if (psmp->uFlags & CHN_PINGPONGSUSTAIN) itss.flags |= 0x80;
		itss.C5Speed = psmp->nC4Speed;
		if (!itss.C5Speed) itss.C5Speed = 8363;
		itss.length = psmp->nLength;
		itss.loopbegin = psmp->nLoopStart;
		itss.loopend = psmp->nLoopEnd;
		itss.susloopbegin = psmp->nSustainStart;
		itss.susloopend = psmp->nSustainEnd;
		itss.vol = psmp->nVolume >> 2;
		itss.dfp = psmp->nPan >> 2;
		itss.vit = autovibxm2it[psmp->nVibType & 7];
		itss.vis = psmp->nVibRate;
		itss.vid = psmp->nVibDepth;
		itss.vir = (psmp->nVibSweep < 64) ? psmp->nVibSweep * 4 : 255;
		if (psmp->uFlags & CHN_PANNING) itss.dfp |= 0x80;
		if ((psmp->pSample) && (psmp->nLength)) itss.cvt = 0x01;
		UINT flags = RS_PCM8S;
#ifndef NO_PACKING
		if (nPacking)
		{
			if ((!(psmp->uFlags & (CHN_16BIT|CHN_STEREO)))
			 && (CanPackSample(psmp->pSample, psmp->nLength, nPacking)))
			{
				flags = RS_ADPCM4;
				itss.cvt = 0xFF;
			}
		} else
#endif // NO_PACKING
		{
			if (psmp->uFlags & CHN_STEREO)
			{
				flags = RS_STPCM8S;
				itss.flags |= 0x04;
			}
			if (psmp->uFlags & CHN_16BIT)
			{
				itss.flags |= 0x02;
				flags = (psmp->uFlags & CHN_STEREO) ? RS_STPCM16S : RS_PCM16S;
			}
		}
		itss.samplepointer = dwPos;
		fseek(f, smppos[nsmp-1], SEEK_SET);
		fwrite(&itss, 1, sizeof(ITSAMPLESTRUCT), f);
		fseek(f, dwPos, SEEK_SET);
		if ((psmp->pSample) && (psmp->nLength))
		{
			dwPos += WriteSample(f, psmp, flags);
		}
	}

	//Save hacked-on extra info
	SaveExtendedInstrumentProperties(Headers, header.insnum, f);
	SaveExtendedSongProperties(f);

	// Updating offsets
	fseek(f, dwHdrPos, SEEK_SET);
	if (header.insnum) fwrite(inspos, 4, header.insnum, f);
	if (header.smpnum) fwrite(smppos, 4, header.smpnum, f);
	if (header.patnum) fwrite(&patpos[0], 4, header.patnum, f);

	if(GetType() == MOD_TYPE_IT)
	{
		fclose(f);
		return TRUE;
	}
	
	//hack
	//BEGIN: MPT SPECIFIC:
	//--------------------

	fseek(f, 0, SEEK_END);
	ofstream fout(f);
	const DWORD MPTStartPos = fout.tellp();

	//Begin: Tuning related things
	CSerializationInstructions si = CreateSerializationInstructions(*this, OUTFLAG);
	string msg;
	CSSBSerialization ms(si);
	ms.SetLogstring(msg);
	CWriteNotification wn = ms.Serialize(fout);
	if(msg.length() > 0) MessageBox(0, msg.c_str(), "MPTm save messages", (wn.ID & SNT_FAILURE) ? MB_ICONERROR : MB_ICONINFORMATION);
	
	//End: Tuning related things.

	//Last 4 bytes should tell where the hack mpt things begin.
	if(!fout.good())
	{
		fout.clear();
		fout.write(reinterpret_cast<const char*>(&MPTStartPos), sizeof(MPTStartPos));
		return FALSE;
	}
	fout.write(reinterpret_cast<const char*>(&MPTStartPos), sizeof(MPTStartPos));
	fout.close();
	//END  : MPT SPECIFIC
	//-------------------

	//NO WRITING HERE ANYMORE.

	return TRUE;
}


#pragma warning(default:4100)
#endif // MODPLUG_NO_FILESAVE

//HACK: This is a quick fix. Needs to be better integrated into player and GUI.
//And need to split into subroutines and eliminate code duplication with SaveIT.
BOOL CSoundFile::SaveCompatIT(LPCSTR lpszFileName) 
//------------------------------------------------
{
	const int IT_MAX_CHANNELS=64;
	DWORD dwPatNamLen, dwChnNamLen;
	ITFILEHEADER header;
	ITINSTRUMENT iti;
	ITSAMPLESTRUCT itss;
	BYTE smpcount[(MAX_SAMPLES+7)/8];
	DWORD inspos[MAX_INSTRUMENTS];
	DWORD patpos[MAX_PATTERNS];
	DWORD smppos[MAX_SAMPLES];
	DWORD dwPos = 0, dwHdrPos = 0, dwExtra = 2;
	WORD patinfo[4];
// -> CODE#0006
// -> DESC="misc quantity changes"
	BYTE chnmask[IT_MAX_CHANNELS];
	MODCOMMAND lastvalue[IT_MAX_CHANNELS];
	UINT nChannels = min(m_nChannels, IT_MAX_CHANNELS);
// -! BEHAVIOUR_CHANGE#0006
	BYTE buf[512];
	FILE *f;


	if ((!lpszFileName) || ((f = fopen(lpszFileName, "wb")) == NULL)) return FALSE;
	memset(inspos, 0, sizeof(inspos));
	memset(patpos, 0, sizeof(patpos));
	memset(smppos, 0, sizeof(smppos));
	// Writing Header
	memset(&header, 0, sizeof(header));
	dwPatNamLen = 0;
	dwChnNamLen = 0;
	header.id = 0x4D504D49;
	lstrcpyn(header.songname, m_szNames[0], 26);

	header.reserved1 = 0x1004;
	header.ordnum = 0;
	header.ordnum=MAX_ORDERS;
	while (header.ordnum>0 && Order[header.ordnum-1]==0xFF) {
		header.ordnum--;
	}
	header.patnum = MAX_PATTERNS;
	while ((header.patnum > 0) && (!Patterns[header.patnum-1])) {
		header.patnum--;
	}

	header.insnum = m_nInstruments;
	header.smpnum = m_nSamples;
	header.cwtv = 0x217;	//Value like in Modplug 1.16
	header.cmwt = 0x200;	//Value like in Modplug 1.16
	header.flags = 0x0001;
	header.special = 0x0006;
	if (m_nInstruments) header.flags |= 0x04;
	if (m_dwSongFlags & SONG_LINEARSLIDES) header.flags |= 0x08;
	if (m_dwSongFlags & SONG_ITOLDEFFECTS) header.flags |= 0x10;
	if (m_dwSongFlags & SONG_ITCOMPATMODE) header.flags |= 0x20;
	if (m_dwSongFlags & SONG_EXFILTERRANGE) header.flags |= 0x1000;
	header.globalvol = m_nDefaultGlobalVolume >> 1;
	header.mv = m_nSamplePreAmp;
	if (header.mv < 0x20) header.mv = 0x20;
	if (header.mv > 0x7F) header.mv = 0x7F;
	header.speed = m_nDefaultSpeed;
 	header.tempo = min(m_nDefaultTempo,255);  //Limit this one to 255, we save the real one as an extension below.
	header.sep = 128;
	dwHdrPos = sizeof(header) + header.ordnum;
	// Channel Pan and Volume
	memset(header.chnpan, 0xFF, 64);
	memset(header.chnvol, 64, 64);
	for (UINT ich=0; ich</*m_nChannels*/64; ich++) //Header only has room for settings for 64 chans...
	{
		header.chnpan[ich] = ChnSettings[ich].nPan >> 2;
		if (ChnSettings[ich].dwFlags & CHN_SURROUND) header.chnpan[ich] = 100;
		header.chnvol[ich] = ChnSettings[ich].nVolume;
		if (ChnSettings[ich].dwFlags & CHN_MUTE) header.chnpan[ich] |= 0x80;
/*		if (ChnSettings[ich].szName[0])
		{
			dwChnNamLen = (ich+1) * MAX_CHANNELNAME;
		}
*/
	}
//	if (dwChnNamLen) dwExtra += dwChnNamLen + 8;
/*#ifdef SAVEITTIMESTAMP
	dwExtra += 8; // Time Stamp
#endif
*/
	if (m_dwSongFlags & SONG_EMBEDMIDICFG)
	{
		header.flags |= 0x80;
		header.special |= 0x08;
		dwExtra += sizeof(MODMIDICFG);
	}
	// Pattern Names
/*	if ((m_nPatternNames) && (m_lpszPatternNames))
	{
		dwPatNamLen = m_nPatternNames * MAX_PATTERNNAME;
		while ((dwPatNamLen >= MAX_PATTERNNAME) && (!m_lpszPatternNames[dwPatNamLen-MAX_PATTERNNAME])) dwPatNamLen -= MAX_PATTERNNAME;
		if (dwPatNamLen < MAX_PATTERNNAME) dwPatNamLen = 0;
		if (dwPatNamLen) dwExtra += dwPatNamLen + 8;
	}
*/	// Mix Plugins
	//dwExtra += SaveMixPlugins(NULL, TRUE);
	// Comments
	if (m_lpszSongComments)
	{
		header.special |= 1;
		header.msglength = strlen(m_lpszSongComments)+1;
		header.msgoffset = dwHdrPos + dwExtra + header.insnum*4 + header.patnum*4 + header.smpnum*4;
	}
	// Write file header
	fwrite(&header, 1, sizeof(header), f);
	Order.WriteAsByte(f, header.ordnum);
	if (header.insnum) fwrite(inspos, 4, header.insnum, f);
	if (header.smpnum) fwrite(smppos, 4, header.smpnum, f);
	if (header.patnum) fwrite(patpos, 4, header.patnum, f);
	// Writing editor history information
	{
/*#ifdef SAVEITTIMESTAMP
		SYSTEMTIME systime;
		FILETIME filetime;
		WORD timestamp[4];
		WORD nInfoEx = 1;
		memset(timestamp, 0, sizeof(timestamp));
		fwrite(&nInfoEx, 1, 2, f);
		GetSystemTime(&systime);
		SystemTimeToFileTime(&systime, &filetime);
		FileTimeToDosDateTime(&filetime, &timestamp[0], &timestamp[1]);
		fwrite(timestamp, 1, 8, f);
#else
*/		WORD nInfoEx = 0;
		fwrite(&nInfoEx, 1, 2, f);
//#endif
	}
	// Writing midi cfg
	if (header.flags & 0x80)
	{
		fwrite(&m_MidiCfg, 1, sizeof(MODMIDICFG), f);
	}
	// Writing pattern names
/*	if (dwPatNamLen)
	{
		DWORD d = 0x4d414e50;
		fwrite(&d, 1, 4, f);
		fwrite(&dwPatNamLen, 1, 4, f);
		fwrite(m_lpszPatternNames, 1, dwPatNamLen, f);
	}
*/	// Writing channel Names
/*	if (dwChnNamLen)
	{
		DWORD d = 0x4d414e43;
		fwrite(&d, 1, 4, f);
		fwrite(&dwChnNamLen, 1, 4, f);
		UINT nChnNames = dwChnNamLen / MAX_CHANNELNAME;
		for (UINT inam=0; inam<nChnNames; inam++)
		{
			fwrite(ChnSettings[inam].szName, 1, MAX_CHANNELNAME, f);
		}
	}
*/	// Writing mix plugins info
/*	SaveMixPlugins(f, FALSE);
*/	// Writing song message
	dwPos = dwHdrPos + dwExtra + (header.insnum + header.smpnum + header.patnum) * 4;
	if (header.special & 1)
	{
		dwPos += strlen(m_lpszSongComments) + 1;
		fwrite(m_lpszSongComments, 1, strlen(m_lpszSongComments)+1, f);
	}
	// Writing instruments
	for (UINT nins=1; nins<=header.insnum; nins++)
	{
		BOOL bKbdEx = FALSE;
		BYTE keyboardex[NOTE_MAX];

		memset(&iti, 0, sizeof(iti));
		iti.id = 0x49504D49;	// "IMPI"
		//iti.trkvers = 0x211;
		iti.trkvers = 0x220;	//rewbs.itVersion
		if (Headers[nins])
		{
			INSTRUMENTHEADER *penv = Headers[nins];
			memset(smpcount, 0, sizeof(smpcount));
			memcpy(iti.filename, penv->filename, 12);
			memcpy(iti.name, penv->name, 26);
			iti.mbank = penv->wMidiBank;
			iti.mpr = penv->nMidiProgram;
			iti.mch = penv->nMidiChannel;
			iti.nna = penv->nNNA;
			if (penv->nDCT<DCT_PLUGIN) iti.dct = penv->nDCT; else iti.dct =0;	
			iti.dca = penv->nDNA;
			iti.fadeout = penv->nFadeOut >> 5;
			iti.pps = penv->nPPS;
			iti.ppc = penv->nPPC;
			iti.gbv = (BYTE)(penv->nGlobalVol << 1);
			iti.dfp = (BYTE)penv->nPan >> 2;
			if (!(penv->dwFlags & ENV_SETPANNING)) iti.dfp |= 0x80;
			iti.rv = penv->nVolSwing;
			iti.rp = penv->nPanSwing;
			iti.ifc = penv->nIFC;
			iti.ifr = penv->nIFR;
			iti.nos = 0;
			for (UINT i=0; i<NOTE_MAX; i++) if (penv->Keyboard[i] < MAX_SAMPLES)
			{
				UINT smp = penv->Keyboard[i];
				if ((smp) && (!(smpcount[smp>>3] & (1<<(smp&7)))))
				{
					smpcount[smp>>3] |= 1 << (smp&7);
					iti.nos++;
				}
				iti.keyboard[i*2] = penv->NoteMap[i] - 1;
				iti.keyboard[i*2+1] = smp;
				if (smp > 0xff) bKbdEx = TRUE;
				keyboardex[i] = (smp>>8);
			} else keyboardex[i] = 0;
			// Writing Volume envelope
			if (penv->dwFlags & ENV_VOLUME) iti.volenv.flags |= 0x01;
			if (penv->dwFlags & ENV_VOLLOOP) iti.volenv.flags |= 0x02;
			if (penv->dwFlags & ENV_VOLSUSTAIN) iti.volenv.flags |= 0x04;
			if (penv->dwFlags & ENV_VOLCARRY) iti.volenv.flags |= 0x08;
			iti.volenv.num = (BYTE)penv->nVolEnv;
			iti.volenv.lpb = (BYTE)penv->nVolLoopStart;
			iti.volenv.lpe = (BYTE)penv->nVolLoopEnd;
			iti.volenv.slb = penv->nVolSustainBegin;
			iti.volenv.sle = penv->nVolSustainEnd;
			// Writing Panning envelope
			if (penv->dwFlags & ENV_PANNING) iti.panenv.flags |= 0x01;
			if (penv->dwFlags & ENV_PANLOOP) iti.panenv.flags |= 0x02;
			if (penv->dwFlags & ENV_PANSUSTAIN) iti.panenv.flags |= 0x04;
			if (penv->dwFlags & ENV_PANCARRY) iti.panenv.flags |= 0x08;
			iti.panenv.num = (BYTE)penv->nPanEnv;
			iti.panenv.lpb = (BYTE)penv->nPanLoopStart;
			iti.panenv.lpe = (BYTE)penv->nPanLoopEnd;
			iti.panenv.slb = penv->nPanSustainBegin;
			iti.panenv.sle = penv->nPanSustainEnd;
			// Writing Pitch Envelope
			if (penv->dwFlags & ENV_PITCH) iti.pitchenv.flags |= 0x01;
			if (penv->dwFlags & ENV_PITCHLOOP) iti.pitchenv.flags |= 0x02;
			if (penv->dwFlags & ENV_PITCHSUSTAIN) iti.pitchenv.flags |= 0x04;
			if (penv->dwFlags & ENV_PITCHCARRY) iti.pitchenv.flags |= 0x08;
			if (penv->dwFlags & ENV_FILTER) iti.pitchenv.flags |= 0x80;
			iti.pitchenv.num = (BYTE)penv->nPitchEnv;
			iti.pitchenv.lpb = (BYTE)penv->nPitchLoopStart;
			iti.pitchenv.lpe = (BYTE)penv->nPitchLoopEnd;
			iti.pitchenv.slb = (BYTE)penv->nPitchSustainBegin;
			iti.pitchenv.sle = (BYTE)penv->nPitchSustainEnd;
			// Writing Envelopes data
			for (UINT ev=0; ev<25; ev++)
			{
				iti.volenv.data[ev*3] = penv->VolEnv[ev];
				iti.volenv.data[ev*3+1] = penv->VolPoints[ev] & 0xFF;
				iti.volenv.data[ev*3+2] = penv->VolPoints[ev] >> 8;
				iti.panenv.data[ev*3] = penv->PanEnv[ev] - 32;
				iti.panenv.data[ev*3+1] = penv->PanPoints[ev] & 0xFF;
				iti.panenv.data[ev*3+2] = penv->PanPoints[ev] >> 8;
				iti.pitchenv.data[ev*3] = penv->PitchEnv[ev] - 32;
				iti.pitchenv.data[ev*3+1] = penv->PitchPoints[ev] & 0xFF;
				iti.pitchenv.data[ev*3+2] = penv->PitchPoints[ev] >> 8;
			}
		} else
		// Save Empty Instrument
		{
			for (UINT i=0; i<NOTE_MAX; i++) iti.keyboard[i*2] = i;
			iti.ppc = 5*12;
			iti.gbv = 128;
			iti.dfp = 0x20;
			iti.ifc = 0xFF;
		}
		if (!iti.nos) iti.trkvers = 0;
		// Writing instrument
		if (bKbdEx) *((int *)iti.dummy) = 'MPTX';
		inspos[nins-1] = dwPos;
		dwPos += sizeof(ITINSTRUMENT);
		fwrite(&iti, 1, sizeof(ITINSTRUMENT), f);
		if (bKbdEx)
		{
			dwPos += NOTE_MAX;
			fwrite(keyboardex, 1, NOTE_MAX, f);
		}

		//------------ rewbs.modularInstData
/*		if (Headers[nins])
		{
			long modularInstSize = 0;
			UINT ModInstID = 'INSM';
			fwrite(&ModInstID, 1, sizeof(ModInstID), f);	// mark this as an instrument with modular extensions
			long sizePos = ftell(f);				// we will want to write the modular data's total size here
			fwrite(&modularInstSize, 1, sizeof(modularInstSize), f);	// write a DUMMY size, just to move file pointer by a long
			
			//Write chunks
			UINT ID;
			{	//VST Slot chunk:
				ID='PLUG';
				fwrite(&ID, 1, sizeof(int), f);
				INSTRUMENTHEADER *penv = Headers[nins];
				fwrite(&(penv->nMixPlug), 1, sizeof(BYTE), f);
				modularInstSize += sizeof(int)+sizeof(BYTE);
			}
*/			//How to save your own modular instrument chunk:
	/*		{
				ID='MYID';
				fwrite(&ID, 1, sizeof(int), f);
				instModularDataSize+=sizeof(int);
				
				//You can save your chunk size somwhere here if you need variable chunk size.
				fwrite(myData, 1, myDataSize, f);
				instModularDataSize+=myDataSize;
			}
	*/
/*			//write modular data's total size
			long curPos = ftell(f);			// remember current pos
			fseek(f, sizePos, SEEK_SET);	// go back to  sizePos
			fwrite(&modularInstSize, 1, sizeof(modularInstSize), f);	// write data
			fseek(f, curPos, SEEK_SET);		// go back to where we were.
			
			//move forward 
			dwPos+=sizeof(ModInstID)+sizeof(modularInstSize)+modularInstSize;
		}
*/		//------------ end rewbs.modularInstData
	}
	// Writing sample headers
	memset(&itss, 0, sizeof(itss));
	for (UINT hsmp=0; hsmp<header.smpnum; hsmp++)
	{
		smppos[hsmp] = dwPos;
		dwPos += sizeof(ITSAMPLESTRUCT);
		fwrite(&itss, 1, sizeof(ITSAMPLESTRUCT), f);
	}
	// Writing Patterns
	for (UINT npat=0; npat<header.patnum; npat++)
	{
		DWORD dwPatPos = dwPos;
		UINT len;
		if (!Patterns[npat]) continue;
		patpos[npat] = dwPos;
		patinfo[0] = 0;
		patinfo[1] = PatternSize[npat];
		patinfo[2] = 0;
		patinfo[3] = 0;
		// Check for empty pattern
		if (PatternSize[npat] == 64)
		{
			MODCOMMAND *pzc = Patterns[npat];
			UINT nz = PatternSize[npat] * nChannels;
			UINT iz = 0;
			for (iz=0; iz<nz; iz++)
			{
				if ((pzc[iz].note) || (pzc[iz].instr)
				 || (pzc[iz].volcmd) || (pzc[iz].command)) break;
			}
			if (iz == nz)
			{
				patpos[npat] = 0;
				continue;
			}
		}
		fwrite(patinfo, 8, 1, f);
		dwPos += 8;
		memset(chnmask, 0xFF, sizeof(chnmask));
		memset(lastvalue, 0, sizeof(lastvalue));
		MODCOMMAND *m = Patterns[npat];
		for (UINT row=0; row<PatternSize[npat]; row++)
		{
			len = 0;
			for (UINT ch=0; ch<nChannels; ch++, m++)
			{
				BYTE b = 0;
				UINT command = m->command;
				UINT param = m->param;
				UINT vol = 0xFF;
				UINT note = m->note;
				if (note) b |= 1;
				if ((note) && (note < 0xFE)) note--;
				if (m->instr) b |= 2;
				if (m->volcmd)
				{
					UINT volcmd = m->volcmd;
					switch(volcmd)
					{
					case VOLCMD_VOLUME:			vol = m->vol; if (vol > 64) vol = 64; break;
					case VOLCMD_PANNING:		vol = m->vol + 128; if (vol > 192) vol = 192; break;
					case VOLCMD_VOLSLIDEUP:		vol = 85 + ConvertVolParam(m->vol); break;
					case VOLCMD_VOLSLIDEDOWN:	vol = 95 + ConvertVolParam(m->vol); break;
					case VOLCMD_FINEVOLUP:		vol = 65 + ConvertVolParam(m->vol); break;
					case VOLCMD_FINEVOLDOWN:	vol = 75 + ConvertVolParam(m->vol); break;
					case VOLCMD_VIBRATO:		vol = 203; break;
					case VOLCMD_VIBRATOSPEED:	vol = 203 + ConvertVolParam(m->vol); break;
					case VOLCMD_TONEPORTAMENTO:	vol = 193 + ConvertVolParam(m->vol); break;
					case VOLCMD_PORTADOWN:		vol = 105 + ConvertVolParam(m->vol); break;
					case VOLCMD_PORTAUP:		vol = 115 + ConvertVolParam(m->vol); break;
					case VOLCMD_VELOCITY:		vol = 213 + ConvertVolParam(m->vol); break; //rewbs.velocity
					case VOLCMD_OFFSET:			vol = 223 + ConvertVolParam(m->vol); break; //rewbs.volOff
					default:					vol = 0xFF;
					}
				}
				if (vol != 0xFF) b |= 4;
				if (command)
				{
					S3MSaveConvert(&command, &param, TRUE);
					if (command) b |= 8;
				}
				// Packing information
				if (b)
				{
					// Same note ?
					if (b & 1)
					{
						if ((note == lastvalue[ch].note) && (lastvalue[ch].volcmd & 1))
						{
							b &= ~1;
							b |= 0x10;
						} else
						{
							lastvalue[ch].note = note;	
							lastvalue[ch].volcmd |= 1;
						}
					}
					// Same instrument ?
					if (b & 2)
					{
						if ((m->instr == lastvalue[ch].instr) && (lastvalue[ch].volcmd & 2))
						{
							b &= ~2;
							b |= 0x20;
						} else
						{
							lastvalue[ch].instr = m->instr;
							lastvalue[ch].volcmd |= 2;
						}
					}
					// Same volume column byte ?
					if (b & 4)
					{
						if ((vol == lastvalue[ch].vol) && (lastvalue[ch].volcmd & 4))
						{
							b &= ~4;
							b |= 0x40;
						} else
						{
							lastvalue[ch].vol = vol;
							lastvalue[ch].volcmd |= 4;
						}
					}
					// Same command / param ?
					if (b & 8)
					{
						if ((command == lastvalue[ch].command) && (param == lastvalue[ch].param) && (lastvalue[ch].volcmd & 8))
						{
							b &= ~8;
							b |= 0x80;
						} else
						{
							lastvalue[ch].command = command;
							lastvalue[ch].param = param;
							lastvalue[ch].volcmd |= 8;
						}
					}
					if (b != chnmask[ch])
					{
						chnmask[ch] = b;
						buf[len++] = (ch+1) | 0x80;
						buf[len++] = b;
					} else
					{
						buf[len++] = ch+1;
					}
					if (b & 1) buf[len++] = note;
					if (b & 2) buf[len++] = m->instr;
					if (b & 4) buf[len++] = vol;
					if (b & 8)
					{
						buf[len++] = command;
						buf[len++] = param;
					}
				}
			}
			buf[len++] = 0;
			dwPos += len;
			patinfo[0] += len;
			fwrite(buf, 1, len, f);
		}
		fseek(f, dwPatPos, SEEK_SET);
		fwrite(patinfo, 8, 1, f);
		fseek(f, dwPos, SEEK_SET);
	}
	// Writing Sample Data
	for (UINT nsmp=1; nsmp<=header.smpnum; nsmp++)
	{
		MODINSTRUMENT *psmp = &Ins[nsmp];
		memset(&itss, 0, sizeof(itss));
		memcpy(itss.filename, psmp->name, 12);
		memcpy(itss.name, m_szNames[nsmp], 26);
		itss.id = 0x53504D49;
		itss.gvl = (BYTE)psmp->nGlobalVol;
		if (m_nInstruments)
		{
			for (UINT iu=1; iu<=m_nInstruments; iu++) if (Headers[iu])
			{
				INSTRUMENTHEADER *penv = Headers[iu];
				for (UINT ju=0; ju<128; ju++) if (penv->Keyboard[ju] == nsmp)
				{
					itss.flags = 0x01;
					break;
				}
			}
		} else
		{
			itss.flags = 0x01;
		}
		if (psmp->uFlags & CHN_LOOP) itss.flags |= 0x10;
		if (psmp->uFlags & CHN_SUSTAINLOOP) itss.flags |= 0x20;
		if (psmp->uFlags & CHN_PINGPONGLOOP) itss.flags |= 0x40;
		if (psmp->uFlags & CHN_PINGPONGSUSTAIN) itss.flags |= 0x80;
		itss.C5Speed = psmp->nC4Speed;
		if (!itss.C5Speed) itss.C5Speed = 8363;
		itss.length = psmp->nLength;
		itss.loopbegin = psmp->nLoopStart;
		itss.loopend = psmp->nLoopEnd;
		itss.susloopbegin = psmp->nSustainStart;
		itss.susloopend = psmp->nSustainEnd;
		itss.vol = psmp->nVolume >> 2;
		itss.dfp = psmp->nPan >> 2;
		itss.vit = autovibxm2it[psmp->nVibType & 7];
		itss.vis = psmp->nVibRate;
		itss.vid = psmp->nVibDepth;
		itss.vir = (psmp->nVibSweep < 64) ? psmp->nVibSweep * 4 : 255;
		if (psmp->uFlags & CHN_PANNING) itss.dfp |= 0x80;
		if ((psmp->pSample) && (psmp->nLength)) itss.cvt = 0x01;
		UINT flags = RS_PCM8S;
#ifndef NO_PACKING
		if (nPacking)
		{
			if ((!(psmp->uFlags & (CHN_16BIT|CHN_STEREO)))
			 && (CanPackSample(psmp->pSample, psmp->nLength, nPacking)))
			{
				flags = RS_ADPCM4;
				itss.cvt = 0xFF;
			}
		} else
#endif // NO_PACKING
		{
			if (psmp->uFlags & CHN_STEREO)
			{
				flags = RS_STPCM8S;
				itss.flags |= 0x04;
			}
			if (psmp->uFlags & CHN_16BIT)
			{
				itss.flags |= 0x02;
				flags = (psmp->uFlags & CHN_STEREO) ? RS_STPCM16S : RS_PCM16S;
			}
		}
		itss.samplepointer = dwPos;
		fseek(f, smppos[nsmp-1], SEEK_SET);
		fwrite(&itss, 1, sizeof(ITSAMPLESTRUCT), f);
		fseek(f, dwPos, SEEK_SET);
		if ((psmp->pSample) && (psmp->nLength))
		{
			dwPos += WriteSample(f, psmp, flags);
		}
	}

	//Save hacked-on extra info
//	SaveExtendedInstrumentProperties(Headers, header.insnum, f);
//	SaveExtendedSongProperties(f);

	// Updating offsets
	fseek(f, dwHdrPos, SEEK_SET);
	if (header.insnum) fwrite(inspos, 4, header.insnum, f);
	if (header.smpnum) fwrite(smppos, 4, header.smpnum, f);
	if (header.patnum) fwrite(patpos, 4, header.patnum, f);
	fclose(f);
	return TRUE;

}

//////////////////////////////////////////////////////////////////////////////
// IT 2.14 compression

DWORD ITReadBits(DWORD &bitbuf, UINT &bitnum, LPBYTE &ibuf, CHAR n)
//-----------------------------------------------------------------
{
	DWORD retval = 0;
	UINT i = n;

	if (n > 0)
	{
		do
		{
			if (!bitnum)
			{
				bitbuf = *ibuf++;
				bitnum = 8;
			}
			retval >>= 1;
			retval |= bitbuf << 31;
			bitbuf >>= 1;
			bitnum--;
			i--;
		} while (i);
		i = n;
	}
	return (retval >> (32-i));
}

#define IT215_SUPPORT
void ITUnpack8Bit(LPSTR pSample, DWORD dwLen, LPBYTE lpMemFile, DWORD dwMemLength, BOOL b215)
//-------------------------------------------------------------------------------------------
{
	LPSTR pDst = pSample;
	LPBYTE pSrc = lpMemFile;
	DWORD wHdr = 0;
	DWORD wCount = 0;
	DWORD bitbuf = 0;
	UINT bitnum = 0;
	BYTE bLeft = 0, bTemp = 0, bTemp2 = 0;

	while (dwLen)
	{
		if (!wCount)
		{
			wCount = 0x8000;
			wHdr = *((LPWORD)pSrc);
			pSrc += 2;
			bLeft = 9;
			bTemp = bTemp2 = 0;
			bitbuf = bitnum = 0;
		}
		DWORD d = wCount;
		if (d > dwLen) d = dwLen;
		// Unpacking
		DWORD dwPos = 0;
		do
		{
			WORD wBits = (WORD)ITReadBits(bitbuf, bitnum, pSrc, bLeft);
			if (bLeft < 7)
			{
				DWORD i = 1 << (bLeft-1);
				DWORD j = wBits & 0xFFFF;
				if (i != j) goto UnpackByte;
				wBits = (WORD)(ITReadBits(bitbuf, bitnum, pSrc, 3) + 1) & 0xFF;
				bLeft = ((BYTE)wBits < bLeft) ? (BYTE)wBits : (BYTE)((wBits+1) & 0xFF);
				goto Next;
			}
			if (bLeft < 9)
			{
				WORD i = (0xFF >> (9 - bLeft)) + 4;
				WORD j = i - 8;
				if ((wBits <= j) || (wBits > i)) goto UnpackByte;
				wBits -= j;
				bLeft = ((BYTE)(wBits & 0xFF) < bLeft) ? (BYTE)(wBits & 0xFF) : (BYTE)((wBits+1) & 0xFF);
				goto Next;
			}
			if (bLeft >= 10) goto SkipByte;
			if (wBits >= 256)
			{
				bLeft = (BYTE)(wBits + 1) & 0xFF;
				goto Next;
			}
		UnpackByte:
			if (bLeft < 8)
			{
				BYTE shift = 8 - bLeft;
				char c = (char)(wBits << shift);
				c >>= shift;
				wBits = (WORD)c;
			}
			wBits += bTemp;
			bTemp = (BYTE)wBits;
			bTemp2 += bTemp;
#ifdef IT215_SUPPORT
			pDst[dwPos] = (b215) ? bTemp2 : bTemp;
#else
			pDst[dwPos] = bTemp;
#endif
		SkipByte:
			dwPos++;
		Next:
			if (pSrc >= lpMemFile+dwMemLength+1) return;
		} while (dwPos < d);
		// Move On
		wCount -= d;
		dwLen -= d;
		pDst += d;
	}
}


void ITUnpack16Bit(LPSTR pSample, DWORD dwLen, LPBYTE lpMemFile, DWORD dwMemLength, BOOL b215)
//--------------------------------------------------------------------------------------------
{
	signed short *pDst = (signed short *)pSample;
	LPBYTE pSrc = lpMemFile;
	DWORD wHdr = 0;
	DWORD wCount = 0;
	DWORD bitbuf = 0;
	UINT bitnum = 0;
	BYTE bLeft = 0;
	signed short wTemp = 0, wTemp2 = 0;

	while (dwLen)
	{
		if (!wCount)
		{
			wCount = 0x4000;
			wHdr = *((LPWORD)pSrc);
			pSrc += 2;
			bLeft = 17;
			wTemp = wTemp2 = 0;
			bitbuf = bitnum = 0;
		}
		DWORD d = wCount;
		if (d > dwLen) d = dwLen;
		// Unpacking
		DWORD dwPos = 0;
		do
		{
			DWORD dwBits = ITReadBits(bitbuf, bitnum, pSrc, bLeft);
			if (bLeft < 7)
			{
				DWORD i = 1 << (bLeft-1);
				DWORD j = dwBits;
				if (i != j) goto UnpackByte;
				dwBits = ITReadBits(bitbuf, bitnum, pSrc, 4) + 1;
				bLeft = ((BYTE)(dwBits & 0xFF) < bLeft) ? (BYTE)(dwBits & 0xFF) : (BYTE)((dwBits+1) & 0xFF);
				goto Next;
			}
			if (bLeft < 17)
			{
				DWORD i = (0xFFFF >> (17 - bLeft)) + 8;
				DWORD j = (i - 16) & 0xFFFF;
				if ((dwBits <= j) || (dwBits > (i & 0xFFFF))) goto UnpackByte;
				dwBits -= j;
				bLeft = ((BYTE)(dwBits & 0xFF) < bLeft) ? (BYTE)(dwBits & 0xFF) : (BYTE)((dwBits+1) & 0xFF);
				goto Next;
			}
			if (bLeft >= 18) goto SkipByte;
			if (dwBits >= 0x10000)
			{
				bLeft = (BYTE)(dwBits + 1) & 0xFF;
				goto Next;
			}
		UnpackByte:
			if (bLeft < 16)
			{
				BYTE shift = 16 - bLeft;
				signed short c = (signed short)(dwBits << shift);
				c >>= shift;
				dwBits = (DWORD)c;
			}
			dwBits += wTemp;
			wTemp = (signed short)dwBits;
			wTemp2 += wTemp;
#ifdef IT215_SUPPORT
			pDst[dwPos] = (b215) ? wTemp2 : wTemp;
#else
			pDst[dwPos] = wTemp;
#endif
		SkipByte:
			dwPos++;
		Next:
			if (pSrc >= lpMemFile+dwMemLength+1) return;
		} while (dwPos < d);
		// Move On
		wCount -= d;
		dwLen -= d;
		pDst += d;
		if (pSrc >= lpMemFile+dwMemLength) break;
	}
}


#ifndef MODPLUG_NO_FILESAVE
UINT CSoundFile::SaveMixPlugins(FILE *f, BOOL bUpdate)
//----------------------------------------------------
{


// -> CODE#0006
// -> DESC="misc quantity changes"
//	DWORD chinfo[64];
	DWORD chinfo[MAX_BASECHANNELS];
// -! BEHAVIOUR_CHANGE#0006
	CHAR s[32];
	DWORD nPluginSize;
	UINT nTotalSize = 0;
	UINT nChInfo = 0;

	for (UINT i=0; i<MAX_MIXPLUGINS; i++)
	{
		PSNDMIXPLUGIN p = &m_MixPlugins[i];
		if ((p->Info.dwPluginId1) || (p->Info.dwPluginId2))
		{
			nPluginSize = sizeof(SNDMIXPLUGININFO)+4; // plugininfo+4 (datalen)
			if ((p->pMixPlugin) && (bUpdate))
			{
				p->pMixPlugin->SaveAllParameters();
			}
			if (p->pPluginData)
			{
				nPluginSize += p->nPluginDataSize;
			}
			
			// rewbs.modularPlugData
			DWORD MPTxPlugDataSize = 4 + (sizeof(m_MixPlugins[i].fDryRatio)) +     //4 for ID and size of dryRatio
									 4 + (sizeof(m_MixPlugins[i].defaultProgram)); //rewbs.plugDefaultProgram
			 					// for each extra entity, add 4 for ID, plus size of entity, plus optionally 4 for size of entity.

			nPluginSize += MPTxPlugDataSize+4; //+4 is for size itself: sizeof(DWORD) is 4	
			// rewbs.modularPlugData
			if (f)
			{
				// write plugin ID
				s[0] = 'F';
				s[1] = 'X';
				s[2] = '0' + (i/10);
				s[3] = '0' + (i%10);
				fwrite(s, 1, 4, f);
				
				// write plugin size:
				fwrite(&nPluginSize, 1, 4, f);
				fwrite(&p->Info, 1, sizeof(SNDMIXPLUGININFO), f);
				fwrite(&m_MixPlugins[i].nPluginDataSize, 1, 4, f);
				if (m_MixPlugins[i].pPluginData) {
					fwrite(m_MixPlugins[i].pPluginData, 1, m_MixPlugins[i].nPluginDataSize, f);
				}
				
				//rewbs.dryRatio
				fwrite(&MPTxPlugDataSize, 1, 4, f);

				//write ID for this xPlugData chunk:
				s[0] = 'D'; s[1] = 'W';	s[2] = 'R'; s[3] = 'T';
				fwrite(s, 1, 4, f);
				//Write chunk data itself (Could include size if you want variable size. Not necessary here.)
				fwrite(&(m_MixPlugins[i].fDryRatio), 1, sizeof(float), f);
				//end rewbs.dryRatio

				//rewbs.plugDefaultProgram
				//if (nProgram>=0) {
					s[0] = 'P'; s[1] = 'R';	s[2] = 'O'; s[3] = 'G';
					fwrite(s, 1, 4, f);
					//Write chunk data itself (Could include size if you want variable size. Not necessary here.)
					fwrite(&(m_MixPlugins[i].defaultProgram), 1, sizeof(float), f);
				//}
				//end rewbs.plugDefaultProgram

			}
			nTotalSize += nPluginSize + 8;
		}
	}
	for (UINT j=0; j<m_nChannels; j++)
	{
// -> CODE#0006
// -> DESC="misc quantity changes"
//		if (j < 64)
		if (j < MAX_BASECHANNELS)
// -! BEHAVIOUR_CHANGE#0006
		{
			if ((chinfo[j] = ChnSettings[j].nMixPlugin) != 0)
			{
				nChInfo = j+1;
			}
		}
	}
	if (nChInfo)
	{
		if (f)
		{
			nPluginSize = 'XFHC';
			fwrite(&nPluginSize, 1, 4, f);
			nPluginSize = nChInfo*4;
			fwrite(&nPluginSize, 1, 4, f);
			fwrite(chinfo, 1, nPluginSize, f);
		}
		nTotalSize += nChInfo*4 + 8;
	}
	if (m_SongEQ.nEQBands > 0)
	{
		nTotalSize += 8 + m_SongEQ.nEQBands * 4;
		if (f)
		{
			nPluginSize = 'XFQE';
			fwrite(&nPluginSize, 1, 4, f);
			nPluginSize = m_SongEQ.nEQBands*4;
			fwrite(&nPluginSize, 1, 4, f);
			fwrite(m_SongEQ.EQFreq_Gains, 1, nPluginSize, f);
		}
	}
	return nTotalSize;
}
#endif


UINT CSoundFile::LoadMixPlugins(const void *pData, UINT nLen)
//-----------------------------------------------------------
{
	const BYTE *p = (const BYTE *)pData;
	UINT nPos = 0;

	while (nPos+8 < nLen)	// so plugin data chunks must be multiples of 8 bytes?
	{
		DWORD nPluginSize;
		UINT nPlugin;

		nPluginSize = *(DWORD *)(p+nPos+4);		// why +4?
		if (nPluginSize > nLen-nPos-8) break;;
		if ((*(DWORD *)(p+nPos)) == 'XFHC')
		{
// -> CODE#0006
// -> DESC="misc quantity changes"
//			for (UINT ch=0; ch<64; ch++) if (ch*4 < nPluginSize)
			for (UINT ch=0; ch<MAX_BASECHANNELS; ch++) if (ch*4 < nPluginSize)
// -! BEHAVIOUR_CHANGE#0006
			{
				ChnSettings[ch].nMixPlugin = *(DWORD *)(p+nPos+8+ch*4);
			}
		}

		else if ((*(DWORD *)(p+nPos)) == 'XFQE')
		{
			m_SongEQ.nEQBands = nPluginSize/4;
			if (m_SongEQ.nEQBands > MAX_EQ_BANDS) m_SongEQ.nEQBands = MAX_EQ_BANDS;
			memcpy(m_SongEQ.EQFreq_Gains, p+nPos+8, m_SongEQ.nEQBands * 4);
		} 

		//Load plugin Data
		else
		{
			if ((p[nPos] != 'F') || (p[nPos+1] != 'X') || (p[nPos+2] < '0') || (p[nPos+3] < '0'))
			{
				break;
			}
			nPlugin = (p[nPos+2]-'0')*10 + (p[nPos+3]-'0');			//calculate plug-in number.

			if ((nPlugin < MAX_MIXPLUGINS) && (nPluginSize >= sizeof(SNDMIXPLUGININFO)+4))
			{
				//MPT's standard plugin data. Size not specified in file.. grrr..
				m_MixPlugins[nPlugin].Info = *(const SNDMIXPLUGININFO *)(p+nPos+8);

				//data for VST setchunk? size lies just after standard plugin data.
				DWORD dwExtra = *(DWORD *)(p+nPos+8+sizeof(SNDMIXPLUGININFO));
				
				if ((dwExtra) && (dwExtra <= nPluginSize-sizeof(SNDMIXPLUGININFO)-4))
				{
					m_MixPlugins[nPlugin].nPluginDataSize = 0;
					m_MixPlugins[nPlugin].pPluginData = new char [dwExtra];
					if (m_MixPlugins[nPlugin].pPluginData)
					{
						m_MixPlugins[nPlugin].nPluginDataSize = dwExtra;
						memcpy(m_MixPlugins[nPlugin].pPluginData, p+nPos+8+sizeof(SNDMIXPLUGININFO)+4, dwExtra);
					}
				}

				//rewbs.modularPlugData
				DWORD dwXPlugData = *(DWORD *)(p+nPos+8+sizeof(SNDMIXPLUGININFO)+dwExtra+4); //read next DWORD into dwMPTExtra
				
				//if dwMPTExtra is positive and there are dwMPTExtra bytes left in nPluginSize, we have some more data!
				if ((dwXPlugData) && ((int)dwXPlugData <= (int)nPluginSize-(int)(sizeof(SNDMIXPLUGININFO)+dwExtra+8)))
				{
					DWORD startPos = nPos+8+sizeof(SNDMIXPLUGININFO)+dwExtra+8; // start of extra data for this plug
					DWORD endPos = startPos + dwXPlugData;						// end of extra data for this plug
					DWORD currPos = startPos;

					while (currPos < endPos) //cycle through all the bytes
					{
						// do we recognize this chunk?
						//rewbs.dryRatio
						//TODO: turn this into a switch statement like for modular instrument data
						if ((p[currPos] == 'D') && (p[currPos+1] == 'W') && (p[currPos+2] == 'R') && (p[currPos+3] == 'T'))
						{	
							currPos+=4;// move past ID
							m_MixPlugins[nPlugin].fDryRatio = *(float*) (p+currPos);
							currPos+= sizeof(float); //move past data
						}
						//end rewbs.dryRatio
						
						//rewbs.plugDefaultProgram
						else if ((p[currPos] == 'P') && (p[currPos+1] == 'R') && (p[currPos+2] == 'O') && (p[currPos+3] == 'G'))
						{	
							currPos+=4;// move past ID
							m_MixPlugins[nPlugin].defaultProgram = *(long*) (p+currPos);
							currPos+= sizeof(long); //move past data
						}
						//end rewbs.plugDefaultProgram
                        //else if.. (add extra attempts to recognize chunks here)
						else // otherwise move forward a byte.
						{
							currPos++;
						}

					}

				}
				//end rewbs.modularPlugData

			}
		}
		nPos += nPluginSize + 8;
	}
	return nPos;
}


void CSoundFile::SaveExtendedInstrumentProperties(INSTRUMENTHEADER *instruments[], UINT nInstruments, FILE* f)
//------------------------------------------------------------------------------------------------------------
// Used only when saving IT and XM. 
// ITI, ITP saves using Ericus' macros etc...
// The reason is that ITs and XMs save [code][size][ins1.Value][ins2.Value]...
// whereas ITP saves [code][size][ins1.Value][code][size][ins2.Value]...
// too late to turn back....
{
	__int32 code=0;

/*	if(Headers[1] == NULL) {
		return;
	}*/

	code = 'MPTX';							// write extension header code
	fwrite(&code, 1, sizeof(__int32), f);		
	
	WriteInstrumentPropertyForAllInstruments('VR..', sizeof(m_defaultInstrument.nVolRamp),    f, instruments, nInstruments);
	WriteInstrumentPropertyForAllInstruments('MiP.', sizeof(m_defaultInstrument.nMixPlug),    f, instruments, nInstruments);
	WriteInstrumentPropertyForAllInstruments('MC..', sizeof(m_defaultInstrument.nMidiChannel),f, instruments, nInstruments);
	WriteInstrumentPropertyForAllInstruments('MP..', sizeof(m_defaultInstrument.nMidiProgram),f, instruments, nInstruments);
	WriteInstrumentPropertyForAllInstruments('MB..', sizeof(m_defaultInstrument.wMidiBank),   f, instruments, nInstruments);
	WriteInstrumentPropertyForAllInstruments('P...', sizeof(m_defaultInstrument.nPan),        f, instruments, nInstruments);
	WriteInstrumentPropertyForAllInstruments('GV..', sizeof(m_defaultInstrument.nGlobalVol),  f, instruments, nInstruments);
	WriteInstrumentPropertyForAllInstruments('FO..', sizeof(m_defaultInstrument.nFadeOut),    f, instruments, nInstruments);
	WriteInstrumentPropertyForAllInstruments('R...', sizeof(m_defaultInstrument.nResampling), f, instruments, nInstruments);
   	WriteInstrumentPropertyForAllInstruments('CS..', sizeof(m_defaultInstrument.nCutSwing),   f, instruments, nInstruments);
	WriteInstrumentPropertyForAllInstruments('RS..', sizeof(m_defaultInstrument.nResSwing),   f, instruments, nInstruments);
	WriteInstrumentPropertyForAllInstruments('FM..', sizeof(m_defaultInstrument.nFilterMode), f, instruments, nInstruments);
	WriteInstrumentPropertyForAllInstruments('PERN', sizeof(m_defaultInstrument.nPitchEnvReleaseNode ), f, instruments, nInstruments);
	WriteInstrumentPropertyForAllInstruments('AERN', sizeof(m_defaultInstrument.nPanEnvReleaseNode), f, instruments, nInstruments);
	WriteInstrumentPropertyForAllInstruments('VERN', sizeof(m_defaultInstrument.nVolEnvReleaseNode), f, instruments, nInstruments);
	WriteInstrumentPropertyForAllInstruments('PTTL', sizeof(m_defaultInstrument.wPitchToTempoLock),  f, instruments, nInstruments);
	WriteInstrumentPropertyForAllInstruments('PVEH', sizeof(m_defaultInstrument.nPluginVelocityHandling),  f, instruments, nInstruments);
	WriteInstrumentPropertyForAllInstruments('PVOH', sizeof(m_defaultInstrument.nPluginVolumeHandling),  f, instruments, nInstruments);
		
	return;
}

void CSoundFile::WriteInstrumentPropertyForAllInstruments(__int32 code,  __int16 size, FILE* f, INSTRUMENTHEADER *instruments[], UINT nInstruments) 
//-------------------------------------------------------------------------------------------------------------------------------------------------
{
	fwrite(&code, 1, sizeof(__int32), f);		//write code
	fwrite(&size, 1, sizeof(__int16), f);		//write size
	for(UINT nins=1; nins<=nInstruments; nins++) {  //for all instruments...
		BYTE* pField;
		if (instruments[nins])	{
			pField = GetInstrumentHeaderFieldPointer(instruments[nins], code, size); //get ptr to field
		} else { 
			pField = GetInstrumentHeaderFieldPointer(&m_defaultInstrument, code, size); //get ptr to field
		}
		fwrite(pField, 1, size, f);				//write field data
	}

	return;
}

void CSoundFile::SaveExtendedSongProperties(FILE* f)
//--------------------------------------------------
{  //Extra song data - Yet Another Hack. 
	__int16 size;
	__int32 code = 'MPTS';					//Extra song file data
	fwrite(&code, 1, sizeof(__int32), f);
	
	code = 'DT..';							//write m_nDefaultTempo field code
	fwrite(&code, 1, sizeof(__int32), f);	
	size = sizeof(m_nDefaultTempo);			//write m_nDefaultTempo field size
	fwrite(&size, 1, sizeof(__int16), f);
	fwrite(&m_nDefaultTempo, 1, size, f);	//write m_nDefaultTempo

	code = 'RPB.';							//write m_nRowsPerBeat
	fwrite(&code, 1, sizeof(__int32), f);	
	size = sizeof(m_nRowsPerBeat);			
	fwrite(&size, 1, sizeof(__int16), f);
	fwrite(&m_nRowsPerBeat, 1, size, f);	

	code = 'RPM.';							//write m_nRowsPerMeasure
	fwrite(&code, 1, sizeof(__int32), f);	
	size = sizeof(m_nRowsPerMeasure);		
	fwrite(&size, 1, sizeof(__int16), f);
	fwrite(&m_nRowsPerMeasure, 1, size, f);	

	code = 'C...';							//write m_nChannels 
	fwrite(&code, 1, sizeof(__int32), f);	
	size = sizeof(m_nChannels);				
	fwrite(&size, 1, sizeof(__int16), f);
	fwrite(&m_nChannels, 1, size, f);		

	code = 'TM..';							//write m_nTempoMode
	fwrite(&code, 1, sizeof(__int32), f);	
	size = sizeof(m_nTempoMode);		
	fwrite(&size, 1, sizeof(__int16), f);
	fwrite(&m_nTempoMode, 1, size, f);	

	code = 'PMM.';							//write m_nMixLevels
	fwrite(&code, 1, sizeof(__int32), f);	
	size = sizeof(m_nMixLevels);		
	fwrite(&size, 1, sizeof(__int16), f);
	fwrite(&m_nMixLevels, 1, size, f);	

	code = 'CWV.';							//write m_dwCreatedWithVersion
	fwrite(&code, 1, sizeof(__int32), f);	
	size = sizeof(m_dwCreatedWithVersion);		
	fwrite(&size, 1, sizeof(__int16), f);
	fwrite(&m_dwCreatedWithVersion, 1, size, f);	

	code = 'LSWV';							//write m_dwCreatedWithVersion
	fwrite(&code, 1, sizeof(__int32), f);	
	size = sizeof(m_dwLastSavedWithVersion);		
	fwrite(&size, 1, sizeof(__int16), f);
	fwrite(&m_dwLastSavedWithVersion, 1, size, f);	

	code = 'SPA.';							//write m_nSamplePreAmp
	fwrite(&code, 1, sizeof(__int32), f);	
	size = sizeof(m_nSamplePreAmp);		
	fwrite(&size, 1, sizeof(__int16), f);
	fwrite(&m_nSamplePreAmp, 1, size, f);	

	code = 'VSTV';							//write m_nVSTiVolume
	fwrite(&code, 1, sizeof(__int32), f);	
	size = sizeof(m_nVSTiVolume);		
	fwrite(&size, 1, sizeof(__int16), f);
	fwrite(&m_nVSTiVolume, 1, size, f);	

	code = 'DGV.';							//write m_nDefaultGlobalVolume
	fwrite(&code, 1, sizeof(__int32), f);	
	size = sizeof(m_nDefaultGlobalVolume);		
	fwrite(&size, 1, sizeof(__int16), f);
	fwrite(&m_nDefaultGlobalVolume, 1, size, f);	

	code = 'RP..';							//write m_nRestartPos
	fwrite(&code, 1, sizeof(__int32), f);	
	size = sizeof(m_nRestartPos);		
	fwrite(&size, 1, sizeof(__int16), f);
	fwrite(&m_nRestartPos, 1, size, f);


	//Additional flags for XM/IT/MPTM
	if(m_ModFlags)							
	{
        code = 'MSF.';
		fwrite(&code, 1, sizeof(__int32), f);	
		size = sizeof(m_ModFlags);		
		fwrite(&size, 1, sizeof(__int16), f);
		fwrite(&m_ModFlags, 1, size, f);	
	}

	//MIMA, MIDI mapping directives
	if(GetMIDIMapper().GetCount() > 0)
	{
		const size_t objectsize = GetMIDIMapper().GetSerializationSize();
		if(objectsize > size_t(int16_max))
			MessageBox(NULL, "Datafield overflow with MIDI to plugparam mappings; data won't be written.", NULL, MB_ICONERROR);
		else
		{
			code = 'MIMA';
			fwrite(&code, 1, sizeof(__int32), f);
			size = static_cast<int16>(objectsize);
			fwrite(&size, 1, sizeof(__int16), f);
			GetMIDIMapper().Serialize(f);
		}
	}
	

	return;
}


void CSoundFile::LoadExtendedSongProperties(const MODTYPE modtype, BYTE*& ptr, const BYTE* lpStream, const size_t searchlimit)
//--------------------------------------------------
{
	int32 code = 0;
	int16 size = 0;
	ptr += sizeof(int32); // jump extension header code
	while( (DWORD)(ptr + 6 - lpStream) <= searchlimit ) //Loop until given limit.
	{ 
		code = (*((__int32 *)ptr));			// read field code
		ptr += sizeof(__int32);				// jump field code
		size = (*((__int16 *)ptr));			// read field size
		if(size <  0) break;
		ptr += sizeof(__int16);				// jump field size

		BYTE * fadr = NULL;
		switch (code) {						// interpret field code
			case 'DT..': fadr = reinterpret_cast<BYTE*>(&m_nDefaultTempo);   break;
			case 'RPB.': fadr = reinterpret_cast<BYTE*>(&m_nRowsPerBeat);    break;
			case 'RPM.': fadr = reinterpret_cast<BYTE*>(&m_nRowsPerMeasure); break;			
			case 'C...': if(modtype != MOD_TYPE_XM) fadr = reinterpret_cast<BYTE*>(&m_nChannels); break;		
			case 'TM..': fadr = reinterpret_cast<BYTE*>(&m_nTempoMode);		 break;
			case 'PMM.': fadr = reinterpret_cast<BYTE*>(&m_nMixLevels);	 break;
			case 'CWV.': fadr = reinterpret_cast<BYTE*>(&m_dwCreatedWithVersion);	 break;
			case 'LSWV': fadr = reinterpret_cast<BYTE*>(&m_dwLastSavedWithVersion);	 break;
			case 'SPA.': fadr = reinterpret_cast<BYTE*>(&m_nSamplePreAmp);	 break;
			case 'VSTV': fadr = reinterpret_cast<BYTE*>(&m_nVSTiVolume);	 break;
			case 'DGV.': fadr = reinterpret_cast<BYTE*>(&m_nDefaultGlobalVolume);	 break;
			case 'RP..': if(modtype != MOD_TYPE_XM) fadr = reinterpret_cast<BYTE*>(&m_nRestartPos);	 break;
			case 'MSF.': fadr = reinterpret_cast<BYTE*>(&m_ModFlags);	 break;
			case 'MIMA':
				if(DWORD(ptr - lpStream + DWORD(size)) > searchlimit)
					MessageBox(NULL, "Error: Bad MIMA datasizefield", NULL, MB_ICONERROR);
				else
				{
					GetMIDIMapper().Unserialize(ptr, size);
					ptr += size;
				}

			break;
		}

		if (fadr != NULL && ptr - lpStream + DWORD(size) <= searchlimit) {	// if field code recognized
			memcpy(fadr,ptr,size);			// read field data
		}
		ptr += size;						// jump field data
	}
}


