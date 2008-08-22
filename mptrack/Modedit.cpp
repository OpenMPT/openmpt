// modedit.cpp : CModDoc operations
//

#include "stdafx.h"
#include "mptrack.h"
#include "mainfrm.h"
#include "moddoc.h"
#include "dlg_misc.h"
#include "dlsbank.h"

#pragma warning(disable:4244) //"conversion from 'type1' to 'type2', possible loss of data"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define str_mptm_conversion_warning		GetStrI18N(_TEXT("Conversion from mptm to any other moduletype may makes certain features unavailable and is not guaranteed to work properly. Do the conversion anyway?"))

const size_t Pow10Table[10] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};

// Return D'th digit(character) of given value.
// GetDigit<0>(123) == '3'
// GetDigit<1>(123) == '2'
// GetDigit<2>(123) == '1'
template<BYTE D>
inline TCHAR GetDigit(const size_t val)
{
	return (D > 9) ? '0' : 48 + ((val / Pow10Table[D]) % 10);
}


//////////////////////////////////////////////////////////////////////
// Module type conversion

BOOL CModDoc::ChangeModType(UINT nNewType)
//----------------------------------------
{
	CHAR s[256];
	UINT b64 = 0;

	const MODTYPE oldtype = m_SndFile.GetType();
	
	if (nNewType == oldtype && nNewType == MOD_TYPE_IT){
		// Even if m_nType doesn't change, we might need to change extension in itp<->it case.
		// This is because ITP is a HACK and doesn't genuinely change m_nType,
		// but uses flages instead.
		ChangeFileExtension(nNewType);
		return TRUE;
	}

	if(nNewType == oldtype) return TRUE;

	const bool oldTypeIsMOD = (oldtype == MOD_TYPE_MOD), oldTypeIsXM = (oldtype == MOD_TYPE_XM),
				oldTypeIsS3M = (oldtype == MOD_TYPE_S3M), oldTypeIsIT = (oldtype == MOD_TYPE_IT),
				oldTypeIsMPT = (oldtype == MOD_TYPE_MPT), oldTypeIsMOD_XM = (oldTypeIsMOD || oldTypeIsXM),
                oldTypeIsS3M_IT_MPT = (oldTypeIsS3M || oldTypeIsIT || oldTypeIsMPT);

	const bool newTypeIsMOD = (nNewType == MOD_TYPE_MOD), newTypeIsXM =  (nNewType == MOD_TYPE_XM), 
				newTypeIsS3M = (nNewType == MOD_TYPE_S3M), newTypeIsIT = (nNewType == MOD_TYPE_IT),
				newTypeIsMPT = (nNewType == MOD_TYPE_MPT), newTypeIsMOD_XM = (newTypeIsMOD || newTypeIsXM), 
				newTypeIsS3M_IT_MPT = (newTypeIsS3M || newTypeIsIT || newTypeIsMPT), 
				newTypeIsXM_IT_MPT = (newTypeIsXM || newTypeIsIT || newTypeIsMPT),
				newTypeIsIT_MPT = (newTypeIsIT || newTypeIsMPT);

	if(oldTypeIsMPT)
	{
		if(::MessageBox(NULL, str_mptm_conversion_warning, 0, MB_YESNO) != IDYES)
			return FALSE;

		/*
		Incomplete list of MPTm-only features and extensions in the old formats:

		Features only available for MPTm:
		-User definable tunings.
		-Extended pattern range
		-Extended sequence

		Extended features in IT/XM/S3M/MOD(not all listed below are available in all of those formats):
		-plugs
		-Extended ranges for
			-sample count
			-instrument count
			-pattern count
			-sequence size
			-Row count
			-channel count
			-tempo limits
		-Extended sample/instrument properties.
		-MIDI mapping directives
		-Versioninfo
		-channel names
		-pattern names
		-Alternative tempomodes
		-For more info, see e.g. SaveExtendedSongProperties(), SaveExtendedInstrumentProperties()
		*/
		
	}

	// Check if conversion to 64 rows is necessary
	for (UINT ipat=0; ipat<m_SndFile.Patterns.Size(); ipat++)
	{
		if ((m_SndFile.Patterns[ipat]) && (m_SndFile.PatternSize[ipat] != 64)) b64++;
	}
	if (((m_SndFile.m_nInstruments) || (b64)) && (nNewType & (MOD_TYPE_MOD|MOD_TYPE_S3M)))
	{
		if (::MessageBox(NULL,
				"This operation will convert all instruments to samples,\n"
				"and resize all patterns to 64 rows.\n"
				"Do you want to continue?", "Warning", MB_YESNO | MB_ICONQUESTION) != IDYES) return FALSE;
		BeginWaitCursor();
		BEGIN_CRITICAL();
		// Converting instruments to samples
		if (m_SndFile.m_nInstruments)
		{
			ConvertInstrumentsToSamples();
			AddToLog("WARNING: All instruments have been converted to samples.\n");
		}
		// Resizing all patterns to 64 rows
		UINT nPatCvt = 0;
		UINT i = 0;
		for (i=0; i<m_SndFile.Patterns.Size(); i++) if ((m_SndFile.Patterns[i]) && (m_SndFile.PatternSize[i] != 64))
		{
			m_SndFile.Patterns[i].Resize(64);
			if (b64 < 5)
			{
				wsprintf(s, "WARNING: Pattern %d resized to 64 rows\n", i);
				AddToLog(s);
			}
			nPatCvt++;
		}
		if (nPatCvt >= 5)
		{
			wsprintf(s, "WARNING: %d patterns have been resized to 64 rows\n", i);
			AddToLog(s);
		} 
		// Removing all instrument headers
		for (UINT i1=0; i1<MAX_CHANNELS; i1++)
		{
			m_SndFile.Chn[i1].pHeader = NULL;
		}
		for (UINT i2=0; i2<m_SndFile.m_nInstruments; i2++) if (m_SndFile.Headers[i2])
		{
			delete m_SndFile.Headers[i2];
			m_SndFile.Headers[i2] = NULL;
		}
		m_SndFile.m_nInstruments = 0;
		END_CRITICAL();
		EndWaitCursor();
	} //End if (((m_SndFile.m_nInstruments) || (b64)) && (nNewType & (MOD_TYPE_MOD|MOD_TYPE_S3M)))
	BeginWaitCursor();
	// Adjust pattern data
	if(oldTypeIsMOD_XM && newTypeIsS3M_IT_MPT)
	{
		for (UINT nPat=0; nPat<m_SndFile.Patterns.Size(); nPat++) if (m_SndFile.Patterns[nPat])
		{
			MODCOMMAND *m = m_SndFile.Patterns[nPat];
			for (UINT len = m_SndFile.PatternSize[nPat] * m_SndFile.m_nChannels; len; m++, len--)
			{
				if (m->command) switch(m->command)
				{
				case CMD_MODCMDEX:
					m->command = CMD_S3MCMDEX;
					switch(m->param & 0xF0)
					{
					case 0x10:	m->command = CMD_PORTAMENTOUP; m->param |= 0xF0; break;
					case 0x20:	m->command = CMD_PORTAMENTODOWN; m->param |= 0xF0; break;
					case 0x30:	m->param = (m->param & 0x0F) | 0x10; break;
					case 0x40:	m->param = (m->param & 0x0F) | 0x30; break;
					case 0x50:	m->param = (m->param & 0x0F) | 0x20; break;
					case 0x60:	m->param = (m->param & 0x0F) | 0xB0; break;
					case 0x70:	m->param = (m->param & 0x0F) | 0x40; break;
					case 0x90:	m->command = CMD_RETRIG; m->param &= 0x0F; break;
					case 0xA0:	if (m->param & 0x0F) { m->command = CMD_VOLUMESLIDE; m->param = (m->param << 4) | 0x0F; } else m->command = 0; break;
					case 0xB0:	if (m->param & 0x0F) { m->command = CMD_VOLUMESLIDE; m->param |= 0xF0; } else m->command = 0; break;
					}
					break;
				case CMD_VOLUME:
					if (!m->volcmd)
					{
						m->volcmd = VOLCMD_VOLUME;
						m->vol = m->param;
						if (m->vol > 0x40) m->vol = 0x40;
						m->command = m->param = 0;
					}
					break;
				case CMD_PORTAMENTOUP:
					if (m->param > 0xDF) m->param = 0xDF;
					break;
				case CMD_PORTAMENTODOWN:
					if (m->param > 0xDF) m->param = 0xDF;
					break;
				case CMD_XFINEPORTAUPDOWN:
					switch(m->param & 0xF0)
					{
					case 0x10:	m->command = CMD_PORTAMENTOUP; m->param = (m->param & 0x0F) | 0xE0; break;
					case 0x20:	m->command = CMD_PORTAMENTODOWN; m->param = (m->param & 0x0F) | 0xE0; break;
					case 0x50:
					case 0x60:
					case 0x70:
					case 0x90:
					case 0xA0: 	m->command = CMD_S3MCMDEX; break;
					}
					break;
				}
			}
		}
	} else
		if (oldTypeIsS3M_IT_MPT && newTypeIsMOD_XM)
	{
		for (UINT nPat=0; nPat<m_SndFile.Patterns.Size(); nPat++) if (m_SndFile.Patterns[nPat])
		{
			MODCOMMAND *m = m_SndFile.Patterns[nPat];
			for (UINT len = m_SndFile.PatternSize[nPat] * m_SndFile.m_nChannels; len--; m++)
			{
				if (m->command) switch(m->command)
				{
				case CMD_S3MCMDEX:
					m->command = CMD_MODCMDEX;
					switch(m->param & 0xF0)
					{
					case 0x10:	m->param = (m->param & 0x0F) | 0x30; break;
					case 0x20:	m->param = (m->param & 0x0F) | 0x50; break;
					case 0x30:	m->param = (m->param & 0x0F) | 0x40; break;
					case 0x40:	m->param = (m->param & 0x0F) | 0x70; break;
					case 0x50:	
					case 0x60:	
					case 0x70:
					case 0x90:
					case 0xA0:	m->command = CMD_XFINEPORTAUPDOWN; break;
					case 0xB0:	m->param = (m->param & 0x0F) | 0x60; break;
					}
					break;
				case CMD_VOLUMESLIDE:
					if ((m->param & 0xF0) && ((m->param & 0x0F) == 0x0F))
					{
						m->command = CMD_MODCMDEX;
						m->param = (m->param >> 4) | 0xA0;
					} else
					if ((m->param & 0x0F) && ((m->param & 0xF0) == 0xF0))
					{
						m->command = CMD_MODCMDEX;
						m->param = (m->param & 0x0F) | 0xB0;
					}
					break;
				case CMD_PORTAMENTOUP:
					if (m->param >= 0xF0)
					{
						m->command = CMD_MODCMDEX;
						m->param = (m->param & 0x0F) | 0x10;
					} else
					if (m->param >= 0xE0)
					{
						m->command = CMD_MODCMDEX;
						m->param = (((m->param & 0x0F)+3) >> 2) | 0x10;
					} else m->command = CMD_PORTAMENTOUP;
					break;
				case CMD_PORTAMENTODOWN:
					if (m->param >= 0xF0)
					{
						m->command = CMD_MODCMDEX;
						m->param = (m->param & 0x0F) | 0x20;
					} else
					if (m->param >= 0xE0)
					{
						m->command = CMD_MODCMDEX;
						m->param = (((m->param & 0x0F)+3) >> 2) | 0x20;
					} else m->command = CMD_PORTAMENTODOWN;
					break;
				case CMD_SPEED:
					{
						UINT spdmax = (nNewType == MOD_TYPE_XM) ? 0x1F : 0x20;
						if (m->param > spdmax) m->param = spdmax;
					}
					break;
				}
			}
		}
	}
	// Convert XM to MOD
	if (oldTypeIsXM && newTypeIsMOD)
	{
	} else
	// Convert MOD to XM
	if (oldTypeIsMOD && newTypeIsXM)
	{
	} else
	// Convert MOD/XM to S3M/IT/MPT
	if (oldTypeIsMOD_XM && newTypeIsS3M_IT_MPT)
	{
		for (UINT i=1; i<=m_SndFile.m_nSamples; i++)
		{
			m_SndFile.Ins[i].nC4Speed = CSoundFile::TransposeToFrequency(m_SndFile.Ins[i].RelativeTone, m_SndFile.Ins[i].nFineTune);
			m_SndFile.Ins[i].RelativeTone = 0;
			m_SndFile.Ins[i].nFineTune = 0;
		}
		if (oldTypeIsXM && newTypeIsIT_MPT) m_SndFile.m_dwSongFlags |= SONG_ITCOMPATMODE;
	} else
	// Convert S3M/IT/MPT to XM
	if (oldTypeIsS3M_IT_MPT && newTypeIsXM)
	{
		for (UINT i=1; i<=m_SndFile.m_nSamples; i++)
		{
			CSoundFile::FrequencyToTranspose(&m_SndFile.Ins[i]);
			if (!(m_SndFile.Ins[i].uFlags & CHN_PANNING)) m_SndFile.Ins[i].nPan = 128;
		}
		BOOL bBrokenNoteMap = FALSE;
		for (UINT j=1; j<=m_SndFile.m_nInstruments; j++)
		{
			INSTRUMENTHEADER *penv = m_SndFile.Headers[j];
			if (penv)
			{
				for (UINT k=0; k<NOTE_MAX; k++)
				{
					if ((penv->NoteMap[k]) && (penv->NoteMap[k] != (BYTE)(k+1)))
					{
						bBrokenNoteMap = TRUE;
						break;
					}
				}
				penv->dwFlags &= ~(ENV_SETPANNING|ENV_VOLCARRY|ENV_PANCARRY|ENV_PITCHCARRY|ENV_FILTER|ENV_PITCH);
				penv->nIFC &= 0x7F;
				penv->nIFR &= 0x7F;
			}
		}
		if (bBrokenNoteMap) AddToLog("WARNING: Note Mapping will be lost when saving as XM\n");
	}
	// Too many samples ?
	if (newTypeIsMOD && (m_SndFile.m_nSamples > 31))
	{
		AddToLog("WARNING: Samples above 31 will be lost when saving this file as MOD!\n");
	}
	BEGIN_CRITICAL();
	m_SndFile.ChangeModTypeTo(nNewType);
	if (!newTypeIsXM_IT_MPT && (m_SndFile.m_dwSongFlags & SONG_LINEARSLIDES))
	{
		AddToLog("WARNING: Linear Frequency Slides not supported by the new format.\n");
		m_SndFile.m_dwSongFlags &= ~SONG_LINEARSLIDES;
	}
	if (!newTypeIsIT_MPT) m_SndFile.m_dwSongFlags &= ~(SONG_ITOLDEFFECTS|SONG_ITCOMPATMODE);
	if (!newTypeIsS3M) m_SndFile.m_dwSongFlags &= ~SONG_FASTVOLSLIDES;
	END_CRITICAL();
	ChangeFileExtension(nNewType);

	//rewbs.cutomKeys: update effect key commands
	CInputHandler *ih = CMainFrame::GetMainFrame()->GetInputHandler();
	if	(newTypeIsMOD_XM) {
		ih->SetXMEffects();
	} else {
		ih->SetITEffects();
	}
	//end rewbs.cutomKeys

	SetModified();
	ClearUndo();
	UpdateAllViews(NULL, HINT_MODTYPE);
	EndWaitCursor();
	return TRUE;
}






