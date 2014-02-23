/*
 * mptString.cpp
 * -------------
 * Purpose: Small string-related utilities, number and message formatting.
 * Notes  : Currently none.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"
#include "mptString.h"

#if defined(MPT_CHARSET_CPP)
#include <codecvt>
#elif defined(MPT_CHARSET_CUSTOMUTF8)
#include <cstdlib>
#endif
#include <iomanip>
#include <iterator>
#include <locale>
#include <sstream>
#include <string>
#include <stdexcept>
#include <vector>

#include <cstdarg>

#if MPT_COMPILER_GCC || MPT_COMPILER_CLANG
#include <strings.h> // for strncasecmp
#endif

#if !defined(MPT_CHARSET_CPP) && !defined(MPT_CHARSET_CUSTOMUTF8) && !defined(WIN32)
#include <iconv.h>
#endif // !WIN32


namespace mpt {


int strnicmp(const char *a, const char *b, size_t count)
//------------------------------------------------------
{
#if MPT_COMPILER_MSVC
	return _strnicmp(a, b, count);
#else
	return strncasecmp(a, b, count);
#endif
}


} // namespace mpt


namespace mpt { namespace String {


#ifdef MODPLUG_TRACKER

std::string Format(const char *format, ...)
{
	#if MPT_COMPILER_MSVC
		va_list argList;
		va_start(argList, format);

		// Count the needed array size.
		const size_t nCount = _vscprintf(format, argList); // null character not included.
		std::vector<char> buf(nCount + 1); // + 1 is for null terminator.
		vsprintf_s(&(buf[0]), buf.size(), format, argList);

		va_end(argList);
		return &(buf[0]);
	#else
		va_list argList;
		va_start(argList, format);
		int size = vsnprintf(NULL, 0, format, argList); // get required size, requires c99 compliant vsnprintf which msvc does not have
		va_end(argList);
		std::vector<char> temp(size + 1);
		va_start(argList, format);
		vsnprintf(&(temp[0]), size + 1, format, argList);
		va_end(argList);
		return &(temp[0]);
	#endif
}

#endif



/*
default 1:1 mapping
static const uint32 CharsetTableISO8859_1[256] = {
	0x0000,0x0001,0x0002,0x0003,0x0004,0x0005,0x0006,0x0007,0x0008,0x0009,0x000a,0x000b,0x000c,0x000d,0x000e,0x000f,
	0x0010,0x0011,0x0012,0x0013,0x0014,0x0015,0x0016,0x0017,0x0018,0x0019,0x001a,0x001b,0x001c,0x001d,0x001e,0x001f,
	0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,0x0028,0x0029,0x002a,0x002b,0x002c,0x002d,0x002e,0x002f,
	0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,0x0038,0x0039,0x003a,0x003b,0x003c,0x003d,0x003e,0x003f,
	0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,0x0048,0x0049,0x004a,0x004b,0x004c,0x004d,0x004e,0x004f,
	0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,0x0058,0x0059,0x005a,0x005b,0x005c,0x005d,0x005e,0x005f,
	0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,0x0068,0x0069,0x006a,0x006b,0x006c,0x006d,0x006e,0x006f,
	0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,0x0078,0x0079,0x007a,0x007b,0x007c,0x007d,0x007e,0x007f,
	0x0080,0x0081,0x0082,0x0083,0x0084,0x0085,0x0086,0x0087,0x0088,0x0089,0x008a,0x008b,0x008c,0x008d,0x008e,0x008f,
	0x0090,0x0091,0x0092,0x0093,0x0094,0x0095,0x0096,0x0097,0x0098,0x0099,0x009a,0x009b,0x009c,0x009d,0x009e,0x009f,
	0x00a0,0x00a1,0x00a2,0x00a3,0x00a4,0x00a5,0x00a6,0x00a7,0x00a8,0x00a9,0x00aa,0x00ab,0x00ac,0x00ad,0x00ae,0x00af,
	0x00b0,0x00b1,0x00b2,0x00b3,0x00b4,0x00b5,0x00b6,0x00b7,0x00b8,0x00b9,0x00ba,0x00bb,0x00bc,0x00bd,0x00be,0x00bf,
	0x00c0,0x00c1,0x00c2,0x00c3,0x00c4,0x00c5,0x00c6,0x00c7,0x00c8,0x00c9,0x00ca,0x00cb,0x00cc,0x00cd,0x00ce,0x00cf,
	0x00d0,0x00d1,0x00d2,0x00d3,0x00d4,0x00d5,0x00d6,0x00d7,0x00d8,0x00d9,0x00da,0x00db,0x00dc,0x00dd,0x00de,0x00df,
	0x00e0,0x00e1,0x00e2,0x00e3,0x00e4,0x00e5,0x00e6,0x00e7,0x00e8,0x00e9,0x00ea,0x00eb,0x00ec,0x00ed,0x00ee,0x00ef,
	0x00f0,0x00f1,0x00f2,0x00f3,0x00f4,0x00f5,0x00f6,0x00f7,0x00f8,0x00f9,0x00fa,0x00fb,0x00fc,0x00fd,0x00fe,0x00ff
};
*/

#if defined(MPT_CHARSET_CPP) || defined(MPT_CHARSET_CUSTOMUTF8)

static const uint32 CharsetTableISO8859_15[256] = {
	0x0000,0x0001,0x0002,0x0003,0x0004,0x0005,0x0006,0x0007,0x0008,0x0009,0x000a,0x000b,0x000c,0x000d,0x000e,0x000f,
	0x0010,0x0011,0x0012,0x0013,0x0014,0x0015,0x0016,0x0017,0x0018,0x0019,0x001a,0x001b,0x001c,0x001d,0x001e,0x001f,
	0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,0x0028,0x0029,0x002a,0x002b,0x002c,0x002d,0x002e,0x002f,
	0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,0x0038,0x0039,0x003a,0x003b,0x003c,0x003d,0x003e,0x003f,
	0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,0x0048,0x0049,0x004a,0x004b,0x004c,0x004d,0x004e,0x004f,
	0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,0x0058,0x0059,0x005a,0x005b,0x005c,0x005d,0x005e,0x005f,
	0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,0x0068,0x0069,0x006a,0x006b,0x006c,0x006d,0x006e,0x006f,
	0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,0x0078,0x0079,0x007a,0x007b,0x007c,0x007d,0x007e,0x007f,
	0x0080,0x0081,0x0082,0x0083,0x0084,0x0085,0x0086,0x0087,0x0088,0x0089,0x008a,0x008b,0x008c,0x008d,0x008e,0x008f,
	0x0090,0x0091,0x0092,0x0093,0x0094,0x0095,0x0096,0x0097,0x0098,0x0099,0x009a,0x009b,0x009c,0x009d,0x009e,0x009f,
	0x00a0,0x00a1,0x00a2,0x00a3,0x20ac,0x00a5,0x0160,0x00a7,0x0161,0x00a9,0x00aa,0x00ab,0x00ac,0x00ad,0x00ae,0x00af,
	0x00b0,0x00b1,0x00b2,0x00b3,0x017d,0x00b5,0x00b6,0x00b7,0x017e,0x00b9,0x00ba,0x00bb,0x0152,0x0153,0x0178,0x00bf,
	0x00c0,0x00c1,0x00c2,0x00c3,0x00c4,0x00c5,0x00c6,0x00c7,0x00c8,0x00c9,0x00ca,0x00cb,0x00cc,0x00cd,0x00ce,0x00cf,
	0x00d0,0x00d1,0x00d2,0x00d3,0x00d4,0x00d5,0x00d6,0x00d7,0x00d8,0x00d9,0x00da,0x00db,0x00dc,0x00dd,0x00de,0x00df,
	0x00e0,0x00e1,0x00e2,0x00e3,0x00e4,0x00e5,0x00e6,0x00e7,0x00e8,0x00e9,0x00ea,0x00eb,0x00ec,0x00ed,0x00ee,0x00ef,
	0x00f0,0x00f1,0x00f2,0x00f3,0x00f4,0x00f5,0x00f6,0x00f7,0x00f8,0x00f9,0x00fa,0x00fb,0x00fc,0x00fd,0x00fe,0x00ff
};

