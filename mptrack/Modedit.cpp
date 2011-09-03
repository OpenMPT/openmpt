// modedit.cpp : CModDoc operations
//

#include "stdafx.h"
#include "mptrack.h"
#include "mainfrm.h"
#include "moddoc.h"
#include "dlg_misc.h"
#include "dlsbank.h"
#include "modsmp_ctrl.h"
#include "../common/misc_util.h"

#pragma warning(disable:4244) //"conversion from 'type1' to 'type2', possible loss of data"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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


// Change the number of channels.
// Return true on success.
bool CModDoc::ChangeNumChannels(CHANNELINDEX nNewChannels, const bool showCancelInRemoveDlg)
//------------------------------------------------------------------------------------------
{
	const CHANNELINDEX maxChans = m_SndFile.GetModSpecifications().channelsMax;

	if (nNewChannels > maxChans)
	{
		CString error;
		error.Format("Error: Max number of channels for this file type is %d", maxChans);
		Reporting::Notification(error, MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	if (nNewChannels == GetNumChannels()) return false;

	if (nNewChannels < GetNumChannels())
	{
		// Remove channels
		UINT nChnToRemove = 0;
		CHANNELINDEX nFound = 0;

		//nNewChannels = 0 means user can choose how many channels to remove
		if(nNewChannels > 0)
		{
			nChnToRemove = GetNumChannels() - nNewChannels;
			nFound = nChnToRemove;
		} else
		{
			nChnToRemove = 0;
			nFound = GetNumChannels();
		}
		
		CRemoveChannelsDlg rem(&m_SndFile, nChnToRemove, showCancelInRemoveDlg);
		CheckUsedChannels(rem.m_bKeepMask, nFound);
		if (rem.DoModal() != IDOK) return false;

		// Removing selected channels
		return RemoveChannels(rem.m_bKeepMask);
	} else
	{
		// Increasing number of channels
		BeginWaitCursor();
		vector<CHANNELINDEX> channels(nNewChannels, CHANNELINDEX_INVALID);
		for(CHANNELINDEX nChn = 0; nChn < GetNumChannels(); nChn++)
		{
			channels[nChn] = nChn;
		}

		const bool success = (ReArrangeChannels(channels) == nNewChannels);
		if(success)
		{
			SetModified();
			UpdateAllViews(NULL, HINT_MODTYPE);
		}
		return success;
	}
}


// To remove all channels whose index corresponds to false in the keepMask vector.
// Return true on success.
bool CModDoc::RemoveChannels(const vector<bool> &keepMask)
//--------------------------------------------------------
{
	UINT nRemainingChannels = 0;
	//First calculating how many channels are to be left
	for(CHANNELINDEX nChn = 0; nChn < GetNumChannels(); nChn++)
	{
		if(keepMask[nChn]) nRemainingChannels++;
	}
	if(nRemainingChannels == GetNumChannels() || nRemainingChannels < m_SndFile.GetModSpecifications().channelsMin)
	{
		CString str;	
		if(nRemainingChannels == GetNumChannels()) str.Format("No channels chosen to be removed.");
		else str.Format("No removal done - channel number is already at minimum.");
		CMainFrame::GetMainFrame()->MessageBox(str, "Remove channel", MB_OK | MB_ICONINFORMATION);
		return false;
	}

	BeginWaitCursor();
	// Create new channel order, with only channels from m_bChnMask left.
	vector<CHANNELINDEX> channels(nRemainingChannels, 0);
	CHANNELINDEX i = 0;
	for(CHANNELINDEX nChn = 0; nChn < GetNumChannels(); nChn++)
	{
		if(keepMask[nChn])
		{
			channels[i++] = nChn;
		}
	}
	const bool success = (ReArrangeChannels(channels) == nRemainingChannels);
	if(success)
	{
		SetModified();
		UpdateAllViews(NULL, HINT_MODTYPE);
	}
	EndWaitCursor();
	return success;

}


// Base code for adding, removing, moving and duplicating channels. Returns new number of channels on success, CHANNELINDEX_INVALID otherwise.
// The new channel vector can contain CHANNELINDEX_INVALID for adding new (empty) channels.
CHANNELINDEX CModDoc::ReArrangeChannels(const vector<CHANNELINDEX> &newOrder, const bool createUndoPoint)
//-------------------------------------------------------------------------------------------------------
{
	//newOrder[i] tells which current channel should be placed to i:th position in
	//the new order, or if i is not an index of current channels, then new channel is
	//added to position i. If index of some current channel is missing from the
	//newOrder-vector, then the channel gets removed.

	const CHANNELINDEX nRemainingChannels = static_cast<CHANNELINDEX>(newOrder.size());

	if(nRemainingChannels > m_SndFile.GetModSpecifications().channelsMax || nRemainingChannels < m_SndFile.GetModSpecifications().channelsMin) 	
	{
		CString str;
		str.Format(GetStrI18N(_TEXT("Can't apply change: Number of channels should be within [%u,%u]")), m_SndFile.GetModSpecifications().channelsMin, m_SndFile.GetModSpecifications().channelsMax);
		CMainFrame::GetMainFrame()->MessageBox(str , "ReArrangeChannels", MB_OK | MB_ICONINFORMATION);
		return CHANNELINDEX_INVALID;
	}

	if(m_SndFile.Patterns.Size() == 0)
	{
		// Nothing to do
		return GetNumChannels();
	}

	bool first = true;
	// Find highest valid pattern number for storing channel undo data with, since the pattern with the highest number will be undone first.
	PATTERNINDEX highestPattern = 0;
	for(PATTERNINDEX nPat = m_SndFile.Patterns.Size() - 1; nPat > 0; nPat--)
	{
		if(m_SndFile.Patterns.IsValidPat(nPat))
		{
			highestPattern = nPat;
			break;
		}
	}

	CriticalSection cs;
	for(PATTERNINDEX nPat = 0; nPat <= highestPattern; nPat++) 
	{
		if(m_SndFile.Patterns.IsValidPat(nPat))
		{
			if(createUndoPoint)
			{
				GetPatternUndo()->PrepareUndo(nPat, 0, 0, GetNumChannels(), m_SndFile.Patterns[nPat].GetNumRows(), !first, (nPat == highestPattern));
				first = false;
			}

			MODCOMMAND *p = m_SndFile.Patterns[nPat];
			MODCOMMAND *newp = CPattern::AllocatePattern(m_SndFile.Patterns[nPat].GetNumRows(), nRemainingChannels);
			if(!newp)
			{
				cs.Leave();
				CMainFrame::GetMainFrame()->MessageBox("ERROR: Pattern allocation failed in ReArrangechannels(...)" , "ReArrangeChannels", MB_OK | MB_ICONINFORMATION);
				return CHANNELINDEX_INVALID;
			}
			MODCOMMAND *tmpsrc = p, *tmpdest = newp;
			for(ROWINDEX nRow = 0; nRow < m_SndFile.Patterns[nPat].GetNumRows(); nRow++) //Scrolling rows
			{
				for(CHANNELINDEX nChn = 0; nChn < nRemainingChannels; nChn++, tmpdest++) //Scrolling channels.
				{
					if(newOrder[nChn] < GetNumChannels()) //Case: getting old channel to the new channel order.
						*tmpdest = tmpsrc[nRow * GetNumChannels() + newOrder[nChn]];
					else //Case: figure newOrder[k] is not the index of any current channel, so adding a new channel.
						*tmpdest = MODCOMMAND::Empty();

				}
			}
			m_SndFile.Patterns[nPat] = newp;
			CPattern::FreePattern(p);
		}
	}

	MODCHANNEL chns[MAX_BASECHANNELS];		
	MODCHANNELSETTINGS settings[MAX_BASECHANNELS];
	vector<UINT> recordStates(GetNumChannels(), 0);
	vector<bool> chnMutePendings(GetNumChannels(), false);

	for(CHANNELINDEX nChn = 0; nChn < GetNumChannels(); nChn++)
	{
		settings[nChn] = m_SndFile.ChnSettings[nChn];
		chns[nChn] = m_SndFile.Chn[nChn];
		recordStates[nChn] = IsChannelRecord(nChn);
		chnMutePendings[nChn] = m_SndFile.m_bChannelMuteTogglePending[nChn];
	}

	ReinitRecordState();

	for(CHANNELINDEX nChn = 0; nChn < nRemainingChannels; nChn++)
	{
		if(newOrder[nChn] < GetNumChannels())
		{
			m_SndFile.ChnSettings[nChn] = settings[newOrder[nChn]];
			m_SndFile.Chn[nChn] = chns[newOrder[nChn]];
			if(recordStates[newOrder[nChn]] == 1) Record1Channel(nChn, true);
			if(recordStates[newOrder[nChn]] == 2) Record2Channel(nChn, true);
			m_SndFile.m_bChannelMuteTogglePending[nChn] = chnMutePendings[newOrder[nChn]];
		}
		else
		{
			m_SndFile.InitChannel(nChn);
		}
	}
	// Reset MOD panning (won't affect other module formats)
	m_SndFile.SetupMODPanning();

	m_SndFile.m_nChannels = nRemainingChannels;

	// Reset removed channels. Most notably, clear the channel name.
	for(CHANNELINDEX nChn = GetNumChannels(); nChn < MAX_BASECHANNELS; nChn++)
	{
		m_SndFile.InitChannel(nChn);
		m_SndFile.Chn[nChn].dwFlags |= CHN_MUTE;
	}

	return GetNumChannels();
}


bool CModDoc::MoveChannel(CHANNELINDEX chnFrom, CHANNELINDEX chnTo)
//-----------------------------------------------------------------
{
	//Implementation of move channel using ReArrangeChannels(...). So this function
	//only creates correct newOrder-vector used in the ReArrangeChannels(...).
	if(chnFrom == chnTo) return false;
	if(chnFrom >= GetNumChannels() || chnTo >= GetNumChannels())
	{
		CString str = "Error: Bad move indexes in CSoundFile::MoveChannel(...)";	
		CMainFrame::GetMainFrame()->MessageBox(str , "MoveChannel(...)", MB_OK | MB_ICONINFORMATION);
		return true;
	}
	vector<CHANNELINDEX> newOrder;
	//First creating new order identical to current order...
	for(CHANNELINDEX i = 0; i < GetNumChannels(); i++)
	{
		newOrder.push_back(i);
	}
	//...and then add the move channel effect.
	if(chnFrom < chnTo)
	{
		CHANNELINDEX temp = newOrder[chnFrom];
		for(CHANNELINDEX i = chnFrom; i < chnTo; i++)
		{
			newOrder[i] = newOrder[i + 1];
		}
		newOrder[chnTo] = temp;
	}
	else //case chnFrom > chnTo(can't be equal, since it has been examined earlier.)
	{
		CHANNELINDEX temp = newOrder[chnFrom];
		for(CHANNELINDEX i = chnFrom; i >= chnTo + 1; i--)
		{
			newOrder[i] = newOrder[i - 1];
		}
		newOrder[chnTo] = temp;
	}

	if(newOrder.size() != ReArrangeChannels(newOrder))
	{
		CMainFrame::GetMainFrame()->MessageBox("BUG: Channel number changed in MoveChannel()" , "", MB_OK | MB_ICONINFORMATION);
	}
	return false;
}


// Functor for converting instrument numbers to sample numbers in the patterns
struct ConvertInstrumentsToSamplesInPatterns
//==========================================
{
	ConvertInstrumentsToSamplesInPatterns(CSoundFile *pSndFile)
	{
		this->pSndFile = pSndFile;
	}

	void operator()(MODCOMMAND& m)
	{
		if(m.instr)
		{
			MODCOMMAND::INSTR instr = m.instr, newinstr = 0;
			MODCOMMAND::NOTE note = m.note, newnote = note;
			if((note >= NOTE_MIN) && (note <= NOTE_MAX))
				note--;
			else
				note = NOTE_MIDDLEC - 1;

			if((instr < MAX_INSTRUMENTS) && (pSndFile->Instruments[instr]))
			{
				const MODINSTRUMENT *pIns = pSndFile->Instruments[instr];
				newinstr = pIns->Keyboard[note];
				newnote = pIns->NoteMap[note];
				if(newinstr >= MAX_SAMPLES) newinstr = 0;
			}
			m.instr = newinstr;
			m.note = newnote;
		}
	}

	CSoundFile *pSndFile;
};


bool CModDoc::ConvertInstrumentsToSamples()
//-----------------------------------------
{
	if (!m_SndFile.GetNumInstruments())
		return false;
	m_SndFile.Patterns.ForEachModCommand(ConvertInstrumentsToSamplesInPatterns(&m_SndFile));
	return true;
}


UINT CModDoc::RemovePlugs(const vector<bool> &keepMask)
//-----------------------------------------------------
{
	//Remove all plugins whose keepMask[plugindex] is false.
	UINT nRemoved = 0;
	const PLUGINDEX maxPlug = min(MAX_MIXPLUGINS, keepMask.size());

	for (PLUGINDEX nPlug = 0; nPlug < maxPlug; nPlug++)
	{
		SNDMIXPLUGIN* pPlug = &m_SndFile.m_MixPlugins[nPlug];		
		if (keepMask[nPlug] || !pPlug)
		{
			continue;
		}

		if (pPlug->pPluginData)
		{
			delete[] pPlug->pPluginData;
			pPlug->pPluginData = NULL;
		}
		if (pPlug->pMixPlugin)
		{
			pPlug->pMixPlugin->Release();
			pPlug->pMixPlugin=NULL;
		}
		if (pPlug->pMixState)
		{
			delete pPlug->pMixState;
		}

		MemsetZero(pPlug->Info);
		Log("Zeroing range (%d) %X - %X\n", nPlug, &(pPlug->Info),  &(pPlug->Info)+sizeof(SNDMIXPLUGININFO));
		pPlug->nPluginDataSize=0;
		pPlug->fDryRatio=0;	
		pPlug->defaultProgram=0;
		nRemoved++;
	}

	return nRemoved;
}


BOOL CModDoc::AdjustEndOfSample(UINT nSample)
//-------------------------------------------
{
	if (nSample >= MAX_SAMPLES) return FALSE;
	MODSAMPLE &sample = m_SndFile.GetSample(nSample);
	if ((!sample.nLength) || (!sample.pSample)) return FALSE;

	ctrlSmp::AdjustEndOfSample(sample, &m_SndFile);

	return TRUE;
}


PATTERNINDEX CModDoc::InsertPattern(ORDERINDEX nOrd, ROWINDEX nRows)
//------------------------------------------------------------------
{
	const PATTERNINDEX i = m_SndFile.Patterns.Insert(nRows);
	if(i == PATTERNINDEX_INVALID)
		return i;

	//Increasing orderlist size if given order is beyond current limit,
	//or if the last order already has a pattern.
	if((nOrd == m_SndFile.Order.size() ||
		m_SndFile.Order.Last() < m_SndFile.Patterns.Size() ) &&
		m_SndFile.Order.GetLength() < m_SndFile.GetModSpecifications().ordersMax)
	{
		m_SndFile.Order.Append();
	}

	for (UINT j=0; j<m_SndFile.Order.size(); j++)
	{
		if (m_SndFile.Order[j] == i) break;
		if (m_SndFile.Order[j] == m_SndFile.Order.GetInvalidPatIndex() && nOrd == ORDERINDEX_INVALID)
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


SAMPLEINDEX CModDoc::InsertSample(bool bLimit)
//--------------------------------------------
{
	SAMPLEINDEX i = 1;
	for(i = 1; i <= m_SndFile.m_nSamples; i++)
	{
		if ((!m_SndFile.m_szNames[i][0]) && (m_SndFile.GetSample(i).pSample == NULL))
		{
			if ((!m_SndFile.m_nInstruments) || (!m_SndFile.IsSampleUsed(i)))
			break;
		}
	}
	if (((bLimit) && (i >= 200) && (!m_SndFile.m_nInstruments))
	 || (i > m_SndFile.GetModSpecifications().samplesMax))
	{
		ErrorBox(IDS_ERR_TOOMANYSMP, CMainFrame::GetMainFrame());
		return SAMPLEINDEX_INVALID;
	}
	if (!m_SndFile.m_szNames[i][0]) strcpy(m_SndFile.m_szNames[i], "untitled");
	if (i > m_SndFile.GetNumSamples()) m_SndFile.m_nSamples = i;
	InitializeSample(m_SndFile.GetSample(i));
	SetModified();
	return i;
}


// Insert a new instrument assigned to sample nSample or duplicate instrument nDuplicate.
// If nSample is invalid, an approriate sample slot is selected. 0 means "no sample".
INSTRUMENTINDEX CModDoc::InsertInstrument(SAMPLEINDEX nSample, INSTRUMENTINDEX nDuplicate)
//----------------------------------------------------------------------------------------
{
	if (m_SndFile.GetModSpecifications().instrumentsMax == 0) return INSTRUMENTINDEX_INVALID;

	MODINSTRUMENT *pDup = nullptr;
	const INSTRUMENTINDEX nInstrumentMax = m_SndFile.GetModSpecifications().instrumentsMax - 1;
	if ((nDuplicate > 0) && (nDuplicate <= m_SndFile.m_nInstruments))
	{
		pDup = m_SndFile.Instruments[nDuplicate];
	}
	if ((!m_SndFile.GetNumInstruments()) && ((m_SndFile.GetNumSamples() > 1) || (m_SndFile.GetSample(1).pSample)))
	{
		if (pDup) return INSTRUMENTINDEX_INVALID;
		UINT n = CMainFrame::GetMainFrame()->MessageBox("Convert existing samples to instruments first?", NULL, MB_YESNOCANCEL|MB_ICONQUESTION);
		if (n == IDYES)
		{
			SAMPLEINDEX nInstruments = m_SndFile.m_nSamples;
			if (nInstruments > nInstrumentMax) nInstruments = nInstrumentMax;
			for (SAMPLEINDEX smp = 1; smp <= nInstruments; smp++)
			{
				m_SndFile.GetSample(smp).uFlags &= ~CHN_MUTE;
				if (!m_SndFile.Instruments[smp])
				{
					try
					{
						MODINSTRUMENT *p = new MODINSTRUMENT;
						InitializeInstrument(p, smp);
						m_SndFile.Instruments[smp] = p;
						lstrcpyn(p->name, m_SndFile.m_szNames[smp], sizeof(p->name));
					} catch(...)
					{
						ErrorBox(IDS_ERR_OUTOFMEMORY, CMainFrame::GetMainFrame());
						return INSTRUMENTINDEX_INVALID;
					}
				}
			}
			m_SndFile.m_nInstruments = nInstruments;
		} else
		if (n != IDNO) return INSTRUMENTINDEX_INVALID;
	}
	UINT newins = 0;
	for (INSTRUMENTINDEX i = 1; i <= m_SndFile.m_nInstruments; i++)
	{
		if (!m_SndFile.Instruments[i])
		{
			newins = i;
			break;
		}
	}
	if (!newins)
	{
		if (m_SndFile.m_nInstruments >= nInstrumentMax)
		{
			ErrorBox(IDS_ERR_TOOMANYINS, CMainFrame::GetMainFrame());
			return INSTRUMENTINDEX_INVALID;
		}
		newins = ++m_SndFile.m_nInstruments;
	}
	MODINSTRUMENT *pIns = new MODINSTRUMENT;
	if (pIns)
	{
		SAMPLEINDEX newsmp = 0;
		if (nSample < m_SndFile.GetModSpecifications().samplesMax)
		{
			newsmp = nSample;
		} else
		if (!pDup)
		{
			for(SAMPLEINDEX k = 1; k <= m_SndFile.m_nSamples; k++)
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
				if (inssmp != SAMPLEINDEX_INVALID) newsmp = inssmp;
			}
		}

		CriticalSection cs;
		if (pDup)
		{
			*pIns = *pDup;
// -> CODE#0023
// -> DESC="IT project files (.itp)"
			strcpy(m_SndFile.m_szInstrumentPath[newins - 1], m_SndFile.m_szInstrumentPath[nDuplicate - 1]);
			m_bsInstrumentModified.reset(newins - 1);
// -! NEW_FEATURE#0023
		} else
		{
			InitializeInstrument(pIns, newsmp);
		}
		m_SndFile.Instruments[newins] = pIns;

		SetModified();
	} else
	{
		ErrorBox(IDS_ERR_OUTOFMEMORY, CMainFrame::GetMainFrame());
		return INSTRUMENTINDEX_INVALID;
	}
	return newins;
}


void CModDoc::InitializeSample(MODSAMPLE &sample)
//-----------------------------------------------
{
	sample.nVolume = 256;
	sample.nGlobalVol = 64;
	sample.nPan = 128;
	sample.nC5Speed = 8363;
	sample.RelativeTone = 0;
	sample.nFineTune = 0;
	sample.nVibType = 0;
	sample.nVibSweep = 0;
	sample.nVibDepth = 0;
	sample.nVibRate = 0;
	sample.uFlags &= ~(CHN_PANNING|CHN_SUSTAINLOOP);
	if(m_SndFile.GetType() == MOD_TYPE_XM)
	{
		sample.uFlags |= CHN_PANNING;
	}
}


void CModDoc::InitializeInstrument(MODINSTRUMENT *pIns, UINT nsample)
//-------------------------------------------------------------------
{
	if(pIns == nullptr)
		return;
	MemsetZero(*pIns);
	pIns->nFadeOut = 256;
	pIns->nGlobalVol = 64;
	pIns->nPan = 128;
	pIns->nPPC = NOTE_MIDDLEC - 1;
	m_SndFile.SetDefaultInstrumentValues(pIns);
	for (UINT n=0; n<128; n++)
	{
		pIns->Keyboard[n] = nsample;
		pIns->NoteMap[n] = n+1;
	}
	pIns->pTuning = pIns->s_DefaultTuning;
}


bool CModDoc::RemoveOrder(SEQUENCEINDEX nSeq, ORDERINDEX nOrd)
//------------------------------------------------------------
{
	if (nSeq >= m_SndFile.Order.GetNumSequences() || nOrd >= m_SndFile.Order.GetSequence(nSeq).size())
		return false;

	CriticalSection cs;

	SEQUENCEINDEX nOldSeq = m_SndFile.Order.GetCurrentSequenceIndex();
	m_SndFile.Order.SetSequence(nSeq);
	for (ORDERINDEX i = nOrd; i < m_SndFile.Order.GetSequence(nSeq).size() - 1; i++)
	{
		m_SndFile.Order[i] = m_SndFile.Order[i + 1];
	}
	m_SndFile.Order[m_SndFile.Order.GetLastIndex()] = m_SndFile.Order.GetInvalidPatIndex();
	m_SndFile.Order.SetSequence(nOldSeq);
	SetModified();

	return true;
}



bool CModDoc::RemovePattern(PATTERNINDEX nPat)
//--------------------------------------------
{
	if ((nPat < m_SndFile.Patterns.Size()) && (m_SndFile.Patterns[nPat]))
	{
		CriticalSection cs;

		m_SndFile.Patterns.Remove(nPat);
		SetModified();

		return true;
	}
	return false;
}


bool CModDoc::RemoveSample(SAMPLEINDEX nSmp)
//------------------------------------------
{
	if ((nSmp) && (nSmp <= m_SndFile.GetNumSamples()))
	{
		CriticalSection cs;

		m_SndFile.DestroySample(nSmp);
		m_SndFile.m_szNames[nSmp][0] = 0;
		while ((m_SndFile.GetNumSamples() > 1)
			&& (!m_SndFile.m_szNames[m_SndFile.GetNumSamples()][0])
			&& (!m_SndFile.GetSample(m_SndFile.GetNumSamples()).pSample))
		{
			m_SndFile.m_nSamples--;
		}
		SetModified();

		return true;
	}
	return false;
}


bool CModDoc::RemoveInstrument(INSTRUMENTINDEX nIns)
//--------------------------------------------------
{
	if ((nIns) && (nIns <= m_SndFile.GetNumInstruments()) && (m_SndFile.Instruments[nIns]))
	{
		bool instrumentsLeft = false;

		m_SndFile.DestroyInstrument(nIns);

		CriticalSection cs;
		if (nIns == m_SndFile.m_nInstruments) m_SndFile.m_nInstruments--;
		for (UINT i=1; i<MAX_INSTRUMENTS; i++) if (m_SndFile.Instruments[i]) instrumentsLeft = true;
		if (!instrumentsLeft) m_SndFile.m_nInstruments = 0;
		SetModified();

		return true;
	}
	return false;
}


bool CModDoc::MoveOrder(ORDERINDEX nSourceNdx, ORDERINDEX nDestNdx, bool bUpdate, bool bCopy, SEQUENCEINDEX nSourceSeq, SEQUENCEINDEX nDestSeq)
//---------------------------------------------------------------------------------------------------------------------------------------------
{
	if (max(nSourceNdx, nDestNdx) >= m_SndFile.Order.size()) return false;
	if (nDestNdx >= m_SndFile.GetModSpecifications().ordersMax) return false;

	if(nSourceSeq == SEQUENCEINDEX_INVALID) nSourceSeq = m_SndFile.Order.GetCurrentSequenceIndex();
	if(nDestSeq == SEQUENCEINDEX_INVALID) nDestSeq = m_SndFile.Order.GetCurrentSequenceIndex();
	if (max(nSourceSeq, nDestSeq) >= m_SndFile.Order.GetNumSequences()) return false;
	PATTERNINDEX nSourcePat = m_SndFile.Order.GetSequence(nSourceSeq)[nSourceNdx];

	// save current working sequence
	SEQUENCEINDEX nWorkingSeq = m_SndFile.Order.GetCurrentSequenceIndex();

	// Delete source
	if (!bCopy)
	{
		m_SndFile.Order.SetSequence(nSourceSeq);
		for (ORDERINDEX i = nSourceNdx; i < m_SndFile.Order.size() - 1; i++) m_SndFile.Order[i] = m_SndFile.Order[i + 1];
		if (nSourceNdx < nDestNdx) nDestNdx--;
	}
	// Insert at dest
	m_SndFile.Order.SetSequence(nDestSeq);
	for (ORDERINDEX nOrd = m_SndFile.Order.size() - 1; nOrd > nDestNdx; nOrd--) m_SndFile.Order[nOrd] = m_SndFile.Order[nOrd - 1];
	m_SndFile.Order[nDestNdx] = nSourcePat;
	if (bUpdate)
	{
		UpdateAllViews(NULL, HINT_MODSEQUENCE, NULL);
	}

	m_SndFile.Order.SetSequence(nWorkingSeq);
	return true;
}


BOOL CModDoc::ExpandPattern(PATTERNINDEX nPattern)
//------------------------------------------------
{
// -> CODE#0008
// -> DESC="#define to set pattern size"

	if ((nPattern >= m_SndFile.Patterns.Size()) || (!m_SndFile.Patterns[nPattern])) return FALSE;
	if(m_SndFile.Patterns[nPattern].Expand())
		return FALSE;
	else
		return TRUE;
}


BOOL CModDoc::ShrinkPattern(PATTERNINDEX nPattern)
//------------------------------------------------
{
	if ((nPattern >= m_SndFile.Patterns.Size()) || (!m_SndFile.Patterns[nPattern])) return FALSE;
	if(m_SndFile.Patterns[nPattern].Shrink())
		return FALSE;
	else
		return TRUE;
}


// Clipboard format:
// Hdr: "ModPlug Tracker S3M\r\n"
// Full:  '|C#401v64A06'
// Reset: '|...........'
// Empty: '|           '
// End of row: '\n'

static LPCSTR lpszClipboardPatternHdr = "ModPlug Tracker %3s\r\n";

bool CModDoc::CopyPattern(PATTERNINDEX nPattern, DWORD dwBeginSel, DWORD dwEndSel)
//--------------------------------------------------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	DWORD dwMemSize;
	HGLOBAL hCpy;
	UINT nrows = (dwEndSel >> 16) - (dwBeginSel >> 16) + 1;
	UINT ncols = ((dwEndSel & 0xFFFF) >> 3) - ((dwBeginSel & 0xFFFF) >> 3) + 1;

	if ((!pMainFrm) || (nPattern >= m_SndFile.Patterns.Size()) || (!m_SndFile.Patterns[nPattern])) return false;
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
				if ((row + (dwBeginSel >> 16)) >= m_SndFile.Patterns[nPattern].GetNumRows()) break;
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
						case NOTE_NONE:		p[1] = p[2] = p[3] = '.'; break;
						case NOTE_KEYOFF:	p[1] = p[2] = p[3] = '='; break;
						case NOTE_NOTECUT:	p[1] = p[2] = p[3] = '^'; break;
						case NOTE_FADE:		p[1] = p[2] = p[3] = '~'; break;
						case NOTE_PC:		p[1] = 'P'; p[2] = 'C'; p[3] = ' '; break;
						case NOTE_PCS:		p[1] = 'P'; p[2] = 'C'; p[3] = 'S'; break;
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
						if(m->IsPcNote())
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
								p[6] = m_SndFile.GetModSpecifications().GetVolEffectLetter(m->volcmd);
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
						if(m->IsPcNote())
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
								p[9] = m_SndFile.GetModSpecifications().GetEffectLetter(m->command);
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
				*p++ = '\r';
				*p++ = '\n';
			}
			*p = 0;
		}
		GlobalUnlock(hCpy);
		SetClipboardData (CF_TEXT, (HANDLE) hCpy);
		CloseClipboard();
	}
	EndWaitCursor();
	return true;
}


