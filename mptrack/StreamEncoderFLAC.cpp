/*
 * StreamEncoder.cpp
 * -----------------
 * Purpose: Exporting streamed music files.
 * Notes  : none
 * Authors: Joern Heusipp
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"

#include "StreamEncoder.h"
#include "StreamEncoderFLAC.h"

#include "Mptrack.h"
#include "TrackerSettings.h"

#include <FLAC/metadata.h>
#include <FLAC/format.h>
#include <FLAC/stream_encoder.h>


OPENMPT_NAMESPACE_BEGIN


class FLACStreamWriter : public StreamWriterBase
{
private:
	const FLACEncoder &enc;
	Encoder::Format formatInfo;
	FLAC__StreamMetadata *flac_metadata[1];
	FLAC__StreamEncoder *encoder;
	std::vector<FLAC__int32> sampleBuf;
private:
	static FLAC__StreamEncoderWriteStatus FLACWriteCallback(const FLAC__StreamEncoder *flacenc, const FLAC__byte buffer[], size_t bytes, unsigned samples, unsigned current_frame, void *client_data)
	{
		return reinterpret_cast<FLACStreamWriter*>(client_data)->WriteCallback(flacenc, buffer, bytes, samples, current_frame);
	}
	static FLAC__StreamEncoderSeekStatus FLACSeekCallback(const FLAC__StreamEncoder *flacenc, FLAC__uint64 absolute_byte_offset, void *client_data)
	{
		return reinterpret_cast<FLACStreamWriter*>(client_data)->SeekCallback(flacenc, absolute_byte_offset);
	}
	static FLAC__StreamEncoderTellStatus FLACTellCallback(const FLAC__StreamEncoder *flacenc, FLAC__uint64 *absolute_byte_offset, void *client_data)
	{
		return reinterpret_cast<FLACStreamWriter*>(client_data)->TellCallback(flacenc, absolute_byte_offset);
	}
	FLAC__StreamEncoderWriteStatus WriteCallback(const FLAC__StreamEncoder *flacenc, const FLAC__byte buffer[], size_t bytes, unsigned samples, unsigned current_frame)
	{
		MPT_UNREFERENCED_PARAMETER(flacenc);
		MPT_UNREFERENCED_PARAMETER(samples);
		MPT_UNREFERENCED_PARAMETER(current_frame);
		f.write(reinterpret_cast<const char*>(buffer), bytes);
		if(!f) return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
		return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
	}
	FLAC__StreamEncoderSeekStatus SeekCallback(const FLAC__StreamEncoder *flacenc, FLAC__uint64 absolute_byte_offset)
	{
		MPT_UNREFERENCED_PARAMETER(flacenc);
		f.seekp(absolute_byte_offset);
		if(!f) return FLAC__STREAM_ENCODER_SEEK_STATUS_ERROR;
		return FLAC__STREAM_ENCODER_SEEK_STATUS_OK;
	}
	FLAC__StreamEncoderTellStatus TellCallback(const FLAC__StreamEncoder *flacenc, FLAC__uint64 *absolute_byte_offset)
	{
		MPT_UNREFERENCED_PARAMETER(flacenc);
		if(absolute_byte_offset)
		{
			*absolute_byte_offset = f.tellp();
		}
		if(!f) return FLAC__STREAM_ENCODER_TELL_STATUS_ERROR;
		return FLAC__STREAM_ENCODER_TELL_STATUS_OK;
	}
private:
	void AddCommentField(const std::string &field, const mpt::ustring &data)
	{
		if(!field.empty() && !data.empty())
		{
			FLAC__StreamMetadata_VorbisComment_Entry entry;
			FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, field.c_str(), mpt::ToCharset(mpt::Charset::UTF8, data).c_str());
			FLAC__metadata_object_vorbiscomment_append_comment(flac_metadata[0], entry, false);
		}
	}
public:
	FLACStreamWriter(const FLACEncoder &enc_, std::ostream &stream, const Encoder::Settings &settings, const FileTags &tags)
		: StreamWriterBase(stream)
		, enc(enc_)
	{
		flac_metadata[0] = nullptr;
		encoder = nullptr;

		formatInfo = enc.GetTraits().formats[settings.Format];
		ASSERT(formatInfo.Sampleformat.IsValid());
		ASSERT(formatInfo.Samplerate > 0);
		ASSERT(formatInfo.Channels > 0);

		encoder = FLAC__stream_encoder_new();

		FLAC__stream_encoder_set_channels(encoder, formatInfo.Channels);
		FLAC__stream_encoder_set_bits_per_sample(encoder, formatInfo.Sampleformat.GetBitsPerSample());
		FLAC__stream_encoder_set_sample_rate(encoder, formatInfo.Samplerate);

		int compressionLevel = settings.Details.FLACCompressionLevel;
		FLAC__stream_encoder_set_compression_level(encoder, compressionLevel);
		
		if(settings.Tags)
		{
			flac_metadata[0] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT);
			AddCommentField("ENCODER",     tags.encoder);
			AddCommentField("SOURCEMEDIA", U_("tracked music file"));
			AddCommentField("TITLE",       tags.title          );
			AddCommentField("ARTIST",      tags.artist         );
			AddCommentField("ALBUM",       tags.album          );
			AddCommentField("DATE",        tags.year           );
			AddCommentField("COMMENT",     tags.comments       );
			AddCommentField("GENRE",       tags.genre          );
			AddCommentField("CONTACT",     tags.url            );
			AddCommentField("BPM",         tags.bpm            ); // non-standard
			AddCommentField("TRACKNUMBER", tags.trackno        );
			FLAC__stream_encoder_set_metadata(encoder, flac_metadata, 1);
		}

		FLAC__stream_encoder_init_stream(encoder, FLACWriteCallback, FLACSeekCallback, FLACTellCallback, nullptr, this);

	}
	void WriteInterleaved(size_t count, const float *interleaved) override
	{
		ASSERT(formatInfo.Sampleformat.IsFloat());
		WriteInterleavedConverted(count, reinterpret_cast<const std::byte*>(interleaved));
	}
	void WriteInterleavedConverted(size_t frameCount, const std::byte *data) override
	{
		sampleBuf.resize(frameCount * formatInfo.Channels);
		switch(formatInfo.Sampleformat.GetBitsPerSample()/8)
		{
			case 1:
			{
				const uint8 *p = reinterpret_cast<const uint8*>(data);
				for(std::size_t frame = 0; frame < frameCount; ++frame)
				{
					for(int channel = 0; channel < formatInfo.Channels; ++channel)
					{
						sampleBuf[frame * formatInfo.Channels + channel] = *p;
						p++;
					}
				}
			}
			break;
			case 2:
			{
				const int16 *p = reinterpret_cast<const int16*>(data);
				for(std::size_t frame = 0; frame < frameCount; ++frame)
				{
					for(int channel = 0; channel < formatInfo.Channels; ++channel)
					{
						sampleBuf[frame * formatInfo.Channels + channel] = *p;
						p++;
					}
				}
			}
			break;
			case 3:
			{
				const int24 *p = reinterpret_cast<const int24*>(data);
				for(std::size_t frame = 0; frame < frameCount; ++frame)
				{
					for(int channel = 0; channel < formatInfo.Channels; ++channel)
					{
						sampleBuf[frame * formatInfo.Channels + channel] = *p;
						p++;
					}
				}
			}
			break;
		}
		while(frameCount > 0)
		{
			unsigned int frameCountChunk = mpt::saturate_cast<unsigned int>(frameCount);
			FLAC__stream_encoder_process_interleaved(encoder, sampleBuf.data(), frameCountChunk);
			frameCount -= frameCountChunk;
		}
	}
	void WriteFinalize() override
	{
		FLAC__stream_encoder_finish(encoder);
	}
	virtual ~FLACStreamWriter()
	{
		FLAC__stream_encoder_delete(encoder);
		encoder = nullptr;

		if(flac_metadata[0])
		{
			FLAC__metadata_object_delete(flac_metadata[0]);
			flac_metadata[0] = nullptr;
		}
	}
};



FLACEncoder::FLACEncoder()
{
	Encoder::Traits traits;
	traits.fileExtension = P_("flac");
	traits.fileShortDescription = U_("FLAC");
	traits.fileDescription = U_("Free Lossless Audio Codec");
	traits.encoderSettingsName = U_("FLAC");
	traits.canTags = true;
	traits.maxChannels = 4;
	traits.samplerates = TrackerSettings::Instance().GetSampleRates();
	traits.modes = Encoder::ModeEnumerated;
	for(std::size_t i = 0; i < traits.samplerates.size(); ++i)
	{
		int samplerate = traits.samplerates[i];
		for(int channels = 1; channels <= traits.maxChannels; channels *= 2)
		{
			const std::array<SampleFormat, 3> sampleFormats = { SampleFormatInt24, SampleFormatInt16, SampleFormatInt8 };
			for(const auto sampleFormat : sampleFormats)
			{
				Encoder::Format format;
				format.Samplerate = samplerate;
				format.Channels = channels;
				format.Sampleformat = sampleFormat;
				format.Description = MPT_UFORMAT("{} Bit")(sampleFormat.GetBitsPerSample());
				format.Bitrate = 0;
				traits.formats.push_back(format);
			}
		}
	}
	traits.defaultSamplerate = 48000;
	traits.defaultChannels = 2;
	traits.defaultMode = Encoder::ModeEnumerated;
	traits.defaultFormat = 0;
	SetTraits(traits);
}


bool FLACEncoder::IsAvailable() const
{
	return true;
}


std::unique_ptr<IAudioStreamEncoder> FLACEncoder::ConstructStreamEncoder(std::ostream &file, const Encoder::Settings &settings, const FileTags &tags) const
{
	if(!IsAvailable())
	{
		return nullptr;
	}
	return std::make_unique<FLACStreamWriter>(*this, file, settings, tags);
}


OPENMPT_NAMESPACE_END
