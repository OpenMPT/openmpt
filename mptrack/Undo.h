/*
 * Undo.h
 * ------
 * Purpose: Header file for undo functionality
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 */

#pragma once

#define MAX_UNDO_LEVEL 1000	// 1000 undo steps for each undo type!

/////////////////////////////////////////////////////////////////////////////////////////
// Pattern Undo

struct PATTERNUNDOBUFFER
{
	PATTERNINDEX pattern;
	ROWINDEX patternsize;
	CHANNELINDEX firstChannel, numChannels;
	ROWINDEX firstRow, numRows;
	MODCOMMAND *pbuffer;
	bool linkToPrevious;
};

//================
class CPatternUndo
//================
{

private:

	std::vector<PATTERNUNDOBUFFER> UndoBuffer;
	CModDoc *m_pModDoc;

	// Pattern undo helper functions
	void DeleteUndoStep(const UINT nStep);
	PATTERNINDEX Undo(bool linkedFromPrevious);

public:

	// Pattern undo functions
	void ClearUndo();
	bool PrepareUndo(PATTERNINDEX pattern, CHANNELINDEX firstChn, ROWINDEX firstRow, CHANNELINDEX numChns, ROWINDEX numRows, bool linkToPrevious = false);
	PATTERNINDEX Undo();
	bool CanUndo();
	void RemoveLastUndoStep();

	void SetParent(CModDoc *pModDoc) {m_pModDoc = pModDoc;}

	CPatternUndo()
	{
		UndoBuffer.clear();
		m_pModDoc = nullptr;
	};
	~CPatternUndo()
	{
		ClearUndo();
	};

};



/////////////////////////////////////////////////////////////////////////////////////////
// Sample Undo

// We will differentiate between different types of undo actions so that we don't have to copy the whole sample everytime.
enum sampleUndoTypes
{
	sundo_none,		// no changes to sample itself, e.g. loop point update
	sundo_update,	// silence, amplify, normalize, dc offset - update complete sample section
	sundo_delete,	// delete part of the sample
	sundo_invert,	// invert sample phase, apply again to undo
	sundo_reverse,	// reverse sample, dito
	sundo_unsign,	// unsign sample, dito
	sundo_insert,	// insert data, delete inserted data to undo
	sundo_replace,	// replace complete sample (16->8Bit, up/downsample, downmix to mono, pitch shifting / time stretching, trimming, pasting)
};

struct SAMPLEUNDOBUFFER
{
	MODSAMPLE OldSample;
	CHAR szOldName[MAX_SAMPLENAME];
	LPSTR SamplePtr;
	UINT nChangeStart, nChangeEnd;
	sampleUndoTypes nChangeType;
};

//===============
class CSampleUndo
//===============
{

private:

	// Undo buffer
	std::vector<std::vector<SAMPLEUNDOBUFFER> > UndoBuffer;
	CModDoc *m_pModDoc;

	// Sample undo helper functions
	void DeleteUndoStep(const SAMPLEINDEX nSmp, const UINT nStep);
	bool SampleBufferExists(const SAMPLEINDEX nSmp, bool bForceCreation = true);
	void RestrictBufferSize();
	UINT GetUndoBufferCapacity();

public:

	// Sample undo functions
	void ClearUndo();
	void ClearUndo(const SAMPLEINDEX nSmp);
	bool PrepareUndo(const SAMPLEINDEX nSmp, sampleUndoTypes nChangeType, UINT nChangeStart = 0, UINT nChangeEnd = 0);
	bool Undo(const SAMPLEINDEX nSmp);
	bool CanUndo(const SAMPLEINDEX nSmp);
	void RemoveLastUndoStep(const SAMPLEINDEX nSmp);

	void SetParent(CModDoc *pModDoc) {m_pModDoc = pModDoc;}

	CSampleUndo()
	{
		UndoBuffer.clear();
		m_pModDoc = nullptr;
	};
	~CSampleUndo()
	{
		ClearUndo();
	};

};