bool CModDoc::PastePattern(PATTERNINDEX nPattern, DWORD dwBeginSel, enmPatternPasteModes pasteMode)
//-------------------------------------------------------------------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if ((!pMainFrm) || (nPattern >= m_SndFile.Patterns.Size()) || (!m_SndFile.Patterns[nPattern])) return false;
	BeginWaitCursor();
	if (pMainFrm->OpenClipboard())
	{
		HGLOBAL hCpy = ::GetClipboardData(CF_TEXT);
		LPSTR p;

		if ((hCpy) && ((p = (LPSTR)GlobalLock(hCpy)) != NULL))
		{
			const TEMPO spdmax = m_SndFile.GetModSpecifications().speedMax;
			const DWORD dwMemSize = GlobalSize(hCpy);
			CHANNELINDEX ncol = (dwBeginSel & 0xFFFF) >> 3, col;
			const ROWINDEX startRow = (ROWINDEX)(dwBeginSel >> 16);
			ROWINDEX nrow = startRow;
			bool bOk = false;
			bool bPrepareUndo = true;	// prepare pattern for undo next time
			bool bFirstUndo = true;		// for chaining undos (see overflow paste)
			MODTYPE origFormat = MOD_TYPE_IT;	// paste format
			size_t pos, startPos = 0;
			MODCOMMAND *m = m_SndFile.Patterns[nPattern];

			const bool doOverflowPaste = (CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_OVERFLOWPASTE) && (pasteMode != pm_pasteflood) && (pasteMode != pm_pushforwardpaste);
			const bool doITStyleMix = (pasteMode == pm_mixpaste_it);
			const bool doMixPaste = ((pasteMode == pm_mixpaste) || doITStyleMix);

			ORDERINDEX oCurrentOrder; //jojo.echopaste
			ROWINDEX rTemp;
			PATTERNINDEX pTemp;
			GetEditPosition(rTemp, pTemp, oCurrentOrder);

			if ((nrow >= m_SndFile.Patterns[nPattern].GetNumRows()) || (ncol >= m_SndFile.GetNumChannels())) goto PasteDone;
			m += nrow * m_SndFile.m_nChannels;

			// Search for signature
			for (pos = startPos; p[pos] != 0 && pos < dwMemSize; pos++)
			{
				CHAR szFormat[4];	// adjust this if the "%3s" part in the format string changes.
				if(sscanf(p + pos, lpszClipboardPatternHdr, szFormat) > 0)
				{
					if(!strcmp(szFormat, "S3M")) origFormat = MOD_TYPE_S3M;
					if(!strcmp(szFormat, "XM")) origFormat = MOD_TYPE_XM;
					if(!strcmp(szFormat, "IT")) origFormat = MOD_TYPE_IT;
					if(!strcmp(szFormat, "MPT")) origFormat = MOD_TYPE_MPT;
					if(!strcmp(szFormat, "MOD")) origFormat = MOD_TYPE_MOD;
					startPos = pos;	// start reading patterns from here
					break;
				}
			}

			const CModSpecifications &sourceSpecs = CSoundFile::GetModSpecifications(origFormat);
			const bool bS3MCommands = (origFormat & (MOD_TYPE_IT|MOD_TYPE_MPT|MOD_TYPE_S3M)) != 0 ? true : false;
			pos = startPos;

			while ((nrow < m_SndFile.Patterns[nPattern].GetNumRows()))
			{
				// Search for column separator or end of paste data
				while ((pos + 11 >= dwMemSize) || p[pos] != '|')
				{
					if (pos + 11 >= dwMemSize || !p[pos])
					{
						if((pasteMode == pm_pasteflood) && (nrow != startRow)) // prevent infinite loop with malformed clipboard data
							pos = startPos; // paste from beginning
						else
							goto PasteDone;
					} else
					{
						pos++;
					}
				}
				bOk = true;
				col = ncol;
				// Paste columns
				while ((p[pos] == '|') && (pos + 11 < dwMemSize))
				{
					LPSTR s = p+pos+1;

					// Check valid paste condition. Paste will be skipped if
					// -col is not a valid channelindex or
					// -doing mix paste and paste destination modcommand is a PCnote or
					// -doing mix paste and trying to paste PCnote on non-empty modcommand.
					const bool bSkipPaste =
						(col >= m_SndFile.GetNumChannels()) ||
						(doMixPaste && m[col].IsPcNote()) ||
						(doMixPaste && s[0] == 'P' && !m[col].IsEmpty());

					if (bSkipPaste == false)
					{
						// Before changing anything in this pattern, we have to create an undo point.
						if(bPrepareUndo)
						{
							GetPatternUndo()->PrepareUndo(nPattern, 0, 0, m_SndFile.m_nChannels, m_SndFile.Patterns[nPattern].GetNumRows(), !bFirstUndo);
							bPrepareUndo = false;
							bFirstUndo = false;
						}
						
						// ITSyle mixpaste requires that we keep a copy of the thing we are about to paste on
						// so that we can refer back to check if there was anything in e.g. the note column before we pasted.
						const MODCOMMAND origModCmd = m[col];

						// push channel data below paste point first.
						if(pasteMode == pm_pushforwardpaste)
						{
							for(ROWINDEX nPushRow = m_SndFile.Patterns[nPattern].GetNumRows() - 1 - nrow; nPushRow > 0; nPushRow--)
							{
								m[col + nPushRow * m_SndFile.m_nChannels] = m[col + (nPushRow - 1) * m_SndFile.m_nChannels];
							}
							m[col].Clear();
						}

						// Note
						if (s[0] > ' ' && (!doMixPaste || ((!doITStyleMix && origModCmd.note==0) || 
												     (doITStyleMix && origModCmd.note==0 && origModCmd.instr==0 && origModCmd.volcmd==0))))
						{
							m[col].note = NOTE_NONE;
							if (s[0] == '=') m[col].note = NOTE_KEYOFF; else
							if (s[0] == '^') m[col].note = NOTE_NOTECUT; else
							if (s[0] == '~') m[col].note = NOTE_FADE; else
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
						if (s[3] > ' ' && (!doMixPaste || ( (!doITStyleMix && origModCmd.instr==0) || 
												     (doITStyleMix  && origModCmd.note==0 && origModCmd.instr==0 && origModCmd.volcmd==0) ) ))

						{
							if ((s[3] >= '0') && (s[3] <= ('0'+(MAX_SAMPLES/10))))
							{
								m[col].instr = (s[3]-'0') * 10 + (s[4]-'0');
							} else m[col].instr = 0;
						}
						// Volume
						if (s[5] > ' ' && (!doMixPaste || ((!doITStyleMix && origModCmd.volcmd==0) || 
												     (doITStyleMix && origModCmd.note==0 && origModCmd.instr==0 && origModCmd.volcmd==0))))

						{
							if (s[5] != '.')
							{
								if(m[col].IsPcNote())
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
										const char cmd = sourceSpecs.GetVolEffectLetter(i);
										if (s[5] == cmd && cmd != '?')
										{
											m[col].volcmd = i;
											break;
										}
									}
									m[col].vol = (s[6]-'0')*10 + (s[7]-'0');
								}
							} else m[col].volcmd = m[col].vol = 0;
						}
						
						if (m[col].IsPcNote())
						{
							if (s[8] != '.' && s[8] > ' ')
							{
								char val[4];
								memcpy(val, s+8, 3);
								val[3] = 0;
								m[col].SetValueEffectCol(ConvertStrTo<uint16>(val));
							}
						}
						else
						{
							if (s[8] > ' ' && (!doMixPaste || ((!doITStyleMix && origModCmd.command==0) || 
														(doITStyleMix && origModCmd.command==0 && origModCmd.param==0))))
							{
								m[col].command = 0;
								if (s[8] != '.')
								{
									for (UINT i=1; i<MAX_EFFECTS; i++)
									{
										const char cmd = sourceSpecs.GetEffectLetter(i);
										if (s[8] == cmd && cmd != '?')
										{
											m[col].command = i;
											break;
										}
									}
								}
							}
							// Effect value
							if (s[9] > ' ' && (!doMixPaste || ((!doITStyleMix && (origModCmd.command == CMD_NONE || origModCmd.param == 0)) || 
														(doITStyleMix && origModCmd.command == CMD_NONE && origModCmd.param == 0))))
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
									if (!bS3MCommands) m[col].command = (m[col].param <= spdmax) ? CMD_SPEED : CMD_TEMPO;
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
									if (!bS3MCommands) m[col].command = (m[col].param <= spdmax) ? CMD_SPEED : CMD_TEMPO;
									break;
								}
							}
						}

						// convert some commands, if necessary. With mix paste convert only
						// if the original modcommand was empty as otherwise the unchanged parts
						// of the old modcommand would falsely be interpreted being of type
						// origFormat and ConvertCommand could change them.
						if (origFormat != m_SndFile.m_nType && (doMixPaste == false || origModCmd.IsEmpty(false)))
							m_SndFile.ConvertCommand(&(m[col]), origFormat, m_SndFile.m_nType);
					}

					pos += 12;
					col++;
				}
				// Next row
				m += m_SndFile.m_nChannels;
				nrow++;

				// Overflow paste. Continue pasting in next pattern if enabled.
				// If Paste Flood is enabled, this won't be called due to obvious reasons.
				if(doOverflowPaste)
				{
					while(nrow >= m_SndFile.Patterns[nPattern].GetNumRows())
					{
						nrow = 0;
						ORDERINDEX oNextOrder = m_SndFile.Order.GetNextOrderIgnoringSkips(oCurrentOrder);
						if((oNextOrder == 0) || (oNextOrder >= m_SndFile.Order.size())) goto PasteDone;
						nPattern = m_SndFile.Order[oNextOrder];
						if(m_SndFile.Patterns.IsValidPat(nPattern) == false) goto PasteDone;
						m = m_SndFile.Patterns[nPattern];
						oCurrentOrder = oNextOrder;
						bPrepareUndo = true;
					}
				}
			
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
	return true;
}


