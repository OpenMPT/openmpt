/*
 * InstrumentExtensions.cpp
 * ------------------------
 * Purpose: Instrument properties I/O
 * Notes  : Welcome to the absolutely horrible abominations that are the "extended instrument properties"
 *          which are some of the earliest additions OpenMPT did to the IT / XM format. They are ugly,
 *          and the way they work even differs between IT/XM and ITI/XI/ITP.
 *          Yes, the world would be a better place without this stuff.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"

#ifndef MODPLUG_NO_FILESAVE
#include "mpt/io/base.hpp"
#include "mpt/io/io.hpp"
#include "mpt/io/io_stdstream.hpp"
#endif

OPENMPT_NAMESPACE_BEGIN

/*---------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------
MODULAR (in/out) ModInstrument :
-----------------------------------------------------------------------------------------------

* to update:
------------

- both following functions need to be updated when adding a new member in ModInstrument :

void WriteInstrumentHeaderStructOrField(ModInstrument * input, std::ostream &file, uint32 onlyThisCode, int16 fixedsize);
void ReadInstrumentHeaderField(ModInstrument &ins, uint32 fcode, FileReader &file);

- see below for body declaration.


* members:
----------

- 32bit identification CODE tag (must be unique)
- 16bit content SIZE in byte(s)
- member field


* CODE tag naming convention:
-----------------------------

- have a look below in current tag dictionnary
- take the initial ones of the field name
- 4 caracters code (not more, not less)
- must be filled with '.' caracters if code has less than 4 caracters
- for arrays, must include a '[' caracter following significant caracters ('.' not significant!!!)
- use only caracters used in full member name, ordered as they appear in it
- match caracter attribute (small,capital)

Example with "PanEnv.nLoopEnd" , "PitchEnv.nLoopEnd" & "VolEnv.Values[MAX_ENVPOINTS]" members :
- use 'PLE.' for PanEnv.nLoopEnd
- use 'PiLE' for PitchEnv.nLoopEnd
- use 'VE[.' for VolEnv.Values[MAX_ENVPOINTS]


* In use CODE tag dictionary (alphabetical order):
--------------------------------------------------

CS.. nCutSwing
DCT. nDCT
dF.. dwFlags
DNA. nDNA
FM.. filterMode
fn[. filename[12]
FO.. nFadeOut
GV.. nGlobalVol
IFC. nIFC
IFR. nIFR
K[.. Keyboard[128]
MB.. wMidiBank
MC.. nMidiChannel
MDK. nMidiDrumKey
MiP. nMixPlug
MP.. nMidiProgram
n[.. name[32]
NNA. nNNA
NM[. NoteMap[128]
P... nPan
PE.. PanEnv.nNodes
PE[. PanEnv.Values[MAX_ENVPOINTS]
PiE. PitchEnv.nNodes
PiE[ PitchEnv.Values[MAX_ENVPOINTS]
PiLE PitchEnv.nLoopEnd
PiLS PitchEnv.nLoopStart
PiP[ PitchEnv.Ticks[MAX_ENVPOINTS]
PiSB PitchEnv.nSustainStart
PiSE PitchEnv.nSustainEnd
PLE. PanEnv.nLoopEnd
PLS. PanEnv.nLoopStart
PP[. PanEnv.Ticks[MAX_ENVPOINTS]
PPC. nPPC
PPS. nPPS
PS.. nPanSwing
PSB. PanEnv.nSustainStart
PSE. PanEnv.nSustainEnd
PTTL pitchToTempoLock
PTTF pitchToTempoLock (fractional part)
PVEH pluginVelocityHandling
PVOH pluginVolumeHandling
R... Resampling
RS.. nResSwing
VE.. VolEnv.nNodes
VE[. VolEnv.Values[MAX_ENVPOINTS]
VLE. VolEnv.nLoopEnd
VLS. VolEnv.nLoopStart
VP[. VolEnv.Ticks[MAX_ENVPOINTS]
VR.. nVolRampUp
VS.. nVolSwing
VSB. VolEnv.nSustainStart
VSE. VolEnv.nSustainEnd
PERN PitchEnv.nReleaseNode
AERN PanEnv.nReleaseNode
VERN VolEnv.nReleaseNode
PFLG PitchEnv.dwFlag
AFLG PanEnv.dwFlags
VFLG VolEnv.dwFlags
MPWD MIDI Pitch Wheel Depth

Note that many of these extensions were only relevant for ITP files, and thus there is no code for writing them, only reading.
Some of them used to be written but were never read ("K[.." sample map - it was only relevant for ITP files, but even there
it was always ignored, because sample indices may change when loading external instruments).


-----------------------------------------------------------------------------------------------
---------------------------------------------------------------------------------------------*/

