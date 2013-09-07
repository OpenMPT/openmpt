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


struct VorbisDynBind;

class VorbisEncoder : public EncoderFactoryBase
{

private:

	VorbisDynBind *m_Vorbis;

public:

	IAudioStreamEncoder *ConstructStreamEncoder(std::ostream &file) const;
	std::string DescribeQuality(float quality) const;
	bool IsAvailable() const;

public:

	VorbisEncoder();
	virtual ~VorbisEncoder();

};
