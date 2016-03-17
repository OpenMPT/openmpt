/*
 * Load_mo3.cpp
 * ------------
 * Purpose: MO3 module loader.
 * Notes  : OpenMPT has its own built-in decoder (enabled with MPT_ENABLE_MO3_BUILTIN),
 *          but can also make use of the official, closed-source library (unmo3.dll / libunmo3.so).
 * Authors: Johannes Schultz / OpenMPT Devs
 *          Based on documentation and the decompression routines from the
 *          open-source UNMO3 project (https://github.com/lclevy/unmo3).
 *          The modified decompression code has been relicensed to the BSD
 *          license with permission from Laurent Clévy.
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"
#include "../common/ComponentManager.h"

#ifdef MPT_ENABLE_MO3_BUILTIN
#include "Tables.h"
#include "../common/version.h"

#include "MPEGFrame.h"
#include "OggStream.h"
#if defined(MPT_WITH_VORBIS) && defined(MPT_WITH_VORBISFILE)
#include "../common/mptBufferIO.h"
#endif

#if defined(MPT_WITH_VORBIS)
#include <vorbis/codec.h>
#endif

#if defined(MPT_WITH_VORBISFILE)
#include <vorbis/vorbisfile.h>
#include "SampleFormatConverters.h"
#endif

#ifdef MPT_WITH_STBVORBIS
#include <stb_vorbis/stb_vorbis.c>
#include "SampleFormatConverters.h"
#endif // MPT_WITH_STBVORBIS

#endif // MPT_ENABLE_MO3_BUILTIN

#if defined(MPT_WITH_UNMO3) || defined(MPT_ENABLE_UNMO3_DYNBIND)
// unmo3.h
#if MPT_OS_WINDOWS
#define UNMO3_API __stdcall
#else
#define UNMO3_API
#endif
#if !defined(MPT_ENABLE_UNMO3_DYNBIND)
extern "C" {
OPENMPT_NAMESPACE::uint32 UNMO3_API UNMO3_GetVersion(void);
void UNMO3_API UNMO3_Free(const void *data);
OPENMPT_NAMESPACE::int32 UNMO3_API UNMO3_Decode(const void **data, OPENMPT_NAMESPACE::uint32 *len, OPENMPT_NAMESPACE::uint32 flags);
}
#endif // !MPT_ENABLE_UNMO3_DYNBIND
#endif // MPT_WITH_UNMO3 || MPT_ENABLE_UNMO3_DYNBIND


OPENMPT_NAMESPACE_BEGIN


#if defined(MPT_WITH_UNMO3) || defined(MPT_ENABLE_UNMO3_DYNBIND)

class ComponentUnMO3 : public ComponentLibrary
{
	MPT_DECLARE_COMPONENT_MEMBERS
public:
	uint32 (UNMO3_API * UNMO3_GetVersion)();
	// Decode a MO3 file (returns the same "exit codes" as UNMO3.EXE, eg. 0=success)
	// IN: data/len = MO3 data/len
	// OUT: data/len = decoded data/len (if successful)
	// flags & 1: Don't load samples
	int32 (UNMO3_API * UNMO3_Decode_Old)(const void **data, uint32 *len);
	int32 (UNMO3_API * UNMO3_Decode_New)(const void **data, uint32 *len, uint32 flags);
	// Free the data returned by UNMO3_Decode
	void (UNMO3_API * UNMO3_Free)(const void *data);
	int32 UNMO3_Decode(const void **data, uint32 *len, uint32 flags)
	{
		return (UNMO3_Decode_New ? UNMO3_Decode_New(data, len, flags) : UNMO3_Decode_Old(data, len));
	}
public:
	ComponentUnMO3() : ComponentLibrary(ComponentTypeForeign) { }
	bool DoInitialize()
	{
#if !defined(MPT_ENABLE_UNMO3_DYNBIND)
		UNMO3_GetVersion = &(::UNMO3_GetVersion);
		UNMO3_Free = &(::UNMO3_Free);
		UNMO3_Decode_Old = nullptr;
		UNMO3_Decode_New = &(::UNMO3_Decode);
		return true;
#else
		AddLibrary("unmo3", mpt::LibraryPath::App(MPT_PATHSTRING("unmo3")));
		MPT_COMPONENT_BIND("unmo3", UNMO3_Free);
		if(MPT_COMPONENT_BIND_OPTIONAL("unmo3", UNMO3_GetVersion))
		{
			UNMO3_Decode_Old = nullptr;
			MPT_COMPONENT_BIND_SYMBOL("unmo3", "UNMO3_Decode", UNMO3_Decode_New);
		} else
		{
			// Old API version: No "flags" parameter.
			UNMO3_Decode_New = nullptr;
			MPT_COMPONENT_BIND_SYMBOL("unmo3", "UNMO3_Decode", UNMO3_Decode_Old);
		}
		return !HasBindFailed();
#endif
	}
};
MPT_REGISTERED_COMPONENT(ComponentUnMO3, "UnMO3")

#endif // MPT_WITH_UNMO3 || MPT_ENABLE_UNMO3_DYNBIND


#ifdef MPT_ENABLE_MO3_BUILTIN

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(push, 1)
#endif

struct PACKED MO3FileHeader
{
	enum MO3HeaderFlags
	{
		linearSlides	= 0x0001,
		isS3M			= 0x0002,
		s3mFastSlides	= 0x0004,
		isMTM			= 0x0008,	// Actually this is simply "not XM". But if none of the S3M, MOD and IT flags are set, it's an MTM.
		s3mAmigaLimits	= 0x0010,
		// 0x20 and 0x40 have been used in old versions for things that can be inferred from the file format anyway.
		// The official UNMO3 ignores them.
		isMOD			= 0x0080,
		isIT			= 0x0100,
		instrumentMode	= 0x0200,
		itCompatGxx		= 0x0400,
		itOldFX			= 0x0800,
		modplugMode		= 0x10000,
		unknown			= 0x20000,	// Always set
		modVBlank		= 0x80000,
		hasPlugins		= 0x100000,
		extFilterRange	= 0x200000,
	};

	uint8  numChannels;		// 1...64 (limited by channel panning and volume)
	uint16 numOrders;
	uint16 restartPos;
	uint16 numPatterns;
	uint16 numTracks;
	uint16 numInstruments;
	uint16 numSamples;
	uint8  defaultSpeed;
	uint8  defaultTempo;
	uint32 flags;			// See MO3HeaderFlags
	uint8  globalVol;		// 0...128 in IT, 0...64 in S3M
	uint8  panSeparation;	// 0...128 in IT
	int8   sampleVolume;	// Only used in IT
	uint8  chnVolume[64];	// 0...64
	uint8  chnPan[64];		// 0...256, 127 = surround
	uint8  sfxMacros[16];
	uint8  fixedMacros[128][2];

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(numOrders);
		SwapBytesLE(restartPos);
		SwapBytesLE(numPatterns);
		SwapBytesLE(numTracks);
		SwapBytesLE(numInstruments);
		SwapBytesLE(numSamples);
		SwapBytesLE(flags);
	}
};

STATIC_ASSERT(sizeof(MO3FileHeader) == 422);


struct PACKED MO3Envelope
{
	enum MO3EnvelopeFlags
	{
		envEnabled	= 0x01,
		envSustain	= 0x02,
		envLoop		= 0x04,
		envFilter	= 0x10,
		envCarry	= 0x20,
	};

	uint8 flags;			// See MO3EnvelopeFlags
	uint8 numNodes;
	uint8 sustainStart;
	uint8 sustainEnd;
	uint8 loopStart;
	uint8 loopEnd;
	int16 points[25][2];

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		for(size_t i = 0; i < CountOf(points); i++)
		{
			SwapBytesLE(points[i][0]);
			SwapBytesLE(points[i][1]);
		}
	}

	// Convert MO3 envelope data into OpenMPT's internal envelope format
	void ConvertToMPT(InstrumentEnvelope &mptEnv, uint8 envShift) const
	{
		if(flags & envEnabled) mptEnv.dwFlags.set(ENV_ENABLED);
		if(flags & envSustain) mptEnv.dwFlags.set(ENV_SUSTAIN);
		if(flags & envLoop) mptEnv.dwFlags.set(ENV_LOOP);
		if(flags & envFilter) mptEnv.dwFlags.set(ENV_FILTER);
		if(flags & envCarry) mptEnv.dwFlags.set(ENV_CARRY);
		mptEnv.nNodes = std::min<uint8>(numNodes, 25);
		mptEnv.nSustainStart = sustainStart;
		mptEnv.nSustainEnd = sustainEnd;
		mptEnv.nLoopStart = loopStart;
		mptEnv.nLoopEnd = loopEnd;
		for(uint32 ev = 0; ev < mptEnv.nNodes; ev++)
		{
			mptEnv.Ticks[ev] = points[ev][0];
			if(ev > 0 && mptEnv.Ticks[ev] < mptEnv.Ticks[ev - 1])
				mptEnv.Ticks[ev] = mptEnv.Ticks[ev - 1] + 1;
			mptEnv.Values[ev] = static_cast<uint8>(Clamp(points[ev][1] >> envShift, 0, 64));
		}
	}
};

STATIC_ASSERT(sizeof(MO3Envelope) == 106);


struct PACKED MO3Instrument
{
	enum MO3InstrumentFlags
	{
		playOnMIDI  = 0x01,
		mute		= 0x02,
	};

	uint32 flags;		// See MO3InstrumentFlags
	uint16 sampleMap[120][2];
	MO3Envelope volEnv;
	MO3Envelope panEnv;
	MO3Envelope pitchEnv;
	struct XMVibratoSettings
	{
		uint8  type;
		uint8  sweep;
		uint8  depth;
		uint8  rate;
	} vibrato;			// Applies to all samples of this instrument (XM)
	uint16 fadeOut;
	uint8  midiChannel;
	uint8  midiBank;
	uint8  midiPatch;
	uint8  midiBend;
	uint8  globalVol;	// 0...128
	uint16 panning;		// 0...256 if enabled, 0xFFFF otherwise
	uint8  nna;
	uint8  pps;
	uint8  ppc;
	uint8  dct;
	uint8  dca;
	uint16 volSwing;	// 0...100
	uint16 panSwing;	// 0...256
	uint8  cutoff;		// 0...127, + 128 if enabled
	uint8  resonance;	// 0...127, + 128 if enabled

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		for(size_t i = 0; i < CountOf(sampleMap); i++)
		{
			SwapBytesLE(sampleMap[i][0]);
			SwapBytesLE(sampleMap[i][1]);
		}

		volEnv.ConvertEndianness();
		panEnv.ConvertEndianness();
		pitchEnv.ConvertEndianness();
		SwapBytesLE(fadeOut);
		SwapBytesLE(panning);
		SwapBytesLE(volSwing);
		SwapBytesLE(panSwing);
	}

	// Convert MO3 instrument data into OpenMPT's internal instrument format
	void ConvertToMPT(ModInstrument &mptIns, MODTYPE type) const
	{
		if(type == MOD_TYPE_XM)
		{
			for(size_t i = 0; i < 96; i++)
			{
				mptIns.Keyboard[i + 12] = sampleMap[i][1] + 1;
			}
		} else
		{
			for(size_t i = 0; i < 120; i++)
			{
				mptIns.NoteMap[i] = static_cast<uint8>(sampleMap[i][0] + NOTE_MIN);
				mptIns.Keyboard[i] = sampleMap[i][1] + 1;
			}
		}
		volEnv.ConvertToMPT(mptIns.VolEnv, 0);
		panEnv.ConvertToMPT(mptIns.PanEnv, 0);
		pitchEnv.ConvertToMPT(mptIns.PitchEnv, 5);
		mptIns.nFadeOut = fadeOut;
		if(midiChannel >= 128)
		{
			// Plugin
			mptIns.nMixPlug = midiChannel - 127;
		} else if(midiChannel < 17 && (flags & playOnMIDI))
		{
			// XM / IT with recent encoder
			mptIns.nMidiChannel = midiChannel + MidiFirstChannel;
		} else if(midiChannel > 0 && midiChannel < 17)
		{
			// IT with old encoder (yes, channel 0 is represented the same way as "no channel")
			mptIns.nMidiChannel = midiChannel + MidiFirstChannel;
		}
		mptIns.wMidiBank = midiBank;
		mptIns.nMidiProgram = midiPatch;
		mptIns.midiPWD =  midiBend;
		if(type == MOD_TYPE_IT)
			mptIns.nGlobalVol = std::min<uint8>(globalVol, 128) / 2u;
		if(panning <= 256)
		{
			mptIns.nPan = panning;
			mptIns.dwFlags.set(INS_SETPANNING);
		}
		mptIns.nNNA = nna;
		mptIns.nPPS = pps;
		mptIns.nPPC = ppc;
		mptIns.nDCT = dct;
		mptIns.nDNA = dca;
		mptIns.nVolSwing = static_cast<uint8>(std::min(volSwing, uint16(100)));
		mptIns.nPanSwing = static_cast<uint8>(std::min(panSwing, uint16(64)) / 4u);
		mptIns.SetCutoff(cutoff & 0x7F, (cutoff & 0x80) != 0);
		mptIns.SetResonance(resonance & 0x7F, (resonance & 0x80) != 0);
	}
};

STATIC_ASSERT(sizeof(MO3Instrument) == 826);


struct PACKED MO3Sample
{
	enum MO3SampleFlags
	{
		smp16Bit			= 0x01,
		smpLoop				= 0x10,
		smpPingPongLoop		= 0x20,
		smpSustain			= 0x100,
		smpSustainPingPong	= 0x200,
		smpStereo			= 0x400,
		smpCompressionMPEG	= 0x1000,					// MPEG 1.0 / 2.0 / 2.5 sample
		smpCompressionOgg	= 0x1000 | 0x2000,			// Ogg sample
		smpSharedOgg		= 0x1000 | 0x2000 | 0x4000,	// Ogg sample with shared vorbis header
		smpDeltaCompression	= 0x2000,					// Deltas + compression
		smpDeltaPrediction	= 0x4000,					// Delta prediction + compression
		smpCompressionMask	= 0x1000 | 0x2000 | 0x4000
	};

	int32  freqFinetune;	// Frequency in S3M and IT, finetune (0...255) in MOD, MTM, XM
	int8   transpose;
	uint8  defaultVolume;	// 0...64
	uint16 panning;			// 0...256 if enabled, 0xFFFF otherwise
	uint32 length;
	uint32 loopStart;
	uint32 loopEnd;
	uint16 flags;			// See MO3SampleFlags
	uint8  vibType;
	uint8  vibSweep;
	uint8  vibDepth;
	uint8  vibRate;
	uint8  globalVol;		// 0...64 in IT, in XM it represents the instrument number
	uint32 sustainStart;
	uint32 sustainEnd;
	int32  compressedSize;
	uint16 encoderDelay;	// MP3: Ignore first n bytes of decoded output. Ogg: Shared Ogg header size

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(freqFinetune);
		SwapBytesLE(panning);
		SwapBytesLE(length);
		SwapBytesLE(loopStart);
		SwapBytesLE(loopEnd);
		SwapBytesLE(flags);
		SwapBytesLE(sustainStart);
		SwapBytesLE(sustainEnd);
		SwapBytesLE(compressedSize);
		SwapBytesLE(encoderDelay);
	}

	// Convert MO3 sample data into OpenMPT's internal instrument format
	void ConvertToMPT(ModSample &mptSmp, MODTYPE type, bool frequencyIsHertz) const
	{
		mptSmp.Initialize();
		if(type & (MOD_TYPE_IT | MOD_TYPE_S3M))
		{
			if(frequencyIsHertz)
				mptSmp.nC5Speed = static_cast<uint32>(freqFinetune);
			else
				mptSmp.nC5Speed = Util::Round<uint32>(8363.0 * std::pow(2.0, (freqFinetune + 1408) / 1536.0));
		} else
		{
			mptSmp.nFineTune = static_cast<int8>(freqFinetune);
			if(type != MOD_TYPE_MTM) mptSmp.nFineTune -= 128;
			mptSmp.RelativeTone = transpose;
		}
		mptSmp.nVolume = std::min(defaultVolume, uint8(64)) * 4u;
		if(panning <= 256)
		{
			mptSmp.nPan = panning;
			mptSmp.uFlags.set(CHN_PANNING);
		}
		mptSmp.nLength = length;
		mptSmp.nLoopStart = loopStart;
		mptSmp.nLoopEnd = loopEnd;
		if(flags & smpLoop) mptSmp.uFlags.set(CHN_LOOP);
		if(flags & smpPingPongLoop) mptSmp.uFlags.set(CHN_PINGPONGLOOP);
		if(flags & smpSustain) mptSmp.uFlags.set(CHN_SUSTAINLOOP);
		if(flags & smpSustainPingPong) mptSmp.uFlags.set(CHN_PINGPONGSUSTAIN);

		mptSmp.nVibType = AutoVibratoIT2XM[vibType & 7];
		mptSmp.nVibSweep = vibSweep;
		mptSmp.nVibDepth = vibDepth;
		mptSmp.nVibRate = vibRate;

		if(type == MOD_TYPE_IT)
			mptSmp.nGlobalVol = std::min(globalVol, uint8(64));
		mptSmp.nSustainStart = sustainStart;
		mptSmp.nSustainEnd = sustainEnd;
	}
};

STATIC_ASSERT(sizeof(MO3Sample) == 41);


#ifdef NEEDS_PRAGMA_PACK
#pragma pack(pop)
#endif


// We need all this information for Ogg-compressed samples with shared headers:
// A shared header can be taken from a sample that has not been read yet, so
// we first need to read all headers, and then load the Ogg samples afterwards.
struct MO3SampleChunk
{
	FileReader chunk;
	uint16 headerSize;
	int16 sharedHeader;
	MO3SampleChunk(const FileReader &chunk_ = FileReader(), uint16 headerSize_ = 0, int16 sharedHeader_ = 0)
		: chunk(chunk_), headerSize(headerSize_), sharedHeader(sharedHeader_) { }
};


// Unpack macros

// shift control bits until it is empty:
// a 0 bit means literal : the next data byte is copied
// a 1 means compressed data
// then the next 2 bits determines what is the LZ ptr
// ('00' same as previous, else stored in stream)

#define READ_CTRL_BIT \
	data <<= 1; \
	carry = (data > 0xFF); \
	data &= 0xFF; \
	if(data == 0) \
	{ \
		data = file.ReadUint8(); \
		data = (data << 1) + 1; \
		carry = (data > 0xFF); \
		data &= 0xFF; \
	}

// length coded within control stream:
// most significant bit is 1
// than the first bit of each bits pair (noted n1),
// until second bit is 0 (noted n0)

#define DECODE_CTRL_BITS \
{ \
	strLen++; \
	do { \
		READ_CTRL_BIT; \
		strLen = (strLen << 1) + carry; \
		READ_CTRL_BIT; \
	} while(carry); \
}

static bool UnpackMO3Data(FileReader &file, uint8 *dst, uint32 size)
//------------------------------------------------------------------
{
	if(!size)
	{
		return false;
	}

	uint16 data = 0;
	int8 carry = 0;			// x86 carry (used to propagate the most significant bit from one byte to another)
	int32 strLen = 0;		// length of previous string
	int32 strOffset;		// string offset
	uint8 *initDst = dst;
	uint32 ebp, previousPtr = 0;
	uint32 initSize = size;

	// Read first uncompressed byte
	*dst++ = file.ReadUint8();
	size--;

	while(size > 0)
	{
		READ_CTRL_BIT;
		if(!carry)
		{
			// a 0 ctrl bit means 'copy', not compressed byte
			*dst++ = file.ReadUint8();
			size--;
		} else
		{
			// a 1 ctrl bit means compressed bytes are following
			ebp = 0; // lenth adjustment
			DECODE_CTRL_BITS; // read length, and if strLen > 3 (coded using more than 1 bits pair) also part of the offset value
			strLen -=3;
			if(strLen < 0)
			{
				// means LZ ptr with same previous relative LZ ptr (saved one)
				strOffset = previousPtr;	// restore previous Ptr
				strLen++;
			} else
			{
				// LZ ptr in ctrl stream
				strOffset = (strLen << 8) | file.ReadUint8(); // read less significant offset byte from stream
				strLen = 0;
				strOffset = ~strOffset;
				if(strOffset < -1280)
					ebp++;
				ebp++;	// length is always at least 1
				if(strOffset < -32000)
					ebp++;
				previousPtr = strOffset; // save current Ptr
			}

			// read the next 2 bits as part of strLen
			READ_CTRL_BIT;
			strLen = (strLen << 1) + carry;
			READ_CTRL_BIT;
			strLen = (strLen << 1) + carry;
			if(strLen == 0)
			{
				// length does not fit in 2 bits
				DECODE_CTRL_BITS;	// decode length: 1 is the most significant bit,
				strLen += 2;		// then first bit of each bits pairs (noted n1), until n0.
			}
			strLen += ebp; // length adjustment
			if(size >= static_cast<uint32>(strLen) && strLen > 0)
			{
				// Copy previous string
				if(strOffset >= 0 || static_cast<std::ptrdiff_t>(dst - initDst) + strOffset < 0)
				{
					break;
				}
				size -= strLen;
				const uint8 *string = dst + strOffset;
				while(strLen > 0)
				{
					*dst++ = *string++;
					strLen--;
				}
			} else
			{
				break;
			}
		}
	}
#ifdef MPT_BUILD_FUZZER
	// When using a fuzzer, we should not care if the decompressed buffer has the correct size.
	// This makes finding new interesting test cases much easier.
	while(size-- > 0)
	{
		*dst++ = 0;
	}
#endif
	return (dst - initDst) == static_cast<std::ptrdiff_t>(initSize);
}


struct MO3Delta8BitParams
{
	typedef int8 sample_t;
	typedef uint8 unsigned_t;
	static const int shift = 7;
	static const uint8 dhInit = 4;

	static inline void Decode(FileReader &file, int8 &carry, uint16 &data, uint8 &/*dh*/, unsigned_t &val)
	{
		do
		{
			READ_CTRL_BIT;
			val = (val << 1) + carry;
			READ_CTRL_BIT;
		} while(carry);
	}
};

