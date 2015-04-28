#include "pfc.h"

#ifndef _WIN32 // on Win32 these reside in shared.dll

static audio_sample noopt_calculate_peak(const audio_sample * p_src,t_size p_num)
{
	audio_sample peak = 0;
	t_size num = p_num;
	for(;num;num--)
	{
		audio_sample temp = (audio_sample)fabs(*(p_src++));
		if (temp>peak) peak = temp;
	}
	return peak;
}




static void noopt_convert_to_32bit(const audio_sample * p_source,t_size p_count,t_int32 * p_output,float p_scale)
{
	t_size num = p_count;
	for(;num;--num)
	{
		t_int64 val = audio_math::rint64( *(p_source++) * p_scale );
		if (val < -(t_int64)0x80000000) val = -(t_int64)0x80000000;
		else if (val > 0x7FFFFFFF) val = 0x7FFFFFFF;
		*(p_output++) = (t_int32) val;
	}
}

inline static void noopt_convert_to_16bit(const audio_sample * p_source,t_size p_count,t_int16 * p_output,float p_scale) {
	for(t_size n=0;n<p_count;n++) {
		*(p_output++) = (t_int16) pfc::clip_t(audio_math::rint32(*(p_source++)*p_scale),-0x8000,0x7FFF);
	}
}

inline static void noopt_convert_from_int16(const t_int16 * p_source,t_size p_count,audio_sample * p_output,float p_scale)
{
	t_size num = p_count;
	for(;num;num--)
		*(p_output++) = (audio_sample)*(p_source++) * p_scale;
}



inline static void noopt_convert_from_int32(const t_int32 * p_source,t_size p_count,audio_sample * p_output,float p_scale)
{
	t_size num = p_count;
	for(;num;num--)
		*(p_output++) = (audio_sample)*(p_source++) * p_scale;
}

inline static void noopt_scale(const audio_sample * p_source,t_size p_count,audio_sample * p_output,audio_sample p_scale)
{
	for(t_size n=0;n<p_count;n++)
		p_output[n] = p_source[n] * p_scale;
}


namespace audio_math {

	void scale(const audio_sample * p_source,t_size p_count,audio_sample * p_output,audio_sample p_scale)
	{
		noopt_scale(p_source,p_count,p_output,p_scale);
	}
	
	void convert_to_int16(const audio_sample * p_source,t_size p_count,t_int16 * p_output,audio_sample p_scale)
	{
		audio_sample scale = (audio_sample)(p_scale * 0x8000);
		noopt_convert_to_16bit(p_source,p_count,p_output,scale);
	}

	audio_sample convert_to_int16_calculate_peak(const audio_sample * p_source,t_size p_count,t_int16 * p_output,audio_sample p_scale)
	{
		//todo?
		convert_to_int16(p_source,p_count,p_output,p_scale);
		return p_scale * calculate_peak(p_source,p_count);
	}

	void convert_from_int16(const t_int16 * p_source,t_size p_count,audio_sample * p_output,audio_sample p_scale)
	{
		audio_sample scale = (audio_sample) ( p_scale / (double) 0x8000 );
		noopt_convert_from_int16(p_source,p_count,p_output,scale);
	}

	void convert_to_int32(const audio_sample * p_source,t_size p_count,t_int32 * p_output,audio_sample p_scale)
	{
		audio_sample scale = (audio_sample)(p_scale * 0x80000000);
		{
			noopt_convert_to_32bit(p_source,p_count,p_output,scale);
		}
	}

	audio_sample convert_to_int32_calculate_peak(const audio_sample * p_source,t_size p_count,t_int32 * p_output,audio_sample p_scale)
	{
		convert_to_int32(p_source,p_count,p_output,p_scale);
		return p_scale * calculate_peak(p_source,p_count);
	}

	void convert_from_int32(const t_int32 * p_source,t_size p_count,audio_sample * p_output,audio_sample p_scale)
	{
		audio_sample scale = (audio_sample) ( p_scale / (double) 0x80000000 );
		noopt_convert_from_int32(p_source,p_count,p_output,scale);
	}


	audio_sample calculate_peak(const audio_sample * p_source,t_size p_count)
	{
		return noopt_calculate_peak(p_source,p_count);
	}

	void kill_denormal(audio_sample * p_buffer,t_size p_count) {
#if audio_sample_size == 32
		t_uint32 * ptr = reinterpret_cast<t_uint32*>(p_buffer);
		for(;p_count;p_count--)
		{
			t_uint32 t = *ptr;
			if ((t & 0x007FFFFF) && !(t & 0x7F800000)) *ptr=0;
			ptr++;
		}
#elif audio_sample_size == 64
		t_uint64 * ptr = reinterpret_cast<t_uint64*>(p_buffer);
		for(;p_count;p_count--)
		{
			t_uint64 t = *ptr;
			if ((t & 0x000FFFFFFFFFFFFF) && !(t & 0x7FF0000000000000)) *ptr=0;
			ptr++;
		}
#else
#error unsupported
#endif
	}

	void add_offset(audio_sample * p_buffer,audio_sample p_delta,t_size p_count) {
		for(t_size n=0;n<p_count;n++) {
			p_buffer[n] += p_delta;
		}
	}

}

#endif // _WIN32