#ifndef MODPLUG_NO_FILESAVE

template<typename T, bool is_signed> struct IsNegativeFunctor { bool operator()(T val) const { return val < 0; } };
template<typename T> struct IsNegativeFunctor<T, true> { bool operator()(T val) const { return val < 0; } };
template<typename T> struct IsNegativeFunctor<T, false> { bool operator()(T /*val*/) const { return false; } };

template<typename T>
bool IsNegative(const T &val)
{
	return IsNegativeFunctor<T, std::numeric_limits<T>::is_signed>()(val);
}

// ------------------------------------------------------------------------------------------
// Convenient macro to help WRITE_HEADER declaration for single type members ONLY (non-array)
// ------------------------------------------------------------------------------------------
#define WRITE_MPTHEADER_sized_member(name,type,code) \
	static_assert(sizeof(input->name) == sizeof(type), "Instrument property does match specified type!");\
	fcode = code;\
	fsize = sizeof( type );\
	if(writeAll) \
	{ \
		mpt::IO::WriteIntLE<uint32>(file, fcode); \
		mpt::IO::WriteIntLE<uint16>(file, fsize); \
	} else if(onlyThisCode == fcode)\
	{ \
		MPT_ASSERT(fixedsize == fsize); \
	} \
	if(onlyThisCode == fcode || onlyThisCode == Util::MaxValueOfType(onlyThisCode)) \
	{ \
		type tmp = (type)(input-> name ); \
		mpt::IO::WriteIntLE(file, tmp); \
	} \
/**/

// -----------------------------------------------------------------------------------------------------
// Convenient macro to help WRITE_HEADER declaration for single type members which are written truncated
// -----------------------------------------------------------------------------------------------------
#define WRITE_MPTHEADER_trunc_member(name,type,code) \
	static_assert(sizeof(input->name) > sizeof(type), "Instrument property would not be truncated, use WRITE_MPTHEADER_sized_member instead!");\
	fcode = code;\
	fsize = sizeof( type );\
	if(writeAll) \
	{ \
		mpt::IO::WriteIntLE<uint32>(file, fcode); \
		mpt::IO::WriteIntLE<uint16>(file, fsize); \
		type tmp = (type)(input-> name ); \
		mpt::IO::WriteIntLE(file, tmp); \
	} else if(onlyThisCode == fcode)\
	{ \
		/* hackish workaround to resolve mismatched size values: */ \
		/* nResampling was a long time declared as uint32 but these macro tables used uint16 and UINT. */ \
		/* This worked fine on little-endian, on big-endian not so much. Thus support writing size-mismatched fields. */ \
		MPT_ASSERT(fixedsize >= fsize); \
		type tmp = (type)(input-> name ); \
		mpt::IO::WriteIntLE(file, tmp); \
		if(fixedsize > fsize) \
		{ \
			for(int16 i = 0; i < fixedsize - fsize; ++i) \
			{ \
				uint8 fillbyte = !IsNegative(tmp) ? 0 : 0xff; /* sign extend */ \
				mpt::IO::WriteIntLE(file, fillbyte); \
			} \
		} \
	} \
/**/

// ------------------------------------------------------------------------
// Convenient macro to help WRITE_HEADER declaration for envelope members ONLY
// ------------------------------------------------------------------------
#define WRITE_MPTHEADER_envelope_member(envType,envField,type,code) \
	{\
		const InstrumentEnvelope &env = input->GetEnvelope(envType); \
		static_assert(sizeof(type) == sizeof(env[0]. envField)); \
		fcode = code;\
		fsize = mpt::saturate_cast<int16>(sizeof( type ) * env.size());\
		MPT_ASSERT(size_t(fsize) == sizeof( type ) * env.size()); \
		\
		if(writeAll) \
		{ \
			mpt::IO::WriteIntLE<uint32>(file, fcode); \
			mpt::IO::WriteIntLE<uint16>(file, fsize); \
		} else if(onlyThisCode == fcode)\
		{ \
			fsize = fixedsize; /* just trust the size we got passed */ \
		} \
		if(onlyThisCode == fcode || onlyThisCode == Util::MaxValueOfType(onlyThisCode)) \
		{ \
			uint32 maxNodes = std::min(static_cast<uint32>(fsize/sizeof(type)), static_cast<uint32>(env.size())); \
			for(uint32 i = 0; i < maxNodes; ++i) \
			{ \
				type tmp; \
				tmp = env[i]. envField ; \
				mpt::IO::WriteIntLE(file, tmp); \
			} \
			/* Not every instrument's envelope will be the same length. fill up with zeros. */ \
			for(uint32 i = maxNodes; i < fsize/sizeof(type); ++i) \
			{ \
				type tmp = 0; \
				mpt::IO::WriteIntLE(file, tmp); \
			} \
		} \
	}\
