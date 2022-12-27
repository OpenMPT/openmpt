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
		MPT_ASSERT(settings.Samplerate > 0);
		MPT_ASSERT(settings.Channels > 0);
		MPT_UNREFERENCED_PARAMETER(tags);
	}
	SampleFormat GetSampleFormat() const override
	{
		return settings.Format.GetSampleFormat();
	}
	void WriteInterleaved(std::size_t frameCount, const double *interleaved) override
	{
		if(settings.Format.endian == mpt::endian::little)
		{
			WriteInterleavedLE(f, settings.Channels, settings.Format, frameCount, interleaved);
		} else
		{
			WriteInterleavedBE(f, settings.Channels, settings.Format, frameCount, interleaved);
		}
	}
	void WriteInterleaved(std::size_t frameCount, const float *interleaved) override
	{
		if(settings.Format.endian == mpt::endian::little)
		{
			WriteInterleavedLE(f, settings.Channels, settings.Format, frameCount, interleaved);
		} else
		{
			WriteInterleavedBE(f, settings.Channels, settings.Format, frameCount, interleaved);
		}
	}
	void WriteInterleaved(std::size_t frameCount, const int32 *interleaved) override
	{
		if(settings.Format.endian == mpt::endian::little)
		{
			WriteInterleavedLE(f, settings.Channels, settings.Format, frameCount, interleaved);
		} else
		{
			WriteInterleavedBE(f, settings.Channels, settings.Format, frameCount, interleaved);
		}
	}
	void WriteInterleaved(std::size_t frameCount, const int24 *interleaved) override
	{
		if(settings.Format.endian == mpt::endian::little)
		{
			WriteInterleavedLE(f, settings.Channels, settings.Format, frameCount, interleaved);
		} else
		{
			WriteInterleavedBE(f, settings.Channels, settings.Format, frameCount, interleaved);
		}
	}
	void WriteInterleaved(std::size_t frameCount, const int16 *interleaved) override
	{
		if(settings.Format.endian == mpt::endian::little)
		{
			WriteInterleavedLE(f, settings.Channels, settings.Format, frameCount, interleaved);
		} else
		{
			WriteInterleavedBE(f, settings.Channels, settings.Format, frameCount, interleaved);
		}
	}
	void WriteInterleaved(std::size_t frameCount, const int8 *interleaved) override
	{
		if(settings.Format.endian == mpt::endian::little)
		{
			WriteInterleavedLE(f, settings.Channels, settings.Format, frameCount, interleaved);
		} else
		{
			WriteInterleavedBE(f, settings.Channels, settings.Format, frameCount, interleaved);
		}
	}
	void WriteInterleaved(std::size_t frameCount, const uint8 *interleaved) override
	{
		if(settings.Format.endian == mpt::endian::little)
		{
			WriteInterleavedLE(f, settings.Channels, settings.Format, frameCount, interleaved);
		} else
		{
			WriteInterleavedBE(f, settings.Channels, settings.Format, frameCount, interleaved);
		}
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
	traits.samplerates = {};
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
	traits.formats.push_back({ Encoder::Format::Encoding::Alaw, 16, mpt::get_endian() });
	traits.formats.push_back({ Encoder::Format::Encoding::ulaw, 16, mpt::get_endian() });
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