struct MO3Delta16BitParams
{
	typedef int16 sample_t;
	typedef uint16 unsigned_t;
	static const int shift = 15;
	static const uint8 dhInit = 8;

	static inline void Decode(FileReader &file, int8 &carry, uint16 &data, uint8 &dh, unsigned_t &val)
	{
		if(dh < 5)
		{
			do
			{
				READ_CTRL_BIT;
				val = (val << 1) + carry;
				READ_CTRL_BIT;
				val = (val << 1) + carry;
				READ_CTRL_BIT; \
			} while(carry);
		} else
		{
			do
			{
				READ_CTRL_BIT;
				val = (val << 1) + carry;
				READ_CTRL_BIT;
			} while(carry);
		}
	}
};


template<typename Properties>
static void UnpackMO3DeltaSample(FileReader &file, typename Properties::sample_t *dst, uint32 length, uint8 numChannels)
//----------------------------------------------------------------------------------------------------------------------
{
	uint8 dh = Properties::dhInit, cl = 0;
	int8 carry = 0;
	uint16 data = 0;
	typename Properties::unsigned_t val;
	typename Properties::sample_t previous = 0;

	for(uint8 chn = 0; chn < numChannels; chn++)
	{
		typename Properties::sample_t *p = dst + chn;
		const typename Properties::sample_t * const pEnd = p + length * numChannels;
		while(p < pEnd)
		{
			val = 0;
			Properties::Decode(file, carry, data, dh, val);
			cl = dh;
			while(cl > 0)
			{
				READ_CTRL_BIT;
				val = (val << 1) + carry;
				cl--;
			}
			cl = 1;
			if(val >= 4)
			{
				cl = Properties::shift;
				while(((1 << cl) & val) == 0 && cl > 1)
					cl--;
			}
			dh = dh + cl;
			dh >>= 1;			// next length in bits of encoded delta second part
			carry = val & 1;	// sign of delta 1=+, 0=not
			val >>= 1;
			if(carry == 0)
				val = ~val;		// negative delta
			val += previous;	// previous value + delta
			*p = val;
			p += numChannels;
			previous = val;
		}
	}
}