/**/


// Write (in 'file') 'input' ModInstrument with 'code' & 'size' extra field infos for each member
void WriteInstrumentHeaderStructOrField(ModInstrument * input, std::ostream &file, uint32 onlyThisCode, uint16 fixedsize)
{
	uint32 fcode;
	uint16 fsize;
	// If true, all extension are written to the file; otherwise only the specified extension is written.
	// writeAll is true iff we are saving an instrument (or, hypothetically, the legacy ITP format)
	const bool writeAll = onlyThisCode == Util::MaxValueOfType(onlyThisCode);

	if(!writeAll)
	{
		MPT_ASSERT(fixedsize > 0);
	}

	// clang-format off
	WRITE_MPTHEADER_sized_member(	nFadeOut					, uint32	, MagicBE("FO..")	)
	WRITE_MPTHEADER_sized_member(	nPan						, uint32	, MagicBE("P...")	)
	WRITE_MPTHEADER_sized_member(	VolEnv.size()				, uint32	, MagicBE("VE..")	)
	WRITE_MPTHEADER_sized_member(	PanEnv.size()				, uint32	, MagicBE("PE..")	)
	WRITE_MPTHEADER_sized_member(	PitchEnv.size()				, uint32	, MagicBE("PiE.")	)
	WRITE_MPTHEADER_sized_member(	wMidiBank					, uint16	, MagicBE("MB..")	)
	WRITE_MPTHEADER_sized_member(	nMidiProgram				, uint8		, MagicBE("MP..")	)
	WRITE_MPTHEADER_sized_member(	nMidiChannel				, uint8		, MagicBE("MC..")	)
	WRITE_MPTHEADER_envelope_member(	ENV_VOLUME	, tick		, uint16	, MagicBE("VP[.")	)
	WRITE_MPTHEADER_envelope_member(	ENV_PANNING	, tick		, uint16	, MagicBE("PP[.")	)
	WRITE_MPTHEADER_envelope_member(	ENV_PITCH	, tick		, uint16	, MagicBE("PiP[")	)
	WRITE_MPTHEADER_envelope_member(	ENV_VOLUME	, value		, uint8		, MagicBE("VE[.")	)
	WRITE_MPTHEADER_envelope_member(	ENV_PANNING	, value		, uint8		, MagicBE("PE[.")	)
	WRITE_MPTHEADER_envelope_member(	ENV_PITCH	, value		, uint8		, MagicBE("PiE[")	)
	WRITE_MPTHEADER_sized_member(	nMixPlug					, uint8		, MagicBE("MiP.")	)
	WRITE_MPTHEADER_sized_member(	nVolRampUp					, uint16	, MagicBE("VR..")	)
	WRITE_MPTHEADER_sized_member(	resampling					, uint8		, MagicBE("R...")	)
	WRITE_MPTHEADER_sized_member(	nCutSwing					, uint8		, MagicBE("CS..")	)
	WRITE_MPTHEADER_sized_member(	nResSwing					, uint8		, MagicBE("RS..")	)
	WRITE_MPTHEADER_sized_member(	filterMode					, uint8		, MagicBE("FM..")	)
	WRITE_MPTHEADER_sized_member(	pluginVelocityHandling		, uint8		, MagicBE("PVEH")	)
	WRITE_MPTHEADER_sized_member(	pluginVolumeHandling		, uint8		, MagicBE("PVOH")	)
	WRITE_MPTHEADER_trunc_member(	pitchToTempoLock.GetInt()	, uint16	, MagicBE("PTTL")	)
	WRITE_MPTHEADER_trunc_member(	pitchToTempoLock.GetFract() , uint16	, MagicLE("PTTF")	)
	WRITE_MPTHEADER_sized_member(	PitchEnv.nReleaseNode		, uint8		, MagicBE("PERN")	)
	WRITE_MPTHEADER_sized_member(	PanEnv.nReleaseNode			, uint8		, MagicBE("AERN")	)
	WRITE_MPTHEADER_sized_member(	VolEnv.nReleaseNode			, uint8		, MagicBE("VERN")	)
	WRITE_MPTHEADER_sized_member(	midiPWD						, int8		, MagicBE("MPWD")	)
	// clang-format on

}


