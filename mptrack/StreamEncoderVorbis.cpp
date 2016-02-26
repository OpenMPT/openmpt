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
#include <vorbis/vorbisenc.h>



OPENMPT_NAMESPACE_BEGIN



class ComponentVorbis
	: public ComponentLibrary
{

	MPT_DECLARE_COMPONENT_MEMBERS

public:

	// vorbis
	const char *(*vorbis_version_string)(void);
	void     (*vorbis_info_init)(vorbis_info *vi);
	void     (*vorbis_comment_init)(vorbis_comment *vc);
	void     (*vorbis_comment_add_tag)(vorbis_comment *vc, const char *tag, const char *contents);
	int      (*vorbis_analysis_init)(vorbis_dsp_state *v,vorbis_info *vi);
	int      (*vorbis_block_init)(vorbis_dsp_state *v, vorbis_block *vb);
	int      (*vorbis_analysis_headerout)(vorbis_dsp_state *v, vorbis_comment *vc, ogg_packet *op, ogg_packet *op_comm, ogg_packet *op_code);
	float  **(*vorbis_analysis_buffer)(vorbis_dsp_state *v,int vals);
	int      (*vorbis_analysis_wrote)(vorbis_dsp_state *v,int vals);
	int      (*vorbis_analysis_blockout)(vorbis_dsp_state *v,vorbis_block *vb);
	int      (*vorbis_analysis)(vorbis_block *vb,ogg_packet *op);
	int      (*vorbis_bitrate_addblock)(vorbis_block *vb);
	int      (*vorbis_bitrate_flushpacket)(vorbis_dsp_state *vd, ogg_packet *op);
	int      (*vorbis_block_clear)(vorbis_block *vb);
	void     (*vorbis_dsp_clear)(vorbis_dsp_state *v);
	void     (*vorbis_comment_clear)(vorbis_comment *vc);
	void     (*vorbis_info_clear)(vorbis_info *vi);

	// vorbisenc
	int (*vorbis_encode_init)(vorbis_info *vi, long channels, long rate, long max_bitrate, long nominal_bitrate, long min_bitrate);
	int (*vorbis_encode_init_vbr)(vorbis_info *vi, long channels, long rate, float base_quality);

private:

	void Reset()
	{
		ClearLibraries();
	}

public:

	ComponentVorbis()
		: ComponentLibrary(ComponentTypeForeign)
	{
		return;
	}

protected:

	bool DoInitialize()
	{
		Reset();
		struct dll_names_t {
			const char *vorbis;
			const char *vorbisenc;
		};
		// start with trying all symbols from a single dll first
		static const dll_names_t dll_names[] = {
			{ "libvorbis"  , "libvorbis"     }, // official xiph.org builds
			{ "vorbis"     , "vorbis"        },
			{ "libvorbis-0", "libvorbis-0"   }, // mingw builds
			{ "libvorbis"  , "libvorbisenc"  },
			{ "vorbis"     , "vorbisenc"     },
			{ "libvorbis-0", "libvorbisenc-0"}, // mingw builds
			{ "libvorbis-0", "libvorbisenc-2"}  // mingw 64-bit builds
		};
		bool ok = false;
		for(std::size_t i=0; i<CountOf(dll_names); ++i)
		{
			if(TryLoad(mpt::PathString::FromUTF8(dll_names[i].vorbis), mpt::PathString::FromUTF8(dll_names[i].vorbisenc)))
			{
				ok = true;
				break;
			}
		}
#ifdef MPT_WITH_OGG
		return ok;
#else
		return false;
#endif
	}
	bool TryLoad(const mpt::PathString &Vorbis_fn, const mpt::PathString &VorbisEnc_fn)
	{
		Reset();
		ClearBindFailed();
		if(!AddLibrary("vorbis", mpt::LibraryPath::AppFullName(Vorbis_fn)))
		{
			Reset();
			return false;
		}
		if(!AddLibrary("vorbisenc", mpt::LibraryPath::AppFullName(VorbisEnc_fn)))
		{
			Reset();
			return false;
		}
		MPT_COMPONENT_BIND_OPTIONAL("vorbis",vorbis_version_string);
		MPT_COMPONENT_BIND("vorbis",vorbis_info_init);
		MPT_COMPONENT_BIND("vorbis",vorbis_comment_init);
		MPT_COMPONENT_BIND("vorbis",vorbis_comment_add_tag);
		MPT_COMPONENT_BIND("vorbis",vorbis_analysis_init);
		MPT_COMPONENT_BIND("vorbis",vorbis_block_init);
		MPT_COMPONENT_BIND("vorbis",vorbis_analysis_headerout);
		MPT_COMPONENT_BIND("vorbis",vorbis_analysis_buffer);
		MPT_COMPONENT_BIND("vorbis",vorbis_analysis_wrote);
		MPT_COMPONENT_BIND("vorbis",vorbis_analysis_blockout);
		MPT_COMPONENT_BIND("vorbis",vorbis_analysis);
		MPT_COMPONENT_BIND("vorbis",vorbis_bitrate_addblock);
		MPT_COMPONENT_BIND("vorbis",vorbis_bitrate_flushpacket);
		MPT_COMPONENT_BIND("vorbis",vorbis_block_clear);
		MPT_COMPONENT_BIND("vorbis",vorbis_dsp_clear);
		MPT_COMPONENT_BIND("vorbis",vorbis_comment_clear);
		MPT_COMPONENT_BIND("vorbis",vorbis_info_clear);
		MPT_COMPONENT_BIND("vorbisenc",vorbis_encode_init);
		MPT_COMPONENT_BIND("vorbisenc",vorbis_encode_init_vbr);
		if(HasBindFailed())
		{
			Reset();
			return false;
		}
		return true;
	}

public:

	Encoder::Traits BuildTraits()
	{
		Encoder::Traits traits;
		if(!IsAvailable())
		{
			return traits;
		}
		traits.fileExtension = MPT_PATHSTRING("ogg");
		traits.fileShortDescription = MPT_USTRING("Vorbis");
		traits.fileDescription = MPT_USTRING("Ogg Vorbis");
		traits.encoderSettingsName = MPT_USTRING("Vorbis");
		traits.encoderName = MPT_USTRING("libVorbis");
		traits.description += MPT_USTRING("Version: ");
		traits.description += mpt::ToUnicode(mpt::CharsetASCII, vorbis_version_string&&vorbis_version_string()?vorbis_version_string():"unknown");
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
		return traits;
	}
};
MPT_REGISTERED_COMPONENT(ComponentVorbis, "Vorbis")

