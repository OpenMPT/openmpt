#include "stdafx.h"
#include "pattern.h"
#include "patternContainer.h"
#include "mainfrm.h"
#include "moddoc.h"


bool CPattern::Resize(const ROWINDEX newRowCount, const bool showDataLossWarning)
//-------------------------------------------
{
	if(m_ModCommands == NULL)
	{
		//For Mimicing old behavior of setting patternsize before even having the
		//actual pattern allocated.
		m_Rows = newRowCount; 
		return false;
	}


	CSoundFile& sndFile = m_rPatternContainer.GetSoundFile();
	if(sndFile.m_pModDoc == NULL) return true;
	CModDoc& rModDoc = *sndFile.m_pModDoc;
	if(newRowCount > sndFile.GetRowMax() || newRowCount < sndFile.GetRowMin())
		return true;

	if (newRowCount == m_Rows) return false;
	rModDoc.BeginWaitCursor();
	BEGIN_CRITICAL();
	if (newRowCount > m_Rows)
	{
		MODCOMMAND *p = CSoundFile::AllocatePattern(newRowCount, sndFile.m_nChannels);
		if (p)
		{
			memcpy(p, m_ModCommands, sndFile.m_nChannels*m_Rows*sizeof(MODCOMMAND));
			CSoundFile::FreePattern(m_ModCommands);
			m_ModCommands = p;
			m_Rows = newRowCount;
		}
	} else
	{
		BOOL bOk = TRUE;
		MODCOMMAND *p = m_ModCommands;
		UINT ndif = (m_Rows - newRowCount) * sndFile.m_nChannels;
		UINT npos = newRowCount * sndFile.m_nChannels;
		if(showDataLossWarning)
		{
			for (UINT i=0; i<ndif; i++)
			{
				if (*((DWORD *)(p+i+npos)))
				{
					bOk = FALSE;
					break;
				}
			}
			if (!bOk && showDataLossWarning)
			{
				END_CRITICAL();
				rModDoc.EndWaitCursor();
				if (CMainFrame::GetMainFrame()->MessageBox("Data at the end of the pattern will be lost.\nDo you want to continue",
									"Shrink Pattern", MB_YESNO|MB_ICONQUESTION) == IDYES) bOk = TRUE;
				rModDoc.BeginWaitCursor();
				BEGIN_CRITICAL();
			}
		}
		if (bOk)
		{
			MODCOMMAND *pnew = CSoundFile::AllocatePattern(newRowCount, sndFile.m_nChannels);
			if (pnew)
			{
				memcpy(pnew, m_ModCommands, sndFile.m_nChannels*newRowCount*sizeof(MODCOMMAND));
				CSoundFile::FreePattern(m_ModCommands);
				m_ModCommands = pnew;
				m_Rows = newRowCount;
			}
		}
	}
	END_CRITICAL();
	rModDoc.EndWaitCursor();
	rModDoc.SetModified();
	return (newRowCount == m_Rows) ? false : true;
}


bool CPattern::ClearData()
//------------------------
{
	BEGIN_CRITICAL();
	m_Rows = 0;
	CSoundFile::FreePattern(m_ModCommands);
	m_ModCommands = NULL;
	END_CRITICAL();
	return false;
}

bool CPattern::Expand()
//---------------------
{
	MODCOMMAND *newPattern, *oldPattern;
	UINT nRows, nChns;

	CSoundFile& sndFile = m_rPatternContainer.GetSoundFile();
	if(sndFile.m_pModDoc == NULL) return true;

	CModDoc& rModDoc = *sndFile.m_pModDoc;

	if ((!m_ModCommands) || (m_Rows > sndFile.GetRowMax() / 2)) return true;

	rModDoc.BeginWaitCursor();
	nRows = m_Rows;
	nChns = sndFile.m_nChannels;
	newPattern = CSoundFile::AllocatePattern(nRows*2, nChns);
	if (!newPattern) return true;

	const UINT nPattern = m_rPatternContainer.GetIndex(this);
	rModDoc.PrepareUndo(nPattern, 0,0, nChns, nRows);
	oldPattern = m_ModCommands;
	for (UINT y=0; y<nRows; y++)
	{
		memcpy(newPattern+y*2*nChns, oldPattern+y*nChns, nChns*sizeof(MODCOMMAND));
	}
	m_ModCommands = newPattern;
	m_Rows = nRows * 2;
	CSoundFile::FreePattern(oldPattern); oldPattern= NULL;
	rModDoc.SetModified();
	rModDoc.UpdateAllViews(NULL, HINT_PATTERNDATA | (nPattern << 24), NULL);
	rModDoc.EndWaitCursor();
	return false;
}

bool CPattern::Shrink()
//---------------------
{
	UINT nRows, nChns;

	if (!m_ModCommands || m_Rows < 32) return true;

	CSoundFile& sndFile = m_rPatternContainer.GetSoundFile();
	if(sndFile.m_pModDoc == NULL) return true;

	CModDoc& rModDoc = *sndFile.m_pModDoc;

	rModDoc.BeginWaitCursor();
	nRows = m_Rows;
	nChns = sndFile.m_nChannels;
	const UINT nPattern = m_rPatternContainer.GetIndex(this);
	rModDoc.PrepareUndo(nPattern, 0,0, nChns, nRows);
	nRows /= 2;
	for (UINT y=0; y<nRows; y++)
	{
		MODCOMMAND *psrc = sndFile.Patterns[nPattern] + (y * 2 * nChns);
		MODCOMMAND *pdest = sndFile.Patterns[nPattern] + (y * nChns);
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
	m_Rows = nRows;
	rModDoc.SetModified();
	rModDoc.UpdateAllViews(NULL, HINT_PATTERNDATA | (nPattern << 24), NULL);
	rModDoc.EndWaitCursor();
	return false;
}

