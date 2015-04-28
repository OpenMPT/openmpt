#ifndef _foobar2000_wav_writer_h_
#define _foobar2000_wav_writer_h_

#ifdef _WIN32
#include <mmreg.h>
#endif

struct wavWriterSetup_t
{
	unsigned m_bps,m_bpsValid,m_samplerate,m_channels,m_channel_mask;
	bool m_float,m_dither, m_wave64;

    
	void initialize(const audio_chunk & p_chunk,unsigned p_bps,bool p_float,bool p_dither, bool p_wave64 = false);
	void initialize2(const audio_chunk & p_chunk,unsigned p_bps, unsigned p_bpsValid,bool p_float,bool p_dither, bool p_wave64 = false);
    void initialize3(const audio_chunk::spec_t & spec, unsigned bps, unsigned bpsValid, bool bFloat, bool bDither, bool bWave64 = false);
    
#ifdef _WAVEFORMATEX_
	void setup_wfx(WAVEFORMATEX & p_wfx);
#endif
#ifdef _WAVEFORMATEXTENSIBLE_
	void setup_wfxe(WAVEFORMATEXTENSIBLE & p_wfx);
#endif
	bool needWFXE() const;
};

class CWavWriter
{
public:
	void open(const char * p_path, const wavWriterSetup_t & p_setup, abort_callback & p_abort);
	void open(service_ptr_t<file> p_file, const wavWriterSetup_t & p_setup, abort_callback & p_abort);
	void write(const audio_chunk & p_chunk,abort_callback & p_abort);
	void finalize(abort_callback & p_abort);
	void close();
	bool is_open() const { return m_file.is_valid(); }
	audio_chunk::spec_t get_spec() const;
private:
	size_t align(abort_callback & abort);
	void writeSize(t_uint64 size, abort_callback & abort);
	bool is64() const {return m_setup.m_wave64;}
	t_uint32 chunkOverhead() const {return is64() ? 24 : 8;}
	t_uint32 idOverhead() const {return is64() ? 16 : 4;}
	void writeID(const GUID & id, abort_callback & abort);
	service_ptr_t<file> m_file;
	service_ptr_t<audio_postprocessor> m_postprocessor;
	wavWriterSetup_t m_setup;
	bool m_wfxe;
	t_uint64 m_offset_fix1,m_offset_fix2,m_offset_fix1_delta,m_bytes_written;
	mem_block_container_aligned_incremental_impl<16> m_postprocessor_output;
};

#endif //_foobar2000_wav_writer_h_