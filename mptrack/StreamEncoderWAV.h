/*
 * StreamEncoder.h
 * ---------------
 * Purpose: Exporting streamed music files.
 * Notes  : none
 * Authors: Joern Heusipp
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "StreamEncoder.h"


class WavStreamWriter;


class WAVEncoder : public EncoderFactoryBase
{

	friend class WavStreamWriter;

public:

	IAudioStreamEncoder *ConstructStreamEncoder(std::ostream &file) const;
	bool IsAvailable() const;

public:

	WAVEncoder();
	virtual ~WAVEncoder();

};