static const uint32 CharsetTableWindows1252[256] = {
	0x0000,0x0001,0x0002,0x0003,0x0004,0x0005,0x0006,0x0007,0x0008,0x0009,0x000a,0x000b,0x000c,0x000d,0x000e,0x000f,
	0x0010,0x0011,0x0012,0x0013,0x0014,0x0015,0x0016,0x0017,0x0018,0x0019,0x001a,0x001b,0x001c,0x001d,0x001e,0x001f,
	0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,0x0028,0x0029,0x002a,0x002b,0x002c,0x002d,0x002e,0x002f,
	0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,0x0038,0x0039,0x003a,0x003b,0x003c,0x003d,0x003e,0x003f,
	0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,0x0048,0x0049,0x004a,0x004b,0x004c,0x004d,0x004e,0x004f,
	0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,0x0058,0x0059,0x005a,0x005b,0x005c,0x005d,0x005e,0x005f,
	0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,0x0068,0x0069,0x006a,0x006b,0x006c,0x006d,0x006e,0x006f,
	0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,0x0078,0x0079,0x007a,0x007b,0x007c,0x007d,0x007e,0x007f,
	0x20ac,0x0081,0x201a,0x0192,0x201e,0x2026,0x2020,0x2021,0x02c6,0x2030,0x0160,0x2039,0x0152,0x008d,0x017d,0x008f,
	0x0090,0x2018,0x2019,0x201c,0x201d,0x2022,0x2013,0x2014,0x02dc,0x2122,0x0161,0x203a,0x0153,0x009d,0x017e,0x0178,
	0x00a0,0x00a1,0x00a2,0x00a3,0x00a4,0x00a5,0x00a6,0x00a7,0x00a8,0x00a9,0x00aa,0x00ab,0x00ac,0x00ad,0x00ae,0x00af,
	0x00b0,0x00b1,0x00b2,0x00b3,0x00b4,0x00b5,0x00b6,0x00b7,0x00b8,0x00b9,0x00ba,0x00bb,0x00bc,0x00bd,0x00be,0x00bf,
	0x00c0,0x00c1,0x00c2,0x00c3,0x00c4,0x00c5,0x00c6,0x00c7,0x00c8,0x00c9,0x00ca,0x00cb,0x00cc,0x00cd,0x00ce,0x00cf,
	0x00d0,0x00d1,0x00d2,0x00d3,0x00d4,0x00d5,0x00d6,0x00d7,0x00d8,0x00d9,0x00da,0x00db,0x00dc,0x00dd,0x00de,0x00df,
	0x00e0,0x00e1,0x00e2,0x00e3,0x00e4,0x00e5,0x00e6,0x00e7,0x00e8,0x00e9,0x00ea,0x00eb,0x00ec,0x00ed,0x00ee,0x00ef,
	0x00f0,0x00f1,0x00f2,0x00f3,0x00f4,0x00f5,0x00f6,0x00f7,0x00f8,0x00f9,0x00fa,0x00fb,0x00fc,0x00fd,0x00fe,0x00ff
};

static const uint32 CharsetTableCP437[256] = {
	0x0000,0x0001,0x0002,0x0003,0x0004,0x0005,0x0006,0x0007,0x0008,0x0009,0x000a,0x000b,0x000c,0x000d,0x000e,0x000f,
	0x0010,0x0011,0x0012,0x0013,0x0014,0x0015,0x0016,0x0017,0x0018,0x0019,0x001a,0x001b,0x001c,0x001d,0x001e,0x001f,
	0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,0x0028,0x0029,0x002a,0x002b,0x002c,0x002d,0x002e,0x002f,
	0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,0x0038,0x0039,0x003a,0x003b,0x003c,0x003d,0x003e,0x003f,
	0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,0x0048,0x0049,0x004a,0x004b,0x004c,0x004d,0x004e,0x004f,
	0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,0x0058,0x0059,0x005a,0x005b,0x005c,0x005d,0x005e,0x005f,
	0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,0x0068,0x0069,0x006a,0x006b,0x006c,0x006d,0x006e,0x006f,
	0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,0x0078,0x0079,0x007a,0x007b,0x007c,0x007d,0x007e,0x2302,
	0x00c7,0x00fc,0x00e9,0x00e2,0x00e4,0x00e0,0x00e5,0x00e7,0x00ea,0x00eb,0x00e8,0x00ef,0x00ee,0x00ec,0x00c4,0x00c5,
	0x00c9,0x00e6,0x00c6,0x00f4,0x00f6,0x00f2,0x00fb,0x00f9,0x00ff,0x00d6,0x00dc,0x00a2,0x00a3,0x00a5,0x20a7,0x0192,
	0x00e1,0x00ed,0x00f3,0x00fa,0x00f1,0x00d1,0x00aa,0x00ba,0x00bf,0x2310,0x00ac,0x00bd,0x00bc,0x00a1,0x00ab,0x00bb,
	0x2591,0x2592,0x2593,0x2502,0x2524,0x2561,0x2562,0x2556,0x2555,0x2563,0x2551,0x2557,0x255d,0x255c,0x255b,0x2510,
	0x2514,0x2534,0x252c,0x251c,0x2500,0x253c,0x255e,0x255f,0x255a,0x2554,0x2569,0x2566,0x2560,0x2550,0x256c,0x2567,
	0x2568,0x2564,0x2565,0x2559,0x2558,0x2552,0x2553,0x256b,0x256a,0x2518,0x250c,0x2588,0x2584,0x258c,0x2590,0x2580,
	0x03b1,0x00df,0x0393,0x03c0,0x03a3,0x03c3,0x00b5,0x03c4,0x03a6,0x0398,0x03a9,0x03b4,0x221e,0x03c6,0x03b5,0x2229,
	0x2261,0x00b1,0x2265,0x2264,0x2320,0x2321,0x00f7,0x2248,0x00b0,0x2219,0x00b7,0x221a,0x207f,0x00b2,0x25a0,0x00a0
};

#endif

#define C(x) ((uint8)(x))

// AMS1 actually only supports ASCII plus the modified control characters and no high chars at all.
// Just default to CP437 for those to keep things simple.
static const uint32 CharsetTableCP437AMS[256] = {
	C(' '),0x0001,0x0002,0x0003,0x00e4,0x0005,0x00e5,0x0007,0x0008,0x0009,0x000a,0x000b,0x000c,0x000d,0x00c4,0x00c5, // differs from CP437
	0x0010,0x0011,0x0012,0x0013,0x00f6,0x0015,0x0016,0x0017,0x0018,0x00d6,0x001a,0x001b,0x001c,0x001d,0x001e,0x001f, // differs from CP437
	0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,0x0028,0x0029,0x002a,0x002b,0x002c,0x002d,0x002e,0x002f,
	0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,0x0038,0x0039,0x003a,0x003b,0x003c,0x003d,0x003e,0x003f,
	0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,0x0048,0x0049,0x004a,0x004b,0x004c,0x004d,0x004e,0x004f,
	0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,0x0058,0x0059,0x005a,0x005b,0x005c,0x005d,0x005e,0x005f,
	0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,0x0068,0x0069,0x006a,0x006b,0x006c,0x006d,0x006e,0x006f,
	0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,0x0078,0x0079,0x007a,0x007b,0x007c,0x007d,0x007e,0x2302,
	0x00c7,0x00fc,0x00e9,0x00e2,0x00e4,0x00e0,0x00e5,0x00e7,0x00ea,0x00eb,0x00e8,0x00ef,0x00ee,0x00ec,0x00c4,0x00c5,
	0x00c9,0x00e6,0x00c6,0x00f4,0x00f6,0x00f2,0x00fb,0x00f9,0x00ff,0x00d6,0x00dc,0x00a2,0x00a3,0x00a5,0x20a7,0x0192,
	0x00e1,0x00ed,0x00f3,0x00fa,0x00f1,0x00d1,0x00aa,0x00ba,0x00bf,0x2310,0x00ac,0x00bd,0x00bc,0x00a1,0x00ab,0x00bb,
	0x2591,0x2592,0x2593,0x2502,0x2524,0x2561,0x2562,0x2556,0x2555,0x2563,0x2551,0x2557,0x255d,0x255c,0x255b,0x2510,
	0x2514,0x2534,0x252c,0x251c,0x2500,0x253c,0x255e,0x255f,0x255a,0x2554,0x2569,0x2566,0x2560,0x2550,0x256c,0x2567,
	0x2568,0x2564,0x2565,0x2559,0x2558,0x2552,0x2553,0x256b,0x256a,0x2518,0x250c,0x2588,0x2584,0x258c,0x2590,0x2580,
	0x03b1,0x00df,0x0393,0x03c0,0x03a3,0x03c3,0x00b5,0x03c4,0x03a6,0x0398,0x03a9,0x03b4,0x221e,0x03c6,0x03b5,0x2229,
	0x2261,0x00b1,0x2265,0x2264,0x2320,0x2321,0x00f7,0x2248,0x00b0,0x2219,0x00b7,0x221a,0x207f,0x00b2,0x25a0,0x00a0
};