// Change the number of channels
BOOL CModDoc::ChangeNumChannels(UINT nNewChannels, const bool showCancelInRemoveDlg)
//------------------------------------------------
{
	const CHANNELINDEX maxChans = m_SndFile.GetModSpecifications().channelsMax;

	if (nNewChannels > maxChans) {
		CString error;
		error.Format("Error: Max number of channels for this type is %d", maxChans);
		::AfxMessageBox(error, MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	if (nNewChannels == m_SndFile.m_nChannels) return FALSE;
	if (nNewChannels < m_SndFile.m_nChannels)
	{
		UINT nChnToRemove = 0;
		CHANNELINDEX nFound = 0;

		//nNewChannels = 0 means user can choose how many channels to remove
		if(nNewChannels > 0) {
			nChnToRemove = m_SndFile.m_nChannels - nNewChannels;
			nFound = nChnToRemove;
		} else {
			nChnToRemove = 0;
			nFound = m_SndFile.m_nChannels;
		}
		
		CRemoveChannelsDlg rem(&m_SndFile, nChnToRemove, showCancelInRemoveDlg);
		CheckUnusedChannels(rem.m_bChnMask, nFound);
		if (rem.DoModal() != IDOK) return FALSE;

		// Removing selected channels
		RemoveChannels(rem.m_bChnMask);
	} else
	{
		BeginWaitCursor();
		// Increasing number of channels
		BEGIN_CRITICAL();
		for (UINT i=0; i<m_SndFile.Patterns.Size(); i++) if (m_SndFile.Patterns[i])
		{
			MODCOMMAND *p = m_SndFile.Patterns[i];
			MODCOMMAND *newp = CSoundFile::AllocatePattern(m_SndFile.PatternSize[i], nNewChannels);
			if (!newp)
			{
				END_CRITICAL();
				AddToLog("ERROR: Not enough memory to create new channels!\nPattern Data is corrupted!\n");
				return FALSE;
			}
			for (UINT j=0; j<m_SndFile.PatternSize[i]; j++)
			{
				memcpy(&newp[j*nNewChannels], &p[j*m_SndFile.m_nChannels], m_SndFile.m_nChannels*sizeof(MODCOMMAND));
			}
			m_SndFile.Patterns[i] = newp;
			CSoundFile::FreePattern(p);
		}
		m_SndFile.m_nChannels = nNewChannels;
		END_CRITICAL();
		EndWaitCursor();
	}
	SetModified();
	ClearUndo();
	UpdateAllViews(NULL, HINT_MODTYPE);
	return TRUE;
}


BOOL CModDoc::RemoveChannels(BOOL m_bChnMask[MAX_CHANNELS])
//--------------------------------------------------------
//To remove all channels whose index corresponds to true value at m_bChnMask[] array. Code is almost non-modified copy of
//the code which was in CModDoc::ChangeNumChannels(UINT nNewChannels) - the only differences are the lines before 
//BeginWaitCursor(), few lines in the end and that nNewChannels is renamed to nRemaningChannels.
{
		UINT nRemainingChannels = 0;
		//First calculating how many channels are to be left
		UINT i = 0;
		for(i = 0; i<m_SndFile.m_nChannels; i++)
		{
			if(!m_bChnMask[i]) nRemainingChannels++;
		}
		if(nRemainingChannels == m_SndFile.m_nChannels || nRemainingChannels < m_SndFile.GetModSpecifications().channelsMin)
		{
			CString str;	
			if(nRemainingChannels == m_SndFile.m_nChannels) str.Format("No channels chosen to be removed.");
			else str.Format("No removal done - channel number is already at minimum.");
			CMainFrame::GetMainFrame()->MessageBox(str , "Remove channel", MB_OK | MB_ICONINFORMATION);
			return FALSE;
		}

		BeginWaitCursor();
		BEGIN_CRITICAL();
		for (i=0; i<m_SndFile.Patterns.Size(); i++) if (m_SndFile.Patterns[i])
		{
			MODCOMMAND *p = m_SndFile.Patterns[i];
			MODCOMMAND *newp = CSoundFile::AllocatePattern(m_SndFile.PatternSize[i], nRemainingChannels);
			if (!newp)
			{
				END_CRITICAL();
				AddToLog("ERROR: Not enough memory to resize patterns!\nPattern Data is corrupted!");
				return TRUE;
			}
			MODCOMMAND *tmpsrc = p, *tmpdest = newp;
			for (UINT j=0; j<m_SndFile.PatternSize[i]; j++)
			{
				for (UINT k=0; k<m_SndFile.m_nChannels; k++, tmpsrc++)
				{
					if (!m_bChnMask[k]) *tmpdest++ = *tmpsrc;
				}
			}
			m_SndFile.Patterns[i] = newp;
			CSoundFile::FreePattern(p);
		}
		UINT tmpchn = 0;
		for (i=0; i<m_SndFile.m_nChannels; i++)
		{
			if (!m_bChnMask[i])
			{
				if (tmpchn != i)
				{
					m_SndFile.ChnSettings[tmpchn] = m_SndFile.ChnSettings[i];
					m_SndFile.Chn[tmpchn] = m_SndFile.Chn[i];
				}
				tmpchn++;
				if (i >= nRemainingChannels)
				{
					m_SndFile.Chn[i].dwFlags |= CHN_MUTE;
					m_SndFile.InitChannel(i);
				}
			}
		}
		m_SndFile.m_nChannels = nRemainingChannels;
		END_CRITICAL();
		EndWaitCursor();
		SetModified();
		ClearUndo();
		UpdateAllViews(NULL, HINT_MODTYPE);
		return FALSE;
}


BOOL CModDoc::RemoveUnusedPatterns(BOOL bRemove)
//----------------------------------------------
{
	const UINT maxPatIndex = m_SndFile.Patterns.Size();
	const UINT maxOrdIndex = m_SndFile.Order.size();
	vector<UINT> nPatMap(maxPatIndex, 0);
	vector<UINT> nPatRows(maxPatIndex, 0);
	vector<MODCOMMAND*> pPatterns(maxPatIndex, NULL);
	vector<BOOL> bPatUsed(maxPatIndex, false);
	
	CHAR s[512];
	BOOL bEnd = FALSE, bReordered = FALSE;
	UINT nPatRemoved = 0, nMinToRemove, nPats;
	
	BeginWaitCursor();
	UINT maxpat = 0;
	for (UINT iord=0; iord<maxOrdIndex; iord++)
	{
		UINT n = m_SndFile.Order[iord];
		if (n < maxPatIndex)
		{
			if (n >= maxpat) maxpat = n+1;
			if (!bEnd) bPatUsed[n] = TRUE;
		} else if (n == m_SndFile.Order.GetInvalidPatIndex()) bEnd = TRUE;
	}
	nMinToRemove = 0;
	if (!bRemove)
	{
		UINT imax = maxPatIndex;
		while (imax > 0)
		{
			imax--;
			if ((m_SndFile.Patterns[imax]) && (bPatUsed[imax])) break;
		}
		nMinToRemove = imax+1;
	}
	for (UINT ipat=maxpat; ipat<maxPatIndex; ipat++) if ((m_SndFile.Patterns[ipat]) && (ipat >= nMinToRemove))
	{
		MODCOMMAND *m = m_SndFile.Patterns[ipat];
		UINT ncmd = m_SndFile.m_nChannels * m_SndFile.PatternSize[ipat];
		for (UINT i=0; i<ncmd; i++)
		{
			if ((m[i].note) || (m[i].instr) || (m[i].volcmd) || (m[i].command)) goto NotEmpty;
		}
		m_SndFile.Patterns.Remove(ipat);
		nPatRemoved++;
	NotEmpty:
		;
	}
	UINT bWaste = 0;
	for (UINT ichk=0; ichk < maxPatIndex; ichk++)
	{
		if ((m_SndFile.Patterns[ichk]) && (!bPatUsed[ichk])) bWaste++;
	}
	if ((bRemove) && (bWaste))
	{
		EndWaitCursor();
		wsprintf(s, "%d pattern(s) present in file, but not used in the song\nDo you want to reorder the sequence list and remove these patterns?", bWaste);
		if (CMainFrame::GetMainFrame()->MessageBox(s, "Pattern Cleanup", MB_YESNO) != IDYES) return TRUE;
		BeginWaitCursor();
	}
	for (UINT irst=0; irst<maxPatIndex; irst++) nPatMap[irst] = 0xFFFF;
	nPats = 0;
	UINT imap = 0;
	for (imap=0; imap<maxOrdIndex; imap++)
	{
		UINT n = m_SndFile.Order[imap];
		if (n < maxPatIndex)
		{
			if (nPatMap[n] > maxPatIndex) nPatMap[n] = nPats++;
			m_SndFile.Order[imap] = nPatMap[n];
		} else if (n == 0xFF) break;
	}
	// Add unused patterns at the end
	if ((!bRemove) || (!bWaste))
	{
		for (UINT iadd=0; iadd<maxPatIndex; iadd++)
		{
			if ((m_SndFile.Patterns[iadd]) && (nPatMap[iadd] >= maxPatIndex))
			{
				nPatMap[iadd] = nPats++;
			}
		}
	}
	while (imap < maxOrdIndex)
	{
		m_SndFile.Order[imap++] = m_SndFile.Order.GetInvalidPatIndex();
	}
	BEGIN_CRITICAL();
	// Reorder patterns & Delete unused patterns
	{
		UINT npatnames = m_SndFile.m_nPatternNames;
		LPSTR lpszpatnames = m_SndFile.m_lpszPatternNames;
		m_SndFile.m_nPatternNames = 0;
		m_SndFile.m_lpszPatternNames = NULL;
		for (UINT i=0; i<maxPatIndex; i++)
		{
			UINT k = nPatMap[i];
			if (k < maxPatIndex)
			{
				if (i != k) bReordered = TRUE;
				// Remap pattern names
				if (i < npatnames)
				{
					UINT noldpatnames = m_SndFile.m_nPatternNames;
					LPSTR lpszoldpatnames = m_SndFile.m_lpszPatternNames;
					m_SndFile.m_nPatternNames = npatnames;
					m_SndFile.m_lpszPatternNames = lpszpatnames;
					m_SndFile.GetPatternName(i, s);
					m_SndFile.m_nPatternNames = noldpatnames;
					m_SndFile.m_lpszPatternNames = lpszoldpatnames;
					if (s[0]) m_SndFile.SetPatternName(k, s);
				}
				nPatRows[k] = m_SndFile.PatternSize[i];
				pPatterns[k] = m_SndFile.Patterns[i];
			} else
			if (m_SndFile.Patterns[i])
			{
				m_SndFile.Patterns.Remove(i);
				nPatRemoved++;
			}
		}
		for (UINT j=0; j<maxPatIndex;j++)
		{
			m_SndFile.Patterns[j].SetData(pPatterns[j], nPatRows[j]);
		}
	}
	END_CRITICAL();
	EndWaitCursor();
	if ((nPatRemoved) || (bReordered))
	{
		ClearUndo();
		SetModified();
		if (nPatRemoved)
		{
			wsprintf(s, "%d pattern(s) removed.\n", nPatRemoved);
			AddToLog(s);
		}
		return TRUE;
	}
	return FALSE;
}


BOOL CModDoc::ConvertInstrumentsToSamples()
//-----------------------------------------
{
	if (!m_SndFile.m_nInstruments) return FALSE;
	for (UINT i=0; i<m_SndFile.Patterns.Size(); i++) if (m_SndFile.Patterns[i])
	{
		MODCOMMAND *p = m_SndFile.Patterns[i];
		for (UINT j=m_SndFile.m_nChannels*m_SndFile.PatternSize[i]; j; j--, p++) if (p->instr)
		{
			UINT instr = p->instr;
			UINT note = p->note;
			UINT newins = 0;
			if ((note) && (note < 128)) note--; else note = 5*12;
			if ((instr < MAX_INSTRUMENTS) && (m_SndFile.Headers[instr]))
			{
				INSTRUMENTHEADER *penv = m_SndFile.Headers[instr];
				newins = penv->Keyboard[note];
				if (newins >= MAX_SAMPLES) newins = 0;
			}
			p->instr = newins;
		}
	}
	return TRUE;
}


BOOL CModDoc::RemoveUnusedSamples()
//---------------------------------
{
	CHAR s[512];
	BOOL bIns[MAX_SAMPLES];
	UINT nExt = 0, nLoopOpt = 0;
	UINT nRemoved = 0;
	

	BeginWaitCursor();
	for (UINT i=m_SndFile.m_nSamples; i>=1; i--) if (m_SndFile.Ins[i].pSample)
	{
		if (!m_SndFile.IsSampleUsed(i))
		{
			BEGIN_CRITICAL();
			m_SndFile.DestroySample(i);
			if ((i == m_SndFile.m_nSamples) && (i > 1)) m_SndFile.m_nSamples--;
			END_CRITICAL();
			nRemoved++;
		}
	}
	if (m_SndFile.m_nInstruments)
	{
		memset(bIns, 0, sizeof(bIns));
		for (UINT ipat=0; ipat<m_SndFile.Patterns.Size(); ipat++)
		{
			MODCOMMAND *p = m_SndFile.Patterns[ipat];
			if (p)
			{
				UINT jmax = m_SndFile.PatternSize[ipat] * m_SndFile.m_nChannels;
				for (UINT j=0; j<jmax; j++, p++)
				{
					if ((p->note) && (p->note <= NOTE_MAX))
					{
						if ((p->instr) && (p->instr < MAX_INSTRUMENTS))
						{
							INSTRUMENTHEADER *penv = m_SndFile.Headers[p->instr];
							if (penv)
							{
								UINT n = penv->Keyboard[p->note-1];
								if (n < MAX_SAMPLES) bIns[n] = TRUE;
							}
						} else
						{
							for (UINT k=1; k<=m_SndFile.m_nInstruments; k++)
							{
								INSTRUMENTHEADER *penv = m_SndFile.Headers[k];
								if (penv)
								{
									UINT n = penv->Keyboard[p->note-1];
									if (n < MAX_SAMPLES) bIns[n] = TRUE;
								}
							}
						}
					}
				}
			}
		}
		for (UINT ichk=1; ichk<MAX_SAMPLES; ichk++)
		{
			if ((!bIns[ichk]) && (m_SndFile.Ins[ichk].pSample)) nExt++;
		}
	}
	EndWaitCursor();
	if (nExt &&  !((m_SndFile.m_nType & MOD_TYPE_IT) && (m_SndFile.m_dwSongFlags&SONG_ITPROJECT)))
	{	//We don't remove an instrument's unused samples in an ITP.
		wsprintf(s, "OpenMPT detected %d sample(s) referenced by an instrument,\n"
					"but not used in the song. Do you want to remove them ?", nExt);
		if (::MessageBox(NULL, s, "Sample Cleanup", MB_YESNO | MB_ICONQUESTION) == IDYES)
		{
			for (UINT j=1; j<MAX_SAMPLES; j++)
			{
				if ((!bIns[j]) && (m_SndFile.Ins[j].pSample))
				{
					BEGIN_CRITICAL();
					m_SndFile.DestroySample(j);
					if ((j == m_SndFile.m_nSamples) && (j > 1)) m_SndFile.m_nSamples--;
					END_CRITICAL();
					nRemoved++;
				}
			}
		}
	}
	for (UINT ilo=1; ilo<=m_SndFile.m_nSamples; ilo++) if (m_SndFile.Ins[ilo].pSample)
	{
		if ((m_SndFile.Ins[ilo].uFlags & (CHN_LOOP|CHN_SUSTAINLOOP))
		 && (m_SndFile.Ins[ilo].nLength > m_SndFile.Ins[ilo].nLoopEnd + 2)
		 && (m_SndFile.Ins[ilo].nLength > m_SndFile.Ins[ilo].nSustainEnd + 2)) nLoopOpt++;
	}
	if (nLoopOpt)
	{
		wsprintf(s, "OpenMPT detected %d sample(s) with unused data after the loop end point,\n"
					"Do you want to optimize it, and remove this unused data ?", nLoopOpt);
		if (::MessageBox(NULL, s, "Sample Cleanup", MB_YESNO | MB_ICONQUESTION) == IDYES)
		{
			for (UINT j=1; j<=m_SndFile.m_nSamples; j++)
			{
				if ((m_SndFile.Ins[j].uFlags & (CHN_LOOP|CHN_SUSTAINLOOP))
				 && (m_SndFile.Ins[j].nLength > m_SndFile.Ins[j].nLoopEnd + 2)
				 && (m_SndFile.Ins[j].nLength > m_SndFile.Ins[j].nSustainEnd + 2))
				{
					UINT lmax = m_SndFile.Ins[j].nLoopEnd + 2;
					if (m_SndFile.Ins[j].nSustainEnd + 2 > lmax) lmax = m_SndFile.Ins[j].nSustainEnd + 2;
					if ((lmax < m_SndFile.Ins[j].nLength) && (lmax >= 16)) m_SndFile.Ins[j].nLength = lmax;
				}
			}
		} else nLoopOpt = 0;
	}
	if ((nRemoved) || (nLoopOpt))
	{
		if (nRemoved)
		{
			wsprintf(s, "%d unused sample(s) removed\n" ,nRemoved);
			AddToLog(s);
		}
		if (nLoopOpt)
		{
			wsprintf(s, "%d sample loop(s) optimized\n" ,nLoopOpt);
			AddToLog(s);
		}
		SetModified();
		return TRUE;
	}
	return FALSE;
}

BOOL CModDoc::RemoveUnusedPlugs() 
//-------------------------------
{
	BYTE usedmap[MAX_MIXPLUGINS];
	memset(usedmap, false, MAX_MIXPLUGINS);
	

	for (PLUGINDEX nPlug=0; nPlug < MAX_MIXPLUGINS; nPlug++) {

		//Is the plugin assigned to a channel?
		for (CHANNELINDEX nChn = 0; nChn < m_SndFile.GetNumChannels(); nChn++) {
			if (m_SndFile.ChnSettings[nChn].nMixPlugin == nPlug + 1u) {
				usedmap[nPlug]=true;
				break;
			}
		}

		//Is the plugin used by an instrument?
		for (INSTRUMENTINDEX nIns=1; nIns<=m_SndFile.GetNumInstruments(); nIns++) {
			if (m_SndFile.Headers[nIns] && (m_SndFile.Headers[nIns]->nMixPlug == nPlug+1)) {
				usedmap[nPlug]=true;
				break;
			}
		}

		//Is the plugin assigned to master?
		if (m_SndFile.m_MixPlugins[nPlug].Info.dwInputRouting & MIXPLUG_INPUTF_MASTEREFFECT) {
			usedmap[nPlug]=true;
		}

		//all outputs of used plugins count as used
		if (usedmap[nPlug]!=0) {
			if (m_SndFile.m_MixPlugins[nPlug].Info.dwOutputRouting & 0x80) {
				int output = m_SndFile.m_MixPlugins[nPlug].Info.dwOutputRouting & 0x7f;
				usedmap[output]=true;
			}
		}

	}

	//Remove unused plugins
	int nRemoved=0;
	for (int nPlug=0; nPlug<MAX_MIXPLUGINS; nPlug++) {
		SNDMIXPLUGIN* pPlug = &m_SndFile.m_MixPlugins[nPlug];		
		if (usedmap[nPlug] || !pPlug) {
			Log("Keeping mixplug addess (%d): %X\n", nPlug, &(pPlug->pMixPlugin));	
			continue;
		}

		if (pPlug->pPluginData) {
			delete pPlug->pPluginData;
			pPlug->pPluginData = NULL;
		}
		if (pPlug->pMixPlugin) {
			pPlug->pMixPlugin->Release();
			pPlug->pMixPlugin=NULL;
		}
		if (pPlug->pMixState) {
			delete pPlug->pMixState;
		}

		memset(&(pPlug->Info), 0, sizeof(SNDMIXPLUGININFO));
		Log("Zeroing range (%d) %X - %X\n", nPlug, &(pPlug->Info),  &(pPlug->Info)+sizeof(SNDMIXPLUGININFO));
		pPlug->nPluginDataSize=0;
		pPlug->fDryRatio=0;	
		pPlug->defaultProgram=0;
		nRemoved++;

	}

	if (nRemoved) {
		SetModified();
	}

	return nRemoved;
}

BOOL CModDoc::RemoveUnusedInstruments()
//-------------------------------------
{
	BYTE usedmap[MAX_INSTRUMENTS];
	BYTE swapmap[MAX_INSTRUMENTS];
	BYTE swapdest[MAX_INSTRUMENTS];
	CHAR s[512];
	UINT nRemoved = 0;
	UINT nSwap, nIndex;
	BOOL bReorg = FALSE;

	if (!m_SndFile.m_nInstruments) return FALSE;

	char removeSamples = -1;
	if ( !((m_SndFile.m_nType & MOD_TYPE_IT) && (m_SndFile.m_dwSongFlags&SONG_ITPROJECT))) { //never remove an instrument's samples in ITP.
		if(::MessageBox(NULL, "Remove samples associated with an instrument if they are unused?", "Removing instrument", MB_YESNO | MB_ICONQUESTION) == IDYES) {
			removeSamples = 1;
		}
	} else {
		MessageBox(NULL, "This is an IT project, so no samples associated with a used instrument will be removed.", "Removing Instruments", MB_OK | MB_ICONINFORMATION);
	}

	BeginWaitCursor();
	memset(usedmap, 0, sizeof(usedmap));

	for (UINT i=m_SndFile.m_nInstruments; i>=1; i--)
	{
		if (!m_SndFile.IsInstrumentUsed(i))
		{
			BEGIN_CRITICAL();
// -> CODE#0003
// -> DESC="remove instrument's samples"
//			m_SndFile.DestroyInstrument(i);
			m_SndFile.DestroyInstrument(i,removeSamples);
// -! BEHAVIOUR_CHANGE#0003
			if ((i == m_SndFile.m_nInstruments) && (i>1)) m_SndFile.m_nInstruments--; else bReorg = TRUE;
			END_CRITICAL();
			nRemoved++;
		} else
		{
			usedmap[i] = 1;
		}
	}
	EndWaitCursor();
	if ((bReorg) && (m_SndFile.m_nInstruments > 1)
	 && (::MessageBox(NULL, "Do you want to reorganize the remaining instruments ?", NULL, MB_YESNO | MB_ICONQUESTION) == IDYES))
	{
		BeginWaitCursor();
		BEGIN_CRITICAL();
		nSwap = 0;
		nIndex = 1;
		for (UINT j=1; j<=m_SndFile.m_nInstruments; j++)
		{
			if (usedmap[j])
			{
				while (nIndex<j)
				{
					if ((!usedmap[nIndex]) && (!m_SndFile.Headers[nIndex]))
					{
						swapmap[nSwap] = j;
						swapdest[nSwap] = nIndex;
						m_SndFile.Headers[nIndex] = m_SndFile.Headers[j];
						m_SndFile.Headers[j] = NULL;
						usedmap[nIndex] = 1;
						usedmap[j] = 0;
						nSwap++;
						nIndex++;
						break;
					}
					nIndex++;
				}
			}
		}
		while ((m_SndFile.m_nInstruments > 1) && (!m_SndFile.Headers[m_SndFile.m_nInstruments])) m_SndFile.m_nInstruments--;
		END_CRITICAL();
		if (nSwap > 0)
		{
			for (UINT iPat=0; iPat<m_SndFile.Patterns.Size(); iPat++) if (m_SndFile.Patterns[iPat])
			{
				MODCOMMAND *p = m_SndFile.Patterns[iPat];
				UINT nLen = m_SndFile.m_nChannels * m_SndFile.PatternSize[iPat];
				while (nLen--)
				{
					if (p->instr)
					{
						for (UINT k=0; k<nSwap; k++)
						{
							if (p->instr == swapmap[k]) p->instr = swapdest[k];
						}
					}
					p++;
				}
			}
		}
		EndWaitCursor();
	}
	if (nRemoved)
	{
		wsprintf(s, "%d unused instrument(s) removed\n", nRemoved);
		AddToLog(s);
		SetModified();
		return TRUE;
	}
	return FALSE;
}


BOOL CModDoc::AdjustEndOfSample(UINT nSample)
//-------------------------------------------
{
	const MODINSTRUMENT *pins;
	if (nSample >= MAX_SAMPLES) return FALSE;
	pins = &m_SndFile.Ins[nSample];
	if ((!pins->nLength) || (!pins->pSample)) return FALSE;
	BEGIN_CRITICAL();
	UINT len = pins->nLength;
	if (pins->uFlags & CHN_16BIT)
	{
		signed short *p = (signed short *)pins->pSample;
		if (pins->uFlags & CHN_STEREO)
		{
			p[(len+3)*2] = p[(len+2)*2] = p[(len+1)*2] = p[(len)*2] = p[(len-1)*2];
			p[(len+3)*2+1] = p[(len+2)*2+1] = p[(len+1)*2+1] = p[(len)*2+1] = p[(len-1)*2+1];
		} else
		{
			p[len+4] = p[len+3] = p[len+2] = p[len+1] = p[len] = p[len-1];
		}
		if (((pins->uFlags & (CHN_LOOP|CHN_PINGPONGLOOP|CHN_STEREO)) == CHN_LOOP)
		 && (pins->nLoopEnd == pins->nLength)
		 && (pins->nLoopEnd > pins->nLoopStart) && (pins->nLength > 2))
		{
			p[len] = p[pins->nLoopStart];
			p[len+1] = p[pins->nLoopStart+1];
			p[len+2] = p[pins->nLoopStart+2];
			p[len+3] = p[pins->nLoopStart+3];
			p[len+4] = p[pins->nLoopStart+4];
		}
	} else
	{
		signed char *p = (signed char *)pins->pSample;
		if (pins->uFlags & CHN_STEREO)
		{
			p[(len+3)*2] = p[(len+2)*2] = p[(len+1)*2] = p[(len)*2] = p[(len-1)*2];
			p[(len+3)*2+1] = p[(len+2)*2+1] = p[(len+1)*2+1] = p[(len)*2+1] = p[(len-1)*2+1];
		} else
		{
			p[len+4] = p[len+3] = p[len+2] = p[len+1] = p[len] = p[len-1];
		}
		if (((pins->uFlags & (CHN_LOOP|CHN_PINGPONGLOOP|CHN_STEREO)) == CHN_LOOP)
		 && (pins->nLoopEnd == pins->nLength)
		 && (pins->nLoopEnd > pins->nLoopStart) && (pins->nLength > 2))
		{
			p[len] = p[pins->nLoopStart];
			p[len+1] = p[pins->nLoopStart+1];
			p[len+2] = p[pins->nLoopStart+2];
			p[len+3] = p[pins->nLoopStart+3];
			p[len+4] = p[pins->nLoopStart+4];
		}
	}
	// Update channels with new loop values
	{
		for (UINT i=0; i<MAX_CHANNELS; i++) if ((m_SndFile.Chn[i].pInstrument == pins) && (m_SndFile.Chn[i].nLength))
		{
			if ((pins->nLoopStart + 3 < pins->nLoopEnd) && (pins->nLoopEnd <= pins->nLength))
			{
				m_SndFile.Chn[i].nLoopStart = pins->nLoopStart;
				m_SndFile.Chn[i].nLoopEnd = pins->nLoopEnd;
				m_SndFile.Chn[i].nLength = pins->nLoopEnd;
				if (m_SndFile.Chn[i].nPos > m_SndFile.Chn[i].nLength)
				{
					m_SndFile.Chn[i].nPos = m_SndFile.Chn[i].nLoopStart;
					m_SndFile.Chn[i].dwFlags &= ~CHN_PINGPONGFLAG;
				}
				DWORD d = m_SndFile.Chn[i].dwFlags & ~(CHN_PINGPONGLOOP|CHN_LOOP);
				if (pins->uFlags & CHN_LOOP)
				{
					d |= CHN_LOOP;
					if (pins->uFlags & CHN_PINGPONGLOOP) d |= CHN_PINGPONGLOOP;
				}
				m_SndFile.Chn[i].dwFlags = d;
			} else
			if (!(pins->uFlags & CHN_LOOP))
			{
				m_SndFile.Chn[i].dwFlags &= ~(CHN_PINGPONGLOOP|CHN_LOOP);
			}
		}
	}
	END_CRITICAL();
	return TRUE;
}


LONG CModDoc::InsertPattern(LONG nOrd, UINT nRows)
//------------------------------------------------
{
	const int i = m_SndFile.Patterns.Insert(nRows);
	if(i < 0)
		return -1;

	//Increasing orderlist size if given order is beyond current limit,
	//or if the last order already has a pattern.
	if((nOrd == m_SndFile.Order.size() ||
		m_SndFile.Order.back() < m_SndFile.Patterns.Size() ) &&
		m_SndFile.Order.size() < m_SndFile.GetModSpecifications().ordersMax)
	{
		m_SndFile.Order.push_back(m_SndFile.Order.GetInvalidPatIndex());
	}

	for (UINT j=0; j<m_SndFile.Order.size(); j++)
	{
		if (m_SndFile.Order[j] == i) break;
		if (m_SndFile.Order[j] == m_SndFile.Order.GetInvalidPatIndex())
		{
			m_SndFile.Order[j] = i;
			break;
		}
		if ((nOrd >= 0) && (j == (UINT)nOrd))
		{
			for (UINT k=m_SndFile.Order.size()-1; k>j; k--)
			{
				m_SndFile.Order[k] = m_SndFile.Order[k-1];
			}
			m_SndFile.Order[j] = i;
			break;
		}
	}

	SetModified();
	return i;
}


LONG CModDoc::InsertSample(BOOL bLimit)
//-------------------------------------
{
	UINT i = 1;
	for (i=1; i<=m_SndFile.m_nSamples; i++)
	{
		if ((!m_SndFile.m_szNames[i][0]) && (m_SndFile.Ins[i].pSample == NULL))
		{
			if ((!m_SndFile.m_nInstruments) || (!m_SndFile.IsSampleUsed(i)))
			break;
		}
	}
	if (((bLimit) && (i >= 200) && (!m_SndFile.m_nInstruments))
	 || (i >= MAX_SAMPLES))
	{
		ErrorBox(IDS_ERR_TOOMANYSMP, CMainFrame::GetMainFrame());
		return -1;
	}
	if (!m_SndFile.m_szNames[i][0]) strcpy(m_SndFile.m_szNames[i], "untitled");
	MODINSTRUMENT *pins = &m_SndFile.Ins[i];
	pins->nVolume = 256;
	pins->nGlobalVol = 64;
	pins->nPan = 128;
	pins->nC4Speed = 8363;
	pins->RelativeTone = 0;
	pins->nFineTune = 0;
	pins->nVibType = 0;
	pins->nVibSweep = 0;
	pins->nVibDepth = 0;
	pins->nVibRate = 0;
	pins->uFlags &= ~(CHN_PANNING|CHN_SUSTAINLOOP);
	if (m_SndFile.m_nType == MOD_TYPE_XM) pins->uFlags |= CHN_PANNING;
	if (i > m_SndFile.m_nSamples) m_SndFile.m_nSamples = i;
	SetModified();
	return i;
}


LONG CModDoc::InsertInstrument(LONG lSample, LONG lDuplicate)
//-----------------------------------------------------------
{
	INSTRUMENTHEADER *pDup = NULL;
	if ((m_SndFile.m_nType != MOD_TYPE_XM) && !(m_SndFile.m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT))) return -1;
	if ((lDuplicate > 0) && (lDuplicate <= (LONG)m_SndFile.m_nInstruments))
	{
		pDup = m_SndFile.Headers[lDuplicate];
	}
	if ((!m_SndFile.m_nInstruments) && ((m_SndFile.m_nSamples > 1) || (m_SndFile.Ins[1].pSample)))
	{
		if (pDup) return -1;
		UINT n = CMainFrame::GetMainFrame()->MessageBox("Convert existing samples to instruments first?", NULL, MB_YESNOCANCEL|MB_ICONQUESTION);
		if (n == IDYES)
		{
			UINT nInstruments = m_SndFile.m_nSamples;
			if (nInstruments >= MAX_INSTRUMENTS) nInstruments = MAX_INSTRUMENTS-1;
			for (UINT smp=1; smp<=nInstruments; smp++)
			{
				m_SndFile.Ins[smp].uFlags &= ~CHN_MUTE;
				if (!m_SndFile.Headers[smp])
				{
					INSTRUMENTHEADER *p = new INSTRUMENTHEADER;
					if (!p)
					{
						ErrorBox(IDS_ERR_OUTOFMEMORY, CMainFrame::GetMainFrame());
						return -1;
					}
					InitializeInstrument(p, smp);
					m_SndFile.Headers[smp] = p;
					lstrcpyn(p->name, m_SndFile.m_szNames[smp], sizeof(p->name));
				}
			}
			m_SndFile.m_nInstruments = nInstruments;
		} else
		if (n != IDNO) return -1;
	}
	UINT newins = 0;
	for (UINT i=1; i<=m_SndFile.m_nInstruments; i++)
	{
		if (!m_SndFile.Headers[i])
		{
			newins = i;
			break;
		}
	}
	if (!newins)
	{
		if (m_SndFile.m_nInstruments >= MAX_INSTRUMENTS-1)
		{
			ErrorBox(IDS_ERR_TOOMANYINS, CMainFrame::GetMainFrame());
			return -1;
		}
		newins = ++m_SndFile.m_nInstruments;
	}
	INSTRUMENTHEADER *penv = new INSTRUMENTHEADER;
	if (penv)
	{
		UINT newsmp = 0;
		if ((lSample > 0) && (lSample < MAX_SAMPLES))
		{
			newsmp = lSample;
		} else
		if (!pDup)
		{
			for (UINT k=1; k<=m_SndFile.m_nSamples; k++)
			{
				if (!m_SndFile.IsSampleUsed(k))
				{
					newsmp = k;
					break;
				}
			}
			if (!newsmp)
			{
				int inssmp = InsertSample();
				if (inssmp > 0) newsmp = inssmp;
			}
		}
		BEGIN_CRITICAL();
		if (pDup)
		{
			*penv = *pDup;
// -> CODE#0023
// -> DESC="IT project files (.itp)"
			strcpy(m_SndFile.m_szInstrumentPath[newins-1],m_SndFile.m_szInstrumentPath[lDuplicate-1]);
			m_SndFile.instrumentModified[newins-1] = FALSE;
// -! NEW_FEATURE#0023
		} else
		{
			InitializeInstrument(penv, newsmp);
		}
		m_SndFile.Headers[newins] = penv;
		END_CRITICAL();
		SetModified();
	} else
	{
		ErrorBox(IDS_ERR_OUTOFMEMORY, CMainFrame::GetMainFrame());
		return -1;
	}
	return newins;
}


void CModDoc::InitializeInstrument(INSTRUMENTHEADER *penv, UINT nsample)
//----------------------------------------------------------------------
{
	memset(penv, 0, sizeof(INSTRUMENTHEADER));
	penv->nFadeOut = 256;
	penv->nGlobalVol = 64;
	penv->nPan = 128;
	penv->nPPC = 5*12;
	m_SndFile.SetDefaultInstrumentValues(penv);
	for (UINT n=0; n<128; n++)
	{
		penv->Keyboard[n] = nsample;
		penv->NoteMap[n] = n+1;
	}
	penv->pTuning = penv->s_DefaultTuning;
}


BOOL CModDoc::RemoveOrder(UINT n)
//-------------------------------
{
	if (n < m_SndFile.Order.size())
	{
		BEGIN_CRITICAL();
		for (UINT i=n; i<m_SndFile.Order.size()-1; i++)
		{
			m_SndFile.Order[i] = m_SndFile.Order[i+1];
		}
		m_SndFile.Order[m_SndFile.Order.size()-1] = m_SndFile.Order.GetInvalidPatIndex();
		END_CRITICAL();
		SetModified();
		return TRUE;
	}
	return FALSE;
}


BOOL CModDoc::RemovePattern(UINT n)
//---------------------------------
{
	if ((n < m_SndFile.Patterns.Size()) && (m_SndFile.Patterns[n]))
	{
		BEGIN_CRITICAL();
		LPVOID p = m_SndFile.Patterns[n];
		m_SndFile.Patterns[n] = NULL;
		CSoundFile::FreePattern(p);
		END_CRITICAL();
		SetModified();
		return TRUE;
	}
	return FALSE;
}


BOOL CModDoc::RemoveSample(UINT n)
//--------------------------------
{
	if ((n) && (n <= m_SndFile.m_nSamples))
	{
		BEGIN_CRITICAL();
		m_SndFile.DestroySample(n);
		m_SndFile.m_szNames[n][0] = 0;
		while ((m_SndFile.m_nSamples > 1)
		 && (!m_SndFile.m_szNames[m_SndFile.m_nSamples][0])
		 && (!m_SndFile.Ins[m_SndFile.m_nSamples].pSample)) m_SndFile.m_nSamples--;
		END_CRITICAL();
		SetModified();
		return TRUE;
	}
	return FALSE;
}


// -> CODE#0020
// -> DESC="rearrange sample list"
void CModDoc::RearrangeSampleList(void)
//-------------------------------------
{
	MessageBox(NULL, "Rearrange samplelist didn't work properly and has been disabled.", NULL, MB_ICONINFORMATION);
	/*
	BEGIN_CRITICAL();
	UINT i,j,k,n,l,c;

	for(n = 1 ; n <= m_SndFile.m_nSamples ; n++){

		if(!m_SndFile.Ins[n].pSample){
			
			k = 1;

			while(n+k <= m_SndFile.m_nSamples && !m_SndFile.Ins[n+k].pSample) k++;
			if(n+k >= m_SndFile.m_nSamples) break;

			c = k;
			l = 0;

			while(n+k <= m_SndFile.m_nSamples && m_SndFile.Ins[n+k].pSample){

				m_SndFile.MoveSample(n+k,n+l);
				strcpy(m_SndFile.m_szNames[n+l], m_SndFile.m_szNames[n+k]);
				m_SndFile.m_szNames[n+k][0] = '\0';

				for(i=1; i<=m_SndFile.m_nInstruments; i++){
					if(m_SndFile.Headers[i]){
						INSTRUMENTHEADER *p = m_SndFile.Headers[i];
						for(j=0; j<128; j++) if(p->Keyboard[j] == n+k) p->Keyboard[j] = n+l;
					}
				}

				k++;
				l++;
			}

			n += l;
			m_SndFile.m_nSamples -= c;
		}
	}

	END_CRITICAL();

	SetModified();
	UpdateAllViews(NULL, HINT_SMPNAMES);
	*/
}
// -! NEW_FEATURE#0020


BOOL CModDoc::RemoveInstrument(UINT n)
//------------------------------------
{
	if ((n) && (n <= m_SndFile.m_nInstruments) && (m_SndFile.Headers[n]))
	{
		BOOL bIns = FALSE;
		BEGIN_CRITICAL();
		m_SndFile.DestroyInstrument(n);
		if (n == m_SndFile.m_nInstruments) m_SndFile.m_nInstruments--;
		for (UINT i=1; i<MAX_INSTRUMENTS; i++) if (m_SndFile.Headers[i]) bIns = TRUE;
		if (!bIns) m_SndFile.m_nInstruments = 0;
		END_CRITICAL();
		SetModified();
		return TRUE;
	}
	return FALSE;
}


BOOL CModDoc::MoveOrder(UINT nSourceNdx, UINT nDestNdx, BOOL bUpdate, BOOL bCopy)
//-------------------------------------------------------------------------------
{
	if ((nSourceNdx >= m_SndFile.Order.size()) || (nDestNdx >= m_SndFile.Order.size())) return FALSE;
	UINT n = m_SndFile.Order[nSourceNdx];
	// Delete source
	if (!bCopy)
	{
		for (UINT i=nSourceNdx; i<m_SndFile.Order.size()-1; i++) m_SndFile.Order[i] = m_SndFile.Order[i+1];
		if (nSourceNdx < nDestNdx) nDestNdx--;
	}
	// Insert at dest
	for (UINT j=m_SndFile.Order.size()-1; j>nDestNdx; j--) m_SndFile.Order[j] = m_SndFile.Order[j-1];
	m_SndFile.Order[nDestNdx] = n;
	if (bUpdate)
	{
		UpdateAllViews(NULL, HINT_MODSEQUENCE, NULL);
	}
	return TRUE;
}


BOOL CModDoc::ExpandPattern(UINT nPattern)
//----------------------------------------
{
// -> CODE#0008
// -> DESC="#define to set pattern size"

	if ((nPattern >= m_SndFile.Patterns.Size()) || (!m_SndFile.Patterns[nPattern])) return FALSE;
	if(m_SndFile.Patterns[nPattern].Expand())
		return FALSE;
	else
		return TRUE;
}


BOOL CModDoc::ShrinkPattern(UINT nPattern)
//----------------------------------------
{
	if ((nPattern >= m_SndFile.Patterns.Size()) || (!m_SndFile.Patterns[nPattern])) return FALSE;
	if(m_SndFile.Patterns[nPattern].Shrink())
		return FALSE;
	else
		return TRUE;
}


// Clipboard format:
// Hdr: "ModPlug Tracker S3M\n"
// Full:  '|C#401v64A06'
// Reset: '|...........'
// Empty: '|           '
// End of row: '\n'

static LPCSTR lpszClipboardPatternHdr = "ModPlug Tracker %3s\x0D\x0A";

BOOL CModDoc::CopyPattern(UINT nPattern, DWORD dwBeginSel, DWORD dwEndSel)
//------------------------------------------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	DWORD dwMemSize;
	HGLOBAL hCpy;
	UINT nrows = (dwEndSel >> 16) - (dwBeginSel >> 16) + 1;
	UINT ncols = ((dwEndSel & 0xFFFF) >> 3) - ((dwBeginSel & 0xFFFF) >> 3) + 1;

	if ((!pMainFrm) || (nPattern >= m_SndFile.Patterns.Size()) || (!m_SndFile.Patterns[nPattern])) return FALSE;
	BeginWaitCursor();
	dwMemSize = strlen(lpszClipboardPatternHdr) + 1;
	dwMemSize += nrows * (ncols * 12 + 2);
	if ((pMainFrm->OpenClipboard()) && ((hCpy = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, dwMemSize))!=NULL))
	{
		LPCSTR pszFormatName;
		EmptyClipboard();
		switch(m_SndFile.m_nType)
		{
		case MOD_TYPE_S3M:	pszFormatName = "S3M"; break;
		case MOD_TYPE_XM:	pszFormatName = "XM"; break;
		case MOD_TYPE_IT:	pszFormatName = "IT"; break;
		case MOD_TYPE_MPT:	pszFormatName = "MPT"; break;
		default:			pszFormatName = "MOD"; break;
		}
		LPSTR p = (LPSTR)GlobalLock(hCpy);
		if (p)
		{
			UINT colmin = dwBeginSel & 0xFFFF;
			UINT colmax = dwEndSel & 0xFFFF;
			wsprintf(p, lpszClipboardPatternHdr, pszFormatName);
			p += strlen(p);
			for (UINT row=0; row<nrows; row++)
			{
				MODCOMMAND *m = m_SndFile.Patterns[nPattern];
				if ((row + (dwBeginSel >> 16)) >= m_SndFile.PatternSize[nPattern]) break;
				m += (row+(dwBeginSel >> 16))*m_SndFile.m_nChannels;
				m += (colmin >> 3);
				for (UINT col=0; col<ncols; col++, m++, p+=12)
				{
					UINT ncursor = ((colmin>>3)+col) << 3;
					p[0] = '|';
					// Note
					if ((ncursor >= colmin) && (ncursor <= colmax))
					{
						UINT note = m->note;
						switch(note)
						{
						case 0:		p[1] = p[2] = p[3] = '.'; break;
						case NOTE_KEYOFF:	p[1] = p[2] = p[3] = '='; break;
						case NOTE_NOTECUT:	p[1] = p[2] = p[3] = '^'; break;
						case NOTE_PC: p[1] = 'P'; p[2] = 'C'; p[3] = ' '; break;
						case NOTE_PCS: p[1] = 'P'; p[2] = 'C'; p[3] = 'S'; break;
						default:
							p[1] = szNoteNames[(note-1) % 12][0];
							p[2] = szNoteNames[(note-1) % 12][1];
							p[3] = '0' + (note-1) / 12;
						}
					} else
					{
						// No note
						p[1] = p[2] = p[3] = ' ';
					}
					// Instrument
					ncursor++;
					if ((ncursor >= colmin) && (ncursor <= colmax))
					{
						if (m->instr)
						{
							p[4] = '0' + (m->instr / 10);
							p[5] = '0' + (m->instr % 10);
						} else p[4] = p[5] = '.';
					} else
					{
						p[4] = p[5] = ' ';
					}
					// Volume
					ncursor++;
					if ((ncursor >= colmin) && (ncursor <= colmax))
					{
						if(m->note == NOTE_PC || m->note == NOTE_PCS)
						{
							const uint16 val = m->GetValueVolCol();
							p[6] = GetDigit<2>(val);
							p[7] = GetDigit<1>(val);
							p[8] = GetDigit<0>(val);
						}
						else
						{
							if ((m->volcmd) && (m->volcmd <= MAX_VOLCMDS))
							{
								p[6] = gszVolCommands[m->volcmd];
								p[7] = '0' + (m->vol / 10);
								p[8] = '0' + (m->vol % 10);
							} else p[6] = p[7] = p[8] = '.';
						}
					} else
					{
						p[6] = p[7] = p[8] = ' ';
					}
					// Effect
					ncursor++;
					if (((ncursor >= colmin) && (ncursor <= colmax))
					 || ((ncursor+1 >= colmin) && (ncursor+1 <= colmax)))
					{
						if(m->note == NOTE_PC || m->note == NOTE_PCS)
						{
							const uint16 val = m->GetValueEffectCol();
							p[9] = GetDigit<2>(val);
							p[10] = GetDigit<1>(val);
							p[11] = GetDigit<0>(val);
						}
						else
						{
							if (m->command)
							{
								if (m_SndFile.m_nType & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT))
									p[9] = gszS3mCommands[m->command];
								else
									p[9] = gszModCommands[m->command];
							} else p[9] = '.';
							if (m->param)
							{
								p[10] = szHexChar[m->param >> 4];
								p[11] = szHexChar[m->param & 0x0F];
							} else p[10] = p[11] = '.';
						}
					} else
					{
						p[9] = p[10] = p[11] = ' ';
					}
				}
				*p++ = 0x0D;
				*p++ = 0x0A;
			}
			*p = 0;
		}
		GlobalUnlock(hCpy);
		SetClipboardData (CF_TEXT, (HANDLE) hCpy);
		CloseClipboard();
	}
	EndWaitCursor();
	return TRUE;
}


