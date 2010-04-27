// modedit.cpp : CModDoc operations
//

#include "stdafx.h"
#include "mptrack.h"
#include "mainfrm.h"
#include "moddoc.h"
#include "dlg_misc.h"
#include "dlsbank.h"
#include "modsmp_ctrl.h"
#include "misc_util.h"

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

BOOL CModDoc::ChangeModType(MODTYPE nNewType)
//-------------------------------------------
{
	CHAR s[256];
	UINT b64 = 0;

	const MODTYPE nOldType = m_SndFile.GetType();
	
	if (nNewType == nOldType && nNewType == MOD_TYPE_IT){
		// Even if m_nType doesn't change, we might need to change extension in itp<->it case.
		// This is because ITP is a HACK and doesn't genuinely change m_nType,
		// but uses flages instead.
		ChangeFileExtension(nNewType);
		return TRUE;
	}

	if(nNewType == nOldType) return TRUE;

	const bool oldTypeIsMOD = (nOldType == MOD_TYPE_MOD), oldTypeIsXM = (nOldType == MOD_TYPE_XM),
				oldTypeIsS3M = (nOldType == MOD_TYPE_S3M), oldTypeIsIT = (nOldType == MOD_TYPE_IT),
				oldTypeIsMPT = (nOldType == MOD_TYPE_MPT), oldTypeIsMOD_XM = (oldTypeIsMOD || oldTypeIsXM),
                oldTypeIsS3M_IT_MPT = (oldTypeIsS3M || oldTypeIsIT || oldTypeIsMPT),
                oldTypeIsIT_MPT = (oldTypeIsIT || oldTypeIsMPT);

	const bool newTypeIsMOD = (nNewType == MOD_TYPE_MOD), newTypeIsXM =  (nNewType == MOD_TYPE_XM), 
				newTypeIsS3M = (nNewType == MOD_TYPE_S3M), newTypeIsIT = (nNewType == MOD_TYPE_IT),
				newTypeIsMPT = (nNewType == MOD_TYPE_MPT), newTypeIsMOD_XM = (newTypeIsMOD || newTypeIsXM), 
				newTypeIsS3M_IT_MPT = (newTypeIsS3M || newTypeIsIT || newTypeIsMPT), 
				newTypeIsXM_IT_MPT = (newTypeIsXM || newTypeIsIT || newTypeIsMPT),
				newTypeIsIT_MPT = (newTypeIsIT || newTypeIsMPT);

	const CModSpecifications& specs = m_SndFile.GetModSpecifications(nNewType);

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
		-Multiple sequences

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
			if(m_SndFile.PatternSize[i] < 64)
			{
				// try to save short patterns by inserting a pattern break.
				m_SndFile.TryWriteEffect(i, m_SndFile.PatternSize[i] - 1, CMD_PATTERNBREAK, 0, false, CHANNELINDEX_INVALID, false, true);
			}
			m_SndFile.Patterns[i].Resize(64, false);
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
			m_SndFile.Chn[i1].pModInstrument = nullptr;
		}
		for (UINT i2=0; i2<m_SndFile.m_nInstruments; i2++) if (m_SndFile.Instruments[i2])
		{
			delete m_SndFile.Instruments[i2];
			m_SndFile.Instruments[i2] = nullptr;
		}
		m_SndFile.m_nInstruments = 0;
		END_CRITICAL();
		EndWaitCursor();
	} //End if (((m_SndFile.m_nInstruments) || (b64)) && (nNewType & (MOD_TYPE_MOD|MOD_TYPE_S3M)))
	BeginWaitCursor();


	/////////////////////////////
	// Converting pattern data

	for (PATTERNINDEX nPat = 0; nPat < m_SndFile.Patterns.Size(); nPat++) if (m_SndFile.Patterns[nPat])
	{
		MODCOMMAND *m = m_SndFile.Patterns[nPat];

		// This is used for -> MOD/XM conversion
		vector<vector<MODCOMMAND::PARAM> > cEffectMemory;
		cEffectMemory.resize(m_SndFile.GetNumChannels());
		for(size_t i = 0; i < m_SndFile.GetNumChannels(); i++)
		{
			cEffectMemory[i].resize(MAX_EFFECTS, 0);
		}

		UINT nChannel = m_SndFile.m_nChannels - 1;

		for (UINT len = m_SndFile.PatternSize[nPat] * m_SndFile.m_nChannels; len; m++, len--)
		{
			nChannel = (nChannel + 1) % m_SndFile.m_nChannels; // 0...Channels - 1

			m_SndFile.ConvertCommand(m, nOldType, nNewType);

			// Deal with effect memory for MOD/XM arpeggio
			if (oldTypeIsS3M_IT_MPT && newTypeIsMOD_XM)
			{
				switch(m->command)
				{
				case CMD_ARPEGGIO:
				case CMD_S3MCMDEX:
				case CMD_MODCMDEX:
					// No effect memory in XM / MOD
					if(m->param == 0)
						m->param = cEffectMemory[nChannel][m->command];
					else
						cEffectMemory[nChannel][m->command] = m->param;
					break;
				}
			}

			// Adjust effect memory for MOD files
			if(newTypeIsMOD)
			{
				switch(m->command)
				{
				case CMD_PORTAMENTOUP:
				case CMD_PORTAMENTODOWN:
				case CMD_TONEPORTAVOL:
				case CMD_VIBRATOVOL:
				case CMD_VOLUMESLIDE:
					// ProTracker doesn't have effect memory for these commands, so let's try to fix them
					if(m->param == 0)
						m->param = cEffectMemory[nChannel][m->command];
					else
						cEffectMemory[nChannel][m->command] = m->param;
					break;				

				}
			}
		}
	}

	////////////////////////////////////////////////
	// Converting instrument / sample / etc. data


	// Do some sample conversion
	for (SAMPLEINDEX nSmp = 1; nSmp <= m_SndFile.m_nSamples; nSmp++)
	{
		// No Sustain loops for MOD/S3M
		if(newTypeIsMOD || newTypeIsS3M)
		{
			m_SndFile.Samples[nSmp].nSustainStart = m_SndFile.Samples[nSmp].nSustainEnd = 0;
		}

		// Transpose to Frequency (MOD/XM to S3M/IT/MPT)
		if(oldTypeIsMOD_XM && newTypeIsS3M_IT_MPT)
		{
			m_SndFile.Samples[nSmp].nC5Speed = CSoundFile::TransposeToFrequency(m_SndFile.Samples[nSmp].RelativeTone, m_SndFile.Samples[nSmp].nFineTune);
			m_SndFile.Samples[nSmp].RelativeTone = 0;
			m_SndFile.Samples[nSmp].nFineTune = 0;
		}

		// Frequency to Transpose (S3M/IT/MPT to MOD/XM)
		if(oldTypeIsS3M_IT_MPT && newTypeIsXM)
		{
			CSoundFile::FrequencyToTranspose(&m_SndFile.Samples[nSmp]);
			if (!(m_SndFile.Samples[nSmp].uFlags & CHN_PANNING)) m_SndFile.Samples[nSmp].nPan = 128;
		}

		if(oldTypeIsXM && newTypeIsIT_MPT)
		{
			// Autovibrato settings (XM to IT, where sweep 0 means "no vibrato")
			if(m_SndFile.Samples[nSmp].nVibSweep == 0 && m_SndFile.Samples[nSmp].nVibRate != 0 && m_SndFile.Samples[nSmp].nVibDepth != 0)
				m_SndFile.Samples[nSmp].nVibSweep = 255;
		} else if(oldTypeIsIT_MPT && newTypeIsXM)
		{
			// Autovibrato settings (IT to XM, where sweep 0 means "no sweep")
			if(m_SndFile.Samples[nSmp].nVibSweep == 0)
				m_SndFile.Samples[nSmp].nVibRate = m_SndFile.Samples[nSmp].nVibDepth = 0;
		}
	}

	// No Autovibrato for MOD/S3M
	if(newTypeIsMOD || newTypeIsS3M)
	{
		ctrlSmp::ResetSamples(m_SndFile, ctrlSmp::SmpResetVibrato);
	}

	if (oldTypeIsXM && newTypeIsIT_MPT) m_SndFile.m_dwSongFlags |= SONG_ITCOMPATMODE;

	// Convert IT/MPT to XM (instruments)
	if (oldTypeIsIT_MPT && newTypeIsXM)
	{
		bool bBrokenNoteMap = false, bBrokenSustainLoop = false;
		for(INSTRUMENTINDEX nIns = 1; nIns <= m_SndFile.m_nInstruments; nIns++)
		{
			MODINSTRUMENT *pIns = m_SndFile.Instruments[nIns];
			if (pIns)
			{
				for (UINT k = 0; k < NOTE_MAX; k++)
				{
					if ((pIns->NoteMap[k]) && (pIns->NoteMap[k] != (BYTE)(k+1)))
					{
						bBrokenNoteMap = true;
						break;
					}
				}
				// Convert sustain loops to sustain "points"
				if(pIns->VolEnv.nSustainStart != pIns->VolEnv.nSustainEnd)
				{
					pIns->VolEnv.nSustainEnd = pIns->VolEnv.nSustainStart;
					bBrokenSustainLoop = true;
				}
				if(pIns->PanEnv.nSustainStart != pIns->PanEnv.nSustainEnd)
				{
					pIns->PanEnv.nSustainEnd = pIns->PanEnv.nSustainStart;
					bBrokenSustainLoop = true;
				}
				pIns->VolEnv.dwFlags &= ~ENV_CARRY;
				pIns->PanEnv.dwFlags &= ~ENV_CARRY;
				pIns->PitchEnv.dwFlags &= ~(ENV_CARRY|ENV_ENABLED|ENV_FILTER);
				pIns->dwFlags &= ~INS_SETPANNING;
				pIns->nIFC &= 0x7F;
				pIns->nIFR &= 0x7F;
			}
		}
		if (bBrokenNoteMap) AddToLog("WARNING: Note Mapping will be lost when saving as XM.\n");
		if (bBrokenSustainLoop) AddToLog("WARNING: Sustain loops were converted to sustain points.\n");
	}

	if(newTypeIsMOD)
	{
		// Not supported in MOD format
		m_SndFile.m_nDefaultSpeed = 6;
		m_SndFile.m_nDefaultTempo = 125;
		m_SndFile.m_nDefaultGlobalVolume = 256;
		m_SndFile.m_nSamplePreAmp = 48;
		m_SndFile.m_nVSTiVolume = 48;
		AddToLog("WARNING: Default speed, tempo and global volume will be lost.\n");

		// Too many samples?
		if(m_SndFile.m_nSamples > 31)
		{
			AddToLog("WARNING: Samples above 31 will be lost when saving as MOD!\n");
		}
	}

	// Is the "restart position" value allowed in this format?
	if(m_SndFile.m_nRestartPos > 0 && !CSoundFile::GetModSpecifications(nNewType).hasRestartPos)
	{
		m_SndFile.m_nRestartPos = 0;
		AddToLog("WARNING: Restart position is not support by the new format.\n");
	}


	// Fix channel settings (pan/vol)
	for(CHANNELINDEX nChn = 0; nChn < m_SndFile.m_nChannels; nChn++)
	{
		if(newTypeIsMOD_XM || newTypeIsS3M)
		{
			m_SndFile.ChnSettings->nVolume = 64;
			m_SndFile.ChnSettings->dwFlags &= ~CHN_SURROUND;
		}
		if(newTypeIsXM)
		{
			m_SndFile.ChnSettings->nPan = 128;
		}
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
	if (!newTypeIsMOD) m_SndFile.m_dwSongFlags &= ~SONG_PT1XMODE;
	if (newTypeIsS3M || newTypeIsMOD) m_SndFile.m_dwSongFlags &= ~SONG_EXFILTERRANGE;
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

	// Check mod specifications
	m_SndFile.m_nDefaultTempo = CLAMP(m_SndFile.m_nDefaultTempo, specs.tempoMin, specs.tempoMax);
	m_SndFile.m_nDefaultSpeed = CLAMP(m_SndFile.m_nDefaultSpeed, specs.speedMin, specs.speedMax);

	bool bTrimmedEnvelopes = false;
	for(INSTRUMENTINDEX i = 1; i <= m_SndFile.m_nInstruments; i++) if(m_SndFile.Instruments[i] != nullptr)
	{
		bTrimmedEnvelopes |= UpdateEnvelopes(&(m_SndFile.Instruments[i]->VolEnv));
		bTrimmedEnvelopes |= UpdateEnvelopes(&(m_SndFile.Instruments[i]->PanEnv));
		bTrimmedEnvelopes |= UpdateEnvelopes(&(m_SndFile.Instruments[i]->PitchEnv));
	}
	if(bTrimmedEnvelopes == true)
		AddToLog("WARNING: Instrument envelopes have been shortened.\n");

	SetModified();
	GetPatternUndo()->ClearUndo();
	GetSampleUndo()->ClearUndo();
	UpdateAllViews(NULL, HINT_MODTYPE | HINT_MODGENERAL);
	EndWaitCursor();
	return TRUE;
}