template<typename TIns, typename PropType>
static bool IsPropertyNeeded(const TIns &Instruments, PropType ModInstrument::*Prop)
{
	const ModInstrument defaultIns;
	for(const auto ins : Instruments)
	{
		if(ins != nullptr && defaultIns.*Prop != ins->*Prop)
			return true;
	}
	return false;
}


template<typename PropType>
static void WritePropertyIfNeeded(const CSoundFile &sndFile, PropType ModInstrument::*Prop, uint32 code, uint16 size, std::ostream &f, INSTRUMENTINDEX numInstruments)
{
	if(IsPropertyNeeded(sndFile.Instruments, Prop))
	{
		sndFile.WriteInstrumentPropertyForAllInstruments(code, size, f, numInstruments);
	}
}


// Used only when saving IT, XM and MPTM.
// ITI, ITP saves using Ericus' macros etc...
// The reason is that ITs and XMs save [code][size][ins1.Value][ins2.Value]...
// whereas ITP saves [code][size][ins1.Value][code][size][ins2.Value]...
// too late to turn back....
void CSoundFile::SaveExtendedInstrumentProperties(INSTRUMENTINDEX numInstruments, std::ostream &f) const
{
	uint32 code = MagicBE("MPTX");	// write extension header code
	mpt::IO::WriteIntLE<uint32>(f, code);

	if (numInstruments == 0)
		return;

	WritePropertyIfNeeded(*this, &ModInstrument::nVolRampUp,   MagicBE("VR.."), sizeof(ModInstrument::nVolRampUp),   f, numInstruments);
	WritePropertyIfNeeded(*this, &ModInstrument::nMixPlug,     MagicBE("MiP."), sizeof(ModInstrument::nMixPlug),     f, numInstruments);
	WritePropertyIfNeeded(*this, &ModInstrument::nMidiChannel, MagicBE("MC.."), sizeof(ModInstrument::nMidiChannel), f, numInstruments);
	WritePropertyIfNeeded(*this, &ModInstrument::nMidiProgram, MagicBE("MP.."), sizeof(ModInstrument::nMidiProgram), f, numInstruments);
	WritePropertyIfNeeded(*this, &ModInstrument::wMidiBank,    MagicBE("MB.."), sizeof(ModInstrument::wMidiBank),    f, numInstruments);
	WritePropertyIfNeeded(*this, &ModInstrument::resampling,   MagicBE("R..."), sizeof(ModInstrument::resampling),   f, numInstruments);
	WritePropertyIfNeeded(*this, &ModInstrument::pluginVelocityHandling, MagicBE("PVEH"), sizeof(ModInstrument::pluginVelocityHandling), f, numInstruments);
	WritePropertyIfNeeded(*this, &ModInstrument::pluginVolumeHandling,   MagicBE("PVOH"), sizeof(ModInstrument::pluginVolumeHandling),   f, numInstruments);

	if(!(GetType() & MOD_TYPE_XM))
	{
		// XM instrument headers already stores full-precision fade-out
		bool writeFadeOut = false, writePan = false, writePWD = false;
		int32 prevPWD = int32_min;
		for(const auto ins : Instruments)
		{
			if(ins == nullptr)
				continue;
			if((ins->nFadeOut % 32u) || ins->nFadeOut > 8192)
				writeFadeOut = true;
			if(ins->dwFlags[INS_SETPANNING] && (ins->nPan % 4u))
				writePan = true;
			if((prevPWD != int32_min && ins->midiPWD != prevPWD) || (ins->midiPWD < 0))
				writePWD = true;
			prevPWD = ins->midiPWD;
		}
		if(writeFadeOut)
			WriteInstrumentPropertyForAllInstruments(MagicBE("FO.."), sizeof(ModInstrument::nFadeOut), f, numInstruments);
		// XM instrument headers already have support for this
		if(writePWD)
			WriteInstrumentPropertyForAllInstruments(MagicBE("MPWD"), sizeof(ModInstrument::midiPWD), f, numInstruments);
		// We never supported these as hacks in XM (luckily!)
		if(writePan)
			WriteInstrumentPropertyForAllInstruments(MagicBE("P..."), sizeof(ModInstrument::nPan), f, numInstruments);
		WritePropertyIfNeeded(*this, &ModInstrument::nCutSwing, MagicBE("CS.."), sizeof(ModInstrument::nCutSwing), f, numInstruments);
		WritePropertyIfNeeded(*this, &ModInstrument::nResSwing, MagicBE("RS.."), sizeof(ModInstrument::nResSwing), f, numInstruments);
		WritePropertyIfNeeded(*this, &ModInstrument::filterMode, MagicBE("FM.."), sizeof(ModInstrument::filterMode), f, numInstruments);
		if(IsPropertyNeeded(Instruments, &ModInstrument::pitchToTempoLock))
		{
			WriteInstrumentPropertyForAllInstruments(MagicBE("PTTL"), sizeof(uint16), f, numInstruments);
			WriteInstrumentPropertyForAllInstruments(MagicLE("PTTF"), sizeof(uint16), f, numInstruments);
		}
	}

	if(GetType() & MOD_TYPE_MPT)
	{
		uint32 maxNodes[3] = { 0, 0, 0 };
		bool hasReleaseNode[3] = { false, false, false };
		for(INSTRUMENTINDEX i = 1; i <= numInstruments; i++) if(Instruments[i] != nullptr)
		{
			maxNodes[0] = std::max(maxNodes[0], Instruments[i]->VolEnv.size());
			maxNodes[1] = std::max(maxNodes[1], Instruments[i]->PanEnv.size());
			maxNodes[2] = std::max(maxNodes[2], Instruments[i]->PitchEnv.size());
			hasReleaseNode[0] |= (Instruments[i]->VolEnv.nReleaseNode != ENV_RELEASE_NODE_UNSET);
			hasReleaseNode[1] |= (Instruments[i]->PanEnv.nReleaseNode != ENV_RELEASE_NODE_UNSET);
			hasReleaseNode[2] |= (Instruments[i]->PitchEnv.nReleaseNode != ENV_RELEASE_NODE_UNSET);
		}
		// write full envelope information for MPTM files (more env points)
		if(maxNodes[0] > 25)
		{
			WriteInstrumentPropertyForAllInstruments(MagicBE("VE.."), sizeof(ModInstrument::VolEnv.size()), f, numInstruments);
			WriteInstrumentPropertyForAllInstruments(MagicBE("VP[."), static_cast<uint16>(maxNodes[0] * sizeof(EnvelopeNode::tick)),  f, numInstruments);
			WriteInstrumentPropertyForAllInstruments(MagicBE("VE[."), static_cast<uint16>(maxNodes[0] * sizeof(EnvelopeNode::value)), f, numInstruments);
		}
		if(maxNodes[1] > 25)
		{
			WriteInstrumentPropertyForAllInstruments(MagicBE("PE.."), sizeof(ModInstrument::PanEnv.size()), f, numInstruments);
			WriteInstrumentPropertyForAllInstruments(MagicBE("PP[."), static_cast<uint16>(maxNodes[1] * sizeof(EnvelopeNode::tick)),  f, numInstruments);
			WriteInstrumentPropertyForAllInstruments(MagicBE("PE[."), static_cast<uint16>(maxNodes[1] * sizeof(EnvelopeNode::value)), f, numInstruments);
		}
		if(maxNodes[2] > 25)
		{
			WriteInstrumentPropertyForAllInstruments(MagicBE("PiE."), sizeof(ModInstrument::PitchEnv.size()), f, numInstruments);
			WriteInstrumentPropertyForAllInstruments(MagicBE("PiP["), static_cast<uint16>(maxNodes[2] * sizeof(EnvelopeNode::tick)),  f, numInstruments);
			WriteInstrumentPropertyForAllInstruments(MagicBE("PiE["), static_cast<uint16>(maxNodes[2] * sizeof(EnvelopeNode::value)), f, numInstruments);
		}
		if(hasReleaseNode[0])
			WriteInstrumentPropertyForAllInstruments(MagicBE("VERN"), sizeof(ModInstrument::VolEnv.nReleaseNode), f, numInstruments);
		if(hasReleaseNode[1])
			WriteInstrumentPropertyForAllInstruments(MagicBE("AERN"), sizeof(ModInstrument::PanEnv.nReleaseNode), f, numInstruments);
		if(hasReleaseNode[2])
			WriteInstrumentPropertyForAllInstruments(MagicBE("PERN"), sizeof(ModInstrument::PitchEnv.nReleaseNode), f, numInstruments);
	}
}

