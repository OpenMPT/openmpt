/*
 * OpenMPT
 *
 * Load_it.cpp
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>
 *          OpenMPT devs
*/

#include "stdafx.h"
#include "Loaders.h"
#include "IT_DEFS.H"
#include "tuningcollection.h"
#include "../mptrack/moddoc.h"
#include "../mptrack/serialization_utils.h"
#include <fstream>
#include <strstream>
#include <list>
#include "../mptrack/version.h"

#define str_tooMuchPatternData	(GetStrI18N((_TEXT("Warning: File format limit was reached. Some pattern data may not get written to file."))))
#define str_pattern				(GetStrI18N((_TEXT("pattern"))))
#define str_PatternSetTruncationNote (GetStrI18N((_TEXT("The module contains %u patterns but only %u patterns can be loaded in this OpenMPT version.\n"))))
#define str_SequenceTruncationNote (GetStrI18N((_TEXT("Module has sequence of length %u; it will be truncated to maximum supported length, %u.\n"))))
#define str_LoadingIncompatibleVersion	TEXT("The file informed that it is incompatible with this version of OpenMPT. Loading was terminated.\n")
#define str_LoadingMoreRecentVersion	TEXT("The loaded file was made with a more recent OpenMPT version and this version may not be able to load all the features or play the file correctly.\n")

const uint16 verMptFileVer = 0x891;
const uint16 verMptFileVerLoadLimit = 0x1000; // If cwtv-field is greater or equal to this value,
											  // the MPTM file will not be loaded.

/*
MPTM version history for cwtv-field in "IT" header (only for MPTM files!):
0x890(1.18.02.00) -> 0x891(1.19.00.00): Pattern-specific time signatures
										Fixed behaviour of Pattern Loop command for rows > 255 (r617)
0x88F(1.18.01.00) -> 0x890(1.18.02.00): Removed volume command velocity :xy, added delay-cut command :xy.
0x88E(1.17.02.50) -> 0x88F(1.18.01.00): Numerous changes
0x88D(1.17.02.49) -> 0x88E(1.17.02.50): Changed ID to that of IT and undone the orderlist change done in
				       0x88A->0x88B. Now extended orderlist is saved as extension.
0x88C(1.17.02.48) -> 0x88D(1.17.02.49): Some tuning related changes - that part fails to read on older versions.
0x88B -> 0x88C: Changed type in which tuning number is printed to file: size_t -> uint16.
0x88A -> 0x88B: Changed order-to-pattern-index table type from BYTE-array to vector<UINT>.
*/


static bool AreNonDefaultTuningsUsed(CSoundFile& sf)
//--------------------------------------------------
{
	const INSTRUMENTINDEX iCount = sf.GetNumInstruments();
	for(INSTRUMENTINDEX i = 1; i <= iCount; i++)
	{
		if(sf.Instruments[i] != nullptr && sf.Instruments[i]->pTuning != 0)
			return true;
	}
	return false;
}

void ReadTuningCollection(istream& iStrm, CTuningCollection& tc, const size_t) {tc.Deserialize(iStrm);}
void WriteTuningCollection(ostream& oStrm, const CTuningCollection& tc) {tc.Serialize(oStrm);}

void WriteTuningMap(ostream& oStrm, const CSoundFile& sf)
//-------------------------------------------------------
{
	if(sf.GetNumInstruments() > 0)
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
		for(UINT i = 1; i <= sf.GetNumInstruments(); i++) if (sf.Instruments[i] != nullptr)
		{
			TNTS_MAP_ITER iter = tNameToShort_Map.find(sf.Instruments[i]->pTuning);
			if(iter != tNameToShort_Map.end())
				continue; //Tuning already mapped.

			tNameToShort_Map[sf.Instruments[i]->pTuning] = figMap;
			figMap++;
		}

		//...and write the map with tuning names replacing
		//the addresses.
		const uint16 tuningMapSize = static_cast<uint16>(tNameToShort_Map.size());
		oStrm.write(reinterpret_cast<const char*>(&tuningMapSize), sizeof(tuningMapSize));
		for(TNTS_MAP_ITER iter = tNameToShort_Map.begin(); iter != tNameToShort_Map.end(); iter++)
		{
			if(iter->first)
				StringToBinaryStream<uint8>(oStrm, iter->first->GetName());
			else //Case: Using original IT tuning.
				StringToBinaryStream<uint8>(oStrm, "->MPT_ORIGINAL_IT<-");

			srlztn::Binarywrite<uint16>(oStrm, iter->second);
		}

		//Writing tuning data for instruments.
		for(UINT i = 1; i <= sf.GetNumInstruments(); i++)
		{
			TNTS_MAP_ITER iter = tNameToShort_Map.find(sf.Instruments[i]->pTuning);
			if(iter == tNameToShort_Map.end()) //Should never happen
			{
#ifdef MODPLUG_TRACKER
				if(sf.GetpModDoc())
					sf.GetpModDoc()->AddToLog(_T("Error: 210807_1\n"));
#endif // MODPLUG_TRACKER
				return;
			}
			srlztn::Binarywrite(oStrm, iter->second);
		}
	}
}

template<class TUNNUMTYPE, class STRSIZETYPE>
static bool ReadTuningMap(istream& iStrm, map<uint16, string>& shortToTNameMap, const size_t maxNum = 500)
//--------------------------------------------------------------------------------------------------------
{
	typedef map<uint16, string> MAP;
	typedef MAP::iterator MAP_ITER;
	TUNNUMTYPE numTuning = 0;
	iStrm.read(reinterpret_cast<char*>(&numTuning), sizeof(numTuning));
	if(numTuning > maxNum)
		return true;

	for(size_t i = 0; i<numTuning; i++)
	{
		string temp;
		uint16 ui;
		if(StringFromBinaryStream<STRSIZETYPE>(iStrm, temp, 255))
			return true;

		iStrm.read(reinterpret_cast<char*>(&ui), sizeof(ui));
		shortToTNameMap[ui] = temp;
	}
	if(iStrm.good())
		return false;
	else
		return true;
}

void ReadTuningMap(istream& iStrm, CSoundFile& csf, const size_t = 0)
//-------------------------------------------------------------------
{
	typedef map<WORD, string> MAP;
	typedef MAP::iterator MAP_ITER;
	MAP shortToTNameMap;
	ReadTuningMap<uint16, uint8>(iStrm, shortToTNameMap);

	//Read & set tunings for instruments
	std::list<string> notFoundTunings;
	for(UINT i = 1; i<=csf.GetNumInstruments(); i++)
	{
		uint16 ui;
		iStrm.read(reinterpret_cast<char*>(&ui), sizeof(ui));
		MAP_ITER iter = shortToTNameMap.find(ui);
		if(csf.Instruments[i] && iter != shortToTNameMap.end())
		{
			const string str = iter->second;

			if(str == string("->MPT_ORIGINAL_IT<-"))
			{
				csf.Instruments[i]->pTuning = nullptr;
				continue;
			}

			csf.Instruments[i]->pTuning = csf.GetTuneSpecificTunings().GetTuning(str);
			if(csf.Instruments[i]->pTuning)
				continue;

			csf.Instruments[i]->pTuning = csf.GetLocalTunings().GetTuning(str);
			if(csf.Instruments[i]->pTuning)
				continue;

			csf.Instruments[i]->pTuning = csf.GetBuiltInTunings().GetTuning(str);
			if(csf.Instruments[i]->pTuning)
				continue;

			if(str == "TET12" && csf.GetBuiltInTunings().GetNumTunings() > 0)
				csf.Instruments[i]->pTuning = &csf.GetBuiltInTunings().GetTuning(0);

			if(csf.Instruments[i]->pTuning)
				continue;

			//Checking if not found tuning already noticed.
			std::list<std::string>::iterator iter;
			iter = find(notFoundTunings.begin(), notFoundTunings.end(), str);
			if(iter == notFoundTunings.end())
			{
				notFoundTunings.push_back(str);
#ifdef MODPLUG_TRACKER
				if(csf.GetpModDoc() != nullptr)
				{
					string erm = string("Tuning ") + str + string(" used by the module was not found.\n");
					csf.GetpModDoc()->AddToLog(erm.c_str());
					csf.GetpModDoc()->SetModified(); //The tuning is changed so the modified flag is set.
				}
#endif // MODPLUG_TRACKER

			}
			csf.Instruments[i]->pTuning = csf.Instruments[i]->s_DefaultTuning;

		}
		else //This 'else' happens probably only in case of corrupted file.
		{
			if(csf.Instruments[i])
				csf.Instruments[i]->pTuning = MODINSTRUMENT::s_DefaultTuning;
		}

	}
	//End read&set instrument tunings
}



#pragma warning(disable:4244) //conversion from 'type1' to 'type2', possible loss of data

// IT Vibrato -> VibratoType
BYTE autovibit2xm[8] =
{ VIB_SINE, VIB_RAMP_DOWN, VIB_SQUARE, VIB_RANDOM, VIB_RAMP_UP, 0, 0, 0 };

// VibratoType -> Vibrato
BYTE autovibxm2it[8] =
{ 0, 2, 4, 1, 3, 0, 0, 0 };

//////////////////////////////////////////////////////////
// Impulse Tracker IT file support


UINT ConvertVolParam(const MODCOMMAND *m)
//---------------------------------------
{
	return min(m->vol, 9);
}


// Convert MPT's internal envelope format into an IT/MPTM envelope.
void MPTEnvToIT(const INSTRUMENTENVELOPE *mptEnv, ITENVELOPE *itEnv, const BYTE envOffset, const BYTE envDefault)
//---------------------------------------------------------------------------------------------------------------
{
	if(mptEnv->dwFlags & ENV_ENABLED)	itEnv->flags |= 1;
	if(mptEnv->dwFlags & ENV_LOOP)		itEnv->flags |= 2;
	if(mptEnv->dwFlags & ENV_SUSTAIN)	itEnv->flags |= 4;
	if(mptEnv->dwFlags & ENV_CARRY)		itEnv->flags |= 8;
	itEnv->num = (BYTE)min(mptEnv->nNodes, 25);
	itEnv->lpb = (BYTE)mptEnv->nLoopStart;
	itEnv->lpe = (BYTE)mptEnv->nLoopEnd;
	itEnv->slb = (BYTE)mptEnv->nSustainStart;
	itEnv->sle = (BYTE)mptEnv->nSustainEnd;

	if(mptEnv->nNodes > 0)
	{
		// Attention: Full MPTM envelope is stored in extended instrument properties
		for(size_t ev = 0; ev < 25; ev++)
		{
			itEnv->data[ev * 3] = mptEnv->Values[ev] - envOffset;
			itEnv->data[ev * 3 + 1] = mptEnv->Ticks[ev] & 0xFF;
			itEnv->data[ev * 3 + 2] = mptEnv->Ticks[ev] >> 8;
		}
	} else
	{
		// Fix non-existing envelopes so that they can still be edited in Impulse Tracker.
		itEnv->num = 2;
		MemsetZero(itEnv->data);
		itEnv->data[0] = itEnv->data[3] = envDefault - envOffset;
		itEnv->data[4] = 10;
	}
}