//rewbs.mixpaste: using eric's method, as it is fat more elegant.
// -> CODE#0014
// -> DESC="vst wet/dry slider"
//BOOL CModDoc::PastePattern(UINT nPattern, DWORD dwBeginSel)
BOOL CModDoc::PastePattern(UINT nPattern, DWORD dwBeginSel, BOOL mix, BOOL ITStyleMix)
// -! NEW_FEATURE#0014
//---------------------------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if ((!pMainFrm) || (nPattern >= m_SndFile.Patterns.Size()) || (!m_SndFile.Patterns[nPattern])) return FALSE;
	BeginWaitCursor();
	if (pMainFrm->OpenClipboard())
	{
		HGLOBAL hCpy = ::GetClipboardData(CF_TEXT);
		LPSTR p;

		if ((hCpy) && ((p = (LPSTR)GlobalLock(hCpy)) != NULL))
		{
			PrepareUndo(nPattern, 0,0, m_SndFile.m_nChannels, m_SndFile.PatternSize[nPattern]);
			BYTE spdmax = (m_SndFile.m_nType & MOD_TYPE_MOD) ? 0x20 : 0x1F;
			DWORD dwMemSize = GlobalSize(hCpy);
			MODCOMMAND *m = m_SndFile.Patterns[nPattern];
			UINT nrow = dwBeginSel >> 16;
			UINT ncol = (dwBeginSel & 0xFFFF) >> 3;
			UINT col;
			BOOL bS3M = FALSE, bOk = FALSE;
			UINT len = 0;
			MODCOMMAND origModCmd;

			if ((nrow >= m_SndFile.PatternSize[nPattern]) || (ncol >= m_SndFile.m_nChannels)) goto PasteDone;
			m += nrow * m_SndFile.m_nChannels;
			// Search for signature
			for (;;)
			{
				if (len + 11 >= dwMemSize) goto PasteDone;
				char c = p[len++];
				if (!c) goto PasteDone;
				if ((c == 0x0D) && (len > 3))
				{
					//if ((p[len-3] == 'I') || (p[len-4] == 'S')) bS3M = TRUE;
					//				IT?					S3M?				MPT?
					if ((p[len-3] == 'I') || (p[len-4] == 'S') || (p[len-3] == 'P')) bS3M = TRUE;
					break;
				}
			}
			bOk = TRUE;
			while ((nrow < m_SndFile.PatternSize[nPattern]) && (len + 11 < dwMemSize))
			{
				// Search for column separator
				while (p[len] != '|')
				{
					if (len + 11 >= dwMemSize) goto PasteDone;
					if (!p[len]) goto PasteDone;
					len++;
				}
				col = ncol;
				// Paste columns
				while ((p[len] == '|') && (len + 11 < dwMemSize))
				{
					origModCmd = m[col]; // ITSyle mixpaste requires that we keep a copy of the thing we are about to paste on
					                     // so that we can refer back to check if there was anything in e.g. the note column before we pasted.
					LPSTR s = p+len+1;
					if (col < m_SndFile.m_nChannels)
					{
						// Note
						if (s[0] > ' ' && (!mix || ((!ITStyleMix && origModCmd.note==0) || 
												     (ITStyleMix && origModCmd.note==0 && origModCmd.instr==0 && origModCmd.volcmd==0))))
						{
							m[col].note = 0;
							if (s[0] == '=') m[col].note = NOTE_KEYOFF; else
							if (s[0] == '^') m[col].note = NOTE_NOTECUT; else
							if (s[0] == 'P')
							{
								if(s[2] == 'S')
									m[col].note = NOTE_PCS;
								else
									m[col].note = NOTE_PC;
							} else
							if (s[0] != '.')
							{
								for (UINT i=0; i<12; i++)
								{
									if ((s[0] == szNoteNames[i][0])
									 && (s[1] == szNoteNames[i][1])) m[col].note = i+1;
								}
								if (m[col].note) m[col].note += (s[2] - '0') * 12;
							}
						}
						// Instrument
						if (s[3] > ' ' && (!mix || ( (!ITStyleMix && origModCmd.instr==0) || 
												     (ITStyleMix  && origModCmd.note==0 && origModCmd.instr==0 && origModCmd.volcmd==0) ) ))

						{
							if ((s[3] >= '0') && (s[3] <= ('0'+(MAX_SAMPLES/10))))
							{
								m[col].instr = (s[3]-'0') * 10 + (s[4]-'0');
							} else m[col].instr = 0;
						}
						// Volume
						if (s[5] > ' ' && (!mix || ((!ITStyleMix && origModCmd.volcmd==0) || 
												     (ITStyleMix && origModCmd.note==0 && origModCmd.instr==0 && origModCmd.volcmd==0))))

						{
							if (s[5] != '.')
							{
								if(m[col].note == NOTE_PCS || m[col].note == NOTE_PC)
								{
									char val[4];
									memcpy(val, s+5, 3);
									val[3] = 0;
									m[col].SetValueVolCol(ConvertStrTo<uint16>(val));
								}
								else
								{
									m[col].volcmd = 0;
									for (UINT i=1; i<MAX_VOLCMDS; i++)
									{
										if (s[5] == gszVolCommands[i])
										{
											m[col].volcmd = i;
											break;
										}
									}
									m[col].vol = (s[6]-'0')*10 + (s[7]-'0');
								}
							} else m[col].volcmd = m[col].vol = 0;
						}
						
						if(m[col].note == NOTE_PCS || m[col].note == NOTE_PC)
						{
							if(s[8] != '.')
							{
								char val[4];
								memcpy(val, s+8, 3);
								val[3] = 0;
								m[col].SetValueEffectCol(ConvertStrTo<uint16>(val));
							}
						}
						else
						{
							if (s[8] > ' ' && (!mix || ((!ITStyleMix && origModCmd.command==0) || 
														(ITStyleMix && origModCmd.command==0 && origModCmd.param==0))))
							{
								m[col].command = 0;
								if (s[8] != '.')
								{
									LPCSTR psc = (bS3M) ? gszS3mCommands : gszModCommands;
									for (UINT i=1; i<MAX_EFFECTS; i++)
									{
										if ((s[8] == psc[i]) && (psc[i] != '?')) m[col].command = i;
									}
								}
							}
							// Effect value
							if (s[9] > ' ' && (!mix || ((!ITStyleMix && origModCmd.param==0) || 
														(ITStyleMix && origModCmd.command==0 && origModCmd.param==0))))
							{
								m[col].param = 0;
								if (s[9] != '.')
								{
									for (UINT i=0; i<16; i++)
									{
										if (s[9] == szHexChar[i]) m[col].param |= (i<<4);
										if (s[10] == szHexChar[i]) m[col].param |= i;
									}
								}
							}
							// Checking command
							if (m_SndFile.m_nType & (MOD_TYPE_MOD|MOD_TYPE_XM))
							{
								switch (m[col].command)
								{
								case CMD_SPEED:
								case CMD_TEMPO:
									if (!bS3M) m[col].command = (m[col].param <= spdmax) ? CMD_SPEED : CMD_TEMPO;
									else
									{
										if ((m[col].command == CMD_SPEED) && (m[col].param > spdmax)) m[col].param = CMD_TEMPO; else
										if ((m[col].command == CMD_TEMPO) && (m[col].param <= spdmax)) m[col].param = CMD_SPEED;
									}
									break;
								}
							} else
							{
								switch (m[col].command)
								{
								case CMD_SPEED:
								case CMD_TEMPO:
									if (!bS3M) m[col].command = (m[col].param <= spdmax) ? CMD_SPEED : CMD_TEMPO;
									break;
								}
							}
						}
					}
					len += 12;
					col++;
				}
				// Next row
				m += m_SndFile.m_nChannels;
				nrow++;
			}
		PasteDone:
			GlobalUnlock(hCpy);
			if (bOk)
			{
				SetModified();
				UpdateAllViews(NULL, HINT_PATTERNDATA | (nPattern << HINT_SHIFT_PAT), NULL);
			}
		}
		CloseClipboard();
	}
	EndWaitCursor();
	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////////////////
