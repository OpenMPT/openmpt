/*
 * Dither.h
 * --------
 * Purpose: Dithering when converting to lower resolution sample formats.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once


struct DitherModPlugState
{
	uint32 rng_a;
	uint32 rng_b;
	DitherModPlugState()
	{
		rng_a = 0;
		rng_b = 0;
	}
};

struct DitherState
{
	DitherModPlugState modplug;
};

enum DitherMode
{
	DitherNone       = 0,
	DitherModPlug    = 1
};

class Dither
{
private:
	DitherState state;
	DitherMode mode;
public:
	Dither();
	void SetMode(DitherMode &mode);
	DitherMode GetMode() const;
	void Reset();
	void Process(int *mixbuffer, std::size_t count, std::size_t channels, int bits);
};