// Convert IT/MPTM envelope data into MPT's internal envelope format - To be used by ITInstrToMPT()
void ITEnvToMPT(const ITENVELOPE *itEnv, INSTRUMENTENVELOPE *mptEnv, const BYTE envOffset, const int iEnvMax)
//-----------------------------------------------------------------------------------------------------------
{
	if(itEnv->flags & 1) mptEnv->dwFlags |= ENV_ENABLED;
	if(itEnv->flags & 2) mptEnv->dwFlags |= ENV_LOOP;
	if(itEnv->flags & 4) mptEnv->dwFlags |= ENV_SUSTAIN;
	if(itEnv->flags & 8) mptEnv->dwFlags |= ENV_CARRY;
	mptEnv->nNodes = min(itEnv->num, iEnvMax);
	mptEnv->nLoopStart = itEnv->lpb;
	mptEnv->nLoopEnd = itEnv->lpe;
	mptEnv->nSustainStart = itEnv->slb;
	mptEnv->nSustainEnd = itEnv->sle;

	// Attention: Full MPTM envelope is stored in extended instrument properties
	for (UINT ev = 0; ev < 25; ev++)
	{
		mptEnv->Values[ev] = itEnv->data[ev * 3] + envOffset;
		mptEnv->Ticks[ev] = (itEnv->data[ev * 3 + 2] << 8) | (itEnv->data[ev * 3 + 1]);
		if(ev > 0 && ev < itEnv->num && mptEnv->Ticks[ev] < mptEnv->Ticks[ev - 1])
		{
			// Fix broken envelopes... Instruments 2 and 3 in NoGap.it by Werewolf have envelope points where the high byte of envelope nodes is missing.
			// NoGap.it was saved with MPT 1.07 or MPT 1.09, which *normally* doesn't do this in IT files.
			// However... It turns out that MPT 1.07 omitted the high byte of envelope nodes when saving an XI instrument file, and it looks like
			// Instrument 2 and 3 in NoGap.it were loaded from XI files.
			mptEnv->Ticks[ev] &= 0xFF;
			mptEnv->Ticks[ev] |= (mptEnv->Ticks[ev] & ~0xFF);
			if(mptEnv->Ticks[ev] < mptEnv->Ticks[ev - 1])
			{
				mptEnv->Ticks[ev] += 0x100;
			}
		}
	}
	// Sanitize envelope
	mptEnv->Ticks[0] = 0;
}


