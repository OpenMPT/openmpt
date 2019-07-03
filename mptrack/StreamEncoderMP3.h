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

#include "BuildSettings.h"

#include "StreamEncoder.h"

OPENMPT_NAMESPACE_BEGIN


#ifdef MPT_WITH_LAME
class ComponentLame;
#endif

enum MP3EncoderType
{
	MP3EncoderLame,
	MP3EncoderLameCompatible,
};

class MP3Encoder : public EncoderFactoryBase
{

private:

	MP3EncoderType m_Type;

public:

	std::unique_ptr<IAudioStreamEncoder> ConstructStreamEncoder(std::ostream &file, const Encoder::Settings &settings, const FileTags &tags) const override;
	mpt::ustring DescribeQuality(float quality) const override;
	mpt::ustring DescribeBitrateABR(int bitrate) const override;
	bool IsAvailable() const override;

public:

	MP3Encoder(MP3EncoderType type);

};


OPENMPT_NAMESPACE_END
