/*
 * tagging.h
 * ---------
 * Purpose: ID3v2.4 / RIFF tags tagging class (for mp3 / wav / etc. support)
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include <string>
#include "Wav.h"

using std::string;

#pragma pack(push, 1)

///////////////////////////////////////////////////////////////////////////////////////////////////
// ID3v1 Genres

#define NUM_GENRES		148

static LPCSTR gpszGenreNames[NUM_GENRES] =
{
	"Blues",                "Classic Rock",     "Country",          "Dance",
	"Disco",                "Funk",             "Grunge",           "Hip-Hop",
	"Jazz",                 "Metal",            "New Age",          "Oldies",
	"Other",                "Pop",              "R&B",              "Rap",
	"Reggae",               "Rock",             "Techno",           "Industrial",
	"Alternative",          "Ska",              "Death Metal",      "Pranks",
	"Soundtrack",           "Euro-Techno",      "Ambient",          "Trip-Hop",
	"Vocal",                "Jazz+Funk",        "Fusion",           "Trance",
	"Classical",            "Instrumental",     "Acid",             "House",
	"Game",                 "Sound Clip",       "Gospel",           "Noise",
	"Alt. Rock",            "Bass",             "Soul",             "Punk",
	"Space",                "Meditative",       "Instrumental Pop", "Instrumental Rock",
	"Ethnic",               "Gothic",           "Darkwave",         "Techno-Industrial",
	"Electronic",           "Pop-Folk",         "Eurodance",        "Dream",
	"Southern Rock",        "Comedy",           "Cult",             "Gangsta",
	"Top 40",               "Christian Rap",    "Pop/Funk",         "Jungle",
	"Native American",      "Cabaret",          "New Wave",         "Psychadelic",
	"Rave",                 "Showtunes",        "Trailer",          "Lo-Fi",
	"Tribal",               "Acid Punk",        "Acid Jazz",        "Polka",
	"Retro",                "Musical",          "Rock & Roll",      "Hard Rock",
	"Folk",                 "Folk-Rock",        "National Folk",    "Swing",
	"Fast Fusion",          "Bebob",            "Latin",            "Revival",
	"Celtic",               "Bluegrass",        "Avantgarde",       "Gothic Rock",
	"Progressive Rock",     "Psychedelic Rock", "Symphonic Rock",   "Slow Rock",
	"Big Band",             "Chorus",           "Easy Listening",   "Acoustic",
	"Humour",               "Speech",           "Chanson",          "Opera",
	"Chamber Music",        "Sonata",           "Symphony",         "Booty Bass",
	"Primus",               "Porn Groove",      "Satire",           "Slow Jam",
	"Club",                 "Tango",            "Samba",            "Folklore",
	"Ballad",               "Power Ballad",     "Rhythmic Soul",    "Freestyle",
	"Duet",                 "Punk Rock",        "Drum Solo",        "A Capella",
	"Euro-House",           "Dance Hall",       "Goa",              "Drum & Bass",
	"Club House",           "Hardcore",         "Terror",           "Indie",
	"BritPop",              "Negerpunk",        "Polsk Punk",       "Beat",
	"Christian Gangsta Rap","Heavy Metal",      "Black Metal",      "Crossover",
	"Contemporary Christian", "Christian Rock", "Merengue",         "Salsa",
	"Thrash Metal",         "Anime",            "JPop",             "Synthpop"
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// ID3v2.4 Tags

typedef struct _TAGID3v2HEADER
{
	uint8 signature[3];
	uint8 version[2];
	uint8 flags;
	uint32 size;
	// Total: 10 bytes
} TAGID3v2HEADER;

typedef struct _TAGID3v2FRAME
{
	uint32 frameid;
	uint32 size;
	uint16 flags;
	// Total: 10 bytes
} TAGID3v2FRAME;

// we will add some padding bytes to our id3v2 tag (extending tags will be easier this way)
#define ID3v2_PADDING 512

// charset... choose text ending accordingly.
// $00 = ISO-8859-1. Terminated with $00.
// $01 = UTF-16 with BOM. Terminated with $00 00.
// $02 = UTF-16BE without BOM. Terminated with $00 00.
// $03 = UTF-8. Terminated with $00.
#ifdef UNICODE
#define ID3v2_CHARSET '\3'
#define ID3v2_TEXTENDING '\0'
#else
#define ID3v2_CHARSET '\0'
#define ID3v2_TEXTENDING '\0'
#endif

//================
class CFileTagging
//================
{
public:
	// Write Tags
	void WriteID3v2Tags(FILE *f);
	void WriteWaveTags(WAVEDATAHEADER *wdh, WAVEFILEHEADER *wfh, FILE *f);

	// Tag data
	string title, artist, album, year, comments, genre, url, encoder, bpm;

	CFileTagging();


private:
	// Convert Integer to Synchsafe Integer (see ID3v2.4 specs)
	uint32 intToSynchsafe(uint32 in);
	// Write a frame
	void WriteID3v2Frame(char cFrameID[4], string sFramecontent, FILE *f);
	// Size of our tag
	uint32 totalID3v2Size;
};

#pragma pack(pop)
