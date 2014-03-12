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
#include "StreamEncoderMP3.h"

#include "Mptrack.h"

#include "../soundlib/Sndfile.h"

#include "../soundlib/SampleFormatConverters.h"
#include "../common/misc_util.h"
#include "../common/StringFixer.h"

#include <deque>
#include <iomanip>
#include <locale>
#include <sstream>

#include <cstdlib>

// For ACM debugging purposes
#define ACMLOG(x, ...)
//#define ACMLOG Log

#ifdef MPT_MP3ENCODER_LAME
//#define MPT_USE_LAME_H
#ifdef MPT_USE_LAME_H
#include <lame/lame.h>
#else
// from lame.h:
#ifndef CDECL
#define CDECL __cdecl
#endif
typedef enum vbr_mode_e {
	vbr_off=0,
	vbr_mt,
	vbr_rh,
	vbr_abr,
	vbr_mtrh,
	vbr_max_indicator,
	vbr_default=vbr_mtrh
} vbr_mode;
#endif
#endif // MPT_MP3ENCODER_LAME

#ifdef MPT_MP3ENCODER_BLADE
#include <bladeenc/bladedll.h>
#endif // MPT_MP3ENCODER_BLADE

#ifdef MPT_MP3ENCODER_ACM
#include <MSAcm.h>
#endif // MPT_MP3ENCODER_ACM



#define MPT_FORCE_ID3V1_TAGS_IN_CBR_MODE



#if defined(MPT_MP3ENCODER_BLADE) || defined(MPT_MP3ENCODER_ACM)

///////////////////////////////////////////////////////////////////////////////////////////////////
// ID3v2.4 Tags

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(push, 1)
#endif

struct PACKED ID3v2Header
{
	uint8 signature[3];
	uint8 version[2];
	uint8 flags;
	uint32 size;
	// Total: 10 bytes
};

STATIC_ASSERT(sizeof(ID3v2Header) == 10);

struct PACKED ID3v2Frame
{
	uint32 frameid;
	uint32 size;
	uint16 flags;
	// Total: 10 bytes
};

STATIC_ASSERT(sizeof(ID3v2Frame) == 10);

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(pop)
#endif

// we will add some padding bytes to our id3v2 tag (extending tags will be easier this way)
#define ID3v2_PADDING 512

// charset... choose text ending accordingly.
// $00 = ISO-8859-1. Terminated with $00.
// $01 = UTF-16 with BOM. Terminated with $00 00.
// $02 = UTF-16BE without BOM. Terminated with $00 00.
// $03 = UTF-8. Terminated with $00.
#define ID3v2_CHARSET '\3'
#define ID3v2_TEXTENDING '\0'

//===============
class ID3V2Tagger
//===============
{
public:
	// Write Tags
	void WriteID3v2Tags(std::ostream &s, const FileTags &tags);

	ID3V2Tagger();

private:
	// Convert Integer to Synchsafe Integer (see ID3v2.4 specs)
	uint32 intToSynchsafe(uint32 in);
	// Write a frame
	void WriteID3v2Frame(char cFrameID[4], std::string sFramecontent, std::ostream &s);
	// Size of our tag
	uint32 totalID3v2Size;
};

///////////////////////////////////////////////////
// CFileTagging - helper class for writing tags

