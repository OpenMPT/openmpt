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

#include "../soundbase/SampleFormatConverters.h"
#include "../common/misc_util.h"
#include "../common/StringFixer.h"

#include <deque>
#include <ostream>

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

#ifdef MPT_MP3ENCODER_ACM
#include <mmreg.h>
#include <msacm.h>
#endif // MPT_MP3ENCODER_ACM



OPENMPT_NAMESPACE_BEGIN



///////////////////////////////////////////////////////////////////////////////////////////////////
// ID3v2.4 Tags

struct ID3v2Header
{
	uint8     signature[3];
	uint8     version[2];
	uint8be  flags;
	uint32be size;
	// Total: 10 bytes
};

MPT_BINARY_STRUCT(ID3v2Header, 10)

struct ID3v2Frame
{
	char      frameid[4];
	uint32be size;
	uint16be flags;
	// Total: 10 bytes
};

MPT_BINARY_STRUCT(ID3v2Frame, 10)


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

class ID3V2Tagger
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
{
	uint32 out = 0, steps = 0;
	do
	{
		out |= (in & 0x7F) << steps;
		steps += 8;
	} while(in >>= 7);
	return out;
}

// Write Tags
void ID3V2Tagger::WriteID3v2Tags(std::ostream &s, const FileTags &tags, ReplayGain replayGain)
{
	if(!s) return;
	
	ID3v2Header tHeader;
	std::streampos fOffset = s.tellp();
	uint32 paddingSize = 0;

	totalID3v2Size = 0;

	// Correct header will be written later (tag size missing)
	memcpy(tHeader.signature, "ID3", 3);
	tHeader.version[0] = 0x04; // Version 2.4.0
	tHeader.version[1] = 0x00; // Ditto
	tHeader.flags = 0; // No flags
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
			gainTimes100 = mpt::abs(gainTimes100);
		}
		content += mpt::fmt::dec(gainTimes100 / 100);
		content += ".";
		content += mpt::fmt::dec0<2>(gainTimes100 % 100);
		content += " ";
		content += "dB";

		content += std::string(1, '\0');

		std::memcpy(&frame.frameid, "TXXX", 4);
		frame.size = intToSynchsafe(content.size());
		frame.flags = 0x4000; // discard if audio data changed
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
		frame.flags = 0x4000; // discard if audio data changed
		if(sizeof(ID3v2Frame) + content.size() <= GetMaxReplayGainTxxxTrackPeakFrameSize())
		{
			s.write(reinterpret_cast<const char*>(&frame), sizeof(ID3v2Frame));
			s.write(content.data(), content.size());
		}

	}

}

// Write a ID3v2 frame
void ID3V2Tagger::WriteID3v2Frame(char cFrameID[4], std::string sFramecontent, std::ostream &s)
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




#ifdef MPT_MP3ENCODER_LAME

struct lame_global_struct;
typedef struct lame_global_struct lame_global_flags;
typedef lame_global_flags *lame_t;

