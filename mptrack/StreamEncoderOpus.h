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

	std::unique_ptr<IAudioStreamEncoder> ConstructStreamEncoder(std::ostream &file, const Encoder::Settings &settings, const FileTags &tags) const;
	bool IsAvailable() const;

public:

	OggOpusEncoder();
	virtual ~OggOpusEncoder();

};


OPENMPT_NAMESPACE_END
