#include "stdafx.h"


static const GUID guid_RIFF = pfc::GUID_from_text("66666972-912E-11CF-A5D6-28DB04C10000");
static const GUID guid_WAVE = pfc::GUID_from_text("65766177-ACF3-11D3-8CD1-00C04F8EDB8A");
static const GUID guid_FMT  = pfc::GUID_from_text("20746D66-ACF3-11D3-8CD1-00C04F8EDB8A");
static const GUID guid_DATA = pfc::GUID_from_text("61746164-ACF3-11D3-8CD1-00C04F8EDB8A");

struct RIFF_chunk_desc {
	GUID m_guid;
	const char * m_name;
};

static const RIFF_chunk_desc RIFF_chunks[] = {
	{guid_RIFF, "RIFF"},
	{guid_WAVE, "WAVE"},
	{guid_FMT , "fmt "},
	{guid_DATA, "data"},
};

bool wavWriterSetup_t::needWFXE() const {
	if (this->m_bpsValid != this->m_bps) return true;
	switch(m_channels)
	{
	case 1:
		return m_channel_mask != audio_chunk::channel_config_mono;
	case 2:
		return m_channel_mask != audio_chunk::channel_config_stereo;
/*	case 4:
		m_wfxe = m_setup.m_channel_mask != (audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_back_left | audio_chunk::channel_back_right);
		break;
	case 6:
		m_wfxe = m_setup.m_channel_mask != audio_chunk::channel_config_5point1;
		break;*/
	default:
		return true;
	}

}

void wavWriterSetup_t::initialize3(const audio_chunk::spec_t & spec, unsigned bps, unsigned bpsValid, bool bFloat, bool bDither, bool bWave64) {
    m_bps = bps;
    m_bpsValid = bpsValid;
    m_samplerate = spec.sampleRate;
    m_channels = spec.chanCount;
    m_channel_mask = spec.chanMask;
    m_float = bFloat;
    m_dither = bDither;
    m_wave64 = bWave64;
}

void wavWriterSetup_t::initialize2(const audio_chunk & p_chunk, unsigned p_bps, unsigned p_bpsValid, bool p_float, bool p_dither, bool p_wave64) {
	m_bps = p_bps;
	m_bpsValid = p_bpsValid;
	m_samplerate = p_chunk.get_srate();
	m_channels = p_chunk.get_channels();
	m_channel_mask = p_chunk.get_channel_config();
	m_float = p_float;
	m_dither = p_dither;
	m_wave64 = p_wave64;
}

void wavWriterSetup_t::initialize(const audio_chunk & p_chunk, unsigned p_bps, bool p_float, bool p_dither, bool p_wave64)
{
	unsigned bpsValid = p_bps;
	unsigned bps = (p_bps + 7) & ~7;
	initialize2(p_chunk, bps, bpsValid, p_float, p_dither, p_wave64);
}

void wavWriterSetup_t::setup_wfx(WAVEFORMATEX & p_wfx)
{
	p_wfx.wFormatTag = m_float ? WAVE_FORMAT_IEEE_FLOAT : WAVE_FORMAT_PCM;
	p_wfx.nChannels = m_channels;
	p_wfx.nSamplesPerSec = m_samplerate;
	p_wfx.nAvgBytesPerSec = (m_bps >> 3) * m_channels * m_samplerate;
	p_wfx.nBlockAlign = (m_bps>>3) * m_channels;
	p_wfx.wBitsPerSample = m_bps;
	p_wfx.cbSize = 0;
}

void wavWriterSetup_t::setup_wfxe(WAVEFORMATEXTENSIBLE & p_wfxe)
{
	setup_wfx(p_wfxe.Format);
	p_wfxe.Format.wFormatTag=WAVE_FORMAT_EXTENSIBLE;
	p_wfxe.Format.cbSize=22;
	p_wfxe.Samples.wValidBitsPerSample = this->m_bpsValid;
	p_wfxe.dwChannelMask = audio_chunk::g_channel_config_to_wfx(m_channel_mask);
	p_wfxe.SubFormat = m_float ? KSDATAFORMAT_SUBTYPE_IEEE_FLOAT : KSDATAFORMAT_SUBTYPE_PCM;	
	
}

void CWavWriter::writeID(const GUID & id, abort_callback & abort) {
	if (is64()) {
		m_file->write_object_t(id, abort);
	} else {
		for(t_size walk = 0; walk < PFC_TABSIZE(RIFF_chunks); ++walk) {
			if (id == RIFF_chunks[walk].m_guid) {
				m_file->write(RIFF_chunks[walk].m_name, 4, abort); return;
			}
		}
		uBugCheck();
	}
}

void CWavWriter::writeSize(t_uint64 size, abort_callback & abort) {
	if (is64()) {
		if (size != ~0) size += 24;
		m_file->write_lendian_t(size, abort);
	} else {
		t_uint32 clipped;
		if (size > 0xFFFFFFFF) clipped = 0xFFFFFFFF;
		else clipped = (t_uint32) size;
		m_file->write_lendian_t(clipped, abort);
	}
}

size_t CWavWriter::align(abort_callback & abort) {
	t_uint8 dummy[8] = {};
	const t_uint32 val = is64() ? 8 : 2;
	t_filesize pos = m_file->get_position(abort);
	t_size delta = (val - (pos%val)) % val;
	if (delta > 0) m_file->write(dummy, delta, abort);	
	return delta;
}

void CWavWriter::open(const char * p_path, const wavWriterSetup_t & p_setup, abort_callback & p_abort)
{
	service_ptr_t<file> l_file;
	filesystem::g_open_write_new(l_file,p_path,p_abort);
	open(l_file,p_setup,p_abort);
}

