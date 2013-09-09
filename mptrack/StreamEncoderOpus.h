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


struct OpusDynBind;

class OggOpusEncoder : public EncoderFactoryBase
{

private:

	OpusDynBind *m_Opus;

public:

	IAudioStreamEncoder *ConstructStreamEncoder(std::ostream &file) const;
	bool IsAvailable() const;

public:

	OggOpusEncoder();
	virtual ~OggOpusEncoder();

};
