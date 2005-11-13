// modedit.cpp : CModDoc operations
//

#include "stdafx.h"
#include "mptrack.h"
#include "mainfrm.h"
#include "moddoc.h"
#include "dlg_misc.h"
#include "dlsbank.h"

#pragma warning(disable:4244)

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//////////////////////////////////////////////////////////////////////
// Module type conversion

BOOL CModDoc::ChangeModType(UINT nNewType)
//----------------------------------------
{
	CHAR s[256];
	UINT b64 = 0;
	
	if (nNewType == m_SndFile.m_nType){
		// Even if m_nType doesn't change, we might need to change extension in itp<->it case.
		// This is because ITP is a HACK and doesn't genuinely change m_nType,
		// but uses flages instead.
		ChangeFileExtension(nNewType);
		return TRUE;
	}


	// Check if conversion to 64 rows is necessary
	for (UINT ipat=0; ipat<MAX_PATTERNS; ipat++)
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
		for (UINT i=0; i<MAX_PATTERNS; i++) if ((m_SndFile.Patterns[i]) && (m_SndFile.PatternSize[i] != 64))
		{
			if (m_SndFile.PatternSize[i] < 64)
			{
				MODCOMMAND *p = CSoundFile::AllocatePattern(64, m_SndFile.m_nChannels);
				if (p)
				{
					memcpy(p, m_SndFile.Patterns[i], m_SndFile.m_nChannels*m_SndFile.PatternSize[i]*sizeof(MODCOMMAND));
					CSoundFile::FreePattern(m_SndFile.Patterns[i]);
					m_SndFile.Patterns[i] = p;
					m_SndFile.PatternSize[i] = 64;
				} else AddToLog("ERROR: Not enough memory to resize pattern!\n");
			} else
			{
				MODCOMMAND *pnew = CSoundFile::AllocatePattern(64, m_SndFile.m_nChannels);
				if (pnew)
				{
					memcpy(pnew, m_SndFile.Patterns[i], m_SndFile.m_nChannels*64*sizeof(MODCOMMAND));
					CSoundFile::FreePattern(m_SndFile.Patterns[i]);
					m_SndFile.Patterns[i] = pnew;
				}
				m_SndFile.PatternSize[i] = 64;
			}
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
	}
	BeginWaitCursor();
	// Adjust pattern data
	if ((m_SndFile.m_nType & (MOD_TYPE_MOD|MOD_TYPE_XM)) && (nNewType & (MOD_TYPE_S3M|MOD_TYPE_IT)))
	{
		for (UINT nPat=0; nPat<MAX_PATTERNS; nPat++) if (m_SndFile.Patterns[nPat])
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
	if ((m_SndFile.m_nType & (MOD_TYPE_S3M|MOD_TYPE_IT)) && (nNewType & (MOD_TYPE_MOD|MOD_TYPE_XM)))
	{
		for (UINT nPat=0; nPat<MAX_PATTERNS; nPat++) if (m_SndFile.Patterns[nPat])
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
	if ((m_SndFile.m_nType == MOD_TYPE_XM) && (nNewType == MOD_TYPE_MOD))
	{
	} else
	// Convert MOD to XM
	if ((m_SndFile.m_nType == MOD_TYPE_MOD) && (nNewType == MOD_TYPE_XM))
	{
	} else
	// Convert XM to S3M/IT
	if ((m_SndFile.m_nType == MOD_TYPE_XM) && (nNewType != MOD_TYPE_XM))
	{
		for (UINT i=1; i<=m_SndFile.m_nSamples; i++)
		{
			m_SndFile.Ins[i].nC4Speed = CSoundFile::TransposeToFrequency(m_SndFile.Ins[i].RelativeTone, m_SndFile.Ins[i].nFineTune);
			m_SndFile.Ins[i].RelativeTone = 0;
			m_SndFile.Ins[i].nFineTune = 0;
		}
		if (nNewType & MOD_TYPE_IT) m_SndFile.m_dwSongFlags |= SONG_ITCOMPATMODE;
	} else
	// Convert S3M/IT to XM
	if ((m_SndFile.m_nType != MOD_TYPE_XM) && (nNewType == MOD_TYPE_XM))
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
				for (UINT k=0; k<120; k++)
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
	if ((nNewType == MOD_TYPE_MOD) && (m_SndFile.m_nSamples > 31))
	{
		AddToLog("WARNING: Samples above 31 will be lost when saving this file as MOD!\n");
	}
	BEGIN_CRITICAL();
	m_SndFile.m_nType = nNewType;
	if ((!(nNewType & (MOD_TYPE_IT|MOD_TYPE_XM))) && (m_SndFile.m_dwSongFlags & SONG_LINEARSLIDES))
	{
		AddToLog("WARNING: Linear Frequency Slides not supported by the new format.\n");
		m_SndFile.m_dwSongFlags &= ~SONG_LINEARSLIDES;
	}
	if (nNewType != MOD_TYPE_IT) m_SndFile.m_dwSongFlags &= ~(SONG_ITOLDEFFECTS|SONG_ITCOMPATMODE);
	if (nNewType != MOD_TYPE_S3M) m_SndFile.m_dwSongFlags &= ~SONG_FASTVOLSLIDES;
	END_CRITICAL();
	ChangeFileExtension(nNewType);

	//rewbs.cutomKeys: update effect key commands
	CInputHandler *ih = CMainFrame::GetMainFrame()->GetInputHandler();
	if	(nNewType & (MOD_TYPE_MOD|MOD_TYPE_XM)) {
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
BOOL CModDoc::ChangeNumChannels(UINT nNewChannels)
//------------------------------------------------
{
	if (nNewChannels == m_SndFile.m_nChannels) return TRUE;
	if (nNewChannels < m_SndFile.m_nChannels)
	{
		UINT nChnToRemove = m_SndFile.m_nChannels - nNewChannels;
		CRemoveChannelsDlg rem(&m_SndFile, nChnToRemove);

		// Checking for unused channels
		UINT nFound = nChnToRemove;
		for (int iRst=m_SndFile.m_nChannels-1; iRst>=0; iRst--) //rewbs.removeChanWindowCleanup
		{
			rem.m_bChnMask[iRst] = TRUE;
			for (UINT ipat=0; ipat<MAX_PATTERNS; ipat++) if (m_SndFile.Patterns[ipat])
			{
				MODCOMMAND *p = m_SndFile.Patterns[ipat] + iRst;
				UINT len = m_SndFile.PatternSize[ipat];
				for (UINT idata=0; idata<len; idata++, p+=m_SndFile.m_nChannels)
				{
					if (*((LPDWORD)p))
					{
						rem.m_bChnMask[iRst] = FALSE;
						break;
					}
				}
				if (!rem.m_bChnMask[iRst]) break;
			}
			if (rem.m_bChnMask[iRst])
			{
				if ((--nFound) == 0) break;
			}
		}
		if (rem.DoModal() != IDOK) return TRUE;
		// Removing selected channels
		BeginWaitCursor();
		BEGIN_CRITICAL();
		for (UINT i=0; i<MAX_PATTERNS; i++) if (m_SndFile.Patterns[i])
		{
			MODCOMMAND *p = m_SndFile.Patterns[i];
			MODCOMMAND *newp = CSoundFile::AllocatePattern(m_SndFile.PatternSize[i], nNewChannels);
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
					if (!rem.m_bChnMask[k]) *tmpdest++ = *tmpsrc;
				}
			}
			m_SndFile.Patterns[i] = newp;
			CSoundFile::FreePattern(p);
		}
		UINT tmpchn = 0;
		for (i=0; i<m_SndFile.m_nChannels; i++)
		{
			if (!rem.m_bChnMask[i])
			{
				if (tmpchn != i)
				{
					m_SndFile.ChnSettings[tmpchn] = m_SndFile.ChnSettings[i];
					m_SndFile.Chn[tmpchn] = m_SndFile.Chn[i];
				}
				tmpchn++;
				if (i >= nNewChannels) m_SndFile.Chn[i].dwFlags |= CHN_MUTE;
			}
		}
		m_SndFile.m_nChannels = nNewChannels;
		END_CRITICAL();
		EndWaitCursor();
	} else
	{
		BeginWaitCursor();
		// Increasing number of channels
		BEGIN_CRITICAL();
		for (UINT i=0; i<MAX_PATTERNS; i++) if (m_SndFile.Patterns[i])
		{
			MODCOMMAND *p = m_SndFile.Patterns[i];
			MODCOMMAND *newp = CSoundFile::AllocatePattern(m_SndFile.PatternSize[i], nNewChannels);
			if (!newp)
			{
				END_CRITICAL();
				AddToLog("ERROR: Not enough memory to create new channels!\nPattern Data is corrupted!!!\n");
				return TRUE;
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
	return FALSE;
}


BOOL CModDoc::ResizePattern(UINT nPattern, UINT nRows)
//----------------------------------------------------
{
// -> CODE#0008
// -> DESC="#define to set pattern size"
//	if ((nPattern >= MAX_PATTERNS) || (nRows < 2) || (nRows > 256)) return FALSE;
	if ((nPattern >= MAX_PATTERNS) || (nRows < 2) || (nRows > MAX_PATTERN_ROWS)) return FALSE;
// -! BEHAVIOUR_CHANGE#0008
	if (m_SndFile.m_nType & (MOD_TYPE_MOD|MOD_TYPE_S3M)) nRows = 64;
	if (nRows == m_SndFile.PatternSize[nPattern]) return TRUE;
	BeginWaitCursor();
	BEGIN_CRITICAL();
	if (!m_SndFile.Patterns[nPattern])
	{
		m_SndFile.Patterns[nPattern] = CSoundFile::AllocatePattern(nRows, m_SndFile.m_nChannels);
		m_SndFile.PatternSize[nPattern] = nRows;
	} else
	if (nRows > m_SndFile.PatternSize[nPattern])
	{
		MODCOMMAND *p = CSoundFile::AllocatePattern(nRows, m_SndFile.m_nChannels);
		if (p)
		{
			memcpy(p, m_SndFile.Patterns[nPattern], m_SndFile.m_nChannels*m_SndFile.PatternSize[nPattern]*sizeof(MODCOMMAND));
			CSoundFile::FreePattern(m_SndFile.Patterns[nPattern]);
			m_SndFile.Patterns[nPattern] = p;
			m_SndFile.PatternSize[nPattern] = nRows;
		}
	} else
	{
		BOOL bOk = TRUE;
		MODCOMMAND *p = m_SndFile.Patterns[nPattern];
		UINT ndif = (m_SndFile.PatternSize[nPattern] - nRows) * m_SndFile.m_nChannels;
		UINT npos = nRows * m_SndFile.m_nChannels;
		for (UINT i=0; i<ndif; i++)
		{
			if (*((DWORD *)(p+i+npos)))
			{
				bOk = FALSE;
				break;
			}
		}
		if (!bOk)
		{
			END_CRITICAL();
			EndWaitCursor();
			if (CMainFrame::GetMainFrame()->MessageBox("Data at the end of the pattern will be lost.\nDo you want to continue",
								"Shrink Pattern", MB_YESNO|MB_ICONQUESTION) == IDYES) bOk = TRUE;
			BeginWaitCursor();
			BEGIN_CRITICAL();
		}
		if (bOk)
		{
			MODCOMMAND *pnew = CSoundFile::AllocatePattern(nRows, m_SndFile.m_nChannels);
			if (pnew)
			{
				memcpy(pnew, m_SndFile.Patterns[nPattern], m_SndFile.m_nChannels*nRows*sizeof(MODCOMMAND));
				CSoundFile::FreePattern(m_SndFile.Patterns[nPattern]);
				m_SndFile.Patterns[nPattern] = pnew;
				m_SndFile.PatternSize[nPattern] = nRows;
			}
		}
	}
	END_CRITICAL();
	EndWaitCursor();
	SetModified();
	return (nRows == m_SndFile.PatternSize[nPattern]) ? TRUE : FALSE;
}


BOOL CModDoc::RemoveUnusedPatterns(BOOL bRemove)
//----------------------------------------------
{
	UINT nPatMap[MAX_PATTERNS];
	UINT nPatRows[MAX_PATTERNS];
	MODCOMMAND *pPatterns[MAX_PATTERNS];
	BOOL bPatUsed[MAX_PATTERNS];
	CHAR s[512];
	BOOL bEnd = FALSE, bReordered = FALSE;
	UINT nPatRemoved = 0, nMinToRemove, nPats;
	
	BeginWaitCursor();
	memset(bPatUsed, 0, sizeof(bPatUsed));
	UINT maxpat = 0;
	for (UINT iord=0; iord<MAX_ORDERS; iord++)
	{
		UINT n = m_SndFile.Order[iord];
		if (n < MAX_PATTERNS)
		{
			if (n >= maxpat) maxpat = n+1;
			if (!bEnd) bPatUsed[n] = TRUE;
		} else if (n == 0xFF) bEnd = TRUE;
	}
	nMinToRemove = 0;
	if (!bRemove)
	{
		UINT imax=MAX_PATTERNS;
		while (imax > 0)
		{
			imax--;
			if ((m_SndFile.Patterns[imax]) && (bPatUsed[imax])) break;
		}
		nMinToRemove = imax+1;
	}
	for (UINT ipat=maxpat; ipat<MAX_PATTERNS; ipat++) if ((m_SndFile.Patterns[ipat]) && (ipat >= nMinToRemove))
	{
		MODCOMMAND *m = m_SndFile.Patterns[ipat];
		UINT ncmd = m_SndFile.m_nChannels * m_SndFile.PatternSize[ipat];
		for (UINT i=0; i<ncmd; i++)
		{
			if ((m[i].note) || (m[i].instr) || (m[i].volcmd) || (m[i].command)) goto NotEmpty;
		}
		BEGIN_CRITICAL();
		m_SndFile.PatternSize[ipat] = 0;
		m_SndFile.FreePattern(m_SndFile.Patterns[ipat]);
		m_SndFile.Patterns[ipat] = NULL;
		nPatRemoved++;
		END_CRITICAL();
	NotEmpty:
		;
	}
	UINT bWaste = 0;
	for (UINT ichk=0; ichk<MAX_PATTERNS; ichk++)
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
	memset(nPatRows, 0, sizeof(nPatRows));
	memset(pPatterns, 0, sizeof(pPatterns));
	for (UINT irst=0; irst<MAX_PATTERNS; irst++) nPatMap[irst] = 0xFFFF;
	nPats = 0;
	for (UINT imap=0; imap<MAX_ORDERS; imap++)
	{
		UINT n = m_SndFile.Order[imap];
		if (n < MAX_PATTERNS)
		{
			if (nPatMap[n] > MAX_PATTERNS) nPatMap[n] = nPats++;
			m_SndFile.Order[imap] = nPatMap[n];
		} else if (n == 0xFF) break;
	}
	// Add unused patterns at the end
	if ((!bRemove) || (!bWaste))
	{
		for (UINT iadd=0; iadd<MAX_PATTERNS; iadd++)
		{
			if ((m_SndFile.Patterns[iadd]) && (nPatMap[iadd] >= MAX_PATTERNS))
			{
				nPatMap[iadd] = nPats++;
			}
		}
	}
	while (imap < MAX_ORDERS)
	{
		m_SndFile.Order[imap++] = 0xFF;
	}
	BEGIN_CRITICAL();
	// Reorder patterns & Delete unused patterns
	{
		UINT npatnames = m_SndFile.m_nPatternNames;
		LPSTR lpszpatnames = m_SndFile.m_lpszPatternNames;
		m_SndFile.m_nPatternNames = 0;
		m_SndFile.m_lpszPatternNames = NULL;
		for (UINT i=0; i<MAX_PATTERNS; i++)
		{
			UINT k = nPatMap[i];
			if (k < MAX_PATTERNS)
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
				m_SndFile.FreePattern(m_SndFile.Patterns[i]);
				m_SndFile.Patterns[i] = NULL;
				m_SndFile.PatternSize[i] = 0;
				nPatRemoved++;
			}
		}
		for (UINT j=0; j<MAX_PATTERNS;j++)
		{
			m_SndFile.PatternSize[j] = nPatRows[j];
			m_SndFile.Patterns[j] = pPatterns[j];
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
	for (UINT i=0; i<MAX_PATTERNS; i++) if (m_SndFile.Patterns[i])
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
		for (UINT ipat=0; ipat<MAX_PATTERNS; ipat++)
		{
			MODCOMMAND *p = m_SndFile.Patterns[ipat];
			if (p)
			{
				UINT jmax = m_SndFile.PatternSize[ipat] * m_SndFile.m_nChannels;
				for (UINT j=0; j<jmax; j++, p++)
				{
					if ((p->note) && (p->note <= 120))
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
	if (nExt &&  !((m_SndFile.m_nType==MOD_TYPE_IT) && (m_SndFile.m_dwSongFlags&SONG_ITPROJECT)))
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
	if ( !((m_SndFile.m_nType==MOD_TYPE_IT) && (m_SndFile.m_dwSongFlags&SONG_ITPROJECT))) { //never remove an instrument's samples in ITP.
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
	 && (::MessageBox(NULL, "Do you want to reorganize the remaining instruments ?", NULL, MB_YESNO | MB_ICONQUESTION) == IDOK))
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
			for (UINT iPat=0; iPat<MAX_PATTERNS; iPat++) if (m_SndFile.Patterns[iPat])
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
	UINT maxpat = (m_SndFile.m_nType & MOD_TYPE_MOD) ? 128 : MAX_PATTERNS;
	for (UINT i=0; i<maxpat; i++)
	{
		if (!m_SndFile.Patterns[i]) break;
	}
	if (i >= maxpat)
	{
		ErrorBox(IDS_ERR_TOOMANYPAT, CMainFrame::GetMainFrame());
		return -1;
	}
	for (UINT j=0; j<MAX_ORDERS; j++)
	{
		if (m_SndFile.Order[j] == i) break;
		if (m_SndFile.Order[j] == 0xFF)
		{
			m_SndFile.Order[j] = i;
			break;
		}
		if ((nOrd >= 0) && (j == (UINT)nOrd))
		{
			for (UINT k=MAX_ORDERS-1; k>j; k--)
			{
				m_SndFile.Order[k] = m_SndFile.Order[k-1];
			}
			m_SndFile.Order[j] = i;
			break;
		}
	}
	if (nRows < 4) nRows = 64;
	if (!m_SndFile.Patterns[i])
	{
		m_SndFile.PatternSize[i] = nRows;
		m_SndFile.Patterns[i] = CSoundFile::AllocatePattern(nRows, m_SndFile.m_nChannels);
		SetModified();
	}
	return i;
}


LONG CModDoc::InsertSample(BOOL bLimit)
//-------------------------------------
{
	for (UINT i=1; i<=m_SndFile.m_nSamples; i++)
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
	if ((m_SndFile.m_nType != MOD_TYPE_XM) && (m_SndFile.m_nType != MOD_TYPE_IT)) return -1;
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
	penv->nResampling = SRCMODE_DEFAULT;
	penv->nFilterMode = FLTMODE_UNCHANGED;
	for (UINT n=0; n<128; n++)
	{
		penv->Keyboard[n] = nsample;
		penv->NoteMap[n] = n+1;
	}
}


BOOL CModDoc::RemoveOrder(UINT n)
//-------------------------------
{
	if (n < MAX_ORDERS)
	{
		BEGIN_CRITICAL();
		for (UINT i=n; i<MAX_ORDERS-1; i++)
		{
			m_SndFile.Order[i] = m_SndFile.Order[i+1];
		}
		m_SndFile.Order[MAX_ORDERS-1] = 0xFF;
		END_CRITICAL();
		SetModified();
		return TRUE;
	}
	return FALSE;
}


BOOL CModDoc::RemovePattern(UINT n)
//---------------------------------
{
	if ((n < MAX_PATTERNS) && (m_SndFile.Patterns[n]))
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
{
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
	if ((nSourceNdx >= MAX_ORDERS) || (nDestNdx >= MAX_ORDERS)) return FALSE;
	UINT n = m_SndFile.Order[nSourceNdx];
	// Delete source
	if (!bCopy)
	{
		for (UINT i=nSourceNdx; i<MAX_ORDERS-1; i++) m_SndFile.Order[i] = m_SndFile.Order[i+1];
		if (nSourceNdx < nDestNdx) nDestNdx--;
	}
	// Insert at dest
	for (UINT j=MAX_ORDERS-1; j>nDestNdx; j--) m_SndFile.Order[j] = m_SndFile.Order[j-1];
	m_SndFile.Order[nDestNdx] = (BYTE)n;
	if (bUpdate)
	{
		UpdateAllViews(NULL, HINT_MODSEQUENCE, NULL);
	}
	return TRUE;
}


BOOL CModDoc::ExpandPattern(UINT nPattern)
//----------------------------------------
{
	MODCOMMAND *newPattern, *oldPattern;
	UINT nRows, nChns;

// -> CODE#0008
// -> DESC="#define to set pattern size"
//	if ((nPattern >= MAX_PATTERNS) || (!m_SndFile.Patterns[nPattern]) || (m_SndFile.PatternSize[nPattern] > 128)) return FALSE;
	if ((nPattern >= MAX_PATTERNS) || (!m_SndFile.Patterns[nPattern]) || (m_SndFile.PatternSize[nPattern] > MAX_PATTERN_ROWS / 2)) return FALSE;
// -! BEHAVIOUR_CHANGE#0008
	BeginWaitCursor();
	nRows = m_SndFile.PatternSize[nPattern];
	nChns = m_SndFile.m_nChannels;
	newPattern = CSoundFile::AllocatePattern(nRows*2, nChns);
	if (!newPattern) return FALSE;
	PrepareUndo(nPattern, 0,0, nChns, nRows);
	oldPattern = m_SndFile.Patterns[nPattern];
	for (UINT y=0; y<nRows; y++)
	{
		memcpy(newPattern+y*2*nChns, oldPattern+y*nChns, nChns*sizeof(MODCOMMAND));
	}
	m_SndFile.Patterns[nPattern] = newPattern;
	m_SndFile.PatternSize[nPattern] = nRows * 2;
	SetModified();
	UpdateAllViews(NULL, HINT_PATTERNDATA | (nPattern << 24), NULL);
	EndWaitCursor();
	return TRUE;
}


BOOL CModDoc::ShrinkPattern(UINT nPattern)
//----------------------------------------
{
	UINT nRows, nChns;

	if ((nPattern >= MAX_PATTERNS) || (!m_SndFile.Patterns[nPattern]) || (m_SndFile.PatternSize[nPattern] < 32)) return FALSE;
	BeginWaitCursor();
	nRows = m_SndFile.PatternSize[nPattern];
	nChns = m_SndFile.m_nChannels;
	PrepareUndo(nPattern, 0,0, nChns, nRows);
	nRows /= 2;
	for (UINT y=0; y<nRows; y++)
	{
		MODCOMMAND *psrc = m_SndFile.Patterns[nPattern] + (y * 2 * nChns);
		MODCOMMAND *pdest = m_SndFile.Patterns[nPattern] + (y * nChns);
		for (UINT x=0; x<nChns; x++)
		{
			pdest[x] = psrc[x];
			if ((!pdest[x].note) && (!pdest[x].instr))
			{
				pdest[x].note = psrc[x+nChns].note;
				pdest[x].instr = psrc[x+nChns].instr;
				if (psrc[x+nChns].volcmd)
				{
					pdest[x].volcmd = psrc[x+nChns].volcmd;
					pdest[x].vol = psrc[x+nChns].vol;
				}
				if (!pdest[x].command)
				{
					pdest[x].command = psrc[x+nChns].command;
					pdest[x].param = psrc[x+nChns].param;
				}
			}
		}
	}
	m_SndFile.PatternSize[nPattern] = nRows;
	SetModified();
	UpdateAllViews(NULL, HINT_PATTERNDATA | (nPattern << 24), NULL);
	EndWaitCursor();
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

	if ((!pMainFrm) || (nPattern >= MAX_PATTERNS) || (!m_SndFile.Patterns[nPattern])) return FALSE;
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
						case 0xFF:	p[1] = p[2] = p[3] = '='; break;
						case 0xFE:	p[1] = p[2] = p[3] = '^'; break;
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
						if ((m->volcmd) && (m->volcmd <= MAX_VOLCMDS))
						{
							p[6] = gszVolCommands[m->volcmd];
							p[7] = '0' + (m->vol / 10);
							p[8] = '0' + (m->vol % 10);
						} else p[6] = p[7] = p[8] = '.';
					} else
					{
						p[6] = p[7] = p[8] = ' ';
					}
					// Effect
					ncursor++;
					if (((ncursor >= colmin) && (ncursor <= colmax))
					 || ((ncursor+1 >= colmin) && (ncursor+1 <= colmax)))
					{
						if (m->command)
						{
							if (m_SndFile.m_nType & (MOD_TYPE_S3M|MOD_TYPE_IT))
								p[9] = gszS3mCommands[m->command];
							else
								p[9] = gszModCommands[m->command];
						} else p[9] = '.';
						if (m->param)
						{
							p[10] = szHexChar[m->param >> 4];
							p[11] = szHexChar[m->param & 0x0F];
						} else p[10] = p[11] = '.';
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
	if ((!pMainFrm) || (nPattern >= MAX_PATTERNS) || (!m_SndFile.Patterns[nPattern])) return FALSE;
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
					if ((p[len-3] == 'I') || (p[len-4] == 'S')) bS3M = TRUE;
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
							if (s[0] == '=') m[col].note = 0xFF; else
							if (s[0] == '^') m[col].note = 0xFE; else
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
							} else m[col].volcmd = m[col].vol = 0;
						}
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
				UpdateAllViews(NULL, HINT_PATTERNDATA | (nPattern << 24), NULL);
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
	UINT susBegin, susEnd, loopBegin, loopEnd, bSus, bLoop, bCarry, nPoints;
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
		break;
	}
	strcpy(s, pszEnvHdr);
	wsprintf(s+strlen(s), pszEnvFmt, nPoints, susBegin, susEnd, loopBegin, loopEnd, bSus, bLoop, bCarry);
	for (UINT i=0; i<nPoints; i++)
	{
		if (strlen(s) >= sizeof(s)-32) break;
		wsprintf(s+strlen(s), "%d,%d\x0D\x0A", pPoints[i], pValues[i]);
	}
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
		UINT susBegin=0, susEnd=0, loopBegin=0, loopEnd=0, bSus=0, bLoop=0, bCarry=0, nPoints=0;
		DWORD dwMemSize = GlobalSize(hCpy), dwPos = strlen(pszEnvHdr);
		if ((dwMemSize > dwPos) && (!strnicmp(p, pszEnvHdr, dwPos-2)))
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
		}
		GlobalUnlock(hCpy);
		CloseClipboard();
		SetModified();
		UpdateAllViews(NULL, (nIns << 24) | HINT_ENVELOPE, NULL);
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
		if (PatternUndo[i].pbuffer) delete PatternUndo[i].pbuffer;
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

	if ((pattern >= MAX_PATTERNS) || (!m_SndFile.Patterns[pattern])) return FALSE;
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
		delete PatternUndo[MAX_UNDO_LEVEL-1].pbuffer;
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

	if ((!PatternUndo[0].pbuffer) || (PatternUndo[0].pattern >= MAX_PATTERNS)) return (UINT)-1;
	nPattern = PatternUndo[0].pattern;
	nRows = PatternUndo[0].patternsize;
	if (PatternUndo[0].column + PatternUndo[0].cx <= m_SndFile.m_nChannels)
	{
		if ((!m_SndFile.Patterns[nPattern]) || (m_SndFile.PatternSize[nPattern] < nRows))
		{
			MODCOMMAND *newPattern = CSoundFile::AllocatePattern(nRows, m_SndFile.m_nChannels);
			MODCOMMAND *oldPattern = m_SndFile.Patterns[nPattern];
			if (!newPattern) return (UINT)-1;
			m_SndFile.Patterns[nPattern] = newPattern;
			if (oldPattern)
			{
				memcpy(newPattern, oldPattern, m_SndFile.m_nChannels*m_SndFile.PatternSize[nPattern]*sizeof(MODCOMMAND));
				CSoundFile::FreePattern(oldPattern);
			}
			m_SndFile.PatternSize[nPattern] = nRows;
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
	delete PatternUndo[0].pbuffer;
	for (UINT i=0; i<MAX_UNDO_LEVEL-1; i++)
	{
		PatternUndo[i] = PatternUndo[i+1];
	}
	PatternUndo[MAX_UNDO_LEVEL-1].pbuffer = NULL;
	if (!PatternUndo[0].pbuffer) UpdateAllViews(NULL, HINT_UNDO);
	return nPattern;
}