// Copy/Paste envelope

static LPCSTR pszEnvHdr = "Modplug Tracker Envelope\x0D\x0A";
static LPCSTR pszEnvFmt = "%d,%d,%d,%d,%d,%d,%d,%d\x0D\x0A";

BOOL CModDoc::CopyEnvelope(UINT nIns, UINT nEnv)
//----------------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	HANDLE hCpy;
	CHAR s[1024];
	INSTRUMENTHEADER *penv;
	DWORD dwMemSize;
	UINT susBegin, susEnd, loopBegin, loopEnd, bSus, bLoop, bCarry, nPoints, releaseNode;
	WORD *pPoints;
	BYTE *pValues;

	if ((nIns < 1) || (nIns > m_SndFile.m_nInstruments) || (!m_SndFile.Headers[nIns]) || (!pMainFrm)) return FALSE;
	BeginWaitCursor();
	penv = m_SndFile.Headers[nIns];
	switch(nEnv)
	{
	case ENV_PANNING:
		pPoints = penv->PanPoints;
		pValues = penv->PanEnv;
		nPoints = penv->nPanEnv;
		bLoop = (penv->dwFlags & ENV_PANLOOP) ? TRUE : FALSE;
		bSus = (penv->dwFlags & ENV_PANSUSTAIN) ? TRUE : FALSE;
		bCarry = (penv->dwFlags & ENV_PANCARRY) ? TRUE : FALSE;
		susBegin = penv->nPanSustainBegin;
		susEnd = penv->nPanSustainEnd;
		loopBegin = penv->nPanLoopStart;
		loopEnd = penv->nPanLoopEnd;
		releaseNode = penv->nPanEnvReleaseNode;
		break;

	case ENV_PITCH:
		pPoints = penv->PitchPoints;
		pValues = penv->PitchEnv;
		nPoints = penv->nPitchEnv;
		bLoop = (penv->dwFlags & ENV_PITCHLOOP) ? TRUE : FALSE;
		bSus = (penv->dwFlags & ENV_PITCHSUSTAIN) ? TRUE : FALSE;
		bCarry = (penv->dwFlags & ENV_PITCHCARRY) ? TRUE : FALSE;
		susBegin = penv->nPitchSustainBegin;
		susEnd = penv->nPitchSustainEnd;
		loopBegin = penv->nPitchLoopStart;
		loopEnd = penv->nPitchLoopEnd;
		releaseNode = penv->nPitchEnvReleaseNode;
		break;

	default:
		pPoints = penv->VolPoints;
		pValues = penv->VolEnv;
		nPoints = penv->nVolEnv;
		bLoop = (penv->dwFlags & ENV_VOLLOOP) ? TRUE : FALSE;
		bSus = (penv->dwFlags & ENV_VOLSUSTAIN) ? TRUE : FALSE;
		bCarry = (penv->dwFlags & ENV_VOLCARRY) ? TRUE : FALSE;
		susBegin = penv->nVolSustainBegin;
		susEnd = penv->nVolSustainEnd;
		loopBegin = penv->nVolLoopStart;
		loopEnd = penv->nVolLoopEnd;
		releaseNode = penv->nVolEnvReleaseNode;
		break;
	}
	strcpy(s, pszEnvHdr);
	wsprintf(s+strlen(s), pszEnvFmt, nPoints, susBegin, susEnd, loopBegin, loopEnd, bSus, bLoop, bCarry);
	for (UINT i=0; i<nPoints; i++)
	{
		if (strlen(s) >= sizeof(s)-32) break;
		wsprintf(s+strlen(s), "%d,%d\x0D\x0A", pPoints[i], pValues[i]);
	}

	//Writing release node
	if(strlen(s) < sizeof(s) - 32)
		wsprintf(s+strlen(s), "%u\x0D\x0A", releaseNode);

	dwMemSize = strlen(s)+1;
	if ((pMainFrm->OpenClipboard()) && ((hCpy = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, dwMemSize))!=NULL))
	{
		EmptyClipboard();
		LPBYTE p = (LPBYTE)GlobalLock(hCpy);
		memcpy(p, s, dwMemSize);
		GlobalUnlock(hCpy);
		SetClipboardData (CF_TEXT, (HANDLE)hCpy);
		CloseClipboard();
	}
	EndWaitCursor();
	return TRUE;
}


