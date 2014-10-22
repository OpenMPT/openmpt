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


class ComponentOpus;

class OggOpusEncoder : public EncoderFactoryBase
{

private:

	MPT_SHARED_PTR<ComponentOpus> m_Opus;

public:

	IAudioStreamEncoder *ConstructStreamEncoder(std::ostream &file) const;
	bool IsAvailable() const;

public:

	OggOpusEncoder();
	virtual ~OggOpusEncoder();

};


OPENMPT_NAMESPACE_END