void CSoundFile::WriteInstrumentPropertyForAllInstruments(uint32 code, uint16 size, std::ostream &f, INSTRUMENTINDEX nInstruments) const
{
	mpt::IO::WriteIntLE<uint32>(f, code);		//write code
	mpt::IO::WriteIntLE<uint16>(f, size);		//write size
	for(INSTRUMENTINDEX i = 1; i <= nInstruments; i++)	//for all instruments...
	{
		if (Instruments[i])
		{
			WriteInstrumentHeaderStructOrField(Instruments[i], f, code, size);
		} else
		{
			ModInstrument emptyInstrument;
			WriteInstrumentHeaderStructOrField(&emptyInstrument, f, code, size);
		}
	}
}


#endif // !MODPLUG_NO_FILESAVE


// Convert instrument flags which were read from 'dF..' extension to proper internal representation.
static void ConvertInstrumentFlags(ModInstrument &ins, uint32 flags)
{
	ins.VolEnv.dwFlags.set(ENV_ENABLED, (flags & 0x0001) != 0);
	ins.VolEnv.dwFlags.set(ENV_SUSTAIN, (flags & 0x0002) != 0);
	ins.VolEnv.dwFlags.set(ENV_LOOP, (flags & 0x0004) != 0);
	ins.VolEnv.dwFlags.set(ENV_CARRY, (flags & 0x0800) != 0);

	ins.PanEnv.dwFlags.set(ENV_ENABLED, (flags & 0x0008) != 0);
	ins.PanEnv.dwFlags.set(ENV_SUSTAIN, (flags & 0x0010) != 0);
	ins.PanEnv.dwFlags.set(ENV_LOOP, (flags & 0x0020) != 0);
	ins.PanEnv.dwFlags.set(ENV_CARRY, (flags & 0x1000) != 0);

	ins.PitchEnv.dwFlags.set(ENV_ENABLED, (flags & 0x0040) != 0);
	ins.PitchEnv.dwFlags.set(ENV_SUSTAIN, (flags & 0x0080) != 0);
	ins.PitchEnv.dwFlags.set(ENV_LOOP, (flags & 0x0100) != 0);
	ins.PitchEnv.dwFlags.set(ENV_CARRY, (flags & 0x2000) != 0);
	ins.PitchEnv.dwFlags.set(ENV_FILTER, (flags & 0x0400) != 0);

	ins.dwFlags.set(INS_SETPANNING, (flags & 0x0200) != 0);
	ins.dwFlags.set(INS_MUTE, (flags & 0x4000) != 0);
}