BOOL CModDoc::PasteEnvelope(UINT nIns, UINT nEnv)
//-----------------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();

	if ((nIns < 1) || (nIns > m_SndFile.m_nInstruments) || (!m_SndFile.Headers[nIns]) || (!pMainFrm)) return FALSE;
	BeginWaitCursor();
	if (!pMainFrm->OpenClipboard())
	{
		EndWaitCursor();
		return FALSE;
	}
	HGLOBAL hCpy = ::GetClipboardData(CF_TEXT);
	LPCSTR p;
	if ((hCpy) && ((p = (LPSTR)GlobalLock(hCpy)) != NULL))
	{
		INSTRUMENTHEADER *penv = m_SndFile.Headers[nIns];
		UINT susBegin=0, susEnd=0, loopBegin=0, loopEnd=0, bSus=0, bLoop=0, bCarry=0, nPoints=0, releaseNode = ENV_RELEASE_NODE_UNSET;
		DWORD dwMemSize = GlobalSize(hCpy), dwPos = strlen(pszEnvHdr);
		if ((dwMemSize > dwPos) && (!_strnicmp(p, pszEnvHdr, dwPos-2)))
		{
			for (UINT h=0; h<8; h++)
			{
				while ((dwPos < dwMemSize) && ((p[dwPos] < '0') || (p[dwPos] > '9'))) dwPos++;
				if (dwPos >= dwMemSize) break;
				int n = atoi(p+dwPos);
				switch(h)
				{
				case 0:		nPoints = n; break;
				case 1:		susBegin = n; break;
				case 2:		susEnd = n; break;
				case 3:		loopBegin = n; break;
				case 4:		loopEnd = n; break;
				case 5:		bSus = n; break;
				case 6:		bLoop = n; break;
				case 7:		bCarry = n; break;
				}
				while ((dwPos < dwMemSize) && ((p[dwPos] >= '0') && (p[dwPos] <= '9'))) dwPos++;
			}
			if (nPoints > 31) nPoints = 31;
			if (susEnd >= nPoints) susEnd = 0;
			if (susBegin > susEnd) susBegin = susEnd;
			if (loopEnd >= nPoints) loopEnd = 0;
			if (loopBegin > loopEnd) loopBegin = loopEnd;
			WORD *pPoints;
			BYTE *pValues;
			switch(nEnv)
			{
			case ENV_PANNING:
				pPoints = penv->PanPoints;
				pValues = penv->PanEnv;
				penv->nPanEnv = nPoints;
				penv->dwFlags &= ~(ENV_PANLOOP|ENV_PANSUSTAIN|ENV_PANCARRY);
				if (bLoop) penv->dwFlags |= ENV_PANLOOP;
				if (bSus) penv->dwFlags |= ENV_PANSUSTAIN;
				if (bCarry) penv->dwFlags |= ENV_PANCARRY;
				penv->nPanSustainBegin = susBegin;
				penv->nPanSustainEnd = susEnd;
				penv->nPanLoopStart = loopBegin;
				penv->nPanLoopEnd = loopEnd;
				penv->nPanEnvReleaseNode = releaseNode;
				break;

			case ENV_PITCH:
				pPoints = penv->PitchPoints;
				pValues = penv->PitchEnv;
				penv->nPitchEnv = nPoints;
				penv->dwFlags &= ~(ENV_PITCHLOOP|ENV_PITCHSUSTAIN|ENV_PITCHCARRY);
				if (bLoop) penv->dwFlags |= ENV_PITCHLOOP;
				if (bSus) penv->dwFlags |= ENV_PITCHSUSTAIN;
				if (bCarry) penv->dwFlags |= ENV_PITCHCARRY;
				penv->nPitchSustainBegin = susBegin;
				penv->nPitchSustainEnd = susEnd;
				penv->nPitchLoopStart = loopBegin;
				penv->nPitchLoopEnd = loopEnd;
				penv->nPitchEnvReleaseNode = releaseNode;
				break;

			default:
				pPoints = penv->VolPoints;
				pValues = penv->VolEnv;
				penv->nVolEnv = nPoints;
				penv->dwFlags &= ~(ENV_VOLLOOP|ENV_VOLSUSTAIN|ENV_VOLCARRY);
				if (bLoop) penv->dwFlags |= ENV_VOLLOOP;
				if (bSus) penv->dwFlags |= ENV_VOLSUSTAIN;
				if (bCarry) penv->dwFlags |= ENV_VOLCARRY;
				penv->nVolSustainBegin = susBegin;
				penv->nVolSustainEnd = susEnd;
				penv->nVolLoopStart = loopBegin;
				penv->nVolLoopEnd = loopEnd;
				penv->nVolEnvReleaseNode = releaseNode;
				break;
			}
			int oldn = 0;
			for (UINT i=0; i<nPoints; i++)
			{
				while ((dwPos < dwMemSize) && ((p[dwPos] < '0') || (p[dwPos] > '9'))) dwPos++;
				if (dwPos >= dwMemSize) break;
				int n1 = atoi(p+dwPos);
				while ((dwPos < dwMemSize) && (p[dwPos] != ',')) dwPos++;
				while ((dwPos < dwMemSize) && ((p[dwPos] < '0') || (p[dwPos] > '9'))) dwPos++;
				if (dwPos >= dwMemSize) break;
				int n2 = atoi(p+dwPos);
				if ((n1 < oldn) || (n1 > 0x3FFF)) n1 = oldn+1;
				pPoints[i] = (WORD)n1;
				pValues[i] = (BYTE)n2;
				oldn = n1;
				while ((dwPos < dwMemSize) && (p[dwPos] != 0x0D)) dwPos++;
				if (dwPos >= dwMemSize) break;
			}

			//Read releasenode information.
			if(dwPos < dwMemSize)
			{
				BYTE r = static_cast<BYTE>(atoi(p + dwPos));
				if(r == 0 || r >= nPoints) r = ENV_RELEASE_NODE_UNSET;
				switch(nEnv)
				{
					case ENV_PANNING:
						penv->nPanEnvReleaseNode = r;
					break;

					case ENV_PITCH:
						penv->nPitchEnvReleaseNode = r;
					break;

					default:
						penv->nVolEnvReleaseNode = r;
					break;
				}
			}
		}
		GlobalUnlock(hCpy);
		CloseClipboard();
		SetModified();
		UpdateAllViews(NULL, (nIns << HINT_SHIFT_INS) | HINT_ENVELOPE, NULL);
	}
	EndWaitCursor();
	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////////////////