bool CModDoc::UpdateEnvelopes(INSTRUMENTENVELOPE *mptEnv)
//-------------------------------------------------------
{
	// shorten instrument envelope if necessary (for mod conversion)
	const UINT iEnvMax = m_SndFile.GetModSpecifications().envelopePointsMax;
	bool bResult = false;

	#define TRIMENV(i) if(i > iEnvMax) {i = iEnvMax; bResult = true;}

	TRIMENV(mptEnv->nNodes);
	TRIMENV(mptEnv->nLoopStart);
	TRIMENV(mptEnv->nLoopEnd);
	TRIMENV(mptEnv->nSustainStart);
	TRIMENV(mptEnv->nSustainEnd);
	if(mptEnv->nReleaseNode != ENV_RELEASE_NODE_UNSET) TRIMENV(mptEnv->nReleaseNode);

	#undef TRIMENV

	return bResult;
}






// Change the number of channels
BOOL CModDoc::ChangeNumChannels(UINT nNewChannels, const bool showCancelInRemoveDlg)
//----------------------------------------------------------------------------------
{
	const CHANNELINDEX maxChans = m_SndFile.GetModSpecifications().channelsMax;

	if (nNewChannels > maxChans) {
		CString error;
		error.Format("Error: Max number of channels for this file type is %d", maxChans);
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

		//if channel was removed before and is added again, mute status has to be unset! (bug 1814)
		for (UINT i=m_SndFile.m_nChannels; i<nNewChannels; i++)
		{
			m_SndFile.InitChannel(i);
		}
	
		m_SndFile.m_nChannels = nNewChannels;
		m_SndFile.SetupMODPanning();
		END_CRITICAL();
		EndWaitCursor();
	}
	SetModified();
	GetPatternUndo()->ClearUndo();
	UpdateAllViews(NULL, HINT_MODTYPE);
	return TRUE;
}