// Convert VFLG / PFLG / AFLG
static void ConvertEnvelopeFlags(ModInstrument &instr, uint32 flags, EnvelopeType envType)
{
	InstrumentEnvelope &env = instr.GetEnvelope(envType);
	env.dwFlags.set(ENV_ENABLED, (flags & 0x01) != 0);
	env.dwFlags.set(ENV_LOOP, (flags & 0x02) != 0);
	env.dwFlags.set(ENV_SUSTAIN, (flags & 0x04) != 0);
	env.dwFlags.set(ENV_CARRY, (flags & 0x08) != 0);
	env.dwFlags.set(ENV_FILTER, (envType == ENV_PITCH) && (flags & 0x10) != 0);
}


static void ReadInstrumentHeaderField(ModInstrument &ins, uint32 fcode, FileReader &file)
{
	const size_t size = static_cast<size_t>(file.GetLength());

	// Note: Various int / enum members have changed their size over the past.
	// Hence we use ReadSizedIntLE everywhere to allow reading both truncated and oversized values.
	constexpr auto ReadInt = [](FileReader &file, auto size, auto &member)
	{
		using T = std::remove_reference_t<decltype(member)>;
		member = file.ReadSizedIntLE<T>(size);
	};
	constexpr auto ReadEnum = [](FileReader &file, auto size, auto &member)
	{
		using T = std::remove_reference_t<decltype(member)>;
		static_assert(std::is_enum_v<T>);
		member = static_cast<T>(file.ReadSizedIntLE<std::underlying_type_t<T>>(size));
	};
	constexpr auto ReadEnvelopeTicks = [](FileReader &file, auto size, InstrumentEnvelope &env)
	{
		const uint32 points = std::min(env.size(), static_cast<uint32>(size / 2));
		for(uint32 i = 0; i < points; i++)
		{
			env[i].tick = file.ReadUint16LE();
		}
	};
	constexpr auto ReadEnvelopeValues = [](FileReader &file, auto size, InstrumentEnvelope &env)
	{
		const uint32 points = std::min(env.size(), static_cast<uint32>(size));
		for(uint32 i = 0; i < points; i++)
		{
			env[i].value = file.ReadUint8();
		}
	};

	// Members which can be found in this table but not in the write table are only required in the legacy ITP format.
	switch(fcode)
	{
	case MagicBE("FO.."): ReadInt(file, size, ins.nFadeOut); break;
	case MagicBE("GV.."): ReadInt(file, size, ins.nGlobalVol); break;
	case MagicBE("P..."): ReadInt(file, size, ins.nPan); break;
	case MagicBE("VLS."): ReadInt(file, size, ins.VolEnv.nLoopStart); break;
	case MagicBE("VLE."): ReadInt(file, size, ins.VolEnv.nLoopEnd); break;
	case MagicBE("VSB."): ReadInt(file, size, ins.VolEnv.nSustainStart); break;
	case MagicBE("VSE."): ReadInt(file, size, ins.VolEnv.nSustainEnd); break;
	case MagicBE("PLS."): ReadInt(file, size, ins.PanEnv.nLoopStart); break;
	case MagicBE("PLE."): ReadInt(file, size, ins.PanEnv.nLoopEnd); break;
	case MagicBE("PSB."): ReadInt(file, size, ins.PanEnv.nSustainStart); break;
	case MagicBE("PSE."): ReadInt(file, size, ins.PanEnv.nSustainEnd); break;
	case MagicBE("PiLS"): ReadInt(file, size, ins.PitchEnv.nLoopStart); break;
	case MagicBE("PiLE"): ReadInt(file, size, ins.PitchEnv.nLoopEnd); break;
	case MagicBE("PiSB"): ReadInt(file, size, ins.PitchEnv.nSustainStart); break;
	case MagicBE("PiSE"): ReadInt(file, size, ins.PitchEnv.nSustainEnd); break;
	case MagicBE("NNA."): ReadEnum(file, size, ins.nNNA); break;
	case MagicBE("DCT."): ReadEnum(file, size, ins.nDCT); break;
	case MagicBE("DNA."): ReadEnum(file, size, ins.nDNA); break;
	case MagicBE("PS.."): ReadInt(file, size, ins.nPanSwing); break;
	case MagicBE("VS.."): ReadInt(file, size, ins.nVolSwing); break;
	case MagicBE("IFC."): ReadInt(file, size, ins.nIFC); break;
	case MagicBE("IFR."): ReadInt(file, size, ins.nIFR); break;
	case MagicBE("MB.."): ReadInt(file, size, ins.wMidiBank); break;
	case MagicBE("MP.."): ReadInt(file, size, ins.nMidiProgram); break;
	case MagicBE("MC.."): ReadInt(file, size, ins.nMidiChannel); break;
	case MagicBE("PPS."): ReadInt(file, size, ins.nPPS); break;
	case MagicBE("PPC."): ReadInt(file, size, ins.nPPC); break;
	case MagicBE("VP[."): ReadEnvelopeTicks(file, size, ins.VolEnv); break;
	case MagicBE("PP[."): ReadEnvelopeTicks(file, size, ins.PanEnv); break;
	case MagicBE("PiP["): ReadEnvelopeTicks(file, size, ins.PitchEnv); break;
	case MagicBE("VE[."): ReadEnvelopeValues(file, size, ins.VolEnv); break;
	case MagicBE("PE[."): ReadEnvelopeValues(file, size, ins.PanEnv); break;
	case MagicBE("PiE["): ReadEnvelopeValues(file, size, ins.PitchEnv); break;
	case MagicBE("MiP."): ReadInt(file, size, ins.nMixPlug); break;
	case MagicBE("VR.."): ReadInt(file, size, ins.nVolRampUp); break;
	case MagicBE("CS.."): ReadInt(file, size, ins.nCutSwing); break;
	case MagicBE("RS.."): ReadInt(file, size, ins.nResSwing); break;
	case MagicBE("FM.."): ReadEnum(file, size, ins.filterMode); break;
	case MagicBE("PVEH"): ReadEnum(file, size, ins.pluginVelocityHandling); break;
	case MagicBE("PVOH"): ReadEnum(file, size, ins.pluginVolumeHandling); break;
	case MagicBE("PERN"): ReadInt(file, size, ins.PitchEnv.nReleaseNode); break;
	case MagicBE("AERN"): ReadInt(file, size, ins.PanEnv.nReleaseNode); break;
	case MagicBE("VERN"): ReadInt(file, size, ins.VolEnv.nReleaseNode); break;
	case MagicBE("MPWD"): ReadInt(file, size, ins.midiPWD); break;
	case MagicBE("dF.."):
		ConvertInstrumentFlags(ins, file.ReadSizedIntLE<uint32>(size));
		break;
	case MagicBE("VFLG"):
		ConvertEnvelopeFlags(ins, file.ReadSizedIntLE<uint32>(size), ENV_VOLUME);
		break;
	case MagicBE("AFLG"):
		ConvertEnvelopeFlags(ins, file.ReadSizedIntLE<uint32>(size), ENV_PANNING);
		break;
	case MagicBE("PFLG"):
		ConvertEnvelopeFlags(ins, file.ReadSizedIntLE<uint32>(size), ENV_PITCH);
		break;
	case MagicBE("NM[."):
		for(std::size_t i = 0; i < std::min(size, ins.NoteMap.size()); i++)
		{
			ins.NoteMap[i] = file.ReadUint8();
		}
		break;
	case MagicBE("n[.."):
		{
			char name[32] = "";
			file.ReadString<mpt::String::maybeNullTerminated>(name, size);
			ins.name = name;
		}
		break;
	case MagicBE("fn[."):
		{
			char filename[32] = "";
			file.ReadString<mpt::String::maybeNullTerminated>(filename, size);
			ins.filename = filename;
		}
		break;
	case MagicBE("R..."):
		// Resampling has been written as various sizes including uint16 and uint32 in the past
		if(uint32 resampling = file.ReadSizedIntLE<uint32>(size); Resampling::IsKnownMode(resampling))
			ins.resampling = static_cast<ResamplingMode>(resampling);
		break;
	case MagicBE("PTTL"):
		// Integer part of pitch/tempo lock
		ins.pitchToTempoLock.Set(file.ReadSizedIntLE<uint16>(size), ins.pitchToTempoLock.GetFract());
		break;
	case MagicLE("PTTF"):
		// Fractional part of pitch/tempo lock
		ins.pitchToTempoLock.Set(ins.pitchToTempoLock.GetInt(), file.ReadSizedIntLE<uint16>(size));
		break;
	case MagicBE("VE.."):
		ins.VolEnv.resize(std::min(uint32(MAX_ENVPOINTS), file.ReadSizedIntLE<uint32>(size)));
		break;
	case MagicBE("PE.."):
		ins.PanEnv.resize(std::min(uint32(MAX_ENVPOINTS), file.ReadSizedIntLE<uint32>(size)));
		break;
	case MagicBE("PiE."):
		ins.PitchEnv.resize(std::min(uint32(MAX_ENVPOINTS), file.ReadSizedIntLE<uint32>(size)));
		break;
	}
}


