/*
 * RowVisitor.cpp
 * --------------
 * Purpose: Class for managing which rows of a song has already been visited. Useful for detecting backwards jumps, loops, etc.
 * Notes  : The class keeps track of rows that have been visited by the player before.
 *          This way, we can tell when the module starts to loop, i.e. we can determine the song length,
 *          or find out that a given point of the module can never be reached.
 *
 *          Specific implementations:
 *
 *          Length detection code:
 *          As the ModPlug engine already deals with pattern loops sufficiently (though not always correctly),
 *          there's no problem with (infinite) pattern loops in this code.
 *
 *          Normal player code:
 *          Bear in mind that rows inside pattern loops should only be evaluated once, or else the algorithm will cancel too early!
 *          So in that case, the pattern loop rows have to be reset when looping back.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Sndfile.h"
#include "RowVisitor.h"

OPENMPT_NAMESPACE_BEGIN

// Resize / Clear the row vector.
// If reset is true, the vector is not only resized to the required dimensions, but also completely cleared (i.e. all visited rows are unset).
void RowVisitor::Initialize(bool reset, SEQUENCEINDEX sequence)
//-------------------------------------------------------------
{
	if(sequence < sndFile.Order.GetNumSequences())
		Order = &sndFile.Order.GetSequence(sequence);
	else
		Order = &sndFile.Order;

	const ORDERINDEX endOrder = Order->GetLengthTailTrimmed();
	visitedRows.resize(endOrder);
	if(reset)
	{
		visitOrder.clear();
	}

	for(ORDERINDEX order = 0; order < endOrder; order++)
	{
		VisitedRowsBaseType &row = visitedRows[order];
		const size_t size = GetVisitedRowsVectorSize(Order->At(order));
		if(reset)
		{
			// If we want to reset the vectors completely, we overwrite existing items with false.
			row.assign(size, false);
		} else
		{
			row.resize(size, false);
		}
	}
}


// (Un)sets a given row as visited.
// order, row - which row should be (un)set
// If visited is true, the row will be set as visited.
void RowVisitor::SetVisited(ORDERINDEX order, ROWINDEX row, bool visited)
//-----------------------------------------------------------------------
{
	if(order >= Order->GetLength() || row >= GetVisitedRowsVectorSize(Order->At(order)))
	{
		return;
	}

	// The module might have been edited in the meantime - so we have to extend this a bit.
	if(order >= visitedRows.size() || row >= visitedRows[order].size())
	{
		Initialize(false);
		// If it's still past the end of the vector, this means that order >= Order->GetLengthTailTrimmed(), i.e. we are trying to play an empty order.
		if(order >= visitedRows.size())
		{
			return;
		}
	}

	visitedRows[order][row] = visited;
	if(visited)
	{
		AddVisitedRow(order, row);
	}
}


// Returns whether a given row has been visited yet.
// If autoSet is true, the queried row will automatically be marked as visited.
// Use this parameter instead of consecutive IsRowVisited / SetRowVisited calls.
bool RowVisitor::IsVisited(ORDERINDEX order, ROWINDEX row, bool autoSet)
//----------------------------------------------------------------------
{
	if(order >= Order->GetLength())
	{
		return false;
	}

	// The row slot for this row has not been assigned yet - Just return false, as this means that the program has not played the row yet.
	if(order >= visitedRows.size() || row >= visitedRows[order].size())
	{
		if(autoSet)
		{
			SetVisited(order, row, true);
		}
		return false;
	}

	if(visitedRows[order][row])
	{
		// We visited this row already - this module must be looping.
		return true;
	} else if(autoSet)
	{
		visitedRows[order][row] = true;
		AddVisitedRow(order, row);
	}

	return false;
}


// Get the needed vector size for pattern nPat.
size_t RowVisitor::GetVisitedRowsVectorSize(PATTERNINDEX pattern) const
//---------------------------------------------------------------------
{
	if(sndFile.Patterns.IsValidPat(pattern))
	{
		return static_cast<size_t>(sndFile.Patterns[pattern].GetNumRows());
	} else
	{
		// Invalid patterns consist of a "fake" row.
		return 1;
	}
}


// Find the first row that has not been played yet.
// The order and row is stored in the order and row variables on success, on failure they contain invalid values.
// If fastSearch is true (default), only the first row of each pattern is looked at, otherwise every row is examined.
// Function returns true on success.
bool RowVisitor::GetFirstUnvisitedRow(ORDERINDEX &order, ROWINDEX &row, bool fastSearch) const
//--------------------------------------------------------------------------------------------
{
	const ORDERINDEX endOrder = Order->GetLengthTailTrimmed();
	for(order = 0; order < endOrder; order++)
	{
		const PATTERNINDEX pattern = Order->At(order);
		if(!sndFile.Patterns.IsValidPat(pattern))
		{
			continue;
		}

		if(order >= visitedRows.size())
		{
			// Not yet initialized => unvisited
			return true;
		}

		const ROWINDEX endRow = (fastSearch ? 1 : sndFile.Patterns[pattern].GetNumRows());
		for(row = 0; row < endRow; row++)
		{
			if(row >= visitedRows[order].size() || visitedRows[order][row] == false)
			{
				// Not yet initialized, or unvisited
				return true;
			}
		}
	}

	// Didn't find anything :(
	order = ORDERINDEX_INVALID;
	row = ROWINDEX_INVALID;
	return false;
}


// Set all rows of a previous pattern loop as unvisited.
void RowVisitor::ResetPatternLoop(ORDERINDEX order, ROWINDEX startRow)
//--------------------------------------------------------------------
{
	MPT_ASSERT(order == currentOrder);	// Shouldn't trigger, unless we're jumping around in the GUI during a pattern loop.
	
	// Unvisit all rows that are in the visited row buffer, until we hit the start row for this pattern loop.
	ROWINDEX row = ROWINDEX_INVALID;
	std::vector<ROWINDEX>::reverse_iterator iter = visitOrder.rbegin();
	while(iter != visitOrder.rend() && row != startRow)
	{
		row = *(iter++);
		Unvisit(order, row);
	}
}


// Add a row to the visited row memory for this pattern.
void RowVisitor::AddVisitedRow(ORDERINDEX order, ROWINDEX row)
//------------------------------------------------------------
{
	if(order != currentOrder)
	{
		// We're in a new pattern! Forget about which rows we previously visited...
		visitOrder.clear();
		currentOrder = order;
	}
	if(visitOrder.empty())
	{
		visitOrder.reserve(GetVisitedRowsVectorSize(Order->At(order)));
	}
	// And now add the played row to our memory.
	visitOrder.push_back(row);
}


OPENMPT_NAMESPACE_END
