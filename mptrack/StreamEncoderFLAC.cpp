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
	bool inited;
	bool started;
	const FLACEncoder &enc;
	Encoder::Format formatInfo;
	bool writeTags;
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
	void StartStream()
	{
		ASSERT(inited && !started);

		FLAC__stream_encoder_init_stream(encoder, FLACWriteCallback, FLACSeekCallback, FLACTellCallback, nullptr, this);

		started = true;
		ASSERT(inited && started);
	}
	void FinishStream()
	{
		if(inited)
		{
			if(!started)
			{
				StartStream();
			}
			ASSERT(inited && started);

			FLAC__stream_encoder_finish(encoder);
			FLAC__stream_encoder_delete(encoder);
			encoder = nullptr;

			if(flac_metadata[0])
			{
				FLAC__metadata_object_delete(flac_metadata[0]);
				flac_metadata[0] = nullptr;
			}

			started = false;
			inited = false;
		}
		ASSERT(!inited && !started);
	}
	void AddCommentField(const std::string &field, const mpt::ustring &data)
	{
		if(!field.empty() && !data.empty())
		{
			FLAC__StreamMetadata_VorbisComment_Entry entry;
			FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, field.c_str(), mpt::ToCharset(mpt::CharsetUTF8, data).c_str());
			FLAC__metadata_object_vorbiscomment_append_comment(flac_metadata[0], entry, false);
		}
	}
public:
	FLACStreamWriter(const FLACEncoder &enc_, std::ostream &stream)
		: StreamWriterBase(stream)
		, enc(enc_)
	{
		inited = false;
		started = false;

		writeTags = false;
		flac_metadata[0] = nullptr;
		encoder = nullptr;

	}
	virtual ~FLACStreamWriter()
	{
		FinishStream();
		ASSERT(!inited && !started);
	}
	virtual void SetFormat(const Encoder::Settings &settings)
	{
		FinishStream();

		ASSERT(!inited && !started);

		formatInfo = enc.GetTraits().formats[settings.Format];
		ASSERT(formatInfo.Sampleformat.IsValid());
		ASSERT(formatInfo.Samplerate > 0);
		ASSERT(formatInfo.Channels > 0);

		writeTags = settings.Tags;

		encoder = FLAC__stream_encoder_new();

		FLAC__stream_encoder_set_channels(encoder, formatInfo.Channels);
		FLAC__stream_encoder_set_bits_per_sample(encoder, formatInfo.Sampleformat.GetBitsPerSample());
		FLAC__stream_encoder_set_sample_rate(encoder, formatInfo.Samplerate);

		int compressionLevel = StreamEncoderSettings::Instance().FLACCompressionLevel;
		FLAC__stream_encoder_set_compression_level(encoder, compressionLevel);
		
		inited = true;

		ASSERT(inited && !started);
	}
	virtual void WriteMetatags(const FileTags &tags)
	{
		ASSERT(inited && !started);
		flac_metadata[0] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT);
		AddCommentField("ENCODER", tags.encoder);
		if(writeTags)
		{
			AddCommentField("SOURCEMEDIA", MPT_USTRING("tracked music file"));
			AddCommentField("TITLE",       tags.title          );
			AddCommentField("ARTIST",      tags.artist         );
			AddCommentField("ALBUM",       tags.album          );
			AddCommentField("DATE",        tags.year           );
			AddCommentField("COMMENT",     tags.comments       );
			AddCommentField("GENRE",       tags.genre          );
			AddCommentField("CONTACT",     tags.url            );
			AddCommentField("BPM",         tags.bpm            ); // non-standard
			AddCommentField("TRACKNUMBER", tags.trackno        );
		}
		FLAC__stream_encoder_set_metadata(encoder, flac_metadata, 1);
	}
	virtual void WriteInterleaved(size_t count, const float *interleaved)
	{
		ASSERT(formatInfo.Sampleformat.IsFloat());
		WriteInterleavedConverted(count, reinterpret_cast<const char*>(interleaved));
	}
	virtual void WriteInterleavedConverted(size_t frameCount, const char *data)
	{
		ASSERT(inited);
		if(!started)
		{
			StartStream();
		}
		ASSERT(inited && started);
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
						sampleBuf[frame * formatInfo.Channels + channel] = *p - 0x80;
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
			FLAC__stream_encoder_process_interleaved(encoder, &sampleBuf[0], frameCountChunk);
			frameCount -= frameCountChunk;
		}
	}
	virtual void Finalize()
	{
		ASSERT(inited);
		FinishStream();
		ASSERT(!inited && !started);
	}
};



FLACEncoder::FLACEncoder()
//------------------------
{
	Encoder::Traits traits;
	traits.fileExtension = MPT_PATHSTRING("flac");
	traits.fileShortDescription = MPT_USTRING("FLAC");
	traits.fileDescription = MPT_USTRING("FLAC");
	traits.encoderSettingsName = MPT_USTRING("FLAC");
	traits.encoderName = MPT_USTRING("libFLAC");
	traits.description = MPT_USTRING("");
	traits.description += mpt::String::Print(MPT_USTRING("Free Lossless Audio Codec\n"));
	traits.description += mpt::String::Print(MPT_USTRING("Vendor: %1\n"), mpt::ToUnicode(mpt::CharsetASCII, FLAC__VENDOR_STRING));
	traits.description += mpt::String::Print(MPT_USTRING("Version: %1\n"), mpt::ToUnicode(mpt::CharsetASCII, FLAC__VERSION_STRING));
	traits.description += mpt::String::Print(MPT_USTRING("API: %1.%2.%3\n"), FLAC_API_VERSION_CURRENT, FLAC_API_VERSION_REVISION, FLAC_API_VERSION_AGE);
	traits.canTags = true;
	traits.maxChannels = 4;
	traits.samplerates = TrackerSettings::Instance().GetSampleRates();
	traits.modes = Encoder::ModeEnumerated;
	for(std::size_t i = 0; i < traits.samplerates.size(); ++i)
	{
		int samplerate = traits.samplerates[i];
		for(int channels = 1; channels <= traits.maxChannels; channels *= 2)
		{
			for(int bytes = 3; bytes >= 1; --bytes)
			{
				Encoder::Format format;
				format.Samplerate = samplerate;
				format.Channels = channels;
				format.Sampleformat = (SampleFormat)(bytes * 8);
				format.Description = mpt::String::Print(MPT_USTRING("%1 Bit"), bytes * 8);
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
//-----------------------------------
{
	return true;
}


FLACEncoder::~FLACEncoder()
//------------------------
{
	return;
}


IAudioStreamEncoder *FLACEncoder::ConstructStreamEncoder(std::ostream &file) const
//--------------------------------------------------------------------------------
{
	if(!IsAvailable())
	{
		return nullptr;
	}
	return new FLACStreamWriter(*this, file);
}


OPENMPT_NAMESPACE_END
