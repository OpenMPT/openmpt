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
#include "StreamEncoderVorbis.h"

#include "../common/ComponentManager.h"

#include "Mptrack.h"

#ifdef MPT_WITH_OGG
#include <ogg/ogg.h>
#endif
#ifdef MPT_WITH_VORBIS
#include <vorbis/codec.h>
#endif
#ifdef MPT_WITH_VORBISENC
#include <vorbis/vorbisenc.h>
#endif



OPENMPT_NAMESPACE_BEGIN



static Encoder::Traits VorbisBuildTraits()
	{
		Encoder::Traits traits;
#if defined(MPT_WITH_OGG) && defined(MPT_WITH_VORBIS) && defined(MPT_WITH_VORBISENC)
		traits.fileExtension = MPT_PATHSTRING("ogg");
		traits.fileShortDescription = MPT_USTRING("Vorbis");
		traits.fileDescription = MPT_USTRING("Ogg Vorbis");
		traits.encoderSettingsName = MPT_USTRING("Vorbis");
		traits.encoderName = MPT_USTRING("libVorbis");
		traits.description += MPT_USTRING("Version: ");
		traits.description += mpt::ToUnicode(mpt::CharsetASCII, vorbis_version_string()?vorbis_version_string():"unknown");
		traits.description += MPT_USTRING("\n");
		traits.canTags = true;
		traits.maxChannels = 4;
		traits.samplerates = std::vector<uint32>(vorbis_samplerates, vorbis_samplerates + CountOf(vorbis_samplerates));
		traits.modes = Encoder::ModeABR | Encoder::ModeQuality;
		traits.bitrates = std::vector<int>(vorbis_bitrates, vorbis_bitrates + CountOf(vorbis_bitrates));
		traits.defaultSamplerate = 48000;
		traits.defaultChannels = 2;
		traits.defaultMode = Encoder::ModeQuality;
		traits.defaultBitrate = 160;
		traits.defaultQuality = 0.5;
#endif
		return traits;
	}

#if defined(MPT_WITH_OGG) && defined(MPT_WITH_VORBIS) && defined(MPT_WITH_VORBISENC)

class VorbisStreamWriter : public StreamWriterBase
{
private:
  ogg_stream_state os;
  ogg_page         og;
  ogg_packet       op;
  vorbis_info      vi;
  vorbis_comment   vc;
  vorbis_dsp_state vd;
  vorbis_block     vb;
	bool inited;
	bool started;
	bool vorbis_cbr;
	int vorbis_channels;
	bool vorbis_tags;
private:
	void StartStream()
	{
		ASSERT(inited && !started);
		ogg_packet header;
		ogg_packet header_comm;
		ogg_packet header_code;
		vorbis_analysis_headerout(&vd, &vc, &header, &header_comm, &header_code);
		ogg_stream_packetin(&os, &header);
		while(ogg_stream_flush(&os, &og))
		{
			WritePage();
		}
		ogg_stream_packetin(&os, &header_comm);
		ogg_stream_packetin(&os, &header_code);
		while(ogg_stream_flush(&os, &og))
		{
			WritePage();
		}
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
			vorbis_analysis_wrote(&vd, 0);
			while(vorbis_analysis_blockout(&vd, &vb) == 1)
			{
				vorbis_analysis(&vb, NULL);
				vorbis_bitrate_addblock(&vb);
				while(vorbis_bitrate_flushpacket(&vd, &op))
				{
					ogg_stream_packetin(&os, &op);
					while(ogg_stream_flush(&os, &og))
					{
						WritePage();
						if(ogg_page_eos(&og))
						{
							break;
						}
					}
				}
			}
			ogg_stream_clear(&os);
			vorbis_block_clear(&vb);
			vorbis_dsp_clear(&vd);
			vorbis_comment_clear(&vc);
			vorbis_info_clear(&vi);
			started = false;
			inited = false;
		}
		ASSERT(!inited && !started);
	}
	void WritePage()
	{
		ASSERT(inited);
		buf.resize(og.header_len);
		std::memcpy(buf.data(), og.header, og.header_len);
		WriteBuffer();
		buf.resize(og.body_len);
		std::memcpy(buf.data(), og.body, og.body_len);
		WriteBuffer();
	}
	void AddCommentField(const std::string &field, const mpt::ustring &data)
	{
		if(!field.empty() && !data.empty())
		{
			vorbis_comment_add_tag(&vc, field.c_str(), mpt::ToCharset(mpt::CharsetUTF8, data).c_str());
		}
	}
