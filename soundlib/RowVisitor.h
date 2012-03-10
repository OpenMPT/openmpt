/*
 * RowVisitor.h
 * ------------
 * Purpose: Class for managing which rows of a song has already been visited. Useful for detecting backwards jumps, loops, etc.
 * Notes  : See implementation file.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include <vector>
#include "Snd_defs.h"
class CSoundFile;

//==============
class RowVisitor
//==============
{
protected:

	// Data type for the visited rows.
	typedef std::vector<bool> VisitedRowsBaseType;
	typedef std::vector<VisitedRowsBaseType> VisitedRowsType;

	VisitedRowsType visitedRows;
	const CSoundFile &sndFile;

public:

	RowVisitor(const CSoundFile &sf) : sndFile(sf)
	{
		Initialize(true);
	};

	RowVisitor(const RowVisitor &other) : sndFile(other.sndFile), visitedRows(other.visitedRows) { };

	// Resize / Clear the row vector.
	// If reset is true, the vector is not only resized to the required dimensions, but also completely cleared (i.e. all visited rows are unset).
	void Initialize(bool reset);

	// Mark a row as visited.
	void Visit(ORDERINDEX order, ROWINDEX row)
	{
		SetVisited(order, row, true);
	};

	// Mark a row as not visited.
	void Unvisit(ORDERINDEX order, ROWINDEX row)
	{
		SetVisited(order, row, false);
	};

	// Returns whether a given row has been visited yet.
	// If autoSet is true, the queried row will automatically be marked as visited.
	// Use this parameter instead of consecutive IsRowVisited / SetRowVisited calls.
	bool IsVisited(ORDERINDEX nOrd, ROWINDEX nRow, bool autoSet);

	// Get the needed vector size for a given pattern.
	size_t GetVisitedRowsVectorSize(PATTERNINDEX pattern) const;

	// Find the first row that has not been played yet.
	// The order and row is stored in the order and row variables on success, on failure they contain invalid values.
	// If fastSearch is true (default), only the first row of each pattern is looked at, otherwise every row is examined.
	// Function returns true on success.
	bool GetFirstUnvisitedRow(ORDERINDEX &order, ROWINDEX &row, bool fastSearch) const;

	// Retrieve visited rows vector from another RowVisitor object.
	void Set(const RowVisitor &other)
	{
		visitedRows = other.visitedRows;
	}

protected:

	// (Un)sets a given row as visited.
	// order, row - which row should be (un)set
	// If visited is true, the row will be set as visited.
	void SetVisited(ORDERINDEX nOrd, ROWINDEX nRow, bool visited);

};