/////////////////////////////////////////////////////////////////////////////////////////
// Copy/Paste envelope

static LPCSTR pszEnvHdr = "Modplug Tracker Envelope\r\n";
static LPCSTR pszEnvFmt = "%d,%d,%d,%d,%d,%d,%d,%d\r\n";

bool CModDoc::CopyEnvelope(UINT nIns, enmEnvelopeTypes nEnv)
//----------------------------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	HANDLE hCpy;
	CHAR s[4096];
	MODINSTRUMENT *pIns;
	DWORD dwMemSize;

	if ((nIns < 1) || (nIns > m_SndFile.m_nInstruments) || (!m_SndFile.Instruments[nIns]) || (!pMainFrm)) return false;
	BeginWaitCursor();
	pIns = m_SndFile.Instruments[nIns];
	if(pIns == nullptr) return false;
	
	INSTRUMENTENVELOPE *pEnv = nullptr;

	switch(nEnv)
	{
	case ENV_PANNING:
		pEnv = &pIns->PanEnv;
		break;
	case ENV_PITCH:
		pEnv = &pIns->PitchEnv;
		break;
	default:
		pEnv = &pIns->VolEnv;
		break;
	}

	// We don't want to copy empty envelopes
	if(pEnv->nNodes == 0)
	{
		return false;
	}

	strcpy(s, pszEnvHdr);
	wsprintf(s + strlen(s), pszEnvFmt, pEnv->nNodes, pEnv->nSustainStart, pEnv->nSustainEnd, pEnv->nLoopStart, pEnv->nLoopEnd, (pEnv->dwFlags & ENV_SUSTAIN) ? 1 : 0, (pEnv->dwFlags & ENV_LOOP) ? 1 : 0, (pEnv->dwFlags & ENV_CARRY) ? 1 : 0);
	for (UINT i = 0; i < pEnv->nNodes; i++)
	{
		if (strlen(s) >= sizeof(s)-32) break;
		wsprintf(s+strlen(s), "%d,%d\r\n", pEnv->Ticks[i], pEnv->Values[i]);
	}

	//Writing release node
	if(strlen(s) < sizeof(s) - 32)
		wsprintf(s+strlen(s), "%u\r\n", pEnv->nReleaseNode);

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
	return true;
}


