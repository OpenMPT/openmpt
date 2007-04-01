#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#ifdef WIN32
	typedef __int8 int8;
	typedef __int16 int16;
	typedef __int32 int32;
	typedef __int64 int64;

	typedef unsigned __int8 uint8;
	typedef unsigned __int16 uint16;
	typedef unsigned __int32 uint32;
	typedef size_t uint64;

	typedef float float32;

#endif

#endif
