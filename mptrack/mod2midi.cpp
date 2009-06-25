#include "stdafx.h"
#include "mptrack.h"
#include "mod2midi.h"

#pragma pack(1)

typedef struct _RMIDDATACHUNK
{
	DWORD id_RIFF;	// "RIFF"
	DWORD filelen;
	DWORD id_RMID;	// "RMID"
	DWORD id_data;	// "data"
	DWORD datalen;
} RMIDDATACHUNK, *PRMIDDATACHUNK;

typedef struct _MTHDCHUNK	// (big endian)
{
	DWORD id;		// "MThd" = 0x6468544D
	DWORD len;		// 6
	WORD wFmt;		// 0=single track, 1=synchro multitrack, 2=asynch multitrack 
	WORD wTrks;		// # of tracks
	WORD wDivision;	// PPQN
} MTHDCHUNK, *PMTHDCHUNK;

typedef struct _MTRKCHUNK	// (big endian)
{
	DWORD id;		// "MTrk" = 0x6B72544D
	DWORD len;
} MTRKCHUNK, *PMTRKCHUNK;

typedef struct _DYNMIDITRACK
{
	UINT nTrackSize;
	PUCHAR pTrackData;
	UINT nAllocatedMem;
	UINT nLastEventClock;
	UINT nMidiChannel;
	UINT nMidiProgram;
	UINT nInstrument;
	BYTE NoteOn[128]; // ch of note + 1
	// Functions
	void Write(const void *, unsigned long nBytes);
	void WriteLen(unsigned long len);
} DYNMIDITRACK, *PDYNMIDITRACK;

#pragma pack()


void DYNMIDITRACK::Write(const void *pBuffer, unsigned long nBytes)
//-----------------------------------------------------------------
{
	if (nTrackSize + nBytes > nAllocatedMem)
	{
		UINT nGrow = 4096;
		if (nBytes >= nGrow) nGrow = nBytes * 2;
		PUCHAR p = new UCHAR[nAllocatedMem+nGrow];
		if (!p) return;
		memcpy(p, pTrackData, nTrackSize);
		delete[] pTrackData;
		pTrackData = p;
		nAllocatedMem += nGrow;
	}
	memcpy(pTrackData+nTrackSize, pBuffer, nBytes);
	nTrackSize += nBytes;
}


void DYNMIDITRACK::WriteLen(unsigned long len)
//--------------------------------------------
{
	BYTE b;
	for (int n=4; n>0; n--)
	{
		if (len >= (UINT)(1<<(n*7)))
		{
			b = (BYTE)(((len >> (n*7)) & 0x7f) | 0x80);
			Write(&b, 1);
		}
	}
	b = (BYTE)(len & 0x7f);
	Write(&b, 1);
}


// Linear =  (nMidiVolume * nMidiVolume << 16) / (127*127);
// -> midi_vol = sqrt(127*127*gain/gain_unit)
static int LinearToDLSMidiVolume(int nVolume)
//-------------------------------------------
{
	const float kLin2Mid = 127.0f*127.0f/65536.0f;
	int result;
	
	_asm {
	fild nVolume
	fld kLin2Mid
	fmulp ST(1), ST(0)
	fsqrt
	fistp result
	}
	return result;
}

////////////////////////////////////////////////////////////////////////////////////
//
// CModToMidi dialog implementation
//

BEGIN_MESSAGE_MAP(CModToMidi, CDialog)
	ON_CBN_SELCHANGE(IDC_COMBO1,	UpdateDialog)
	ON_CBN_SELCHANGE(IDC_COMBO2,	OnChannelChanged)
	ON_CBN_SELCHANGE(IDC_COMBO3,	OnProgramChanged)
	ON_WM_VSCROLL()
END_MESSAGE_MAP()

void CModToMidi::DoDataExchange(CDataExchange *pDX)
//-------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1,	m_CbnInstrument);
	DDX_Control(pDX, IDC_COMBO2,	m_CbnChannel);
	DDX_Control(pDX, IDC_COMBO3,	m_CbnProgram);
	DDX_Control(pDX, IDC_SPIN1,		m_SpinInstrument);
}


