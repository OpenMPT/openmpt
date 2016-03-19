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

#include "../common/ComponentManager.h"

#include "Mptrack.h"

#include "../soundlib/Sndfile.h"

#include "../soundlib/SampleFormatConverters.h"
#include "../common/misc_util.h"
#include "../common/StringFixer.h"

#include <deque>

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
#include <mmreg.h>
#include <msacm.h>
#endif // MPT_MP3ENCODER_ACM



OPENMPT_NAMESPACE_BEGIN



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

// charset... choose text ending accordingly.
// $00 = ISO-8859-1. Terminated with $00.
// $01 = UTF-16 with BOM. Terminated with $00 00.
// $02 = UTF-16BE without BOM. Terminated with $00 00.
// $03 = UTF-8. Terminated with $00.
#define ID3v2_CHARSET '\3'
#define ID3v2_TEXTENDING '\0'

struct ReplayGain
{
	enum GainTag
	{
		TagSkip,
		TagReserve,
		TagWrite
	};
	GainTag Tag;
	float TrackPeak;
	bool TrackPeakValid;
	float TrackGaindB;
	bool TrackGaindBValid;
	ReplayGain()
		: Tag(TagSkip)
		, TrackPeak(0.0f)
		, TrackPeakValid(false)
		, TrackGaindB(0.0f)
		, TrackGaindBValid(false)
	{
		return;
	}
};

