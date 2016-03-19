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


#define MPT_MP3ENCODER_LAME
#define MPT_MP3ENCODER_BLADE
#define MPT_MP3ENCODER_ACM


#ifdef MPT_MP3ENCODER_LAME
class ComponentLame;
#endif
#ifdef MPT_MP3ENCODER_BLADE
class ComponentBlade;
#endif
#ifdef MPT_MP3ENCODER_ACM
class ComponentAcmMP3;
#endif

enum MP3EncoderType
{
	MP3EncoderDefault,
	MP3EncoderLame,
	MP3EncoderLameCompatible,
	MP3EncoderBlade,
	MP3EncoderACM,
};

class MP3Encoder : public EncoderFactoryBase
{

private:

#ifdef MPT_MP3ENCODER_LAME
	ComponentHandle<ComponentLame> m_Lame;
#endif
#ifdef MPT_MP3ENCODER_BLADE
	ComponentHandle<ComponentBlade> m_Blade;
#endif
#ifdef MPT_MP3ENCODER_ACM
	ComponentHandle<ComponentAcmMP3> m_Acm;
#endif

	MP3EncoderType m_Type;

public:

	IAudioStreamEncoder *ConstructStreamEncoder(std::ostream &file) const;
	mpt::ustring DescribeQuality(float quality) const;
	mpt::ustring DescribeBitrateABR(int bitrate) const;
	bool IsAvailable() const;

public:

	MP3Encoder(MP3EncoderType type=MP3EncoderDefault);

	virtual ~MP3Encoder();

};


OPENMPT_NAMESPACE_END
