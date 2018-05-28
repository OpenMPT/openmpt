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
#include "../common/mptStringBuffer.h"

#include <deque>

// For ACM debugging purposes
#define ACMLOG(x, ...)
//#define ACMLOG Log

#ifdef MPT_WITH_LAME
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
#endif // MPT_WITH_LAME



OPENMPT_NAMESPACE_BEGIN



///////////////////////////////////////////////////////////////////////////////////////////////////
// ID3v2.4 Tags

struct ID3v2Header
{
	uint8    signature[3];
	uint8    version[2];
	uint8be  flags;
	uint32be size;
};

MPT_BINARY_STRUCT(ID3v2Header, 10)

struct ID3v2Frame
{
	char     frameid[4];
	uint32be size;
	uint16be flags;
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
	void WriteID3v2Frame(const char cFrameID[4], std::string sFramecontent, std::ostream &s);
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
void ID3V2Tagger::WriteID3v2Frame(const char cFrameID[4], std::string sFramecontent, std::ostream &s)
{
	if(!cFrameID[0] || sFramecontent.empty() || !s) return;

	if(!memcmp(cFrameID, "COMM", 4))
	{
		// English language for comments - no description following (hence the text ending nullchar(s))
		// For language IDs, see https://en.wikipedia.org/wiki/ISO-639-2
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




#ifdef MPT_WITH_LAME

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
	void WriteInterleaved(size_t count, const float *interleaved) override
	{
		if(!gfp_inited)
		{
			lame.lame_init_params(gfp);
			gfp_inited = true;
		}
		const int count_max = 0xffff;
		while(count > 0)
		{
			int count_chunk = mpt::clamp(mpt::saturate_cast<int>(count), int(0), count_max);
			buf.resize(count_chunk + (count_chunk+3)/4 + 7200);
			int result = 0;
			if(lame.lame_get_num_channels(gfp) == 1)
			{
				// lame always assumes stereo input with interleaved interface, so use non-interleaved for mono
				result = lame.lame_encode_buffer_ieee_float(gfp, interleaved, nullptr, count_chunk, mpt::byte_cast<unsigned char*>(buf.data()), mpt::saturate_cast<int>(buf.size()));
			} else
			{
				result = lame.lame_encode_buffer_interleaved_ieee_float(gfp, interleaved, count_chunk, mpt::byte_cast<unsigned char*>(buf.data()), mpt::saturate_cast<int>(buf.size()));
			}
			buf.resize((result >= 0) ? result : 0);
			if(result == -2)
			{
				throw std::bad_alloc();
			}
			WriteBuffer();
			count -= static_cast<size_t>(count_chunk);
		}
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
		buf.resize(lame.lame_encode_flush(gfp, mpt::byte_cast<unsigned char*>(buf.data()), mpt::saturate_cast<int>(buf.size())));
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

#endif // MPT_WITH_LAME



MP3Encoder::MP3Encoder(MP3EncoderType type)
	: m_Type(MP3EncoderDefault)
{
#ifdef MPT_WITH_LAME
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
#endif // MPT_WITH_LAME
}


bool MP3Encoder::IsAvailable() const
{
	return false
#ifdef MPT_WITH_LAME
		|| ((m_Type == MP3EncoderLame) && IsComponentAvailable(m_Lame))
		|| ((m_Type == MP3EncoderLameCompatible) && IsComponentAvailable(m_Lame))
#endif // MPT_WITH_LAME
		;
}


std::unique_ptr<IAudioStreamEncoder> MP3Encoder::ConstructStreamEncoder(std::ostream &file, const Encoder::Settings &settings, const FileTags &tags) const
{
	std::unique_ptr<IAudioStreamEncoder> result = nullptr;
	if(false)
	{
		// nothing
#ifdef MPT_WITH_LAME
	} else if(m_Type == MP3EncoderLame || m_Type == MP3EncoderLameCompatible)
	{
		result = mpt::make_unique<MP3LameStreamWriter>(*m_Lame, file, (m_Type == MP3EncoderLameCompatible), settings, tags);
#endif // MPT_WITH_LAME
	}
	return std::move(result);
}


mpt::ustring MP3Encoder::DescribeQuality(float quality) const
{
#ifdef MPT_WITH_LAME
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
			return mpt::format(MPT_USTRING("VBR -V%1 (~%2 kbit)"))(q, q_table[q]);
		}
	}
#endif // MPT_WITH_LAME
	return EncoderFactoryBase::DescribeQuality(quality);
}

mpt::ustring MP3Encoder::DescribeBitrateABR(int bitrate) const
{
	return EncoderFactoryBase::DescribeBitrateABR(bitrate);
}



OPENMPT_NAMESPACE_END
