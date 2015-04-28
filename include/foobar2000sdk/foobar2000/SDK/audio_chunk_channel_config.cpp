#include "foobar2000.h"

#ifdef _WIN32
#include <ks.h>
#include <ksmedia.h>

#if 0
#define SPEAKER_FRONT_LEFT             0x1
#define SPEAKER_FRONT_RIGHT            0x2
#define SPEAKER_FRONT_CENTER           0x4
#define SPEAKER_LOW_FREQUENCY          0x8
#define SPEAKER_BACK_LEFT              0x10
#define SPEAKER_BACK_RIGHT             0x20
#define SPEAKER_FRONT_LEFT_OF_CENTER   0x40
#define SPEAKER_FRONT_RIGHT_OF_CENTER  0x80
#define SPEAKER_BACK_CENTER            0x100
#define SPEAKER_SIDE_LEFT              0x200
#define SPEAKER_SIDE_RIGHT             0x400
#define SPEAKER_TOP_CENTER             0x800
#define SPEAKER_TOP_FRONT_LEFT         0x1000
#define SPEAKER_TOP_FRONT_CENTER       0x2000
#define SPEAKER_TOP_FRONT_RIGHT        0x4000
#define SPEAKER_TOP_BACK_LEFT          0x8000
#define SPEAKER_TOP_BACK_CENTER        0x10000
#define SPEAKER_TOP_BACK_RIGHT         0x20000

static struct {DWORD m_wfx; unsigned m_native; } const g_translation_table[] =
{
	{SPEAKER_FRONT_LEFT,			audio_chunk::channel_front_left},
	{SPEAKER_FRONT_RIGHT,			audio_chunk::channel_front_right},
	{SPEAKER_FRONT_CENTER,			audio_chunk::channel_front_center},
	{SPEAKER_LOW_FREQUENCY,			audio_chunk::channel_lfe},
	{SPEAKER_BACK_LEFT,				audio_chunk::channel_back_left},
	{SPEAKER_BACK_RIGHT,			audio_chunk::channel_back_right},
	{SPEAKER_FRONT_LEFT_OF_CENTER,	audio_chunk::channel_front_center_left},
	{SPEAKER_FRONT_RIGHT_OF_CENTER,	audio_chunk::channel_front_center_right},
	{SPEAKER_BACK_CENTER,			audio_chunk::channel_back_center},
	{SPEAKER_SIDE_LEFT,				audio_chunk::channel_side_left},
	{SPEAKER_SIDE_RIGHT,			audio_chunk::channel_side_right},
	{SPEAKER_TOP_CENTER,			audio_chunk::channel_top_center},
	{SPEAKER_TOP_FRONT_LEFT,		audio_chunk::channel_top_front_left},
	{SPEAKER_TOP_FRONT_CENTER,		audio_chunk::channel_top_front_center},
	{SPEAKER_TOP_FRONT_RIGHT,		audio_chunk::channel_top_front_right},
	{SPEAKER_TOP_BACK_LEFT,			audio_chunk::channel_top_back_left},
	{SPEAKER_TOP_BACK_CENTER,		audio_chunk::channel_top_back_center},
	{SPEAKER_TOP_BACK_RIGHT,		audio_chunk::channel_top_back_right},
};

#endif
#endif

// foobar2000 channel flags are 1:1 identical to Windows WFX ones.
uint32_t audio_chunk::g_channel_config_to_wfx(unsigned p_config)
{
    return p_config;
#if 0
	DWORD ret = 0;
	unsigned n;
	for(n=0;n<PFC_TABSIZE(g_translation_table);n++)
	{
		if (p_config & g_translation_table[n].m_native) ret |= g_translation_table[n].m_wfx;
	}
	return ret;
#endif
}

unsigned audio_chunk::g_channel_config_from_wfx(uint32_t p_wfx)
{
    return p_wfx;
#if 0
	unsigned ret = 0;
	unsigned n;
	for(n=0;n<PFC_TABSIZE(g_translation_table);n++)
	{
		if (p_wfx & g_translation_table[n].m_wfx) ret |= g_translation_table[n].m_native;
	}
	return ret;
#endif
}


static const unsigned g_audio_channel_config_table[] = 
{
	0,
	audio_chunk::channel_config_mono,
	audio_chunk::channel_config_stereo,
	audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_lfe,
	audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_back_left | audio_chunk::channel_back_right,
	audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_back_left | audio_chunk::channel_back_right | audio_chunk::channel_lfe,
	audio_chunk::channel_config_5point1,
	audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_back_left | audio_chunk::channel_back_right | audio_chunk::channel_lfe | audio_chunk::channel_front_center_right | audio_chunk::channel_front_center_left,
	audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_back_left | audio_chunk::channel_back_right | audio_chunk::channel_front_center | audio_chunk::channel_lfe | audio_chunk::channel_front_center_right | audio_chunk::channel_front_center_left,
};