// For ITP and internal usage
void ReadExtendedInstrumentProperty(ModInstrument *ins, const uint32 code, FileReader &file)
{
	uint16 size = file.ReadUint16LE();
	FileReader chunk = file.ReadChunk(size);
	if(ins && chunk.GetLength() == size)
		ReadInstrumentHeaderField(*ins, code, chunk);
}


// For ITI / XI
void ReadExtendedInstrumentProperties(ModInstrument &ins, FileReader &file)
{
	if(!file.ReadMagic("XTPM"))  // 'MPTX'
		return;

	while(file.CanRead(7))
	{
		ReadExtendedInstrumentProperty(&ins, file.ReadUint32LE(), file);
	}
}


// For IT / XM / MO3
bool CSoundFile::LoadExtendedInstrumentProperties(FileReader &file)
{
	if(!file.ReadMagic("XTPM"))  // 'MPTX'
		return false;

	while(file.CanRead(6))
	{
		uint32 code = file.ReadUint32LE();

		if(code == MagicBE("MPTS")                          // Reached song extensions, break out of this loop
		   || code == MagicLE("228\x04")                    // Reached MPTM extensions (in case there are no song extensions)
		   || (code & 0x80808080) || !(code & 0x60606060))  // Non-ASCII chunk ID
		{
			file.SkipBack(4);
			break;
		}

		// Read size of this property for *one* instrument
		const uint16 size = file.ReadUint16LE();

		for(INSTRUMENTINDEX i = 1; i <= GetNumInstruments(); i++)
		{
			FileReader chunk = file.ReadChunk(size);
			if(Instruments[i] && chunk.GetLength() == size)
				ReadInstrumentHeaderField(*Instruments[i], code, chunk);
		}
	}
	return true;
}


OPENMPT_NAMESPACE_END