// AMS2: Looking at Velvet Studio's bitmap font (TPIC32.PCX), these appear to be the only supported non-ASCII chars.
static const uint32 CharsetTableCP437AMS2[256] = {
	C(' '),0x00a9,0x221a,0x00b7,C('0'),C('1'),C('2'),C('3'),C('4'),C('5'),C('6'),C('7'),C('8'),C('9'),C('A'),C('B'), // differs from CP437
	C('C'),C('D'),C('E'),C('F'),C(' '),0x00a7,C(' '),C(' '),C(' '),C(' '),C(' '),C(' '),C(' '),C(' '),C(' '),C(' '), // differs from CP437
	0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,0x0028,0x0029,0x002a,0x002b,0x002c,0x002d,0x002e,0x002f,
	0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,0x0038,0x0039,0x003a,0x003b,0x003c,0x003d,0x003e,0x003f,
	0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,0x0048,0x0049,0x004a,0x004b,0x004c,0x004d,0x004e,0x004f,
	0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,0x0058,0x0059,0x005a,0x005b,0x005c,0x005d,0x005e,0x005f,
	0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,0x0068,0x0069,0x006a,0x006b,0x006c,0x006d,0x006e,0x006f,
	0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,0x0078,0x0079,0x007a,0x007b,0x007c,0x007d,0x007e,0x2302,
	0x00c7,0x00fc,0x00e9,0x00e2,0x00e4,0x00e0,0x00e5,0x00e7,0x00ea,0x00eb,0x00e8,0x00ef,0x00ee,0x00ec,0x00c4,0x00c5,
	0x00c9,0x00e6,0x00c6,0x00f4,0x00f6,0x00f2,0x00fb,0x00f9,0x00ff,0x00d6,0x00dc,0x00a2,0x00a3,0x00a5,0x20a7,0x0192,
	0x00e1,0x00ed,0x00f3,0x00fa,0x00f1,0x00d1,0x00aa,0x00ba,0x00bf,0x2310,0x00ac,0x00bd,0x00bc,0x00a1,0x00ab,0x00bb,
	0x2591,0x2592,0x2593,0x2502,0x2524,0x2561,0x2562,0x2556,0x2555,0x2563,0x2551,0x2557,0x255d,0x255c,0x255b,0x2510,
	0x2514,0x2534,0x252c,0x251c,0x2500,0x253c,0x255e,0x255f,0x255a,0x2554,0x2569,0x2566,0x2560,0x2550,0x256c,0x2567,
	0x2568,0x2564,0x2565,0x2559,0x2558,0x2552,0x2553,0x256b,0x256a,0x2518,0x250c,0x2588,0x2584,0x258c,0x2590,0x2580,
	0x03b1,0x00df,0x0393,0x03c0,0x03a3,0x03c3,0x00b5,0x03c4,0x03a6,0x0398,0x03a9,0x03b4,0x221e,0x03c6,0x03b5,0x2229,
	0x2261,0x00b1,0x2265,0x2264,0x2320,0x2321,0x00f7,0x2248,0x00b0,0x2219,0x00b7,0x221a,0x207f,0x00b2,0x25a0,0x00a0
};

#undef C

#if MPT_COMPILER_MSVC
#pragma warning(disable:4428) // universal-character-name encountered in source
#endif

static std::wstring From8bit(const std::string &str, const uint32 (&table)[256], wchar_t replacement = L'\uFFFD')
//---------------------------------------------------------------------------------------------------------------
{
	std::wstring res;
	for(std::size_t i = 0; i < str.length(); ++i)
	{
		uint32 c = static_cast<uint32>(static_cast<uint8>(str[i]));
		if(c <= CountOf(table))
		{
			res.push_back(static_cast<wchar_t>(static_cast<uint32>(table[c])));
		} else
		{
			res.push_back(replacement);
		}
	}
	return res;
}

static std::string To8bit(const std::wstring &str, const uint32 (&table)[256], char replacement = '?')
//----------------------------------------------------------------------------------------------------
{
	std::string res;
	for(std::size_t i = 0; i < str.length(); ++i)
	{
		uint32 c = str[i];
		bool found = false;
		// Try non-control characters first.
		// In cases where there are actual characters mirrored in this range (like in AMS/AMS2 character sets),
		// characters in the common range are preferred this way.
		for(std::size_t x = 0x20; x < CountOf(table); ++x)
		{
			if(c == table[x])
			{
				res.push_back(static_cast<char>(static_cast<uint8>(x)));
				found = true;
				break;
			}
		}
		if(!found)
		{
			// try control characters
			for(std::size_t x = 0x00; x < CountOf(table) && x < 0x20; ++x)
			{
				if(c == table[x])
				{
					res.push_back(static_cast<char>(static_cast<uint8>(x)));
					found = true;
					break;
				}
			}
		}
		if(!found)
		{
			res.push_back(replacement);
		}
	}
	return res;
}

#if defined(MPT_CHARSET_CPP) || defined(MPT_CHARSET_CUSTOMUTF8)

static std::wstring FromAscii(const std::string &str, wchar_t replacement = L'\uFFFD')
//------------------------------------------------------------------------------------
{
	std::wstring res;
	for(std::size_t i = 0; i < str.length(); ++i)
	{
		uint8 c = str[i];
		if(c <= 0x7f)
		{
			res.push_back(static_cast<wchar_t>(static_cast<uint32>(c)));
		} else
		{
			res.push_back(replacement);
		}
	}
	return res;
}

static std::string ToAscii(const std::wstring &str, char replacement = '?')
//-------------------------------------------------------------------------
{
	std::string res;
	for(std::size_t i = 0; i < str.length(); ++i)
	{
		uint32 c = str[i];
		if(c <= 0x7f)
		{
			res.push_back(static_cast<char>(static_cast<uint8>(c)));
		} else
		{
			res.push_back(replacement);
		}
	}
	return res;
}

static std::wstring FromISO_8859_1(const std::string &str, wchar_t replacement = L'\uFFFD')
//-----------------------------------------------------------------------------------------
{
	MPT_UNREFERENCED_PARAMETER(replacement);
	std::wstring res;
	for(std::size_t i = 0; i < str.length(); ++i)
	{
		uint8 c = str[i];
		res.push_back(static_cast<wchar_t>(static_cast<uint32>(c)));
	}
	return res;
}

