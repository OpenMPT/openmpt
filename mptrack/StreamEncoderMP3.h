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


#define MPT_MP3ENCODER_LAME
#define MPT_MP3ENCODER_BLADE
#define MPT_MP3ENCODER_ACM


#ifdef MPT_MP3ENCODER_LAME
struct LameDynBind;
#endif
#ifdef MPT_MP3ENCODER_BLADE
struct BladeDynBind;
#endif
#ifdef MPT_MP3ENCODER_ACM
struct AcmDynBind;
#endif

enum MP3EncoderType
{
	MP3EncoderDefault,
	MP3EncoderLame,
	MP3EncoderBlade,
	MP3EncoderACM,
};

class MP3Encoder : public EncoderFactoryBase
{

private:

#ifdef MPT_MP3ENCODER_LAME
	LameDynBind *m_Lame;
#endif
#ifdef MPT_MP3ENCODER_BLADE
	BladeDynBind *m_Blade;
#endif
#ifdef MPT_MP3ENCODER_ACM
	AcmDynBind *m_Acm;
#endif

	MP3EncoderType m_Type;

public:

	IAudioStreamEncoder *ConstructStreamEncoder(std::ostream &file) const;
	std::string DescribeQuality(float quality) const;
	std::string DescribeBitrateABR(int bitrate) const;
	bool IsAvailable() const;

public:

	MP3Encoder(MP3EncoderType type=MP3EncoderDefault);

	virtual ~MP3Encoder();

};


OPENMPT_NAMESPACE_END