#ifdef MPT_WITH_OGG

class VorbisStreamWriter : public StreamWriterBase
{
private:
	ComponentVorbis &vorbis;
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
		vorbis.vorbis_analysis_headerout(&vd, &vc, &header, &header_comm, &header_code);
		ogg_stream_packetin(&os, &header);
		ogg_stream_packetin(&os, &header_comm);
		ogg_stream_packetin(&os, &header_code);
		while(true)
		{
			int gotPage = ogg_stream_flush(&os, &og);
			if(!gotPage) break;
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
			vorbis.vorbis_analysis_wrote(&vd, 0);
			while(vorbis.vorbis_analysis_blockout(&vd, &vb) == 1)
			{
				vorbis.vorbis_analysis(&vb, NULL);
				vorbis.vorbis_bitrate_addblock(&vb);
				while(vorbis.vorbis_bitrate_flushpacket(&vd, &op))
				{
					ogg_stream_packetin(&os, &op);
					while(true)
					{
						int gotPage = ogg_stream_flush(&os, &og);
						if(!gotPage) break;
						WritePage();
						if(ogg_page_eos(&og))
							break;
					}
				}
			}
			ogg_stream_clear(&os);
			vorbis.vorbis_block_clear(&vb);
			vorbis.vorbis_dsp_clear(&vd);
			vorbis.vorbis_comment_clear(&vc);
			vorbis.vorbis_info_clear(&vi);
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
			vorbis.vorbis_comment_add_tag(&vc, field.c_str(), mpt::ToCharset(mpt::CharsetUTF8, data).c_str());
		}
	}
public:
	VorbisStreamWriter(ComponentVorbis &vorbis_, std::ostream &stream)
		: StreamWriterBase(stream)
		, vorbis(vorbis_)
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

		vorbis.vorbis_info_init(&vi);
		vorbis.vorbis_comment_init(&vc);

		if(settings.Mode == Encoder::ModeQuality)
		{
			vorbis.vorbis_encode_init_vbr(&vi, vorbis_channels, samplerate, settings.Quality);
		} else
		{
			vorbis.vorbis_encode_init(&vi, vorbis_channels, samplerate, -1, settings.Bitrate * 1000, -1);
		}

		vorbis.vorbis_analysis_init(&vd, &vi);
		vorbis.vorbis_block_init(&vd, &vb);
		ogg_stream_init(&os, std::rand());

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
			float **buffer = vorbis.vorbis_analysis_buffer(&vd, countChunk);
			for(int frame = 0; frame < countChunk; ++frame)
			{
				for(int channel = 0; channel < vorbis_channels; ++channel)
				{
					buffer[channel][frame] = interleaved[frame*vorbis_channels+channel];
				}
			}
			vorbis.vorbis_analysis_wrote(&vd, countChunk);
			while(vorbis.vorbis_analysis_blockout(&vd, &vb) == 1)
			{
				vorbis.vorbis_analysis(&vb, NULL);
				vorbis.vorbis_bitrate_addblock(&vb);
				while(vorbis.vorbis_bitrate_flushpacket(&vd, &op))
				{
					ogg_stream_packetin(&os, &op);
					while(true)
					{
						int gotPage = ogg_stream_pageout(&os, &og);
						if(!gotPage) break;
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

#endif // MPT_WITH_OGG



VorbisEncoder::VorbisEncoder()
//----------------------------
{
	if(IsComponentAvailable(m_Vorbis))
	{
		SetTraits(m_Vorbis->BuildTraits());
	}
}


bool VorbisEncoder::IsAvailable() const
//-------------------------------------
{
	return IsComponentAvailable(m_Vorbis);
}


VorbisEncoder::~VorbisEncoder()
//-----------------------------
{
	return;
}


IAudioStreamEncoder *VorbisEncoder::ConstructStreamEncoder(std::ostream &file) const
//----------------------------------------------------------------------------------
{
	if(!IsAvailable())
	{
		return nullptr;
	}
#ifdef MPT_WITH_OGG
	return new VorbisStreamWriter(*m_Vorbis, file);
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
