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

	
class FLACEncoder : public EncoderFactoryBase
{

public:

	std::unique_ptr<IAudioStreamEncoder> ConstructStreamEncoder(std::ostream &file, const Encoder::Settings &settings, const FileTags &tags) const override;
	bool IsAvailable() const override;

public:

	FLACEncoder();

};


OPENMPT_NAMESPACE_END
