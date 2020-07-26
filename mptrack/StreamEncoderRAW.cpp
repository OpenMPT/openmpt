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
	Encoder::Format formatInfo;

public:
	RawStreamWriter(const RAWEncoder &enc_, std::ostream &file, const Encoder::Settings &settings, const FileTags &tags)
		: enc(enc_)
		, f(file)
	{
		formatInfo = enc.GetTraits().formats[settings.Format];
		ASSERT(formatInfo.Sampleformat.IsValid());
		ASSERT(formatInfo.Samplerate > 0);
		ASSERT(formatInfo.Channels > 0);

		MPT_UNREFERENCED_PARAMETER(tags);
	}
	mpt::endian GetConvertedEndianness() const override
	{
		return mpt::get_endian();
	}
	void WriteInterleaved(size_t count, const float *interleaved) override
	{
		ASSERT(formatInfo.Sampleformat.IsFloat());
		WriteInterleavedConverted(count, reinterpret_cast<const std::byte*>(interleaved));
	}
	void WriteInterleavedConverted(size_t frameCount, const std::byte *data) override
	{
		mpt::IO::WriteRaw(f, data, frameCount * formatInfo.Channels * (formatInfo.Sampleformat.GetBitsPerSample() / 8));
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
	traits.modes = Encoder::ModeEnumerated;
	for(std::size_t i = 0; i < traits.samplerates.size(); ++i)
	{
		int samplerate = traits.samplerates[i];
		for(int channels = 1; channels <= traits.maxChannels; channels *= 2)
		{
			const std::array<SampleFormat, 7> sampleFormats = { SampleFormatFloat64, SampleFormatFloat32, SampleFormatInt32, SampleFormatInt24, SampleFormatInt16, SampleFormatInt8, SampleFormatUnsigned8 };
			for(const auto sampleFormat : sampleFormats)
			{
				Encoder::Format format;
				format.Samplerate = samplerate;
				format.Channels = channels;
				format.Sampleformat = sampleFormat;
				if(sampleFormat.IsFloat())
				{
					format.Description = MPT_UFORMAT("{} Bit Floating Point Little-Endian")(sampleFormat.GetBitsPerSample());
				} else if(sampleFormat.IsUnsigned())
				{
					format.Description = MPT_UFORMAT("{} Bit Little-Endian (unsigned)")(sampleFormat.GetBitsPerSample());
				} else
				{
					format.Description = MPT_UFORMAT("{} Bit Little-Endian")(sampleFormat.GetBitsPerSample());
				}
				format.Bitrate = 0;
				traits.formats.push_back(format);
			}
		}
	}
	traits.defaultSamplerate = 48000;
	traits.defaultChannels = 2;
	traits.defaultMode = Encoder::ModeEnumerated;
	traits.defaultFormat = 1;  // 32-bit float
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
