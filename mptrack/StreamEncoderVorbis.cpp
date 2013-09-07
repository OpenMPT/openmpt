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

#include "Mptrack.h"

#include <vorbis/vorbisenc.h>



struct VorbisDynBind
{

	HMODULE hOgg;
	HMODULE hVorbis;
	HMODULE hVorbisEnc;

	// ogg
	int      (*ogg_stream_init)(ogg_stream_state *os,int serialno);
	int      (*ogg_stream_packetin)(ogg_stream_state *os, ogg_packet *op);
	int      (*ogg_stream_flush)(ogg_stream_state *os, ogg_page *og);
	int      (*ogg_stream_pageout)(ogg_stream_state *os, ogg_page *og);
	int      (*ogg_page_eos)(const ogg_page *og);
	int      (*ogg_stream_clear)(ogg_stream_state *os);

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

	void Reset()
	{
		std::memset(this, 0, sizeof(*this));
	}
	VorbisDynBind()
	{
		Reset();
		struct dll_names_t {
			const char *ogg;
			const char *vorbis;
			const char *vorbisenc;
		};
		// start with trying all symbols from a single dll first
		static const dll_names_t dll_names[] = {
			{ "libvorbis.dll", "libvorbis.dll"  , "libvorbis.dll"     },
			{ "vorbis.dll"   , "vorbis.dll"     , "vorbis.dll"        },
			{ "libogg.dll"   , "libvorbis.dll"  , "libvorbis.dll"     }, // official xiph.org builds
			{ "ogg.dll"      , "vorbis.dll"     , "vorbis.dll"        },
			{ "libogg-0.dll" , "libvorbis-0.dll", "libvorbis-0.dll"   }, // mingw builds
			{ "libogg.dll"   , "libvorbis.dll"  , "libvorbisenc.dll"  },
			{ "ogg.dll"      , "vorbis.dll"     , "vorbisenc.dll"     },
			{ "libogg-0.dll" , "libvorbis-0.dll", "libvorbisenc-0.dll"}  // mingw builds
		};
		for(std::size_t i=0; i<CountOf(dll_names); ++i)
		{
			if(TryLoad(dll_names[i].ogg, dll_names[i].vorbis, dll_names[i].vorbisenc))
			{
				// success
				break;
			}
		}
	}
	bool TryLoad(std::string Ogg_fn, std::string Vorbis_fn, std::string VorbisEnc_fn)
	{
		#ifdef MODPLUG_TRACKER
			Ogg_fn = std::string(theApp.GetAppDirPath()) + Ogg_fn;
			Vorbis_fn = std::string(theApp.GetAppDirPath()) + Vorbis_fn;
			VorbisEnc_fn = std::string(theApp.GetAppDirPath()) + VorbisEnc_fn;
		#endif
		hOgg = LoadLibrary(Ogg_fn.c_str());
		if(!hOgg)
		{
			if(hOgg) { FreeLibrary(hOgg); hOgg = NULL; }
			if(hVorbis) { FreeLibrary(hVorbis); hVorbis = NULL; }
			if(hVorbisEnc) { FreeLibrary(hVorbisEnc); hVorbisEnc = NULL; }
			return false;
		}
		hVorbis = LoadLibrary(Vorbis_fn.c_str());
		if(!hVorbis)
		{
			if(hOgg) { FreeLibrary(hOgg); hOgg = NULL; }
			if(hVorbis) { FreeLibrary(hVorbis); hVorbis = NULL; }
			if(hVorbisEnc) { FreeLibrary(hVorbisEnc); hVorbisEnc = NULL; }
			return false;
		}
		hVorbisEnc = LoadLibrary(VorbisEnc_fn.c_str());
		if(!hVorbisEnc)
		{
			if(hOgg) { FreeLibrary(hOgg); hOgg = NULL; }
			if(hVorbis) { FreeLibrary(hVorbis); hVorbis = NULL; }
			if(hVorbisEnc) { FreeLibrary(hVorbisEnc); hVorbisEnc = NULL; }
			return false;
		}
		bool ok = true;
		#define VORBIS_BIND(l,f) do { \
			FARPROC pf = GetProcAddress( l , #f ); \
			if(!pf) \
			{ \
				ok = false; \
			} \
			*reinterpret_cast<void**>(& f ) = reinterpret_cast<void*>(pf); \
		} while(0)
		#define VORBIS_BIND_OPTIONAL(l,f) do { \
			FARPROC pf = GetProcAddress( l , #f ); \
			*reinterpret_cast<void**>(& f ) = reinterpret_cast<void*>(pf); \
		} while(0)
		VORBIS_BIND(hOgg,ogg_stream_init);
		VORBIS_BIND(hOgg,ogg_stream_packetin);
		VORBIS_BIND(hOgg,ogg_stream_flush);
		VORBIS_BIND(hOgg,ogg_stream_pageout);
		VORBIS_BIND(hOgg,ogg_page_eos);
		VORBIS_BIND(hOgg,ogg_stream_clear);
		VORBIS_BIND_OPTIONAL(hVorbis,vorbis_version_string);
		VORBIS_BIND(hVorbis,vorbis_info_init);
		VORBIS_BIND(hVorbis,vorbis_comment_init);
		VORBIS_BIND(hVorbis,vorbis_comment_add_tag);
		VORBIS_BIND(hVorbis,vorbis_analysis_init);
		VORBIS_BIND(hVorbis,vorbis_block_init);
		VORBIS_BIND(hVorbis,vorbis_analysis_headerout);
		VORBIS_BIND(hVorbis,vorbis_analysis_buffer);
		VORBIS_BIND(hVorbis,vorbis_analysis_wrote);
		VORBIS_BIND(hVorbis,vorbis_analysis_blockout);
		VORBIS_BIND(hVorbis,vorbis_analysis);
		VORBIS_BIND(hVorbis,vorbis_bitrate_addblock);
		VORBIS_BIND(hVorbis,vorbis_bitrate_flushpacket);
		VORBIS_BIND(hVorbis,vorbis_block_clear);
		VORBIS_BIND(hVorbis,vorbis_dsp_clear);
		VORBIS_BIND(hVorbis,vorbis_comment_clear);
		VORBIS_BIND(hVorbis,vorbis_info_clear);
		VORBIS_BIND(hVorbisEnc,vorbis_encode_init);
		VORBIS_BIND(hVorbisEnc,vorbis_encode_init_vbr);
		#undef VORBIS_BIND
		#undef VORBIS_BIND_OPTIONAL
		if(!ok)
		{
			if(hOgg) { FreeLibrary(hOgg); hOgg = NULL; }
			if(hVorbis) { FreeLibrary(hVorbis); hVorbis = NULL; }
			if(hVorbisEnc) { FreeLibrary(hVorbisEnc); hVorbisEnc = NULL; }
			Reset();
			return false;
		}
		return true;
	}
	operator bool () const { return hOgg && hVorbis && hVorbisEnc; }
	~VorbisDynBind()
	{
		if(hOgg) { FreeLibrary(hOgg); hOgg = NULL; }
		if(hVorbis) { FreeLibrary(hVorbis); hVorbis = NULL; }
		if(hVorbisEnc) { FreeLibrary(hVorbisEnc); hVorbisEnc = NULL; }
		Reset();
	}
	Encoder::Traits BuildTraits()
	{
		Encoder::Traits traits;
		if(!*this)
		{
			return traits;
		}
		traits.fileExtension = "ogg";
		traits.fileShortDescription = "Vorbis";
		traits.fileDescription = "Ogg Vorbis";
		traits.name = "Ogg Vorbis";
		traits.description += "Version: ";
		traits.description += (vorbis_version_string&&vorbis_version_string()?vorbis_version_string():"unknown");
		traits.description += "\n";
		traits.canTags = true;
		traits.maxChannels = 4;
		traits.samplerates = std::vector<uint32>(vorbis_samplerates, vorbis_samplerates + CountOf(vorbis_samplerates));
		traits.modes = Encoder::ModeVBR | Encoder::ModeQuality;
		traits.bitrates = std::vector<int>(vorbis_bitrates, vorbis_bitrates + CountOf(vorbis_bitrates));
		traits.defaultMode = Encoder::ModeQuality;
		traits.defaultBitrate = 160;
		traits.defaultQuality = 0.5;
		return traits;
	}
};