template<typename Properties>
static void UnpackMO3DeltaPredictionSample(FileReader &file, typename Properties::sample_t *dst, uint32 length, uint8 numChannels)
//--------------------------------------------------------------------------------------------------------------------------------
{
	uint8 dh = Properties::dhInit, cl = 0;
	int8 carry;
	uint16 data = 0;
	int32 next = 0;
	typename Properties::unsigned_t val = 0;
	typename Properties::sample_t sval = 0, delta = 0, previous = 0;

	for(uint8 chn = 0; chn < numChannels; chn++)
	{
		typename Properties::sample_t *p = dst + chn;
		const typename Properties::sample_t * const pEnd = p + length * numChannels;
		while(p < pEnd)
		{
			val = 0;
			Properties::Decode(file, carry, data, dh, val);
			cl = dh;	// length in bits of: delta second part (right most bits of delta) and sign bit
			while(cl > 0)
			{
				READ_CTRL_BIT;
				val = (val << 1) + carry;
				cl--;
			}
			cl = 1;
			if(val >= 4)
			{
				cl = Properties::shift;
				while(((1 << cl) & val) == 0 && cl > 1)
					cl--;
			}
			dh = dh + cl;
			dh >>= 1;			// next length in bits of encoded delta second part
			carry = val & 1;	// sign of delta 1=+, 0=not
			val >>= 1;
			if(carry == 0)
				val = ~val;		// negative delta

			delta = static_cast<typename Properties::sample_t>(val);
			val = val + static_cast<typename Properties::unsigned_t>(next);	// predicted value + delta
			*p = val;
			p += numChannels;
			sval = static_cast<typename Properties::sample_t>(val);
			next = (sval << 1) + (delta >> 1) - previous;  // corrected next value

			Limit(next, std::numeric_limits<typename Properties::sample_t>::min(), std::numeric_limits<typename Properties::sample_t>::max());

			previous = sval;
		}
	}
}


#undef READ_CTRL_BIT
#undef DECODE_CTRL_BITS


#if defined(MPT_WITH_VORBIS) && defined(MPT_WITH_VORBISFILE)

static size_t VorbisfileFilereaderRead(void *ptr, size_t size, size_t nmemb, void *datasource)
{
	FileReader &file = *reinterpret_cast<FileReader*>(datasource);
	return file.ReadRaw(ptr, size * nmemb) / size;
}

static int VorbisfileFilereaderSeek(void *datasource, ogg_int64_t offset, int whence)
{
	FileReader &file = *reinterpret_cast<FileReader*>(datasource);
	switch(whence)
	{
	case SEEK_SET:
		{
			if(!Util::TypeCanHoldValue<FileReader::off_t>(offset))
			{
				return -1;
			}
			return file.Seek(mpt::saturate_cast<FileReader::off_t>(offset)) ? 0 : -1;
		}
		break;
	case SEEK_CUR:
		{
			if(offset < 0)
			{
				if(offset == std::numeric_limits<ogg_int64_t>::min())
				{
					return -1;
				}
				if(!Util::TypeCanHoldValue<FileReader::off_t>(0-offset))
				{
					return -1;
				}
				return file.SkipBack(mpt::saturate_cast<FileReader::off_t>(0 - offset)) ? 0 : -1;
			} else
			{
				if(!Util::TypeCanHoldValue<FileReader::off_t>(offset))
				{
					return -1;
				}
				return file.Skip(mpt::saturate_cast<FileReader::off_t>(offset)) ? 0 : -1;
			}
		}
		break;
	case SEEK_END:
		{
			if(!Util::TypeCanHoldValue<FileReader::off_t>(offset))
			{
				return -1;
			}
			if(!Util::TypeCanHoldValue<FileReader::off_t>(file.GetLength() + offset))
			{
				return -1;
			}
			return file.Seek(mpt::saturate_cast<FileReader::off_t>(file.GetLength() + offset)) ? 0 : -1;
		}
		break;
	default:
		return -1;
	}
}

static long VorbisfileFilereaderTell(void *datasource)
{
	FileReader &file = *reinterpret_cast<FileReader*>(datasource);
	FileReader::off_t result = file.GetPosition();
	if(!Util::TypeCanHoldValue<long>(result))
	{
		return -1;
	}
	return static_cast<long>(result);
}

#endif // MPT_WITH_VORBIS && MPT_WITH_VORBISFILE



#endif // MPT_ENABLE_MO3_BUILTIN