bool CModDoc::PasteEnvelope(UINT nIns, enmEnvelopeTypes nEnv)
//-----------------------------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();

	if ((nIns < 1) || (nIns > m_SndFile.m_nInstruments) || (!m_SndFile.Instruments[nIns]) || (!pMainFrm)) return false;
	BeginWaitCursor();
	if (!pMainFrm->OpenClipboard())
	{
		EndWaitCursor();
		return false;
	}
	HGLOBAL hCpy = ::GetClipboardData(CF_TEXT);
	LPCSTR p;
	if ((hCpy) && ((p = (LPSTR)GlobalLock(hCpy)) != NULL))
	{
		MODINSTRUMENT *pIns = m_SndFile.Instruments[nIns];
		INSTRUMENTENVELOPE *pEnv = nullptr;

		UINT susBegin = 0, susEnd = 0, loopBegin = 0, loopEnd = 0, bSus = 0, bLoop = 0, bCarry = 0, nPoints = 0, releaseNode = ENV_RELEASE_NODE_UNSET;
		DWORD dwMemSize = GlobalSize(hCpy), dwPos = strlen(pszEnvHdr);
		if ((dwMemSize > dwPos) && (!_strnicmp(p, pszEnvHdr, dwPos - 2)))
		{
			sscanf(p + dwPos, pszEnvFmt, &nPoints, &susBegin, &susEnd, &loopBegin, &loopEnd, &bSus, &bLoop, &bCarry);
			while ((dwPos < dwMemSize) && (p[dwPos] != '\r') && (p[dwPos] != '\n')) dwPos++;

			nPoints = min(nPoints, m_SndFile.GetModSpecifications().envelopePointsMax);
			if (susEnd >= nPoints) susEnd = 0;
			if (susBegin > susEnd) susBegin = susEnd;
			if (loopEnd >= nPoints) loopEnd = 0;
			if (loopBegin > loopEnd) loopBegin = loopEnd;

			switch(nEnv)
			{
			case ENV_PANNING:
				pEnv = &pIns->PanEnv;
				break;
			case ENV_PITCH:
				pEnv = &pIns->PitchEnv;
				break;
			default:
				pEnv = &pIns->VolEnv;
				break;
			}
			pEnv->nNodes = nPoints;
			pEnv->nSustainStart = susBegin;
			pEnv->nSustainEnd = susEnd;
			pEnv->nLoopStart = loopBegin;
			pEnv->nLoopEnd = loopEnd;
			pEnv->nReleaseNode = releaseNode;
			pEnv->dwFlags = (pEnv->dwFlags & ~(ENV_LOOP|ENV_SUSTAIN|ENV_CARRY)) | (bLoop ? ENV_LOOP : 0) | (bSus ? ENV_SUSTAIN : 0) | (bCarry ? ENV_CARRY: 0) | (nPoints > 0 ? ENV_ENABLED : 0);

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
				if ((n1 < oldn) || (n1 > ENVELOPE_MAX_LENGTH)) n1 = oldn + 1;
				pEnv->Ticks[i] = (WORD)n1;
				pEnv->Values[i] = (BYTE)n2;
				oldn = n1;
				while ((dwPos < dwMemSize) && (p[dwPos] != '\r') && (p[dwPos] != '\n')) dwPos++;
				if (dwPos >= dwMemSize) break;
			}

			//Read releasenode information.
			if(dwPos < dwMemSize)
			{
				BYTE r = static_cast<BYTE>(atoi(p + dwPos));
				if(r == 0 || r >= nPoints || !m_SndFile.GetModSpecifications().hasReleaseNode)
					r = ENV_RELEASE_NODE_UNSET;
				pEnv->nReleaseNode = r;
			}
		}
		GlobalUnlock(hCpy);
		CloseClipboard();
		SetModified();
		UpdateAllViews(NULL, (nIns << HINT_SHIFT_INS) | HINT_ENVELOPE, NULL);
	}
	EndWaitCursor();
	return true;
}