CModToMidi::CModToMidi(LPCSTR pszPathName, CSoundFile *pSndFile, CWnd *pWndParent):CDialog(IDD_MOD2MIDI, pWndParent)
//------------------------------------------------------------------------------------------------------------------
{
	CHAR fext[_MAX_EXT];
	
	m_bRmi = FALSE;
	m_pSndFile = pSndFile;
	strcpy(m_szFileName, pszPathName);
	_splitpath(pszPathName, NULL, NULL, NULL, fext);
	if (!_stricmp(fext, ".rmi")) m_bRmi = TRUE;
	memset(m_InstrMap, 0, sizeof(m_InstrMap));
	for (UINT nIns=1; nIns<=m_pSndFile->m_nInstruments; nIns++)
	{
		INSTRUMENTHEADER *penv = m_pSndFile->Headers[nIns];
		if ((penv) && (penv->nMidiChannel <= 16))
		{
			m_InstrMap[nIns].nChannel = penv->nMidiChannel;
			if (m_InstrMap[nIns].nChannel == 10)
			{
				if ((penv->nMidiProgram > 20) && (penv->nMidiProgram < 120))
					m_InstrMap[nIns].nProgram = penv->nMidiProgram;
				else
					m_InstrMap[nIns].nProgram = (penv->NoteMap[60]-1) & 0x7f;
			} else
			{
				m_InstrMap[nIns].nProgram = penv->nMidiProgram & 0x7f;
			}
		}
	}
}


BOOL CModToMidi::OnInitDialog()
//-----------------------------
{
	CHAR s[256];

	CDialog::OnInitDialog();
	// Fill instruments box
	m_SpinInstrument.SetRange(-1, 1);
	m_SpinInstrument.SetPos(0);
	m_nCurrInstr = 1;
	if (m_pSndFile->m_nInstruments)
	{
		for (UINT nIns=1; nIns<=m_pSndFile->m_nInstruments; nIns++)
		{
			INSTRUMENTHEADER *penv = m_pSndFile->Headers[nIns];
			if ((penv) && (m_pSndFile->IsInstrumentUsed(nIns)))
			{
				memset(s, 0, sizeof(s));
				wsprintf(s, "%02d: ", nIns);
				memcpy(s+strlen(s), penv->name, 32);
				m_CbnInstrument.SetItemData(m_CbnInstrument.AddString(s), nIns);
			}
		}
	} else
	{
		for (UINT nIns=1; nIns<=m_pSndFile->m_nSamples; nIns++)
		{
			if ((m_pSndFile->Ins[nIns].pSample)
			 && (m_pSndFile->IsSampleUsed(nIns)))
			{
				memset(s, 0, sizeof(s));
				wsprintf(s, "%02d: ", nIns);
				memcpy(s+strlen(s), m_pSndFile->m_szNames[nIns], 32);
				m_CbnInstrument.SetItemData(m_CbnInstrument.AddString(s), nIns);
			}
		}
	}
	// Fill channels box
	m_CbnChannel.SetItemData(m_CbnChannel.AddString("Melodic (any)"), 0);
	m_CbnChannel.SetItemData(m_CbnChannel.AddString("Percussions"), 10);
	for (UINT iCh=1; iCh<=16; iCh++) if (iCh != 10)
	{
		wsprintf(s, "Melodic %d", iCh);
		m_CbnChannel.SetItemData(m_CbnChannel.AddString(s), iCh);
	}
	m_nCurrInstr = 1;
	m_bPerc = TRUE;
	m_CbnChannel.SetCurSel(0);
	m_CbnInstrument.SetCurSel(0);
	FillProgramBox(FALSE);
	m_CbnProgram.SetCurSel(0);
	UpdateDialog();
	return TRUE;
}


VOID CModToMidi::FillProgramBox(BOOL bPerc)
//-----------------------------------------
{
	CHAR s[256];
	
	if (m_bPerc == bPerc) return;
	m_CbnProgram.ResetContent();
	if (bPerc)
	{
		for (UINT i=0; i<61; i++)
		{
			UINT note = i+24;
			wsprintf(s, "%d (%s): %s", note, szDefaultNoteNames[note], szMidiPercussionNames[i]);
			m_CbnProgram.SetItemData(m_CbnProgram.AddString(s), note/*+1*/); //+1 removed by rewbs because MIDI drums appear to be offset by 1
		}
	} else
	{
		for (UINT i=0; i<128; i++)
		{
			wsprintf(s, "%03d: %s", i, szMidiProgramNames[i]);
			m_CbnProgram.SetItemData(m_CbnProgram.AddString(s), i);
		}
	}
	m_bPerc = bPerc;
}


