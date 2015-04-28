#ifndef _FOOBAR2000_SDK_OUTPUT_H_
#define _FOOBAR2000_SDK_OUTPUT_H_

PFC_DECLARE_EXCEPTION(exception_output_device_not_found, pfc::exception, "Audio device not found")
PFC_DECLARE_EXCEPTION(exception_output_module_not_found, exception_output_device_not_found, "Output module not found")


// =======================================================
// IDEA BIN
// ========
// Accurate timing info required! get_latency NOT safe to call from any thread while it should be
// There should be a legitimate way ( as in other than matching get_latency() against the amount of sent data ) to know when the output has finished prebuffering and started actual playback
// Outputs should be able to handle idling : idle(abort_callback&) => while(!update()) aborter.sleep();  or optimized for specific output
// =======================================================

//! Structure describing PCM audio data format, with basic helper functions.
struct t_pcmspec
{
	inline t_pcmspec() {reset();}
	inline t_pcmspec(const t_pcmspec & p_source) {*this = p_source;}
	unsigned m_sample_rate;
	unsigned m_bits_per_sample;
	unsigned m_channels,m_channel_config;
	bool m_float;

	inline unsigned align() const {return (m_bits_per_sample / 8) * m_channels;}

	t_size time_to_bytes(double p_time) const {return (t_size)audio_math::time_to_samples(p_time,m_sample_rate) * (m_bits_per_sample / 8) * m_channels;}
	double bytes_to_time(t_size p_bytes) const {return (double) (p_bytes / ((m_bits_per_sample / 8) * m_channels)) / (double) m_sample_rate;}

	inline bool operator==(/*const t_pcmspec & p_spec1,*/const t_pcmspec & p_spec2) const
	{
		return /*p_spec1.*/m_sample_rate == p_spec2.m_sample_rate 
			&& /*p_spec1.*/m_bits_per_sample == p_spec2.m_bits_per_sample
			&& /*p_spec1.*/m_channels == p_spec2.m_channels
			&& /*p_spec1.*/m_channel_config == p_spec2.m_channel_config
			&& /*p_spec1.*/m_float == p_spec2.m_float;
	}

	inline bool operator!=(/*const t_pcmspec & p_spec1,*/const t_pcmspec & p_spec2) const
	{
		return !(*this == p_spec2);
	}

	inline void reset() {m_sample_rate = 0; m_bits_per_sample = 0; m_channels = 0; m_channel_config = 0; m_float = false;}
	inline bool is_valid() const
	{
		return m_sample_rate >= 1000 && m_sample_rate <= 1000000 &&
			m_channels > 0 && m_channels <= 256 && m_channel_config != 0 &&
			(m_bits_per_sample == 8 || m_bits_per_sample == 16 || m_bits_per_sample == 24 || m_bits_per_sample == 32);
	}
};

struct t_samplespec {
	t_uint32 m_sample_rate;
	t_uint32 m_channels,m_channel_config;

	t_size time_to_samples(double p_time) const {PFC_ASSERT(is_valid());return (t_size)audio_math::time_to_samples(p_time,m_sample_rate);}
	double samples_to_time(t_size p_samples) const {PFC_ASSERT(is_valid()); return audio_math::samples_to_time(p_samples,m_sample_rate);}

	inline t_samplespec() {reset();}
	inline t_samplespec(audio_chunk const & in) {fromchunk(in);}

	inline void reset() {m_sample_rate = 0; m_channels = 0; m_channel_config = 0;}

	inline bool operator==(const t_samplespec & p_spec2) const {
		return m_sample_rate == p_spec2.m_sample_rate && m_channels == p_spec2.m_channels && m_channel_config == p_spec2.m_channel_config;
	}

	inline bool operator!=(const t_samplespec & p_spec2) const {
		return !(*this == p_spec2);
	}

	inline bool is_valid() const {
		return m_sample_rate > 0 && m_channels > 0 && audio_chunk::g_count_channels(m_channel_config) == m_channels;
	}

	static t_samplespec g_fromchunk(const audio_chunk & p_chunk) {
		t_samplespec temp; temp.fromchunk(p_chunk); return temp;
	}