// Check which channels contain note data. maxRemoveCount specified how many empty channels are reported at max.
void CModDoc::CheckUsedChannels(vector<bool> &usedMask, CHANNELINDEX maxRemoveCount) const
//----------------------------------------------------------------------------------------
{
	// Checking for unused channels
	const int nChannels = GetNumChannels();
	usedMask.resize(nChannels);
	for(int iRst = nChannels - 1; iRst >= 0; iRst--)
	{
		usedMask[iRst] = !IsChannelUnused(iRst);
		if(!usedMask[iRst])
		{
			// Found enough empty channels yet?
			if((--maxRemoveCount) == 0) break;
		}
	}
}


// Check if a given channel contains note data.
bool CModDoc::IsChannelUnused(CHANNELINDEX nChn) const
//----------------------------------------------------
{
	const CHANNELINDEX nChannels = GetNumChannels();
	if(nChn >= nChannels)
	{
		return true;
	}
	for(PATTERNINDEX nPat = 0; nPat < m_SndFile.Patterns.Size(); nPat++)
	{
		if(m_SndFile.Patterns.IsValidPat(nPat))
		{
			const MODCOMMAND *p = m_SndFile.Patterns[nPat] + nChn;
			for(ROWINDEX nRow = m_SndFile.Patterns[nPat].GetNumRows(); nRow > 0; nRow--, p += nChannels)
			{
				if(!p->IsEmpty())
				{
					return false;
				}
			}
		}
	}
	return true;
}


// Convert the module's restart position information to a pattern command.
bool CModDoc::RestartPosToPattern()
//---------------------------------
{
	bool result = false;
	GetLengthType length = m_SndFile.GetLength(eNoAdjust);
	if(length.endOrder != ORDERINDEX_INVALID && length.endRow != ROWINDEX_INVALID)
	{
		result = m_SndFile.TryWriteEffect(m_SndFile.Order[length.endOrder], length.endRow, CMD_POSITIONJUMP, m_SndFile.m_nRestartPos, false, CHANNELINDEX_INVALID, false, weTryNextRow);
	}
	m_SndFile.m_nRestartPos = 0;
	return result;
}