static std::string ToISO_8859_1(const std::wstring &str, char replacement = '?')
//------------------------------------------------------------------------------
{
	std::string res;
	for(std::size_t i = 0; i < str.length(); ++i)
	{
		uint32 c = str[i];
		if(c <= 0xff)
		{
			res.push_back(static_cast<char>(static_cast<uint8>(c)));
		} else
		{
			res.push_back(replacement);
		}
	}
	return res;
}

static std::wstring LocaleDecode(const std::string &str, const std::locale & locale, wchar_t replacement = L'\uFFFD')
//-------------------------------------------------------------------------------------------------------------------
{
	if(str.empty())
	{
		return std::wstring();
	}
	std::vector<wchar_t> out;
	typedef std::codecvt<wchar_t, char, std::mbstate_t> codecvt_type;
	std::mbstate_t state = std::mbstate_t();
	const codecvt_type & facet = std::use_facet<codecvt_type>(locale);
	codecvt_type::result result = codecvt_type::partial;
	const char * in_begin = &*str.begin();
	const char * in_end = in_begin + str.length();
	out.resize((in_end - in_begin) * (facet.max_length() + 1));
	wchar_t * out_begin = out.data();
	wchar_t * out_end = out.data() + out.size();
	const char * in_next = nullptr;
	wchar_t * out_next = nullptr;
	do
	{
		in_next = nullptr;
		out_next = nullptr;
		result = facet.in(state, in_begin, in_end, in_next, out_begin, out_end, out_next);
		if(result == codecvt_type::partial || (result == codecvt_type::error && out_next == out_end))
		{
			out.resize(out.size() * 2);
			in_begin = in_next;
			out_begin = out.data() + (out_next - out_begin);
			out_end = out.data() + out.size();
			continue;
		}
		if(result == codecvt_type::error)
		{
			++in_next;
			*out_next = replacement;
			++out_next;
		}
		in_begin = in_next;
		out_begin = out_next;
	} while(result == codecvt_type::error && in_next < in_end && out_next < out_end);
	return std::wstring(out.data(), out_next);
}

static std::string LocaleEncode(const std::wstring &str, const std::locale & locale, char replacement = '?')
//----------------------------------------------------------------------------------------------------------
{
	if(str.empty())
	{
		return std::string();
	}
	std::vector<char> out;
	typedef std::codecvt<wchar_t, char, std::mbstate_t> codecvt_type;
	std::mbstate_t state = std::mbstate_t();
	const codecvt_type & facet = std::use_facet<codecvt_type>(locale);
	codecvt_type::result result = codecvt_type::partial;
	const wchar_t * in_begin = &*str.begin();
	const wchar_t * in_end = in_begin + str.length();
	out.resize((in_end - in_begin) * (facet.max_length() + 1));
	char * out_begin = out.data();
	char * out_end = out.data() + out.size();
	const wchar_t * in_next = nullptr;
	char * out_next = nullptr;
	do
	{
		in_next = nullptr;
		out_next = nullptr;
		result = facet.out(state, in_begin, in_end, in_next, out_begin, out_end, out_next);
		if(result == codecvt_type::partial || (result == codecvt_type::error && out_next == out_end))
		{
			out.resize(out.size() * 2);
			in_begin = in_next;
			out_begin = out.data() + (out_next - out_begin);
			out_end = out.data() + out.size();
			continue;
		}
		if(result == codecvt_type::error)
		{
			++in_next;
			*out_next = replacement;
			++out_next;
		}
		in_begin = in_next;
		out_begin = out_next;
	} while(result == codecvt_type::error && in_next < in_end && out_next < out_end);
	return std::string(out.data(), out_next);
}

static std::wstring FromLocale(const std::string &str, wchar_t replacement = L'\uFFFD')
//-------------------------------------------------------------------------------------
{
	try
	{
		std::locale locale(""); // user locale
		return String::LocaleDecode(str, locale, replacement);
	} catch(...)
	{
		// nothing
	}
	try
	{
		std::locale locale; // current c++ locale
		return String::LocaleDecode(str, locale, replacement);
	} catch(...)
	{
		// nothing
	}
	try
	{
		std::locale locale = std::locale::classic(); // "C" locale
		return String::LocaleDecode(str, locale, replacement);
	} catch(...)
	{
		// nothing
	}
	ASSERT(false);
	return String::FromAscii(str, replacement); // fallback
}

static std::string ToLocale(const std::wstring &str, char replacement = '?')
//--------------------------------------------------------------------------
{
	try
	{
		std::locale locale(""); // user locale
		return String::LocaleEncode(str, locale, replacement);
	} catch(...)
	{
		// nothing
	}
	try
	{
		std::locale locale; // current c++ locale
		return String::LocaleEncode(str, locale, replacement);
	} catch(...)
	{
		// nothing
	}
	try
	{
		std::locale locale = std::locale::classic(); // "C" locale
		return String::LocaleEncode(str, locale, replacement);
	} catch(...)
	{
		// nothing
	}
	ASSERT(false);
	return String::ToAscii(str, replacement); // fallback
}

#if defined(MPT_CHARSET_CPP)

static std::wstring FromUTF8(const std::string &str, wchar_t replacement = L'\uFFFD')
//-----------------------------------------------------------------------------------
{
	MPT_UNREFERENCED_PARAMETER(replacement);
	std::wstring_convert<std::codecvt_utf8<wchar_t> > conv;
	return conv.from_bytes(str);
}

static std::string ToUTF8(const std::wstring &str, char replacement = '?')
//------------------------------------------------------------------------
{
	MPT_UNREFERENCED_PARAMETER(replacement);
	std::wstring_convert<std::codecvt_utf8<wchar_t> > conv;
	return conv.to_bytes(str);
}

#elif defined(MPT_CHARSET_CUSTOMUTF8)

static std::wstring FromUTF8(const std::string &str, wchar_t replacement = L'\uFFFD')
//-----------------------------------------------------------------------------------
{
	const std::string &in = str;

	std::wstring out;

	// state:
	std::size_t charsleft = 0;
	uint32 ucs4 = 0;

	for ( std::string::const_iterator i = in.begin(); i != in.end(); ++i ) {

		uint8 c = *i;

		if ( charsleft == 0 ) {

			if ( ( c & 0x80 ) == 0x00 ) {
				out.push_back( (wchar_t)c );
			} else if ( ( c & 0xE0 ) == 0xC0 ) {
				ucs4 = c & 0x1F;
				charsleft = 1;
			} else if ( ( c & 0xF0 ) == 0xE0 ) {
				ucs4 = c & 0x0F;
				charsleft = 2;
			} else if ( ( c & 0xF8 ) == 0xF0 ) {
				ucs4 = c & 0x07;
				charsleft = 3;
			} else {
				out.push_back( replacement );
				ucs4 = 0;
				charsleft = 0;
			}

		} else {

			if ( ( c & 0xC0 ) != 0x80 ) {
				out.push_back( replacement );
				ucs4 = 0;
				charsleft = 0;
			}
			ucs4 <<= 6;
			ucs4 |= c & 0x3F;
			charsleft--;

			if ( charsleft == 0 ) {
				if ( sizeof( wchar_t ) == 2 ) {
					if ( ucs4 > 0x1fffff ) {
						out.push_back( replacement );
						ucs4 = 0;
						charsleft = 0;
					}
					if ( ucs4 <= 0xffff ) {
						out.push_back( (uint16)ucs4 );
					} else {
						uint32 surrogate = ucs4 - 0x10000;
						uint16 hi_sur = static_cast<uint16>( ( 0x36 << 10 ) | ( (surrogate>>10) & ((1<<10)-1) ) );
						uint16 lo_sur = static_cast<uint16>( ( 0x37 << 10 ) | ( (surrogate>> 0) & ((1<<10)-1) ) );
						out.push_back( hi_sur );
						out.push_back( lo_sur );
					}
				} else {
					out.push_back( ucs4 );
				}
				ucs4 = 0;
			}

		}

	}

	if ( charsleft != 0 ) {
		out.push_back( replacement );
		ucs4 = 0;
		charsleft = 0;
	}

	return out;

}