	void fromchunk(const audio_chunk & p_chunk) {
		m_sample_rate = p_chunk.get_sample_rate();
		m_channels = p_chunk.get_channels();
		m_channel_config = p_chunk.get_channel_config();
	}
};

class NOVTABLE output_device_enum_callback
{
public:
	virtual void on_device(const GUID & p_guid,const char * p_name,unsigned p_name_length) = 0;
};

class NOVTABLE output : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE(output,service_base);
public:
	//! Retrieves amount of audio data queued for playback, in seconds.
	virtual double get_latency() = 0;
	//! Sends new samples to the device. Allowed to be called only when update() indicates that the device is ready.
	virtual void process_samples(const audio_chunk & p_chunk) = 0;
	//! Updates playback; queries whether the device is ready to receive new data.
	//! @param p_ready On success, receives value indicating whether the device is ready for next process_samples() call.
	virtual void update(bool & p_ready) = 0;
	//! Pauses/unpauses playback.
	virtual void pause(bool p_state) = 0;
	//! Flushes queued audio data. Called after seeking.
	virtual void flush() = 0;
	//! Forces playback of queued data. Called when there's no more data to send, to prevent infinite waiting if output implementation starts actually playing after amount of data in internal buffer reaches some level.
	virtual void force_play() = 0;
	
	//! Sets playback volume.
	//! @p_val Volume level in dB. Value of 0 indicates full ("100%") volume, negative values indciate different attenuation levels.
	virtual void volume_set(double p_val) = 0;

};

class NOVTABLE output_v2 : public output {
	FB2K_MAKE_SERVICE_INTERFACE(output_v2, output);
public:
	virtual bool want_track_marks() {return false;}
	virtual void on_track_mark() {}
	virtual void enable_fading(bool state) {}
	virtual void flush_changing_track() {flush();}
};

class NOVTABLE output_entry : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(output_entry);
public:
	//! Instantiates output class.
	virtual void instantiate(service_ptr_t<output> & p_out,const GUID & p_device,double p_buffer_length,bool p_dither,t_uint32 p_bitdepth) = 0;
	//! Enumerates devices supported by this output_entry implementation.
	virtual void enum_devices(output_device_enum_callback & p_callback) = 0;
	//! For internal use by backend. Each output_entry implementation must have its own guid.
	virtual GUID get_guid() = 0;
	//! For internal use by backend. Retrieves human-readable name of this output_entry implementation.
	virtual const char * get_name() = 0;

	//! Pops up advanced settings dialog. This method is optional and not supported if get_config_flag() return value does not have flag_needs_advanced_config set.
	//! @param p_parent Parent window for the dialog.
	//! @param p_menupoint Point in screen coordinates - can be used to display a simple popup menu with options to be checked instead of a full dialog.
	virtual void advanced_settings_popup(HWND p_parent,POINT p_menupoint) = 0;

	enum {
		flag_needs_bitdepth_config = 1 << 0,
		flag_needs_dither_config = 1 << 1,
		flag_needs_advanced_config = 1 << 2,
		flag_needs_device_list_prefixes = 1 << 3,
	};

	virtual t_uint32 get_config_flags() = 0;

	pfc::string8 get_device_name( const GUID & deviceID);
	bool get_device_name( const GUID & deviceID, pfc::string_base & out );

	static bool g_find( const GUID & outputID, output_entry::ptr & outObj );
	static output_entry::ptr g_find(const GUID & outputID );
};

//! Helper; implements output_entry for specific output class implementation. output_entry methods are forwarded to static methods of your output class. Use output_factory_t<myoutputclass> instead of using this class directly.
template<typename T, typename E = output_entry>
class output_entry_impl_t : public E
{
public:
	void instantiate(service_ptr_t<output> & p_out,const GUID & p_device,double p_buffer_length,bool p_dither,t_uint32 p_bitdepth) {
		p_out = new service_impl_t<T>(p_device,p_buffer_length,p_dither,p_bitdepth);
	}
	void enum_devices(output_device_enum_callback & p_callback) {T::g_enum_devices(p_callback);}
	GUID get_guid() {return T::g_get_guid();}
	const char * get_name() {return T::g_get_name();}
	void advanced_settings_popup(HWND p_parent,POINT p_menupoint) {T::g_advanced_settings_popup(p_parent,p_menupoint);}
	