//===============
class ID3V2Tagger
//===============
{
public:
	// Write Tags
	void WriteID3v2Tags(std::ostream &s, const FileTags &tags, ReplayGain replayGain = ReplayGain());

	ID3V2Tagger();

private:
	// Convert Integer to Synchsafe Integer (see ID3v2.4 specs)
	uint32 intToSynchsafe(uint32 in);
	// Write a frame
	void WriteID3v2Frame(char cFrameID[4], std::string sFramecontent, std::ostream &s);
	// Return an upper bound for the size of all replay gain frames
	uint32 GetMaxReplayGainFramesSizes();
	uint32 GetMaxReplayGainTxxxTrackGainFrameSize();
	uint32 GetMaxReplayGainTxxxTrackPeakFrameSize();
	// Write out all ReplayGain frames
	void WriteID3v2ReplayGainFrames(ReplayGain replaygain, std::ostream &s);
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
void ID3V2Tagger::WriteID3v2Tags(std::ostream &s, const FileTags &tags, ReplayGain replayGain)
//--------------------------------------------------------------------------------------------
{
	if(!s) return;
	
	ID3v2Header tHeader;
	std::streampos fOffset = s.tellp();
	uint32 paddingSize = 0;

	totalID3v2Size = 0;

	// Correct header will be written later (tag size missing)
	tHeader.signature[0] = 'I';
	tHeader.signature[1] = 'D';
	tHeader.signature[2] = '3';
	tHeader.version[0] = 0x04; // Version 2.4.0
	tHeader.version[1] = 0x00; // Ditto
	tHeader.flags = 0x00; // No flags
	tHeader.size  = 0; // will be filled later
	s.write(reinterpret_cast<const char*>(&tHeader), sizeof(tHeader));
	totalID3v2Size += sizeof(tHeader);

	// Write TIT2 (Title), TCOM / TPE1 (Composer), TALB (Album), TCON (Genre), TYER / TDRC (Date), WXXX (URL), TENC (Encoder), COMM (Comment)
	WriteID3v2Frame("TIT2", mpt::ToCharset(mpt::CharsetUTF8, tags.title), s);
	WriteID3v2Frame("TPE1", mpt::ToCharset(mpt::CharsetUTF8, tags.artist), s);
	WriteID3v2Frame("TCOM", mpt::ToCharset(mpt::CharsetUTF8, tags.artist), s);
	WriteID3v2Frame("TALB", mpt::ToCharset(mpt::CharsetUTF8, tags.album), s);
	WriteID3v2Frame("TCON", mpt::ToCharset(mpt::CharsetUTF8, tags.genre), s);
	//WriteID3v2Frame("TYER", mpt::ToCharset(mpt::CharsetUTF8, tags.year), s);		// Deprecated
	WriteID3v2Frame("TDRC", mpt::ToCharset(mpt::CharsetUTF8, tags.year), s);
	WriteID3v2Frame("TBPM", mpt::ToCharset(mpt::CharsetUTF8, tags.bpm), s);
	WriteID3v2Frame("WXXX", mpt::ToCharset(mpt::CharsetUTF8, tags.url), s);
	WriteID3v2Frame("TENC", mpt::ToCharset(mpt::CharsetUTF8, tags.encoder), s);
	WriteID3v2Frame("COMM", mpt::ToCharset(mpt::CharsetUTF8, tags.comments), s);
	if(replayGain.Tag == ReplayGain::TagReserve)
	{
		paddingSize += GetMaxReplayGainFramesSizes();
	} else if(replayGain.Tag == ReplayGain::TagWrite)
	{
		std::streampos replayGainBeg = s.tellp();
		WriteID3v2ReplayGainFrames(replayGain, s);
		std::streampos replayGainEnd = s.tellp();
		paddingSize += GetMaxReplayGainFramesSizes() - static_cast<uint32>(replayGainEnd - replayGainBeg);
	}

	// Write Padding
	uint32 totalID3v2SizeWithoutPadding = totalID3v2Size;
	paddingSize += StreamEncoderSettings::Instance().MP3ID3v2MinPadding;
	totalID3v2Size += paddingSize;
	if(StreamEncoderSettings::Instance().MP3ID3v2PaddingAlignHint > 0)
	{
		totalID3v2Size = Util::AlignUp<uint32>(totalID3v2Size, StreamEncoderSettings::Instance().MP3ID3v2PaddingAlignHint);
		paddingSize = totalID3v2Size - totalID3v2SizeWithoutPadding;
	}
	for(size_t i = 0; i < paddingSize; i++)
	{
		char c = 0;
		s.write(&c, 1);
	}

	// Write correct header (update tag size)
	tHeader.size = intToSynchsafe(totalID3v2Size - sizeof(tHeader));
	s.seekp(fOffset);
	s.write(reinterpret_cast<const char*>(&tHeader), sizeof(tHeader));
	s.seekp(totalID3v2Size - sizeof(tHeader), std::ios::cur);

}

uint32 ID3V2Tagger::GetMaxReplayGainTxxxTrackGainFrameSize()
{
	return sizeof(ID3v2Frame) + 1 + std::strlen("REPLAYGAIN_TRACK_GAIN") + 1 + std::strlen("-123.45 dB") + 1; // should be enough
}

uint32 ID3V2Tagger::GetMaxReplayGainTxxxTrackPeakFrameSize()
{
	return sizeof(ID3v2Frame) + 1 + std::strlen("REPLAYGAIN_TRACK_PEAK") + 1 + std::strlen("2147483648.123456") + 1; // unrealistic worst case

}

uint32 ID3V2Tagger::GetMaxReplayGainFramesSizes()
//-----------------------------------------------
{
	uint32 size = 0;
	if(StreamEncoderSettings::Instance().MP3ID3v2WriteReplayGainTXXX)
	{
		size += GetMaxReplayGainTxxxTrackGainFrameSize();
		size += GetMaxReplayGainTxxxTrackPeakFrameSize();
	}
	return size;
}

void ID3V2Tagger::WriteID3v2ReplayGainFrames(ReplayGain replayGain, std::ostream &s)
//----------------------------------------------------------------------------------
{
	ID3v2Frame frame;
	std::string content;


	if(StreamEncoderSettings::Instance().MP3ID3v2WriteReplayGainTXXX && replayGain.TrackGaindBValid)
	{

		std::memset(&frame, 0, sizeof(ID3v2Frame));
		content.clear();

		content += std::string(1, 0x00); // ISO-8859-1
		content += std::string("REPLAYGAIN_TRACK_GAIN");
		content += std::string(1, '\0');

		int32 gainTimes100 = Util::Round<int32>(replayGain.TrackGaindB * 100.0f);
		if(gainTimes100 < 0)
		{
			content += "-";
			gainTimes100 = std::abs(gainTimes100);
		}
		content += mpt::fmt::dec(gainTimes100 / 100);
		content += ".";
		content += mpt::fmt::dec0<2>(gainTimes100 % 100);
		content += " ";
		content += "dB";

		content += std::string(1, '\0');

		std::memcpy(&frame.frameid, "TXXX", 4);
		frame.size = intToSynchsafe(content.size());
		frame.flags = SwapBytesReturnBE(uint16(0x4000)); // discard if audio data changed
		if(sizeof(ID3v2Frame) + content.size() <= GetMaxReplayGainTxxxTrackGainFrameSize())
		{
			s.write(reinterpret_cast<const char*>(&frame), sizeof(ID3v2Frame));
			s.write(content.data(), content.size());
		}

	}


	if(StreamEncoderSettings::Instance().MP3ID3v2WriteReplayGainTXXX && replayGain.TrackPeakValid)
	{

		std::memset(&frame, 0, sizeof(ID3v2Frame));
		content.clear();

		content += std::string(1, 0x00); // ISO-8859-1
		content += std::string("REPLAYGAIN_TRACK_PEAK");
		content += std::string(1, '\0');

		int32 peakTimes1000000 = Util::Round<int32>(std::fabs(replayGain.TrackPeak) * 1000000.0f);
		std::string number;
		number += mpt::fmt::dec(peakTimes1000000 / 1000000);
		number += ".";
		number += mpt::fmt::dec0<6>(peakTimes1000000 % 1000000);
		content += number;

		content += std::string(1, '\0');

		std::memcpy(&frame.frameid, "TXXX", 4);
		frame.size = intToSynchsafe(content.size());
		frame.flags = SwapBytesReturnBE(uint16(0x4000)); // discard if audio data changed
		if(sizeof(ID3v2Frame) + content.size() <= GetMaxReplayGainTxxxTrackPeakFrameSize())
		{
			s.write(reinterpret_cast<const char*>(&frame), sizeof(ID3v2Frame));
			s.write(content.data(), content.size());
		}

	}

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

class ComponentLame
	: public ComponentLibrary
{

	MPT_DECLARE_COMPONENT_MEMBERS

public:

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
	int  (CDECL * lame_set_decode_on_the_fly)(lame_global_flags *, int);
	int  (CDECL * lame_set_findReplayGain)(lame_global_flags *, int);

	void (CDECL * id3tag_genre_list)(void (*handler)(int, const char *, void *), void* cookie);
	void (CDECL * id3tag_init)     (lame_t gfp);
	void (CDECL * id3tag_v1_only)  (lame_t gfp);
	void (CDECL * id3tag_add_v2)   (lame_t gfp);
	void (CDECL * id3tag_v2_only)  (lame_t gfp);
	void (CDECL * id3tag_set_pad)  (lame_t gfp, size_t n);
	void (CDECL * lame_set_write_id3tag_automatic)(lame_global_flags * gfp, int);

	float (CDECL* lame_get_PeakSample)(const lame_global_flags *);
	int  (CDECL * lame_get_RadioGain)(const lame_global_flags *);

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

private:

	void Reset()
	{
		ClearLibraries();
	}

public:

	ComponentLame()
		: ComponentLibrary(ComponentTypeForeign)
	{
		return;
	}

protected:

	bool DoInitialize()
	{
		Reset();
		struct dll_names_t {
			const char *lame;
		};
		static const dll_names_t dll_names[] = {
			{ "libmp3lame" },
			{ "liblame" },
			{ "mp3lame" },
			{ "lame" },
			{ "lame_enc" },
		};
		bool ok = false;
		for(std::size_t i=0; i<CountOf(dll_names); ++i)
		{
			if(TryLoad(mpt::PathString::FromUTF8(dll_names[i].lame)))
			{
				ok = true;
				break;
			}
		}
		return ok;
	}

private:

	bool TryLoad(const mpt::PathString &filename)
	{
		Reset();
		ClearBindFailed();
		if(!AddLibrary("libmp3lame", mpt::LibraryPath::AppFullName(filename)))
		{
			Reset();
			return false;
		}
		MPT_COMPONENT_BIND("libmp3lame", get_lame_version);
		MPT_COMPONENT_BIND("libmp3lame", get_lame_short_version);
		MPT_COMPONENT_BIND("libmp3lame", get_lame_very_short_version);
		MPT_COMPONENT_BIND("libmp3lame", get_psy_version);
		MPT_COMPONENT_BIND("libmp3lame", get_lame_url);
		MPT_COMPONENT_BIND("libmp3lame", lame_init);
		MPT_COMPONENT_BIND("libmp3lame", lame_set_in_samplerate);
		MPT_COMPONENT_BIND("libmp3lame", lame_set_num_channels);
		MPT_COMPONENT_BIND("libmp3lame", lame_get_num_channels);
		MPT_COMPONENT_BIND("libmp3lame", lame_get_quality);
		MPT_COMPONENT_BIND("libmp3lame", lame_set_quality);
		MPT_COMPONENT_BIND("libmp3lame", lame_set_out_samplerate);
		MPT_COMPONENT_BIND("libmp3lame", lame_set_brate);
		MPT_COMPONENT_BIND("libmp3lame", lame_set_VBR_quality);
		MPT_COMPONENT_BIND("libmp3lame", lame_set_VBR);
		MPT_COMPONENT_BIND("libmp3lame", lame_set_bWriteVbrTag);
		MPT_COMPONENT_BIND("libmp3lame", lame_set_strict_ISO);
		MPT_COMPONENT_BIND("libmp3lame", lame_set_disable_reservoir);
		MPT_COMPONENT_BIND("libmp3lame", lame_set_decode_on_the_fly);
		MPT_COMPONENT_BIND("libmp3lame", lame_set_findReplayGain);
		MPT_COMPONENT_BIND("libmp3lame", id3tag_genre_list);
		MPT_COMPONENT_BIND("libmp3lame", id3tag_init);
		MPT_COMPONENT_BIND("libmp3lame", id3tag_v1_only);
		MPT_COMPONENT_BIND("libmp3lame", id3tag_add_v2);
		MPT_COMPONENT_BIND("libmp3lame", id3tag_v2_only);
		MPT_COMPONENT_BIND("libmp3lame", id3tag_set_pad);
		MPT_COMPONENT_BIND("libmp3lame", lame_set_write_id3tag_automatic);
		MPT_COMPONENT_BIND("libmp3lame", lame_get_PeakSample);
		MPT_COMPONENT_BIND("libmp3lame", lame_get_RadioGain);
		MPT_COMPONENT_BIND("libmp3lame", id3tag_set_title);
		MPT_COMPONENT_BIND("libmp3lame", id3tag_set_artist);
		MPT_COMPONENT_BIND("libmp3lame", id3tag_set_album);
		MPT_COMPONENT_BIND("libmp3lame", id3tag_set_year);
		MPT_COMPONENT_BIND("libmp3lame", id3tag_set_comment);
		MPT_COMPONENT_BIND("libmp3lame", id3tag_set_track);
		MPT_COMPONENT_BIND("libmp3lame", id3tag_set_genre);
		MPT_COMPONENT_BIND("libmp3lame", lame_init_params);
		MPT_COMPONENT_BIND("libmp3lame", lame_encode_buffer_ieee_float);
		MPT_COMPONENT_BIND("libmp3lame", lame_encode_buffer_interleaved_ieee_float);
		MPT_COMPONENT_BIND("libmp3lame", lame_encode_flush);
		MPT_COMPONENT_BIND("libmp3lame", lame_get_lametag_frame);
		MPT_COMPONENT_BIND("libmp3lame", lame_get_id3v2_tag);
		MPT_COMPONENT_BIND("libmp3lame", lame_close);
		if(HasBindFailed())
		{
			Reset();
			return false;
		}
		return true;
	}

public:

	static void GenreEnumCallback(int num, const char *name, void *cookie)
	{
		MPT_UNREFERENCED_PARAMETER(num);
		Encoder::Traits &traits = *reinterpret_cast<Encoder::Traits*>(cookie);
		if(name)
		{
			traits.genres.push_back(mpt::ToUnicode(mpt::CharsetISO8859_1, name));
		}
	}
	Encoder::Traits BuildTraits(bool compatible)
	{
		Encoder::Traits traits;
		if(!IsAvailable())
		{
			return traits;
		}
		traits.fileExtension = MPT_PATHSTRING("mp3");
		traits.fileShortDescription = (compatible ? MPT_USTRING("MP3 (compatible)") : MPT_USTRING("MP3"));
		traits.fileDescription = (compatible ? MPT_USTRING("MPEG-1 Layer 3") : MPT_USTRING("MPEG-1/2 Layer 3"));
		traits.encoderSettingsName = (compatible ? MPT_USTRING("MP3LameCompatible") : MPT_USTRING("MP3Lame"));
		traits.encoderName = MPT_USTRING("libMP3Lame");
		traits.description += MPT_USTRING("Version: ");
		traits.description += mpt::ToUnicode(mpt::CharsetASCII, get_lame_version()?get_lame_version():"");
		traits.description += MPT_USTRING("\n");
		traits.description += MPT_USTRING("Psycho acoustic model version: ");
		traits.description += mpt::ToUnicode(mpt::CharsetASCII, get_psy_version()?get_psy_version():"");
		traits.description += MPT_USTRING("\n");
		traits.description += MPT_USTRING("URL: ");
		traits.description += mpt::ToUnicode(mpt::CharsetASCII, get_lame_url()?get_lame_url():"");
		traits.description += MPT_USTRING("\n");
		traits.canTags = true;
		traits.genres.clear();
		id3tag_genre_list(&GenreEnumCallback, &traits);
		traits.modesWithFixedGenres = (compatible ? Encoder::ModeCBR : Encoder::ModeInvalid);
		traits.maxChannels = 2;
		traits.samplerates = (compatible
			? std::vector<uint32>(mpeg1layer3_samplerates, mpeg1layer3_samplerates + CountOf(mpeg1layer3_samplerates))
			: std::vector<uint32>(layer3_samplerates, layer3_samplerates + CountOf(layer3_samplerates))
			);
		traits.modes = (compatible ? Encoder::ModeCBR : (Encoder::ModeCBR | Encoder::ModeQuality));
		traits.bitrates = (compatible
			? std::vector<int>(mpeg1layer3_bitrates, mpeg1layer3_bitrates + CountOf(mpeg1layer3_bitrates))
			: std::vector<int>(layer3_bitrates, layer3_bitrates + CountOf(layer3_bitrates))
			);
		traits.defaultSamplerate = 44100;
		traits.defaultChannels = 2;
		traits.defaultMode = (compatible ? Encoder::ModeCBR : Encoder::ModeQuality);
		traits.defaultBitrate = 256;
		traits.defaultQuality = 0.8f;
		return traits;
	}
};
MPT_REGISTERED_COMPONENT(ComponentLame, "LibMP3Lame")

class MP3LameStreamWriter : public StreamWriterBase
{
private:
	ComponentLame &lame;
	bool compatible;
	Encoder::Mode Mode;
	bool gfp_inited;
	lame_t gfp;
	enum ID3Type
	{
		ID3None,
		ID3v1,
		ID3v2Lame,
		ID3v2OpenMPT,
	};
	ID3Type id3type;
	std::streamoff id3v2Size;
	FileTags Tags;
public:
	MP3LameStreamWriter(ComponentLame &lame_, std::ostream &stream, bool compatible)
		: StreamWriterBase(stream)
		, lame(lame_)
		, compatible(compatible)
	{
		Mode = Encoder::ModeInvalid;
		gfp_inited = false;
		gfp = lame_t();
		id3type = ID3v2Lame;
		id3v2Size = 0;
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
		if(settings.Tags)
		{
			if(compatible)
			{
				id3type = ID3v1;
			} else if(StreamEncoderSettings::Instance().MP3LameID3v2UseLame)
			{
				id3type = ID3v2Lame;
			} else
			{
				id3type = ID3v2OpenMPT;
			}
		} else
		{
			id3type = ID3None;
		}
		id3v2Size = 0;

		lame.lame_set_in_samplerate(gfp, samplerate);
		lame.lame_set_num_channels(gfp, channels);

		int lameQuality = StreamEncoderSettings::Instance().MP3LameQuality;
		lame.lame_set_quality(gfp, lameQuality);

		if(settings.Mode == Encoder::ModeCBR)
		{

			if(compatible && settings.Bitrate >= 32)
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

			if(compatible)
			{
				lame.lame_set_bWriteVbrTag(gfp, 0);
				lame.lame_set_strict_ISO(gfp, 1);
				lame.lame_set_disable_reservoir(gfp, 1);
			} else
			{
				lame.lame_set_bWriteVbrTag(gfp, 1);
			}

		} else
		{

			float lame_quality = 10.0f - (settings.Quality * 10.0f);
			Limit(lame_quality, 0.0f, 9.999f);
			lame.lame_set_VBR_quality(gfp, lame_quality);
			lame.lame_set_VBR(gfp, vbr_default);

			lame.lame_set_bWriteVbrTag(gfp, 1);

		}

		lame.lame_set_decode_on_the_fly(gfp, StreamEncoderSettings::Instance().MP3LameCalculatePeakSample ? 1 : 0); // see LAME docs for why
		lame.lame_set_findReplayGain(gfp, StreamEncoderSettings::Instance().MP3LameCalculateReplayGain ? 1 : 0);

		switch(id3type)
		{
		case ID3None:
			lame.lame_set_write_id3tag_automatic(gfp, 0);
			break;
		case ID3v1:
			lame.id3tag_init(gfp);
			lame.id3tag_v1_only(gfp);
			break;
		case ID3v2Lame:
			lame.id3tag_init(gfp);
			lame.id3tag_add_v2(gfp);
			lame.id3tag_v2_only(gfp);
			lame.id3tag_set_pad(gfp, StreamEncoderSettings::Instance().MP3ID3v2MinPadding);
			break;
		case ID3v2OpenMPT:
			lame.lame_set_write_id3tag_automatic(gfp, 0);
			break;
		}

		Mode = settings.Mode;
	}
	virtual void WriteMetatags(const FileTags &tags)
	{
		if(id3type == ID3v2Lame || id3type == ID3v1)
		{
			// Lame API expects Latin1, which is sad, but we cannot change that.
			if(!tags.title.empty())    lame.id3tag_set_title(  gfp, mpt::ToCharset(mpt::CharsetISO8859_1, tags.title   ).c_str());
			if(!tags.artist.empty())   lame.id3tag_set_artist( gfp, mpt::ToCharset(mpt::CharsetISO8859_1, tags.artist  ).c_str());
			if(!tags.album.empty())    lame.id3tag_set_album(  gfp, mpt::ToCharset(mpt::CharsetISO8859_1, tags.album   ).c_str());
			if(!tags.year.empty())     lame.id3tag_set_year(   gfp, mpt::ToCharset(mpt::CharsetISO8859_1, tags.year    ).c_str());
			if(!tags.comments.empty()) lame.id3tag_set_comment(gfp, mpt::ToCharset(mpt::CharsetISO8859_1, tags.comments).c_str());
			if(!tags.trackno.empty())  lame.id3tag_set_track(  gfp, mpt::ToCharset(mpt::CharsetISO8859_1, tags.trackno ).c_str());
			if(!tags.genre.empty())    lame.id3tag_set_genre(  gfp, mpt::ToCharset(mpt::CharsetISO8859_1, tags.genre   ).c_str());
		} else if(id3type == ID3v2OpenMPT)
		{
			Tags = tags;
			std::streampos id3beg = f.tellp();
			ID3V2Tagger tagger;
			ReplayGain replayGain;
			if(StreamEncoderSettings::Instance().MP3LameCalculatePeakSample || StreamEncoderSettings::Instance().MP3LameCalculateReplayGain)
			{
				replayGain.Tag = ReplayGain::TagReserve;
			}
			tagger.WriteID3v2Tags(f, tags, replayGain);
			std::streampos id3end = f.tellp();
			id3v2Size = id3end - id3beg;
		}
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
		ReplayGain replayGain;
		if(StreamEncoderSettings::Instance().MP3LameCalculatePeakSample)
		{
			replayGain.TrackPeak = std::fabs(lame.lame_get_PeakSample(gfp)) / 32768.0f;
			replayGain.TrackPeakValid = true;
		}
		if(StreamEncoderSettings::Instance().MP3LameCalculateReplayGain)
		{
			replayGain.TrackGaindB = lame.lame_get_RadioGain(gfp) / 10.0f;
			replayGain.TrackGaindBValid = true;
		}
		if(id3type == ID3v2OpenMPT && (StreamEncoderSettings::Instance().MP3LameCalculatePeakSample || StreamEncoderSettings::Instance().MP3LameCalculateReplayGain))
		{ // update ID3v2 tag with replay gain information
			replayGain.Tag = ReplayGain::TagWrite;
			std::streampos endPos = f.tellp();
			f.seekp(fStart);
			std::string tagdata(static_cast<std::size_t>(id3v2Size), '\0');
			f.write(tagdata.data(), id3v2Size); // clear out the old tag
			f.seekp(fStart);
			ID3V2Tagger tagger;
			tagger.WriteID3v2Tags(f, Tags, replayGain);
			f.seekp(endPos);
		}
		if(id3type == ID3v2Lame)
		{
			id3v2Size = lame.lame_get_id3v2_tag(gfp, nullptr, 0);
		} else if(id3type == ID3v2OpenMPT)
		{
			// id3v2Size already set
		}
		if(!compatible)
		{
			std::streampos endPos = f.tellp();
			f.seekp(fStart + id3v2Size);
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

class ComponentBlade
	: public ComponentLibrary
{

	MPT_DECLARE_COMPONENT_MEMBERS

public:

	bool isLame;
	mpt::PathString encoderDLL;

	VOID (*beVersion) (PBE_VERSION);
	BE_ERR (*beInitStream) (PBE_CONFIG, PDWORD, PDWORD, PHBE_STREAM);
	BE_ERR (*beEncodeChunk) (HBE_STREAM, DWORD, PSHORT, PBYTE, PDWORD);
	BE_ERR (*beDeinitStream) (HBE_STREAM, PBYTE, PDWORD);
	BE_ERR (*beCloseStream) (HBE_STREAM);

private:

	void Reset()
	{
		ClearLibraries();
		isLame = false;
		encoderDLL = mpt::PathString();
	}

public:

	ComponentBlade()
		: ComponentLibrary(ComponentTypeForeign)
		, isLame(false)
	{
		return;
	}

protected:

	bool DoInitialize()
	{
		Reset();
		if(TryLoad(MPT_PATHSTRING("lame_enc")))
		{
			isLame = true;
			encoderDLL = MPT_PATHSTRING("lame_enc");
			return true;
		} else if(TryLoad(MPT_PATHSTRING("BladeMP3EncDLL")))
		{
			isLame = true;
			encoderDLL = MPT_PATHSTRING("BladeMP3EncDLL");
			return true;
		} else if(TryLoad(MPT_PATHSTRING("bladeenc")))
		{
			isLame = false;
			encoderDLL = MPT_PATHSTRING("bladeenc");
			return true;
		}
		return false;
	}

private:

	bool TryLoad(const mpt::PathString &filename)
	{
		Reset();
		ClearBindFailed();
		if(!AddLibrary("BladeEnc", mpt::LibraryPath::AppFullName(filename)))
		{
			Reset();
			return false;
		}
		MPT_COMPONENT_BIND("BladeEnc", beVersion);
		MPT_COMPONENT_BIND("BladeEnc", beInitStream);
		MPT_COMPONENT_BIND("BladeEnc", beEncodeChunk);
		MPT_COMPONENT_BIND("BladeEnc", beDeinitStream);
		MPT_COMPONENT_BIND("BladeEnc", beCloseStream);
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
		traits.fileExtension = MPT_PATHSTRING("mp3");
		traits.fileShortDescription = MPT_USTRING("MP3");
		traits.fileDescription = MPT_USTRING("MPEG-1 Layer 3");
		traits.encoderSettingsName = MPT_USTRING("MP3Blade");
		traits.encoderName = mpt::String::Print(MPT_USTRING("%2.dll (%1)"), (isLame ? MPT_USTRING("Lame_enc") : MPT_USTRING("BladeEnc")), encoderDLL.ToUnicode());
		BE_VERSION ver;
		MemsetZero(ver);
		beVersion(&ver);
		traits.description = MPT_USTRING("");
		traits.description += mpt::String::Print(MPT_USTRING("DLL version: %1.%2\n"), (int)ver.byDLLMajorVersion, (int)ver.byDLLMinorVersion);
		traits.description += mpt::String::Print(MPT_USTRING("Engine version: %1.%2 %3-%4-%5\n")
			, (int)ver.byMajorVersion, (int)ver.byMinorVersion
			, mpt::ufmt::dec0<4>((int)ver.wYear), mpt::ufmt::dec0<2>((int)ver.byMonth), mpt::ufmt::dec0<2>((int)ver.byDay)
			);
		std::string url;
		mpt::String::Copy(url, ver.zHomepage);
		traits.description += mpt::String::Print(MPT_USTRING("URL: %1\n"), mpt::ToUnicode(mpt::CharsetASCII, url));
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
MPT_REGISTERED_COMPONENT(ComponentBlade, "BladeEnc")

class MP3BladeStreamWriter : public StreamWriterBase
{
private:
	ComponentBlade &blade;
	DWORD blade_inputsamples;
	DWORD blade_outputbytes;
	SC::Convert<int16,float> sampleConv[2];
	std::deque<int16> blade_sampleBuf;
	PBE_CONFIG becfg;
	HBE_STREAM bestream;

	std::vector<float> interleaved;
	std::vector<SHORT> samples;
public:
	MP3BladeStreamWriter(ComponentBlade &blade_, std::ostream &stream)
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
			blade.beEncodeChunk(bestream, static_cast<DWORD>(samples.size()), &samples[0], (PBYTE)&buf[0], &size);
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
			blade.beEncodeChunk(bestream, static_cast<DWORD>(samples.size()), &samples[0], (PBYTE)&buf[0], &size);
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

class ComponentAcmMP3
	: public ComponentBase
{

	MPT_DECLARE_COMPONENT_MEMBERS

public:

	bool found_codec;
	int samplerates[CountOf(layer3_samplerates)];

	std::vector<Encoder::Format> formats;
	std::vector<HACMDRIVERID> formats_driverids;
	std::vector<std::vector<char> > formats_waveformats;
	std::set<mpt::ustring> drivers;

private:

	void Reset()
	{
		found_codec = false;
		MemsetZero(samplerates);
		formats.clear();
		formats_driverids.clear();
		formats_waveformats.clear();
	}

public:

	ComponentAcmMP3()
		: ComponentBase(ComponentTypeSystemInstallable)
	{
		return;
	}

protected:

	bool DoInitialize()
	{
		Reset();
		return TryLoad();
	}

private:

	static BOOL CALLBACK AcmFormatEnumCallback(HACMDRIVERID driver, LPACMFORMATDETAILS pafd, DWORD_PTR inst, DWORD fdwSupport)
	{
		return reinterpret_cast<ComponentAcmMP3*>(inst)->AcmFormatEnumCB(driver, pafd, fdwSupport);
	}
	static void AppendField(mpt::ustring &s, const mpt::ustring &field, const mpt::ustring &val)
	{
		if(!val.empty())
		{
			s += field + MPT_USTRING(": ") + val + MPT_USTRING("\n");
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
			ACMLOG(mpt::String::Print("  Format = %1", mpt::FromTcharBuf(pafd->szFormat)));
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
				ACMLOG(mpt::String::Print("  ShortName = %1", mpt::FromTcharBuf(add.szShortName)));
				ACMLOG(mpt::String::Print("  LongName = %1", mpt::FromTcharBuf(add.szLongName)));
				ACMLOG(mpt::String::Print("  Copyright = %1", mpt::FromTcharBuf(add.szCopyright)));
				ACMLOG(mpt::String::Print("  Licensing = %1", mpt::FromTcharBuf(add.szLicensing)));
				ACMLOG(mpt::String::Print("  Features = %1", mpt::FromTcharBuf(add.szFeatures)));
			} catch(...)
			{
				ACMLOG(" acmDriverDetails = EXCEPTION");
				// Driver crashed? Skip it.
				return TRUE;
			}

			mpt::ustring driverNameLong;
			mpt::ustring driverNameShort;
			mpt::ustring driverNameCombined;
			mpt::ustring driverDescription;

			if(add.szLongName[0])
			{
				driverNameLong = mpt::FromTcharBuf(add.szLongName);
			}
			if(add.szShortName[0])
			{
				driverNameShort = mpt::FromTcharBuf(add.szShortName);
			}
			if(driverNameShort.empty())
			{
				if(driverNameLong.empty())
				{
					driverNameCombined = MPT_USTRING("");
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

			AppendField(driverDescription, MPT_USTRING("Driver"), mpt::FromTcharBuf(add.szShortName));
			AppendField(driverDescription, MPT_USTRING("Description"), mpt::FromTcharBuf(add.szLongName));
			AppendField(driverDescription, MPT_USTRING("Copyright"), mpt::FromTcharBuf(add.szCopyright));
			AppendField(driverDescription, MPT_USTRING("Licensing"), mpt::FromTcharBuf(add.szLicensing));
			AppendField(driverDescription, MPT_USTRING("Features"), mpt::FromTcharBuf(add.szFeatures));

			for(int i = 0; i < CountOf(layer3_samplerates); ++i)
			{
				if(layer3_samplerates[i] == (int)pafd->pwfx->nSamplesPerSec)
				{

					mpt::ustring formatName;

					formatName = mpt::FromTcharBuf(pafd->szFormat);
					
					if(!driverNameCombined.empty())
					{
						formatName += MPT_USTRING(" (");
						formatName += driverNameCombined;
						formatName += MPT_USTRING(")");
					}

					// Blacklist Wine MP3 Decoder because it can only decode.
					if(driverNameShort == MPT_USTRING("WINE-MPEG3") && driverNameLong == MPT_USTRING("Wine MPEG3 decoder"))
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

					if(!driverDescription.empty())
					{
						drivers.insert(driverDescription);
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

private:

	bool TryLoad()
	{
		if(acmGetVersion() <= 0x03320000)
		{
			Reset();
			return false;
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
		traits.fileExtension = MPT_PATHSTRING("mp3");
		traits.fileShortDescription = MPT_USTRING("MP3");
		traits.fileDescription = MPT_USTRING("MPEG Layer 3");
		DWORD ver = acmGetVersion();
		if(ver & 0xffff)
		{
			traits.encoderName = mpt::String::Print(MPT_USTRING("%1 %2.%3.%4"), MPT_USTRING("Microsoft Windows ACM"), mpt::ufmt::hex0<2>((ver>>24)&0xff), mpt::ufmt::hex0<2>((ver>>16)&0xff), mpt::ufmt::hex0<4>((ver>>0)&0xffff));
		} else
		{
			traits.encoderName = mpt::String::Print(MPT_USTRING("%1 %2.%3"), MPT_USTRING("Microsoft Windows ACM"), mpt::ufmt::hex0<2>((ver>>24)&0xff), mpt::ufmt::hex0<2>((ver>>16)&0xff));
		}
		traits.encoderSettingsName = MPT_USTRING("MP3ACM");
		for(std::set<mpt::ustring>::const_iterator i = drivers.begin(); i != drivers.end(); ++i)
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
MPT_REGISTERED_COMPONENT(ComponentAcmMP3, "ACM_MP3")

class MP3AcmStreamWriter : public StreamWriterBase
{
private:
	ComponentAcmMP3 &acm;
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
	MP3AcmStreamWriter(ComponentAcmMP3 &acm_, std::ostream &stream)
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
			acmHeader.cbSrcLength = static_cast<DWORD>(acmSrcBuf.size());
			acmHeader.cbSrcLengthUsed = 0;
			acmHeader.cbDstLength = static_cast<DWORD>(acmDstBuf.size());
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
		acmHeader.cbSrcLength = static_cast<DWORD>(samples.size() * sizeof(int16));
		acmHeader.cbSrcLengthUsed = 0;
		acmHeader.cbDstLength = static_cast<DWORD>(acmDstBuf.size());
		acmHeader.cbDstLengthUsed = 0;
		acmStreamConvert(acmStream, &acmHeader, ACM_STREAMCONVERTF_END);
		if(acmHeader.cbDstLengthUsed)
		{
			buf.resize(acmHeader.cbDstLengthUsed);
			std::memcpy(&buf[0], &acmDstBuf[0], acmHeader.cbDstLengthUsed);
			WriteBuffer();
		}
		// acmStreamUnprepareHeader demands original buffer sizes to be specified
		acmHeader.cbSrcLength = static_cast<DWORD>(acmSrcBuf.size());
		acmHeader.cbSrcLengthUsed = 0;
		acmHeader.cbDstLength = static_cast<DWORD>(acmDstBuf.size());
		acmHeader.cbDstLengthUsed = 0;
		std::fill(acmSrcBuf.begin(), acmSrcBuf.end(), 0);
		std::fill(acmDstBuf.begin(), acmDstBuf.end(), 0);
		acmStreamUnprepareHeader(acmStream, &acmHeader, 0);
		if(0 < acmHeader.cbDstLengthUsed && acmHeader.cbDstLengthUsed <= acmDstBuf.size())
		{
			// LAME ACM flushes the MP3 codec here, instead of when getting passed
			// ACM_STREAMCONVERTF_END.
			// This is not documented or supported behaviour according to MSDN,
			// and probably even violates the interface.
			// Instead of checking against the codec name string, we rely on other
			// codecs at least not messing with cbDstLengthUsed in
			// acmStreamUnprepareHeader.
			buf.resize(acmHeader.cbDstLengthUsed);
			std::memcpy(&buf[0], &acmDstBuf[0], acmHeader.cbDstLengthUsed);
			WriteBuffer();
		}
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
	: m_Type(MP3EncoderDefault)
{
#ifdef MPT_MP3ENCODER_LAME
	if(type == MP3EncoderDefault || type == MP3EncoderLame)
	{
		if(IsComponentAvailable(m_Lame))
		{
			m_Type = MP3EncoderLame;
			SetTraits(m_Lame->BuildTraits(false));
			return;
		}
	}
	if(type == MP3EncoderDefault || type == MP3EncoderLameCompatible)
	{
		if(IsComponentAvailable(m_Lame))
		{
			m_Type = MP3EncoderLameCompatible;
			SetTraits(m_Lame->BuildTraits(true));
			return;
		}
	}
#endif // MPT_MP3ENCODER_LAME
#ifdef MPT_MP3ENCODER_BLADE
	if(type == MP3EncoderDefault || type == MP3EncoderBlade)
	{
		if(IsComponentAvailable(m_Blade))
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
		if(IsComponentAvailable(m_Acm))
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
		|| ((m_Type == MP3EncoderLame) && IsComponentAvailable(m_Lame))
		|| ((m_Type == MP3EncoderLameCompatible) && IsComponentAvailable(m_Lame))
#endif // MPT_MP3ENCODER_ACM
#ifdef MPT_MP3ENCODER_BLADE
		|| ((m_Type == MP3EncoderBlade) && IsComponentAvailable(m_Blade))
#endif // MPT_MP3ENCODER_BLADE
#ifdef MPT_MP3ENCODER_ACM
		|| ((m_Type == MP3EncoderACM) && IsComponentAvailable(m_Acm))
#endif // MPT_MP3ENCODER_ACM
		;
}


MP3Encoder::~MP3Encoder()
//-----------------------
{
	return;
}


IAudioStreamEncoder *MP3Encoder::ConstructStreamEncoder(std::ostream &file) const
//-------------------------------------------------------------------------------
{
	StreamWriterBase *result = nullptr;
	if(false)
	{
		// nothing
#ifdef MPT_MP3ENCODER_LAME
	} else if(m_Type == MP3EncoderLame || m_Type == MP3EncoderLameCompatible)
	{
		result = new MP3LameStreamWriter(*m_Lame, file, (m_Type == MP3EncoderLameCompatible));
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


mpt::ustring MP3Encoder::DescribeQuality(float quality) const
//-----------------------------------------------------------
{
#ifdef MPT_MP3ENCODER_LAME
	if(m_Type == MP3EncoderLame)
	{
		static const int q_table[11] = { 240, 220, 190, 170, 160, 130, 120, 100, 80, 70, 50 }; // http://wiki.hydrogenaud.io/index.php?title=LAME
		int q = Util::Round<int>((1.0f - quality) * 10.0f);
		if(q < 0) q = 0;
		if(q >= 10)
		{
			return mpt::String::Print(MPT_USTRING("VBR -V%1 (~%2 kbit)"), MPT_USTRING("9.999"), q_table[q]);
		} else
		{
			return mpt::String::Print(MPT_USTRING("VBR -V%1 (~%2 kbit)"), Util::Round<int>((1.0f - quality) * 10.0f), q_table[q]);
		}
	}
#endif // MPT_MP3ENCODER_LAME
	return EncoderFactoryBase::DescribeQuality(quality);
}

mpt::ustring MP3Encoder::DescribeBitrateABR(int bitrate) const
//------------------------------------------------------------
{
#ifdef MPT_MP3ENCODER_BLADE
	if(m_Type == MP3EncoderBlade)
	{
		return mpt::String::Print(MPT_USTRING("%1 kbit"), bitrate);
	}
#endif // MPT_MP3ENCODER_BLADE
	return EncoderFactoryBase::DescribeBitrateABR(bitrate);
}



OPENMPT_NAMESPACE_END