static std::string ToUTF8(const std::wstring &str, char replacement = '?')
//------------------------------------------------------------------------
{
	const std::wstring &in = str;

	std::string out;

	for ( std::size_t i=0; i<in.length(); i++ ) {

		wchar_t c = in[i];
		if ( c > 0x1fffff || c < 0 ) {
			out.push_back( replacement );
		}

		uint32 ucs4 = 0;
		if ( sizeof( wchar_t ) == 2 ) {
			if ( i + 1 < in.length() ) {
				// check for surrogate pair
				uint16 hi_sur = in[i+0];
				uint16 lo_sur = in[i+1];
				if ( hi_sur >> 10 == 0x36 && lo_sur >> 10 == 0x37 ) {
					// surrogate pair
					++i;
					hi_sur &= (1<<10)-1;
					lo_sur &= (1<<10)-1;
					ucs4 = ( (uint32)hi_sur << 10 ) | ( (uint32)lo_sur << 0 );
				} else {
					// no surrogate pair
					ucs4 = (uint32)(uint16)c;
				}
			} else {
				// no surrogate possible
				ucs4 = (uint32)(uint16)c;
			}
		} else {
			ucs4 = c;
		}

		uint8 utf8[6];
		std::size_t numchars = 0;
		for ( numchars = 0; numchars < 6; numchars++ ) {
			utf8[numchars] = ucs4 & 0x3F;
			ucs4 >>= 6;
			if ( ucs4 == 0 ) {
				break;
			}
		}
		numchars++;

		if ( numchars == 1 ) {
			out.push_back( utf8[0] );
			continue;
		}

		if ( numchars == 2 && utf8[numchars-1] == 0x01 ) {
			// generate shortest form
			out.push_back( utf8[0] | 0x40 );
			continue;
		}

		std::size_t charsleft = numchars;
		while ( charsleft > 0 ) {
			if ( charsleft == numchars ) {
				out.push_back( utf8[ charsleft - 1 ] | ( ((1<<numchars)-1) << (8-numchars) ) );
			} else {
				out.push_back( utf8[ charsleft - 1 ] | 0x80 );
			}
			charsleft--;
		}

	}

	return out;

}

#endif // MPT_CHARSET_CPP || MPT_CHARSET_CUSTOMUTF8

#elif defined(WIN32)
static UINT CharsetToCodepage(Charset charset)
{
	switch(charset)
	{
		case CharsetLocale:      return CP_ACP;  break;
		case CharsetUTF8:        return CP_UTF8; break;
		case CharsetASCII:       return 20127;   break;
		case CharsetISO8859_1:   return 28591;   break;
		case CharsetISO8859_15:  return 28605;   break;
		case CharsetCP437:       return 437;     break;
		case CharsetCP437AMS:    return 437;     break; // fallback, should not happen
		case CharsetCP437AMS2:   return 437;     break; // fallback, should not happen
		case CharsetWindows1252: return 1252;    break;
	}
	return 0;
}
#else // !WIN32
static const char * CharsetToString(Charset charset)
{
	switch(charset)
	{
		case CharsetLocale:      return "";            break; // "char" breaks with glibc when no locale is set
		case CharsetUTF8:        return "UTF-8";       break;
		case CharsetASCII:       return "ASCII";       break;
		case CharsetISO8859_1:   return "ISO-8859-1";  break;
		case CharsetISO8859_15:  return "ISO-8859-15"; break;
		case CharsetCP437:       return "CP437";       break;
		case CharsetCP437AMS:    return "CP437";       break; // fallback, should not happen
		case CharsetCP437AMS2:   return "CP437";       break; // fallback, should not happen
		case CharsetWindows1252: return "CP1252";      break;
	}
	return 0;
}
static const char * CharsetToStringTranslit(Charset charset)
{
	switch(charset)
	{
		case CharsetLocale:      return "//TRANSLIT";            break; // "char" breaks with glibc when no locale is set
		case CharsetUTF8:        return "UTF-8//TRANSLIT";       break;
		case CharsetASCII:       return "ASCII//TRANSLIT";       break;
		case CharsetISO8859_1:   return "ISO-8859-1//TRANSLIT";  break;
		case CharsetISO8859_15:  return "ISO-8859-15//TRANSLIT"; break;
		case CharsetCP437:       return "CP437//TRANSLIT";       break;
		case CharsetCP437AMS:    return "CP437//TRANSLIT";       break; // fallback, should not happen
		case CharsetCP437AMS2:   return "CP437//TRANSLIT";       break; // fallback, should not happen
		case CharsetWindows1252: return "CP1252//TRANSLIT";      break;
	}
	return 0;
}
static const char * Charset_wchar_t()
{
	#if !defined(MPT_ICONV_NO_WCHAR)
		return "wchar_t";
	#else // MPT_ICONV_NO_WCHAR
		// iconv on OSX does not handle wchar_t if no locale is set
		STATIC_ASSERT(sizeof(wchar_t) == 2 || sizeof(wchar_t) == 4);
		if(sizeof(wchar_t) == 2)
		{
			// "UTF-16" generates BOM
			#if defined(MPT_PLATFORM_LITTLE_ENDIAN)
				return "UTF-16LE";
			#elif defined(MPT_PLATFORM_BIG_ENDIAN)
				return "UTF-16BE";
			#else
				STATIC_ASSERT(false);
			#endif
		} else if(sizeof(wchar_t) == 4)
		{
			// "UTF-32" generates BOM
			#if defined(MPT_PLATFORM_LITTLE_ENDIAN)
				return "UTF-32LE";
			#elif defined(MPT_PLATFORM_BIG_ENDIAN)
				return "UTF-32BE";
			#else
				STATIC_ASSERT(false);
			#endif
		}
		return "";
	#endif // !MPT_ICONV_NO_WCHAR | MPT_ICONV_NO_WCHAR
}
#endif // WIN32


// templated on 8bit strings because of type-safe variants
template<typename Tdststring>
Tdststring EncodeImpl(Charset charset, const std::wstring &src)
{
	STATIC_ASSERT(sizeof(typename Tdststring::value_type) == sizeof(char));
	if(charset == CharsetCP437AMS || charset == CharsetCP437AMS2)
	{
		std::string out;
		if(charset == CharsetCP437AMS ) out = String::To8bit(src, CharsetTableCP437AMS );
		if(charset == CharsetCP437AMS2) out = String::To8bit(src, CharsetTableCP437AMS2);
		Tdststring result;
		std::copy(out.begin(), out.end(), std::back_inserter(result));
		return result;
	}
	#if defined(MPT_CHARSET_CPP) || defined(MPT_CHARSET_CUSTOMUTF8)
		std::string out;
		switch(charset)
		{
			case CharsetLocale:      out = String::ToLocale(src); break;
			case CharsetUTF8:        out = String::ToUTF8(src); break;
			case CharsetASCII:       out = String::ToAscii(src); break;
			case CharsetISO8859_1:   out = String::ToISO_8859_1(src); break;
			case CharsetISO8859_15:  out = String::To8bit(src, CharsetTableISO8859_15); break;
			case CharsetCP437:       out = String::To8bit(src, CharsetTableCP437); break;
			case CharsetCP437AMS:    out = String::To8bit(src, CharsetTableCP437AMS); break;
			case CharsetCP437AMS2:   out = String::To8bit(src, CharsetTableCP437AMS2); break;
			case CharsetWindows1252: out = String::To8bit(src, CharsetTableWindows1252); break;
		}
		Tdststring result;
		std::copy(out.begin(), out.end(), std::back_inserter(result));
		return result;
	#elif defined(WIN32)
		const UINT codepage = CharsetToCodepage(charset);
		int required_size = WideCharToMultiByte(codepage, 0, src.c_str(), -1, nullptr, 0, nullptr, nullptr);
		if(required_size <= 0)
		{
			return Tdststring();
		}
		std::vector<CHAR> encoded_string(required_size);
		WideCharToMultiByte(codepage, 0, src.c_str(), -1, &encoded_string[0], required_size, nullptr, nullptr);
		return reinterpret_cast<const typename Tdststring::value_type*>(&encoded_string[0]);
	#else // !WIN32
		iconv_t conv = iconv_t();
		conv = iconv_open(CharsetToStringTranslit(charset), Charset_wchar_t());
		if(!conv)
		{
			conv = iconv_open(CharsetToString(charset), Charset_wchar_t());
			if(!conv)
			{
				throw std::runtime_error("iconv conversion not working");
			}
		}
		std::vector<wchar_t> wide_string(src.c_str(), src.c_str() + src.length() + 1);
		std::vector<char> encoded_string(wide_string.size() * 8); // large enough
		char * inbuf = (char*)&wide_string[0];
		size_t inbytesleft = wide_string.size() * sizeof(wchar_t);
		char * outbuf = &encoded_string[0];
		size_t outbytesleft = encoded_string.size();
		while(iconv(conv, &inbuf, &inbytesleft, &outbuf, &outbytesleft) == (size_t)-1)
		{
			if(errno == EILSEQ || errno == EILSEQ)
			{
				inbuf += sizeof(wchar_t);
				inbytesleft -= sizeof(wchar_t);
				outbuf[0] = '?';
				outbuf++;
				outbytesleft--;
				iconv(conv, NULL, NULL, NULL, NULL); // reset state
			} else
			{
				iconv_close(conv);
				conv = iconv_t();
				return Tdststring();
			}
		}
		iconv_close(conv);
		conv = iconv_t();
		return reinterpret_cast<const typename Tdststring::value_type*>(&encoded_string[0]);
	#endif // WIN32
}