// Undo Functions

BOOL CModDoc::ClearUndo()
//-----------------------
{
	for (UINT i=0; i<MAX_UNDO_LEVEL; i++)
	{
		if (PatternUndo[i].pbuffer) delete[] PatternUndo[i].pbuffer;
		PatternUndo[i].cx = 0;
		PatternUndo[i].cy = 0;
		PatternUndo[i].pbuffer = NULL;
	}
	return TRUE;
}


BOOL CModDoc::CanUndo()
//---------------------
{
	return (PatternUndo[0].pbuffer) ? TRUE : FALSE;
}


BOOL CModDoc::PrepareUndo(UINT pattern, UINT x, UINT y, UINT cx, UINT cy)
//-----------------------------------------------------------------------
{
	MODCOMMAND *pUndo, *pPattern;
	UINT nRows;
	BOOL bUpdate;

	if ((pattern >= m_SndFile.Patterns.Size()) || (!m_SndFile.Patterns[pattern])) return FALSE;
	nRows = m_SndFile.PatternSize[pattern];
	pPattern = m_SndFile.Patterns[pattern];
	if ((y >= nRows) || (cx < 1) || (cy < 1) || (x >= m_SndFile.m_nChannels)) return FALSE;
	if (y+cy >= nRows) cy = nRows-y;
	if (x+cx >= m_SndFile.m_nChannels) cx = m_SndFile.m_nChannels - x;
	BeginWaitCursor();
	pUndo = new MODCOMMAND[cx*cy];
	if (!pUndo)
	{
		EndWaitCursor();
		return FALSE;
	}
	bUpdate = (PatternUndo[0].pbuffer) ? FALSE : TRUE;
	if (PatternUndo[MAX_UNDO_LEVEL-1].pbuffer)
	{
		delete[] PatternUndo[MAX_UNDO_LEVEL-1].pbuffer;
		PatternUndo[MAX_UNDO_LEVEL-1].pbuffer = NULL;
	}
	for (UINT i=MAX_UNDO_LEVEL-1; i>=1; i--)
	{
		PatternUndo[i] = PatternUndo[i-1];
	}
	PatternUndo[0].pattern = pattern;
	PatternUndo[0].patternsize = m_SndFile.PatternSize[pattern];
	PatternUndo[0].column = x;
	PatternUndo[0].row = y;
	PatternUndo[0].cx = cx;
	PatternUndo[0].cy = cy;
	PatternUndo[0].pbuffer = pUndo;
	pPattern += x + y*m_SndFile.m_nChannels;
	for (UINT iy=0; iy<cy; iy++)
	{
		memcpy(pUndo, pPattern, cx*sizeof(MODCOMMAND));
		pUndo += cx;
		pPattern += m_SndFile.m_nChannels;
	}
	EndWaitCursor();
	if (bUpdate) UpdateAllViews(NULL, HINT_UNDO);
	return TRUE;
}


