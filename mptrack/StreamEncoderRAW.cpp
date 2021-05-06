/*
 * StreamEncoderRAW.cpp
 * --------------------
 * Purpose: Exporting streamed music files.
 * Notes  : none
 * Authors: Joern Heusipp
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"

#include "StreamEncoder.h"
#include "StreamEncoderRAW.h"

#include "Mptrack.h"
#include "TrackerSettings.h"

#include "../common/mptFileIO.h"
#include "../soundlib/Sndfile.h"


OPENMPT_NAMESPACE_BEGIN


class RawStreamWriter : public IAudioStreamEncoder
{
private:
	const RAWEncoder &enc;
	std::ostream &f;
	Encoder::Settings settings;

public:
	RawStreamWriter(const RAWEncoder &enc_, std::ostream &file, const Encoder::Settings &settings_, const FileTags &tags)
		: enc(enc_)
		, f(file)
		, settings(settings_)
	{
		MPT_ASSERT(settings.Format.GetSampleFormat().IsValid());
		MPT_ASSERT(settings.Samplerate > 0);
		MPT_ASSERT(settings.Channels > 0);
		MPT_UNREFERENCED_PARAMETER(tags);
	}
	mpt::endian GetConvertedEndianness() const override
	{
		if(settings.Format.endian == mpt::endian::big)
		{
			return mpt::endian::big;
		}
		if(settings.Format.endian == mpt::endian::little)
		{
			return mpt::endian::little;
		}
		return mpt::get_endian();
	}
	void WriteInterleaved(size_t count, const float *interleaved) override
	{
		MPT_ASSERT(settings.Format.GetSampleFormat().IsFloat());
		WriteInterleavedConverted(count, reinterpret_cast<const std::byte*>(interleaved));
	}
	void WriteInterleavedConverted(size_t frameCount, const std::byte *data) override
	{
		mpt::IO::WriteRaw(f, data, frameCount * settings.Channels * settings.Format.GetSampleFormat().GetSampleSize());
	}
	void WriteCues(const std::vector<uint64> &cues) override
	{
		MPT_UNREFERENCED_PARAMETER(cues);
	}
	void WriteFinalize() override
	{
		// nothing
	}
	virtual ~RawStreamWriter()
	{
		// nothing
	}
};



RAWEncoder::RAWEncoder()
{
	Encoder::Traits traits;
	traits.fileExtension = P_("raw");
	traits.fileShortDescription = U_("Raw PCM");
	traits.fileDescription = U_("Headerless raw little-endian PCM");
	traits.encoderSettingsName = U_("RAW");
	traits.canTags = false;
	traits.canCues = false;
	traits.maxChannels = 4;
	traits.samplerates = TrackerSettings::Instance().GetSampleRates();
	traits.modes = Encoder::ModeLossless;
	traits.formats.push_back({ Encoder::Format::Encoding::Float, 64, mpt::endian::little });
	traits.formats.push_back({ Encoder::Format::Encoding::Float, 64, mpt::endian::big });
	traits.formats.push_back({ Encoder::Format::Encoding::Float, 32, mpt::endian::little });
	traits.formats.push_back({ Encoder::Format::Encoding::Float, 32, mpt::endian::big });
	traits.formats.push_back({ Encoder::Format::Encoding::Integer, 32, mpt::endian::little });
	traits.formats.push_back({ Encoder::Format::Encoding::Integer, 32, mpt::endian::big });
	traits.formats.push_back({ Encoder::Format::Encoding::Integer, 24, mpt::endian::little });
	traits.formats.push_back({ Encoder::Format::Encoding::Integer, 24, mpt::endian::big });
	traits.formats.push_back({ Encoder::Format::Encoding::Integer, 16, mpt::endian::little });
	traits.formats.push_back({ Encoder::Format::Encoding::Integer, 16, mpt::endian::big });
	traits.formats.push_back({ Encoder::Format::Encoding::Integer, 8, mpt::endian::little });
	traits.formats.push_back({ Encoder::Format::Encoding::Unsigned, 8, mpt::endian::little });
	traits.defaultSamplerate = 48000;
	traits.defaultChannels = 2;
	traits.defaultMode = Encoder::ModeLossless;
	traits.defaultFormat = { Encoder::Format::Encoding::Float, 32, mpt::endian::little };
	SetTraits(traits);
}


bool RAWEncoder::IsAvailable() const
{
	return true;
}


std::unique_ptr<IAudioStreamEncoder> RAWEncoder::ConstructStreamEncoder(std::ostream &file, const Encoder::Settings &settings, const FileTags &tags) const
{
	if(!IsAvailable())
	{
		return nullptr;
	}
	return std::make_unique<RawStreamWriter>(*this, file, settings, tags);
}


OPENMPT_NAMESPACE_END