// templated on 8bit strings because of type-safe variants
template<typename Tsrcstring>
std::wstring DecodeImpl(Charset charset, const Tsrcstring &src)
{
	STATIC_ASSERT(sizeof(typename Tsrcstring::value_type) == sizeof(char));
	if(charset == CharsetCP437AMS || charset == CharsetCP437AMS2)
	{
		std::string in;
		std::copy(src.begin(), src.end(), std::back_inserter(in));
		std::wstring out;
		if(charset == CharsetCP437AMS ) out = String::From8bit(in, CharsetTableCP437AMS );
		if(charset == CharsetCP437AMS2) out = String::From8bit(in, CharsetTableCP437AMS2);
		return out;
	}
	#if defined(MPT_CHARSET_CPP) || defined(MPT_CHARSET_CUSTOMUTF8)
		std::string in;
		std::copy(src.begin(), src.end(), std::back_inserter(in));
		std::wstring out;
		switch(charset)
		{
			case CharsetLocale:      out = String::FromLocale(in); break;
			case CharsetUTF8:        out = String::FromUTF8(in); break;
			case CharsetASCII:       out = String::FromAscii(in); break;
			case CharsetISO8859_1:   out = String::FromISO_8859_1(in); break;
			case CharsetISO8859_15:  out = String::From8bit(in, CharsetTableISO8859_15); break;
			case CharsetCP437:       out = String::From8bit(in, CharsetTableCP437); break;
			case CharsetCP437AMS:    out = String::From8bit(in, CharsetTableCP437AMS); break;
			case CharsetCP437AMS2:   out = String::From8bit(in, CharsetTableCP437AMS2); break;
			case CharsetWindows1252: out = String::From8bit(in, CharsetTableWindows1252); break;
		}
		return out;
	#elif defined(WIN32)
		const UINT codepage = CharsetToCodepage(charset);
		int required_size = MultiByteToWideChar(codepage, 0, reinterpret_cast<const char*>(src.c_str()), -1, nullptr, 0);
		if(required_size <= 0)
		{
			return std::wstring();
		}
		std::vector<WCHAR> decoded_string(required_size);
		MultiByteToWideChar(codepage, 0, reinterpret_cast<const char*>(src.c_str()), -1, &decoded_string[0], required_size);
		return &decoded_string[0];
	#else // !WIN32
		iconv_t conv = iconv_t();
		conv = iconv_open(Charset_wchar_t(), CharsetToString(charset));
		if(!conv)
		{
			throw std::runtime_error("iconv conversion not working");
		}
		std::vector<char> encoded_string(reinterpret_cast<const char*>(src.c_str()), reinterpret_cast<const char*>(src.c_str()) + src.length() + 1);
		std::vector<wchar_t> wide_string(encoded_string.size() * 8); // large enough
		char * inbuf = &encoded_string[0];
		size_t inbytesleft = encoded_string.size();
		char * outbuf = (char*)&wide_string[0];
		size_t outbytesleft = wide_string.size() * sizeof(wchar_t);
		while(iconv(conv, &inbuf, &inbytesleft, &outbuf, &outbytesleft) == (size_t)-1)
		{
			if(errno == EILSEQ || errno == EILSEQ)
			{
				inbuf++;
				inbytesleft--;
				for(std::size_t i = 0; i < sizeof(wchar_t); ++i)
				{
					outbuf[i] = 0;
				}
				#ifdef MPT_PLATFORM_BIG_ENDIAN
					outbuf[sizeof(wchar_t)-1 - 1] = 0xff; outbuf[sizeof(wchar_t)-1 - 0] = 0xfd;
				#else
					outbuf[1] = 0xff; outbuf[0] = 0xfd;
				#endif
				outbuf += sizeof(wchar_t);
				outbytesleft -= sizeof(wchar_t);
				iconv(conv, NULL, NULL, NULL, NULL); // reset state
			} else
			{
				iconv_close(conv);
				conv = iconv_t();
				return std::wstring();
			}
		}
		iconv_close(conv);
		conv = iconv_t();
		return &wide_string[0];
	#endif // WIN32
}


// templated on 8bit strings because of type-safe variants
template<typename Tdststring, typename Tsrcstring>
Tdststring ConvertImpl(Charset to, Charset from, const Tsrcstring &src)
{
	STATIC_ASSERT(sizeof(typename Tdststring::value_type) == sizeof(char));
	STATIC_ASSERT(sizeof(typename Tsrcstring::value_type) == sizeof(char));
	if(to == from)
	{
		return Tdststring(reinterpret_cast<const typename Tdststring::value_type*>(&*src.begin()), reinterpret_cast<const typename Tdststring::value_type*>(&*src.end()));
	}
	#if defined(MPT_CHARSET_CPP) || defined(MPT_CHARSET_CUSTOMUTF8) || defined(WIN32)
		return EncodeImpl<Tdststring>(to, DecodeImpl(from, src));
	#else // !WIN32
		if(to == CharsetCP437AMS || to == CharsetCP437AMS2 || from == CharsetCP437AMS || from == CharsetCP437AMS2)
		{
			return EncodeImpl<Tdststring>(to, DecodeImpl(from, src));
		}
		iconv_t conv = iconv_t();
		conv = iconv_open(CharsetToStringTranslit(to), CharsetToString(from));
		if(!conv)
		{
			conv = iconv_open(CharsetToString(to), CharsetToString(from));
			if(!conv)
			{
				throw std::runtime_error("iconv conversion not working");
			}
		}
		std::vector<char> src_string(reinterpret_cast<const char*>(src.c_str()), reinterpret_cast<const char*>(src.c_str()) + src.length() + 1);
		std::vector<char> dst_string(src_string.size() * 8); // large enough
		char * inbuf = &src_string[0];
		size_t inbytesleft = src_string.size();
		char * outbuf = &dst_string[0];
		size_t outbytesleft = dst_string.size();
		while(iconv(conv, &inbuf, &inbytesleft, &outbuf, &outbytesleft) == (size_t)-1)
		{
			if(errno == EILSEQ || errno == EILSEQ)
			{
				inbuf++;
				inbytesleft--;
				outbuf[0] = '?';
				outbuf++;
				outbytesleft--;
				iconv(conv, NULL, NULL, NULL, NULL); // reset state
			} else
			{
				iconv_close(conv);
				conv = iconv_t();
				return Tdststring();
			}
		}
		iconv_close(conv);
		conv = iconv_t();
		return reinterpret_cast<const typename Tdststring::value_type*>(&dst_string[0]);
	#endif // WIN32
}


} // namespace String