VOID CModToMidi::UpdateDialog()
//-----------------------------
{
	m_nCurrInstr = m_CbnInstrument.GetItemData(m_CbnInstrument.GetCurSel());
	if ((m_nCurrInstr > 0) && (m_nCurrInstr < MAX_SAMPLES))
	{
		UINT nMidiCh = m_InstrMap[m_nCurrInstr].nChannel;
		if (nMidiCh > 16) nMidiCh = 0;
		if ((!m_bPerc) && (nMidiCh == 10))
		{
			FillProgramBox(TRUE);
		} else
		if ((m_bPerc) && (nMidiCh != 10))
		{
			FillProgramBox(FALSE);
		}
		if (nMidiCh == 10) nMidiCh = 1; else
		if ((nMidiCh) && (nMidiCh < 10)) nMidiCh++;
		m_CbnChannel.SetCurSel(nMidiCh);
		UINT nMidiProgram = m_InstrMap[m_nCurrInstr].nProgram;
		if (m_bPerc)
		{
			//nMidiProgram -= 25;
			nMidiProgram -= 24;	//rewbs: tentative fix to MIDI drums being offset by 1.
			if (nMidiProgram > 60) nMidiProgram = 24;
		} else
		{
			if (nMidiProgram > 127) nMidiProgram = 0;
		}
		m_CbnProgram.SetCurSel(nMidiProgram);
	}
}


VOID CModToMidi::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
//-------------------------------------------------------------------------
{
	CDialog::OnVScroll(nSBCode, nPos, pScrollBar);
	short int pos = (short int)m_SpinInstrument.GetPos();
	if (pos)
	{
		m_SpinInstrument.SetPos(0);
		int nmax = m_CbnInstrument.GetCount();
		int nins = m_CbnInstrument.GetCurSel() + pos;
		if (nins < 0) nins = nmax-1;
		if (nins >= nmax) nins = 0;
		m_CbnInstrument.SetCurSel(nins);
		UpdateDialog();
	}
}


VOID CModToMidi::OnChannelChanged()
//---------------------------------
{
	UINT nMidiCh = m_CbnChannel.GetItemData(m_CbnChannel.GetCurSel());
	if (nMidiCh > 16) return;
	if ((m_nCurrInstr > 0) && (m_nCurrInstr < MAX_SAMPLES))
	{
		m_InstrMap[m_nCurrInstr].nChannel = nMidiCh;
		if (((!m_bPerc) && (nMidiCh == 10))
		 || ((m_bPerc) && (nMidiCh != 10)))
		{
			UpdateDialog();
		}
	}
}


VOID CModToMidi::OnProgramChanged()
//---------------------------------
{
	UINT nProgram = m_CbnProgram.GetItemData(m_CbnProgram.GetCurSel());
	if (nProgram > 127) return;
	if ((m_nCurrInstr > 0) && (m_nCurrInstr < MAX_SAMPLES))
	{
		m_InstrMap[m_nCurrInstr].nProgram = nProgram;
	}
}


VOID CModToMidi::OnOK()
//---------------------
{
	for (UINT i=1; i<=m_pSndFile->m_nInstruments; i++)
	{
		INSTRUMENTHEADER *penv = m_pSndFile->Headers[i];
		if (penv)
		{
			penv->nMidiProgram = m_InstrMap[i].nProgram;
		}
	}
	CDialog::OnOK();
}


#define MOD2MIDI_TEMPOFACTOR	4