//BOOL CSoundFile::ITInstrToMPT(const void *p, MODINSTRUMENT *pIns, UINT trkvers)
long CSoundFile::ITInstrToMPT(const void *p, MODINSTRUMENT *pIns, UINT trkvers) //rewbs.modularInstData
//-----------------------------------------------------------------------------
{
	// Envelope point count. Limited to 25 in IT format.
	const int iEnvMax = (m_nType & MOD_TYPE_MPT) ? MAX_ENVPOINTS : 25;

	long returnVal=0;
	if (trkvers < 0x0200)
	{
		const ITOLDINSTRUMENT *pis = (const ITOLDINSTRUMENT *)p;
		memcpy(pIns->name, pis->name, 26);
		memcpy(pIns->filename, pis->filename, 12);
		StringFixer::SpaceToNullStringFixed<26>(pIns->name);
		StringFixer::SpaceToNullStringFixed<12>(pIns->filename);
		pIns->nFadeOut = pis->fadeout << 6;
		pIns->nGlobalVol = 64;
		for (UINT j = 0; j < 120; j++)
		{
			UINT note = pis->keyboard[j*2];
			UINT ins = pis->keyboard[j*2+1];
			if (ins < MAX_SAMPLES) pIns->Keyboard[j] = ins;
			if (note < 120) pIns->NoteMap[j] = note + 1;
			else pIns->NoteMap[j] = j + 1;
		}
		if (pis->flags & 0x01) pIns->VolEnv.dwFlags |= ENV_ENABLED;
		if (pis->flags & 0x02) pIns->VolEnv.dwFlags |= ENV_LOOP;
		if (pis->flags & 0x04) pIns->VolEnv.dwFlags |= ENV_SUSTAIN;
		pIns->VolEnv.nLoopStart = pis->vls;
		pIns->VolEnv.nLoopEnd = pis->vle;
		pIns->VolEnv.nSustainStart = pis->sls;
		pIns->VolEnv.nSustainEnd = pis->sle;
		pIns->VolEnv.nNodes = 25;
		for (UINT ev=0; ev<25; ev++)
		{
			if ((pIns->VolEnv.Ticks[ev] = pis->nodes[ev*2]) == 0xFF)
			{
				pIns->VolEnv.nNodes = ev;
				break;
			}
			pIns->VolEnv.Values[ev] = pis->nodes[ev*2+1];
		}

		pIns->nNNA = pis->nna;
		pIns->nDCT = pis->dnc;
		pIns->nPan = 0x80;
	} else
	{
		const ITINSTRUMENT *pis = (const ITINSTRUMENT *)p;
		memcpy(pIns->name, pis->name, 26);
		memcpy(pIns->filename, pis->filename, 12);
		StringFixer::SpaceToNullStringFixed<26>(pIns->name);
		StringFixer::SpaceToNullStringFixed<12>(pIns->filename);
		if (pis->mpr<=128)
			pIns->nMidiProgram = pis->mpr;
		pIns->nMidiChannel = pis->mch;
		if (pIns->nMidiChannel >= 128)	//rewbs.instroVSTi
		{								//(handle old format where midichan
										// and mixplug are 1 value)
			pIns->nMixPlug = pIns->nMidiChannel - 128;
			pIns->nMidiChannel = 0;
		}
		if (pis->mbank<=128)
			pIns->wMidiBank = pis->mbank;
		pIns->nFadeOut = pis->fadeout << 5;
		pIns->nGlobalVol = pis->gbv >> 1;
		if (pIns->nGlobalVol > 64) pIns->nGlobalVol = 64;
		for (UINT j = 0; j < 120; j++)
		{
			UINT note = pis->keyboard[j*2];
			UINT ins = pis->keyboard[j*2+1];
			if (ins < MAX_SAMPLES) pIns->Keyboard[j] = ins;
			if (note < 120) pIns->NoteMap[j] = note + 1;
			else pIns->NoteMap[j] = j + 1;
		}
		// Olivier's MPT Instrument Extension
		if (*((int *)pis->dummy) == 'MPTX')
		{
			const ITINSTRUMENTEX *pisex = (const ITINSTRUMENTEX *)pis;
			for (UINT k = 0; k < 120; k++)
			{
				pIns->Keyboard[k] |= ((UINT)pisex->keyboardhi[k] << 8);
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
						pIns->nMixPlug = *(pModularInst);
						pModularInst+=sizeof(pIns->nMixPlug);
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
		ITEnvToMPT(&pis->volenv, &pIns->VolEnv, 0, iEnvMax);
		// Panning Envelope
		ITEnvToMPT(&pis->panenv, &pIns->PanEnv, 32, iEnvMax);
		// Pitch Envelope
		ITEnvToMPT(&pis->pitchenv, &pIns->PitchEnv, 32, iEnvMax);
		if (pis->pitchenv.flags & 0x80) pIns->PitchEnv.dwFlags |= ENV_FILTER;

		pIns->nNNA = pis->nna;
		pIns->nDCT = pis->dct;
		pIns->nDNA = pis->dca;
		pIns->nPPS = pis->pps;
		pIns->nPPC = pis->ppc;
		pIns->SetCutoff(pis->ifc & 0x7F, (pis->ifc & 0x80) != 0);
		pIns->SetResonance(pis->ifr & 0x7F, (pis->ifr & 0x80) != 0);
		pIns->nVolSwing = min(pis->rv, 100);
		pIns->nPanSwing = min(pis->rp, 64);
		pIns->nPan = (pis->dfp & 0x7F) << 2;
		if (pIns->nPan > 256) pIns->nPan = 128;
		if (pis->dfp < 0x80) pIns->dwFlags |= INS_SETPANNING;
	}

	if ((pIns->VolEnv.nLoopStart >= iEnvMax) || (pIns->VolEnv.nLoopEnd >= iEnvMax)) pIns->VolEnv.dwFlags &= ~ENV_LOOP;
	if ((pIns->VolEnv.nSustainStart >= iEnvMax) || (pIns->VolEnv.nSustainEnd >= iEnvMax)) pIns->VolEnv.dwFlags &= ~ENV_SUSTAIN;

	return returnVal; //return offset
}


void CopyPatternName(CPattern &pattern, const char **patNames, UINT &patNamesLen)
//-------------------------------------------------------------------------------
{
	if(*patNames != nullptr && patNamesLen > 0)
	{
		pattern.SetName(*patNames, min(MAX_PATTERNNAME, patNamesLen));
		*patNames += MAX_PATTERNNAME;
		patNamesLen -= min(MAX_PATTERNNAME, patNamesLen);
	}
}


bool CSoundFile::ReadIT(const LPCBYTE lpStream, const DWORD dwMemLength)
//----------------------------------------------------------------------
{
	ITFILEHEADER *pifh = (ITFILEHEADER *)lpStream;

	DWORD dwMemPos = sizeof(ITFILEHEADER);
	vector<DWORD> inspos;
	vector<DWORD> smppos;
	vector<DWORD> patpos;

	bool interpretModPlugMade = false;
	bool hasModPlugExtensions = false;

	if ((!lpStream) || (dwMemLength < 0xC0)) return false;
	if ((pifh->id != LittleEndian(IT_IMPM) && pifh->id != LittleEndian(IT_MPTM)) || (pifh->insnum > 0xFF)
	 || (pifh->smpnum >= MAX_SAMPLES)) return false;
	if (dwMemPos + pifh->ordnum + pifh->insnum*4
	 + pifh->smpnum*4 + pifh->patnum*4 > dwMemLength) return false;


	DWORD mptStartPos = dwMemLength;
	memcpy(&mptStartPos, lpStream + (dwMemLength - sizeof(DWORD)), sizeof(DWORD));
	if(mptStartPos >= dwMemLength || mptStartPos < 0x100)
		mptStartPos = dwMemLength;

	if(pifh->id == LittleEndian(IT_MPTM))
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
			// Which tracker was used to made this?
			if((pifh->cwtv & 0xF000) == 0x5000)
			{
				// OpenMPT Version number (Major.Minor)
				// This will only be interpreted as "made with ModPlug" (i.e. disable compatible playback etc) if the "reserved" field is set to "OMPT" - else, compatibility was used.
				m_dwLastSavedWithVersion = (pifh->cwtv & 0x0FFF) << 16;
				if(pifh->reserved == LittleEndian(IT_OMPT))
					interpretModPlugMade = true;
			} else if(pifh->cmwt == 0x888 || pifh->cwtv == 0x888)
			{
				// OpenMPT 1.17 and 1.18 (raped IT format)
				// Exact version number will be determined later.
				interpretModPlugMade = true;
				m_dwLastSavedWithVersion = MAKE_VERSION_NUMERIC(1, 17, 00, 00);
			} else if(pifh->cwtv == 0x0217 && pifh->cmwt == 0x0200 && pifh->reserved == 0)
			{
				if(memchr(pifh->chnpan, 0xFF, sizeof(pifh->chnpan)) != NULL)
				{
					// ModPlug Tracker 1.16 (semi-raped IT format)
					m_dwLastSavedWithVersion = MAKE_VERSION_NUMERIC(1, 16, 00, 00);
				} else
				{
					// OpenMPT 1.17 disguised as this in compatible mode,
					// but never writes 0xFF in the pan map for unused channels (which is an invalid value).
					m_dwLastSavedWithVersion = MAKE_VERSION_NUMERIC(1, 17, 00, 00);
				}
				interpretModPlugMade = true;
			} else if(pifh->cwtv == 0x0214 && pifh->cmwt == 0x0202 && pifh->reserved == 0)
			{
				// ModPlug Tracker b3.3 - 1.09, instruments 557 bytes apart
				m_dwLastSavedWithVersion = MAKE_VERSION_NUMERIC(1, 09, 00, 00);
				interpretModPlugMade = true;
			}
			else if(pifh->cwtv == 0x0214 && pifh->cmwt == 0x0200 && pifh->reserved == 0)
			{
				// ModPlug Tracker 1.00a5, instruments 560 bytes apart
				m_dwLastSavedWithVersion = MAKE_VERSION_NUMERIC(1, 00, 00, A5);
				interpretModPlugMade = true;
			}
		}
		else // case: type == MOD_TYPE_MPT
		{
			if (pifh->cwtv >= verMptFileVerLoadLimit)
			{
				if (GetpModDoc())
					GetpModDoc()->AddToLog(str_LoadingIncompatibleVersion);
				return false;
			}
			else if (pifh->cwtv > verMptFileVer)
			{
				if (GetpModDoc())
					GetpModDoc()->AddToLog(str_LoadingMoreRecentVersion);
			}
		}
	}

	if(GetType() == MOD_TYPE_IT) mptStartPos = dwMemLength;

	// Read row highlights
	if((pifh->special & 0x04))
	{
		// MPT 1.09, 1.07 and most likely other old MPT versions leave this blank (0/0), but have the "special" flag set.
		// Newer versions of MPT and OpenMPT 1.17 *always* write 4/16 here.
		// Thus, we will just ignore those old versions.
		if(m_dwLastSavedWithVersion == 0 || m_dwLastSavedWithVersion >= MAKE_VERSION_NUMERIC(1, 17, 03, 02))
		{
			m_nDefaultRowsPerBeat = pifh->highlight_minor;
			m_nDefaultRowsPerMeasure = pifh->highlight_major;
		}
#ifdef DEBUG
		if((pifh->highlight_minor | pifh->highlight_major) == 0)
		{
			Log("IT Header: Row highlight is 0");
		}
#endif
	}

	if (pifh->flags & 0x08) m_dwSongFlags |= SONG_LINEARSLIDES;
	if (pifh->flags & 0x10) m_dwSongFlags |= SONG_ITOLDEFFECTS;
	if (pifh->flags & 0x20) m_dwSongFlags |= SONG_ITCOMPATGXX;
	if ((pifh->flags & 0x80) || (pifh->special & 0x08)) m_dwSongFlags |= SONG_EMBEDMIDICFG;
	if (pifh->flags & 0x1000) m_dwSongFlags |= SONG_EXFILTERRANGE;

	memcpy(m_szNames[0], pifh->songname, 26);
	StringFixer::SpaceToNullStringFixed<26>(m_szNames[0]);

	// Global Volume
	m_nDefaultGlobalVolume = pifh->globalvol << 1;
	if (m_nDefaultGlobalVolume > MAX_GLOBAL_VOLUME) m_nDefaultGlobalVolume = MAX_GLOBAL_VOLUME;
	if (pifh->speed) m_nDefaultSpeed = pifh->speed;
	m_nDefaultTempo = max(32, pifh->tempo); // tempo 31 is possible. due to conflicts with the rest of the engine, let's just clamp it to 32.
	m_nSamplePreAmp = min(pifh->mv, 128);

	// Reading Channels Pan Positions
	for (int ipan=0; ipan< 64; ipan++) if (pifh->chnpan[ipan] != 0xFF)
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
	if ((pifh->special & 0x01) && (pifh->msglength) && (pifh->msglength <= dwMemLength) && (pifh->msgoffset < dwMemLength - pifh->msglength))
	{
		// Generally, IT files should use CR for line endings. However, ChibiTracker uses LF. One could do...
		// if(pifh->cwtv == 0x0214 && pifh->cmwt == 0x0214 && pifh->reserved == LittleEndian(IT_CHBI)) --> Chibi detected.
		// But we'll just use autodetection here:
		ReadMessage(lpStream + pifh->msgoffset, pifh->msglength, leAutodetect);
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
		if(nordsize > GetModSpecifications().ordersMax)
		{
#ifdef MODPLUG_TRACKER
			CString str;
			str.Format(str_SequenceTruncationNote, nordsize, GetModSpecifications().ordersMax);
			if(GetpModDoc() != nullptr) GetpModDoc()->AddToLog(str);
#endif // MODPLUG_TRACKER
			nordsize = GetModSpecifications().ordersMax;
		}

		if(pifh->cwtv > 0x88A && pifh->cwtv <= 0x88D)
			dwMemPos += Order.Deserialize(lpStream+dwMemPos, dwMemLength-dwMemPos);
		else
		{
			Order.ReadAsByte(lpStream + dwMemPos, nordsize, dwMemLength - dwMemPos);
			dwMemPos += pifh->ordnum;
			//Replacing 0xFF and 0xFE with new corresponding indexes
			Order.Replace(0xFE, Order.GetIgnoreIndex());
			Order.Replace(0xFF, Order.GetInvalidPatIndex());
		}
	}

	// Find the first parapointer.
	// This is used for finding out whether the edit history is actually stored in the file or not,
	// as some early versions of Schism Tracker set the history flag, but didn't save anything.
	// We will consider the history invalid if it ends after the first parapointer.
	DWORD minptr = dwMemLength;

	// Reading Instrument Offsets
	inspos.resize(pifh->insnum);
	for(size_t n = 0; n < pifh->insnum; n++)
	{
		if(4 > dwMemLength - dwMemPos)
			return false;
		DWORD insptr = LittleEndian(*(DWORD *)(lpStream + dwMemPos));
		inspos[n] = insptr;
		if(insptr > 0)
		{
			minptr = min(minptr, insptr);
		}
		dwMemPos += 4;
	}

	// Reading Sample Offsets
	smppos.resize(pifh->smpnum);
	for(size_t n = 0; n < pifh->smpnum; n++)
	{
		if(4 > dwMemLength - dwMemPos)
			return false;
		DWORD smpptr = LittleEndian(*(DWORD *)(lpStream + dwMemPos));
		smppos[n] = smpptr;
		if(smpptr > 0)
		{
			minptr = min(minptr, smpptr);
		}
		dwMemPos += 4;
	}

	// Reading Pattern Offsets
	patpos.resize(pifh->patnum);
	for(size_t n = 0; n < pifh->patnum; n++)
	{
		if(4 > dwMemLength - dwMemPos)
			return false;
		DWORD patptr = LittleEndian(*(DWORD *)(lpStream + dwMemPos));
		patpos[n] = patptr;
		if(patptr > 0)
		{
			minptr = min(minptr, patptr);
		}
		dwMemPos += 4;
	}

	// Reading Patterns Offsets
	if(patpos.size() > GetModSpecifications().patternsMax)
	{
		// Hack: Note user here if file contains more patterns than what can be read.
#ifdef MODPLUG_TRACKER
		if(GetpModDoc() != nullptr)
		{
			CString str;
			str.Format(str_PatternSetTruncationNote, patpos.size(), GetModSpecifications().patternsMax);
			GetpModDoc()->AddToLog(str);
		}
#endif // MODPLUG_TRACKER
	}

	if(pifh->special & 0x01)
	{
		minptr = min(minptr, pifh->msgoffset);
	}

	// Reading IT Edit History Info
	// This is only supposed to be present if bit 1 of the special flags is set.
	// However, old versions of Schism and probably other trackers always set this bit
	// even if they don't write the edit history count. So we have to filter this out...
	// This is done by looking at the parapointers. If the history data end after
	// the first parapointer, we assume that it's actually no history data.
	if (dwMemPos + 2 < dwMemLength && (pifh->special & 0x02))
	{
		const size_t nflt = LittleEndianW(*((uint16*)(lpStream + dwMemPos)));
		dwMemPos += 2;

		if (nflt * 8 <= dwMemLength - dwMemPos && dwMemPos + nflt * 8 <= minptr)
		{
#ifdef MODPLUG_TRACKER
			if(GetpModDoc() != nullptr)
			{
				GetpModDoc()->GetFileHistory().resize(nflt);
				for(size_t n = 0; n < nflt; n++)
				{
					ITHISTORYSTRUCT itHistory = *((ITHISTORYSTRUCT *)(lpStream + dwMemPos));
					itHistory.fatdate = LittleEndianW(itHistory.fatdate);
					itHistory.fattime = LittleEndianW(itHistory.fattime);
					itHistory.runtime = LittleEndian(itHistory.runtime);

					FileHistory mptHistory;
					MemsetZero(mptHistory);
					// Decode FAT date and time
					mptHistory.loadDate.tm_year = ((itHistory.fatdate >> 9) & 0x7F) + 80;
					mptHistory.loadDate.tm_mon = CLAMP((itHistory.fatdate >> 5) & 0x0F, 1, 12) - 1;
					mptHistory.loadDate.tm_mday = CLAMP(itHistory.fatdate & 0x1F, 1, 31);
					mptHistory.loadDate.tm_hour = CLAMP((itHistory.fattime >> 11) & 0x1F, 0, 23);
					mptHistory.loadDate.tm_min = CLAMP((itHistory.fattime >> 5) & 0x3F, 0, 59);
					mptHistory.loadDate.tm_sec = CLAMP((itHistory.fattime & 0x1F) * 2, 0, 59);
					mptHistory.openTime = itHistory.runtime * (HISTORY_TIMER_PRECISION / 18.2f);
					GetpModDoc()->GetFileHistory().at(n) = mptHistory;

#ifdef DEBUG
					const uint32 seconds = (uint32)(((double)itHistory.runtime) / 18.2f);
					CHAR stime[128];
					wsprintf(stime, "IT Edit History: Loaded %04u-%02u-%02u %02u:%02u:%02u, open for %u:%02u:%02u (%u ticks)\n", ((itHistory.fatdate >> 9) & 0x7F) + 1980, (itHistory.fatdate >> 5) & 0x0F, itHistory.fatdate & 0x1F, (itHistory.fattime >> 11) & 0x1F, (itHistory.fattime >> 5) & 0x3F, (itHistory.fattime & 0x1F) * 2, seconds / 3600, (seconds / 60) % 60, seconds % 60, itHistory.runtime);
					Log(stime);
#endif // DEBUG

					dwMemPos += 8;
				}
			} else
#endif // MODPLUG_TRACKER
			{
				dwMemPos += nflt * 8;
			}
		} else
		{
			// Oops, we were not supposed to read this.
			dwMemPos -= 2;
		}
	}
	// Another non-conforming application is unmo3 < v2.4.0.1, which doesn't set the special bit
	// at all, but still writes the two edit history length bytes (zeroes)...
	else if(dwMemPos + 2 < dwMemLength && pifh->highlight_major == 0 && pifh->highlight_minor == 0 && pifh->cmwt == 0x0214 && pifh->cwtv == 0x0214 && pifh->reserved == 0 && (pifh->special & (0x02|0x04)) == 0)
	{
		const size_t nflt = LittleEndianW(*((uint16*)(lpStream + dwMemPos)));
		if(nflt == 0)
		{
			dwMemPos += 2;
		}
	}

	// Reading MIDI Output & Macros
	if (m_dwSongFlags & SONG_EMBEDMIDICFG)
	{
		if (dwMemPos + sizeof(MIDIMacroConfig) < dwMemLength)
		{
			memcpy(&m_MidiCfg, lpStream + dwMemPos, sizeof(MIDIMacroConfig));
			m_MidiCfg.Sanitize();
			dwMemPos += sizeof(MIDIMacroConfig);
		}
	}
	// Ignore MIDI data. Fixes some files like denonde.it that were made with old versions of Impulse Tracker (which didn't support Zxx filters) and have Zxx effects in the patterns.
	if (pifh->cwtv < 0x0214)
	{
		MemsetZero(m_MidiCfg.szMidiSFXExt);
		MemsetZero(m_MidiCfg.szMidiZXXExt);
		m_dwSongFlags |= SONG_EMBEDMIDICFG;
	}

	// Read pattern names: "PNAM"
	const char *patNames = nullptr;
	UINT patNamesLen = 0;
	if ((dwMemPos + 8 < dwMemLength) && (*((DWORD *)(lpStream+dwMemPos)) == LittleEndian(IT_PNAM)))
	{
		patNamesLen = *((DWORD *)(lpStream + dwMemPos + 4));
		dwMemPos += 8;
		if ((dwMemPos + patNamesLen <= dwMemLength) && (patNamesLen > 0))
		{
			patNames = (char *)(lpStream + dwMemPos);
			dwMemPos += patNamesLen;
		}
	}

	m_nChannels = GetModSpecifications().channelsMin;
	// Read channel names: "CNAM"
	if ((dwMemPos + 8 < dwMemLength) && (*((DWORD *)(lpStream+dwMemPos)) == LittleEndian(IT_CNAM)))
	{
		UINT len = *((DWORD *)(lpStream+dwMemPos+4));
		dwMemPos += 8;
		if ((dwMemPos + len <= dwMemLength) && (len <= MAX_BASECHANNELS * MAX_CHANNELNAME))
		{
			UINT n = len / MAX_CHANNELNAME;
			if (n > m_nChannels) m_nChannels = n;
			for (UINT i=0; i<n; i++)
			{
				memcpy(ChnSettings[i].szName, (lpStream + dwMemPos + i * MAX_CHANNELNAME), MAX_CHANNELNAME);
				ChnSettings[i].szName[MAX_CHANNELNAME - 1] = 0;
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
		if ((!patpos[patchk]) || ((DWORD)patpos[patchk] >= dwMemLength - 4))
			continue;

		UINT len = *((WORD *)(lpStream+patpos[patchk]));
		UINT rows = *((WORD *)(lpStream+patpos[patchk]+2));

		if(rows <= ModSpecs::itEx.patternRowsMax && rows > ModSpecs::it.patternRowsMax)
		{
			//interpretModPlugMade = true;	// Chibi also does this.
			hasModPlugExtensions = true;
		}

		if ((rows < GetModSpecifications().patternRowsMin) || (rows > GetModSpecifications().patternRowsMax))
			continue;

		if (patpos[patchk]+8+len > dwMemLength)
			continue;

		UINT i = 0;
		const BYTE *p = lpStream+patpos[patchk]+8;
		UINT nrow = 0;

		vector<BYTE> chnmask;

		while (nrow<rows)
		{
			if (i >= len) break;
			BYTE b = p[i++]; // p is the bytestream offset at current pattern_position
			if (!b)
			{
				nrow++;
				continue;
			}

			UINT ch = b & IT_bitmask_patternChanField_c;   // 0x7f We have some data grab a byte keeping only 7 bits
			if (ch)
				ch = (ch - 1);// & IT_bitmask_patternChanMask_c;   // 0x3f mask of the byte again, keeping only 6 bits

			if(ch >= chnmask.size())
			{
				chnmask.resize(ch + 1, 0);
			}

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
		if ((inspos[nins] > 0) && (inspos[nins] <= dwMemLength - (pifh->cmwt < 0x200 ? sizeof(ITOLDINSTRUMENT) : sizeof(ITINSTRUMENT))))
		{
			try
			{
				Instruments[nins + 1] = new MODINSTRUMENT();
				ITInstrToMPT(lpStream + inspos[nins], Instruments[nins + 1], pifh->cmwt);
			} catch(MPTMemoryException)
			{
				continue;
			}
		}
	}

// -> CODE#0027
// -> DESC="per-instrument volume ramping setup (refered as attack)"
	// In order to properly compute the position, in file, of eventual extended settings
	// such as "attack" we need to keep the "real" size of the last sample as those extra
	// setting will follow this sample in the file
	UINT lastSampleOffset = 0;
	if(pifh->smpnum > 0)
	{
		lastSampleOffset = smppos[pifh->smpnum - 1] + sizeof(ITSAMPLESTRUCT);
	}
// -! NEW_FEATURE#0027

	// Reading Samples
	m_nSamples = min(pifh->smpnum, MAX_SAMPLES - 1);
	for (UINT nsmp = 0; nsmp < m_nSamples; nsmp++) if ((smppos[nsmp]) && (smppos[nsmp] <= dwMemLength - sizeof(ITSAMPLESTRUCT)))
	{
		ITSAMPLESTRUCT *pis = (ITSAMPLESTRUCT *)(lpStream+smppos[nsmp]);
		if (pis->id == LittleEndian(IT_IMPS))
		{
			MODSAMPLE *pSmp = &Samples[nsmp+1];
			memcpy(pSmp->filename, pis->filename, 12);
			StringFixer::SpaceToNullStringFixed<12>(pSmp->filename);
			pSmp->uFlags = 0;
			pSmp->nLength = 0;
			pSmp->nLoopStart = pis->loopbegin;
			pSmp->nLoopEnd = pis->loopend;
			pSmp->nSustainStart = pis->susloopbegin;
			pSmp->nSustainEnd = pis->susloopend;
			pSmp->nC5Speed = pis->C5Speed;
			if (!pSmp->nC5Speed) pSmp->nC5Speed = 8363;
			if (pis->C5Speed < 256) pSmp->nC5Speed = 256;
			pSmp->nVolume = pis->vol << 2;
			if (pSmp->nVolume > 256) pSmp->nVolume = 256;
			pSmp->nGlobalVol = pis->gvl;
			if (pSmp->nGlobalVol > 64) pSmp->nGlobalVol = 64;
			if (pis->flags & 0x10) pSmp->uFlags |= CHN_LOOP;
			if (pis->flags & 0x20) pSmp->uFlags |= CHN_SUSTAINLOOP;
			if (pis->flags & 0x40) pSmp->uFlags |= CHN_PINGPONGLOOP;
			if (pis->flags & 0x80) pSmp->uFlags |= CHN_PINGPONGSUSTAIN;
			pSmp->nPan = (pis->dfp & 0x7F) << 2;
			if (pSmp->nPan > 256) pSmp->nPan = 256;
			if (pis->dfp & 0x80) pSmp->uFlags |= CHN_PANNING;
			pSmp->nVibType = autovibit2xm[pis->vit & 7];
			pSmp->nVibRate = pis->vis;
			pSmp->nVibDepth = pis->vid & 0x7F;
			pSmp->nVibSweep = pis->vir; //(pis->vir + 3) / 4;

			if(pis->samplepointer) lastSampleOffset = pis->samplepointer; // MPTX hack

			if ((pis->samplepointer) && (pis->samplepointer < dwMemLength) && (pis->length))
			{
				pSmp->nLength = pis->length;
				if (pSmp->nLength > MAX_SAMPLE_LENGTH) pSmp->nLength = MAX_SAMPLE_LENGTH;
				UINT flags = (pis->cvt & 1) ? RS_PCM8S : RS_PCM8U;
				if (pis->flags & 2)	// 16-bit
				{
					flags += 5;
					if (pis->flags & 4 && pifh->cwtv >= 0x214) flags |= RSF_STEREO;	// some old version of IT didn't clear the stereo flag when importing samples. Luckily, all other trackers are identifying as IT 2.14+, so let's check for old IT versions.
					pSmp->uFlags |= CHN_16BIT;
					// IT 2.14 16-bit packed sample?
					if (pis->flags & 8) flags = ((pifh->cmwt >= 0x215) && (pis->cvt & 4)) ? RS_IT21516 : RS_IT21416;
				} else	// 8-bit
				{
					if (pis->flags & 4 && pifh->cwtv >= 0x214) flags |= RSF_STEREO;	// some old version of IT didn't clear the stereo flag when importing samples. Luckily, all other trackers are identifying as IT 2.14+, so let's check for old IT versions.
					if (pis->cvt == 0xFF) flags = RS_ADPCM4; else
					// IT 2.14 8-bit packed sample?
					if (pis->flags & 8)	flags =	((pifh->cmwt >= 0x215) && (pis->cvt & 4)) ? RS_IT2158 : RS_IT2148;
				}
// -> CODE#0027
// -> DESC="per-instrument volume ramping setup (refered as attack)"
//				ReadSample(&Ins[nsmp+1], flags, (LPSTR)(lpStream+pis->samplepointer), dwMemLength - pis->samplepointer);
				lastSampleOffset = pis->samplepointer + ReadSample(&Samples[nsmp+1], flags, (LPSTR)(lpStream+pis->samplepointer), dwMemLength - pis->samplepointer);
// -! NEW_FEATURE#0027
			}
		}
		memcpy(m_szNames[nsmp + 1], pis->name, 26);
		StringFixer::SpaceToNullStringFixed<26>(m_szNames[nsmp + 1]);
	}
	m_nSamples = max(1, m_nSamples);

	m_nMinPeriod = 8;
	m_nMaxPeriod = 0xF000;

// -> CODE#0027
// -> DESC="per-instrument volume ramping setup (refered as attack)"

	// Compute extra instruments settings position
	if(lastSampleOffset > 0) dwMemPos = lastSampleOffset;

	// Load instrument and song extensions.
	if(mptStartPos >= dwMemPos)
	{
		if(interpretModPlugMade)
		{
			m_nMixLevels = mixLevels_original;
		}
		LPCBYTE ptr = LoadExtendedInstrumentProperties(lpStream + dwMemPos, lpStream + mptStartPos, &interpretModPlugMade);
		LoadExtendedSongProperties(GetType(), ptr, lpStream, mptStartPos, &interpretModPlugMade);
	}

// -! NEW_FEATURE#0027


	// Reading Patterns
	Patterns.ResizeArray(max(MAX_PATTERNS, npatterns));
	for (UINT npat=0; npat<npatterns; npat++)
	{
		if ((!patpos[npat]) || ((DWORD)patpos[npat] >= dwMemLength - 4))
		{
			if(Patterns.Insert(npat, 64))
			{
#ifdef MODPLUG_TRACKER
				CString s;
				s.Format(TEXT("Allocating patterns failed starting from pattern %u\n"), npat);
				if(GetpModDoc() != nullptr) GetpModDoc()->AddToLog(s);
#endif // MODPLUG_TRACKER
				break;
			}
			// Now (after the Insert() call), we can read the pattern name.
			CopyPatternName(Patterns[npat], &patNames, patNamesLen);
			continue;
		}
		UINT len = *((WORD *)(lpStream+patpos[npat]));
		UINT rows = *((WORD *)(lpStream+patpos[npat]+2));
		if ((rows < GetModSpecifications().patternRowsMin) || (rows > GetModSpecifications().patternRowsMax)) continue;
		if (patpos[npat] + 8 + len > dwMemLength) continue;

		if(Patterns.Insert(npat, rows)) continue;

		// Now (after the Insert() call), we can read the pattern name.
		CopyPatternName(Patterns[npat], &patNames, patNamesLen);

		vector<BYTE> chnmask;
		vector<MODCOMMAND> lastvalue;

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

			if(ch >= chnmask.size())
			{
				chnmask.resize(ch + 1, 0);
				lastvalue.resize(ch + 1, MODCOMMAND::Empty());
				ASSERT(chnmask.size() <= GetNumChannels());
			}

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
					if(!(m_nType & MOD_TYPE_MPT))
					{
						if(note > NOTE_MAX && note < 0xFD) note = NOTE_FADE;
						else if(note == 0xFD) note = NOTE_NONE;
					}
					m[ch].note = note;
					lastvalue[ch].note = note;
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
					// 203-212: Vibrato depth
					if ((vol >= 203) && (vol <= 212))
					{
						m[ch].volcmd = VOLCMD_VIBRATODEPTH; m[ch].vol = vol - 203;
						// Old versions of ModPlug saved this as vibrato speed instead, so let's fix that
						if(m_dwLastSavedWithVersion && m_dwLastSavedWithVersion <= MAKE_VERSION_NUMERIC(1, 17, 02, 54))
							m[ch].volcmd = VOLCMD_VIBRATOSPEED;
					} else
					// 213-222: Unused (was velocity)
					// 223-232: Offset
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
						S3MConvert(&m[ch], true);
						lastvalue[ch].command = m[ch].command;
						lastvalue[ch].param = m[ch].param;
					}
				}
			}
		}
	}

	UpgradeModFlags();

	if(GetType() == MOD_TYPE_IT)
	{
		// Set appropriate mod flags if the file was not made with MPT.
		if(!interpretModPlugMade)
		{
			SetModFlag(MSF_MIDICC_BUGEMULATION, false);
			SetModFlag(MSF_OLDVOLSWING, false);
			SetModFlag(MSF_COMPATIBLE_PLAY, true);
		}
	}
	else
	{
		//START - mpt specific:
		//Using member cwtv on pifh as the version number.
		const uint16 version = pifh->cwtv;
		if(version > 0x889)
		{
			const char* const cpcMPTStart = reinterpret_cast<const char*>(lpStream + mptStartPos);
			std::istrstream iStrm(cpcMPTStart, dwMemLength-mptStartPos);

			if(version >= 0x88D)
			{
				srlztn::Ssb ssb(iStrm);
				ssb.BeginRead("mptm", MptVersion::num);
				ssb.ReadItem(GetTuneSpecificTunings(), "0", 1, &ReadTuningCollection);
				ssb.ReadItem(*this, "1", 1, &ReadTuningMap);
				ssb.ReadItem(Order, "2", 1, &ReadModSequenceOld);
				ssb.ReadItem(Patterns, FileIdPatterns, strlen(FileIdPatterns), &ReadModPatterns);
				ssb.ReadItem(Order, FileIdSequences, strlen(FileIdSequences), &ReadModSequences);

				if (ssb.m_Status & srlztn::SNT_FAILURE)
				{
#ifdef MODPLUG_TRACKER
					if(GetpModDoc() != nullptr) GetpModDoc()->AddToLog(_T("Unknown error occured while deserializing file.\n"));
#endif // MODPLUG_TRACKER
				}
			}
			else //Loading for older files.
			{
				if(GetTuneSpecificTunings().Deserialize(iStrm))
				{
#ifdef MODPLUG_TRACKER
					if(GetpModDoc() != nullptr) GetpModDoc()->AddToLog(_T("Error occured - loading failed while trying to load tune specific tunings.\n"));
#endif // MODPLUG_TRACKER
				}
				else
				{
					ReadTuningMap(iStrm, *this);
				}
			}
		} //version condition(MPT)
	}

	return true;
}
//end plastiq: code readability improvements

