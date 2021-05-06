/*
 * StreamEncoderAU.cpp
 * -------------------
 * Purpose: Exporting streamed music files.
 * Notes  : none
 * Authors: Joern Heusipp
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"

#include "StreamEncoder.h"
#include "StreamEncoderAU.h"

#include "Mptrack.h"
#include "TrackerSettings.h"

#include "../common/mptFileIO.h"
#include "../soundbase/SampleFormatConverters.h"


OPENMPT_NAMESPACE_BEGIN


class AUStreamWriter : public IAudioStreamEncoder
{
private:

	const AUEncoder &enc;
	std::ostream &f;
	Encoder::Settings settings;

private:
	static std::string TagToAnnotation(const std::string & field, const mpt::ustring & tag)
	{
		if(tag.empty())
		{
			return std::string();
		}
		return MPT_FORMAT("{}={}\n")(field, mpt::ToCharset(mpt::Charset::UTF8, mpt::String::Replace(tag, U_("="), MPT_UTF8("\xEF\xBF\xBD")))); // U+FFFD
	}

public:
	AUStreamWriter(const AUEncoder &enc_, std::ostream &file, const Encoder::Settings &settings_, const FileTags &tags)
		: enc(enc_)
		, f(file)
		, settings(settings_)
	{

		MPT_ASSERT(settings.Format.GetSampleFormat().IsValid());
		MPT_ASSERT(settings.Samplerate > 0);
		MPT_ASSERT(settings.Channels > 0);

		std::string annotation;
		std::size_t annotationSize = 0;
		std::size_t annotationTotalSize = 8;
		if(settings.Tags)
		{
			// same format as invented by sox and implemented by ffmpeg
			annotation += TagToAnnotation("title", tags.title);
			annotation += TagToAnnotation("artist", tags.artist);
			annotation += TagToAnnotation("album", tags.album);
			annotation += TagToAnnotation("track", tags.trackno);
			annotation += TagToAnnotation("genre", tags.genre);
			annotation += TagToAnnotation("comment", tags.comments);
			annotationSize = annotation.length() + 1;
			annotationTotalSize = annotationSize;
			if(settings.Details.AUPaddingAlignHint > 0)
			{
				annotationTotalSize = Util::AlignUp<std::size_t>(24u + annotationTotalSize, settings.Details.AUPaddingAlignHint) - 24u;
			}
			annotationTotalSize = Util::AlignUp<std::size_t>(annotationTotalSize, 8u);
		}
		MPT_ASSERT(annotationTotalSize >= annotationSize);
		MPT_ASSERT(annotationTotalSize % 8 == 0);

		mpt::IO::WriteText(f, ".snd");
		mpt::IO::WriteIntBE<uint32>(f, mpt::saturate_cast<uint32>(24u + annotationTotalSize));
		mpt::IO::WriteIntBE<uint32>(f, ~uint32(0));
		uint32 encoding = 0;
		if(settings.Format.encoding == Encoder::Format::Encoding::Float)
		{
			switch(settings.Format.bits)
			{
			case 32: encoding = 6; break;
			case 64: encoding = 7; break;
			}
		} else if(settings.Format.encoding == Encoder::Format::Encoding::Integer)
		{
			switch(settings.Format.bits)
			{
			case  8: encoding = 2; break;
			case 16: encoding = 3; break;
			case 24: encoding = 4; break;
			case 32: encoding = 5; break;
			}
		} else if(settings.Format.encoding == Encoder::Format::Encoding::Alaw)
		{
			encoding = 27;
		} else if (settings.Format.encoding == Encoder::Format::Encoding::ulaw)
		{
			encoding = 1;
		}
		mpt::IO::WriteIntBE<uint32>(f, encoding);
		mpt::IO::WriteIntBE<uint32>(f, settings.Samplerate);
		mpt::IO::WriteIntBE<uint32>(f, settings.Channels);
		if(annotationSize > 0)
		{
			mpt::IO::WriteText(f, annotation);
			mpt::IO::WriteIntBE<uint8>(f, '\0');
		}
		for(std::size_t i = 0; i < annotationTotalSize - annotationSize; ++i)
		{
			mpt::IO::WriteIntBE<uint8>(f, '\0');
		}

	}
	mpt::endian GetConvertedEndianness() const override
	{
		return mpt::endian::big;
	}
	void WriteInterleaved(size_t count, const float *interleaved) override
	{
		MPT_ASSERT(settings.Format.GetSampleFormat().IsFloat());
		MPT_MAYBE_CONSTANT_IF(mpt::endian_is_big())
		{
			WriteInterleavedConverted(count, reinterpret_cast<const std::byte*>(interleaved));
		} else
		{
			std::vector<IEEE754binary32BE> frameData(settings.Channels);
			for(std::size_t frame = 0; frame < count; ++frame)
			{
				for(int channel = 0; channel < settings.Channels; ++channel)
				{
					frameData[channel] = IEEE754binary32BE(interleaved[channel]);
				}
				mpt::IO::WriteRaw(f, reinterpret_cast<const char*>(frameData.data()), settings.Channels * settings.Format.GetSampleFormat().GetSampleSize());
				interleaved += settings.Channels;
			}
		}
	}
	void WriteInterleavedConverted(size_t frameCount, const std::byte *data) override
	{
		if(settings.Format.encoding == Encoder::Format::Encoding::Alaw)
		{
			SC::EncodeALaw conv;
			for(std::size_t frame = 0; frame < frameCount; ++frame)
			{
				for(uint16 channel = 0; channel < settings.Channels; ++channel)
				{
					int16be sample;
					std::memcpy(&sample, data, sizeof(int16));
					std::byte sampledata = conv(sample);
					mpt::IO::WriteRaw(f, &sampledata, 1);
					data += sizeof(int16);
				}
			}
		} else if(settings.Format.encoding == Encoder::Format::Encoding::ulaw)
		{
			SC::EncodeuLaw conv; 
			for(std::size_t frame = 0; frame < frameCount; ++frame)
			{
				for(uint16 channel = 0; channel < settings.Channels; ++channel)
				{
					int16be sample;
					std::memcpy(&sample, data, sizeof(int16));
					std::byte sampledata = conv(sample);
					mpt::IO::WriteRaw(f, &sampledata, 1);
					data += sizeof(int16);
				}
			}
		} else
		{
			mpt::IO::WriteRaw(f, data, frameCount * settings.Channels * settings.Format.GetSampleFormat().GetSampleSize());
		}
	}
	void WriteCues(const std::vector<uint64> &cues) override
	{
		MPT_UNREFERENCED_PARAMETER(cues);
	}
	void WriteFinalize() override
	{
		return;
	}
	virtual ~AUStreamWriter()
	{
		return;
	}
};



AUEncoder::AUEncoder()
{
	Encoder::Traits traits;
	traits.fileExtension = P_("au");
	traits.fileShortDescription = U_("AU");
	traits.fileDescription = U_("NeXT/Sun Audio");
	traits.encoderSettingsName = U_("AU");
	traits.canTags = true;
	traits.canCues = false;
	traits.maxChannels = 4;
	traits.samplerates = TrackerSettings::Instance().GetSampleRates();
	traits.modes = Encoder::ModeLossless;
	traits.formats.push_back({ Encoder::Format::Encoding::Float, 64, mpt::endian::big });
	traits.formats.push_back({ Encoder::Format::Encoding::Float, 32, mpt::endian::big });
	traits.formats.push_back({ Encoder::Format::Encoding::Integer, 32, mpt::endian::big });
	traits.formats.push_back({ Encoder::Format::Encoding::Integer, 24, mpt::endian::big });
	traits.formats.push_back({ Encoder::Format::Encoding::Integer, 16, mpt::endian::big });
	traits.formats.push_back({ Encoder::Format::Encoding::Integer, 8, mpt::endian::big });
	traits.formats.push_back({ Encoder::Format::Encoding::Alaw, 16, mpt::endian::big });
	traits.formats.push_back({ Encoder::Format::Encoding::ulaw, 16, mpt::endian::big });
	traits.defaultSamplerate = 48000;
	traits.defaultChannels = 2;
	traits.defaultMode = Encoder::ModeLossless;
	traits.defaultFormat = { Encoder::Format::Encoding::Float, 32, mpt::endian::big };
	SetTraits(traits);
}


bool AUEncoder::IsAvailable() const
{
	return true;
}


std::unique_ptr<IAudioStreamEncoder> AUEncoder::ConstructStreamEncoder(std::ostream &file, const Encoder::Settings &settings, const FileTags &tags) const
{
	if(!IsAvailable())
	{
		return nullptr;
	}
	return std::make_unique<AUStreamWriter>(*this, file, settings, tags);
}


OPENMPT_NAMESPACE_END
