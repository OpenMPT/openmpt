#include "stdafx.h"
#include "sndfile.h"
#include "it_defs.h"
#include <map>
#include <string>
#include <vector>
#include <algorithm>
#include "MPT_module.h"
#include "../mptrack/serialization_utils.h"
#include "tuningcollection.h"
#include "../mptrack/moddoc.h"
#include <fstream>
#include <strstream>
#include <list>

using std::map;
using std::list;
using std::vector;
using std::string;
using std::ofstream;
using std::ifstream;
using std::istrstream;
using std::runtime_error;
using namespace srlztn; //SeRiaLiZaTioN

static inline UINT ConvertVolParam(UINT value)
//--------------------------------------------
{
	return (value > 9)  ? 9 : value;
}


static bool AreNonDefaultTuningsUsed(CSoundFile& sf)
//--------------------------------------------------
{
	for(INSTRUMENTINDEX i = 1; i<=sf.GetNumInstruments(); i++)
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
			for(UINT i = 1; i<=m_rSoundfile.GetNumInstruments(); i++)
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
			for(UINT i = 1; i<=m_rSoundfile.GetNumInstruments(); i++)
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
					csf.m_pModDoc->SetModified(); //The tuning is changed so
					//the modified flag is set.
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
	CSerializationInstructions si("mptm", 1, flag, 2);
	//Adding entries always for reading and for writing only when there is something to write.
	if(flag & INFLAG || sf.GetTuneSpecificTunings().GetNumTunings() > 0) 
		si.AddEntry(CSerializationentry("0", new CTuningCollectionStreamer(sf.GetTuneSpecificTunings())));
	if(flag & INFLAG || AreNonDefaultTuningsUsed(sf))
		si.AddEntry(CSerializationentry("1", new CInstrumentTuningMapStreamer(sf)));
	return si;
}