public:
	VorbisStreamWriter(std::ostream &stream)
		: StreamWriterBase(stream)
	{
		inited = false;
		started = false;
		vorbis_channels = 0;
		vorbis_tags = true;
	}
	virtual ~VorbisStreamWriter()
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

		vorbis_channels = channels;
		vorbis_tags = settings.Tags;

		vorbis_info_init(&vi);
		vorbis_comment_init(&vc);

		if(settings.Mode == Encoder::ModeQuality)
		{
			vorbis_encode_init_vbr(&vi, vorbis_channels, samplerate, settings.Quality);
		} else
		{
			vorbis_encode_init(&vi, vorbis_channels, samplerate, -1, settings.Bitrate * 1000, -1);
		}

		vorbis_analysis_init(&vd, &vi);
		vorbis_block_init(&vd, &vb);
		ogg_stream_init(&os, mpt::random<uint32>(theApp.PRNG()));

		inited = true;

		ASSERT(inited && !started);
	}
	virtual void WriteMetatags(const FileTags &tags)
	{
		ASSERT(inited && !started);
		AddCommentField("ENCODER", tags.encoder);
		if(vorbis_tags)
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
		size_t countTotal = count;
		while(countTotal > 0)
		{
			int countChunk = mpt::saturate_cast<int>(countTotal);
			countTotal -= countChunk;
			float **buffer = vorbis_analysis_buffer(&vd, countChunk);
			for(int frame = 0; frame < countChunk; ++frame)
			{
				for(int channel = 0; channel < vorbis_channels; ++channel)
				{
					buffer[channel][frame] = interleaved[frame*vorbis_channels+channel];
				}
			}
			vorbis_analysis_wrote(&vd, countChunk);
			while(vorbis_analysis_blockout(&vd, &vb) == 1)
			{
				vorbis_analysis(&vb, NULL);
				vorbis_bitrate_addblock(&vb);
				while(vorbis_bitrate_flushpacket(&vd, &op))
				{
					ogg_stream_packetin(&os, &op);
					while(ogg_stream_pageout(&os, &og))
					{
						WritePage();
					}
				}
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

#endif // MPT_WITH_OGG && MPT_WITH_VORBIS && MPT_WITH_VORBISENC



VorbisEncoder::VorbisEncoder()
//----------------------------
{
	SetTraits(VorbisBuildTraits());
}


bool VorbisEncoder::IsAvailable() const
//-------------------------------------
{
#if defined(MPT_WITH_OGG) && defined(MPT_WITH_VORBIS) && defined(MPT_WITH_VORBISENC)
	return true;
#else
	return false;
#endif
}


VorbisEncoder::~VorbisEncoder()
//-----------------------------
{
	return;
}


IAudioStreamEncoder *VorbisEncoder::ConstructStreamEncoder(std::ostream &file) const
//----------------------------------------------------------------------------------
{
#if defined(MPT_WITH_OGG) && defined(MPT_WITH_VORBIS) && defined(MPT_WITH_VORBISENC)
	return new VorbisStreamWriter(file);
#else
	return nullptr;
#endif
}


mpt::ustring VorbisEncoder::DescribeQuality(float quality) const
//--------------------------------------------------------------
{
	static const int q_table[11] = { 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 500 }; // http://wiki.hydrogenaud.io/index.php?title=Recommended_Ogg_Vorbis
	int q = Clamp(Util::Round<int>(quality * 10.0f), 0, 10);
	return mpt::String::Print(MPT_USTRING("Q%1 (~%2 kbit)"), mpt::ufmt::f("%3.1f", quality * 10.0f), q_table[q]);
}



OPENMPT_NAMESPACE_END