UINT CModDoc::DoUndo()
//--------------------
{
	MODCOMMAND *pUndo, *pPattern;
	UINT nPattern, nRows;

	if ((!PatternUndo[0].pbuffer) || (PatternUndo[0].pattern >= m_SndFile.Patterns.Size())) return (UINT)-1;
	nPattern = PatternUndo[0].pattern;
	nRows = PatternUndo[0].patternsize;
	if (PatternUndo[0].column + PatternUndo[0].cx <= m_SndFile.m_nChannels)
	{
		if ((!m_SndFile.Patterns[nPattern]) || (m_SndFile.PatternSize[nPattern] < nRows))
		{
			MODCOMMAND *newPattern = CSoundFile::AllocatePattern(nRows, m_SndFile.m_nChannels);
			MODCOMMAND *oldPattern = m_SndFile.Patterns[nPattern];
			if (!newPattern) return (UINT)-1;
			m_SndFile.Patterns[nPattern].SetData(newPattern, nRows);
			if (oldPattern)
			{
				memcpy(newPattern, oldPattern, m_SndFile.m_nChannels*m_SndFile.PatternSize[nPattern]*sizeof(MODCOMMAND));
				CSoundFile::FreePattern(oldPattern);
			}
		}
		pUndo = PatternUndo[0].pbuffer;
		pPattern = m_SndFile.Patterns[nPattern];
		if (!m_SndFile.Patterns[nPattern]) return (UINT)-1;
		pPattern += PatternUndo[0].column + (PatternUndo[0].row * m_SndFile.m_nChannels);
		for (UINT iy=0; iy<PatternUndo[0].cy; iy++)
		{
			memcpy(pPattern, pUndo, PatternUndo[0].cx * sizeof(MODCOMMAND));
			pPattern += m_SndFile.m_nChannels;
			pUndo += PatternUndo[0].cx;
		}
	}		
	delete[] PatternUndo[0].pbuffer;
	for (UINT i=0; i<MAX_UNDO_LEVEL-1; i++)
	{
		PatternUndo[i] = PatternUndo[i+1];
	}
	PatternUndo[MAX_UNDO_LEVEL-1].pbuffer = NULL;
	if (!PatternUndo[0].pbuffer) UpdateAllViews(NULL, HINT_UNDO);
	return nPattern;
}


void CModDoc::CheckUnusedChannels(BOOL mask[MAX_CHANNELS], CHANNELINDEX maxRemoveCount)
//--------------------------------------------------------
{
	// Checking for unused channels
	for (int iRst=m_SndFile.m_nChannels-1; iRst>=0; iRst--) //rewbs.removeChanWindowCleanup
	{
		mask[iRst] = TRUE;
		for (UINT ipat=0; ipat<m_SndFile.Patterns.Size(); ipat++) if (m_SndFile.Patterns[ipat])
		{
			MODCOMMAND *p = m_SndFile.Patterns[ipat] + iRst;
			UINT len = m_SndFile.PatternSize[ipat];
			for (UINT idata=0; idata<len; idata++, p+=m_SndFile.m_nChannels)
			{
				if (*((LPDWORD)p))
				{
					mask[iRst] = FALSE;
					break;
				}
			}
			if (!mask[iRst]) break;
		}
		if (mask[iRst])
		{
			if ((--maxRemoveCount) == 0) break;
		}
	}
}


