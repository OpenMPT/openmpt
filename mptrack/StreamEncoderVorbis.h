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


OPENMPT_NAMESPACE_BEGIN


class VorbisEncoder : public EncoderFactoryBase
{

public:

	std::unique_ptr<IAudioStreamEncoder> ConstructStreamEncoder(std::ostream &file, const Encoder::Settings &settings, const FileTags &tags) const;
	mpt::ustring DescribeQuality(float quality) const;
	bool IsAvailable() const;

public:

	VorbisEncoder();
	virtual ~VorbisEncoder();

};


OPENMPT_NAMESPACE_END