std::wstring ToWide(Charset from, const std::string &str)
{
	return String::DecodeImpl(from, str);
}

std::string ToLocale(const std::wstring &str)
{
	return String::EncodeImpl<std::string>(CharsetLocale, str);
}
std::string ToLocale(Charset from, const std::string &str)
{
	return String::ConvertImpl<std::string>(CharsetLocale, from, str);
}

std::string To(Charset to, const std::wstring &str)
{
	return String::EncodeImpl<std::string>(to, str);
}
std::string To(Charset to, Charset from, const std::string &str)
{
	return String::ConvertImpl<std::string>(to, from, str);
}


#if defined(_MFC_VER)

CString ToCString(const std::wstring &str)
{
	#ifdef UNICODE
		return str.c_str();
	#else
		return ToLocale(str).c_str();
	#endif
}
CString ToCString(Charset from, const std::string &str)
{
	#ifdef UNICODE
		return ToWide(from, str).c_str();
	#else
		return ToLocale(from, str).c_str();
	#endif
}
std::wstring ToWide(const CString &str)
{
	#ifdef UNICODE
		return str.GetString();
	#else
		return ToWide(CharsetLocale, str.GetString());
	#endif
}
std::string ToLocale(const CString &str)
{
	#ifdef UNICODE
		return ToLocale(str.GetString());
	#else
		return str.GetString();
	#endif
}
std::string To(Charset to, const CString &str)
{
	#ifdef UNICODE
		return To(to, str.GetString());
	#else
		return To(to, CharsetLocale, str.GetString());
	#endif
}

#endif


} // namespace mpt