namespace {
PFC_DECLARE_EXCEPTION(exceptionBadBitDepth, exception_io_data, "Invalid bit depth specified");
}
void CWavWriter::open(service_ptr_t<file> p_file, const wavWriterSetup_t & p_setup, abort_callback & p_abort)
{
	m_file = p_file;
	m_setup = p_setup;

	if (m_setup.m_channels == 0 || m_setup.m_channels > 18 || m_setup.m_channels != audio_chunk::g_count_channels(m_setup.m_channel_mask)) throw exception_io_data();

	if (!audio_chunk::g_is_valid_sample_rate(m_setup.m_samplerate)) throw exception_io_data();

	if (m_setup.m_bpsValid > m_setup.m_bps) throw exceptionBadBitDepth();

	if (m_setup.m_float)
	{
		if (m_setup.m_bps != 32 && m_setup.m_bps != 64) throw exceptionBadBitDepth();
		if (m_setup.m_bpsValid != m_setup.m_bps) throw exceptionBadBitDepth();
	}
	else
	{
		if (m_setup.m_bps != 8 && m_setup.m_bps != 16 && m_setup.m_bps != 24 && m_setup.m_bps != 32) throw exceptionBadBitDepth();
		if (m_setup.m_bpsValid < 1) throw exceptionBadBitDepth();
	}

	m_wfxe = m_setup.needWFXE();

	writeID(guid_RIFF, p_abort);
	m_offset_fix1 = m_file->get_position(p_abort);
	writeSize(~0, p_abort);
	
	writeID(guid_WAVE, p_abort);
	
	writeID(guid_FMT, p_abort);
	if (m_wfxe) {
		writeSize(sizeof(WAVEFORMATEXTENSIBLE),p_abort);

		WAVEFORMATEXTENSIBLE wfxe;
		m_setup.setup_wfxe(wfxe);
		m_file->write_object(&wfxe,sizeof(wfxe),p_abort);
	} else {
		writeSize(sizeof(PCMWAVEFORMAT),p_abort);

		WAVEFORMATEX wfx;
		m_setup.setup_wfx(wfx);
		m_file->write_object(&wfx,/* blah */ sizeof(PCMWAVEFORMAT),p_abort);
	}
	align(p_abort);

	writeID(guid_DATA, p_abort);
	m_offset_fix2 = m_file->get_position(p_abort);
	writeSize(~0, p_abort);
	m_offset_fix1_delta = m_file->get_position(p_abort) - chunkOverhead();


	m_bytes_written = 0;

	if (!m_setup.m_float)
	{
		m_postprocessor = standard_api_create_t<audio_postprocessor>();
	}
}

void CWavWriter::write(const audio_chunk & p_chunk, abort_callback & p_abort)
{
	if (p_chunk.get_channels() != m_setup.m_channels 
		|| p_chunk.get_channel_config() != m_setup.m_channel_mask
		|| p_chunk.get_srate() != m_setup.m_samplerate
		) throw exception_unexpected_audio_format_change();

	
	if (m_setup.m_float)
	{
		switch(m_setup.m_bps)
		{
		case 32:
			{
#if audio_sample_size == 32
				t_size bytes = p_chunk.get_sample_count() * p_chunk.get_channels() * sizeof(audio_sample);
				m_file->write_object(p_chunk.get_data(),bytes,p_abort);
				m_bytes_written += bytes;
#else
				enum {tempsize = 256};
				float temp[tempsize];
				t_size todo = p_chunk.get_sample_count() * p_chunk.get_channels();
				const audio_sample * readptr = p_chunk.get_data();
				while(todo > 0)
				{
					unsigned n,delta = todo;
					if (delta > tempsize) delta = tempsize;
                    for(n=0;n<delta;n++)
						temp[n] = (float)(*(readptr++));
					unsigned bytes = delta * sizeof(float);
					m_file->write_object_e(temp,bytes,p_abort);
					m_bytes_written += bytes;
					todo -= delta;
				}
#endif
			}
			break;
#if 0
		case 64:
			{
				unsigned bytes = p_chunk.get_sample_count() * p_chunk.get_channels() * sizeof(audio_sample);
				m_file->write_object_e(p_chunk.get_data(),bytes,p_abort);
				m_bytes_written += bytes;
			}
			break;
#endif
		default:
			throw exception_io_data();
		}
	}
	else
	{
		m_postprocessor->run(p_chunk,m_postprocessor_output,m_setup.m_bpsValid,m_setup.m_bps,m_setup.m_dither,1.0f);
		m_file->write_object(m_postprocessor_output.get_ptr(),m_postprocessor_output.get_size(),p_abort);
		m_bytes_written += m_postprocessor_output.get_size();
	}
}

void CWavWriter::finalize(abort_callback & p_abort)
{
	if (m_file.is_valid())
	{
		const size_t alignG = align(p_abort);

		if (m_file->can_seek()) {
			m_file->seek(m_offset_fix1,p_abort);
			writeSize(m_bytes_written + alignG + m_offset_fix1_delta, p_abort);
			m_file->seek(m_offset_fix2,p_abort);
			writeSize(m_bytes_written, p_abort);
		}
		m_file.release();
	}
	m_postprocessor.release();
}

void CWavWriter::close()
{
	m_file.release();
	m_postprocessor.release();
}

audio_chunk::spec_t CWavWriter::get_spec() const {
	audio_chunk::spec_t spec = {};
	spec.sampleRate = m_setup.m_samplerate;
	spec.chanCount = m_setup.m_channels;
	spec.chanMask = m_setup.m_channel_mask;
	return spec;
}