static const unsigned g_audio_channel_config_table_xiph[] = 
{
	0,
	audio_chunk::channel_config_mono,
	audio_chunk::channel_config_stereo,
	audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_front_center,
	audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_back_left | audio_chunk::channel_back_right,
	audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_back_left | audio_chunk::channel_back_right | audio_chunk::channel_front_center,
	audio_chunk::channel_config_5point1,
	audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_front_center | audio_chunk::channel_lfe | audio_chunk::channel_back_center | audio_chunk::channel_side_left | audio_chunk::channel_side_right,
	audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_front_center | audio_chunk::channel_lfe | audio_chunk::channel_back_left | audio_chunk::channel_back_right | audio_chunk::channel_side_left | audio_chunk::channel_side_right,
};

unsigned audio_chunk::g_guess_channel_config(unsigned count)
{
	if (count >= PFC_TABSIZE(g_audio_channel_config_table)) return 0;
	return g_audio_channel_config_table[count];
}

unsigned audio_chunk::g_guess_channel_config_xiph(unsigned count) {
	if (count == 0 || count >= PFC_TABSIZE(g_audio_channel_config_table_xiph)) throw exception_io_data();
	return g_audio_channel_config_table_xiph[count];
}

unsigned audio_chunk::g_channel_index_from_flag(unsigned p_config,unsigned p_flag) {
	unsigned index = 0;
	for(unsigned walk = 0; walk < 32; walk++) {
		unsigned query = 1 << walk;
		if (p_flag & query) return index;
		if (p_config & query) index++;
	}
	return ~0;
}

unsigned audio_chunk::g_extract_channel_flag(unsigned p_config,unsigned p_index)
{
	unsigned toskip = p_index;
	unsigned flag = 1;
	while(flag)
	{
		if (p_config & flag)
		{
			if (toskip == 0) break;
			toskip--;
		}
		flag <<= 1;
	}
	return flag;
}

unsigned audio_chunk::g_count_channels(unsigned p_config)
{
	return pfc::countBits32(p_config);
}

static const char * const chanNames[] = {
	"FL", //channel_front_left			= 1<<0,
	"FR", //channel_front_right			= 1<<1,
	"FC", //channel_front_center		= 1<<2,
	"LFE", //channel_lfe					= 1<<3,
	"BL", //channel_back_left			= 1<<4,
	"BR", //channel_back_right			= 1<<5,
	"FCL", //channel_front_center_left	= 1<<6,
	"FCR", //channel_front_center_right	= 1<<7,
	"BC", //channel_back_center			= 1<<8,
	"SL", //channel_side_left			= 1<<9,
	"SR", //channel_side_right			= 1<<10,
	"TC", //channel_top_center			= 1<<11,
	"TFL", //channel_top_front_left		= 1<<12,
	"TFC", //channel_top_front_center	= 1<<13,
	"TFR", //channel_top_front_right		= 1<<14,
	"TBL", //channel_top_back_left		= 1<<15,
	"TBC", //channel_top_back_center		= 1<<16,
	"TBR", //channel_top_back_right		= 1<<17,
};

unsigned audio_chunk::g_find_channel_idx(unsigned p_flag) {
	unsigned rv = 0;
	if ((p_flag & 0xFFFF) == 0) {
		rv += 16; p_flag >>= 16;
	}
	if ((p_flag & 0xFF) == 0) {
		rv += 8; p_flag >>= 8;
	}
	if ((p_flag & 0xF) == 0) {
		rv += 4; p_flag >>= 4;
	}
	if ((p_flag & 0x3) == 0) {
		rv += 2; p_flag >>= 2;
	}
	if ((p_flag & 0x1) == 0) {
		rv += 1; p_flag >>= 1;
	}
	PFC_ASSERT( p_flag & 1 );
	return rv;
}

const char * audio_chunk::g_channel_name(unsigned p_flag) {
	return g_channel_name_byidx(g_find_channel_idx(p_flag));
}

const char * audio_chunk::g_channel_name_byidx(unsigned p_index) {
	if (p_index < PFC_TABSIZE(chanNames)) return chanNames[p_index];
	else return "?";
}

void audio_chunk::g_formatChannelMaskDesc(unsigned flags, pfc::string_base & out) {
	out.reset();
	unsigned idx = 0;
	while(flags) {
		if (flags & 1) {
			if (!out.is_empty()) out << " ";
			out << g_channel_name_byidx(idx);
		}
		flags >>= 1;
		++idx;
	}
}