#ifndef MODPLUG_NO_FILESAVE

// Save edit history. Pass a null pointer for *f to retrieve the number of bytes that would be written.
DWORD SaveITEditHistory(const CSoundFile *pSndFile, FILE *f)
//----------------------------------------------------------
{
#ifdef MODPLUG_TRACKER
	CModDoc *pModDoc = pSndFile->GetpModDoc();
	const size_t num = (pModDoc != nullptr) ? pModDoc->GetFileHistory().size() + 1 : 0;	// + 1 for this session
#else
	const size_t num = 0;
#endif // MODPLUG_TRACKER

	uint16 fnum = min(num, uint16_max);	// Number of entries that are actually going to be written
	const size_t bytes_written = 2 + fnum * 8;	// Number of bytes that are actually going to be written

	if(f == nullptr)
		return bytes_written;

	// Write number of history entries
	fnum = LittleEndianW(fnum);
	fwrite(&fnum, 2, 1, f);

#ifdef MODPLUG_TRACKER
	// Write history data
	const size_t start = (num > uint16_max) ? num - uint16_max : 0;
	for(size_t n = start; n < num; n++)
	{
		tm loadDate;
		MemsetZero(loadDate);
		uint32 openTime;

		if(n < num - 1)
		{
			// Previous timestamps
			const FileHistory *mptHistory = &(pModDoc->GetFileHistory().at(n));
			loadDate = mptHistory->loadDate;
			openTime = mptHistory->openTime * (18.2f / HISTORY_TIMER_PRECISION);
		} else
		{
			// Current ("new") timestamp
			const time_t creationTime = pModDoc->GetCreationTime();
			//localtime_s(&loadDate, &creationTime);
			const tm* const p = localtime(&creationTime);
			if (p != nullptr)
				loadDate = *p;
			else if (pSndFile->GetpModDoc() != nullptr)
				pSndFile->GetpModDoc()->AddLogEvent(LogEventUnexpectedError,
													  __FUNCTION__,
													  _T("localtime() returned nullptr."));
			openTime = (uint32)((double)difftime(time(nullptr), creationTime) * 18.2f);
		}

		ITHISTORYSTRUCT itHistory;
		// Create FAT file dates
		itHistory.fatdate = loadDate.tm_mday | ((loadDate.tm_mon + 1) << 5) | ((loadDate.tm_year - 80) << 9);
		itHistory.fattime = (loadDate.tm_sec / 2) | (loadDate.tm_min << 5) | (loadDate.tm_hour << 11);
		itHistory.runtime = openTime;

		itHistory.fatdate = LittleEndianW(itHistory.fatdate);
		itHistory.fattime = LittleEndianW(itHistory.fattime);
		itHistory.runtime = LittleEndian(itHistory.runtime);

		fwrite(&itHistory, 1, sizeof(itHistory), f);
	}
#endif // MODPLUG_TRACKER

	return bytes_written;
}