namespace mpt
{


template<typename Tstream, typename T> inline void SaneInsert(Tstream & s, const T & x) { s << x; }
// do the right thing for signed/unsigned char and bool
template<typename Tstream> void SaneInsert(Tstream & s, const bool & x) { s << static_cast<int>(x); }
template<typename Tstream> void SaneInsert(Tstream & s, const signed char & x) { s << static_cast<signed int>(x); }
template<typename Tstream> void SaneInsert(Tstream & s, const unsigned char & x) { s << static_cast<unsigned int>(x); }
 
template<typename T>
inline std::string ToStringHelper(const T & x)
{
	std::ostringstream o;
	o.imbue(std::locale::classic());
	SaneInsert(o, x);
	return o.str();
}

template<typename T>
inline std::wstring ToWStringHelper(const T & x)
{
	std::wostringstream o;
	o.imbue(std::locale::classic());
	SaneInsert(o, x);
	return o.str();
}

std::string ToString(const std::wstring & x) { return mpt::ToLocale(x); }
std::string ToString(const wchar_t * const & x) { return mpt::ToLocale(x); };
std::string ToString(const char & x) { return std::string(1, x); }
std::string ToString(const wchar_t & x) { return mpt::ToLocale(std::wstring(1, x)); }
std::string ToString(const bool & x) { return ToStringHelper(x); }
std::string ToString(const signed char & x) { return ToStringHelper(x); }
std::string ToString(const unsigned char & x) { return ToStringHelper(x); }
std::string ToString(const signed short & x) { return ToStringHelper(x); }
std::string ToString(const unsigned short & x) { return ToStringHelper(x); }
std::string ToString(const signed int & x) { return ToStringHelper(x); }
std::string ToString(const unsigned int & x) { return ToStringHelper(x); }
std::string ToString(const signed long & x) { return ToStringHelper(x); }
std::string ToString(const unsigned long & x) { return ToStringHelper(x); }
std::string ToString(const signed long long & x) { return ToStringHelper(x); }
std::string ToString(const unsigned long long & x) { return ToStringHelper(x); }
std::string ToString(const float & x) { return ToStringHelper(x); }
std::string ToString(const double & x) { return ToStringHelper(x); }
std::string ToString(const long double & x) { return ToStringHelper(x); }

std::wstring ToWString(const std::string & x) { return mpt::ToWide(mpt::CharsetLocale, x); }
std::wstring ToWString(const char * const & x) { return mpt::ToWide(mpt::CharsetLocale, x); }
std::wstring ToWString(const char & x) { return mpt::ToWide(mpt::CharsetLocale, std::string(1, x)); }
std::wstring ToWString(const wchar_t & x) { return std::wstring(1, x); }
std::wstring ToWString(const bool & x) { return ToWStringHelper(x); }
std::wstring ToWString(const signed char & x) { return ToWStringHelper(x); }
std::wstring ToWString(const unsigned char & x) { return ToWStringHelper(x); }
std::wstring ToWString(const signed short & x) { return ToWStringHelper(x); }
std::wstring ToWString(const unsigned short & x) { return ToWStringHelper(x); }
std::wstring ToWString(const signed int & x) { return ToWStringHelper(x); }
std::wstring ToWString(const unsigned int & x) { return ToWStringHelper(x); }
std::wstring ToWString(const signed long & x) { return ToWStringHelper(x); }
std::wstring ToWString(const unsigned long & x) { return ToWStringHelper(x); }
std::wstring ToWString(const signed long long & x) { return ToWStringHelper(x); }
std::wstring ToWString(const unsigned long long & x) { return ToWStringHelper(x); }
std::wstring ToWString(const float & x) { return ToWStringHelper(x); }
std::wstring ToWString(const double & x) { return ToWStringHelper(x); }
std::wstring ToWString(const long double & x) { return ToWStringHelper(x); }


#if defined(MPT_FMT)


template<typename Tostream>
inline void ApplyFormat(Tostream & o, const Format & format)
{
	FormatFlags f = format.GetFlags();
	std::size_t width = format.GetWidth();
	int precision = format.GetPrecision();
	if(precision != -1 && width != 0 && !(f & fmt::NotaFix) && !(f & fmt::NotaSci))
	{
		// fixup:
		// precision behaves differently from .#
		// avoid default format when precision and width are set
		f &= ~fmt::NotaNrm;
		f |= fmt::NotaFix;
	}
	if(f & fmt::BaseDec) { o << std::dec; }
	else if(f & fmt::BaseHex) { o << std::hex; }
	if(f & fmt::NotaNrm ) { /*nothing*/ }
	else if(f & fmt::NotaFix ) { o << std::setiosflags(std::ios::fixed); }
	else if(f & fmt::NotaSci ) { o << std::setiosflags(std::ios::scientific); }
	if(f & fmt::CaseLow) { o << std::nouppercase; }
	else if(f & fmt::CaseUpp) { o << std::uppercase; }
	if(f & fmt::FillOff) { /* nothing */ }
	else if(f & fmt::FillNul) { o << std::setw(width) << std::setfill(typename Tostream::char_type('0')); }
	else if(f & fmt::FillSpc) { o << std::setw(width) << std::setfill(typename Tostream::char_type(' ')); }
	if(precision != -1) { o << std::setprecision(precision); }
}


template<typename T>
inline std::string FormatValHelper(const T & x, const Format & f)
{
	std::ostringstream o;
	o.imbue(std::locale::classic());
	ApplyFormat(o, f);
	SaneInsert(o, x);
	return o.str();
}

template<typename T>
inline std::wstring FormatValWHelper(const T & x, const Format & f)
{
	std::wostringstream o;
	o.imbue(std::locale::classic());
	ApplyFormat(o, f);
	SaneInsert(o, x);
	return o.str();
}

// Parses a useful subset of standard sprintf syntax for specifying floating point formatting.
template<typename Tchar>
inline Format ParseFormatStringFloat(const Tchar * str)
{
	ASSERT(str);
	FormatFlags f = FormatFlags();
	std::size_t width = 0;
	int precision = -1;
	if(!str)
	{
		return Format();
	}
	const Tchar * p = str;
	while(*p && *p != Tchar('%'))
	{
		++p;
	}
	++p;
	while(*p && (*p == Tchar(' ') || *p == Tchar('0')))
	{
		if(*p == Tchar(' ')) f |= mpt::fmt::FillSpc;
		if(*p == Tchar('0')) f |= mpt::fmt::FillNul;
		++p;
	}
	if(!(f & mpt::fmt::FillSpc) && !(f & mpt::fmt::FillNul))
	{
		f |= mpt::fmt::FillOff;
	}
	while(*p && (Tchar('0') <= *p && *p <= Tchar('9')))
	{
		if(f & mpt::fmt::FillOff)
		{
			f &= ~mpt::fmt::FillOff;
			f |= mpt::fmt::FillSpc;
		}
		width *= 10;
		width += *p - Tchar('0');
		++p;
	}
	if(*p && *p == Tchar('.'))
	{
		++p;
		precision = 0;
		while(*p && (Tchar('0') <= *p && *p <= Tchar('9')))
		{
			precision *= 10;
			precision += *p - Tchar('0');
			++p;
		}
	}
	if(*p && (*p == Tchar('g') || *p == Tchar('G') || *p == Tchar('f') || *p == Tchar('F') || *p == Tchar('e') || *p == Tchar('E')))
	{
		if(*p == Tchar('g')) f |= mpt::fmt::NotaNrm | mpt::fmt::CaseLow;
		if(*p == Tchar('G')) f |= mpt::fmt::NotaNrm | mpt::fmt::CaseUpp;
		if(*p == Tchar('f')) f |= mpt::fmt::NotaFix | mpt::fmt::CaseLow;
		if(*p == Tchar('F')) f |= mpt::fmt::NotaFix | mpt::fmt::CaseUpp;
		if(*p == Tchar('e')) f |= mpt::fmt::NotaSci | mpt::fmt::CaseLow;
		if(*p == Tchar('E')) f |= mpt::fmt::NotaSci | mpt::fmt::CaseUpp;
		++p;
	}
	return Format().SetFlags(f).SetWidth(width).SetPrecision(precision);
}

Format & Format::ParsePrintf(const char * format)
{
	*this = ParseFormatStringFloat(format);
	return *this;
}
Format & Format::ParsePrintf(const wchar_t * format)
{
	*this = ParseFormatStringFloat(format);
	return *this;
}
Format & Format::ParsePrintf(const std::string & format)
{
	*this = ParseFormatStringFloat(format.c_str());
	return *this;
}
Format & Format::ParsePrintf(const std::wstring & format)
{
	*this = ParseFormatStringFloat(format.c_str());
	return *this;
}


std::string FormatVal(const char & x, const Format & f) { return FormatValHelper(x, f); }
std::string FormatVal(const wchar_t & x, const Format & f) { return FormatValHelper(x, f); }
std::string FormatVal(const bool & x, const Format & f) { return FormatValHelper(x, f); }
std::string FormatVal(const signed char & x, const Format & f) { return FormatValHelper(x, f); }
std::string FormatVal(const unsigned char & x, const Format & f) { return FormatValHelper(x, f); }
std::string FormatVal(const signed short & x, const Format & f) { return FormatValHelper(x, f); }
std::string FormatVal(const unsigned short & x, const Format & f) { return FormatValHelper(x, f); }
std::string FormatVal(const signed int & x, const Format & f) { return FormatValHelper(x, f); }
std::string FormatVal(const unsigned int & x, const Format & f) { return FormatValHelper(x, f); }
std::string FormatVal(const signed long & x, const Format & f) { return FormatValHelper(x, f); }
std::string FormatVal(const unsigned long & x, const Format & f) { return FormatValHelper(x, f); }
std::string FormatVal(const signed long long & x, const Format & f) { return FormatValHelper(x, f); }
std::string FormatVal(const unsigned long long & x, const Format & f) { return FormatValHelper(x, f); }

std::string FormatVal(const float & x, const Format & f) { return FormatValHelper(x, f); }
std::string FormatVal(const double & x, const Format & f) { return FormatValHelper(x, f); }
std::string FormatVal(const long double & x, const Format & f) { return FormatValHelper(x, f); }

std::wstring FormatValW(const char & x, const Format & f) { return FormatValWHelper(x, f); }
std::wstring FormatValW(const wchar_t & x, const Format & f) { return FormatValWHelper(x, f); }
std::wstring FormatValW(const bool & x, const Format & f) { return FormatValWHelper(x, f); }
std::wstring FormatValW(const signed char & x, const Format & f) { return FormatValWHelper(x, f); }
std::wstring FormatValW(const unsigned char & x, const Format & f) { return FormatValWHelper(x, f); }
std::wstring FormatValW(const signed short & x, const Format & f) { return FormatValWHelper(x, f); }
std::wstring FormatValW(const unsigned short & x, const Format & f) { return FormatValWHelper(x, f); }
std::wstring FormatValW(const signed int & x, const Format & f) { return FormatValWHelper(x, f); }
std::wstring FormatValW(const unsigned int & x, const Format & f) { return FormatValWHelper(x, f); }
std::wstring FormatValW(const signed long & x, const Format & f) { return FormatValWHelper(x, f); }
std::wstring FormatValW(const unsigned long & x, const Format & f) { return FormatValWHelper(x, f); }
std::wstring FormatValW(const signed long long & x, const Format & f) { return FormatValWHelper(x, f); }
std::wstring FormatValW(const unsigned long long & x, const Format & f) { return FormatValWHelper(x, f); }

std::wstring FormatValW(const float & x, const Format & f) { return FormatValWHelper(x, f); }
std::wstring FormatValW(const double & x, const Format & f) { return FormatValWHelper(x, f); }
std::wstring FormatValW(const long double & x, const Format & f) { return FormatValWHelper(x, f); }


#endif // MPT_FMT


namespace String
{


namespace detail
{

template<typename Tstring>
Tstring PrintImplTemplate(const Tstring & format
	, const Tstring & x1
	, const Tstring & x2
	, const Tstring & x3
	, const Tstring & x4
	, const Tstring & x5
	, const Tstring & x6
	, const Tstring & x7
	, const Tstring & x8
	)
{
	Tstring result;
	const std::size_t len = format.length();
	for(std::size_t pos = 0; pos != len; ++pos)
	{
		typename Tstring::value_type c = format[pos];
		if(pos + 1 != len && c == '%')
		{
			pos++;
			c = format[pos];
			if('1' <= c && c <= '9')
			{
				const std::size_t n = c - '0';
				switch(n)
				{
					case 1: result.append(x1); break;
					case 2: result.append(x2); break;
					case 3: result.append(x3); break;
					case 4: result.append(x4); break;
					case 5: result.append(x5); break;
					case 6: result.append(x6); break;
					case 7: result.append(x7); break;
					case 8: result.append(x8); break;
				}
				continue;
			} else if(c != '%')
			{
				result.append(1, '%');
			}
		}
		result.append(1, c);
	}
	return result;
}

std::string PrintImpl(const std::string & format
	, const std::string & x1
	, const std::string & x2
	, const std::string & x3
	, const std::string & x4
	, const std::string & x5
	, const std::string & x6
	, const std::string & x7
	, const std::string & x8
	)
{
	return PrintImplTemplate<std::string>(format, x1,x2,x3,x4,x5,x6,x7,x8);
}

std::wstring PrintImplW(const std::wstring & format
	, const std::wstring & x1
	, const std::wstring & x2
	, const std::wstring & x3
	, const std::wstring & x4
	, const std::wstring & x5
	, const std::wstring & x6
	, const std::wstring & x7
	, const std::wstring & x8
	)
{
	return PrintImplTemplate<std::wstring>(format, x1,x2,x3,x4,x5,x6,x7,x8);
}

} // namespace detail


} // namespace String


} // namespace mpt
