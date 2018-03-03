#pragma once



//! Container of ReplayGain scan results from one or more tracks.
class replaygain_result : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE(replaygain_result, service_base);
public:
	//! Retrieves the gain value, in dB.
	virtual float get_gain() = 0;
	//! Retrieves the peak value, normalized to 0..1 range (audio_sample value).
	virtual float get_peak() = 0;
	//! Merges ReplayGain scan results from different tracks. Merge results from all tracks in an album to get album gain/peak values. \n
	//! This function returns a newly created replaygain_result object. Existing replaygain_result objects remain unaltered.
	virtual replaygain_result::ptr merge(replaygain_result::ptr other) = 0;

	replaygain_info make_track_info() {
		replaygain_info ret = replaygain_info_invalid; ret.m_track_gain = this->get_gain(); ret.m_track_peak = this->get_peak(); return ret;
	}
};

//! Instance of a ReplayGain scanner. \n
//! Use replaygain_scanner_entry::instantiate() to create a replaygain_scanner object; see replaygain_scanner_entry for more info. \n
//! Typical use: call process_chunk() with each chunk read from your track, call finalize() to obtain results for this track and reset replaygain_scanner's state for scanning another track; to obtain album gain/peak values, merge results (replaygain_result::merge) from all tracks. \n
class replaygain_scanner : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE(replaygain_scanner, service_base);
public:
	//! Processes a PCM chunk. \n
	//! May throw exception_io_data if the chunk contains something that can't be processed properly.
	virtual void process_chunk(const audio_chunk & chunk) = 0;
	//! Finalizes the scanning process; resets scanner's internal state and returns results for the track we've just scanned. \n
	//! After calling finalize(), scanner's state becomes the same as after instantiation; you can continue with processing another track without creating a new scanner object.
	virtual replaygain_result::ptr finalize() = 0;
};


//! Entrypoint class for instantiating replaygain_scanner objects. \n
//! This service is OPTIONAL; it's available from foobar2000 0.9.5.3 up but only if the ReplayGain Scanner component is installed. \n
//! It is recommended that you use replaygain_scanner like this: \n
//! replaygain_scanner_entry::ptr theAPI; \n
//! if (replaygain_scanner_entry::tryGet(theAPI)) { \n
//!     myInstance = theAPI->instantiate(); \n
//! } else { \n
//!     no foo_rgscan installed - complain/fail/etc \n
//! } \n
//! Note that replaygain_scanner_entry::get() is provided for convenience - it WILL crash with no foo_rgscan present. Use it only after prior checks.
class replaygain_scanner_entry : public service_base {
	FB2K_MAKE_SERVICE_COREAPI(replaygain_scanner_entry);
public:
	//! Instantiates a replaygain_scanner object.
	virtual replaygain_scanner::ptr instantiate() = 0;
};

//! \since 1.4
//! This service is OPTIONAL; it's available from foobar2000 v1.4 up but only if the ReplayGain Scanner component is installed. \n
//! Use tryGet() to instantiate - get() only after prior verification of availability.
class replaygain_scanner_entry_v2 : public replaygain_scanner_entry {
	FB2K_MAKE_SERVICE_COREAPI_EXTENSION(replaygain_scanner_entry_v2, replaygain_scanner_entry)
public:
	enum { 
		flagScanPeak = 1 << 0,
		flagScanGain = 1 << 1,
	};

	//! Extended instantiation method. \n
	//! Allows you to declare which parts of the scanning process are relevant for you
	//! so irrelevant parts of the processing can be skipped.
	//! For an example, if you don't care about the peak, specify only flagScanGain -
	//! as peak scan while normally cheap may be very expensive with extreme oversampling specified by user.
	virtual replaygain_scanner::ptr instantiate(uint32_t flags) = 0;
};

#ifdef FOOBAR2000_DESKTOP
//! \since 1.4
//! A class for applying gain to compressed audio packets such as MP3 or AAC. \n
//! Implemented by foo_rgscan for common formats. May be extended to allow foo_rgscan to manipulate more different codecs.
class replaygain_alter_stream : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE(replaygain_alter_stream, service_base)
public:
	//! @returns The amount to which all adjustments are quantized for this format. Essential for caller to be able to correctly prevent clipping.
	virtual float get_adjustment_step( ) = 0;
	//! Sets the adjustment in decibels. Note that the actual applied adjustment will be quantized with nearest-rounding to a multiple of get_adjustment_step() value.
	virtual void set_adjustment( float deltaDB ) = 0;
	//! Passes the first frame playload. This serves as a hint and may be safely ignored for most formats. However in some cases - MP3 vs MP2 in particular - you do knot know what exact format you're dealing with until you've examined the first frame. \n
	//! If you're calling this service, always feed the first frame before calling get_adjustment_step().
	virtual void on_first_frame( const void * frame, size_t bytes ) = 0;
	//! Applies gain to the frame. \n
	//! May throw exception_io_data if the frame payload is corrupted and cannot be altered. The user will be informed about bad frame statistics, however the operation will continue until EOF.
	virtual void alter_frame( void * frame, size_t bytes ) = 0;
};

//! \since 1.4
//! Entrypoint class for instantiating replaygain_alter_stream. Walk with service_enum_t<> to find one that supports specific format.
class replaygain_alter_stream_entry : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(replaygain_alter_stream_entry);
public:
	//! @returns Newly created replaygain_alter_stream object. Null if this format is not supported by this implementation.
	//! Arguments as per packet_decoder::g_open().
	virtual replaygain_alter_stream::ptr open(const GUID & p_owner, size_t p_param1, const void * p_param2, size_t p_param2size ) = 0;
};
#endif