class ComponentLame
	: public ComponentLibrary
{

	MPT_DECLARE_COMPONENT_MEMBERS

public:

	const char* (CDECL * get_lame_version)() = nullptr;
	const char* (CDECL * get_lame_short_version)() = nullptr;
	const char* (CDECL * get_lame_very_short_version)() = nullptr;
	const char* (CDECL * get_psy_version)() = nullptr;
	const char* (CDECL * get_lame_url)() = nullptr;

	lame_global_flags * (CDECL * lame_init)(void) = nullptr;

	int  (CDECL * lame_set_in_samplerate)(lame_global_flags *, int) = nullptr;
	int  (CDECL * lame_set_num_channels)(lame_global_flags *, int) = nullptr;
	int  (CDECL * lame_get_num_channels)(const lame_global_flags *) = nullptr;
	int  (CDECL * lame_get_quality)(const lame_global_flags *) = nullptr;
	int  (CDECL * lame_set_quality)(lame_global_flags *, int) = nullptr;
	int  (CDECL * lame_set_out_samplerate)(lame_global_flags *, int) = nullptr;
	int  (CDECL * lame_set_brate)(lame_global_flags *, int) = nullptr;
	int  (CDECL * lame_set_VBR_quality)(lame_global_flags *, float) = nullptr;
	int  (CDECL * lame_set_VBR)(lame_global_flags *, vbr_mode) = nullptr;
	int  (CDECL * lame_set_bWriteVbrTag)(lame_global_flags *, int) = nullptr;
	int  (CDECL * lame_set_strict_ISO)(lame_global_flags *, int) = nullptr;
	int  (CDECL * lame_set_disable_reservoir)(lame_global_flags *, int) = nullptr;
	int  (CDECL * lame_set_decode_on_the_fly)(lame_global_flags *, int) = nullptr;
	int  (CDECL * lame_set_findReplayGain)(lame_global_flags *, int) = nullptr;

	void (CDECL * id3tag_genre_list)(void (*handler)(int, const char *, void *), void* cookie) = nullptr;
	void (CDECL * id3tag_init)     (lame_t gfp) = nullptr;
	void (CDECL * id3tag_v1_only)  (lame_t gfp) = nullptr;
	void (CDECL * id3tag_add_v2)   (lame_t gfp) = nullptr;
	void (CDECL * id3tag_v2_only)  (lame_t gfp) = nullptr;
	void (CDECL * id3tag_set_pad)  (lame_t gfp, size_t n) = nullptr;
	void (CDECL * lame_set_write_id3tag_automatic)(lame_global_flags * gfp, int) = nullptr;

	float (CDECL* lame_get_PeakSample)(const lame_global_flags *) = nullptr;
	int  (CDECL * lame_get_RadioGain)(const lame_global_flags *) = nullptr;

	void (CDECL * id3tag_set_title)(lame_t gfp, const char* title) = nullptr;
	void (CDECL * id3tag_set_artist)(lame_t gfp, const char* artist) = nullptr;
	void (CDECL * id3tag_set_album)(lame_t gfp, const char* album) = nullptr;
	void (CDECL * id3tag_set_year)(lame_t gfp, const char* year) = nullptr;
	void (CDECL * id3tag_set_comment)(lame_t gfp, const char* comment) = nullptr;
	int  (CDECL * id3tag_set_track)(lame_t gfp, const char* track) = nullptr;
	int  (CDECL * id3tag_set_genre)(lame_t gfp, const char* genre) = nullptr;

	int  (CDECL * lame_init_params)(lame_global_flags *) = nullptr;

	int  (CDECL * lame_encode_buffer_ieee_float)(
	                                             lame_t          gfp,
	                                             const float     pcm_l [],
	                                             const float     pcm_r [],
	                                             const int       nsamples,
	                                             unsigned char * mp3buf,
	                                             const int       mp3buf_size) = nullptr;
	int  (CDECL * lame_encode_buffer_interleaved_ieee_float)(
	                                                         lame_t          gfp,
	                                                         const float     pcm[],
	                                                         const int       nsamples,
	                                                         unsigned char * mp3buf,
	                                                         const int       mp3buf_size) = nullptr;
	int  (CDECL * lame_encode_flush)(lame_global_flags * gfp, unsigned char* mp3buf, int size) = nullptr;
	size_t(CDECL* lame_get_lametag_frame)(const lame_global_flags *, unsigned char* buffer, size_t size) = nullptr;
	size_t(CDECL* lame_get_id3v2_tag)(lame_t gfp, unsigned char* buffer, size_t size) = nullptr;

	int  (CDECL * lame_close) (lame_global_flags *) = nullptr;

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
		if(TryLoad(mpt::PathString::FromUTF8("openmpt-lame"), false))
		{
			ok = true;
		}
		if(ok)
		{
			return true;
		}
		for(const auto & dll : dll_names)
		{
			if(TryLoad(mpt::PathString::FromUTF8(dll.lame), true))
			{
				ok = true;
				break;
			}
		}
		if(ok)
		{
			return true;
		}
		for(const auto & dll : dll_names)
		{
			if(TryLoad(mpt::PathString::FromUTF8(dll.lame), false))
			{
				ok = true;
				break;
			}
		}
		return ok;
	}

