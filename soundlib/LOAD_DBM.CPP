/*
 * Load_dbm.cpp
 * ------------
 * Purpose: DigiBooster Pro module Loader (DBM)
 * Notes  : This loader doesn't handle multiple songs.
 * Authors: Olivier Lapicque
 *          Adam Goode (endian and char fixes for PPC)
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"

#define DBM_FILE_MAGIC	0x304d4244
#define DBM_ID_NAME		0x454d414e
#define DBM_NAMELEN		0x2c000000
#define DBM_ID_INFO		0x4f464e49
#define DBM_INFOLEN		0x0a000000
#define DBM_ID_SONG		0x474e4f53
#define DBM_ID_INST		0x54534e49
#define DBM_ID_VENV		0x564e4556
#define DBM_ID_PATT		0x54544150
#define DBM_ID_SMPL		0x4c504d53

#pragma pack(push, 1)

struct DBMFileHeader
{
	uint32 dbm_id;		// "DBM0" = 0x304d4244
	uint16 trkver;		// Tracker version: 02.15
	uint16 reserved;
	uint32 name_id;		// "NAME" = 0x454d414e
	uint32 name_len;		// name length: always 44
	char   songname[44];
	uint32 info_id;		// "INFO" = 0x4f464e49
	uint32 info_len;		// 0x0a000000
	uint16 instruments;
	uint16 samples;
	uint16 songs;
	uint16 patterns;
	uint16 channels;
	uint32 song_id;		// "SONG" = 0x474e4f53
	uint32 song_len;
	char   songname2[44];
	uint16 orders;
//	uint16 orderlist[0];	// orderlist[orders] in words
};

struct DBMInstrument
{
	char   name[30];
	uint16 sampleno;
	uint16 volume;
	uint32 finetune;
	uint32 loopstart;
	uint32 looplen;
	uint16 panning;
	uint16 flags;
};

struct DBMEnvelope
{
	uint16 instrument;
	uint8  flags;
	uint8  numpoints;
	uint8  sustain1;
	uint8  loopbegin;
	uint8  loopend;
	uint8  sustain2;
	uint16 volenv[2 * 32];
};

struct DBMPattern
{
	uint16 rows;
	uint32 packedsize;
	uint8  patterndata[2];	// [packedsize]
};

struct DBMSample
{
	uint32 flags;
	uint32 samplesize;
	uint8  sampledata[2];		// [samplesize]
};

#pragma pack(pop)


static const ModCommand::COMMAND dbmEffects[23] =
{
	CMD_ARPEGGIO, CMD_PORTAMENTOUP, CMD_PORTAMENTODOWN, CMD_TONEPORTAMENTO,
	CMD_VIBRATO, CMD_TONEPORTAVOL, CMD_VIBRATOVOL, CMD_TREMOLO,
	CMD_PANNING8, CMD_OFFSET, CMD_VOLUMESLIDE, CMD_POSITIONJUMP,
	CMD_VOLUME, CMD_PATTERNBREAK, CMD_MODCMDEX, CMD_TEMPO,
	CMD_GLOBALVOLUME, CMD_GLOBALVOLSLIDE, CMD_KEYOFF, CMD_SETENVPOSITION,
	CMD_CHANNELVOLUME, CMD_CHANNELVOLSLIDE, CMD_PANNINGSLIDE,
};


void ConvertDBMEffect(uint8 &command, uint8 &param)
//-------------------------------------------------
{
	if(command < CountOf(dbmEffects))
		command = dbmEffects[command];
	else
		command = CMD_NONE;

	switch (command)
	{
	case CMD_ARPEGGIO:
		if(param == 0)
			command = CMD_NONE;
		break;

	case CMD_VOLUMESLIDE:
		if(param & 0xF0)
			param &= 0xF0;
		break;

	case CMD_GLOBALVOLUME:
		if(param <= 64)
			param *= 2;
		else
			param = 128;
		break;

	case CMD_MODCMDEX:
		switch(param & 0xF0)
		{
		case 0x00:	// set filter
			command = CMD_NONE;
			break;
		case 0x30:	// play backwards
			command = CMD_S3MCMDEX;
			param = 0x9F;
			break;
		case 0x40:	// turn off sound in channel
			// TODO
			break;
		case 0x50:	// turn on/off channel
			// TODO is this correct?
			if((param & 0x0F) <= 0x01)
			{
				command = CMD_CHANNELVOLUME;
				param = (param == 0x50) ? 0x00 : 0x40;
			}
			break;
		case 0x60:	// set loop begin / loop
			// TODO
			break;
		case 0x70:	// set offset
			// TODO
			break;
		case 0xF0:	// turn on/off channel
			// TODO
			break;
		default:
			// Rest will be converted later from CMD_MODCMDEX to CMD_S3MCMDEX.
			break;
		}
		break;

	case CMD_TEMPO:
		if(param <= 0x1F) command = CMD_SPEED;
		break;

	case CMD_KEYOFF:
		if (param == 0)
		{
			// TODO key of at tick 0
		}
		break;

	case CMD_OFFSET:
		// TODO Sample offset slide
		command = CMD_NONE;
		break;
	}
}


bool CSoundFile::ReadDBM(const BYTE *lpStream, const DWORD dwMemLength)
//---------------------------------------------------------------------
{
	const DBMFileHeader *pfh = (DBMFileHeader *)lpStream;
	DWORD dwMemPos;
	uint16 nOrders, nSamples, nInstruments, nPatterns;
	
	if ((!lpStream) || (dwMemLength <= sizeof(DBMFileHeader)) || (!pfh->channels)
	 || (pfh->dbm_id != DBM_FILE_MAGIC) || (!pfh->songs) || (pfh->song_id != DBM_ID_SONG)
	 || (pfh->name_id != DBM_ID_NAME) || (pfh->name_len != DBM_NAMELEN)
	 || (pfh->info_id != DBM_ID_INFO) || (pfh->info_len != DBM_INFOLEN)) return false;
	dwMemPos = sizeof(DBMFileHeader);
	nOrders = BigEndianW(pfh->orders);
	if (dwMemPos + 2 * nOrders + 8*3 >= dwMemLength) return false;
	nInstruments = BigEndianW(pfh->instruments);
	nSamples = BigEndianW(pfh->samples);
	nPatterns = BigEndianW(pfh->patterns);
	m_nType = MOD_TYPE_DBM;
	m_nChannels = CLAMP(BigEndianW(pfh->channels), 1, MAX_BASECHANNELS);	// note: MAX_BASECHANNELS is currently 127, but DBM supports up to 128 channels.

	if(pfh->songname[0])
	{
		StringFixer::ReadString<StringFixer::maybeNullTerminated>(m_szNames[0], pfh->songname);
	} else
	{
		StringFixer::ReadString<StringFixer::maybeNullTerminated>(m_szNames[0], pfh->songname2);

	}

	Order.resize(nOrders, Order.GetInvalidPatIndex());
	for (UINT iOrd=0; iOrd < nOrders; iOrd++)
	{
		if (iOrd >= MAX_ORDERS) break;
		Order[iOrd] = (PATTERNINDEX)BigEndianW(*((WORD *)(lpStream + dwMemPos + iOrd * 2)));
	}
	dwMemPos += 2 * nOrders;
	while (dwMemPos + 10 < dwMemLength)
	{
		uint32 chunk_id = LittleEndian(((uint32 *)(lpStream + dwMemPos))[0]);
		uint32 chunk_size = BigEndian(((uint32 *)(lpStream + dwMemPos))[1]);
		uint32 chunk_pos;
		
		dwMemPos += 8;
		chunk_pos = dwMemPos;
		if ((dwMemPos + chunk_size > dwMemLength) || (chunk_size > dwMemLength)) break;
		dwMemPos += chunk_size;
		// Instruments
		if (chunk_id == DBM_ID_INST)
		{
			if (nInstruments >= MAX_INSTRUMENTS) nInstruments = MAX_INSTRUMENTS-1;
			for(INSTRUMENTINDEX iIns = 0; iIns < nInstruments; iIns++)
			{
				ModSample *psmp;
				ModInstrument *pIns;
				DBMInstrument *pih;
				uint16 nsmp;

				if (chunk_pos + sizeof(DBMInstrument) > dwMemPos) break;

				pih = (DBMInstrument *)(lpStream + chunk_pos);
				nsmp = BigEndianW(pih->sampleno);
				psmp = ((nsmp) && (nsmp < MAX_SAMPLES)) ? &Samples[nsmp] : nullptr;

				try
				{
					pIns = new ModInstrument(nsmp);
				} catch(MPTMemoryException)
				{
					break;
				}

				Instruments[iIns + 1] = pIns;

				StringFixer::ReadString<StringFixer::maybeNullTerminated>(pIns->name, pih->name);
				if (psmp)
				{
					StringFixer::ReadString<StringFixer::maybeNullTerminated>(m_szNames[nsmp], pih->name);
				}

				pIns->nFadeOut = 1024;	// ???
				pIns->nPan = BigEndianW(pih->panning);
				if ((pIns->nPan) && (pIns->nPan < 256))
					pIns->dwFlags = INS_SETPANNING;
				else
					pIns->nPan = 128;

				// Sample Info
				if(psmp)
				{
					uint16 sflags = BigEndianW(pih->flags);
					psmp->nVolume = BigEndianW(pih->volume) * 4;
					if(/*!psmp->nVolume ||*/ psmp->nVolume > 256) psmp->nVolume = 256;	// XXX First condition looks like a typical "modplug-ism"
					psmp->nGlobalVol = 64;
					psmp->nC5Speed = BigEndian(pih->finetune);

					if(pih->looplen && (sflags & 3))
					{
						psmp->nLoopStart = BigEndian(pih->loopstart);
						psmp->nLoopEnd = psmp->nLoopStart + BigEndian(pih->looplen);
						psmp->uFlags |= CHN_LOOP;
						psmp->uFlags &= ~CHN_PINGPONGLOOP;
						if(sflags & 2) psmp->uFlags |= CHN_PINGPONGLOOP;
					}
				}
				chunk_pos += sizeof(DBMInstrument);
				m_nInstruments = iIns + 1;
			}
		} else
		// Volume Envelopes
		if (chunk_id == DBM_ID_VENV)
		{
			UINT nEnvelopes = lpStream[chunk_pos+1];
			
			chunk_pos += 2;
			for (UINT iEnv=0; iEnv<nEnvelopes; iEnv++)
			{
				DBMEnvelope *peh;
				UINT nins;
				
				if (chunk_pos + sizeof(DBMEnvelope) > dwMemPos) break;
				peh = (DBMEnvelope *)(lpStream+chunk_pos);
				nins = BigEndianW(peh->instrument);
				if ((nins) && (nins < MAX_INSTRUMENTS) && (Instruments[nins]) && (peh->numpoints))
				{
					ModInstrument *pIns = Instruments[nins];

					if (peh->flags & 1) pIns->VolEnv.dwFlags |= ENV_ENABLED;
					if (peh->flags & 2) pIns->VolEnv.dwFlags |= ENV_SUSTAIN;
					if (peh->flags & 4) pIns->VolEnv.dwFlags |= ENV_LOOP;
					pIns->VolEnv.nNodes = peh->numpoints + 1;
					if (pIns->VolEnv.nNodes > MAX_ENVPOINTS) pIns->VolEnv.nNodes = MAX_ENVPOINTS;
					pIns->VolEnv.nLoopStart = peh->loopbegin;
					pIns->VolEnv.nLoopEnd = peh->loopend;
					pIns->VolEnv.nSustainStart = pIns->VolEnv.nSustainEnd = peh->sustain1;
					for (UINT i=0; i<pIns->VolEnv.nNodes; i++)
					{
						pIns->VolEnv.Ticks[i] = BigEndianW(peh->volenv[i*2]);
						pIns->VolEnv.Values[i] = (BYTE)BigEndianW(peh->volenv[i*2+1]);
					}
				}
				chunk_pos += sizeof(DBMEnvelope);
			}
		} else
		// Packed Pattern Data
		if (chunk_id == DBM_ID_PATT)
		{
			if (nPatterns > MAX_PATTERNS) nPatterns = MAX_PATTERNS;
			for(PATTERNINDEX iPat = 0; iPat < nPatterns; iPat++)
			{
				DBMPattern *pph;
				DWORD pksize;
				UINT nRows;

				if (chunk_pos + sizeof(DBMPattern) > dwMemPos) break;
				pph = (DBMPattern *)(lpStream+chunk_pos);
				pksize = BigEndian(pph->packedsize);
				if ((chunk_pos + pksize + 6 > dwMemPos) || (pksize > dwMemPos)) break;
				nRows = BigEndianW(pph->rows);
				if ((nRows >= 4) && (nRows <= 256))
				{
					Patterns.Insert(iPat, nRows);
					ModCommand *m = Patterns[iPat];
					if (m)
					{
						LPBYTE pkdata = (LPBYTE)&pph->patterndata;
						UINT row = 0;
						UINT i = 0;

						while ((i+3<pksize) && (row < nRows))
						{
							UINT ch = pkdata[i++];

							if (ch)
							{
								BYTE b = pkdata[i++];
								ch--;
								if (ch < m_nChannels)
								{
									if (b & 0x01)
									{
										uint8 note = pkdata[i++];

										if (note == 0x1F) note = NOTE_KEYOFF; else
										if ((note) && (note < 0xFE))
										{
											note = ((note >> 4) * 12) + (note & 0x0F) + 13;
										}
										m[ch].note = note;
									}
									if (b & 0x02) m[ch].instr = pkdata[i++];
									if (b & 0x3C)
									{
										uint8 cmd1 = CMD_NONE, cmd2 = CMD_NONE;
										uint8 param1 = 0, param2 = 0;
										if (b & 0x04) cmd2 = pkdata[i++];
										if (b & 0x08) param2 = pkdata[i++];
										if (b & 0x10) cmd1 = pkdata[i++];
										if (b & 0x20) param1 = pkdata[i++];
										ConvertDBMEffect(cmd1, param1);
										ConvertDBMEffect(cmd2, param2);

										// this is the same conversion algorithm as in the ULT loader. maybe this should be merged at some point...
										if (cmd2 == CMD_VOLUME || (cmd2 == CMD_NONE && cmd1 != CMD_VOLUME))
										{
											std::swap(cmd1, cmd2);
											std::swap(param1, param2);
										}

										int n;
										for (n = 0; n < 4; n++)
										{
											if(ModCommand::ConvertVolEffect(cmd1, param1, (n >> 1) != 0))
											{
												n = 5;
												break;
											}
											std::swap(cmd1, cmd2);
											std::swap(param1, param2);
										}
										if (n < 5)
										{
											if (ModCommand::GetEffectWeight((ModCommand::COMMAND)cmd1) > ModCommand::GetEffectWeight((ModCommand::COMMAND)cmd2))
											{
												std::swap(cmd1, cmd2);
												std::swap(param1, param2);
											}
											cmd1 = CMD_NONE;
										}
										if (!cmd1)
											param1 = 0;
										if (!cmd2)
											param2 = 0;

										m[ch].volcmd = cmd1;
										m[ch].vol = param1;
										m[ch].command = cmd2;
										m[ch].param = param2;
										m[ch].ExtendedMODtoS3MEffect();
									}
								} else
								{
									if (b & 0x01) i++;
									if (b & 0x02) i++;
									if (b & 0x04) i++;
									if (b & 0x08) i++;
									if (b & 0x10) i++;
									if (b & 0x20) i++;
								}
							} else
							{
								row++;
								m += m_nChannels;
							}
						}
					}
				}
				chunk_pos += 6 + pksize;
			}
		} else
		// Reading Sample Data
		if (chunk_id == DBM_ID_SMPL)
		{
			if (nSamples >= MAX_SAMPLES) nSamples = MAX_SAMPLES-1;
			m_nSamples = nSamples;
			for (UINT iSmp=1; iSmp<=nSamples; iSmp++)
			{
				ModSample *pSmp;
				DBMSample *psh;
				DWORD samplesize;
				DWORD sampleflags;

				if (chunk_pos + sizeof(DBMSample) >= dwMemPos) break;
				psh = (DBMSample *)(lpStream+chunk_pos);
				chunk_pos += 8;
				samplesize = BigEndian(psh->samplesize);
				sampleflags = BigEndian(psh->flags);
				pSmp = &Samples[iSmp];
				pSmp->nLength = samplesize;
				if (sampleflags & 2)
				{
					pSmp->uFlags |= CHN_16BIT;
					samplesize <<= 1;
				}
				if ((chunk_pos+samplesize > dwMemPos) || (samplesize > dwMemLength)) break;
				if (sampleflags & 3)
				{
					ReadSample(pSmp, (pSmp->uFlags & CHN_16BIT) ? RS_PCM16M : RS_PCM8S,
								(LPSTR)(psh->sampledata), samplesize);
				}
				chunk_pos += samplesize;
			}
		}
	}
	return true;
}