	t_uint32 get_config_flags() {
		t_uint32 flags = 0;
		if (T::g_advanced_settings_query()) flags |= output_entry::flag_needs_advanced_config;
		if (T::g_needs_bitdepth_config()) flags |= output_entry::flag_needs_bitdepth_config;
		if (T::g_needs_dither_config()) flags |= output_entry::flag_needs_dither_config;
		if (T::g_needs_device_list_prefixes()) flags |= output_entry::flag_needs_device_list_prefixes ;
		return flags;
	}
};


//! Use this to register your output implementation.
template<class T>
class output_factory_t : public service_factory_single_t<output_entry_impl_t<T> > {};

class output_impl : public output_v2 {
protected:
	output_impl() : m_incoming_ptr(0) {}
	virtual void on_update() = 0;
	//! Will never get more input than as returned by can_write_samples().
	virtual void write(const audio_chunk & p_data) = 0;
	virtual t_size can_write_samples() = 0;
	virtual t_size get_latency_samples() = 0;
	virtual void on_flush() = 0;
	virtual void on_flush_changing_track() {on_flush();}
	virtual void open(t_samplespec const & p_spec) = 0;
	
	virtual void pause(bool p_state) = 0;
	virtual void force_play() = 0;
	virtual void volume_set(double p_val) = 0;
protected:
	void on_need_reopen() {m_active_spec = t_samplespec();}
private:
	void flush();
	void flush_changing_track();
	void update(bool & p_ready);
	double get_latency();
	void process_samples(const audio_chunk & p_chunk);

	pfc::array_t<audio_sample,pfc::alloc_fast_aggressive> m_incoming;
	t_size m_incoming_ptr;
	t_samplespec m_incoming_spec,m_active_spec;
};


class NOVTABLE volume_callback {
public:
	virtual void on_volume_scale(float v) = 0;
	virtual void on_volume_arbitrary(int v) = 0;
};

class NOVTABLE volume_control : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE(volume_control, service_base)
public:
	virtual void add_callback(volume_callback * ptr) = 0;
	virtual void remove_callback(volume_callback * ptr) = 0;
	
	enum style_t {
		styleScale,
		styleArbitrary
	};

	virtual style_t getStyle() = 0;

	virtual float scaleGet() = 0;
	virtual void scaleSet(float v) = 0;

	virtual void arbitrarySet(int val) = 0;
	virtual int arbitraryGet() = 0;
	virtual int arbitraryGetMin() = 0;
	virtual int arbitraryGetMax() = 0;
	virtual bool arbitraryGetMute() = 0;
	virtual void arbitrarySetMute(bool val) = 0;
};


class NOVTABLE output_entry_v2 : public output_entry {
	FB2K_MAKE_SERVICE_INTERFACE(output_entry_v2, output_entry)
public:
	virtual bool get_volume_control(const GUID & id, volume_control::ptr & out) = 0;
	virtual bool hasVisualisation() = 0;
};

#pragma pack(push, 1)
//! \since 1.3.5
struct outputCoreConfig_t {

	static outputCoreConfig_t defaults();

	GUID m_output;
	GUID m_device;
	double m_buffer_length;
	uint32_t m_flags;
	uint32_t m_bitDepth;
	enum { flagUseDither = 1 << 0 };
};
#pragma pack(pop)

//! \since 1.3.5
//! Allows components to access foobar2000 core's output settings.
class NOVTABLE output_manager : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(output_manager);
public:
	//! Instantiates an output instance with core settings.
	//! @param overrideBufferLength Specify non zero to override user-configured buffer length in core settings.
	//! @returns The new output instance. Throws exceptions on failure (invalid settings or other).
	virtual output::ptr instantiateCoreDefault(double overrideBufferLength = 0) = 0;
	virtual void getCoreConfig( void * out, size_t outSize ) = 0;

	void getCoreConfig(outputCoreConfig_t & out ) { getCoreConfig(&out, sizeof(out) ); }
};


extern const GUID output_id_null;
extern const GUID output_id_default;

#endif //_FOOBAR2000_SDK_OUTPUT_H_
