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
#include "StreamEncoderOpus.h"

#include "../common/mptIO.h"
#include "../common/mptBufferIO.h"

#include "../common/ComponentManager.h"

#include "Mptrack.h"

#include <deque>

#ifdef MPT_WITH_OGG
#include <ogg/ogg.h>
#endif
#include <opus/opus.h>
#include <opus/opus_multistream.h>


OPENMPT_NAMESPACE_BEGIN



class ComponentOpus
	: public ComponentLibrary
{

	MPT_DECLARE_COMPONENT_MEMBERS

public:

	// opus
	const char * (*opus_get_version_string)();

	int (*opus_multistream_encoder_ctl)(OpusMSEncoder *st, int request, ...);
	int (*opus_multistream_encode_float)(OpusMSEncoder *st, const float *pcm, int frame_size, unsigned char *data, opus_int32 max_data_bytes);
	void (*opus_multistream_encoder_destroy)(OpusMSEncoder *st);

	// 1.0
	OpusMSEncoder * (*opus_multistream_encoder_create)(opus_int32 Fs, int channels, int streams, int coupled_streams, const unsigned char *mapping, int application, int *error);

	// 1.1
	OpusMSEncoder * (*opus_multistream_surround_encoder_create)(opus_int32 Fs, int channels, int mapping_family, int *streams, int *coupled_streams, unsigned char *mapping, int application, int *error);
		
private:

	void Reset()
	{
		ClearLibraries();
	}

public:

	ComponentOpus()
		: ComponentLibrary(ComponentTypeForeign)
	{
		return;
	}

protected:

	bool DoInitialize()
	{
		Reset();
		struct dll_names_t {
			const char *opus;
		};
		// start with trying all symbols from a single dll first
		static const dll_names_t dll_names[] = {
			{ "libopus-0"  }, // official xiph.org builds
			{ "libopus"    },
			{ "opus"       }
		};
		bool ok = false;
		for(std::size_t i=0; i<CountOf(dll_names); ++i)
		{
			if(TryLoad(mpt::PathString::FromUTF8(dll_names[i].opus)))
			{
				ok = true;
				break;
			}
		}
		return ok;
	}

private:

	bool TryLoad(const mpt::PathString &Opus_fn)
	{
		Reset();
		ClearBindFailed();
		if(!AddLibrary("opus", mpt::LibraryPath::AppFullName(Opus_fn)))
		{
			Reset();
			return false;
		}
		MPT_COMPONENT_BIND_OPTIONAL("opus", opus_get_version_string);
		MPT_COMPONENT_BIND("opus", opus_multistream_encoder_ctl);
		MPT_COMPONENT_BIND("opus", opus_multistream_encode_float);
		MPT_COMPONENT_BIND("opus", opus_multistream_encoder_destroy);
		MPT_COMPONENT_BIND("opus", opus_multistream_encoder_create);
		MPT_COMPONENT_BIND_OPTIONAL("opus", opus_multistream_surround_encoder_create);
		if(HasBindFailed())
		{
			Reset();
			return false;
		}
#ifdef MPT_WITH_OGG
		return true;
#else
		return false;
#endif
	}

public:

	Encoder::Traits BuildTraits()
	{
		Encoder::Traits traits;
		if(!IsAvailable())
		{
			return traits;
		}
		traits.fileExtension = MPT_PATHSTRING("opus");
		traits.fileShortDescription = MPT_USTRING("Opus");
		traits.fileDescription = MPT_USTRING("Opus");
		traits.encoderSettingsName = MPT_USTRING("Opus");
		traits.encoderName = MPT_USTRING("libOpus");
		traits.description += MPT_USTRING("Version: ");
		traits.description += mpt::ToUnicode(mpt::CharsetASCII, opus_get_version_string && opus_get_version_string() ? opus_get_version_string() : "");
		traits.description += MPT_USTRING("\n");
		traits.canTags = true;
		traits.maxChannels = 4;
		traits.samplerates = std::vector<uint32>(opus_samplerates, opus_samplerates + CountOf(opus_samplerates));
		traits.modes = Encoder::ModeCBR | Encoder::ModeVBR;
		traits.bitrates = std::vector<int>(opus_bitrates, opus_bitrates + CountOf(opus_bitrates));
		traits.defaultSamplerate = 48000;
		traits.defaultChannels = 2;
		traits.defaultMode = Encoder::ModeVBR;
		traits.defaultBitrate = 128;
		return traits;
	}

};
MPT_REGISTERED_COMPONENT(ComponentOpus, "Opus")