bool CSoundFile::ReadMO3(FileReader &file, ModLoadingFlags loadFlags)
//-------------------------------------------------------------------
{
	file.Rewind();

	// No valid MO3 file (magic bytes: "MO3")
	if(!file.CanRead(12) || !file.ReadMagic("MO3"))
	{
		return false;
	}
	const uint8 version = file.ReadUint8();
	const uint32 musicSize = file.ReadUint32LE();
	if(musicSize <= 422 /*sizeof(MO3FileHeader)*/)
	{
		return false;
	} else if(loadFlags == onlyVerifyHeader)
	{
		return true;
	}

#if !(defined(MPT_WITH_UNMO3) || defined(MPT_ENABLE_UNMO3_DYNBIND)) && !defined(MPT_ENABLE_MO3_BUILTIN)
	// As of March 2016, the format revision is 5; Versions > 31 are unlikely to ever exist,
	// so we will just ignore those if there's no UNMO3 library to tell us if the file is valid or not
	// (avoid log entry with e.g. .MOD files that have a song name starting with "MO3").
	if(version > 31)
	{
		return false;
	}

	AddToLog(LogError, MPT_USTRING("The file appears to be a MO3 file, but this OpenMPT build does not support loading MO3 files."));
	return false;

#elif defined(MPT_WITH_UNMO3) || defined(MPT_ENABLE_UNMO3_DYNBIND)

	MPT_UNUSED_VARIABLE(version);

	bool shouldUseUnmo3 = true;
	#ifdef MPT_ENABLE_MO3_BUILTIN
		shouldUseUnmo3 = !CanReadMP3() || !CanReadVorbis(); 
	#endif // MPT_ENABLE_MO3_BUILTIN

	if(shouldUseUnmo3)
	{
	// Try to load unmo3 dynamically.
	ComponentHandle<ComponentUnMO3> unmo3;
	if(IsComponentAvailable(unmo3))
	{
		file.Rewind();
		FileReader::PinnedRawDataView fileView = file.GetPinnedRawDataView();
		const void *stream = mpt::byte_cast<const void*>(fileView.data());
		uint32 length = mpt::saturate_cast<uint32>(fileView.size());

		if(unmo3->UNMO3_Decode(&stream, &length, (loadFlags & loadSampleData) ? 0 : 1) != 0)
		{
			return false;
		}

		fileView.invalidate();

		// If decoding was successful, stream and length will keep the new pointers now.
		FileReader unpackedFile(mpt::as_span(mpt::byte_cast<const mpt::byte*>(stream), length));

		bool result = false;	// Result of trying to load the module, false == fail.

		result = ReadXM(unpackedFile, loadFlags)
			|| ReadIT(unpackedFile, loadFlags)
			|| ReadS3M(unpackedFile, loadFlags)
			|| ReadMTM(unpackedFile, loadFlags)
			|| ReadMod(unpackedFile, loadFlags)
			|| ReadM15(unpackedFile, loadFlags);
		if(result)
		{
			AddToLog(LogNotification, MPT_USTRING("Loading MO3 file used Un4seen unmo3."));
			m_ContainerType = MOD_CONTAINERTYPE_MO3;
		}

		unmo3->UNMO3_Free(stream);

		if(result)
		{
			return true;
		}
	} else
	{
		#ifndef MPT_ENABLE_MO3_BUILTIN
			AddToLog(LogError, MPT_USTRING("Loading MO3 file failed because the unmo3 library could not be loaded."));
			return false;
		#endif // MPT_ENABLE_MO3_BUILTIN
	}
	}

#endif

#ifdef MPT_ENABLE_MO3_BUILTIN

	if(version > 5)
	{
		return false;
	}
	uint32 compressedSize = uint32_max;
	if(version >= 5)
	{
		// Size of compressed music chunk
		compressedSize = file.ReadUint32LE();
#ifndef MPT_BUILD_FUZZER
		if(!file.CanRead(compressedSize))
		{
			return false;
		}
#endif // MPT_BUILD_FUZZER
	}

	uint8 *musicData = new (std::nothrow) uint8[musicSize];
	if(musicData == nullptr)
	{
		return false;
	}

	if(!UnpackMO3Data(file, musicData, musicSize))
	{
		delete[] musicData;
		return false;
	}
	if(version >= 5)
	{
		file.Seek(12 + compressedSize);
	}

	InitializeGlobals();
	InitializeChannels();

	FileReader musicChunk(mpt::as_span(musicData, musicSize));
	musicChunk.ReadNullString(m_songName);
	musicChunk.ReadNullString(m_songMessage);

	STATIC_ASSERT(MAX_BASECHANNELS >= 64);
	MO3FileHeader fileHeader;
	if(!musicChunk.ReadConvertEndianness(fileHeader)
		|| fileHeader.numChannels == 0 || fileHeader.numChannels > 64
		|| fileHeader.numInstruments >= MAX_INSTRUMENTS
		|| fileHeader.numSamples >= MAX_SAMPLES)
	{
		delete[] musicData;
		return false;
	}

	m_nChannels = fileHeader.numChannels;
	Order.SetRestartPos(fileHeader.restartPos);
	m_nInstruments = fileHeader.numInstruments;
	m_nSamples = fileHeader.numSamples;
	m_nDefaultSpeed = fileHeader.defaultSpeed ? fileHeader.defaultSpeed : 6;
	m_nDefaultTempo.Set(fileHeader.defaultTempo ? fileHeader.defaultTempo : 125, 0);

	m_ContainerType = MOD_CONTAINERTYPE_MO3;
	if(fileHeader.flags & MO3FileHeader::isIT)
		SetType(MOD_TYPE_IT);
	else if(fileHeader.flags & MO3FileHeader::isS3M)
		SetType(MOD_TYPE_S3M);
	else if(fileHeader.flags & MO3FileHeader::isMOD)
		SetType(MOD_TYPE_MOD);
	else if(fileHeader.flags & MO3FileHeader::isMTM)
		SetType(MOD_TYPE_MTM);
	else
		SetType(MOD_TYPE_XM);

	if(fileHeader.flags & MO3FileHeader::linearSlides)
		m_SongFlags.set(SONG_LINEARSLIDES);
	if((fileHeader.flags & MO3FileHeader::s3mAmigaLimits) && m_nType == MOD_TYPE_S3M)
		m_SongFlags.set(SONG_AMIGALIMITS);
	if((fileHeader.flags & MO3FileHeader::s3mFastSlides) && m_nType == MOD_TYPE_S3M)
		m_SongFlags.set(SONG_FASTVOLSLIDES);
	if(!(fileHeader.flags & MO3FileHeader::itOldFX) && m_nType == MOD_TYPE_IT)
		m_SongFlags.set(SONG_ITOLDEFFECTS);
	if(!(fileHeader.flags & MO3FileHeader::itCompatGxx) && m_nType == MOD_TYPE_IT)
		m_SongFlags.set(SONG_ITCOMPATGXX);
	if(fileHeader.flags & MO3FileHeader::extFilterRange)
		m_SongFlags.set(SONG_EXFILTERRANGE);
	if(fileHeader.flags & MO3FileHeader::modVBlank)
		m_playBehaviour.set(kMODVBlankTiming);

	if(m_nType == MOD_TYPE_IT)
		m_nDefaultGlobalVolume = std::min<uint16>(fileHeader.globalVol, 128) * 2;
	else if(m_nType == MOD_TYPE_S3M)
		m_nDefaultGlobalVolume = std::min<uint16>(fileHeader.globalVol, 64) * 4;

	if(fileHeader.sampleVolume < 0)
		m_nSamplePreAmp = fileHeader.sampleVolume + 52;
	else
		m_nSamplePreAmp = static_cast<uint32>(std::exp(fileHeader.sampleVolume * 3.1 / 20.0)) + 51;

	for(CHANNELINDEX i = 0; i < m_nChannels; i++)
	{
		if(m_nType == MOD_TYPE_IT)
			ChnSettings[i].nVolume = std::min(fileHeader.chnVolume[i], uint8(64));
		if(m_nType != MOD_TYPE_XM)
		{
			if(fileHeader.chnPan[i] == 127)
				ChnSettings[i].dwFlags = CHN_SURROUND;
			else if(fileHeader.chnPan[i] == 255)
				ChnSettings[i].nPan = 256;
			else
				ChnSettings[i].nPan = fileHeader.chnPan[i];
		}
	}

	bool anyMacros = false;
	for(uint32 i = 0; i < 16; i++)
	{
		if(fileHeader.sfxMacros[i])
			anyMacros = true;
	}
	for(uint32 i = 0; i < 128; i++)
	{
		if(fileHeader.fixedMacros[i][1])
			anyMacros = true;
	}

	if(anyMacros)
	{
		for(uint32 i = 0; i < 16; i++)
		{
			if(fileHeader.sfxMacros[i])
				sprintf(m_MidiCfg.szMidiSFXExt[i], "F0F0%02Xz", fileHeader.sfxMacros[i] - 1);
			else
				strcpy(m_MidiCfg.szMidiSFXExt[i], "");
		}
		for(uint32 i = 0; i < 128; i++)
		{
			if(fileHeader.fixedMacros[i][1])
				sprintf(m_MidiCfg.szMidiZXXExt[i], "F0F0%02X%02X", fileHeader.fixedMacros[i][1] - 1, fileHeader.fixedMacros[i][0]);
			else
				strcpy(m_MidiCfg.szMidiZXXExt[i], "");
		}
		m_SongFlags.set(SONG_EMBEDMIDICFG, !m_MidiCfg.IsMacroDefaultSetupUsed());
	}

	Order.ReadAsByte(musicChunk, fileHeader.numOrders, fileHeader.numOrders, 0xFF, 0xFE);

	// Track assignments for all patterns
	FileReader trackChunk = musicChunk.ReadChunk(fileHeader.numPatterns * fileHeader.numChannels * sizeof(uint16));
	FileReader patLengthChunk = musicChunk.ReadChunk(fileHeader.numPatterns * sizeof(uint16));
	std::vector<FileReader> tracks(fileHeader.numTracks);

	for(uint32 track = 0; track < fileHeader.numTracks; track++)
	{
		uint32 len = musicChunk.ReadUint32LE();
		tracks[track] = musicChunk.ReadChunk(len);
	}

	/*
	MO3 pattern commands:
	01 = Note
	02 = Instrument
	03 = CMD_ARPEGGIO (IT, XM, S3M, MOD, MTM)
	04 = CMD_PORTAMENTOUP (XM, MOD, MTM)   [for formats with separate fine slides]
	05 = CMD_PORTAMENTODOWN (XM, MOD, MTM) [for formats with separate fine slides]
	06 = CMD_TONEPORTAMENTO (IT, XM, S3M, MOD, MTM) / VOLCMD_TONEPORTA (IT, XM)
	07 = CMD_VIBRATO (IT, XM, S3M, MOD, MTM) / VOLCMD_VIBRATODEPTH (IT)
	08 = CMD_TONEPORTAVOL (XM, MOD, MTM)
	09 = CMD_VIBRATOVOL (XM, MOD, MTM)
	0A = CMD_TREMOLO (IT, XM, S3M, MOD, MTM)
	0B = CMD_PANNING8 (IT, XM, S3M, MOD, MTM) / VOLCMD_PANNING (IT, XM)
	0C = CMD_OFFSET (IT, XM, S3M, MOD, MTM)
	0D = CMD_VOLUMESLIDE (XM, MOD, MTM)
	0E = CMD_POSITIONJUMP (IT, XM, S3M, MOD, MTM)
	0F = CMD_VOLUME (XM, MOD, MTM) / VOLCMD_VOLUME (IT, XM, S3M)
	10 = CMD_PATTERNBREAK (IT, XM, MOD, MTM) - BCD-encoded in MOD/XM/S3M/MTM!
	11 = CMD_MODCMDEX (XM, MOD, MTM)
	12 = CMD_TEMPO (XM, MOD, MTM) / CMD_SPEED (XM, MOD, MTM)
	13 = CMD_TREMOR (XM)
	14 = VOLCMD_VOLSLIDEUP x=X0 (XM) / VOLCMD_VOLSLIDEDOWN x=0X (XM)
	15 = VOLCMD_FINEVOLUP x=X0 (XM) / VOLCMD_FINEVOLDOWN x=0X (XM)
	16 = CMD_GLOBALVOLUME (IT, XM, S3M)
	17 = CMD_GLOBALVOLSLIDE (XM)
	18 = CMD_KEYOFF (XM)
	19 = CMD_SETENVPOSITION (XM)
	1A = CMD_PANNINGSLIDE (XM)
	1B = VOLCMD_PANSLIDELEFT x=0X (XM) / VOLCMD_PANSLIDERIGHT x=X0 (XM)
	1C = CMD_RETRIG (XM)
	1D = CMD_XFINEPORTAUPDOWN X1x (XM)
	1E = CMD_XFINEPORTAUPDOWN X2x (XM)
	1F = VOLCMD_VIBRATOSPEED (XM)
	20 = VOLCMD_VIBRATODEPTH (XM)
	21 = CMD_SPEED (IT, S3M)
	22 = CMD_VOLUMESLIDE (IT, S3M)
	23 = CMD_PORTAMENTODOWN (IT, S3M) [for formats without separate fine slides]
	24 = CMD_PORTAMENTOUP (IT, S3M)   [for formats without separate fine slides]
	25 = CMD_TREMOR (IT, S3M)
	26 = CMD_RETRIG (IT, S3M)
	27 = CMD_FINEVIBRATO (IT, S3M)
	28 = CMD_CHANNELVOLUME (IT, S3M)
	29 = CMD_CHANNELVOLSLIDE (IT, S3M)
	2A = CMD_PANNINGSLIDE (IT, S3M)
	2B = CMD_S3MCMDEX (IT, S3M)
	2C = CMD_TEMPO (IT, S3M)
	2D = CMD_GLOBALVOLSLIDE (IT, S3M)
	2E = CMD_PANBRELLO (IT, XM, S3M)
	2F = CMD_MIDI (IT, XM, S3M)
	30 = VOLCMD_FINEVOLUP x=0...9 (IT) / VOLCMD_FINEVOLDOWN x=10...19 (IT) / VOLCMD_VOLSLIDEUP x=20...29 (IT) / VOLCMD_VOLSLIDEDOWN x=30...39 (IT)
	31 = VOLCMD_PORTADOWN (IT)
	32 = VOLCMD_PORTAUP (IT)
	33 = Unused XM command "W" (XM)
	34 = Any other IT volume column command to support OpenMPT extensions (IT)
	35 = CMD_XPARAM (IT)
	36 = CMD_SMOOTHMIDI (IT)
	37 = CMD_DELAYCUT (IT)

	Note: S3M/IT CMD_TONEPORTAVOL / CMD_VIBRATOVOL are encoded as two commands:
	K= 07 00 22 x
	L= 06 00 22 x
	*/

	static const ModCommand::COMMAND effTrans[] =
	{
		CMD_NONE,				CMD_NONE,				CMD_NONE,				CMD_ARPEGGIO,
		CMD_PORTAMENTOUP,		CMD_PORTAMENTODOWN,		CMD_TONEPORTAMENTO,		CMD_VIBRATO,
		CMD_TONEPORTAVOL,		CMD_VIBRATOVOL,			CMD_TREMOLO,			CMD_PANNING8,
		CMD_OFFSET,				CMD_VOLUMESLIDE,		CMD_POSITIONJUMP,		CMD_VOLUME,
		CMD_PATTERNBREAK,		CMD_MODCMDEX,			CMD_TEMPO,				CMD_TREMOR,
		VOLCMD_VOLSLIDEUP,		VOLCMD_FINEVOLUP,		CMD_GLOBALVOLUME,		CMD_GLOBALVOLSLIDE,
		CMD_KEYOFF,				CMD_SETENVPOSITION,		CMD_PANNINGSLIDE,		VOLCMD_PANSLIDELEFT,
		CMD_RETRIG,				CMD_XFINEPORTAUPDOWN,	CMD_XFINEPORTAUPDOWN,	VOLCMD_VIBRATOSPEED,
		VOLCMD_VIBRATODEPTH,	CMD_SPEED,				CMD_VOLUMESLIDE,		CMD_PORTAMENTODOWN,
		CMD_PORTAMENTOUP,		CMD_TREMOR,				CMD_RETRIG,				CMD_FINEVIBRATO,
		CMD_CHANNELVOLUME,		CMD_CHANNELVOLSLIDE,	CMD_PANNINGSLIDE,		CMD_S3MCMDEX,
		CMD_TEMPO,				CMD_GLOBALVOLSLIDE,		CMD_PANBRELLO,			CMD_MIDI,
		VOLCMD_FINEVOLUP,		VOLCMD_PORTADOWN,		VOLCMD_PORTAUP,			CMD_NONE,
		VOLCMD_OFFSET,			CMD_XPARAM,				CMD_SMOOTHMIDI,			CMD_DELAYCUT
	};

	uint8 noteOffset = NOTE_MIN;
	if(m_nType == MOD_TYPE_MTM)
		noteOffset = 13 + NOTE_MIN;
	else if(m_nType != MOD_TYPE_IT)
		noteOffset = 12 + NOTE_MIN;
	for(PATTERNINDEX pat = 0; pat < fileHeader.numPatterns; pat++)
	{
		const ROWINDEX numRows = patLengthChunk.ReadUint16LE();
		if(!(loadFlags & loadPatternData) || !Patterns.Insert(pat, numRows))
			continue;

		for(CHANNELINDEX chn = 0; chn < fileHeader.numChannels; chn++)
		{
			uint16 trackIndex = trackChunk.ReadUint16LE();
			if(trackIndex >= tracks.size())
				continue;
			FileReader &track = tracks[trackIndex];
			track.Rewind();
			ROWINDEX row = 0;
			ModCommand *patData = Patterns[pat].GetpModCommand(0, chn);
			while(row < numRows)
			{
				const uint8 b = track.ReadUint8();
				if(!b)
					break;

				const uint8 numCommands = (b & 0x0F), rep = (b >> 4);
				ModCommand m = ModCommand::Empty();
				for(uint8 i = 0; i < numCommands; i++)
				{
					uint8 cmd[2];
					track.ReadArray(cmd);

					// Import pattern commands
					switch(cmd[0])
					{
					case 0x01:
						// Note
						m.note = cmd[1];
						if(m.note < 120) m.note += noteOffset;
						else if(m.note == 0xFF) m.note = NOTE_KEYOFF;
						else if(m.note == 0xFE) m.note = NOTE_NOTECUT;
						else m.note = NOTE_FADE;
						break;
					case 0x02:
						// Instrument
						m.instr = cmd[1] + 1;
						break;
					case 0x06:
						// Tone portamento
						if(m.volcmd == VOLCMD_NONE && m_nType == MOD_TYPE_XM && !(cmd[1] & 0x0F))
						{
							m.volcmd = VOLCMD_TONEPORTAMENTO;
							m.vol = cmd[1] >> 4;
							break;
						} else if(m.volcmd == VOLCMD_NONE && m_nType == MOD_TYPE_IT)
						{
							for(uint8 i = 0; i < 10; i++)
							{
								if(ImpulseTrackerPortaVolCmd[i] == cmd[1])
								{
									m.volcmd = VOLCMD_TONEPORTAMENTO;
									m.vol = i;
									break;
								}
							}
							if(m.volcmd != VOLCMD_NONE)
								break;
						}
						m.command = CMD_TONEPORTAMENTO;
						m.param = cmd[1];
						break;
					case 0x07:
						// Vibrato
						if(m.volcmd == VOLCMD_NONE && cmd[1] < 10 && m_nType == MOD_TYPE_IT)
						{
							m.volcmd = VOLCMD_VIBRATODEPTH;
							m.vol = cmd[1];
						} else
						{
							m.command = CMD_VIBRATO;
							m.param = cmd[1];
						}
						break;
					case 0x0B:
						// Panning
						if(m.volcmd == VOLCMD_NONE)
						{
							if(m_nType == MOD_TYPE_IT && cmd[1] == 0xFF)
							{
								m.volcmd = VOLCMD_PANNING;
								m.vol = 64;
								break;
							}
							if((m_nType == MOD_TYPE_IT && !(cmd[1] & 0x03))
								|| (m_nType == MOD_TYPE_XM && !(cmd[1] & 0x0F)))
							{
								m.volcmd = VOLCMD_PANNING;
								m.vol = cmd[1] / 4;
								break;
							}
						}
						m.command = CMD_PANNING8;
						m.param = cmd[1];
						break;
					case 0x0F:
						// Volume
						if(m_nType != MOD_TYPE_MOD && m.volcmd == VOLCMD_NONE && cmd[1] <= 64)
						{
							m.volcmd = VOLCMD_VOLUME;
							m.vol = cmd[1];
						} else
						{
							m.command = CMD_VOLUME;
							m.param = cmd[1];
						}
						break;
					case 0x10:
						// Pattern break
						m.command = CMD_PATTERNBREAK;
						m.param = cmd[1];
						if(m_nType != MOD_TYPE_IT)
							m.param = ((m.param >> 4) * 10) + (m.param & 0x0F);
						break;
					case 0x12:
						// Combined Tempo / Speed command
						m.param = cmd[1];
						if(m.param < 0x20)
							m.command = CMD_SPEED;
						else
							m.command = CMD_TEMPO;
						break;
					case 0x14:
					case 0x15:
						// XM volume column volume slides
						if(cmd[1] & 0xF0)
						{
							m.volcmd = static_cast<ModCommand::VOLCMD>((cmd[0] == 0x14) ? VOLCMD_VOLSLIDEUP : VOLCMD_FINEVOLUP);
							m.vol = cmd[1] >> 4;
						} else
						{
							m.volcmd = static_cast<ModCommand::VOLCMD>((cmd[0] == 0x14) ? VOLCMD_VOLSLIDEDOWN : VOLCMD_FINEVOLDOWN);
							m.vol = cmd[1] & 0x0F;
						}
						break;
					case 0x1B:
						// XM volume column panning slides
						if(cmd[1] & 0xF0)
						{
							m.volcmd = VOLCMD_PANSLIDERIGHT;
							m.vol = cmd[1] >> 4;
						} else
						{
							m.volcmd = VOLCMD_PANSLIDELEFT;
							m.vol = cmd[1] & 0x0F;
						}
						break;
					case 0x1D:
						// XM extra fine porta up
						m.command = CMD_XFINEPORTAUPDOWN;
						m.param = 0x10 | cmd[1];
						break;
					case 0x1E:
						// XM extra fine porta down
						m.command = CMD_XFINEPORTAUPDOWN;
						m.param = 0x20 | cmd[1];
						break;
					case 0x1F:
					case 0x20:
						// XM volume column vibrato
						m.volcmd = effTrans[cmd[0]];
						m.vol = cmd[1];
						break;
					case 0x22:
						// IT / S3M volume slide
						if(m.command == CMD_TONEPORTAMENTO)
							m.command = CMD_TONEPORTAVOL;
						else if(m.command == CMD_VIBRATO)
							m.command = CMD_VIBRATOVOL;
						else
							m.command = CMD_VOLUMESLIDE;
						m.param = cmd[1];
						break;
					case 0x30:
						// IT volume column volume slides
						m.vol = cmd[1] % 10;
						if(cmd[1] < 10)
							m.volcmd = VOLCMD_FINEVOLUP;
						else if(cmd[1] < 20)
							m.volcmd = VOLCMD_FINEVOLDOWN;
						else if(cmd[1] < 30)
							m.volcmd = VOLCMD_VOLSLIDEUP;
						else if(cmd[1] < 40)
							m.volcmd = VOLCMD_VOLSLIDEDOWN;
						break;
					case 0x31:
					case 0x32:
						// IT volume column portamento
						m.volcmd = effTrans[cmd[0]];
						m.vol = cmd[1];
						break;
					case 0x34:
						// Any unrecognized IT volume command
						if(cmd[1] >= 223 && cmd[1] <= 232)
						{
							m.volcmd = VOLCMD_OFFSET;
							m.vol = cmd[1] - 223;
						}
						break;
					default:
						if(cmd[0] < CountOf(effTrans))
						{
							m.command = effTrans[cmd[0]];
							m.param = cmd[1];
						}
						break;
					}
				}
#ifdef MODPLUG_TRACKER
				if(m_nType == MOD_TYPE_MTM)
					m.Convert(MOD_TYPE_MOD, MOD_TYPE_S3M, *this);
#endif
				ROWINDEX targetRow = std::min(row + rep, numRows);
				while(row < targetRow)
				{
					*patData = m;
					patData += fileHeader.numChannels;
					row++;
				}
			}
		}
	}

	const bool isSampleMode = (m_nType != MOD_TYPE_XM && !(fileHeader.flags & MO3FileHeader::instrumentMode));
	std::vector<MO3Instrument::XMVibratoSettings> instrVibrato(m_nType == MOD_TYPE_XM ? m_nInstruments : 0);
	for(INSTRUMENTINDEX ins = 1; ins <= m_nInstruments; ins++)
	{
		ModInstrument *pIns = nullptr;
		if(isSampleMode || (pIns = AllocateInstrument(ins)) == nullptr)
		{
			// Even in IT sample mode, instrument headers are still stored....
			while(musicChunk.ReadUint8() != 0);
			if(version >= 5)
			{
				while(musicChunk.ReadUint8() != 0);
			}
			musicChunk.Skip(sizeof(MO3Instrument));
			continue;
		}

		std::string name;
		musicChunk.ReadNullString(name);
		mpt::String::Copy(pIns->name, name);
		if(version >= 5)
		{
			musicChunk.ReadNullString(name);
			mpt::String::Copy(pIns->filename, name);
		}

		MO3Instrument insHeader;
		if(!musicChunk.ReadConvertEndianness(insHeader))
			break;
		insHeader.ConvertToMPT(*pIns, m_nType);

		if(m_nType == MOD_TYPE_XM)
			instrVibrato[ins - 1] = insHeader.vibrato;
	}
	if(isSampleMode)
		m_nInstruments = 0;

	std::vector<MO3SampleChunk> sampleChunks(m_nSamples);

	const bool frequencyIsHertz = (version >= 5 || !(fileHeader.flags & MO3FileHeader::linearSlides));
	bool unsupportedSamples = false;
	for(SAMPLEINDEX smp = 1; smp <= m_nSamples; smp++)
	{
		ModSample &sample = Samples[smp];
		std::string name;
		musicChunk.ReadNullString(name);
		mpt::String::Copy(m_szNames[smp], name);
		if(version >= 5)
		{
			musicChunk.ReadNullString(name);
			mpt::String::Copy(sample.filename, name);
		}

		MO3Sample smpHeader;
		if(!musicChunk.ReadConvertEndianness(smpHeader))
			break;
		smpHeader.ConvertToMPT(sample, m_nType, frequencyIsHertz);

		int16 sharedOggHeader = 0;
		if(version >= 5 && (smpHeader.flags & MO3Sample::smpCompressionMask) == MO3Sample::smpSharedOgg)
		{
			sharedOggHeader = musicChunk.ReadInt16LE();
		}

		if(!(loadFlags & loadSampleData))
			continue;

		const uint32 compression = (smpHeader.flags & MO3Sample::smpCompressionMask);
		if(!compression && smpHeader.compressedSize == 0)
		{
			// Uncompressed sample
			SampleIO(
				(smpHeader.flags & MO3Sample::smp16Bit) ? SampleIO::_16bit : SampleIO::_8bit,
				(smpHeader.flags & MO3Sample::smpStereo) ? SampleIO::stereoSplit : SampleIO::mono,
				SampleIO::littleEndian,
				SampleIO::signedPCM)
				.ReadSample(Samples[smp], file);
		} else if(smpHeader.compressedSize < 0 && (smp + smpHeader.compressedSize) > 0)
		{
			// Duplicate sample
			const ModSample &smpFrom = Samples[smp + smpHeader.compressedSize];
			LimitMax(sample.nLength, smpFrom.nLength);
			sample.uFlags.set(CHN_16BIT, smpFrom.uFlags[CHN_16BIT]);
			sample.uFlags.set(CHN_STEREO, smpFrom.uFlags[CHN_STEREO]);
			if(smpFrom.pSample != nullptr && sample.AllocateSample())
			{
				memcpy(sample.pSample, smpFrom.pSample, sample.GetSampleSizeInBytes());
			}
		} else if(smpHeader.compressedSize > 0)
		{
			if(smpHeader.flags & MO3Sample::smp16Bit) sample.uFlags.set(CHN_16BIT);
			if(smpHeader.flags & MO3Sample::smpStereo) sample.uFlags.set(CHN_STEREO);

			FileReader sampleData = file.ReadChunk(smpHeader.compressedSize);
			const uint8 numChannels = sample.GetNumChannels();

			if(compression == MO3Sample::smpDeltaCompression)
			{
				if(sample.AllocateSample())
				{
					if(smpHeader.flags & MO3Sample::smp16Bit)
						UnpackMO3DeltaSample<MO3Delta16BitParams>(sampleData, sample.pSample16, sample.nLength, numChannels);
					else
						UnpackMO3DeltaSample<MO3Delta8BitParams>(sampleData, sample.pSample8, sample.nLength, numChannels);
				}
			} else if(compression == MO3Sample::smpDeltaPrediction)
			{
				if(sample.AllocateSample())
				{
					if(smpHeader.flags & MO3Sample::smp16Bit)
						UnpackMO3DeltaPredictionSample<MO3Delta16BitParams>(sampleData, sample.pSample16, sample.nLength, numChannels);
					else
						UnpackMO3DeltaPredictionSample<MO3Delta8BitParams>(sampleData, sample.pSample8, sample.nLength, numChannels);
				}
			} else if(compression == MO3Sample::smpCompressionOgg || compression == MO3Sample::smpSharedOgg)
			{
				// Since shared Ogg headers can stem from a sample that has not been read yet, postpone Ogg import.
				sampleChunks[smp - 1] = MO3SampleChunk(sampleData, smpHeader.encoderDelay, sharedOggHeader);
			} else if(compression == MO3Sample::smpCompressionMPEG)
			{
				// Old MO3 encoders didn't remove LAME info frames. This is unfortunate since the encoder delay
				// specified in the sample header does not take the gapless information from the LAME info frame
				// into account. We should not depend on the MP3 decoder's capabilities to read or ignore such frames:
				// - libmpg123 has MPG123_IGNORE_INFOFRAME but that requires API version 31 (mpg123 v1.14) or higher
				// - Media Foundation does (currently) not read LAME gapless information at all
				// So we just play safe and remove such frames.
				FileReader mpegData(sampleData);
				MPEGFrame frame(sampleData);
				uint16 frameDelay = frame.numSamples * 2;
				if(frame.isLAME && smpHeader.encoderDelay >= frameDelay)
				{
					// The info frame does not produce any output, but still counts towards the encoder delay.
					smpHeader.encoderDelay -= frameDelay;
					sampleData.Seek(frame.frameSize);
					mpegData = sampleData.ReadChunk(sampleData.BytesLeft());
				}
				
				if(ReadMP3Sample(smp, mpegData, true) || ReadMediaFoundationSample(smp, mpegData, true))
				{
					if(smpHeader.encoderDelay > 0 && smpHeader.encoderDelay < sample.GetSampleSizeInBytes())
					{
						SmpLength delay = smpHeader.encoderDelay / sample.GetBytesPerSample();
						memmove(sample.pSample8, sample.pSample8 + smpHeader.encoderDelay, sample.GetSampleSizeInBytes() - smpHeader.encoderDelay);
						sample.nLength -= delay;
					}
					LimitMax(sample.nLength, smpHeader.length);
				} else
				{
					unsupportedSamples = true;
				}
			} else
			{
				unsupportedSamples = true;
			}
		}
	}

	// Now we can load Ogg samples with shared headers.
	if(loadFlags & loadSampleData)
	{
		for(SAMPLEINDEX smp = 1; smp <= m_nSamples; smp++)
		{
			MO3SampleChunk &sampleChunk = sampleChunks[smp - 1];
			// Is this an Ogg sample?
			if(!sampleChunk.chunk.IsValid())
				continue;

			SAMPLEINDEX sharedOggHeader = smp + sampleChunk.sharedHeader;
			// Which chunk are we going to read the header from?
			// Note: Every Ogg stream has a unique serial number.
			// stb_vorbis (currently) ignores this serial number so we can just stitch
			// together our sample without adjusting the shared header's serial number.
			const bool sharedHeader = sharedOggHeader != smp && sharedOggHeader > 0 && sharedOggHeader <= m_nSamples;

#if defined(MPT_WITH_VORBIS) && defined(MPT_WITH_VORBISFILE)

			std::vector<char> mergedData;
			if(sharedHeader)
			{
				// Prepend the shared header to the actual sample data and adjust bitstream serial numbers.
				// We do not handle multiple muxed logical streams as they do not exist in practice in mo3.
				// We assume sequence numbers are consecutive at the end of the headers.
				// Corrupted pages get dropped as required by Ogg spec. We cannot do any further sane parsing on them anyway.
				// We do not match up multiple muxed stream properly as this wold need parsing of actual packet data to determine or guess the codec.
				// Ogg Vorbis files may contain at least an additional Ogg Skeleton stream. It is not clear whether these actually exist in MO3.
				// We do not validate packet structure or logical bitstream structure (i.e. sequence numbers and granule positions).

				// TODO: At least handle Skeleton streams here, as they violate our stream ordering assumptions here.

#if 0
				// This block may still turn out to be useful as it does a more thourough validation of the stream than the optimized version below.

				// We copy the whole data into a single consecutive buffer in order to keep things simple when interfacing libvorbisfile.
				// We could in theory only adjust the header and pass 2 chunks to libvorbisfile.
				// Another option would be to demux both chunks on our own (or using libogg) and pass the raw packet data to libvorbis directly.

				mpt::ostringstream mergedStream(std::ios::binary);
				mergedStream.imbue(std::locale::classic());

				sampleChunks[sharedOggHeader - 1].chunk.Rewind();
				FileReader sharedChunk = sampleChunks[sharedOggHeader - 1].chunk.ReadChunk(sampleChunk.headerSize);
				sharedChunk.Rewind();

				std::vector<uint32> streamSerials;
				Ogg::PageInfo oggPageInfo;
				std::vector<uint8> oggPageData;

				streamSerials.clear();
				while(Ogg::ReadPageAndSkipJunk(sharedChunk, oggPageInfo, oggPageData))
				{
					std::vector<uint32>::iterator it = std::find(streamSerials.begin(), streamSerials.end(), oggPageInfo.header.bitstream_serial_number);
					if(it == streamSerials.end())
					{
						streamSerials.push_back(oggPageInfo.header.bitstream_serial_number);
						it = streamSerials.begin() + (streamSerials.size() - 1);
					}
					uint32 newSerial = it - streamSerials.begin() + 1;
					oggPageInfo.header.bitstream_serial_number = newSerial;
					Ogg::UpdatePageCRC(oggPageInfo, oggPageData);
					Ogg::WritePage(mergedStream, oggPageInfo, oggPageData);
				}

				streamSerials.clear();
				while(Ogg::ReadPageAndSkipJunk(sampleChunk.chunk, oggPageInfo, oggPageData))
				{
					std::vector<uint32>::iterator it = std::find(streamSerials.begin(), streamSerials.end(), oggPageInfo.header.bitstream_serial_number);
					if(it == streamSerials.end())
					{
						streamSerials.push_back(oggPageInfo.header.bitstream_serial_number);
						it = streamSerials.begin() + (streamSerials.size() - 1);
					}
					uint32 newSerial = it - streamSerials.begin() + 1;
					oggPageInfo.header.bitstream_serial_number = newSerial;
					Ogg::UpdatePageCRC(oggPageInfo, oggPageData);
					Ogg::WritePage(mergedStream, oggPageInfo, oggPageData);
				}

				std::string mergedStreamData = mergedStream.str();
				mergedData.insert(mergedData.end(), mergedStreamData.begin(), mergedStreamData.end());

#else

				// We assume same ordering of streams in both header and data if
				// multiple streams are present.

				mpt::ostringstream mergedStream(std::ios::binary);
				mergedStream.imbue(std::locale::classic());

				sampleChunks[sharedOggHeader - 1].chunk.Rewind();
				FileReader sharedChunk = sampleChunks[sharedOggHeader - 1].chunk.ReadChunk(sampleChunk.headerSize);
				sharedChunk.Rewind();

				std::vector<uint32> dataStreamSerials;
				std::vector<uint32> headStreamSerials;
				Ogg::PageInfo oggPageInfo;
				std::vector<uint8> oggPageData;

				// Gather bitstream serial numbers form sample data chunk
				dataStreamSerials.clear();
				while(Ogg::ReadPageAndSkipJunk(sampleChunk.chunk, oggPageInfo, oggPageData))
				{
					std::vector<uint32>::iterator it = std::find(dataStreamSerials.begin(), dataStreamSerials.end(), oggPageInfo.header.bitstream_serial_number);
					if(it == dataStreamSerials.end())
					{
						dataStreamSerials.push_back(oggPageInfo.header.bitstream_serial_number);
					}
				}

				// Apply the data bitstream serial numbers to the header
				headStreamSerials.clear();
				while(Ogg::ReadPageAndSkipJunk(sharedChunk, oggPageInfo, oggPageData))
				{
					std::vector<uint32>::iterator it = std::find(headStreamSerials.begin(), headStreamSerials.end(), oggPageInfo.header.bitstream_serial_number);
					if(it == headStreamSerials.end())
					{
						headStreamSerials.push_back(oggPageInfo.header.bitstream_serial_number);
						it = headStreamSerials.begin() + (headStreamSerials.size() - 1);
					}
					uint32 newSerial = 0;
					if(dataStreamSerials.size() >= static_cast<std::size_t>(it - headStreamSerials.begin()))
					{
						// Found corresponding stream in data chunk.
						newSerial = dataStreamSerials[it - headStreamSerials.begin()];
					} else
					{
						// No corresponding stream in data chunk. Find a free serialno.
						std::size_t extraIndex = (it - headStreamSerials.begin()) - dataStreamSerials.size();
						for(newSerial = 1; newSerial < 0xffffffffu; ++newSerial)
						{
							std::vector<uint32>::iterator it = std::find(dataStreamSerials.begin(), dataStreamSerials.end(), newSerial);
							if(it == dataStreamSerials.end())
							{
								extraIndex -= 1;
							}
							if(extraIndex == 0)
							{
								break;
							}
						}
					}
					oggPageInfo.header.bitstream_serial_number = newSerial;
					Ogg::UpdatePageCRC(oggPageInfo, oggPageData);
					Ogg::WritePage(mergedStream, oggPageInfo, oggPageData);
				}

				if(headStreamSerials.size() > 1)
				{
					AddToLog(LogWarning, mpt::format(MPT_USTRING("Sample %1: Ogg Vorbis data with shared header and multiple logical bitstreams in header chunk found. This may be handled incorrectly."))(smp));
				} else if(dataStreamSerials.size() > 1)
				{
					AddToLog(LogWarning, mpt::format(MPT_USTRING("Sample %1: Ogg Vorbis sample with shared header and multiple logical bitstreams found. This may be handled incorrectly."))(smp));
				} else if((dataStreamSerials.size() == 1) && (headStreamSerials.size() == 1) && (dataStreamSerials[0] != headStreamSerials[0]))
				{
					AddToLog(LogInformation, mpt::format(MPT_USTRING("Sample %1: Ogg Vorbis data with shared header and different logical bitstream serials found."))(smp));
				}

				std::string mergedStreamData = mergedStream.str();
				mergedData.insert(mergedData.end(), mergedStreamData.begin(), mergedStreamData.end());

				sampleChunk.chunk.Rewind();
				FileReader::PinnedRawDataView sampleChunkView = sampleChunk.chunk.GetPinnedRawDataView();
				mergedData.insert(mergedData.end(), sampleChunkView.begin(), sampleChunkView.end());

#endif

			}
			FileReader mergedDataChunk(mpt::byte_cast<mpt::const_byte_span>(mpt::as_span(mergedData)));

			FileReader &sampleData = sharedHeader ? mergedDataChunk : sampleChunk.chunk;
			FileReader &headerChunk = sampleData;

#else // !(MPT_WITH_VORBIS && MPT_WITH_VORBISFILE)

			FileReader &sampleData = sampleChunk.chunk;
			FileReader &headerChunk = sharedHeader ? sampleChunks[sharedOggHeader - 1].chunk : sampleData;
			int initialRead = sharedHeader ? sampleChunk.headerSize : headerChunk.GetLength();

#endif // MPT_WITH_VORBIS && MPT_WITH_VORBISFILE

			headerChunk.Rewind();
			if(sharedHeader && !headerChunk.CanRead(sampleChunk.headerSize))
				continue;

#if defined(MPT_WITH_VORBIS) && defined(MPT_WITH_VORBISFILE)

			ov_callbacks callbacks = {
				&VorbisfileFilereaderRead,
				&VorbisfileFilereaderSeek,
				NULL,
				&VorbisfileFilereaderTell
			};
			OggVorbis_File vf;
			MemsetZero(vf);
			if(ov_open_callbacks(&sampleData, &vf, NULL, 0, callbacks) == 0)
			{
				if(ov_streams(&vf) == 1)
				{ // we do not support chained vorbis samples
					vorbis_info *vi = ov_info(&vf, -1);
					if(vi && vi->rate > 0 && vi->channels > 0)
					{
						ModSample &sample = Samples[smp];
						sample.AllocateSample();
						SmpLength offset = 0;
						int channels = vi->channels;
						int current_section = 0;
						long decodedSamples = 0;
						bool eof = false;
						while(!eof && offset < sample.nLength && sample.pSample != nullptr)
						{
							float **output = nullptr;
							long ret = ov_read_float(&vf, &output, 1024, &current_section);
							if(ret == 0)
							{
								eof = true;
							} else if(ret < 0)
							{
								// stream error, just try to continue
							} else
							{
								decodedSamples = ret;
								LimitMax(decodedSamples, mpt::saturate_cast<long>(sample.nLength - offset));
								if(decodedSamples > 0 && channels == sample.GetNumChannels())
								{
									for(int chn = 0; chn < channels; chn++)
									{
										if(sample.uFlags[CHN_16BIT])
										{
											CopyChannelToInterleaved<SC::Convert<int16, float> >(sample.pSample16 + offset * sample.GetNumChannels(), output[chn], channels, decodedSamples, chn);
										} else
										{
											CopyChannelToInterleaved<SC::Convert<int8, float> >(sample.pSample8 + offset * sample.GetNumChannels(), output[chn], channels, decodedSamples, chn);
										}
									}
								}
								offset += decodedSamples;
							}
						}
					} else
					{
						unsupportedSamples = true;
					}
				} else
				{
					AddToLog(LogWarning, mpt::format(MPT_USTRING("Sample %1: Unsupported Ogg Vorbis chained stream found."))(smp));
					unsupportedSamples = true;
				}
				ov_clear(&vf);
			} else
			{
				unsupportedSamples = true;
			}

#elif defined(MPT_WITH_STBVORBIS)

			// NOTE/TODO: stb_vorbis does not handle inferred negative PCM sample
			// position at stream start. (See
			// <https://www.xiph.org/vorbis/doc/Vorbis_I_spec.html#x1-132000A.2>).
			// This means that, for remuxed and re-aligned/cutted (at stream start)
			// Vorbis files, stb_vorbis will include superfluous samples at the
			// beginning. MO3 files with this property are yet to be spotted in the
			// wild, thus, this behaviour is currently not problematic.

			int consumed = 0, error = 0;
			stb_vorbis *vorb = stb_vorbis_open_pushdata(headerChunk.GetRawData<uint8>(), initialRead, &consumed, &error, nullptr);
			if(vorb)
			{
				// Header has been read, proceed to reading the sample data
				headerChunk.Skip(consumed);
				ModSample &sample = Samples[smp];
				sample.AllocateSample();
				SmpLength offset = 0;
				while((error == VORBIS__no_error || (error == VORBIS_need_more_data && sampleData.CanRead(1)))
					&& offset < sample.nLength && sample.pSample != nullptr)
				{
					int channels = 0, decodedSamples = 0;
					float **output;
					consumed = stb_vorbis_decode_frame_pushdata(vorb, sampleData.GetRawData<uint8>(), mpt::saturate_cast<int>(sampleData.BytesLeft()), &channels, &output, &decodedSamples);
					sampleData.Skip(consumed);
					LimitMax(decodedSamples, mpt::saturate_cast<int>(sample.nLength - offset));
					if(decodedSamples > 0 && channels == sample.GetNumChannels())
					{
						for(int chn = 0; chn < channels; chn++)
						{
							if(sample.uFlags[CHN_16BIT])
								CopyChannelToInterleaved<SC::Convert<int16, float> >(sample.pSample16 + offset * sample.GetNumChannels(), output[chn], channels, decodedSamples, chn);
							else
								CopyChannelToInterleaved<SC::Convert<int8, float> >(sample.pSample8 + offset * sample.GetNumChannels(), output[chn], channels, decodedSamples, chn);
						}
					}
					offset += decodedSamples;
					error = stb_vorbis_get_error(vorb);
				}
				stb_vorbis_close(vorb);
			} else
			{
				unsupportedSamples = true;
			}

#else // !VORBIS

			unsupportedSamples = true;

#endif // VORBIS
		}
	}

	if(m_nType == MOD_TYPE_XM)
	{
		// Transfer XM instrument vibrato to samples
		for(INSTRUMENTINDEX ins = 0; ins < m_nInstruments; ins++)
		{
			PropagateXMAutoVibrato(ins + 1, instrVibrato[ins].type, instrVibrato[ins].sweep, instrVibrato[ins].depth, instrVibrato[ins].rate);
		}
	}

	if((fileHeader.flags & MO3FileHeader::hasPlugins) && musicChunk.CanRead(1))
	{
		// Plugin data
		uint8 pluginFlags = musicChunk.ReadUint8();
		if(pluginFlags & 1)
		{
			// Channel plugins
			for(CHANNELINDEX chn = 0; chn < m_nChannels; chn++)
			{
				ChnSettings[chn].nMixPlugin = static_cast<PLUGINDEX>(musicChunk.ReadUint32LE());
			}
		}
		while(musicChunk.CanRead(1))
		{
			PLUGINDEX plug = musicChunk.ReadUint8();
			if(!plug)
				break;
			FileReader pluginChunk = musicChunk.ReadChunk(musicChunk.ReadUint32LE());
#ifndef NO_PLUGINS
			if(plug <= MAX_MIXPLUGINS)
			{
				ReadMixPluginChunk(pluginChunk, m_MixPlugins[plug - 1]);
			}
#endif // NO_PLUGINS
		}
	}

	uint16 cwtv = 0;
	uint16 cmwt = 0;
	MPT_UNUSED_VARIABLE(cmwt);
	while(musicChunk.CanRead(8))
	{
		uint32 id = musicChunk.ReadUint32LE();
		uint32 len = musicChunk.ReadUint32LE();
		FileReader chunk = musicChunk.ReadChunk(len);
		switch(id)
		{
		case MAGIC4LE('V','E','R','S'):
			// Tracker magic bytes (depending on format)
			switch(m_nType)
			{
			case MOD_TYPE_IT:
				cwtv = chunk.ReadUint16LE();
				cmwt = chunk.ReadUint16LE();
				/*switch(cwtv >> 12)
				{
					
				}*/
				break;
			case MOD_TYPE_S3M:
				cwtv = chunk.ReadUint16LE();
				break;
			case MOD_TYPE_XM:
				chunk.ReadString<mpt::String::spacePadded>(m_madeWithTracker, std::min(FileReader::off_t(32), chunk.GetLength()));
				break;
			case MOD_TYPE_MTM:
				{
					uint8 version = chunk.ReadUint8();
					m_madeWithTracker = mpt::String::Print("MultiTracker %1.%2", version >> 4, version & 0x0F);
				}
				break;
			default:
				break;
			}
			break;
		case MAGIC4LE('M', 'I', 'D', 'I'):
			// Full MIDI config
			chunk.ReadStruct<MIDIMacroConfigData>(m_MidiCfg);
			m_MidiCfg.Sanitize();
			break;
		case MAGIC4LE('O', 'M', 'P', 'T'):
			// Read pattern names: "PNAM"
			if(chunk.ReadMagic("PNAM"))
			{
				FileReader patterns = chunk.ReadChunk(chunk.ReadUint32LE());
				const PATTERNINDEX namedPats = std::min(static_cast<PATTERNINDEX>(patterns.GetLength() / MAX_PATTERNNAME), Patterns.Size());

				for(PATTERNINDEX pat = 0; pat < namedPats; pat++)
				{
					char patName[MAX_PATTERNNAME];
					patterns.ReadString<mpt::String::maybeNullTerminated>(patName, MAX_PATTERNNAME);
					Patterns[pat].SetName(patName);
				}
			}

			// Read channel names: "CNAM"
			if(chunk.ReadMagic("CNAM"))
			{
				FileReader channels = chunk.ReadChunk(chunk.ReadUint32LE());
				const CHANNELINDEX namedChans = std::min(static_cast<CHANNELINDEX>(channels.GetLength() / MAX_CHANNELNAME), GetNumChannels());
				for(CHANNELINDEX chn = 0; chn < namedChans; chn++)
				{
					channels.ReadString<mpt::String::maybeNullTerminated>(ChnSettings[chn].szName, MAX_CHANNELNAME);
				}
			}

			LoadExtendedInstrumentProperties(chunk);
			LoadExtendedSongProperties(chunk);
			if(cwtv > 0x0889 && cwtv <= 0x8FF)
			{
				m_nType = MOD_TYPE_MPT;
				LoadMPTMProperties(chunk, cwtv);
			}

			if(m_dwLastSavedWithVersion)
			{
				m_madeWithTracker = "OpenMPT " + MptVersion::ToStr(m_dwLastSavedWithVersion);
			}
			break;
		}
	}

	if(m_madeWithTracker.empty())
		m_madeWithTracker = mpt::String::Print("MO3 v%1", version);
	else
		m_madeWithTracker = mpt::String::Print("MO3 v%1 (%2)", version, m_madeWithTracker);

	delete[] musicData;

	if(unsupportedSamples)
	{
		AddToLog(LogWarning, MPT_USTRING("Some compressed samples could not be loaded because they use an unsupported codec."));
	}

	return true;
#else
	return false;
#endif // MPT_ENABLE_MO3_BUILTIN
}


OPENMPT_NAMESPACE_END
