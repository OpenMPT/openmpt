/*
 * AppendModule.cpp
 * ----------------
 * Purpose: Appending one module to an existing module
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Moddoc.h"
#include "../soundlib/mod_specifications.h"


OPENMPT_NAMESPACE_BEGIN

// Add samples, instruments, plugins and patterns from another module to the current module
void CModDoc::AppendModule(const CSoundFile &source)
{
	const CModSpecifications &specs = m_SndFile.GetModSpecifications();
	// Mappings between old and new indices
	std::vector<PLUGINDEX> pluginMapping(MAX_MIXPLUGINS + 1, 0);
	std::vector<INSTRUMENTINDEX> instrMapping((source.GetNumInstruments() ? source.GetNumInstruments() : source.GetNumSamples()) + 1, INSTRUMENTINDEX_INVALID);
	std::vector<ORDERINDEX> orderMapping;
	std::vector<PATTERNINDEX> patternMapping(source.Patterns.GetNumPatterns(), PATTERNINDEX_INVALID);

	///////////////////////////////////////////////////////////////////////////
	// Copy plugins
#ifndef NO_PLUGINS
	if(specs.supportsPlugins)
	{
		PLUGINDEX plug = 0;
		for(PLUGINDEX i = 0; i < MAX_MIXPLUGINS; i++)
		{
			if(!source.m_MixPlugins[i].IsValidPlugin())
			{
				continue;
			}
			while(plug < MAX_MIXPLUGINS && m_SndFile.m_MixPlugins[plug].IsValidPlugin())
			{
				plug++;
			}
			if(plug < MAX_MIXPLUGINS)
			{
				ClonePlugin(m_SndFile.m_MixPlugins[plug], source.m_MixPlugins[i]);
				pluginMapping[i + 1] = plug + 1;
			} else
			{
				AddToLog("Too many plugins!");
				break;
			}
		}
		// Fix up references between plugins
		for(PLUGINDEX i = 0; i < MAX_MIXPLUGINS; i++)
		{
			if(pluginMapping[i + 1] != 0 && source.m_MixPlugins[i].GetOutputPlugin() < MAX_MIXPLUGINS)
			{
				m_SndFile.m_MixPlugins[pluginMapping[i + 1] - 1].SetOutputPlugin(pluginMapping[source.m_MixPlugins[i].GetOutputPlugin() + 1] - 1);
			}
		}
	}
#endif // NO_PLUGINS

	///////////////////////////////////////////////////////////////////////////
	// Copy samples / instruments
	if(source.GetNumInstruments() != 0 && m_SndFile.GetNumInstruments() == 0 && specs.instrumentsMax)
	{
		// Convert to instruments first
		ConvertSamplesToInstruments();
	}

	// Check which samples / instruments are actually referenced.
	for(const auto &pat : source.Patterns) if(pat.IsValid())
	{
		for(const auto &m : pat)
		{
			if(!m.IsPcNote() && m.instr < instrMapping.size()) instrMapping[m.instr] = 0;
		}
	}

	if(m_SndFile.GetNumInstruments())
	{
		INSTRUMENTINDEX targetIns = 0;
		if(source.GetNumInstruments())
		{
			for(INSTRUMENTINDEX i = 1; i <= source.GetNumInstruments(); i++) if(source.Instruments[i] != nullptr && !instrMapping[i])
			{
				targetIns = m_SndFile.GetNextFreeInstrument(targetIns + 1);
				if(targetIns == INSTRUMENTINDEX_INVALID)
				{
					AddToLog("Too many instruments!");
					break;
				}
				if(m_SndFile.ReadInstrumentFromSong(targetIns, source, i))
				{
					ModInstrument *ins = m_SndFile.Instruments[targetIns];
					if(ins->nMixPlug <= MAX_MIXPLUGINS)
					{
						ins->nMixPlug = pluginMapping[ins->nMixPlug];
					}
					instrMapping[i] = targetIns;
				}
			}
		} else
		{
			SAMPLEINDEX targetSmp = 0;
			for(SAMPLEINDEX i = 1; i <= source.GetNumSamples(); i++) if(!instrMapping[i])
			{
				targetIns = m_SndFile.GetNextFreeInstrument(targetIns + 1);
				targetSmp = m_SndFile.GetNextFreeSample(targetIns, targetSmp + 1);
				if(targetIns == INSTRUMENTINDEX_INVALID)
				{
					AddToLog("Too many instruments!");
					break;
				} else if(targetSmp == SAMPLEINDEX_INVALID)
				{
					AddToLog("Too many samples!");
					break;
				}
				if(m_SndFile.AllocateInstrument(targetIns, targetSmp) != nullptr)
				{
					m_SndFile.ReadSampleFromSong(targetSmp, source, i);
				}
				instrMapping[i] = targetIns;
			}
		}
	} else
	{
		SAMPLEINDEX targetSmp = 0;
		if(source.GetNumInstruments())
		{
			for(INSTRUMENTINDEX i = 1; i <= source.GetNumInstruments(); i++) if(source.Instruments[i] != nullptr && !instrMapping[i])
			{
				targetSmp = m_SndFile.GetNextFreeSample(INSTRUMENTINDEX_INVALID, targetSmp + 1);
				if(targetSmp == SAMPLEINDEX_INVALID)
				{
					AddToLog("Too many samples!");
					break;
				}
				m_SndFile.ReadSampleFromSong(targetSmp, source, source.Instruments[i]->Keyboard[NOTE_MIDDLEC - NOTE_MIN]);
				instrMapping[i] = targetSmp;
			}
		} else
		{
			for(SAMPLEINDEX i = 1; i <= source.GetNumSamples(); i++) if(!instrMapping[i])
			{
				targetSmp = m_SndFile.GetNextFreeSample(INSTRUMENTINDEX_INVALID, targetSmp + 1);
				if(targetSmp == SAMPLEINDEX_INVALID)
				{
					AddToLog("Too many samples!");
					break;
				}
				m_SndFile.ReadSampleFromSong(targetSmp, source, i);
				instrMapping[i] = targetSmp;
			}
		}
	}

	///////////////////////////////////////////////////////////////////////////
	// Copy order lists
	const bool useOrderMapping = source.Order.GetNumSequences() == 1;
	for(auto &srcOrder : source.Order)
	{
		ORDERINDEX insertPos = 0;
		if(m_SndFile.Order.GetNumSequences() < specs.sequencesMax)
		{
			m_SndFile.Order.AddSequence();
			m_SndFile.Order().SetName(srcOrder.GetName());
		} else
		{
			insertPos = m_SndFile.Order().GetLengthTailTrimmed();
			if(specs.hasStopIndex)
				insertPos++;
		}
		const ORDERINDEX ordLen = srcOrder.GetLengthTailTrimmed();
		if(useOrderMapping) orderMapping.resize(ordLen, ORDERINDEX_INVALID);
		for(ORDERINDEX ord = 0; ord < ordLen; ord++)
		{
			if(insertPos >= specs.ordersMax)
			{
				AddToLog("Too many order items!");
				break;
			}

			PATTERNINDEX insertPat = PATTERNINDEX_INVALID;
			PATTERNINDEX srcPat = srcOrder[ord];
			if(source.Patterns.IsValidPat(srcPat) && srcPat < patternMapping.size())
			{
				if(patternMapping[srcPat] == PATTERNINDEX_INVALID && source.Patterns.IsValidPat(srcPat))
				{
					patternMapping[srcPat] = InsertPattern(Clamp(source.Patterns[srcPat].GetNumRows(), specs.patternRowsMin, specs.patternRowsMax));
					if(patternMapping[srcPat] == PATTERNINDEX_INVALID)
					{
						AddToLog("Too many patterns!");
						break;
					}
				}
				if(patternMapping[srcPat] == PATTERNINDEX_INVALID)
				{
					continue;
				}
				insertPat = patternMapping[srcPat];
			} else if(srcPat == srcOrder.GetIgnoreIndex() && specs.hasIgnoreIndex)
			{
				insertPat = m_SndFile.Order.GetIgnoreIndex();
			} else if(srcPat == srcOrder.GetInvalidPatIndex() && specs.hasStopIndex)
			{
				insertPat = m_SndFile.Order.GetInvalidPatIndex();
			} else
			{
				continue;
			}
			m_SndFile.Order().insert(insertPos, 1, insertPat);
			if(useOrderMapping) orderMapping[ord] = insertPos++;
		}
	}

	///////////////////////////////////////////////////////////////////////////
	// Adjust number of channels
	if(source.GetNumChannels() > m_SndFile.GetNumChannels())
	{
		CHANNELINDEX newChn = source.GetNumChannels();
		if(newChn > specs.channelsMax)
		{
			AddToLog("Too many channels!");
			newChn = specs.channelsMax;
		}
		if(newChn > m_SndFile.GetNumChannels())
		{
			ChangeNumChannels(newChn, false);
		}
	}

	///////////////////////////////////////////////////////////////////////////
	// Copy patterns
	const bool tempoSwingDiffers = source.m_tempoSwing != m_SndFile.m_tempoSwing;
	const bool timeSigDiffers = source.m_nDefaultRowsPerBeat != m_SndFile.m_nDefaultRowsPerBeat || source.m_nDefaultRowsPerMeasure != m_SndFile.m_nDefaultRowsPerMeasure;
	const CHANNELINDEX copyChannels = std::min(m_SndFile.GetNumChannels(), source.GetNumChannels());
	for(PATTERNINDEX pat = 0; pat < patternMapping.size(); pat++)
	{
		if(patternMapping[pat] == PATTERNINDEX_INVALID)
		{
			continue;
		}

		const CPattern &sourcePat = source.Patterns[pat];
		CPattern &targetPat = m_SndFile.Patterns[patternMapping[pat]];

		if(specs.hasPatternNames)
		{
			targetPat.SetName(sourcePat.GetName());
		}
		if(specs.hasPatternSignatures)
		{
			if(sourcePat.GetOverrideSignature())
			{
				targetPat.SetSignature(sourcePat.GetRowsPerBeat(), sourcePat.GetRowsPerMeasure());
			} else if(timeSigDiffers)
			{
				// Try fixing differing signature settings by copying them to the newly created patterns
				targetPat.SetSignature(source.m_nDefaultRowsPerBeat, source.m_nDefaultRowsPerMeasure);
			}
		}
		if(m_SndFile.m_nTempoMode == TempoMode::Modern)
		{
			// Swing only works in modern tempo mode
			if(sourcePat.HasTempoSwing())
			{
				targetPat.SetTempoSwing(sourcePat.GetTempoSwing());
			} else if(tempoSwingDiffers)
			{
				// Try fixing differing swing settings by copying them to the newly created patterns
				targetPat.SetSignature(source.m_nDefaultRowsPerBeat, source.m_nDefaultRowsPerMeasure);
				targetPat.SetTempoSwing(source.m_tempoSwing);
			}
		}

		const ROWINDEX copyRows = std::min(sourcePat.GetNumRows(), targetPat.GetNumRows());
		for(ROWINDEX row = 0; row < copyRows; row++)
		{
			const ModCommand *src = sourcePat.GetpModCommand(row, 0);
			ModCommand *m = targetPat.GetpModCommand(row, 0);
			for(CHANNELINDEX chn = 0; chn < copyChannels; chn++, src++, m++)
			{
				*m = *src;
				m->Convert(source.GetType(), m_SndFile.GetType(), source);
				if(m->IsPcNote())
				{
					if(m->instr && m->instr < pluginMapping.size()) m->instr = static_cast<ModCommand::INSTR>(pluginMapping[m->instr]);
				} else
				{
					if(m->instr && m->instr < instrMapping.size()) m->instr = static_cast<ModCommand::INSTR>(instrMapping[m->instr]);

					if(m->command == CMD_POSITIONJUMP && m->param < orderMapping.size())
					{
						if(orderMapping[m->param] == ORDERINDEX_INVALID)
						{
							m->command = CMD_NONE;
						} else
						{
							m->param = static_cast<ModCommand::PARAM>(orderMapping[m->param]);
						}
					}
				}
			}
		}
		if(copyRows < targetPat.GetNumRows())
		{
			// If source pattern was smaller, write pattern break effect.
			targetPat.WriteEffect(EffectWriter(CMD_PATTERNBREAK, 0).Row(copyRows - 1).RetryNextRow());
		}
	}
}

OPENMPT_NAMESPACE_END