#ifdef MPT_WITH_OGG

class OpusStreamWriter : public StreamWriterBase
{
private:
	ComponentOpus &opus;
  ogg_stream_state os;
  ogg_page         og;
  ogg_packet       op;
	OpusMSEncoder*   st;
	bool inited;
	bool started;
	int opus_bitrate;
	int opus_samplerate;
	int opus_channels;
	bool opus_tags;

	std::vector<std::string> opus_comments;

	int opus_extrasamples;

	ogg_int64_t packetno;
	ogg_int64_t last_granulepos;
	ogg_int64_t enc_granulepos;
	ogg_int64_t original_samples;

	std::deque<float> opus_sampleBuf;
	std::vector<float> opus_frameBuf;
	std::vector<unsigned char> opus_frameData;
private:
	static void PushUint32LE(std::vector<unsigned char> &buf, uint32 val)
	{
		buf.push_back((val>> 0)&0xff);
		buf.push_back((val>> 8)&0xff);
		buf.push_back((val>>16)&0xff);
		buf.push_back((val>>24)&0xff);
	}
	void StartStream()
	{
		ASSERT(inited && !started);

		std::vector<unsigned char> opus_comments_buf;
		for(const char *it = "OpusTags"; *it; ++it)
		{
			opus_comments_buf.push_back(*it);
		}
		const char *version_string = opus.opus_get_version_string ? opus.opus_get_version_string() : nullptr;
		if(version_string)
		{
			PushUint32LE(opus_comments_buf, mpt::saturate_cast<uint32>(std::strlen(version_string)));
			for(/*nothing*/; *version_string; ++version_string)
			{
				opus_comments_buf.push_back(*version_string);
			}
		} else
		{
			PushUint32LE(opus_comments_buf, 0);
		}
		PushUint32LE(opus_comments_buf, mpt::saturate_cast<uint32>(opus_comments.size()));
		for(std::vector<std::string>::const_iterator it = opus_comments.begin(); it != opus_comments.end(); ++it)
		{
			PushUint32LE(opus_comments_buf, mpt::saturate_cast<uint32>(it->length()));
			for(std::size_t i = 0; i < it->length(); ++i)
			{
				opus_comments_buf.push_back((*it)[i]);
			}
		}
		op.packet = &opus_comments_buf[0];
		op.bytes = mpt::saturate_cast<long>(opus_comments_buf.size());
		op.b_o_s = 0;
		op.e_o_s = 0;
		op.granulepos = 0;
		op.packetno = 1;
		ogg_stream_packetin(&os, &op);
		while(ogg_stream_flush(&os, &og))
		{
			WritePage();
		}
		packetno = 2;

		last_granulepos = 0;
		enc_granulepos = 0;
		original_samples = 0;
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

			std::vector<float> extraBuf(opus_extrasamples * opus_channels);
			WriteInterleaved(opus_extrasamples, &extraBuf[0]);

			int cur_frame_size = 960 * opus_samplerate / 48000;
			int last_frame_size = (static_cast<int>(opus_sampleBuf.size()) / opus_channels) * opus_samplerate / 48000;

			opus_frameBuf.resize(opus_channels * cur_frame_size);
			for(size_t sample = 0; sample < opus_frameBuf.size(); ++sample)
			{
				opus_frameBuf[sample] = 0.0f;
			}

			for(size_t sample = 0; sample < opus_sampleBuf.size(); ++sample)
			{
				opus_frameBuf[sample] = opus_sampleBuf[sample];
			}
			opus_sampleBuf.clear();

			opus_frameData.resize(65536);
			opus_frameData.resize(opus.opus_multistream_encode_float(st, &opus_frameBuf[0], cur_frame_size, &opus_frameData[0], static_cast<opus_int32>(opus_frameData.size())));
			enc_granulepos += last_frame_size * 48000 / opus_samplerate;

			op.b_o_s = 0;
			op.e_o_s = 1;
			op.granulepos = enc_granulepos;
			op.packetno = packetno;
			op.packet = &opus_frameData[0];
			op.bytes = static_cast<long>(opus_frameData.size());
			ogg_stream_packetin(&os, &op);

			packetno++;

			while(ogg_stream_flush(&os, &og))
			{
				WritePage();
			}

			ogg_stream_clear(&os);

			started = false;
			inited = false;
		}
		ASSERT(!inited && !started);
	}
	void WritePage()
	{
		ASSERT(inited);
		buf.resize(og.header_len);
		std::memcpy(&buf[0], og.header, og.header_len);
		WriteBuffer();
		buf.resize(og.body_len);
		std::memcpy(&buf[0], og.body, og.body_len);
		WriteBuffer();
	}
	void AddCommentField(const std::string &field, const mpt::ustring &data)
	{
		if(!field.empty() && !data.empty())
		{
			opus_comments.push_back(field + "=" + mpt::ToCharset(mpt::CharsetUTF8, data));
		}
	}