class VorbisStreamWriter : public StreamWriterBase
{
private:
	VorbisDynBind &vorbis;
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
		vorbis.ogg_stream_packetin(&os, &header);
		vorbis.ogg_stream_packetin(&os, &header_comm);
		vorbis.ogg_stream_packetin(&os, &header_code);
		while(true)
		{
			int gotPage = vorbis.ogg_stream_flush(&os, &og);
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
					vorbis.ogg_stream_packetin(&os, &op);
					while(true)
					{
						int gotPage = vorbis.ogg_stream_flush(&os, &og);
						if(!gotPage) break;
						WritePage();
						if(vorbis.ogg_page_eos(&og))
							break;
					}
				}
			}
			vorbis.ogg_stream_clear(&os);
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
	void AddCommentField(const std::string &field, const std::wstring &data)
	{
		if(!field.empty() && !data.empty())
		{
			vorbis.vorbis_comment_add_tag(&vc, field.c_str(), mpt::String::Encode(data, mpt::CharsetUTF8).c_str());
		}
	}
public:
	VorbisStreamWriter(VorbisDynBind &vorbis_, std::ostream &stream)
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
	virtual void SetFormat(int samplerate, int channels, const Encoder::Settings &settings)
	{

		FinishStream();

		ASSERT(!inited && !started);

		vorbis_channels = channels;
		vorbis_tags = settings.Tags;

		vorbis.vorbis_info_init(&vi);
		vorbis.vorbis_comment_init(&vc);

		if(settings.Mode == Encoder::ModeQuality)
		{
			vorbis.vorbis_encode_init_vbr(&vi, vorbis_channels, samplerate, settings.Quality);
		} else {
			vorbis.vorbis_encode_init(&vi, vorbis_channels, samplerate, -1, settings.Bitrate * 1000, -1);
		}

		vorbis.vorbis_analysis_init(&vd, &vi);
		vorbis.vorbis_block_init(&vd, &vb);
		vorbis.ogg_stream_init(&os, std::rand());

		inited = true;

		ASSERT(inited && !started);
	}
	virtual void WriteMetatags(const FileTags &tags)
	{
		ASSERT(inited && !started);
		AddCommentField("ENCODER", tags.encoder);
		if(vorbis_tags)
		{
			AddCommentField("SOURCEMEDIA",L"tracked music file");
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
		float **buffer = vorbis.vorbis_analysis_buffer(&vd, count);
		for(size_t frame = 0; frame < count; ++frame)
		{
			for(int channel = 0; channel < vorbis_channels; ++channel)
			{
				buffer[channel][frame] = interleaved[frame*vorbis_channels+channel];
			}
		}
		vorbis.vorbis_analysis_wrote(&vd, count);
    while(vorbis.vorbis_analysis_blockout(&vd, &vb) == 1)
		{
      vorbis.vorbis_analysis(&vb, NULL);
      vorbis.vorbis_bitrate_addblock(&vb);
      while(vorbis.vorbis_bitrate_flushpacket(&vd, &op))
			{
        vorbis.ogg_stream_packetin(&os, &op);
				while(true)
				{
					int gotPage = vorbis.ogg_stream_pageout(&os, &og);
					if(!gotPage) break;
					WritePage();
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



VorbisEncoder::VorbisEncoder()
//----------------------------
	: m_Vorbis(nullptr)
{
	m_Vorbis = new VorbisDynBind();
	if(*m_Vorbis)
	{
		SetTraits(m_Vorbis->BuildTraits());
	}
}


bool VorbisEncoder::IsAvailable() const
//-------------------------------------
{
	return m_Vorbis && *m_Vorbis;
}


VorbisEncoder::~VorbisEncoder()
//-----------------------------
{
	if(m_Vorbis)
	{
		delete m_Vorbis;
		m_Vorbis = nullptr;
	}
}


IAudioStreamEncoder *VorbisEncoder::ConstructStreamEncoder(std::ostream &file) const
//----------------------------------------------------------------------------------
{
	if(!IsAvailable())
	{
		return nullptr;
	}
	return new VorbisStreamWriter(*m_Vorbis, file);
}


std::string VorbisEncoder::DescribeQuality(float quality) const
//-------------------------------------------------------------
{
	return mpt::String::Format("Q%3.1f", quality * 10.0f);
}