private:

	bool TryLoad(const mpt::PathString &filename, bool appdata)
	{
		Reset();
		ClearBindFailed();
		if(appdata)
		{
			if(!AddLibrary("libmp3lame", mpt::LibraryPath::AppDataFullName(filename, GetComponentPath())))
			{
				Reset();
				return false;
			}
		} else
		{
			if(!AddLibrary("libmp3lame", mpt::LibraryPath::AppFullName(filename)))
			{
				Reset();
				return false;
			}
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
	Encoder::Traits BuildTraits(bool compatible) const
	{
		Encoder::Traits traits;
		if(!IsAvailable())
		{
			return traits;
		}
		mpt::ustring version;
		if(get_lame_version())
		{
			version = MPT_USTRING("Lame ") + mpt::ToUnicode(mpt::CharsetASCII, get_lame_version());
		} else
		{
			version = MPT_USTRING("Lame");
		}
		traits.fileExtension = MPT_PATHSTRING("mp3");
		traits.fileShortDescription = (compatible ? mpt::format(MPT_USTRING("compatible MP3 (%1)"))(version) : mpt::format(MPT_USTRING("MP3 (%1)"))(version));
		traits.encoderSettingsName = (compatible ? MPT_USTRING("MP3LameCompatible") : MPT_USTRING("MP3Lame"));
		traits.showEncoderInfo = true;
		traits.fileDescription = (compatible ? MPT_USTRING("MPEG-1 Layer 3") : MPT_USTRING("MPEG-1/2 Layer 3"));
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
			? mpt::make_vector(mpeg1layer3_samplerates)
			: mpt::make_vector(layer3_samplerates)
			);
		traits.modes = (compatible ? Encoder::ModeCBR : (Encoder::ModeABR | Encoder::ModeQuality));
		traits.bitrates = (compatible
			? mpt::make_vector(mpeg1layer3_bitrates)
			: mpt::make_vector(layer3_bitrates)
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
	const ComponentLame &lame;
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
	MP3LameStreamWriter(const ComponentLame &lame_, std::ostream &stream, bool compatible, const Encoder::Settings &settings, const FileTags &tags)
		: StreamWriterBase(stream)
		, lame(lame_)
		, compatible(compatible)
	{
		Mode = Encoder::ModeInvalid;
		gfp_inited = false;
		gfp = lame_t();
		id3type = ID3v2Lame;
		id3v2Size = 0;

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

			if(compatible)
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

		} else if(settings.Mode == Encoder::ModeABR)
		{

			lame.lame_set_brate(gfp, settings.Bitrate);
			lame.lame_set_VBR(gfp, vbr_abr);

			lame.lame_set_bWriteVbrTag(gfp, 1);

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

		if(settings.Tags)
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
			buf.resize(lame.lame_encode_buffer_ieee_float(gfp, interleaved, nullptr, count, (unsigned char*)buf.data(), buf.size()));
		} else
		{
			buf.resize(lame.lame_encode_buffer_interleaved_ieee_float(gfp, interleaved, count, (unsigned char*)buf.data(), buf.size()));
		}
		WriteBuffer();
	}
	virtual ~MP3LameStreamWriter()
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
		buf.resize(lame.lame_encode_flush(gfp, (unsigned char*)buf.data(), buf.size()));
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
			buf.resize(lame.lame_get_lametag_frame(gfp, (unsigned char*)buf.data(), buf.size()));
			WriteBuffer();
			f.seekp(endPos);
		}
		lame.lame_close(gfp);
		gfp = lame_t();
		gfp_inited = false;
	}
};

