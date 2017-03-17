/*
 * Pattern.h
 * ---------
 * Purpose: Module Pattern header class
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include <vector>
#include "modcommand.h"
#include "Snd_defs.h"


OPENMPT_NAMESPACE_BEGIN


class CPatternContainer;
class CSoundFile;
class EffectWriter;

typedef ModCommand* PatternRow;


//============
class CPattern
//============
{
	friend class CPatternContainer;
	
public:
//BEGIN: OPERATORS
	//To mimic ModCommand*
	operator ModCommand*() { return m_ModCommands; }
	operator const ModCommand*() const { return m_ModCommands; }
	CPattern& operator=(ModCommand* const p) { m_ModCommands = p; return *this; }
	CPattern& operator=(const CPattern& pat)
	{
		m_ModCommands = pat.m_ModCommands;
		m_Rows = pat.m_Rows;
		m_RowsPerBeat = pat.m_RowsPerBeat;
		m_RowsPerMeasure = pat.m_RowsPerMeasure;
		m_PatternName = pat.m_PatternName;
		return *this;
	}

	bool operator== (const CPattern &other) const;
//END: OPERATORS

//BEGIN: INTERFACE METHODS
public:
	ModCommand* GetpModCommand(const ROWINDEX r, const CHANNELINDEX c) { return &m_ModCommands[r * GetNumChannels() + c]; }
	const ModCommand* GetpModCommand(const ROWINDEX r, const CHANNELINDEX c) const { return &m_ModCommands[r * GetNumChannels() + c]; }
	
	ROWINDEX GetNumRows() const { return m_Rows; }
	ROWINDEX GetRowsPerBeat() const { return m_RowsPerBeat; }			// pattern-specific rows per beat
	ROWINDEX GetRowsPerMeasure() const { return m_RowsPerMeasure; }		// pattern-specific rows per measure
	bool GetOverrideSignature() const { return (m_RowsPerBeat + m_RowsPerMeasure > 0); }	// override song time signature?

	// Return true if modcommand can be accessed from given row, false otherwise.
	bool IsValidRow(const ROWINDEX row) const { return (row < GetNumRows()); }

	// Return PatternRow object which has operator[] defined so that ModCommand
	// at (iRow, iChn) can be accessed with GetRow(iRow)[iChn].
	PatternRow GetRow(const ROWINDEX row) { return GetpModCommand(row, 0); }
	PatternRow GetRow(const ROWINDEX row) const { return const_cast<ModCommand *>(GetpModCommand(row, 0)); }

	CHANNELINDEX GetNumChannels() const;

	// Add or remove rows from the pattern.
	bool Resize(const ROWINDEX newRowCount, bool enforceFormatLimits = true);

	// Check if there is any note data on a given row.
	bool IsEmptyRow(ROWINDEX row) const;

	// Allocate new pattern memory and replace old pattern data.
	bool AllocatePattern(ROWINDEX rows);
	// Deallocate pattern data.
	void Deallocate();

	// Removes all modcommands from the pattern.
	void ClearCommands();

	// Returns associated soundfile.
	CSoundFile& GetSoundFile();
	const CSoundFile& GetSoundFile() const;

	bool SetData(ModCommand* p, const ROWINDEX rows) { m_ModCommands = p; m_Rows = rows; return false; }

	// Set pattern signature (rows per beat, rows per measure). Returns true on success.
	bool SetSignature(const ROWINDEX rowsPerBeat, const ROWINDEX rowsPerMeasure);
	void RemoveSignature() { m_RowsPerBeat = m_RowsPerMeasure = 0; }

	bool HasTempoSwing() const { return !m_tempoSwing.empty(); }
	const TempoSwing& GetTempoSwing() const { return m_tempoSwing; }
	void SetTempoSwing(const TempoSwing &swing) { m_tempoSwing = swing; m_tempoSwing.Normalize(); }
	void RemoveTempoSwing() { m_tempoSwing.clear(); }

	// Pattern name functions - bool functions return true on success.
	bool SetName(const std::string &newName);
	bool SetName(const char *newName, size_t maxChars);
	template<size_t bufferSize>
	bool SetName(const char (&buffer)[bufferSize])
	{
		return SetName(buffer, bufferSize);
	}

	template<size_t bufferSize>
	void GetName(char (&buffer)[bufferSize]) const
	{
		strncpy(buffer, m_PatternName.c_str(), bufferSize - 1);
		buffer[bufferSize - 1] = '\0';
	}
	std::string GetName() const { return m_PatternName; };

#ifdef MODPLUG_TRACKER
	// Double number of rows
	bool Expand();

	// Halve number of rows
	bool Shrink();
#endif // MODPLUG_TRACKER

	// Write some kind of effect data to the pattern
	bool WriteEffect(EffectWriter &settings);

	bool WriteITPdata(FILE* f) const;

	// Static allocation / deallocation helpers
	static ModCommand *AllocatePattern(ROWINDEX rows, CHANNELINDEX nchns);
	static void FreePattern(ModCommand *pat);

//END: INTERFACE METHODS

	typedef ModCommand* iterator;
	typedef const ModCommand *const_iterator;

	iterator Begin() { return m_ModCommands; }
	const_iterator Begin() const { return m_ModCommands; }

	iterator End() { return (m_ModCommands != nullptr) ? m_ModCommands + m_Rows * GetNumChannels() : nullptr; }
	const_iterator End() const { return (m_ModCommands != nullptr) ? m_ModCommands + m_Rows * GetNumChannels() : nullptr; }

	CPattern(CPatternContainer& patCont) : m_ModCommands(0), m_Rows(64), m_RowsPerBeat(0), m_RowsPerMeasure(0), m_rPatternContainer(patCont) {};

protected:
	ModCommand& GetModCommand(size_t i) { return m_ModCommands[i]; }
	//Returns modcommand from (floor[i/channelCount], i%channelCount) 

	ModCommand& GetModCommand(ROWINDEX r, CHANNELINDEX c) { return m_ModCommands[r * GetNumChannels() + c]; }
	const ModCommand& GetModCommand(ROWINDEX r, CHANNELINDEX c) const { return m_ModCommands[r * GetNumChannels() + c]; }


//BEGIN: DATA
protected:
	ModCommand* m_ModCommands;
	ROWINDEX m_Rows;
	ROWINDEX m_RowsPerBeat;		// patterns-specific time signature. if != 0, this is implicitely set.
	ROWINDEX m_RowsPerMeasure;	// ditto
	TempoSwing m_tempoSwing;
	std::string m_PatternName;
	CPatternContainer& m_rPatternContainer;
//END: DATA
};


const char FileIdPattern[] = "mptP";

void ReadModPattern(std::istream& iStrm, CPattern& patc, const size_t nSize = 0);
void WriteModPattern(std::ostream& oStrm, const CPattern& patc);


// Class for conveniently writing an effect to the pattern.

//================
class EffectWriter
//================
{
	friend class CPattern;
	
public:
	// Row advance mode
	enum RetryMode
	{
		rmIgnore,			// If effect can't be written, abort.
		rmTryNextRow,		// If effect can't be written, try next row.
		rmTryPreviousRow,	// If effect can't be written, try previous row.
	};

	// Constructors with effect commands
	EffectWriter(EffectCommands cmd, ModCommand::PARAM param) : m_command(static_cast<uint8>(cmd)), m_param(param), m_isVolEffect(false) { Init(); }
	EffectWriter(VolumeCommands cmd, ModCommand::VOL param) : m_command(static_cast<uint8>(cmd)), m_param(param), m_isVolEffect(true) { Init(); }

	// Additional constructors:
	// Set row in which writing should start
	EffectWriter &Row(ROWINDEX row) { m_row = row; return *this; }
	// Set channel to which writing should be restricted to
	EffectWriter &Channel(CHANNELINDEX chn) { m_channel = chn; return *this; }
	// Allow multiple effects of the same kind to be written in the same row.
	EffectWriter &AllowMultiple() { m_allowMultiple = true; return *this; }
	// Set retry mode.
	EffectWriter &Retry(RetryMode retryMode) { m_retryMode = retryMode; return *this; }

protected:
	uint8 m_command, m_param;
	
	ROWINDEX m_row;
	CHANNELINDEX m_channel;
	RetryMode m_retryMode;
	bool m_retry : 1;
	bool m_allowMultiple : 1;
	bool m_isVolEffect : 1;

	// Common data initialisation
	void Init()
	{
		m_row = 0;
		m_channel = CHANNELINDEX_INVALID;	// Any channel
		m_retryMode = rmIgnore;			// If effect couldn't be written, abort.
		m_retry = true;
		m_allowMultiple = false;		// Stop if same type of effect is encountered
	}
};


OPENMPT_NAMESPACE_END
