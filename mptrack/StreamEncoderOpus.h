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

#include "../common/ComponentManager.h"


OPENMPT_NAMESPACE_BEGIN


class OggOpusEncoder : public EncoderFactoryBase
{

public:

	IAudioStreamEncoder *ConstructStreamEncoder(std::ostream &file) const;
	bool IsAvailable() const;

public:

	OggOpusEncoder();
	virtual ~OggOpusEncoder();

};


OPENMPT_NAMESPACE_END
