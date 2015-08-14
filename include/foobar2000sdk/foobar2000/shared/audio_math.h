#ifndef audio_sample_size
#error PFC not included?
#endif

/*
PROBLEM: 
audio_math is implemented in pfc (pfc::audio_math) and in shared.dll (::audio_math)
We must overlay shared.dll methods on top of PFC ones
*/

namespace audio_math
{
	//! p_source/p_output can point to same buffer
	void SHARED_EXPORT scale(const audio_sample * p_source,t_size p_count,audio_sample * p_output,audio_sample p_scale);
	void SHARED_EXPORT convert_to_int16(const audio_sample * p_source,t_size p_count,t_int16 * p_output,audio_sample p_scale);
	void SHARED_EXPORT convert_to_int32(const audio_sample * p_source,t_size p_count,t_int32 * p_output,audio_sample p_scale);
	audio_sample SHARED_EXPORT convert_to_int16_calculate_peak(const audio_sample * p_source,t_size p_count,t_int16 * p_output,audio_sample p_scale);
	void SHARED_EXPORT convert_from_int16(const t_int16 * p_source,t_size p_count,audio_sample * p_output,audio_sample p_scale);
	void SHARED_EXPORT convert_from_int32(const t_int32 * p_source,t_size p_count,audio_sample * p_output,audio_sample p_scale);
	audio_sample SHARED_EXPORT convert_to_int32_calculate_peak(const audio_sample * p_source,t_size p_count,t_int32 * p_output,audio_sample p_scale);
	audio_sample SHARED_EXPORT calculate_peak(const audio_sample * p_source,t_size p_count);
	void SHARED_EXPORT kill_denormal(audio_sample * p_buffer,t_size p_count);
	void SHARED_EXPORT add_offset(audio_sample * p_buffer,audio_sample p_delta,t_size p_count);
}

namespace audio_math_shareddll = audio_math;
typedef pfc::audio_math audio_math_pfc;

// Overlay class, overrides specific pfc::audio_math methods
class fb2k_audio_math : public audio_math_pfc {
public:
	static void scale(const audio_sample * p_source,t_size p_count,audio_sample * p_output,audio_sample p_scale) {
		audio_math_shareddll::scale(p_source, p_count, p_output, p_scale);
	}
	static void convert_to_int16(const audio_sample * p_source,t_size p_count,t_int16 * p_output,audio_sample p_scale) {
		audio_math_shareddll::convert_to_int16(p_source, p_count, p_output, p_scale);
	}
	static void convert_to_int32(const audio_sample * p_source,t_size p_count,t_int32 * p_output,audio_sample p_scale) {
		audio_math_shareddll::convert_to_int32(p_source, p_count, p_output, p_scale);
	}
	static audio_sample convert_to_int16_calculate_peak(const audio_sample * p_source,t_size p_count,t_int16 * p_output,audio_sample p_scale) {
		return audio_math_shareddll::convert_to_int16_calculate_peak(p_source,p_count,p_output,p_scale);
	}
	static void convert_from_int16(const t_int16 * p_source,t_size p_count,audio_sample * p_output,audio_sample p_scale) {
		audio_math_shareddll::convert_from_int16(p_source,p_count,p_output,p_scale);
	}
	static void convert_from_int32(const t_int32 * p_source,t_size p_count,audio_sample * p_output,audio_sample p_scale) {
		audio_math_shareddll::convert_from_int32(p_source,p_count,p_output,p_scale);
	}
	static audio_sample convert_to_int32_calculate_peak(const audio_sample * p_source,t_size p_count,t_int32 * p_output,audio_sample p_scale) {
		return audio_math_shareddll::convert_to_int32_calculate_peak(p_source,p_count,p_output,p_scale);
	}
	static audio_sample calculate_peak(const audio_sample * p_source,t_size p_count) {
		return audio_math_shareddll::calculate_peak(p_source,p_count);
	}
	static void kill_denormal(audio_sample * p_buffer,t_size p_count) {
		audio_math_shareddll::kill_denormal(p_buffer, p_count);
	}
	static void add_offset(audio_sample * p_buffer,audio_sample p_delta,t_size p_count) {
		audio_math_shareddll::add_offset(p_buffer,p_delta,p_count);
	}
};

// Anyone trying to talk to audio_math namespace will reach fb2k_audio_math which calls the right thing
#define audio_math fb2k_audio_math
