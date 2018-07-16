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

	
class WavStreamWriter;


class WAVEncoder : public EncoderFactoryBase
{

	friend class WavStreamWriter;

public:

	std::unique_ptr<IAudioStreamEncoder> ConstructStreamEncoder(std::ostream &file, const Encoder::Settings &settings, const FileTags &tags) const override;
	bool IsAvailable() const override;

public:

	WAVEncoder();

};


OPENMPT_NAMESPACE_END