#endif // MPT_MP3ENCODER_LAME



#ifdef MPT_MP3ENCODER_ACM

class ComponentAcmMP3
	: public ComponentBase
{

	MPT_DECLARE_COMPONENT_MEMBERS

public:

	bool found_codec;
	int samplerates[mpt::size(layer3_samplerates)];

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
		drivers.clear();
	}

public:

	ComponentAcmMP3()
		: ComponentBase(ComponentTypeSystemInstallable)
	{
		Reset();
	}

protected:

	bool DoInitialize()
	{
		Reset();
		return TryLoad();
	}

private:

	struct Crash : public std::exception { };

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
	static MMRESULT acmDriverDetailsSafe(HACMDRIVERID hadid, LPACMDRIVERDETAILS padd, DWORD fdwDetails)
	{
		MMRESULT result = static_cast<MMRESULT>(-1);
		bool crashed = false;
		__try
		{
			result = acmDriverDetails(hadid, padd, fdwDetails);
		} __except (EXCEPTION_EXECUTE_HANDLER)
		{
			crashed = true;
		}
		if(crashed)
		{
			throw Crash();
		}
		return result;
	}
	static MMRESULT acmFormatEnumSafe(HACMDRIVER had, LPACMFORMATDETAILS pafd, ACMFORMATENUMCB fnCallback, DWORD_PTR dwInstance, DWORD fdwEnum)
	{
		MMRESULT result = static_cast<MMRESULT>(-1);
		bool crashed = false;
		__try
		{
			result = acmFormatEnum(had, pafd, fnCallback, dwInstance, fdwEnum);
		} __except (EXCEPTION_EXECUTE_HANDLER)
		{
			crashed = true;
		}
		if(crashed)
		{
			throw Crash();
		}
		return result;
	}
	BOOL AcmFormatEnumCB(HACMDRIVERID driver, LPACMFORMATDETAILS pafd, DWORD fdwSupport)
	{
		ACMLOG(MPT_USTRING("ACM format found:"));
		ACMLOG(mpt::format(MPT_USTRING(" fdwSupport = 0x%1"))(mpt::ufmt::hex0<8>(fdwSupport)));

		if(pafd)
		{
			ACMLOG(MPT_USTRING(" ACMFORMATDETAILS:"));
			ACMLOG(mpt::format(MPT_USTRING("  cbStruct = %1"))(pafd->cbStruct));
			ACMLOG(mpt::format(MPT_USTRING("  dwFormatIndex = %1"))(pafd->dwFormatIndex));
			ACMLOG(mpt::format(MPT_USTRING("  dwFormatTag = %1"))(pafd->dwFormatTag));
			ACMLOG(mpt::format(MPT_USTRING("  fdwSupport = 0x%1"))(mpt::ufmt::hex0<8>(pafd->fdwSupport)));
			ACMLOG(mpt::format(MPT_USTRING("  Format = %1"))(mpt::FromTcharBuf(pafd->szFormat)));
		} else
		{
			ACMLOG(MPT_USTRING(" ACMFORMATDETAILS = NULL"));
		}

		if(pafd && pafd->pwfx && (fdwSupport & ACMDRIVERDETAILS_SUPPORTF_CODEC) && (pafd->dwFormatTag == WAVE_FORMAT_MPEGLAYER3))
		{
			ACMLOG(MPT_USTRING(" MP3 format found!"));

			ACMDRIVERDETAILS add;
			MemsetZero(add);
			add.cbStruct = sizeof(add);
			try
			{
				if(acmDriverDetailsSafe(driver, &add, 0) != MMSYSERR_NOERROR)
				{
					ACMLOG(MPT_USTRING(" acmDriverDetails = ERROR"));
					// No driver details? Skip it.
					return TRUE;
				}
				ACMLOG(MPT_USTRING(" ACMDRIVERDETAILS:"));
				ACMLOG(mpt::format(MPT_USTRING("  cbStruct = %1"))(add.cbStruct));
				ACMLOG(mpt::format(MPT_USTRING("  fccType = 0x%1"))(mpt::ufmt::hex0<4>(add.fccType)));
				ACMLOG(mpt::format(MPT_USTRING("  fccComp = 0x%1"))(mpt::ufmt::hex0<4>(add.fccComp)));
				ACMLOG(mpt::format(MPT_USTRING("  wMid = %1"))(add.wMid));
				ACMLOG(mpt::format(MPT_USTRING("  wPid = %1"))(add.wPid));
				ACMLOG(mpt::format(MPT_USTRING("  vdwACM = 0x%1"))(mpt::ufmt::hex0<8>(add.vdwACM)));
				ACMLOG(mpt::format(MPT_USTRING("  vdwDriver = 0x%1"))(mpt::ufmt::hex0<8>(add.vdwDriver)));
				ACMLOG(mpt::format(MPT_USTRING("  fdwSupport = %1"))(mpt::ufmt::hex0<8>(add.fdwSupport)));
				ACMLOG(mpt::format(MPT_USTRING("  cFormatTags = %1"))(add.cFormatTags));
				ACMLOG(mpt::format(MPT_USTRING("  cFilterTags = %1"))(add.cFilterTags));
				ACMLOG(mpt::format(MPT_USTRING("  ShortName = %1"))(mpt::FromTcharBuf(add.szShortName)));
				ACMLOG(mpt::format(MPT_USTRING("  LongName = %1"))(mpt::FromTcharBuf(add.szLongName)));
				ACMLOG(mpt::format(MPT_USTRING("  Copyright = %1"))(mpt::FromTcharBuf(add.szCopyright)));
				ACMLOG(mpt::format(MPT_USTRING("  Licensing = %1"))(mpt::FromTcharBuf(add.szLicensing)));
				ACMLOG(mpt::format(MPT_USTRING("  Features = %1"))(mpt::FromTcharBuf(add.szFeatures)));
			} catch(const Crash &)
			{
				ACMLOG(MPT_USTRING(" acmDriverDetails = EXCEPTION"));
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

			for(std::size_t i = 0; i < mpt::size(layer3_samplerates); ++i)
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
						std::memcpy(wfex.data(), pafd->pwfx, wfex.size());
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
		acmFormatEnumSafe(NULL, &afd, AcmFormatEnumCallback, reinterpret_cast<DWORD_PTR>(this), ACM_FORMATENUMF_CONVERT);
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
			for(auto rate : mpeg1layer3_samplerates)
			{
				EnumFormats(rate, 2);
			}
		} catch(const Crash &)
		{
			// continue
		}
		try
		{
			for(auto rate : mpeg1layer3_samplerates)
			{
				EnumFormats(rate, 1);
			}
		} catch(const Crash &)
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

	Encoder::Traits BuildTraits() const
	{
		Encoder::Traits traits;
		if(!IsAvailable())
		{
			return traits;
		}
		traits.fileExtension = MPT_PATHSTRING("mp3");
		traits.fileShortDescription = MPT_USTRING("MP3 (ACM)");
		traits.encoderSettingsName = MPT_USTRING("MP3ACM");
		traits.showEncoderInfo = true;
		traits.fileDescription = MPT_USTRING("MPEG Layer 3");
		DWORD ver = acmGetVersion();
		if(ver & 0xffff)
		{
			traits.encoderName = mpt::format(MPT_USTRING("%1 %2.%3.%4"))(MPT_USTRING("Microsoft Windows ACM"), mpt::ufmt::hex0<2>((ver>>24)&0xff), mpt::ufmt::hex0<2>((ver>>16)&0xff), mpt::ufmt::hex0<4>((ver>>0)&0xffff));
		} else
		{
			traits.encoderName = mpt::format(MPT_USTRING("%1 %2.%3"))(MPT_USTRING("Microsoft Windows ACM"), mpt::ufmt::hex0<2>((ver>>24)&0xff), mpt::ufmt::hex0<2>((ver>>16)&0xff));
		}
		for(const auto &i : drivers)
		{
			traits.description += i;
		}
		traits.canTags = true;
		traits.maxChannels = 2;
		traits.samplerates.clear();
		for(auto rate : samplerates)
		{
			if(rate)
			{
				traits.samplerates.push_back(rate);
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
	const ComponentAcmMP3 &acm;
	static const size_t acmBufSize = 1024;
	int acmChannels;
	SC::Convert<int16,float> sampleConv[2];
	HACMDRIVER acmDriver;
	HACMSTREAM acmStream;
	ACMSTREAMHEADER acmHeader;
	std::vector<BYTE> acmSrcBuf;
	std::vector<BYTE> acmDstBuf;
	std::deque<int16> acm_sampleBuf;

	std::vector<int16> samples;
public:
	MP3AcmStreamWriter(const ComponentAcmMP3 &acm_, std::ostream &stream, const Encoder::Settings &settings, const FileTags &tags)
		: StreamWriterBase(stream)
		, acm(acm_)
	{
		acmDriver = NULL;
		acmStream = NULL;
		MemsetZero(acmHeader);

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
		acmHeader.pbSrc = acmSrcBuf.data();
		acmHeader.cbSrcLength = acmBufSize;
		acmHeader.pbDst = acmDstBuf.data();
		acmHeader.cbDstLength = acmDstBufSize;
		if(acmStreamPrepareHeader(acmStream, &acmHeader, 0) != MMSYSERR_NOERROR)
		{
			acmStreamClose(acmStream, 0);
			acmStream = NULL;
			acmDriverClose(acmDriver, 0);
			acmDriver = NULL;
			return;
		}

		if(settings.Tags)
		{
			ID3V2Tagger tagger;
			tagger.WriteID3v2Tags(f, tags);
		}
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
			std::memcpy(acmSrcBuf.data(), samples.data(), acmSrcBuf.size());
			acmHeader.cbSrcLength = static_cast<DWORD>(acmSrcBuf.size());
			acmHeader.cbSrcLengthUsed = 0;
			acmHeader.cbDstLength = static_cast<DWORD>(acmDstBuf.size());
			acmHeader.cbDstLengthUsed = 0;
			acmStreamConvert(acmStream, &acmHeader, ACM_STREAMCONVERTF_BLOCKALIGN);
			if(acmHeader.cbDstLengthUsed)
			{
				buf.resize(acmHeader.cbDstLengthUsed);
				std::memcpy(buf.data(), acmDstBuf.data(), acmHeader.cbDstLengthUsed);
				WriteBuffer();
			}
			if(acmHeader.cbSrcLengthUsed < acmSrcBuf.size())
			{
				samples.resize((acmSrcBuf.size() - acmHeader.cbSrcLengthUsed) / sizeof(int16));
				std::memcpy(samples.data(), &acmSrcBuf[acmHeader.cbSrcLengthUsed], acmSrcBuf.size() - acmHeader.cbSrcLengthUsed);
				while(samples.size() > 0)
				{
					acm_sampleBuf.push_front(samples.back());
					samples.pop_back();
				}
			}
		}
	}
	virtual ~MP3AcmStreamWriter()
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
			std::memcpy(acmSrcBuf.data(), samples.data(), samples.size() * sizeof(int16));
		}
		acmHeader.cbSrcLength = static_cast<DWORD>(samples.size() * sizeof(int16));
		acmHeader.cbSrcLengthUsed = 0;
		acmHeader.cbDstLength = static_cast<DWORD>(acmDstBuf.size());
		acmHeader.cbDstLengthUsed = 0;
		acmStreamConvert(acmStream, &acmHeader, ACM_STREAMCONVERTF_END);
		if(acmHeader.cbDstLengthUsed)
		{
			buf.resize(acmHeader.cbDstLengthUsed);
			std::memcpy(buf.data(), acmDstBuf.data(), acmHeader.cbDstLengthUsed);
			WriteBuffer();
		}
		// acmStreamUnprepareHeader demands original buffer sizes to be specified
		acmHeader.cbSrcLength = static_cast<DWORD>(acmSrcBuf.size());
		acmHeader.cbSrcLengthUsed = 0;
		acmHeader.cbDstLength = static_cast<DWORD>(acmDstBuf.size());
		acmHeader.cbDstLengthUsed = 0;
		std::fill(acmSrcBuf.begin(), acmSrcBuf.end(), BYTE(0));
		std::fill(acmDstBuf.begin(), acmDstBuf.end(), BYTE(0));
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
			std::memcpy(buf.data(), acmDstBuf.data(), acmHeader.cbDstLengthUsed);
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
{
	return false
#ifdef MPT_MP3ENCODER_LAME
		|| ((m_Type == MP3EncoderLame) && IsComponentAvailable(m_Lame))
		|| ((m_Type == MP3EncoderLameCompatible) && IsComponentAvailable(m_Lame))
#endif // MPT_MP3ENCODER_ACM
#ifdef MPT_MP3ENCODER_ACM
		|| ((m_Type == MP3EncoderACM) && IsComponentAvailable(m_Acm))
#endif // MPT_MP3ENCODER_ACM
		;
}


MP3Encoder::~MP3Encoder()
{
	return;
}


std::unique_ptr<IAudioStreamEncoder> MP3Encoder::ConstructStreamEncoder(std::ostream &file, const Encoder::Settings &settings, const FileTags &tags) const
{
	std::unique_ptr<IAudioStreamEncoder> result = nullptr;
	if(false)
	{
		// nothing
#ifdef MPT_MP3ENCODER_LAME
	} else if(m_Type == MP3EncoderLame || m_Type == MP3EncoderLameCompatible)
	{
		result = mpt::make_unique<MP3LameStreamWriter>(*m_Lame, file, (m_Type == MP3EncoderLameCompatible), settings, tags);
#endif // MPT_MP3ENCODER_LAME
#ifdef MPT_MP3ENCODER_ACM
	} else if(m_Type == MP3EncoderACM)
	{
		result = mpt::make_unique<MP3AcmStreamWriter>(*m_Acm, file, settings, tags);
#endif // MPT_MP3ENCODER_ACM
	}
	return std::move(result);
}


mpt::ustring MP3Encoder::DescribeQuality(float quality) const
{
#ifdef MPT_MP3ENCODER_LAME
	if(m_Type == MP3EncoderLame)
	{
		static const int q_table[11] = { 240, 220, 190, 170, 160, 130, 120, 100, 80, 70, 50 }; // http://wiki.hydrogenaud.io/index.php?title=LAME
		int q = Util::Round<int>((1.0f - quality) * 10.0f);
		if(q < 0) q = 0;
		if(q >= 10)
		{
			return mpt::format(MPT_USTRING("VBR -V%1 (~%2 kbit)"))(MPT_USTRING("9.999"), q_table[q]);
		} else
		{
			return mpt::format(MPT_USTRING("VBR -V%1 (~%2 kbit)"))(Util::Round<int>((1.0f - quality) * 10.0f), q_table[q]);
		}
	}
#endif // MPT_MP3ENCODER_LAME
	return EncoderFactoryBase::DescribeQuality(quality);
}

mpt::ustring MP3Encoder::DescribeBitrateABR(int bitrate) const
{
	return EncoderFactoryBase::DescribeBitrateABR(bitrate);
}



OPENMPT_NAMESPACE_END
