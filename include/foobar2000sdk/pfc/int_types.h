#pragma once

#include <stdint.h>
typedef int64_t t_int64;
typedef uint64_t t_uint64;
typedef int32_t t_int32;
typedef uint32_t t_uint32;
typedef int16_t t_int16;
typedef uint16_t t_uint16;
typedef int8_t t_int8;
typedef uint8_t t_uint8;

typedef int t_int;
typedef unsigned int t_uint;

typedef float t_float32;
typedef double t_float64;



namespace pfc {
	template<unsigned t_bytes>
	class sized_int_t;

	template<> class sized_int_t<1> {
	public:
		typedef t_uint8 t_unsigned;
		typedef t_int8 t_signed;
	};

	template<> class sized_int_t<2> {
	public:
		typedef t_uint16 t_unsigned;
		typedef t_int16 t_signed;
	};

	template<> class sized_int_t<4> {
	public:
		typedef t_uint32 t_unsigned;
		typedef t_int32 t_signed;
	};

	template<> class sized_int_t<8> {
	public:
		typedef t_uint64 t_unsigned;
		typedef t_int64 t_signed;
	};
}

typedef size_t t_size;
typedef pfc::sized_int_t< sizeof(size_t) >::t_signed t_ssize;


inline t_size MulDiv_Size(t_size x,t_size y,t_size z) {return (t_size) ( ((t_uint64)x * (t_uint64)y) / (t_uint64)z );}

#define pfc_infinite (~0)

namespace pfc {
	const t_uint16 infinite16 = (t_uint16)(~0);
	const t_uint32 infinite32 = (t_uint32)(~0);
	const t_uint64 infinite64 = (t_uint64)(~0);
	const t_size infinite_size = (t_size)(~0);

	template<typename T> class int_specs_t;

	template<typename T>
	class int_specs_signed_t {
	public:
		inline static T get_min() {return ((T)1<<(sizeof(T)*8-1));}
		inline static T get_max() {return ~((T)1<<(sizeof(T)*8-1));}
		enum {is_signed = true};
	};

	template<typename T>
	class int_specs_unsigned_t {
	public:
		inline static T get_min() {return (T)0;}
		inline static T get_max() {return (T)~0;}
		enum {is_signed = false};
	};

	template<> class int_specs_t<char> : public int_specs_signed_t<char> {};
	template<> class int_specs_t<unsigned char> : public int_specs_unsigned_t<unsigned char> {};
	template<> class int_specs_t<short> : public int_specs_signed_t<short> {};
	template<> class int_specs_t<unsigned short> : public int_specs_unsigned_t<unsigned short> {};
	template<> class int_specs_t<int> : public int_specs_signed_t<int> {};
	template<> class int_specs_t<unsigned int> : public int_specs_unsigned_t<unsigned int> {};
	template<> class int_specs_t<long> : public int_specs_signed_t<long> {};
	template<> class int_specs_t<unsigned long> : public int_specs_unsigned_t<unsigned long> {};
	template<> class int_specs_t<long long> : public int_specs_signed_t<long long> {};
	template<> class int_specs_t<unsigned long long> : public int_specs_unsigned_t<unsigned long long> {};
	
	template<> class int_specs_t<wchar_t> : public int_specs_unsigned_t<wchar_t> {};

};