public:
	OpusStreamWriter(ComponentOpus &opus_, std::ostream &stream)
		: StreamWriterBase(stream)
		, opus(opus_)
	{
		inited = false;
		started = false;
		opus_channels = 0;
		opus_tags = true;
		opus_comments.clear();
		opus_extrasamples = 0;
	}
	virtual ~OpusStreamWriter()
	{
		FinishStream();
		ASSERT(!inited && !started);
	}
	virtual void SetFormat(const Encoder::Settings &settings)
	{

		FinishStream();

		ASSERT(!inited && !started);

		uint32 samplerate = settings.Samplerate;
		uint16 channels = settings.Channels;

		opus_bitrate = settings.Bitrate * 1000;
		opus_samplerate = samplerate;
		opus_channels = channels;
		opus_tags = settings.Tags;

		int opus_error = 0;

		int num_streams = 0;
		int num_coupled = 0;
		unsigned char mapping[4] = { 0, 0, 0, 0 };

		if(opus.opus_multistream_surround_encoder_create)
		{
			// 1.1

			st = opus.opus_multistream_surround_encoder_create(samplerate, opus_channels, opus_channels > 2 ? 1 : 0, &num_streams, &num_coupled, mapping, OPUS_APPLICATION_AUDIO, &opus_error);

		} else
		{
			// 1.0

			struct opus_channels_to_streams_t {
				unsigned char streams;
				unsigned char coupled;
			};
			static const opus_channels_to_streams_t channels_to_streams[] = {
				/*0*/ { 0, 0 },
				/*1*/ { 1, 0 },
				/*2*/ { 1, 1 },
				/*3*/ { 2, 1 },
				/*4*/ { 2, 2 }
			};
			static const unsigned char channels_to_mapping [4][4] = {
				{ 0, 0, 0, 0 },
				{ 0, 1, 0, 0 },
				{ 0, 1, 2, 0 },
				{ 0, 1, 2, 3 }
			};

			st = opus.opus_multistream_encoder_create(samplerate, opus_channels, channels_to_streams[channels].streams, channels_to_streams[channels].coupled, channels_to_mapping[channels], OPUS_APPLICATION_AUDIO, &opus_error);

			num_streams = channels_to_streams[channels].streams;
			num_coupled = channels_to_streams[channels].coupled;
			for(int channel=0; channel<opus_channels; ++channel)
			{
				mapping[channel] = channels_to_mapping[opus_channels][channel];
			}
		}

		opus_int32 ctl_lookahead = 0;
		opus.opus_multistream_encoder_ctl(st, OPUS_GET_LOOKAHEAD(&ctl_lookahead));

		opus_int32 ctl_bitrate = opus_bitrate;
		opus.opus_multistream_encoder_ctl(st, OPUS_SET_BITRATE(ctl_bitrate));

		if(settings.Mode == Encoder::ModeCBR)
		{
			opus_int32 ctl_vbr = 0;
			opus.opus_multistream_encoder_ctl(st, OPUS_SET_VBR(ctl_vbr));
		} else
		{
			opus_int32 ctl_vbr = 1;
			opus.opus_multistream_encoder_ctl(st, OPUS_SET_VBR(ctl_vbr));
			opus_int32 ctl_vbrcontraint = 0;
			opus.opus_multistream_encoder_ctl(st, OPUS_SET_VBR_CONSTRAINT(ctl_vbrcontraint));
		}

		opus_int32 complexity = StreamEncoderSettings::Instance().OpusComplexity;
		if(complexity >= 0)
		{
			opus.opus_multistream_encoder_ctl(st, OPUS_SET_COMPLEXITY(complexity));
		}

		opus_extrasamples = ctl_lookahead;

		opus_comments.clear();

		ogg_stream_init(&os, std::rand());

		inited = true;

		mpt::ostringstream buf(std::ios::binary);
		buf.imbue(std::locale::classic());

		mpt::IO::WriteRaw(buf, "Opus", 4);
		mpt::IO::WriteRaw(buf, "Head", 4);
		mpt::IO::WriteIntLE<uint8>(buf, 1); // version
		mpt::IO::WriteIntLE<uint8>(buf, static_cast<uint8>(opus_channels)); // channels
		mpt::IO::WriteIntLE<uint16>(buf, static_cast<uint16>(ctl_lookahead * (48000/samplerate))); // preskip
		mpt::IO::WriteIntLE<uint32>(buf, samplerate); // samplerate
		mpt::IO::WriteIntLE<uint16>(buf, 0); // gain
		mpt::IO::WriteIntLE<uint8>(buf, (opus_channels > 2) ? 1 : 0); //chanmap

		if(opus_channels > 2)
		{
			mpt::IO::WriteIntLE<uint8>(buf, static_cast<uint8>(num_streams));
			mpt::IO::WriteIntLE<uint8>(buf, static_cast<uint8>(num_coupled));
			for(int channel=0; channel<opus_channels; ++channel)
			{
				mpt::IO::WriteIntLE<uint8>(buf, mapping[channel]);
			}
		}

		std::string header_str = buf.str();
		std::vector<unsigned char> header_buf(header_str.data(), header_str.data() + header_str.size());

		op.packet = &(header_buf[0]);
		op.bytes = header_buf.size();
		op.b_o_s = 1;
		op.e_o_s = 0;
		op.granulepos = 0;
		op.packetno = 0;
		ogg_stream_packetin(&os, &op);

		while(ogg_stream_flush(&os, &og))
		{
			WritePage();
		}

		ASSERT(inited && !started);
	}
	virtual void WriteMetatags(const FileTags &tags)
	{
		ASSERT(inited && !started);
		AddCommentField("ENCODER", tags.encoder);
		if(opus_tags)
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
	}
	virtual void WriteInterleaved(size_t count, const float *interleaved)
	{
		ASSERT(inited);
		if(!started)
		{
			StartStream();
		}
		ASSERT(inited && started);
		original_samples += count;
		for(size_t frame = 0; frame < count; ++frame)
		{
			for(int channel = 0; channel < opus_channels; ++channel)
			{
				opus_sampleBuf.push_back(interleaved[frame*opus_channels+channel]);
			}
		}
		int cur_frame_size = 960 * opus_samplerate / 48000;
		opus_frameBuf.resize(opus_channels * cur_frame_size);
		while(opus_sampleBuf.size() > opus_frameBuf.size())
		{
			for(size_t sample = 0; sample < opus_frameBuf.size(); ++sample)
			{
				opus_frameBuf[sample] = opus_sampleBuf.front();
				opus_sampleBuf.pop_front();
			}

			opus_frameData.resize(65536);
			opus_frameData.resize(opus.opus_multistream_encode_float(st, &opus_frameBuf[0], cur_frame_size, &opus_frameData[0], static_cast<opus_int32>(opus_frameData.size())));
			enc_granulepos += cur_frame_size * 48000 / opus_samplerate;

			op.b_o_s = 0;
			op.e_o_s = 0;
			op.granulepos = enc_granulepos;
			op.packetno = packetno;
			op.packet = &opus_frameData[0];
			op.bytes = static_cast<long>(opus_frameData.size());
			ogg_stream_packetin(&os, &op);

			packetno++;

			while(ogg_stream_pageout(&os, &og))
			{
				WritePage();
			}

		}
	}
	virtual void Finalize()
	{
		ASSERT(inited);
		FinishStream();
		ASSERT(!inited && !started);
	}
};

#endif // MPT_WITH_OGG



OggOpusEncoder::OggOpusEncoder()
//------------------------------
{
	if(IsComponentAvailable(m_Opus))
	{
		SetTraits(m_Opus->BuildTraits());
	}
}


bool OggOpusEncoder::IsAvailable() const
//--------------------------------------
{
	return IsComponentAvailable(m_Opus);
}


OggOpusEncoder::~OggOpusEncoder()
//-------------------------------
{
	return;
}


IAudioStreamEncoder *OggOpusEncoder::ConstructStreamEncoder(std::ostream &file) const
//-----------------------------------------------------------------------------------
{
	if(!IsAvailable())
	{
		return nullptr;
	}
#ifdef MPT_WITH_OGG
	return new OpusStreamWriter(*m_Opus, file);
#else
	return nullptr;
#endif
}



OPENMPT_NAMESPACE_END
