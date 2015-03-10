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


class ComponentVorbis;

class VorbisEncoder : public EncoderFactoryBase
{

private:

	ComponentHandle<ComponentVorbis> m_Vorbis;

public:

	IAudioStreamEncoder *ConstructStreamEncoder(std::ostream &file) const;
	mpt::ustring DescribeQuality(float quality) const;
	bool IsAvailable() const;

public:

	VorbisEncoder();
	virtual ~VorbisEncoder();

};


OPENMPT_NAMESPACE_END
