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


class FLACEncoder : public EncoderFactoryBase
{

public:

	IAudioStreamEncoder *ConstructStreamEncoder(std::ostream &file) const;
	bool IsAvailable() const;

public:

	FLACEncoder();
	virtual ~FLACEncoder();

};