ID3V2Tagger::ID3V2Tagger()
//------------------------
	: totalID3v2Size(0)
{
	return;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// ID3v2.4 Tags

// Convert Integer to Synchsafe Integer (see ID3v2.4 specs)
// Basically, it's a BigEndian integer, but the MSB of all bytes is 0.
// Thus, a 32-bit integer turns into a 28-bit integer.
uint32 ID3V2Tagger::intToSynchsafe(uint32 in)
//-------------------------------------------
{
	uint32 out = 0, steps = 0;
	do
	{
		out |= (in & 0x7F) << steps;
		steps += 8;
	} while(in >>= 7);
	SwapBytesBE(out);
	return out;
}

// Write Tags
void ID3V2Tagger::WriteID3v2Tags(std::ostream &s, const FileTags &tags)
//---------------------------------------------------------------------
{
	if(!s) return;
	
	ID3v2Header tHeader;
	std::streampos fOffset = s.tellp();

	totalID3v2Size = 0;

	// Correct header will be written later (tag size missing)
	tHeader.signature[0] = 'I';
	tHeader.signature[1] = 'D';
	tHeader.signature[2] = '3';
	tHeader.version[0] = 0x04; // Version 2.4.0
	tHeader.version[1] = 0x00; // Dito
	tHeader.flags = 0x00; // No flags
	s.write(reinterpret_cast<const char*>(&tHeader), sizeof(tHeader));

	// Write TIT2 (Title), TCOM / TPE1 (Composer), TALB (Album), TCON (Genre), TYER / TDRC (Date), WXXX (URL), TENC (Encoder), COMM (Comment)
	WriteID3v2Frame("TIT2", mpt::To(mpt::CharsetUTF8, tags.title), s);
	WriteID3v2Frame("TPE1", mpt::To(mpt::CharsetUTF8, tags.artist), s);
	WriteID3v2Frame("TCOM", mpt::To(mpt::CharsetUTF8, tags.artist), s);
	WriteID3v2Frame("TALB", mpt::To(mpt::CharsetUTF8, tags.album), s);
	WriteID3v2Frame("TCON", mpt::To(mpt::CharsetUTF8, tags.genre), s);
	//WriteID3v2Frame("TYER", mpt::To(mpt::CharsetUTF8, tags.year), s);		// Deprecated
	WriteID3v2Frame("TDRC", mpt::To(mpt::CharsetUTF8, tags.year), s);
	WriteID3v2Frame("TBPM", mpt::To(mpt::CharsetUTF8, tags.bpm), s);
	WriteID3v2Frame("WXXX", mpt::To(mpt::CharsetUTF8, tags.url), s);
	WriteID3v2Frame("TENC", mpt::To(mpt::CharsetUTF8, tags.encoder), s);
	WriteID3v2Frame("COMM", mpt::To(mpt::CharsetUTF8, tags.comments), s);

	// Write Padding
	for(size_t i = 0; i < ID3v2_PADDING; i++)
	{
		char c = 0;
		s.write(&c, 1);
	}
	totalID3v2Size += ID3v2_PADDING;

	// Write correct header (update tag size)
	tHeader.size = intToSynchsafe(totalID3v2Size);
	s.seekp(fOffset);
	s.write(reinterpret_cast<const char*>(&tHeader), sizeof(tHeader));
	s.seekp(s.tellp() + std::streampos(totalID3v2Size));

}

// Write a ID3v2 frame
void ID3V2Tagger::WriteID3v2Frame(char cFrameID[4], std::string sFramecontent, std::ostream &s)
//---------------------------------------------------------------------------------------------
{
	if(!cFrameID[0] || sFramecontent.empty() || !s) return;

	if(!memcmp(cFrameID, "COMM", 4))
	{
		// English language for comments - no description following (hence the text ending nullchar(s))
		// For language IDs, see http://en.wikipedia.org/wiki/ISO-639-2
		sFramecontent = "eng" + (ID3v2_TEXTENDING + sFramecontent);
	}
	if(!memcmp(cFrameID, "WXXX", 4))
	{
		// User-defined URL field (we have no description for the URL, so we leave it out)
		sFramecontent = ID3v2_TEXTENDING + sFramecontent;
	}
	sFramecontent = ID3v2_CHARSET + sFramecontent;
	sFramecontent += ID3v2_TEXTENDING;

	ID3v2Frame tFrame;

	memcpy(&tFrame.frameid, cFrameID, 4); // ID
	tFrame.size = intToSynchsafe(sFramecontent.size()); // Text size
	tFrame.flags = 0x0000; // No flags
	s.write(reinterpret_cast<const char*>(&tFrame), sizeof(tFrame));
	s.write(sFramecontent.c_str(), sFramecontent.size());

	totalID3v2Size += (sizeof(tFrame) + sFramecontent.size());
}


#endif // MPT_MP3ENCODER_BLADE || MPT_MP3ENCODER_ACM




#ifdef MPT_MP3ENCODER_LAME

struct lame_global_struct;
typedef struct lame_global_struct lame_global_flags;
typedef lame_global_flags *lame_t;

struct LameDynBind
{

	mpt::Library hLame;

	const char* (CDECL * get_lame_version)();
	const char* (CDECL * get_lame_short_version)();
	const char* (CDECL * get_lame_very_short_version)();
	const char* (CDECL * get_psy_version)();
	const char* (CDECL * get_lame_url)();

	lame_global_flags * (CDECL * lame_init)(void);

	int  (CDECL * lame_set_in_samplerate)(lame_global_flags *, int);
	int  (CDECL * lame_set_num_channels)(lame_global_flags *, int);
	int  (CDECL * lame_get_num_channels)(const lame_global_flags *);
	int  (CDECL * lame_get_quality)(const lame_global_flags *);
	int  (CDECL * lame_set_quality)(lame_global_flags *, int);
	int  (CDECL * lame_set_out_samplerate)(lame_global_flags *, int);
	int  (CDECL * lame_set_brate)(lame_global_flags *, int);
	int  (CDECL * lame_set_VBR_quality)(lame_global_flags *, float);
	int  (CDECL * lame_set_VBR)(lame_global_flags *, vbr_mode);
	int  (CDECL * lame_set_bWriteVbrTag)(lame_global_flags *, int);
	int  (CDECL * lame_set_strict_ISO)(lame_global_flags *, int);
	int  (CDECL * lame_set_disable_reservoir)(lame_global_flags *, int);

	void (CDECL * id3tag_genre_list)(void (*handler)(int, const char *, void *), void* cookie);
	void (CDECL * id3tag_init)     (lame_t gfp);
	void (CDECL * id3tag_v1_only)  (lame_t gfp);
	void (CDECL * id3tag_add_v2)   (lame_t gfp);
	void (CDECL * id3tag_v2_only)  (lame_t gfp);
	void (CDECL * lame_set_write_id3tag_automatic)(lame_global_flags * gfp, int);

	void (CDECL * id3tag_set_title)(lame_t gfp, const char* title);
	void (CDECL * id3tag_set_artist)(lame_t gfp, const char* artist);
	void (CDECL * id3tag_set_album)(lame_t gfp, const char* album);
	void (CDECL * id3tag_set_year)(lame_t gfp, const char* year);
	void (CDECL * id3tag_set_comment)(lame_t gfp, const char* comment);
	int  (CDECL * id3tag_set_track)(lame_t gfp, const char* track);
	int  (CDECL * id3tag_set_genre)(lame_t gfp, const char* genre);

	int  (CDECL * lame_init_params)(lame_global_flags *);

	int  (CDECL * lame_encode_buffer_ieee_float)(
	                                             lame_t          gfp,
	                                             const float     pcm_l [],
	                                             const float     pcm_r [],
	                                             const int       nsamples,
	                                             unsigned char * mp3buf,
	                                             const int       mp3buf_size);
	int  (CDECL * lame_encode_buffer_interleaved_ieee_float)(
	                                                         lame_t          gfp,
	                                                         const float     pcm[],
	                                                         const int       nsamples,
	                                                         unsigned char * mp3buf,
	                                                         const int       mp3buf_size);
	int  (CDECL * lame_encode_flush)(lame_global_flags * gfp, unsigned char* mp3buf, int size);
	size_t(CDECL* lame_get_lametag_frame)(const lame_global_flags *, unsigned char* buffer, size_t size);
	size_t(CDECL* lame_get_id3v2_tag)(lame_t gfp, unsigned char* buffer, size_t size);

	int  (CDECL * lame_close) (lame_global_flags *);

	void Reset()
	{
		return;
	}
	LameDynBind()
	{
		Reset();
		if(!hLame.IsValid()) TryLoad(MPT_PATHSTRING("libmp3lame"), true);
		if(!hLame.IsValid()) TryLoad(MPT_PATHSTRING("liblame"), true);
		if(!hLame.IsValid()) TryLoad(MPT_PATHSTRING("mp3lame"), true);
		if(!hLame.IsValid()) TryLoad(MPT_PATHSTRING("lame"), true);
		if(!hLame.IsValid()) TryLoad(MPT_PATHSTRING("lame_enc"), false);
	}
	void TryLoad(const mpt::PathString &filename, bool warn)
	{
		hLame = mpt::Library(mpt::LibraryPath::AppFullName(filename));
		if(!hLame.IsValid())
		{
			return;
		}
		bool ok = true;
		#define LAME_BIND(f) do { \
			if(!hLame.Bind( f , #f )) \
			{ \
				ok = false; \
				if(warn) \
				{ \
					Reporting::Error(mpt::String::Format("Your '%s' is missing '%s'.\n\nPlease copy a newer 'libmp3lame.dll' into OpenMPT's root directory.", filename, #f ).c_str(), "OpenMPT - MP3 Export"); \
				} \
			} \
		} while(0)
		LAME_BIND(get_lame_version);
		LAME_BIND(get_lame_short_version);
		LAME_BIND(get_lame_very_short_version);
		LAME_BIND(get_psy_version);
		LAME_BIND(get_lame_url);
		LAME_BIND(lame_init);
		LAME_BIND(lame_set_in_samplerate);
		LAME_BIND(lame_set_num_channels);
		LAME_BIND(lame_get_num_channels);
		LAME_BIND(lame_get_quality);
		LAME_BIND(lame_set_quality);
		LAME_BIND(lame_set_out_samplerate);
		LAME_BIND(lame_set_brate);
		LAME_BIND(lame_set_VBR_quality);
		LAME_BIND(lame_set_VBR);
		LAME_BIND(lame_set_bWriteVbrTag);
		LAME_BIND(lame_set_strict_ISO);
		LAME_BIND(lame_set_disable_reservoir);
		LAME_BIND(id3tag_genre_list);
		LAME_BIND(id3tag_init);
		LAME_BIND(id3tag_v1_only);
		LAME_BIND(id3tag_add_v2);
		LAME_BIND(id3tag_v2_only);
		LAME_BIND(lame_set_write_id3tag_automatic);
		LAME_BIND(id3tag_set_title);
		LAME_BIND(id3tag_set_artist);
		LAME_BIND(id3tag_set_album);
		LAME_BIND(id3tag_set_year);
		LAME_BIND(id3tag_set_comment);
		LAME_BIND(id3tag_set_track);
		LAME_BIND(id3tag_set_genre);
		LAME_BIND(lame_init_params);
		LAME_BIND(lame_encode_buffer_ieee_float);
		LAME_BIND(lame_encode_buffer_interleaved_ieee_float);
		LAME_BIND(lame_encode_flush);
		LAME_BIND(lame_get_lametag_frame);
		LAME_BIND(lame_get_id3v2_tag);
		LAME_BIND(lame_close);
		#undef LAME_BIND
		if(!ok)
		{
			hLame.Unload();
			Reset();
			return;
		}
	}
	operator bool () const { return hLame.IsValid() ? true : false; }
	~LameDynBind()
	{
		hLame.Unload();
		Reset();
	}
	static void GenreEnumCallback(int num, const char *name, void *cookie)
	{
		MPT_UNREFERENCED_PARAMETER(num);
		Encoder::Traits &traits = *reinterpret_cast<Encoder::Traits*>(cookie);
		if(name)
		{
			traits.genres.push_back(name);
		}
	}
	Encoder::Traits BuildTraits()
	{
		Encoder::Traits traits;
		if(!*this)
		{
			return traits;
		}
		traits.fileExtension = "mp3";
		traits.fileShortDescription = "MP3 (Lame)";
		traits.fileDescription = "MPEG-1/2 Layer 3";
		traits.encoderSettingsName = "MP3Lame";
		traits.encoderName = "libMP3Lame";
		traits.description += "Version: ";
		traits.description += (get_lame_version()?get_lame_version():"");
		traits.description += "\n";
		traits.description += "Psycho acoustic model version: ";
		traits.description += (get_psy_version()?get_psy_version():"");
		traits.description += "\n";
		traits.description += "URL: ";
		traits.description += (get_lame_url()?get_lame_url():"");
		traits.description += "\n";
		traits.canTags = true;
		traits.genres.clear();
		id3tag_genre_list(&GenreEnumCallback, &traits);
#ifdef MPT_FORCE_ID3V1_TAGS_IN_CBR_MODE
		traits.modesWithFixedGenres = Encoder::ModeCBR;
#endif
		traits.maxChannels = 2;
		traits.samplerates = std::vector<uint32>(layer3_samplerates, layer3_samplerates + CountOf(layer3_samplerates));
		traits.modes = Encoder::ModeCBR | Encoder::ModeQuality;
		traits.bitrates = std::vector<int>(layer3_bitrates, layer3_bitrates + CountOf(layer3_bitrates));
		traits.defaultSamplerate = 44100;
		traits.defaultChannels = 2;
		traits.defaultMode = Encoder::ModeQuality;
		traits.defaultBitrate = 256;
		traits.defaultQuality = 0.8f;
		return traits;
	}
};

class MP3LameStreamWriter : public StreamWriterBase
{
private:
	LameDynBind &lame;
	Encoder::Mode Mode;
	bool gfp_inited;
	lame_t gfp;
public:
	MP3LameStreamWriter(LameDynBind &lame_, std::ostream &stream)
		: StreamWriterBase(stream)
		, lame(lame_)
	{
		Mode = Encoder::ModeInvalid;
		gfp_inited = false;
		gfp = lame_t();
	}
	virtual ~MP3LameStreamWriter()
	{
		Finalize();
	}
	virtual void SetFormat(const Encoder::Settings &settings)
	{
		if(!gfp)
		{
			gfp = lame.lame_init();
		}

		uint32 samplerate = settings.Samplerate;
		uint16 channels = settings.Channels;

		lame.lame_set_in_samplerate(gfp, samplerate);
		lame.lame_set_num_channels(gfp, channels);

		int lameQuality = StreamEncoderSettings::Instance().MP3LameQuality;
		lame.lame_set_quality(gfp, lameQuality);

		if(settings.Mode == Encoder::ModeCBR)
		{

			if(settings.Bitrate >= 32)
			{
				// For maximum compatibility,
				// force samplerate to a samplerate supported by MPEG1 streams.
				if(samplerate <= 32000)
				{
					samplerate = 32000;
				} else if(samplerate >= 48000)
				{
					samplerate = 48000;
				} else
				{
					samplerate = 44100;
				}
				lame.lame_set_out_samplerate(gfp, samplerate);
			} else
			{
				// A very low bitrate was chosen,
				// force samplerate to lowest possible for MPEG2.
				// Disable unofficial MPEG2.5 however.
				lame.lame_set_out_samplerate(gfp, 16000);
			}

			lame.lame_set_brate(gfp, settings.Bitrate);
			lame.lame_set_VBR(gfp, vbr_off);

			lame.lame_set_bWriteVbrTag(gfp, 0);
			lame.lame_set_strict_ISO(gfp, 1);
			lame.lame_set_disable_reservoir(gfp, 1);

		} else
		{

			float lame_quality = 10.0f - (settings.Quality * 10.0f);
			Limit(lame_quality, 0.0f, 9.999f);
			lame.lame_set_VBR_quality(gfp, lame_quality);
			lame.lame_set_VBR(gfp, vbr_default);

			lame.lame_set_bWriteVbrTag(gfp, 1);

		}

		if(settings.Tags)
		{
			lame.id3tag_init(gfp);
			if(settings.Mode == Encoder::ModeCBR)
			{
				#ifdef MPT_FORCE_ID3V1_TAGS_IN_CBR_MODE
					lame.id3tag_v1_only(gfp);
				#else
					lame.id3tag_add_v2(gfp);
					lame.id3tag_v2_only(gfp);
				#endif
			} else
			{
				lame.id3tag_add_v2(gfp);
				lame.id3tag_v2_only(gfp);
			}

		} else
		{
			lame.lame_set_write_id3tag_automatic(gfp, 0);
		}

		Mode = settings.Mode;
	}
	virtual void WriteMetatags(const FileTags &tags)
	{
		// Lame API expects Latin1, which is sad, but we cannot change that.
		if(!tags.title.empty())    lame.id3tag_set_title(   gfp, mpt::To(mpt::CharsetISO8859_1, tags.title   ).c_str());
		if(!tags.artist.empty())   lame.id3tag_set_artist(  gfp, mpt::To(mpt::CharsetISO8859_1, tags.artist  ).c_str());
		if(!tags.album.empty())    lame.id3tag_set_album(   gfp, mpt::To(mpt::CharsetISO8859_1, tags.album   ).c_str());
		if(!tags.year.empty())     lame.id3tag_set_year(    gfp, mpt::To(mpt::CharsetISO8859_1, tags.year    ).c_str());
		if(!tags.comments.empty()) lame.id3tag_set_comment( gfp, mpt::To(mpt::CharsetISO8859_1, tags.comments).c_str());
		if(!tags.trackno.empty())  lame.id3tag_set_track(   gfp, mpt::To(mpt::CharsetISO8859_1, tags.trackno ).c_str());
		if(!tags.genre.empty())    lame.id3tag_set_genre(   gfp, mpt::To(mpt::CharsetISO8859_1, tags.genre   ).c_str());
	}
	virtual void WriteInterleaved(size_t count, const float *interleaved)
	{
		if(!gfp_inited)
		{
			lame.lame_init_params(gfp);
			gfp_inited = true;
		}
		buf.resize(count + (count+3)/4 + 7200);
		if(lame.lame_get_num_channels(gfp) == 1)
		{
			// lame always assumes stereo input with interleaved interface, so use non-interleaved for mono
			buf.resize(lame.lame_encode_buffer_ieee_float(gfp, interleaved, nullptr, count, (unsigned char*)&buf[0], buf.size()));
		} else
		{
			buf.resize(lame.lame_encode_buffer_interleaved_ieee_float(gfp, interleaved, count, (unsigned char*)&buf[0], buf.size()));
		}
		WriteBuffer();
	}
	virtual void Finalize()
	{
		if(!gfp)
		{
			return;
		}
		if(!gfp_inited)
		{
			lame.lame_init_params(gfp);
			gfp_inited = true;
		}
		buf.resize(7200);
		buf.resize(lame.lame_encode_flush(gfp, (unsigned char*)&buf[0], buf.size()));
		WriteBuffer();
#ifdef MPT_FORCE_ID3V1_TAGS_IN_CBR_MODE
		if(Mode != Encoder::ModeCBR)
#endif
		{
			std::streampos endPos = f.tellp();
			std::size_t id3v2Size = lame.lame_get_id3v2_tag(gfp, nullptr, 0);
			f.seekp(fStart + std::streampos(id3v2Size));
			buf.resize(lame.lame_get_lametag_frame(gfp, nullptr, 0));
			buf.resize(lame.lame_get_lametag_frame(gfp, (unsigned char*)&buf[0], buf.size()));
			WriteBuffer();
			f.seekp(endPos);
		}
		lame.lame_close(gfp);
		gfp = lame_t();
		gfp_inited = false;
	}
};

#endif // MPT_MP3ENCODER_LAME



#ifdef MPT_MP3ENCODER_BLADE

struct BladeDynBind
{

	mpt::Library hBlade;

	int lame;

	VOID (*beVersion) (PBE_VERSION);
	BE_ERR (*beInitStream) (PBE_CONFIG, PDWORD, PDWORD, PHBE_STREAM);
	BE_ERR (*beEncodeChunk) (HBE_STREAM, DWORD, PSHORT, PBYTE, PDWORD);
	BE_ERR (*beDeinitStream) (HBE_STREAM, PBYTE, PDWORD);
	BE_ERR (*beCloseStream) (HBE_STREAM);

	void Reset()
	{
		lame = 0;
	}
	BladeDynBind()
	{
		Reset();
		if(!hBlade.IsValid())
		{
			TryLoad(MPT_PATHSTRING("lame_enc"));
			if(hBlade.IsValid()) lame = 1;
		}
		if(!hBlade.IsValid())
		{
			TryLoad(MPT_PATHSTRING("bladeenc"));
			if(hBlade.IsValid()) lame = 0;
		}
	}
	void TryLoad(const mpt::PathString &filename)
	{
		hBlade = mpt::Library(mpt::LibraryPath::AppFullName(filename));
		if(!hBlade.IsValid())
		{
			return;
		}
		bool ok = true;
		#define BLADE_BIND(f) do { \
			if(!hBlade.Bind( f , #f )) \
			{ \
				ok = false; \
			} \
		} while(0)
		BLADE_BIND(beVersion);
		BLADE_BIND(beInitStream);
		BLADE_BIND(beEncodeChunk);
		BLADE_BIND(beDeinitStream);
		BLADE_BIND(beCloseStream);
		#undef BLADE_BIND
		if(!ok)
		{
			hBlade.Unload();
			Reset();
			return;
		}
	}
	operator bool () const { return hBlade.IsValid(); }
	~BladeDynBind()
	{
		hBlade.Unload();
		Reset();
	}
	Encoder::Traits BuildTraits()
	{
		Encoder::Traits traits;
		if(!*this)
		{
			return traits;
		}
		traits.fileExtension = "mp3";
		traits.fileShortDescription = std::string("MP3 ") + (lame ? "(Lame_enc)" : "(BladeEnc)");
		traits.fileDescription = "MPEG-1 Layer 3";
		traits.encoderSettingsName = "MP3Blade";
		traits.encoderName = lame ? "Lame_enc.dll" : "BladeEnc.dll";
		std::ostringstream description;
		description.imbue(std::locale::classic());
		BE_VERSION ver;
		MemsetZero(ver);
		beVersion(&ver);
		description << "DLL version: " << (int)ver.byDLLMajorVersion << "." << (int)ver.byDLLMinorVersion << std::endl;
		description << "Engine version: " << (int)ver.byMajorVersion << "." << (int)ver.byMinorVersion << " ";
		description << std::setfill('0') << std::setw(4) << (int)ver.wYear << "-" << std::setfill('0') << std::setw(2) << (int)ver.byMonth << "-" << std::setfill('0') << std::setw(2) << (int)ver.byDay << std::endl;
		std::string url;
		mpt::String::Copy(url, ver.zHomepage);
		description << "URL: " << url << std::endl;
		traits.description = description.str();
		traits.canTags = true;
		traits.maxChannels = 2;
		traits.samplerates = std::vector<uint32>(mpeg1layer3_samplerates, mpeg1layer3_samplerates + CountOf(mpeg1layer3_samplerates));;
		traits.modes = Encoder::ModeABR;
		traits.bitrates = std::vector<int>(mpeg1layer3_bitrates, mpeg1layer3_bitrates + CountOf(mpeg1layer3_bitrates));
		traits.defaultSamplerate = 44100;
		traits.defaultChannels = 2;
		traits.defaultMode = Encoder::ModeABR;
		traits.defaultBitrate = 256;
		return traits;
	}
};

class MP3BladeStreamWriter : public StreamWriterBase
{
private:
	BladeDynBind &blade;
	DWORD blade_inputsamples;
	DWORD blade_outputbytes;
	SC::Convert<int16,float> sampleConv[2];
	std::deque<int16> blade_sampleBuf;
	PBE_CONFIG becfg;
	HBE_STREAM bestream;

	std::vector<float> interleaved;
	std::vector<SHORT> samples;
public:
	MP3BladeStreamWriter(BladeDynBind &blade_, std::ostream &stream)
		: StreamWriterBase(stream)
		, blade(blade_)
	{
		blade_inputsamples = 0;
		blade_outputbytes = 0;
		becfg = nullptr;
		bestream = HBE_STREAM();
	}
	virtual ~MP3BladeStreamWriter()
	{
		Finalize();
	}
	virtual void SetFormat(const Encoder::Settings &settings)
	{
		uint32 samplerate = settings.Samplerate;
		uint16 channels = settings.Channels;

		if(samplerate <= 32000)
		{
			samplerate = 32000;
		} else if(samplerate <= 44100)
		{
			samplerate = 44100;
		} else
		{
			samplerate = 48000;
		}

		int bitrate = settings.Bitrate;
		for(std::size_t i = 0; i < CountOf(mpeg1layer3_bitrates); ++i)
		{
			if(bitrate <= mpeg1layer3_bitrates[i])
			{
				bitrate = mpeg1layer3_bitrates[i];
				break;
			}
		}
		LimitMax(bitrate, mpeg1layer3_bitrates[CountOf(mpeg1layer3_bitrates)-1]);

		bestream = HBE_STREAM();

		becfg = new BE_CONFIG();
		becfg->dwConfig = BE_CONFIG_MP3;
		becfg->format.mp3.dwSampleRate = samplerate;
		becfg->format.mp3.byMode = (channels == 2) ? BE_MP3_MODE_STEREO : BE_MP3_MODE_MONO;
		becfg->format.mp3.wBitrate = (WORD)bitrate;
		becfg->format.mp3.bPrivate = FALSE;
		becfg->format.mp3.bCRC = FALSE;
		becfg->format.mp3.bCopyright = FALSE;
		becfg->format.mp3.bOriginal = TRUE;

		blade_inputsamples = 1152 * channels; // 1 frame
		blade_outputbytes = 1440; // 320kBit 32kHz frame
		blade.beInitStream(becfg, &blade_inputsamples, &blade_outputbytes, &bestream);

	}
	virtual void WriteMetatags(const FileTags &tags)
	{
		ID3V2Tagger tagger;
		tagger.WriteID3v2Tags(f, tags);
	}
	virtual void WriteInterleaved(size_t count, const float *interleaved)
	{
		if(becfg->format.mp3.byMode == BE_MP3_MODE_MONO)
		{
			for(std::size_t i = 0; i < count; ++i)
			{
				blade_sampleBuf.push_back(sampleConv[0](interleaved[i]));
			}
		} else
		{
			for(std::size_t i = 0; i < count; ++i)
			{
				blade_sampleBuf.push_back(sampleConv[0](interleaved[i*2+0]));
				blade_sampleBuf.push_back(sampleConv[1](interleaved[i*2+1]));
			}
		}
		count *= (becfg->format.mp3.byMode == BE_MP3_MODE_STEREO) ? 2 : 1;
		while(blade_sampleBuf.size() >= blade_inputsamples)
		{
			samples.clear();
			for(std::size_t i = 0; i < blade_inputsamples; ++i)
			{
				samples.push_back(blade_sampleBuf.front());
				blade_sampleBuf.pop_front();
			}
			DWORD size = 0;
			buf.resize(blade_outputbytes);
			blade.beEncodeChunk(bestream, samples.size(), &samples[0], (PBYTE)&buf[0], &size);
			ASSERT(size <= buf.size());
			buf.resize(size);
			WriteBuffer();
		}
	}
	virtual void Finalize()
	{
		if(!bestream)
		{
			return;
		}
		if(blade_sampleBuf.size() > 0)
		{
			samples.resize(blade_sampleBuf.size());
			std::copy(blade_sampleBuf.begin(), blade_sampleBuf.end(), samples.begin());
			blade_sampleBuf.clear();
			DWORD size = 0;
			buf.resize(blade_outputbytes);
			blade.beEncodeChunk(bestream, samples.size(), &samples[0], (PBYTE)&buf[0], &size);
			ASSERT(size <= buf.size());
			buf.resize(size);
			WriteBuffer();
		}
		{
			DWORD size = 0;
			buf.resize(blade_outputbytes);
			blade.beDeinitStream(bestream, (PBYTE)&buf[0], &size);
			buf.resize(size);
			WriteBuffer();
		}
		blade.beCloseStream(bestream);
		bestream = HBE_STREAM();
		delete becfg;
		becfg = nullptr;
	}
};

#endif // MPT_MP3ENCODER_BLADE



#ifdef MPT_MP3ENCODER_ACM

struct AcmDynBind
{
	bool found_codec;
	int samplerates[CountOf(layer3_samplerates)];

	std::vector<Encoder::Format> formats;
	std::vector<HACMDRIVERID> formats_driverids;
	std::vector<std::vector<char> > formats_waveformats;
	std::set<std::string> drivers;

	void Reset()
	{
		found_codec = false;
		MemsetZero(samplerates);
		formats.clear();
		formats_driverids.clear();
		formats_waveformats.clear();
	}
	AcmDynBind()
	{
		Reset();
		TryLoad();
	}
	static BOOL CALLBACK AcmFormatEnumCallback(HACMDRIVERID driver, LPACMFORMATDETAILS pafd, DWORD_PTR inst, DWORD fdwSupport)
	{
		return reinterpret_cast<AcmDynBind*>(inst)->AcmFormatEnumCB(driver, pafd, fdwSupport);
	}
	template<size_t size>
	static void AppendField(std::ostream &s, const std::string &field, const CHAR(&val)[size])
	{
		std::string tmp;
		mpt::String::Copy(tmp, val);
		if(!tmp.empty())
		{
			s << field << ": " << tmp << std::endl;
		}
	}
	BOOL AcmFormatEnumCB(HACMDRIVERID driver, LPACMFORMATDETAILS pafd, DWORD fdwSupport)
	{
		ACMLOG("ACM format found:");
		ACMLOG(mpt::String::Print(" fdwSupport = 0x%1", mpt::fmt::hex0<8>(fdwSupport)));

		if(pafd)
		{
			ACMLOG(" ACMFORMATDETAILS:");
			ACMLOG(mpt::String::Print("  cbStruct = %1", pafd->cbStruct));
			ACMLOG(mpt::String::Print("  dwFormatIndex = %1", pafd->dwFormatIndex));
			ACMLOG(mpt::String::Print("  dwFormatTag = %1", pafd->dwFormatTag));
			ACMLOG(mpt::String::Print("  fdwSupport = 0x%1", mpt::fmt::hex0<8>(pafd->fdwSupport)));
			std::string str;
			mpt::String::Read<mpt::String::maybeNullTerminated>(str, pafd->szFormat);
			ACMLOG(mpt::String::Print("  Format = %1", str));
		} else
		{
			ACMLOG(" ACMFORMATDETAILS = NULL");
		}

		if(pafd && pafd->pwfx && (fdwSupport & ACMDRIVERDETAILS_SUPPORTF_CODEC) && (pafd->dwFormatTag == WAVE_FORMAT_MPEGLAYER3))
		{
			ACMLOG(" MP3 format found!");

			ACMDRIVERDETAILS add;
			MemsetZero(add);
			add.cbStruct = sizeof(add);
			try
			{
				if(acmDriverDetails(driver, &add, 0) != MMSYSERR_NOERROR)
				{
					ACMLOG(" acmDriverDetails = ERROR");
					// No driver details? Skip it.
					return TRUE;
				}
				ACMLOG(" ACMDRIVERDETAILS:");
				ACMLOG(mpt::String::Print("  cbStruct = %1", add.cbStruct));
				ACMLOG(mpt::String::Print("  fccType = 0x%1", mpt::fmt::hex0<4>(add.fccType)));
				ACMLOG(mpt::String::Print("  fccComp = 0x%1", mpt::fmt::hex0<4>(add.fccComp)));
				ACMLOG(mpt::String::Print("  wMid = %1", add.wMid));
				ACMLOG(mpt::String::Print("  wPid = %1", add.wPid));
				ACMLOG(mpt::String::Print("  vdwACM = 0x%1", mpt::fmt::hex0<8>(add.vdwACM)));
				ACMLOG(mpt::String::Print("  vdwDriver = 0x%1", mpt::fmt::hex0<8>(add.vdwDriver)));
				ACMLOG(mpt::String::Print("  fdwSupport = %1", mpt::fmt::hex0<8>(add.fdwSupport)));
				ACMLOG(mpt::String::Print("  cFormatTags = %1", add.cFormatTags));
				ACMLOG(mpt::String::Print("  cFilterTags = %1", add.cFilterTags));
				std::string str;
				mpt::String::Read<mpt::String::maybeNullTerminated>(str, add.szShortName);
				ACMLOG(mpt::String::Print("  ShortName = %1", str));
				mpt::String::Read<mpt::String::maybeNullTerminated>(str, add.szLongName);
				ACMLOG(mpt::String::Print("  LongName = %1", str));
				mpt::String::Read<mpt::String::maybeNullTerminated>(str, add.szCopyright);
				ACMLOG(mpt::String::Print("  Copyright = %1", str));
				mpt::String::Read<mpt::String::maybeNullTerminated>(str, add.szLicensing);
				ACMLOG(mpt::String::Print("  Licensing = %1", str));
				mpt::String::Read<mpt::String::maybeNullTerminated>(str, add.szFeatures);
				ACMLOG(mpt::String::Print("  Features = %1", str));
			} catch(...)
			{
				ACMLOG(" acmDriverDetails = EXCEPTION");
				// Driver crashed? Skip it.
				return TRUE;
			}

			std::string driverNameLong;
			std::string driverNameShort;
			std::string driverNameCombined;
			std::ostringstream driverDescription;
			driverDescription.imbue(std::locale::classic());

			if(add.szLongName[0])
			{
				mpt::String::Copy(driverNameLong, add.szLongName);
			}
			if(add.szShortName[0])
			{
				mpt::String::Copy(driverNameShort, add.szShortName);
			}
			if(driverNameShort.empty())
			{
				if(driverNameLong.empty())
				{
					driverNameCombined = "";
				} else
				{
					driverNameCombined = driverNameLong;
				}
			} else
			{
				if(driverNameLong.empty())
				{
					driverNameCombined = driverNameShort;
				} else
				{
					driverNameCombined = driverNameLong;
				}
			}

			AppendField(driverDescription, "Driver", add.szShortName);
			AppendField(driverDescription, "Description", add.szLongName);
			AppendField(driverDescription, "Copyright", add.szCopyright);
			AppendField(driverDescription, "Licensing", add.szLicensing);
			AppendField(driverDescription, "Features", add.szFeatures);

			for(int i = 0; i < CountOf(layer3_samplerates); ++i)
			{
				if(layer3_samplerates[i] == (int)pafd->pwfx->nSamplesPerSec)
				{

					std::string formatName;

					formatName = pafd->szFormat;
					
					if(!driverNameCombined.empty())
					{
						formatName += " (";
						formatName += driverNameCombined;
						formatName += ")";
					}

					// Blacklist Wine MP3 Decoder because it can only decode.
					if(driverNameShort == "WINE-MPEG3" && driverNameLong == "Wine MPEG3 decoder")
					{
						continue;
					}

					samplerates[i] = layer3_samplerates[i];
					Encoder::Format format;
					format.Samplerate = samplerates[i];
					format.Channels = pafd->pwfx->nChannels;
					format.Sampleformat = SampleFormatInvalid;
					format.Bitrate = pafd->pwfx->nAvgBytesPerSec * 8 / 1000;
					format.Description = formatName;
					formats.push_back(format);
					formats_driverids.push_back(driver);
					{
						std::vector<char> wfex(sizeof(WAVEFORMATEX) + pafd->pwfx->cbSize);
						std::memcpy(&wfex[0], pafd->pwfx, wfex.size());
						formats_waveformats.push_back(wfex);
					}

					if(!driverDescription.str().empty())
					{
						drivers.insert(driverDescription.str());
					}

					found_codec = true;
				}
			}
		}
		return TRUE;
	}
	void EnumFormats(int samplerate, int channels)
	{
		ACMFORMATDETAILS afd;
		BYTE wfx[256];
		WAVEFORMATEX *pwfx = (WAVEFORMATEX *)&wfx;
		MemsetZero(afd);
		MemsetZero(*pwfx);
		afd.cbStruct = sizeof(ACMFORMATDETAILS);
		afd.dwFormatTag = WAVE_FORMAT_PCM;
		afd.pwfx = pwfx;
		afd.cbwfx = sizeof(wfx);
		pwfx->wFormatTag = WAVE_FORMAT_PCM;
		pwfx->nChannels = (WORD)channels;
		pwfx->nSamplesPerSec = samplerate;
		pwfx->wBitsPerSample = 16;
		pwfx->nBlockAlign = (WORD)((pwfx->nChannels * pwfx->wBitsPerSample) / 8);
		pwfx->nAvgBytesPerSec = pwfx->nSamplesPerSec * pwfx->nBlockAlign;
		acmFormatEnum(NULL, &afd, AcmFormatEnumCallback, reinterpret_cast<DWORD_PTR>(this), ACM_FORMATENUMF_CONVERT);
	}
	void TryLoad()
	{
		if(acmGetVersion() <= 0x03320000)
		{
			Reset();
			return;
		}
		try
		{
			for(std::size_t i = 0; i < CountOf(mpeg1layer3_samplerates); ++i)
			{
				EnumFormats(mpeg1layer3_samplerates[i], 2);
			}
		} catch(...)
		{
			// continue
		}
		try
		{
			for(std::size_t i = 0; i < CountOf(mpeg1layer3_samplerates); ++i)
			{
				EnumFormats(mpeg1layer3_samplerates[i], 1);
			}
		} catch(...)
		{
			// continue
		}
		if(!found_codec)
		{
			Reset();
			return;
		}
	}
	operator bool () const { return found_codec ? true : false; }
	~AcmDynBind()
	{
		Reset();
	}
	Encoder::Traits BuildTraits()
	{
		Encoder::Traits traits;
		if(!*this)
		{
			return traits;
		}
		traits.fileExtension = "mp3";
		traits.fileShortDescription = "MP3 (ACM)";
		traits.fileDescription = "MPEG Layer 3";
		std::ostringstream name;
		name.imbue(std::locale::classic());
		DWORD ver = acmGetVersion();
		if(ver & 0xffff)
		{
			name << "Microsoft Windows ACM " << ((ver>>24)&0xff) << "." << ((ver>>16)&0xff) << "." << ((ver>>0)&0xffff);
		} else
		{
			name << "Microsoft Windows ACM " << ((ver>>24)&0xff) << "." << ((ver>>16)&0xff);
		}
		traits.encoderSettingsName = "MP3ACM";
		traits.encoderName = name.str();
		for(std::set<std::string>::const_iterator i = drivers.begin(); i != drivers.end(); ++i)
		{
			traits.description += (*i);
		}
		traits.canTags = true;
		traits.maxChannels = 2;
		traits.samplerates.clear();
		for(int i = 0; i < CountOf(samplerates); ++i)
		{
			if(samplerates[i])
			{
				traits.samplerates.push_back(samplerates[i]);
			}
		}
		traits.modes = Encoder::ModeEnumerated;
		traits.formats = formats;
		traits.defaultSamplerate = 44100;
		traits.defaultChannels = 2;
		traits.defaultMode = Encoder::ModeEnumerated;
		traits.defaultFormat = 0;
		traits.defaultBitrate = 256;
		return traits;
	}
};

class MP3AcmStreamWriter : public StreamWriterBase
{
private:
	AcmDynBind &acm;
	static const size_t acmBufSize = 1024;
	int acmChannels;
	SC::Convert<int16,float> sampleConv[2];
	HACMDRIVER acmDriver;
	HACMSTREAM acmStream;
	ACMSTREAMHEADER acmHeader;
	std::vector<BYTE> acmSrcBuf;
	std::vector<BYTE> acmDstBuf;
	std::deque<int16> acm_sampleBuf;

	std::vector<float> interleaved;
	std::vector<int16> samples;
public:
	MP3AcmStreamWriter(AcmDynBind &acm_, std::ostream &stream)
		: StreamWriterBase(stream)
		, acm(acm_)
	{
		acmDriver = NULL;
		acmStream = NULL;
		MemsetZero(acmHeader);
	}
	virtual ~MP3AcmStreamWriter()
	{
		Finalize();
	}
	virtual void SetFormat(const Encoder::Settings &settings)
	{
		uint32 samplerate = settings.Samplerate;
		uint16 channels = settings.Channels;

		int format = settings.Format;
		format = Clamp(format, 0, (int)acm.formats.size());

		if(acmDriverOpen(&acmDriver, acm.formats_driverids[format], 0) != MMSYSERR_NOERROR)
		{
			return;
		}

		acmChannels = channels;

		WAVEFORMATEX wfex;
		MemsetZero(wfex);
		wfex.wFormatTag = WAVE_FORMAT_PCM;
		wfex.nSamplesPerSec = samplerate;
		wfex.nChannels = (WORD)acmChannels;
		wfex.wBitsPerSample = 16;
		wfex.nBlockAlign = (wfex.nChannels * wfex.wBitsPerSample) / 8;
		wfex.nAvgBytesPerSec = wfex.nSamplesPerSec * wfex.nBlockAlign;
		wfex.cbSize = 0;
		LPWAVEFORMATEX pwfexDst = (LPWAVEFORMATEX)&acm.formats_waveformats[format][0];

		bool acmFast = StreamEncoderSettings::Instance().MP3ACMFast;
		if(acmStreamOpen(&acmStream, acmDriver, &wfex, pwfexDst, NULL, 0, 0, acmFast ? 0 : ACM_STREAMOPENF_NONREALTIME) != MMSYSERR_NOERROR)
		{
			acmDriverClose(acmDriver, 0);
			acmDriver = NULL;
			return;
		}

		DWORD acmDstBufSize = 0;
		if(acmStreamSize(acmStream, acmBufSize, &acmDstBufSize, ACM_STREAMSIZEF_SOURCE) != MMSYSERR_NOERROR)
		{
			acmStreamClose(acmStream, 0);
			acmStream = NULL;
			acmDriverClose(acmDriver, 0);
			acmDriver = NULL;
			return;
		}

		acmSrcBuf.resize(acmBufSize);
		acmDstBuf.resize(acmDstBufSize);
		if(acmSrcBuf.empty() || acmDstBuf.empty())
		{
			acmStreamClose(acmStream, 0);
			acmStream = NULL;
			acmDriverClose(acmDriver, 0);
			acmDriver = NULL;
			return;
		}

		MemsetZero(acmHeader);
		acmHeader.cbStruct = sizeof(acmHeader);
		acmHeader.pbSrc = &acmSrcBuf[0];
		acmHeader.cbSrcLength = acmBufSize;
		acmHeader.pbDst = &acmDstBuf[0];
		acmHeader.cbDstLength = acmDstBufSize;
		if(acmStreamPrepareHeader(acmStream, &acmHeader, 0) != MMSYSERR_NOERROR)
		{
			acmStreamClose(acmStream, 0);
			acmStream = NULL;
			acmDriverClose(acmDriver, 0);
			acmDriver = NULL;
			return;
		}
	}
	virtual void WriteMetatags(const FileTags &tags)
	{
		ID3V2Tagger tagger;
		tagger.WriteID3v2Tags(f, tags);
	}
	virtual void WriteInterleaved(size_t count, const float *interleaved)
	{
		if(!acmStream)
		{
			return;
		}
		if(acmChannels == 1)
		{
			for(std::size_t i = 0; i < count; ++i)
			{
				acm_sampleBuf.push_back(sampleConv[0](interleaved[i]));
			}
		} else
		{
			for(std::size_t i = 0; i < count; ++i)
			{
				acm_sampleBuf.push_back(sampleConv[0](interleaved[i*2+0]));
				acm_sampleBuf.push_back(sampleConv[1](interleaved[i*2+1]));
			}
		}
		count *= acmChannels;
		while(acm_sampleBuf.size() >= acmSrcBuf.size() / sizeof(int16))
		{
			samples.clear();
			for(std::size_t i = 0; i < acmSrcBuf.size() / sizeof(int16); ++i)
			{
				samples.push_back(acm_sampleBuf.front());
				acm_sampleBuf.pop_front();
			}
			std::memcpy(&acmSrcBuf[0], &samples[0], acmSrcBuf.size());
			acmHeader.cbSrcLength = acmSrcBuf.size();
			acmHeader.cbSrcLengthUsed = 0;
			acmHeader.cbDstLength = acmDstBuf.size();
			acmHeader.cbDstLengthUsed = 0;
			acmStreamConvert(acmStream, &acmHeader, ACM_STREAMCONVERTF_BLOCKALIGN);
			if(acmHeader.cbDstLengthUsed)
			{
				buf.resize(acmHeader.cbDstLengthUsed);
				std::memcpy(&buf[0], &acmDstBuf[0], acmHeader.cbDstLengthUsed);
				WriteBuffer();
			}
			if(acmHeader.cbSrcLengthUsed < acmSrcBuf.size())
			{
				samples.resize((acmSrcBuf.size() - acmHeader.cbSrcLengthUsed) / sizeof(int16));
				std::memcpy(&samples[0], &acmSrcBuf[acmHeader.cbSrcLengthUsed], acmSrcBuf.size() - acmHeader.cbSrcLengthUsed);
				while(samples.size() > 0)
				{
					acm_sampleBuf.push_front(samples.back());
					samples.pop_back();
				}
			}
		}
	}
	virtual void Finalize()
	{
		if(!acmStream)
		{
			return;
		}
		samples.clear();
		if(!acm_sampleBuf.empty())
		{
			while(!acm_sampleBuf.empty())
			{
				samples.push_back(acm_sampleBuf.front());
				acm_sampleBuf.pop_front();
			}
			std::memcpy(&acmSrcBuf[0], &samples[0], samples.size() * sizeof(int16));
		}
		acmHeader.cbSrcLength = samples.size() * sizeof(int16);
		acmHeader.cbSrcLengthUsed = 0;
		acmHeader.cbDstLength = acmDstBuf.size();
		acmHeader.cbDstLengthUsed = 0;
		acmStreamConvert(acmStream, &acmHeader, ACM_STREAMCONVERTF_END);
		if(acmHeader.cbDstLengthUsed)
		{
			buf.resize(acmHeader.cbDstLengthUsed);
			std::memcpy(&buf[0], &acmDstBuf[0], acmHeader.cbDstLengthUsed);
			WriteBuffer();
		}
		acmStreamUnprepareHeader(acmStream, &acmHeader, 0);
		MemsetZero(acmHeader);
		acmStreamClose(acmStream, 0);
		acmStream = NULL;
		acmDriverClose(acmDriver, 0);
		acmDriver = NULL;
	}
};

#endif // MPT_MP3ENCODER_ACM



MP3Encoder::MP3Encoder(MP3EncoderType type)
//-----------------------------------------
#ifdef MPT_MP3ENCODER_LAME
	: m_Lame(nullptr)
#endif // MPT_MP3ENCODER_LAME
#ifdef MPT_MP3ENCODER_BLADE
	, m_Blade(nullptr)
#endif // MPT_MP3ENCODER_BLADE
#ifdef MPT_MP3ENCODER_ACM
	, m_Acm(nullptr)
#endif // MPT_MP3ENCODER_ACM
	, m_Type(MP3EncoderDefault)
{
#ifdef MPT_MP3ENCODER_LAME
	if(type == MP3EncoderDefault || type == MP3EncoderLame)
	{
		m_Lame = new LameDynBind();
		if(*m_Lame)
		{
			m_Type = MP3EncoderLame;
			SetTraits(m_Lame->BuildTraits());
			return;
		}
	}
#endif // MPT_MP3ENCODER_LAME
#ifdef MPT_MP3ENCODER_BLADE
	if(type == MP3EncoderDefault || type == MP3EncoderBlade)
	{
		m_Blade = new BladeDynBind();
		if(*m_Blade)
		{
			m_Type = MP3EncoderBlade;
			SetTraits(m_Blade->BuildTraits());
			return;
		}
	}
#endif // MPT_MP3ENCODER_BLADE
#ifdef MPT_MP3ENCODER_ACM
	if(type == MP3EncoderDefault || type == MP3EncoderACM)
	{
		m_Acm = new AcmDynBind();
		if(*m_Acm)
		{
			m_Type = MP3EncoderACM;
			SetTraits(m_Acm->BuildTraits());
			return;
		}
	}
#endif // MPT_MP3ENCODER_ACM
}


bool MP3Encoder::IsAvailable() const
//----------------------------------
{
	return false
#ifdef MPT_MP3ENCODER_LAME
		|| (m_Lame && *m_Lame)
#endif // MPT_MP3ENCODER_ACM
#ifdef MPT_MP3ENCODER_BLADE
		|| (m_Blade && *m_Blade)
#endif // MPT_MP3ENCODER_BLADE
#ifdef MPT_MP3ENCODER_ACM
		|| (m_Acm && *m_Acm)
#endif // MPT_MP3ENCODER_ACM
		;
}


MP3Encoder::~MP3Encoder()
//-----------------------
{
#ifdef MPT_MP3ENCODER_ACM
	if(m_Acm)
	{
		delete m_Acm;
		m_Acm = nullptr;
	}
#endif // MPT_MP3ENCODER_ACM
#ifdef MPT_MP3ENCODER_BLADE
	if(m_Blade)
	{
		delete m_Blade;
		m_Blade = nullptr;
	}
#endif // MPT_MP3ENCODER_BLADE
#ifdef MPT_MP3ENCODER_LAME
	if(m_Lame)
	{
		delete m_Lame;
		m_Lame = nullptr;
	}
#endif // MPT_MP3ENCODER_LAME
}


IAudioStreamEncoder *MP3Encoder::ConstructStreamEncoder(std::ostream &file) const
//-------------------------------------------------------------------------------
{
	StreamWriterBase *result = nullptr;
	if(false)
	{
		// nothing
#ifdef MPT_MP3ENCODER_LAME
	} else if(m_Type == MP3EncoderLame)
	{
		result = new MP3LameStreamWriter(*m_Lame, file);
#endif // MPT_MP3ENCODER_LAME
#ifdef MPT_MP3ENCODER_BLADE
	} else if(m_Type == MP3EncoderBlade)
	{
		result = new MP3BladeStreamWriter(*m_Blade, file);
#endif // MPT_MP3ENCODER_BLADE
#ifdef MPT_MP3ENCODER_ACM
	} else if(m_Type == MP3EncoderACM)
	{
		result = new MP3AcmStreamWriter(*m_Acm, file);
#endif // MPT_MP3ENCODER_ACM
	}
	return result;
}


std::string MP3Encoder::DescribeQuality(float quality) const
//----------------------------------------------------------
{
#ifdef MPT_MP3ENCODER_LAME
	if(m_Type == MP3EncoderLame)
	{
		int q = static_cast<int>((1.0f - quality) * 10.0f);
		if(q < 0) q = 0;
		if(q >= 10)
		{
			return "VBR -V9.999";
		} else
		{
			return mpt::String::Format("VBR -V%i", static_cast<int>((1.0f - quality) * 10.0f));
		}
	}
#endif // MPT_MP3ENCODER_LAME
	return EncoderFactoryBase::DescribeQuality(quality);
}

std::string MP3Encoder::DescribeBitrateABR(int bitrate) const
//-----------------------------------------------------------
{
#ifdef MPT_MP3ENCODER_BLADE
	if(m_Type == MP3EncoderBlade)
	{
		return mpt::String::Format("%i kbit", bitrate);
	}
#endif // MPT_MP3ENCODER_BLADE
	return EncoderFactoryBase::DescribeBitrateABR(bitrate);
}