BOOL CModDoc::RemoveChannels(BOOL m_bChnMask[MAX_CHANNELS])
//---------------------------------------------------------
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
					m_SndFile.InitChannel(i);
					m_SndFile.Chn[i].dwFlags |= CHN_MUTE;
				}
			}
		}
		m_SndFile.m_nChannels = nRemainingChannels;
		END_CRITICAL();
		EndWaitCursor();
		SetModified();
		GetPatternUndo()->ClearUndo();
		UpdateAllViews(NULL, HINT_MODTYPE);
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
			if ((instr < MAX_INSTRUMENTS) && (m_SndFile.Instruments[instr]))
			{
				MODINSTRUMENT *pIns = m_SndFile.Instruments[instr];
				newins = pIns->Keyboard[note];
				if (newins >= MAX_SAMPLES) newins = 0;
			}
			p->instr = newins;
		}
	}
	return TRUE;
}



UINT CModDoc::RemovePlugs(const bool (&keepMask)[MAX_MIXPLUGINS])
//---------------------------------------------------------------
{
	//Remove all plugins whose keepMask[plugindex] is false.
	UINT nRemoved=0;
	for (PLUGINDEX nPlug=0; nPlug<MAX_MIXPLUGINS; nPlug++)
	{
		SNDMIXPLUGIN* pPlug = &m_SndFile.m_MixPlugins[nPlug];		
		if (keepMask[nPlug] || !pPlug)
		{
			Log("Keeping mixplug addess (%d): %X\n", nPlug, &(pPlug->pMixPlugin));	
			continue;
		}

		if (pPlug->pPluginData)
		{
			delete pPlug->pPluginData;
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

		memset(&(pPlug->Info), 0, sizeof(SNDMIXPLUGININFO));
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
	MODSAMPLE *pSmp;
	if (nSample >= MAX_SAMPLES) return FALSE;
	pSmp = &m_SndFile.Samples[nSample];
	if ((!pSmp->nLength) || (!pSmp->pSample)) return FALSE;

	ctrlSmp::AdjustEndOfSample(*pSmp, &m_SndFile);

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
		if ((!m_SndFile.m_szNames[i][0]) && (m_SndFile.Samples[i].pSample == NULL))
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
	MODSAMPLE *pSmp = &m_SndFile.Samples[i];
	pSmp->nVolume = 256;
	pSmp->nGlobalVol = 64;
	pSmp->nPan = 128;
	pSmp->nC5Speed = 8363;
	pSmp->RelativeTone = 0;
	pSmp->nFineTune = 0;
	pSmp->nVibType = 0;
	pSmp->nVibSweep = 0;
	pSmp->nVibDepth = 0;
	pSmp->nVibRate = 0;
	pSmp->uFlags &= ~(CHN_PANNING|CHN_SUSTAINLOOP);
	if (m_SndFile.m_nType == MOD_TYPE_XM) pSmp->uFlags |= CHN_PANNING;
	if (i > m_SndFile.m_nSamples) m_SndFile.m_nSamples = i;
	SetModified();
	return i;
}


INSTRUMENTINDEX CModDoc::InsertInstrument(LONG lSample, LONG lDuplicate)
//----------------------------------------------------------------------
{
	MODINSTRUMENT *pDup = NULL;
	INSTRUMENTINDEX nInstrumentMax = m_SndFile.GetModSpecifications().instrumentsMax - 1;
	if ((m_SndFile.m_nType != MOD_TYPE_XM) && !(m_SndFile.m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT))) return INSTRUMENTINDEX_INVALID;
	if ((lDuplicate > 0) && (lDuplicate <= (LONG)m_SndFile.m_nInstruments))
	{
		pDup = m_SndFile.Instruments[lDuplicate];
	}
	if ((!m_SndFile.m_nInstruments) && ((m_SndFile.m_nSamples > 1) || (m_SndFile.Samples[1].pSample)))
	{
		if (pDup) return INSTRUMENTINDEX_INVALID;
		UINT n = CMainFrame::GetMainFrame()->MessageBox("Convert existing samples to instruments first?", NULL, MB_YESNOCANCEL|MB_ICONQUESTION);
		if (n == IDYES)
		{
			UINT nInstruments = m_SndFile.m_nSamples;
			if (nInstruments > nInstrumentMax) nInstruments = nInstrumentMax;
			for (UINT smp=1; smp<=nInstruments; smp++)
			{
				m_SndFile.Samples[smp].uFlags &= ~CHN_MUTE;
				if (!m_SndFile.Instruments[smp])
				{
					MODINSTRUMENT *p = new MODINSTRUMENT;
					if (!p)
					{
						ErrorBox(IDS_ERR_OUTOFMEMORY, CMainFrame::GetMainFrame());
						return INSTRUMENTINDEX_INVALID;
					}
					InitializeInstrument(p, smp);
					m_SndFile.Instruments[smp] = p;
					lstrcpyn(p->name, m_SndFile.m_szNames[smp], sizeof(p->name));
				}
			}
			m_SndFile.m_nInstruments = nInstruments;
		} else
		if (n != IDNO) return INSTRUMENTINDEX_INVALID;
	}
	UINT newins = 0;
	for (UINT i=1; i<=m_SndFile.m_nInstruments; i++)
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
		UINT newsmp = 0;
		if ((lSample > 0) && (lSample < m_SndFile.GetModSpecifications().samplesMax))
		{
			newsmp = lSample;
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
		BEGIN_CRITICAL();
		if (pDup)
		{
			*pIns = *pDup;
// -> CODE#0023
// -> DESC="IT project files (.itp)"
			strcpy(m_SndFile.m_szInstrumentPath[newins-1],m_SndFile.m_szInstrumentPath[lDuplicate-1]);
			m_SndFile.instrumentModified[newins-1] = FALSE;
// -! NEW_FEATURE#0023
		} else
		{
			InitializeInstrument(pIns, newsmp);
		}
		m_SndFile.Instruments[newins] = pIns;
		END_CRITICAL();
		SetModified();
	} else
	{
		ErrorBox(IDS_ERR_OUTOFMEMORY, CMainFrame::GetMainFrame());
		return INSTRUMENTINDEX_INVALID;
	}
	return newins;
}


void CModDoc::InitializeInstrument(MODINSTRUMENT *pIns, UINT nsample)
//-------------------------------------------------------------------
{
	memset(pIns, 0, sizeof(MODINSTRUMENT));
	pIns->nFadeOut = 256;
	pIns->nGlobalVol = 64;
	pIns->nPan = 128;
	pIns->nPPC = 5*12;
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

	BEGIN_CRITICAL();
	SEQUENCEINDEX nOldSeq = m_SndFile.Order.GetCurrentSequenceIndex();
	m_SndFile.Order.SetSequence(nSeq);
	for (ORDERINDEX i = nOrd; i < m_SndFile.Order.GetSequence(nSeq).size() - 1; i++)
	{
		m_SndFile.Order[i] = m_SndFile.Order[i + 1];
	}
	m_SndFile.Order[m_SndFile.Order.GetLastIndex()] = m_SndFile.Order.GetInvalidPatIndex();
	m_SndFile.Order.SetSequence(nOldSeq);
	END_CRITICAL();
	SetModified();
	return true;
}



bool CModDoc::RemovePattern(PATTERNINDEX nPat)
//--------------------------------------------
{
	if ((nPat < m_SndFile.Patterns.Size()) && (m_SndFile.Patterns[nPat]))
	{
		BEGIN_CRITICAL();
		LPVOID p = m_SndFile.Patterns[nPat];
		m_SndFile.Patterns[nPat] = nullptr;
		m_SndFile.SetPatternName(nPat, "");
		CSoundFile::FreePattern(p);
		END_CRITICAL();
		SetModified();
		return true;
	}
	return false;
}


bool CModDoc::RemoveSample(SAMPLEINDEX nSmp)
//------------------------------------------
{
	if ((nSmp) && (nSmp <= m_SndFile.m_nSamples))
	{
		BEGIN_CRITICAL();
		m_SndFile.DestroySample(nSmp);
		m_SndFile.m_szNames[nSmp][0] = 0;
		while ((m_SndFile.m_nSamples > 1)
		 && (!m_SndFile.m_szNames[m_SndFile.m_nSamples][0])
		 && (!m_SndFile.Samples[m_SndFile.m_nSamples].pSample)) m_SndFile.m_nSamples--;
		END_CRITICAL();
		SetModified();
		return true;
	}
	return false;
}


bool CModDoc::RemoveInstrument(INSTRUMENTINDEX nIns)
//--------------------------------------------------
{
	if ((nIns) && (nIns <= m_SndFile.m_nInstruments) && (m_SndFile.Instruments[nIns]))
	{
		BOOL bIns = FALSE;
		BEGIN_CRITICAL();
		m_SndFile.DestroyInstrument(nIns);
		if (nIns == m_SndFile.m_nInstruments) m_SndFile.m_nInstruments--;
		for (UINT i=1; i<MAX_INSTRUMENTS; i++) if (m_SndFile.Instruments[i]) bIns = TRUE;
		if (!bIns) m_SndFile.m_nInstruments = 0;
		END_CRITICAL();
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
// Hdr: "ModPlug Tracker S3M\n"
// Full:  '|C#401v64A06'
// Reset: '|...........'
// Empty: '|           '
// End of row: '\n'

static LPCSTR lpszClipboardPatternHdr = "ModPlug Tracker %3s\x0D\x0A";

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
						case NOTE_NONE:		p[1] = p[2] = p[3] = '.'; break;
						case NOTE_KEYOFF:	p[1] = p[2] = p[3] = '='; break;
						case NOTE_NOTECUT:	p[1] = p[2] = p[3] = '^'; break;
						case NOTE_FADE:	p[1] = p[2] = p[3] = '~'; break;
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
			MODCOMMAND *m = m_SndFile.Patterns[nPattern];
			UINT nrow = dwBeginSel >> 16;
			UINT ncol = (dwBeginSel & 0xFFFF) >> 3;
			UINT col;
			bool bS3MCommands = false, bOk = false;
			bool bPrepareUndo = true, bFirstUndo = true;
			MODTYPE origFormat = MOD_TYPE_IT;
			UINT len = 0, startLen;

			const bool doOverflowPaste = (CMainFrame::m_dwPatternSetup & PATTERN_OVERFLOWPASTE) && (pasteMode != pm_pasteflood) && (pasteMode != pm_pushforwardpaste);
			const bool doITStyleMix = (pasteMode == pm_mixpaste_it);
			const bool doMixPaste = ((pasteMode == pm_mixpaste) || doITStyleMix);

			ORDERINDEX oCurrentOrder; //jojo.echopaste
			ROWINDEX rTemp, startRow;
			PATTERNINDEX pTemp;
			GetEditPosition(rTemp, pTemp, oCurrentOrder);

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
					if(p[len - 3] == 'I') origFormat = MOD_TYPE_IT;
					if(p[len - 3] == 'P') origFormat = MOD_TYPE_MPT;
					if(p[len - 4] == 'S') origFormat = MOD_TYPE_S3M;
					if(p[len - 3] == 'X') origFormat = MOD_TYPE_XM;
					if(p[len - 3] == 'O') origFormat = MOD_TYPE_MOD;
					break;
				}
			}
			bS3MCommands = (origFormat & (MOD_TYPE_IT|MOD_TYPE_MPT|MOD_TYPE_S3M)) != 0 ? true : false;
			bOk = true;

			startLen = len;
			startRow = nrow;

			while ((nrow < m_SndFile.PatternSize[nPattern]))
			{
				// Search for column separator or end of paste data
				while ((len + 11 >= dwMemSize) || p[len] != '|')
				{
					if (len + 11 >= dwMemSize || !p[len])
					{
						if((pasteMode == pm_pasteflood) && (nrow != startRow)) // prevent infinite loop with malformed clipboard data
							len = startLen; // paste from beginning
						else
							goto PasteDone;
					} else
					{
						len++;
					}
				}
				col = ncol;
				// Paste columns
				while ((p[len] == '|') && (len + 11 < dwMemSize))
				{
					LPSTR s = p+len+1;

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
							GetPatternUndo()->PrepareUndo(nPattern, 0, 0, m_SndFile.m_nChannels, m_SndFile.PatternSize[nPattern], !bFirstUndo);
							bPrepareUndo = false;
							bFirstUndo = false;
						}
						
						// ITSyle mixpaste requires that we keep a copy of the thing we are about to paste on
						// so that we can refer back to check if there was anything in e.g. the note column before we pasted.
						const MODCOMMAND origModCmd = m[col];

						// push channel data below paste point first.
						if(pasteMode == pm_pushforwardpaste)
						{
							for(ROWINDEX nPushRow = m_SndFile.PatternSize[nPattern] - 1 - nrow; nPushRow > 0; nPushRow--)
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
									LPCSTR psc = (bS3MCommands) ? gszS3mCommands : gszModCommands;
									for (UINT i=1; i<MAX_EFFECTS; i++)
									{
										if ((s[8] == psc[i]) && (psc[i] != '?')) m[col].command = i;
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
						if (origFormat != m_SndFile.m_nType && (doMixPaste == false || origModCmd.IsEmpty()))
							m_SndFile.ConvertCommand(&(m[col]), origFormat, m_SndFile.m_nType);
					}

					len += 12;
					col++;
				}
				// Next row
				m += m_SndFile.m_nChannels;
				nrow++;

				// Overflow paste. Continue pasting in next pattern if enabled.
				// If Paste Flood is enabled, this won't be called due to obvious reasons.
				if(doOverflowPaste)
				{
					while(nrow >= m_SndFile.PatternSize[nPattern])
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

static LPCSTR pszEnvHdr = "Modplug Tracker Envelope\x0D\x0A";
static LPCSTR pszEnvFmt = "%d,%d,%d,%d,%d,%d,%d,%d\x0D\x0A";

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

	strcpy(s, pszEnvHdr);
	wsprintf(s + strlen(s), pszEnvFmt, pEnv->nNodes, pEnv->nSustainStart, pEnv->nSustainEnd, pEnv->nLoopStart, pEnv->nLoopEnd, (pEnv->dwFlags & ENV_SUSTAIN) ? 1 : 0, (pEnv->dwFlags & ENV_LOOP) ? 1 : 0, (pEnv->dwFlags & ENV_CARRY) ? 1 : 0);
	for (UINT i = 0; i < pEnv->nNodes; i++)
	{
		if (strlen(s) >= sizeof(s)-32) break;
		wsprintf(s+strlen(s), "%d,%d\x0D\x0A", pEnv->Ticks[i], pEnv->Values[i]);
	}

	//Writing release node
	if(strlen(s) < sizeof(s) - 32)
		wsprintf(s+strlen(s), "%u\x0D\x0A", pEnv->nReleaseNode);

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
			pEnv->dwFlags = (pEnv->dwFlags & ~(ENV_LOOP|ENV_SUSTAIN|ENV_CARRY)) | (bLoop ? ENV_LOOP : 0) | (bSus ? ENV_SUSTAIN : 0) | (bCarry ? ENV_CARRY: 0) | ENV_ENABLED;

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
				pEnv->Ticks[i] = (WORD)n1;
				pEnv->Values[i] = (BYTE)n2;
				oldn = n1;
				while ((dwPos < dwMemSize) && (p[dwPos] != 0x0D)) dwPos++;
				if (dwPos >= dwMemSize) break;
			}

			//Read releasenode information.
			if(dwPos < dwMemSize)
			{
				BYTE r = static_cast<BYTE>(atoi(p + dwPos));
				if(r == 0 || r >= nPoints) r = ENV_RELEASE_NODE_UNSET;
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


void CModDoc::CheckUnusedChannels(BOOL mask[MAX_CHANNELS], CHANNELINDEX maxRemoveCount)
//-------------------------------------------------------------------------------------
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