#pragma warning(disable:4100)


bool CSoundFile::SaveIT(LPCSTR lpszFileName, UINT nPacking, const bool compatExport)
//----------------------------------------------------------------------------------
{
	const CModSpecifications &specs = (GetType() == MOD_TYPE_MPT ? ModSpecs::mptm : (compatExport ? ModSpecs::it : ModSpecs::itEx));

	DWORD dwChnNamLen;
	ITFILEHEADER header;
	ITINSTRUMENT iti;
	ITSAMPLESTRUCT itss;
	vector<bool>smpcount(GetNumSamples(), false);
	DWORD dwPos = 0, dwHdrPos = 0, dwExtra = 0;
	FILE *f;


	if ((!lpszFileName) || ((f = fopen(lpszFileName, "wb")) == NULL)) return false;

	// Writing Header
	MemsetZero(header);
	dwChnNamLen = 0;
	header.id = LittleEndian(IT_IMPM);
	lstrcpyn(header.songname, m_szNames[0], 26);

	header.highlight_minor = (BYTE)min(m_nDefaultRowsPerBeat, 0xFF);
	header.highlight_major = (BYTE)min(m_nDefaultRowsPerMeasure, 0xFF);

	if(GetType() == MOD_TYPE_MPT)
	{
		if(!Order.NeedsExtraDatafield()) header.ordnum = Order.size();
		else header.ordnum = min(Order.size(), MAX_ORDERS); //Writing MAX_ORDERS at max here, and if there's more, writing them elsewhere.

		//Crop unused orders from the end.
		while(header.ordnum > 1 && Order[header.ordnum - 1] == Order.GetInvalidPatIndex()) header.ordnum--;
	} else
	{
		// An additional "---" pattern is appended so Impulse Tracker won't ignore the last order item.
		// Interestingly, this can exceed IT's 256 order limit. Also, IT will always save at least two orders.
		header.ordnum = min(Order.GetLengthTailTrimmed(), specs.ordersMax) + 1;
		if(header.ordnum < 2) header.ordnum = 2;
	}

	header.insnum = min(m_nInstruments, specs.instrumentsMax);
	header.smpnum = min(m_nSamples, specs.samplesMax);
	header.patnum = min(Patterns.GetNumPatterns(), specs.patternsMax);

	// Parapointers
	vector<DWORD> patpos(header.patnum, 0);
	vector<DWORD> smppos(header.smpnum, 0);
	vector<DWORD> inspos(header.insnum, 0);

	//VERSION
	if(GetType() == MOD_TYPE_MPT)
	{
		header.cwtv = verMptFileVer;	// Used in OMPT-hack versioning.
		header.cmwt = 0x888;
	}
	else //IT
	{
		MptVersion::VersionNum vVersion = MptVersion::num;
		header.cwtv = LittleEndianW(0x5000 | (WORD)((vVersion >> 16) & 0x0FFF)); // format: txyy (t = tracker ID, x = version major, yy = version minor), e.g. 0x5117 (OpenMPT = 5, 117 = v1.17)
		header.cmwt = LittleEndianW(0x0214);	// Common compatible tracker :)
		// Hack from schism tracker:
		for(INSTRUMENTINDEX nIns = 1; nIns <= GetNumInstruments(); nIns++)
		{
			if(Instruments[nIns] && Instruments[nIns]->PitchEnv.dwFlags & ENV_FILTER)
			{
				header.cmwt = LittleEndianW(0x0216);
				break;
			}
		}

		if(!compatExport)
		{
			// This way, we indicate that the file will most likely contain OpenMPT hacks. Compatibility export puts 0 here.
			header.reserved = LittleEndian(IT_OMPT);
		}
	}

	header.flags = 0x0001;
	header.special = 0x02 | 0x04 ;	// 0x02: embed file edit history, 0x04: store row highlight in the header
	if (m_nInstruments) header.flags |= 0x04;
	if (m_dwSongFlags & SONG_LINEARSLIDES) header.flags |= 0x08;
	if (m_dwSongFlags & SONG_ITOLDEFFECTS) header.flags |= 0x10;
	if (m_dwSongFlags & SONG_ITCOMPATGXX) header.flags |= 0x20;
	if ((m_dwSongFlags & SONG_EXFILTERRANGE) && !compatExport) header.flags |= 0x1000;
	header.globalvol = m_nDefaultGlobalVolume >> 1;
	header.mv = min(m_nSamplePreAmp, 128);
	header.speed = m_nDefaultSpeed;
 	header.tempo = min(m_nDefaultTempo, 255);  //Limit this one to 255, we save the real one as an extension below.
	header.sep = 128; // pan separation
	dwHdrPos = sizeof(header) + header.ordnum;
	// Channel Pan and Volume
	memset(header.chnpan, 0xA0, 64);
	memset(header.chnvol, 64, 64);

	for (CHANNELINDEX ich = 0; ich < min(m_nChannels, 64); ich++) // Header only has room for settings for 64 chans...
	{
		header.chnpan[ich] = ChnSettings[ich].nPan >> 2;
		if (ChnSettings[ich].dwFlags & CHN_SURROUND) header.chnpan[ich] = 100;
		header.chnvol[ich] = ChnSettings[ich].nVolume;
		if (ChnSettings[ich].dwFlags & CHN_MUTE) header.chnpan[ich] |= 0x80;
	}

	// Channel names
	if(!compatExport)
	{
		for (UINT ich=0; ich<m_nChannels; ich++)
		{
			if (ChnSettings[ich].szName[0])
			{
				dwChnNamLen = (ich + 1) * MAX_CHANNELNAME;
			}
		}
		if (dwChnNamLen) dwExtra += dwChnNamLen + 8;
	}

	if (m_dwSongFlags & SONG_EMBEDMIDICFG)
	{
		header.flags |= 0x80;
		header.special |= 0x08;
		dwExtra += sizeof(MIDIMacroConfig);
	}

	// Pattern Names
	const PATTERNINDEX numNamedPats = Patterns.GetNumNamedPatterns();
	if(numNamedPats > 0 && !compatExport)
	{
		dwExtra += (numNamedPats * MAX_PATTERNNAME) + 8;
	}

	// Mix Plugins. Just calculate the size of this extra block for now.
	if(!compatExport)
	{
		dwExtra += SaveMixPlugins(NULL, TRUE);
	}

	// Edit History. Just calculate the size of this extra block for now.
	dwExtra += SaveITEditHistory(this, nullptr);

	// Comments
	if (m_lpszSongComments)
	{
		header.special |= 1;
		header.msglength = strlen(m_lpszSongComments) + 1;
		header.msgoffset = dwHdrPos + dwExtra + (header.insnum + header.smpnum + header.patnum) * 4;
	}

	// Write file header
	fwrite(&header, 1, sizeof(header), f);
	Order.WriteAsByte(f, header.ordnum);
	if (header.insnum) fwrite(&inspos[0], 4, header.insnum, f);
	if (header.smpnum) fwrite(&smppos[0], 4, header.smpnum, f);
	if (header.patnum) fwrite(&patpos[0], 4, header.patnum, f);

	// Writing edit history information
	SaveITEditHistory(this, f);

	// Writing midi cfg
	if (header.flags & 0x80)
	{
		fwrite(&m_MidiCfg, 1, sizeof(MIDIMacroConfig), f);
	}

	// Writing pattern names
	if (numNamedPats && !compatExport)
	{
		DWORD d = LittleEndian(IT_PNAM); // "PNAM"
		fwrite(&d, 1, 4, f);
		d = numNamedPats * MAX_PATTERNNAME;
		fwrite(&d, 1, 4, f);

		for(PATTERNINDEX nPat = 0; nPat < numNamedPats; nPat++)
		{
			char name[MAX_PATTERNNAME];
			MemsetZero(name);
			Patterns[nPat].GetName(name, MAX_PATTERNNAME);
			fwrite(name, 1, MAX_PATTERNNAME, f);
		}
	}

	// Writing channel names
	if (dwChnNamLen && !compatExport)
	{
		DWORD d = LittleEndian(IT_CNAM); // "CNAM"
		fwrite(&d, 1, 4, f);
		fwrite(&dwChnNamLen, 1, 4, f);
		UINT nChnNames = dwChnNamLen / MAX_CHANNELNAME;
		for (UINT inam=0; inam<nChnNames; inam++)
		{
			fwrite(ChnSettings[inam].szName, 1, MAX_CHANNELNAME, f);
		}
	}

	// Writing mix plugins info
	if(!compatExport)
	{
		SaveMixPlugins(f, FALSE);
	}

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
		bool bKbdEx = false;	// extended sample map (for samples > 255)
		BYTE keyboardex[NOTE_MAX];

		MemsetZero(iti);
		iti.id = LittleEndian(IT_IMPI);	// "IMPI"
		iti.trkvers = (compatExport ? LittleEndianW(0x0214) : LittleEndianW(0x220));	//rewbs.itVersion
		if (Instruments[nins])
		{
			MODINSTRUMENT *pIns = Instruments[nins];
			memcpy(iti.filename, pIns->filename, 12);
			memcpy(iti.name, pIns->name, 26);
			StringFixer::SetNullTerminator(iti.name);

			iti.mbank = pIns->wMidiBank;
			iti.mpr = pIns->nMidiProgram;
			if(pIns->nMidiChannel || pIns->nMixPlug == 0 || compatExport)
			{
				// Default. Prefer MIDI channel over mixplug to keep the semantics intact.
				iti.mch = pIns->nMidiChannel;
			} else
			{
				// Keep compatibility with MPT 1.16's instrument format if possible, as XMPlay / BASS also uses this.
				iti.mch = pIns->nMixPlug + 128;
			}
			iti.nna = pIns->nNNA;
			iti.dct = (pIns->nDCT < DCT_PLUGIN || !compatExport) ? pIns->nDCT : 0;
			iti.dca = pIns->nDNA;
			iti.fadeout = min(pIns->nFadeOut >> 5, 256);
			iti.pps = pIns->nPPS;
			iti.ppc = pIns->nPPC;
			iti.gbv = (BYTE)(pIns->nGlobalVol << 1);
			iti.dfp = (BYTE)(pIns->nPan >> 2);
			if (!(pIns->dwFlags & INS_SETPANNING)) iti.dfp |= 0x80;
			iti.rv = min(pIns->nVolSwing, 100);
			iti.rp = min(pIns->nPanSwing, 64);
			iti.ifc = pIns->GetCutoff() | (pIns->IsCutoffEnabled() ? 0x80 : 0x00);
			iti.ifr = pIns->GetResonance() | (pIns->IsResonanceEnabled() ? 0x80 : 0x00);
			iti.nos = 0;
			for (UINT i=0; i<NOTE_MAX; i++) if (pIns->Keyboard[i] < MAX_SAMPLES)
			{
				const UINT smp = pIns->Keyboard[i];
				if (smp && smp <= GetNumSamples() && !smpcount[smp - 1])
				{
					smpcount[smp - 1] = true;
					iti.nos++;
				}
				iti.keyboard[i*2] = (pIns->NoteMap[i] >= NOTE_MIN && pIns->NoteMap[i] <= NOTE_MAX) ? (pIns->NoteMap[i] - 1) : i;
				iti.keyboard[i*2+1] = smp;
				if (smp > 0xff && !compatExport) bKbdEx = true;
				keyboardex[i] = (smp>>8);
			} else keyboardex[i] = 0;
			// Writing Volume envelope
			MPTEnvToIT(&pIns->VolEnv, &iti.volenv, 0, 64);
			// Writing Panning envelope
			MPTEnvToIT(&pIns->PanEnv, &iti.panenv, 32, 32);
			// Writing Pitch Envelope
			MPTEnvToIT(&pIns->PitchEnv, &iti.pitchenv, 32, 32);
			if (pIns->PitchEnv.dwFlags & ENV_FILTER) iti.pitchenv.flags |= 0x80;
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
		if (Instruments[nins] && !compatExport)
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
				MODINSTRUMENT *pIns = Instruments[nins];
				fwrite(&(pIns->nMixPlug), 1, sizeof(BYTE), f);
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
	MemsetZero(itss);
	for (UINT hsmp=0; hsmp<header.smpnum; hsmp++)
	{
		smppos[hsmp] = dwPos;
		dwPos += sizeof(ITSAMPLESTRUCT);
		fwrite(&itss, 1, sizeof(ITSAMPLESTRUCT), f);
	}

	// Writing Patterns
	bool bNeedsMptPatSave = false;
	for (UINT npat=0; npat<header.patnum; npat++)
	{
		DWORD dwPatPos = dwPos;
		if (!Patterns[npat]) continue;

		if(Patterns[npat].GetOverrideSignature())
			bNeedsMptPatSave = true;

		// Check for empty pattern
		if (Patterns[npat].GetNumRows() == 64 && Patterns.IsPatternEmpty(npat))
		{
			patpos[npat] = 0;
			continue;
		}

		patpos[npat] = dwPos;

		// Write pattern header
		WORD patinfo[4];
		patinfo[0] = 0;
		patinfo[1] = LittleEndianW(Patterns[npat].GetNumRows());
		patinfo[2] = 0;
		patinfo[3] = 0;

		fwrite(patinfo, 8, 1, f);
		dwPos += 8;

		const CHANNELINDEX maxChannels = min(specs.channelsMax, GetNumChannels());
		vector<BYTE> chnmask(maxChannels, 0xFF);
		vector<MODCOMMAND> lastvalue(maxChannels, MODCOMMAND::Empty());

		for (ROWINDEX row = 0; row<Patterns[npat].GetNumRows(); row++)
		{
			UINT len = 0;
			BYTE buf[8 * MAX_BASECHANNELS];
			const MODCOMMAND *m = Patterns[npat].GetRow(row);

			for (CHANNELINDEX ch = 0; ch < maxChannels; ch++, m++)
			{
				// Skip mptm-specific notes.
				if(m->IsPcNote())
				{
					bNeedsMptPatSave = true;
					continue;
				}

				BYTE b = 0;
				UINT command = m->command;
				UINT param = m->param;
				UINT vol = 0xFF;
				UINT note = m->note;
				if (note) b |= 1;
				if ((note) && (note < NOTE_MIN_SPECIAL)) note--;
				if (note == NOTE_FADE && GetType() != MOD_TYPE_MPT) note = 0xF6;
				if (m->instr) b |= 2;
				if (m->volcmd)
				{
					UINT volcmd = m->volcmd;
					switch(volcmd)
					{
					case VOLCMD_VOLUME:			vol = m->vol; if (vol > 64) vol = 64; break;
					case VOLCMD_PANNING:		vol = m->vol + 128; if (vol > 192) vol = 192; break;
					case VOLCMD_VOLSLIDEUP:		vol = 85 + ConvertVolParam(m); break;
					case VOLCMD_VOLSLIDEDOWN:	vol = 95 + ConvertVolParam(m); break;
					case VOLCMD_FINEVOLUP:		vol = 65 + ConvertVolParam(m); break;
					case VOLCMD_FINEVOLDOWN:	vol = 75 + ConvertVolParam(m); break;
					case VOLCMD_VIBRATODEPTH:	vol = 203 + ConvertVolParam(m); break;
					case VOLCMD_VIBRATOSPEED:	if(command == CMD_NONE)
												{
													// illegal command -> move if possible
													command = CMD_VIBRATO;
													param = ConvertVolParam(m) << 4;
												} else
												{
													vol = 203;
												}
												break;
					case VOLCMD_TONEPORTAMENTO:	vol = 193 + ConvertVolParam(m); break;
					case VOLCMD_PORTADOWN:		vol = 105 + ConvertVolParam(m); break;
					case VOLCMD_PORTAUP:		vol = 115 + ConvertVolParam(m); break;
					case VOLCMD_OFFSET:			if(!compatExport) vol = 223 + ConvertVolParam(m); //rewbs.volOff
												break;
					default:					vol = 0xFF;
					}
				}
				if (vol != 0xFF) b |= 4;
				if (command)
				{
					S3MSaveConvert(&command, &param, true, compatExport);
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
#ifdef MODPLUG_TRACKER
				if(GetpModDoc())
				{
					CString str;
					str.Format("%s (%s %u)\n", str_tooMuchPatternData, str_pattern, npat);
					GetpModDoc()->AddToLog(str);
				}
#endif // MODPLUG_TRACKER
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
		const MODSAMPLE &sample = Samples[nsmp];
		MemsetZero(itss);
		memcpy(itss.filename, sample.filename, 12);
		memcpy(itss.name, m_szNames[nsmp], 26);
		StringFixer::SetNullTerminator(itss.name);

		itss.id = LittleEndian(IT_IMPS);
		itss.gvl = (BYTE)sample.nGlobalVol;

		UINT flags = RS_PCM8S;
		if(sample.nLength && sample.pSample)
		{
			itss.flags = 0x01;
			if (sample.uFlags & CHN_LOOP) itss.flags |= 0x10;
			if (sample.uFlags & CHN_SUSTAINLOOP) itss.flags |= 0x20;
			if (sample.uFlags & CHN_PINGPONGLOOP) itss.flags |= 0x40;
			if (sample.uFlags & CHN_PINGPONGSUSTAIN) itss.flags |= 0x80;
#ifndef NO_PACKING
			if (nPacking && !compatExport)
			{
				if ((!(sample.uFlags & (CHN_16BIT|CHN_STEREO)))
					&& (CanPackSample(sample.pSample, sample.nLength, nPacking)))
				{
					flags = RS_ADPCM4;
					itss.cvt = 0xFF;
				}
			} else
#endif // NO_PACKING
			{
				if (sample.uFlags & CHN_STEREO)
				{
					flags = RS_STPCM8S;
					itss.flags |= 0x04;
				}
				if (sample.uFlags & CHN_16BIT)
				{
					itss.flags |= 0x02;
					flags = (sample.uFlags & CHN_STEREO) ? RS_STPCM16S : RS_PCM16S;
				}
			}
			itss.cvt = 0x01;
		}
		else
		{
			itss.flags = 0x00;
		}

		itss.C5Speed = sample.nC5Speed;
		if (!itss.C5Speed) itss.C5Speed = 8363;
		itss.length = sample.nLength;
		itss.loopbegin = sample.nLoopStart;
		itss.loopend = sample.nLoopEnd;
		itss.susloopbegin = sample.nSustainStart;
		itss.susloopend = sample.nSustainEnd;
		itss.vol = sample.nVolume >> 2;
		itss.dfp = sample.nPan >> 2;
		itss.vit = autovibxm2it[sample.nVibType & 7];
		itss.vis = min(sample.nVibRate, 64);
		itss.vid = min(sample.nVibDepth, 32);
		itss.vir = min(sample.nVibSweep, 255); //(sample.nVibSweep < 64) ? sample.nVibSweep * 4 : 255;
		if (sample.uFlags & CHN_PANNING) itss.dfp |= 0x80;

		itss.samplepointer = dwPos;
		fseek(f, smppos[nsmp-1], SEEK_SET);
		fwrite(&itss, 1, sizeof(ITSAMPLESTRUCT), f);
		fseek(f, dwPos, SEEK_SET);
		if ((sample.pSample) && (sample.nLength))
		{
			dwPos += WriteSample(f, &sample, flags);
		}
	}

	//Save hacked-on extra info
	if(!compatExport)
	{
		SaveExtendedInstrumentProperties(header.insnum, f);
		SaveExtendedSongProperties(f);
	}

	// Updating offsets
	fseek(f, dwHdrPos, SEEK_SET);
	if (header.insnum) fwrite(&inspos[0], 4, header.insnum, f);
	if (header.smpnum) fwrite(&smppos[0], 4, header.smpnum, f);
	if (header.patnum) fwrite(&patpos[0], 4, header.patnum, f);

	if(GetType() == MOD_TYPE_IT)
	{
		fclose(f);
		return true;
	}

	//hack
	//BEGIN: MPT SPECIFIC:
	//--------------------

	fseek(f, 0, SEEK_END);
	std::ofstream fout(f);
	const DWORD MPTStartPos = fout.tellp();

	srlztn::Ssb ssb(fout);
	ssb.BeginWrite("mptm", MptVersion::num);

	if (GetTuneSpecificTunings().GetNumTunings() > 0)
		ssb.WriteItem(GetTuneSpecificTunings(), "0", 1, &WriteTuningCollection);
	if (AreNonDefaultTuningsUsed(*this))
		ssb.WriteItem(*this, "1", 1, &WriteTuningMap);
	if (Order.NeedsExtraDatafield())
		ssb.WriteItem(Order, "2", 1, &WriteModSequenceOld);
	if (bNeedsMptPatSave)
		ssb.WriteItem(Patterns, FileIdPatterns, strlen(FileIdPatterns), &WriteModPatterns);
	ssb.WriteItem(Order, FileIdSequences, strlen(FileIdSequences), &WriteModSequences);

	ssb.FinishWrite();

	if (ssb.m_Status & srlztn::SNT_FAILURE)
	{
#ifdef MODPLUG_TRACKER
		if(GetpModDoc())
			GetpModDoc()->AddToLog("Error occured in writing MPTM extensions.\n");
#endif // MODPLUG_TRACKER
	}

	//Last 4 bytes should tell where the hack mpt things begin.
	if(!fout.good())
	{
		fout.clear();
		fout.write(reinterpret_cast<const char*>(&MPTStartPos), sizeof(MPTStartPos));
		return false;
	}
	fout.write(reinterpret_cast<const char*>(&MPTStartPos), sizeof(MPTStartPos));
	fout.close();
	//END  : MPT SPECIFIC
	//-------------------

	//NO WRITING HERE ANYMORE.

	return true;
}


#pragma warning(default:4100)
#endif // MODPLUG_NO_FILESAVE


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
			pDst[dwPos] = (b215) ? bTemp2 : bTemp;

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
			pDst[dwPos] = (b215) ? wTemp2 : wTemp;

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
					fwrite(&(m_MixPlugins[i].defaultProgram), 1, sizeof(long), f);
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
	return nTotalSize;
}
#endif


UINT CSoundFile::LoadMixPlugins(const void *pData, UINT nLen)
//-----------------------------------------------------------
{
	const BYTE *p = (const BYTE *)pData;
	UINT nPos = 0;

	while (nLen - nPos >= 8)	// read 4 magic bytes + size
	{
		DWORD nPluginSize;
		UINT nPlugin;

		nPluginSize = *(DWORD *)(p + nPos + 4);
		if (nPluginSize > nLen - nPos - 8) break;

		// Channel FX
		if (!memcmp(p + nPos, "CHFX", 4))
		{
			for (size_t ch = 0; ch < MAX_BASECHANNELS; ch++) if (ch * 4 < nPluginSize)
			{
				ChnSettings[ch].nMixPlugin = *(DWORD *)(p + nPos + 8 + ch * 4);
			}
		}
		// Plugin Data
		else if (memcmp(p + nPos, "FX00", 4) >= 0 && memcmp(p + nPos, "FX99", 4) <= 0)
		{

			nPlugin = (p[nPos + 2] - '0') * 10 + (p[nPos + 3] - '0');			//calculate plug-in number.

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

					while (endPos - currPos >= 4) //cycle through all the bytes
					{
						// do we recognize this chunk?
						//rewbs.dryRatio
						//TODO: turn this into a switch statement like for modular instrument data
						if (!memcmp(p + currPos, "DWRT", 4))
						{
							currPos += 4; // move past ID
							if (endPos - currPos >= sizeof(float))
							{
								m_MixPlugins[nPlugin].fDryRatio = *(float*) (p+currPos);
								currPos += sizeof(float); //move past data
							}
						}
						//end rewbs.dryRatio
						//rewbs.plugDefaultProgram
						else if (!memcmp(p + currPos, "PROG", 4))
						{
							currPos += 4; // move past ID
							if (endPos - currPos >= sizeof(long))
							{
								m_MixPlugins[nPlugin].defaultProgram = *(long*) (p+currPos);
								currPos += sizeof(long); //move past data
							}
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


// Used only when saving IT, XM and MPTM.
// ITI, ITP saves using Ericus' macros etc...
// The reason is that ITs and XMs save [code][size][ins1.Value][ins2.Value]...
// whereas ITP saves [code][size][ins1.Value][code][size][ins2.Value]...
// too late to turn back....
void CSoundFile::SaveExtendedInstrumentProperties(UINT nInstruments, FILE* f) const
//---------------------------------------------------------------------------------
{
	__int32 code=0;

/*	if(Instruments[1] == NULL) {
		return;
	}*/

	code = 'MPTX';							// write extension header code
	fwrite(&code, 1, sizeof(__int32), f);

	if (nInstruments == 0)
		return;

	WriteInstrumentPropertyForAllInstruments('VR..', sizeof(MODINSTRUMENT().nVolRampUp),  f, nInstruments);
	WriteInstrumentPropertyForAllInstruments('MiP.', sizeof(MODINSTRUMENT().nMixPlug),    f, nInstruments);
	WriteInstrumentPropertyForAllInstruments('MC..', sizeof(MODINSTRUMENT().nMidiChannel),f, nInstruments);
	WriteInstrumentPropertyForAllInstruments('MP..', sizeof(MODINSTRUMENT().nMidiProgram),f, nInstruments);
	WriteInstrumentPropertyForAllInstruments('MB..', sizeof(MODINSTRUMENT().wMidiBank),   f, nInstruments);
	WriteInstrumentPropertyForAllInstruments('P...', sizeof(MODINSTRUMENT().nPan),        f, nInstruments);
	WriteInstrumentPropertyForAllInstruments('GV..', sizeof(MODINSTRUMENT().nGlobalVol),  f, nInstruments);
	WriteInstrumentPropertyForAllInstruments('FO..', sizeof(MODINSTRUMENT().nFadeOut),    f, nInstruments);
	WriteInstrumentPropertyForAllInstruments('R...', sizeof(MODINSTRUMENT().nResampling), f, nInstruments);
	WriteInstrumentPropertyForAllInstruments('CS..', sizeof(MODINSTRUMENT().nCutSwing),   f, nInstruments);
	WriteInstrumentPropertyForAllInstruments('RS..', sizeof(MODINSTRUMENT().nResSwing),   f, nInstruments);
	WriteInstrumentPropertyForAllInstruments('FM..', sizeof(MODINSTRUMENT().nFilterMode), f, nInstruments);
	WriteInstrumentPropertyForAllInstruments('PERN', sizeof(MODINSTRUMENT().PitchEnv.nReleaseNode ), f, nInstruments);
	WriteInstrumentPropertyForAllInstruments('AERN', sizeof(MODINSTRUMENT().PanEnv.nReleaseNode), f, nInstruments);
	WriteInstrumentPropertyForAllInstruments('VERN', sizeof(MODINSTRUMENT().VolEnv.nReleaseNode), f, nInstruments);
	WriteInstrumentPropertyForAllInstruments('PTTL', sizeof(MODINSTRUMENT().wPitchToTempoLock),  f, nInstruments);
	WriteInstrumentPropertyForAllInstruments('PVEH', sizeof(MODINSTRUMENT().nPluginVelocityHandling),  f, nInstruments);
	WriteInstrumentPropertyForAllInstruments('PVOH', sizeof(MODINSTRUMENT().nPluginVolumeHandling),  f, nInstruments);

	if(GetType() & MOD_TYPE_MPT)
	{
		UINT maxNodes = 0;
		for(INSTRUMENTINDEX nIns = 1; nIns <= m_nInstruments; nIns++) if(Instruments[nIns] != nullptr)
		{
			maxNodes = max(maxNodes, Instruments[nIns]->VolEnv.nNodes);
			maxNodes = max(maxNodes, Instruments[nIns]->PanEnv.nNodes);
			maxNodes = max(maxNodes, Instruments[nIns]->PitchEnv.nNodes);
		}
		// write full envelope information for MPTM files (more env points)
		if(maxNodes > 25)
		{
			WriteInstrumentPropertyForAllInstruments('VE..', sizeof(MODINSTRUMENT().VolEnv.nNodes), f, nInstruments);
			WriteInstrumentPropertyForAllInstruments('VP[.', sizeof(MODINSTRUMENT().VolEnv.Ticks ), f, nInstruments);
			WriteInstrumentPropertyForAllInstruments('VE[.', sizeof(MODINSTRUMENT().VolEnv.Values), f, nInstruments);

			WriteInstrumentPropertyForAllInstruments('PE..', sizeof(MODINSTRUMENT().PanEnv.nNodes), f, nInstruments);
			WriteInstrumentPropertyForAllInstruments('PP[.', sizeof(MODINSTRUMENT().PanEnv.Ticks),  f, nInstruments);
			WriteInstrumentPropertyForAllInstruments('PE[.', sizeof(MODINSTRUMENT().PanEnv.Values), f, nInstruments);

			WriteInstrumentPropertyForAllInstruments('PiE.', sizeof(MODINSTRUMENT().PitchEnv.nNodes), f, nInstruments);
			WriteInstrumentPropertyForAllInstruments('PiP[', sizeof(MODINSTRUMENT().PitchEnv.Ticks),  f, nInstruments);
			WriteInstrumentPropertyForAllInstruments('PiE[', sizeof(MODINSTRUMENT().PitchEnv.Values), f, nInstruments);
		}
	}

	return;
}

void CSoundFile::WriteInstrumentPropertyForAllInstruments(__int32 code, __int16 size, FILE* f, UINT nInstruments) const
//---------------------------------------------------------------------------------------------------------------------
{
	fwrite(&code, 1, sizeof(__int32), f);		//write code
	fwrite(&size, 1, sizeof(__int16), f);		//write size
	for(UINT nins=1; nins<=nInstruments; nins++)	//for all instruments...
	{
		BYTE* pField;
		if (Instruments[nins])
		{
			pField = GetInstrumentHeaderFieldPointer(Instruments[nins], code, size); //get ptr to field
		} else
		{
			MODINSTRUMENT *emptyInstrument = new MODINSTRUMENT();
			pField = GetInstrumentHeaderFieldPointer(emptyInstrument, code, size); //get ptr to field
			delete emptyInstrument;
		}
		fwrite(pField, 1, size, f);				//write field data
	}
}

void CSoundFile::SaveExtendedSongProperties(FILE* f) const
//--------------------------------------------------------
{
	//Extra song data - Yet Another Hack.
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
	size = sizeof(m_nDefaultRowsPerBeat);
	fwrite(&size, 1, sizeof(__int16), f);
	fwrite(&m_nDefaultRowsPerBeat, 1, size, f);

	code = 'RPM.';							//write m_nRowsPerMeasure
	fwrite(&code, 1, sizeof(__int32), f);
	size = sizeof(m_nDefaultRowsPerMeasure);
	fwrite(&size, 1, sizeof(__int16), f);
	fwrite(&m_nDefaultRowsPerMeasure, 1, size, f);

	code = 'C...';							//write m_nChannels
	fwrite(&code, 1, sizeof(__int32), f);
	size = sizeof(m_nChannels);
	fwrite(&size, 1, sizeof(__int16), f);
	fwrite(&m_nChannels, 1, size, f);

	if(TypeIsIT_MPT() && GetNumChannels() > 64)	//IT header has room only for 64 channels. Save the
	{											//settings that do not fit to the header here as an extension.
		code = 'ChnS';
		fwrite(&code, 1, sizeof(__int32), f);
		size = (GetNumChannels() - 64) * 2;
		fwrite(&size, 1, sizeof(__int16), f);
		for(UINT ich = 64; ich < m_nChannels; ich++)
		{
			BYTE panvol[2];
			panvol[0] = ChnSettings[ich].nPan >> 2;
			if (ChnSettings[ich].dwFlags & CHN_SURROUND) panvol[0] = 100;
			if (ChnSettings[ich].dwFlags & CHN_MUTE) panvol[0] |= 0x80;
			panvol[1] = ChnSettings[ich].nVolume;
			fwrite(&panvol, sizeof(panvol), 1, f);
		}
	}

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

	code = 'LSWV';							//write m_dwLastSavedWithVersion
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
		{
#ifdef MODPLUG_TRACKER
			if(GetpModDoc())
				GetpModDoc()->AddToLog("Datafield overflow with MIDI to plugparam mappings; data won't be written.\n");
#endif // MODPLUG_TRACKER
		}
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

LPCBYTE CSoundFile::LoadExtendedInstrumentProperties(const LPCBYTE pStart,
													 const LPCBYTE pEnd,
													 bool* pInterpretMptMade)
//---------------------------------------------------------------------------
{
	if( pStart == NULL || pEnd <= pStart || uintptr_t(pEnd - pStart) < 4)
		return NULL;

	int32 code = 0;
	int16 size = 0;
	LPCBYTE ptr = pStart;

	memcpy(&code, ptr, sizeof(code));

	if(code != 'MPTX')
		return NULL;

	// Found MPTX, interpret the file MPT made.
	if(pInterpretMptMade != NULL)
		*pInterpretMptMade = true;

	ptr += sizeof(int32);							// jump extension header code
	while( ptr < pEnd && uintptr_t(pEnd-ptr) >= 4) //Loop 'till beginning of end of file/mpt specific looking for inst. extensions
	{
		memcpy(&code, ptr, sizeof(code));	// read field code
		if (code == 'MPTS')					//Reached song extensions, break out of this loop
			return ptr;

		ptr += sizeof(code);				// jump field code

		if((uintptr_t)(pEnd - ptr) < 2)
			return NULL;

		memcpy(&size, ptr, sizeof(size));	// read field size
		ptr += sizeof(size);				// jump field size

		if(IsValidSizeField(ptr, pEnd, size) == false)
			return NULL;

		for(UINT nins=1; nins<=m_nInstruments; nins++)
		{
			if(Instruments[nins])
				ReadInstrumentExtensionField(Instruments[nins], ptr, code, size);
		}
	}

	return NULL;
}


void CSoundFile::LoadExtendedSongProperties(const MODTYPE modtype,
											LPCBYTE ptr,
											const LPCBYTE lpStream,
											const size_t searchlimit,
											bool* pInterpretMptMade)
//-------------------------------------------------------------------
{
	if(searchlimit < 6 || ptr == NULL || ptr < lpStream || uintptr_t(ptr - lpStream) > searchlimit - 4)
		return;

	const LPCBYTE pEnd = lpStream + searchlimit;

	int32 code = 0;
	int16 size = 0;

	memcpy(&code, ptr, sizeof(code));

	if(code != 'MPTS')
		return;

	// Found MPTS, interpret the file MPT made.
	if(pInterpretMptMade != NULL)
		*pInterpretMptMade = true;

	// HACK: Reset mod flags to default values here, as they are not always written.
	m_ModFlags = 0;

	// Case macros.
	#define CASE(id, data)	\
		case id: fadr = reinterpret_cast<BYTE*>(&data); nMaxReadCount = min(size, sizeof(data)); break;
	#define CASE_NOTXM(id, data) \
		case id: if(modtype != MOD_TYPE_XM) {fadr = reinterpret_cast<BYTE*>(&data); nMaxReadCount = min(size, sizeof(data));} break;

	ptr += sizeof(code); // jump extension header code
	while( uintptr_t(ptr - lpStream) <= searchlimit-6 ) //Loop until given limit.
	{
		code = (*((int32 *)ptr));			// read field code
		ptr += sizeof(int32);				// jump field code
		size = (*((int16 *)ptr));			// read field size
		ptr += sizeof(int16);				// jump field size

		if(IsValidSizeField(ptr, pEnd, size) == false)
			break;

		size_t nMaxReadCount = 0;
		BYTE * fadr = NULL;

		switch (code)					// interpret field code
		{
			CASE('DT..', m_nDefaultTempo);
			CASE('RPB.', m_nDefaultRowsPerBeat);
			CASE('RPM.', m_nDefaultRowsPerMeasure);
			CASE_NOTXM('C...', m_nChannels);
			CASE('TM..', m_nTempoMode);
			CASE('PMM.', m_nMixLevels);
			CASE('CWV.', m_dwCreatedWithVersion);
			CASE('LSWV', m_dwLastSavedWithVersion);
			CASE('SPA.', m_nSamplePreAmp);
			CASE('VSTV', m_nVSTiVolume);
			CASE('DGV.', m_nDefaultGlobalVolume);
			CASE_NOTXM('RP..', m_nRestartPos);
			CASE('MSF.', m_ModFlags);
			case 'MIMA': GetMIDIMapper().Deserialize(ptr, size); break;
			case 'ChnS':
				if( (size <= 63*2) && (size % 2 == 0) )
				{
					const BYTE* pData = ptr;
					STATIC_ASSERT(CountOf(ChnSettings) >= 64);
					const __int16 nLoopLimit = min(size/2, CountOf(ChnSettings) - 64);
					for(__int16 i = 0; i<nLoopLimit; i++, pData += 2) if(pData[0] != 0xFF)
					{
						ChnSettings[i+64].nVolume = pData[1];
						ChnSettings[i+64].nPan = 128;
						if (pData[0] & 0x80) ChnSettings[i+64].dwFlags |= CHN_MUTE;
						const UINT n = pData[0] & 0x7F;
						if (n <= 64) ChnSettings[i+64].nPan = n << 2;
						if (n == 100) ChnSettings[i+64].dwFlags |= CHN_SURROUND;
					}
				}

			break;
		}

		if (fadr != NULL)					// if field code recognized
			memcpy(fadr,ptr,nMaxReadCount);	// read field data

		ptr += size;						// jump field data
	}

	// Validate read values.
	Limit(m_nDefaultTempo, GetModSpecifications().tempoMin, GetModSpecifications().tempoMax);
	//m_nRowsPerBeat
	//m_nRowsPerMeasure
	LimitMax(m_nChannels, GetModSpecifications().channelsMax);
    //m_nTempoMode
	//m_nMixLevels
	//m_dwCreatedWithVersion
	//m_dwLastSavedWithVersion
	//m_nSamplePreAmp
	//m_nVSTiVolume
	//m_nDefaultGlobalVolume);
	//m_nRestartPos
	//m_ModFlags


	#undef CASE
	#undef CASE_NOTXM
}
