#ifndef audio_sample_size
#error PFC not included?
#endif

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
	void SHARED_EXPORT remove_denormals(audio_sample * p_buffer,t_size p_count);
	void SHARED_EXPORT add_offset(audio_sample * p_buffer,audio_sample p_delta,t_size p_count);

}