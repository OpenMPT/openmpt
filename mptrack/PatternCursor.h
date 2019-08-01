/*
 * PatternCursor.h
 * ---------------
 * Purpose: Class for storing pattern cursor information.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

#include "../soundlib/Snd_defs.h"

OPENMPT_NAMESPACE_BEGIN

class PatternCursor
{
	friend class PatternRect;

public:

	enum Columns
	{
		firstColumn = 0,
		noteColumn = firstColumn,
		instrColumn,
		volumeColumn,
		effectColumn,
		paramColumn,
		lastColumn = paramColumn,
	};

protected:
	// Pattern cursor structure (MSB to LSB):
	// | 16 bits - row | 13 bits - channel | 3 bits - channel component |
	// As you can see, the highest 16 bits contain a row index.
	// It is followed by a channel index, which is 13 bits wide.
	// The lowest 3 bits are used for addressing the components of a channel.
	// They are *not* used as a bit set, but treated as one of the Columns enum entries that can be found above.
	uint32 cursor;

	static_assert(MAX_BASECHANNELS <= 0x1FFF, "Check: Channel index in class PatternCursor is only 13 bits wide!");
	static_assert(MAX_PATTERN_ROWS <= 0xFFFF, "Check: Row index in class PatternCursor is only 16 bits wide!");

public:

	// Construct cursor from given coordinates.
	PatternCursor(ROWINDEX row = 0, CHANNELINDEX channel = 0, Columns column = firstColumn)
	{
		Set(row, channel, column);
	};

	// Construct cursor from a given row and another cursor's horizontal position (channel + column).
	PatternCursor(ROWINDEX row, const PatternCursor &other)
	{
		Set(row, other);
	};

	// Get the pattern row the cursor is located in.
	ROWINDEX GetRow() const
	{
		return static_cast<ROWINDEX>(cursor >> 16);
	};

	// Get the pattern channel the cursor is located in.
	CHANNELINDEX GetChannel() const
	{
		return static_cast<CHANNELINDEX>((cursor & 0xFFFF) >> 3);
	};

	// Get the pattern channel column the cursor is located in.
	Columns GetColumnType() const
	{
		return static_cast<Columns>((cursor & 0x07));
	};

	// Set the cursor to given coordinates.
	void Set(ROWINDEX row, CHANNELINDEX channel, Columns column = firstColumn)
	{
		cursor = (row << 16) | ((channel << 3) & 0x1FFF) | (column & 0x07);
	};

	// Set the cursor to the same position as another curosr.
	void Set(const PatternCursor &other)
	{
		*this = other;
	};

	// Set the cursor to a given row and another cursor's horizontal position (channel + column).
	void Set(ROWINDEX row, const PatternCursor &other)
	{
		cursor = (row << 16) | (other.cursor & 0xFFFF);
	};

	// Only update the row of a cursor.
	void SetRow(ROWINDEX row)
	{
		Set(row, *this);
	};

	// Only update the horizontal position of a cursor.
	void SetColumn(CHANNELINDEX chn, Columns col)
	{
		Set(GetRow(), chn, col);
	};

	// Move the cursor relatively.
	void Move(int rows, int channels, int columns)
	{
		int row = std::max(int(0), static_cast<int>(GetRow()) + rows);
		int chn = static_cast<int>(GetChannel()) + channels;
		int col = static_cast<int>(GetColumnType() + columns);

		// Boundary checking
		if(chn < 0)
		{
			// Moving too far left
			chn = 0;
			col = firstColumn;
		}

		if(col < firstColumn)
		{
			// Extending beyond first column
			col = lastColumn;
			chn--;
		} else if(col > lastColumn)
		{
			// Extending beyond last column
			col = firstColumn;
			chn++;
		}

		Set(static_cast<ROWINDEX>(row), static_cast<CHANNELINDEX>(chn), static_cast<Columns>(col));
	}


	// Remove the channel column type from the cursor position.
	void RemoveColType()
	{
		cursor &= ~0x07;
	}

	// Compare the row of two cursors.
	// Returns -1 if this cursor is above of the other cursor,
	// 1 if it is below of the other cursor and 0 if they are at the same vertical position.
	int CompareRow(const PatternCursor &other) const
	{
		if(GetRow() < other.GetRow()) return -1;
		if(GetRow() > other.GetRow()) return 1;
		return 0;
	}

	// Compare the column (channel + sub column) of two cursors.
	// Returns -1 if this cursor is left of the other cursor,
	// 1 if it is right of the other cursor and 0 if they are at the same horizontal position.
	int CompareColumn(const PatternCursor &other) const
	{
		if((cursor & 0xFFFF) < (other.cursor & 0xFFFF)) return -1;
		if((cursor & 0xFFFF) > (other.cursor & 0xFFFF)) return 1;
		return 0;
	}


	// Check whether the cursor is placed on the first channel and first column.
	bool IsInFirstColumn() const
	{
		return (cursor & 0xFFFF) == 0;
	}


	// Ensure that the point lies within a given pattern size.
	void Sanitize(ROWINDEX maxRows, CHANNELINDEX maxChans)
	{
		ROWINDEX row = std::min(GetRow(), static_cast<ROWINDEX>(maxRows - 1));
		CHANNELINDEX chn = GetChannel();
		Columns col = GetColumnType();

		if(chn >= maxChans)
		{
			chn = maxChans - 1;
			col = lastColumn;
		} else if(col > lastColumn)
		{
			col = lastColumn;
		}
		Set(row, chn, col);
	};

	bool operator == (const PatternCursor &other) const
	{
		return cursor == other.cursor;
	}

	bool operator != (const PatternCursor &other) const
	{
		return !(*this == other);
	}

};


class PatternRect
{
protected:
	PatternCursor upperLeft, lowerRight;

public:

	// Construct selection from two pattern cursors.
	PatternRect(const PatternCursor &p1, const PatternCursor &p2)
	{
		if(p1.CompareColumn(p2) <= 0)
		{
			upperLeft.cursor = (p1.cursor & 0xFFFF);
			lowerRight.cursor = (p2.cursor & 0xFFFF);
		} else
		{
			upperLeft.cursor = (p2.cursor & 0xFFFF);
			lowerRight.cursor = (p1.cursor & 0xFFFF);
		}

		if(p1.CompareRow(p2) <= 0)
		{
			upperLeft.cursor |= (p1.GetRow() << 16);
			lowerRight.cursor |= (p2.GetRow() << 16);
		} else
		{
			upperLeft.cursor |= (p2.GetRow() << 16);
			lowerRight.cursor |= (p1.GetRow() << 16);
		}
	};

	// Construct empty rect.
	PatternRect()
	{
		upperLeft.Set(0);
		lowerRight.Set(0);
	}

	// Return upper-left corner of selection.
	PatternCursor GetUpperLeft() const
	{
		return upperLeft;
	};

	// Return lower-right corner of selection.
	PatternCursor GetLowerRight() const
	{
		return lowerRight;
	};

	// Check if a given point is within the horizontal boundaries of the rect
	bool ContainsHorizontal(const PatternCursor &point) const
	{
		return point.CompareColumn(upperLeft) >= 0 && point.CompareColumn(lowerRight) <= 0;
	}

	// Check if a given point is within the vertical boundaries of the rect
	bool ContainsVertical(const PatternCursor &point) const
	{
		return point.CompareRow(upperLeft) >= 0 && point.CompareRow(lowerRight) <= 0;
	}

	// Check if a given point is within the rect
	bool Contains(const PatternCursor &point) const
	{
		return ContainsHorizontal(point) && ContainsVertical(point);
	}

	// Ensure that the selection doesn't exceed a given pattern size.
	void Sanitize(ROWINDEX maxRows, CHANNELINDEX maxChans)
	{
		upperLeft.Sanitize(maxRows, maxChans);
		lowerRight.Sanitize(maxRows, maxChans);
	};


	// Get first row of selection
	ROWINDEX GetStartRow() const
	{
		ASSERT(upperLeft.GetRow() <= lowerRight.GetRow());
		return upperLeft.GetRow();
	}

	// Get last row of selection
	ROWINDEX GetEndRow() const
	{
		ASSERT(upperLeft.GetRow() <= lowerRight.GetRow());
		return lowerRight.GetRow();
	}

	// Get first channel of selection
	CHANNELINDEX GetStartChannel() const
	{
		ASSERT(upperLeft.GetChannel() <= lowerRight.GetChannel());
		return upperLeft.GetChannel();
	}

	// Get last channel of selection
	CHANNELINDEX GetEndChannel() const
	{
		ASSERT(upperLeft.GetChannel() <= lowerRight.GetChannel());
		return lowerRight.GetChannel();
	}

	PatternCursor::Columns GetStartColumn() const
	{
		ASSERT((upperLeft.cursor & 0xFFFF) <= (lowerRight.cursor & 0xFFFF));
		return upperLeft.GetColumnType();
	}

	PatternCursor::Columns GetEndColumn() const
	{
		ASSERT((upperLeft.cursor & 0xFFFF) <= (lowerRight.cursor & 0xFFFF));
		return lowerRight.GetColumnType();
	}

	// Create a bitset of the selected columns of a channel. If a column is selected, the corresponding bit is set.
	// Example: If the first and second column of the channel are selected, the bits 00000011 would be returned.
	uint8 GetSelectionBits(CHANNELINDEX chn) const
	{
		const CHANNELINDEX startChn = GetStartChannel(), endChn = GetEndChannel();
		uint8 bits = 0;

		if(chn >= startChn && chn <= endChn)
		{
			// All columns could be selected (unless this is the first or last channel).
			bits = uint8_max;

			if(chn == startChn)
			{
				// First channel: Remove columns left of the start column type.
				bits <<= GetUpperLeft().GetColumnType();
			}
			if(chn == endChn)
			{
				// Last channel: Remove columns right of the end column type.
				bits &= (2 << GetLowerRight().GetColumnType()) - 1;
			}
		}
		return (bits & 0x1F);
	}

	// Get number of rows in selection
	ROWINDEX GetNumRows() const
	{
		ASSERT(upperLeft.GetRow() <= lowerRight.GetRow());
		return 1 + GetEndRow() - GetStartRow();
	}

	// Get number of channels in selection
	CHANNELINDEX GetNumChannels() const
	{
		ASSERT(upperLeft.GetChannel() <= lowerRight.GetChannel());
		return 1 + GetEndChannel() - GetStartChannel();
	}

	bool operator == (const PatternRect &other) const
	{
		return upperLeft == other.upperLeft && lowerRight == other.lowerRight;
	}

};

OPENMPT_NAMESPACE_END