//Originally copied from SaveIT
BOOL CSoundFile::SaveMPT(LPCSTR lpszFileName, UINT)
//-------------------------------------------------------------
{
	DWORD dwPatNamLen, dwChnNamLen;
	ITFILEHEADER header;
	ITINSTRUMENT iti;
	ITSAMPLESTRUCT itss;
	BYTE smpcount[(MAX_SAMPLES+7)/8];
	DWORD inspos[MAX_INSTRUMENTS];
	vector<DWORD> patpos; patpos.resize(Patterns.Size(), 0);
	ASSERT(patpos.size() == Patterns.Size());
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
	BYTE buf[512];
	FILE *f;


	if ((!lpszFileName) || ((f = fopen(lpszFileName, "wb")) == NULL)) return FALSE;


	memset(inspos, 0, sizeof(inspos));
	memset(smppos, 0, sizeof(smppos));
	// Writing Header
	memset(&header, 0, sizeof(header));
	dwPatNamLen = 0;
	dwChnNamLen = 0;
	header.id = CMPTModule::m_ModID;
	lstrcpyn(header.songname, m_szNames[0], 26);

	header.reserved1 = 0x1004;
	header.ordnum = Order.size();

	header.insnum = m_nInstruments;
	header.smpnum = m_nSamples;
	header.patnum = Patterns.Size();
	while ((header.patnum > 0) && (!Patterns[header.patnum-1])) header.patnum--;

	//VERSION
	header.cwtv = 0x88D;	// Used in OMPT-hack versioning.
	header.cmwt = 0x888;	// Might come up as "Impulse Tracker 8" file in XMPlay. :)

	/*
	Version history:
	0x88C(v.02.48) -> 0x88D: Some tuning related changes - that part fails to read on older versions.
	0x88B -> 0x88C: Changed type in which tuning number is printed
					to file: size_t -> uint16.
	0x88A -> 0x88B: Changed order-to-pattern-index table type from BYTE-array to vector<UINT>.
	*/


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
	dwHdrPos = sizeof(header) + Order.GetSerializationByteNum();
	// Channel Pan and Volume
	memset(header.chnpan, 0xFF, 64);
	memset(header.chnvol, 64, 64);
	for (UINT ich=0; ich<m_nChannels; ich++)
	{
		header.chnpan[ich] = ChnSettings[ich].nPan >> 2;
		if (ChnSettings[ich].dwFlags & CHN_SURROUND) header.chnpan[ich] = 100;
		header.chnvol[ich] = ChnSettings[ich].nVolume;
		if (ChnSettings[ich].dwFlags & CHN_MUTE) header.chnpan[ich] |= 0x80;
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
	Order.Serialize(f);
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
		BYTE keyboardex[120];

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
			for (UINT i=0; i<120; i++) if (penv->Keyboard[i] < MAX_SAMPLES)
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
			for (UINT i=0; i<120; i++) iti.keyboard[i*2] = i;
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
			dwPos += 120;
			fwrite(keyboardex, 1, 120, f);
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
			len = 0;
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
	SaveExtendedInstrumentProperties(Headers, header.insnum, f);
	SaveExtendedSongProperties(f);

	// Updating offsets
	fseek(f, dwHdrPos, SEEK_SET);
	if (header.insnum) fwrite(inspos, 4, header.insnum, f);
	if (header.smpnum) fwrite(smppos, 4, header.smpnum, f);
	if (header.patnum) fwrite(&patpos[0], 4, header.patnum, f);
	
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


	//Last bytes tell where the hack mpt things begin.
	//It _must_ be there.
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


BOOL CSoundFile::ReadMPT(const BYTE *lpStream, const DWORD dwMemLength)
//--------------------------------------------------------------
{
	ITFILEHEADER *pifh = (ITFILEHEADER *)lpStream;

	DWORD dwMemPos = sizeof(ITFILEHEADER);
	DWORD inspos[MAX_INSTRUMENTS];
	DWORD smppos[MAX_SAMPLES];
	vector<DWORD> patpos;
	BYTE chnmask[MAX_BASECHANNELS], channels_used[MAX_BASECHANNELS];
	MODCOMMAND lastvalue[MAX_BASECHANNELS];
	if ((!lpStream) || (dwMemLength < 0x100)) return FALSE;
	if ((pifh->id != 0x2e6D7074) || (pifh->insnum > 0xFF)
	 || (!pifh->smpnum) || (pifh->smpnum >= MAX_SAMPLES) || (!pifh->ordnum)) return FALSE;
	if (dwMemPos + pifh->ordnum + pifh->insnum*4
	 + pifh->smpnum*4 + pifh->patnum*4 > dwMemLength) return FALSE;

    DWORD temp = 0;
	memcpy(&temp, lpStream + (dwMemLength - sizeof(DWORD)), sizeof(DWORD));
	const DWORD mptStartPos = temp;
	if(mptStartPos >= dwMemLength)
	{
		::MessageBox(NULL, "Corrupted savefile - loading unsuccesful", 0, MB_OK);
		return FALSE;
	}

	ChangeModTypeTo(MOD_TYPE_MPT);
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
	for (int ipan=0; ipan<MAX_BASECHANNELS; ipan++) if (pifh->chnpan[ipan] != 0xFF)
	{
		ChnSettings[ipan].nVolume = pifh->chnvol[ipan];
		ChnSettings[ipan].nPan = 128;
		if (pifh->chnpan[ipan] & 0x80) ChnSettings[ipan].dwFlags |= CHN_MUTE;
		UINT n = pifh->chnpan[ipan] & 0x7F;
		if (n <= 64) ChnSettings[ipan].nPan = n << 2;
		if (n == 100) ChnSettings[ipan].dwFlags |= CHN_SURROUND;
	}
	if (m_nChannels < 4) m_nChannels = 4;
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
	if (nordsize > Order.GetOrderNumberLimitMax()) nordsize = Order.GetOrderNumberLimitMax();
	if(pifh->cwtv > 0x88A)
		dwMemPos += Order.UnSerialize(lpStream+dwMemPos, dwMemLength-dwMemPos);
	else
	{
		Order.ReadAsByte(lpStream+dwMemPos, nordsize, dwMemLength - dwMemPos);
		dwMemPos += pifh->ordnum;
		//Replacing 0xFF and 0xFE with new corresponding indexes
		replace(Order.begin(), Order.end(), static_cast<UINT>(0xFE), Patterns.GetIgnoreIndex());
		replace(Order.begin(), Order.end(), static_cast<UINT>(0xFF), Patterns.GetInvalidIndex());
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
	if(patpossize > Patterns.GetPatternNumberLimitMax())
	{
		ASSERT(false);
		patpossize = Patterns.GetPatternNumberLimitMax();
	}
	patpos.resize(patpossize);
	patpossize <<= 2; // <-> patpossize *= sizeof(DWORD);
	memcpy(&patpos[0], lpStream+dwMemPos, patpossize);
	dwMemPos += pifh->patnum * 4;
	// Reading IT Extra Info
	if (dwMemPos + 2 < dwMemLength)
	{
		UINT nflt = *((WORD *)(lpStream + dwMemPos));
		dwMemPos += 2;
		if (dwMemPos + nflt * 8 < dwMemLength) dwMemPos += nflt * 8;
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
		if ((dwMemPos + len <= dwMemLength) && (len <= pifh->patnum*MAX_PATTERNNAME) && (len >= MAX_PATTERNNAME))
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
	// 4-channels minimum
	m_nChannels = 4;
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
	// Checking for unused channels
	UINT npatterns = pifh->patnum;
	
	if (npatterns > Patterns.GetPatternNumberLimitMax()) 
		npatterns = Patterns.GetPatternNumberLimitMax();

	for (UINT patchk=0; patchk<npatterns; patchk++)
	{
		memset(chnmask, 0, sizeof(chnmask));

		if ((!patpos[patchk]) || ((DWORD)patpos[patchk] + 4 >= dwMemLength)) 
			continue;

		UINT len = *((WORD *)(lpStream+patpos[patchk]));
		UINT rows = *((WORD *)(lpStream+patpos[patchk]+2));

		if ((rows < 4) || (rows > MAX_PATTERN_ROWS)) 
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
				ch = (ch - 1) & IT_bitmask_patternChanMask_c;   // 0x3f mask of the byte again, keeping only 64 bits
			if (b & IT_bitmask_patternChanEnabled_c)            // 0x80 check if the upper bit is enabled.
			{
				if (i >= len)               
				    break;        
				chnmask[ch] = p[i++];       // set the channel mask for this channel.
			}
			// Channel used
			if (chnmask[ch] & 0x0F)         // if this channel is used set m_nChannels
			{
				if ((ch >= m_nChannels) && (ch < MAX_BASECHANNELS)) m_nChannels = ch+1;
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
			penv->pTuning = penv->s_DefaultTuning;
			ITInstrToMPT(lpStream + inspos[nins], penv, pifh->cmwt);
		}
	}

// -> DESC="per-instrument volume ramping setup (refered as attack)"
	// In order to properly compute the position, in file, of eventual extended settings
	// such as "attack" we need to keep the "real" size of the last sample as those extra
	// setting will follow this sample in the file
	UINT lastSampleSize = 0;

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
// -> DESC="per-instrument volume ramping setup (refered as attack)"
				lastSampleSize = ReadSample(&Ins[nsmp+1], flags, (LPSTR)(lpStream+pis->samplepointer), dwMemLength - pis->samplepointer);
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
		ptr += sizeof(__int32);							// jump extension header code
		while( (DWORD)(ptr - lpStream) < mptStartPos ){ //Loop 'till beginning of mpt specific looking for inst. extensions
		
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
		LoadExtendedSongProperties(MOD_TYPE_MPT, ptr, lpStream, mptStartPos);


	// Reading Patterns
	for (UINT npat=0; npat<npatterns; npat++)
	{
		if ((!patpos[npat]) || ((DWORD)patpos[npat] + 4 >= dwMemLength))
		{
			Patterns.Insert(npat, 64);
			continue;
		}
		UINT len = *((WORD *)(lpStream+patpos[npat]));
		UINT rows = *((WORD *)(lpStream+patpos[npat]+2));
// -> CODE#0008
// -> DESC="#define to set pattern size"
//		if ((rows < 4) || (rows > 256)) continue;
		if ((rows < 4) || (rows > MAX_PATTERN_ROWS)) continue;
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
				ch = (ch - 1) & IT_bitmask_patternChanMask_c; // 0x3f
		
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

	return TRUE;
}