BOOL CModToMidi::DoConvert()
//--------------------------
{
	RMIDDATACHUNK rmid;
	MTHDCHUNK mthd;
	MTRKCHUNK mtrk;
	DYNMIDITRACK Tracks[64];
	UINT nMidiChCurPrg[16];
	BYTE tmp[256];
	CHAR s[256];
	UINT nPPQN, nTickMultiplier, nClock, nOrder, nRow;
	UINT nSpeed;
	CFile f;

	const CHANNELINDEX chnCount = min(64, m_pSndFile->GetNumChannels());
	if(chnCount < m_pSndFile->GetNumChannels())
		MessageBox("Note: Only 64 channels will be exported.");

	if (!f.Open(m_szFileName, CFile::modeCreate | CFile::modeWrite))
	{
		return FALSE;
	}
	memset(Tracks, 0, sizeof(Tracks));
	if (!m_pSndFile->m_nDefaultTempo) m_pSndFile->m_nDefaultTempo = 125;
	nTickMultiplier = MOD2MIDI_TEMPOFACTOR;
	nPPQN = (m_pSndFile->m_nDefaultTempo*nTickMultiplier) / 5;
	rmid.id_RIFF = IFFID_RIFF;
	rmid.filelen = sizeof(rmid)+sizeof(mthd)-8;
	rmid.id_RMID = 0x44494D52; // "RMID"
	rmid.id_data = IFFID_data;
	rmid.datalen = 0;
	mthd.id = 0x6468544d; // "MThd"
	mthd.len = BigEndian(sizeof(mthd)-8);
	mthd.wFmt = BigEndianW(1);
	mthd.wTrks = chnCount; // 1 track/channel
	mthd.wTrks = BigEndianW(mthd.wTrks); //Convert to big endian value.
	mthd.wDivision = BigEndianW(nPPQN);
	if (m_bRmi) f.Write(&rmid, sizeof(rmid));
	f.Write(&mthd, sizeof(mthd));

	

	// Add Song Name on track 0
	m_pSndFile->GetTitle(s);
	if (s[0])
	{
		tmp[0] = 0; tmp[1] = 0xff; tmp[2] = 0x01;
		Tracks[0].Write(tmp, 3);
		Tracks[0].WriteLen(strlen(s));
		Tracks[0].Write(s, strlen(s));
	}
	// Add Song comments on track 0
	if ((m_pSndFile->m_lpszSongComments) && (m_pSndFile->m_lpszSongComments[0]))
	{
		tmp[0] = 0; tmp[1] = 0xff; tmp[2] = 0x02;
		Tracks[0].Write(tmp, 3);
		Tracks[0].WriteLen(strlen(m_pSndFile->m_lpszSongComments));
		Tracks[0].Write(m_pSndFile->m_lpszSongComments, strlen(m_pSndFile->m_lpszSongComments));
	}
	// Add channel names
	for (UINT iInit=0; iInit<chnCount; iInit++)
	{
		PDYNMIDITRACK pTrk = &Tracks[iInit];
		lstrcpyn(s, m_pSndFile->ChnSettings[iInit].szName, MAX_CHANNELNAME);
		pTrk->nMidiChannel = iInit & 7;
		if (s[0])
		{
			tmp[0] = 0x00;
			tmp[1] = 0xff;
			tmp[2] = 0x03;
			pTrk->Write(tmp, 3);
			pTrk->WriteLen(strlen(s));
			pTrk->Write(s, strlen(s));
		}
	}
	for (UINT iMidiCh=0; iMidiCh<16; iMidiCh++)
	{
		nMidiChCurPrg[iMidiCh] = 0xff;
	}
	// Play song
	nClock = 0;
	nOrder = 0;
	nRow = 0;
	nSpeed = m_pSndFile->m_nDefaultSpeed;
	while ((nOrder < m_pSndFile->Order.size()) && (m_pSndFile->Order[nOrder] != m_pSndFile->Order.GetInvalidPatIndex()))
	{
		int pattern_break = -1;
		UINT nPat = m_pSndFile->Order[nOrder];
		if ((nPat >= m_pSndFile->Patterns.Size()) || (nPat == m_pSndFile->Order.GetIgnoreIndex()) || (!m_pSndFile->Patterns[nPat]) || (nRow >= m_pSndFile->PatternSize[nPat]))
		{
			nOrder++;
			nRow = 0;
			continue;
		}
		for (UINT nChn=0; nChn<chnCount; nChn++)
		{
			PDYNMIDITRACK pTrk = &Tracks[nChn];
			MODCOMMAND *m = m_pSndFile->Patterns[nPat].GetpModCommand(nRow, nChn);
			UINT delta_time = nClock - pTrk->nLastEventClock;
			UINT len = 0;

			//Process note data only in non-muted channels.
			if((m_pSndFile->ChnSettings[nChn].dwFlags & CHN_MUTE) == 0)
			{
				// Instrument change
				if ((m->instr) && (m->instr != pTrk->nInstrument) && (m->instr < MAX_SAMPLES))
				{
					UINT nIns = m->instr;
					UINT nMidiCh = m_InstrMap[nIns].nChannel;
					UINT nProgram = m_InstrMap[nIns].nProgram;
					if ((nMidiCh) && (nMidiCh <= 16)) nMidiCh--;
					else nMidiCh = nChn&7;
					pTrk->nMidiChannel = nMidiCh;
					pTrk->nMidiProgram = nProgram;
					pTrk->nInstrument = nIns;
					if ((nMidiCh != 9) && (nProgram != nMidiChCurPrg[nMidiCh]))
					{
						tmp[len] = 0xC0|nMidiCh;
						tmp[len+1] = nProgram;
						tmp[len+2] = 0;
						len += 3;
					}
				}
				// Note change
				if (m->note)
				{
					UINT note = m->note - 1;
					for (UINT i=0; i<128; i++)
					{
						if (pTrk->NoteOn[i])
						{
							tmp[len] = 0x90|(pTrk->NoteOn[i]-1);
							tmp[len+1] = i;
							tmp[len+2] = 0;
							tmp[len+3] = 0;
							len += 4;
							pTrk->NoteOn[i] = 0;
						}
					}
					if (m->note <= NOTE_MAX)
					{
						pTrk->NoteOn[note] = pTrk->nMidiChannel+1;
						tmp[len] = 0x90|pTrk->nMidiChannel;
						tmp[len+1] = (pTrk->nMidiChannel==9) ? pTrk->nMidiProgram : note;
						UINT vol = 0x7f;
						UINT nsmp = pTrk->nInstrument;
						if (m_pSndFile->m_nInstruments)
						{
							if ((nsmp < MAX_INSTRUMENTS) && (m_pSndFile->Headers[nsmp]))
							{
								INSTRUMENTHEADER *penv = m_pSndFile->Headers[nsmp];
								nsmp = penv->Keyboard[note];
							} else nsmp = 0;
						}
						if ((nsmp) && (nsmp < MAX_SAMPLES)) vol = m_pSndFile->Ins[nsmp].nVolume;
						if (m->volcmd == VOLCMD_VOLUME) vol = m->vol*4;
						if (m->command == CMD_VOLUME) vol = m->param*4;
						vol = LinearToDLSMidiVolume(vol<<8);
						if (vol > 0x7f) vol = 0x7f;
						tmp[len+2] = (BYTE)vol;
						tmp[len+3] = 0;
						len += 4;
					}
				}
			}
			// Effects
			if (m->command)
			{
				UINT param = m->param;
				switch(m->command)
				{
				case CMD_SPEED:	if ((param) && (param < 32)) nSpeed = param; break;
				case CMD_PATTERNBREAK: pattern_break = param; break;
				}
			}
			if (len > 1)
			{
				pTrk->WriteLen(delta_time);
				pTrk->Write(tmp, len-1);
				pTrk->nLastEventClock = nClock;
			}
		}
		// Increase position
		nClock += nSpeed * nTickMultiplier;
		if (pattern_break >= 0)
		{
			nRow = pattern_break;
			nOrder++;
		} else
		{
			nRow++;
			if (nRow >= m_pSndFile->PatternSize[nPat])
			{
				nRow = 0;
				nOrder++;
			}
		}
	}
	// Write midi tracks
	for (UINT iTrk=0; iTrk<chnCount; iTrk++)
	{
		tmp[0] = 0x00;
		tmp[1] = 0xff;
		tmp[2] = 0x2f;
		tmp[3] = 0x00;
		Tracks[iTrk].Write(tmp, 4);
		mtrk.id = 0x6B72544D;
		mtrk.len = BigEndian(Tracks[iTrk].nTrackSize);
		f.Write(&mtrk, sizeof(mtrk));
		rmid.filelen += sizeof(mtrk) + Tracks[iTrk].nTrackSize;
		if (Tracks[iTrk].nTrackSize > 0)
		{
			f.Write(Tracks[iTrk].pTrackData, Tracks[iTrk].nTrackSize);
			delete[] Tracks[iTrk].pTrackData;
		}
	}
	// Misc fields
	if (m_bRmi)
	{
		// Update header file size
		f.SeekToBegin();
		f.Write(&rmid, sizeof(rmid));
	}
	f.Close();
	return TRUE;
}



