/*
 * OPL.cpp
 * -------
 * Purpose: Translate data coming from OpenMPT's mixer into OPL commands to be sent to the Opal emulator.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 *          Schism Tracker contributors (bisqwit, JosepMa, Malvineous, code relicensed from GPL to BSD with permission)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"
#include "../common/misc_util.h"
#include "OPL.h"
#include "opal.h"

OPENMPT_NAMESPACE_BEGIN

enum OPLRegisters
{
	// Operators (combine with result of OperatorToRegister)
	AM_VIB          = 0x20, // AM / VIB / EG / KSR / Multiple (0x20 to 0x35)
	KSL_LEVEL       = 0x40, // KSL / Total level (0x40 to 0x55)
	ATTACK_DECAY    = 0x60, // Attack rate / Decay rate (0x60 to 0x75)
	SUSTAIN_RELEASE = 0x80, // Sustain level / Release rate (0x80 to 0x95)
	WAVE_SELECT     = 0xE0, // Wave select (0xE0 to 0xF5)

	// Channels (combine with result of ChannelToRegister)
	FNUM_LOW            = 0xA0, // F-number low bits (0xA0 to 0xA8)
	KEYON_BLOCK         = 0xB0, // F-number high bits / Key on / Block (octave) (0xB0 to 0xB8)
	FEEDBACK_CONNECTION = 0xC0, // Feedback / Connection (0xC0 to 0xC8)
};


enum OPLValues
{
	// AM_VIB
	TREMOLO_ON       = 0x80,
	VIBRATO_ON       = 0x40,
	SUSTAIN_ON       = 0x20,
	KSR              = 0x10, // Key scaling rate
	MULTIPLE_MASK    = 0x0F, // Frequency multiplier

	// KSL_LEVEL
	KSL_MASK         = 0xC0, // Envelope scaling bits
	TOTAL_LEVEL_MASK = 0x3F, // Strength (volume) of OP

	// ATTACK_DECAY
	ATTACK_MASK      = 0xF0,
	DECAY_MASK       = 0x0F,

	// SUSTAIN_RELEASE
	SUSTAIN_MASK     = 0xF0,
	RELEASE_MASK     = 0x0F,

	// KEYON_BLOCK
	KEYON_BIT        = 0x20,
	BLOCKNUM_MASK    = 0x1C,
	FNUM_HIGH_MASK   = 0x03,

	// FEEDBACK_CONNECTION
	FEEDBACK_MASK    = 0x0E, // Valid just for 1st OP of a voice
	CONNECTION_BIT   = 0x01,
	STEREO_BITS      = 0x30,
	VOICE_TO_LEFT    = 0x10,
	VOICE_TO_RIGHT   = 0x20,
};


OPL::OPL()
{
	m_KeyOnBlock.fill(0);
	m_Panning.fill(0);
	m_OPLtoChan.fill(CHANNELINDEX_INVALID);
	m_ChanToOPL.fill(OPL_CHANNEL_INVALID);
}


OPL::~OPL()
{
	// This destructor is put here so that we can forward-declare the Opal emulator class.
}


void OPL::Initialize(uint32 samplerate)
{
	if(m_opl == nullptr)
		m_opl = mpt::make_unique<Opal>(samplerate);
	else
		m_opl->SetSampleRate(samplerate);
	Reset();
}


void OPL::Mix(int *target, size_t count)
{
	if(!isActive)
		return;

	while(count--)
	{
		int16 l, r;
		m_opl->Sample(&l, &r);
		target[0] += l * (1 << 13);
		target[1] += r * (1 << 13);
		target += 2;
	}
}


uint16 OPL::ChannelToRegister(uint8 oplCh)
{
	if(oplCh < 9)
		return oplCh;
	else
		return (oplCh - 9) | 0x100;
}


// Translate a channel's first operator address into a register
uint16 OPL::OperatorToRegister(uint8 oplCh)
{
	static const uint8 OPLChannelToOperator[] = { 0, 1, 2, 8, 9, 10, 16, 17, 18 };
	if(oplCh < 9)
		return OPLChannelToOperator[oplCh];
	else
		return OPLChannelToOperator[oplCh - 9] | 0x100;
}


uint8 OPL::GetVoice(CHANNELINDEX c) const
{
	return m_ChanToOPL[c];
}


uint8 OPL::AllocateVoice(CHANNELINDEX c)
{
	// Can we re-use a previous channel?
	if(m_ChanToOPL[c] != OPL_CHANNEL_INVALID)
	{
		return GetVoice(c);
	}
	// Search for unused channel or channel with released note
	uint8 releasedChn = OPL_CHANNEL_INVALID;
	for(uint8 oplCh = 0; oplCh < OPL_CHANNELS; oplCh++)
	{
		if(m_OPLtoChan[oplCh] == CHANNELINDEX_INVALID)
		{
			m_OPLtoChan[oplCh] = c;
			m_ChanToOPL[c] = oplCh;
			return GetVoice(c);
		} else if(!(m_KeyOnBlock[oplCh] & KEYON_BIT))
		{
			releasedChn = oplCh;
		}
	}
	if(releasedChn != OPL_CHANNEL_INVALID)
	{
		m_ChanToOPL[m_OPLtoChan[releasedChn]] = OPL_CHANNEL_INVALID;
		m_OPLtoChan[releasedChn] = c;
		m_ChanToOPL[c] = releasedChn;
	}
	return GetVoice(c);
}


void OPL::NoteOff(CHANNELINDEX c)
{
	uint8 oplCh = GetVoice(c);
	if(oplCh == OPL_CHANNEL_INVALID || m_opl == nullptr)
		return;
	m_KeyOnBlock[oplCh] &= ~KEYON_BIT;
	m_opl->Port(KEYON_BLOCK | ChannelToRegister(oplCh), m_KeyOnBlock[oplCh]);
}


void OPL::NoteCut(CHANNELINDEX c)
{
	NoteOff(c);
	Volume(c, 0);
}


void OPL::Frequency(CHANNELINDEX c, uint32 milliHertz, bool keyOff)
{
	uint8 oplCh = GetVoice(c);
	if(oplCh == OPL_CHANNEL_INVALID || m_opl == nullptr)
		return;

	uint16 fnum = 0;
	uint8 block = 0;
	if(milliHertz > 6208431)
	{
		// Frequencies too high to produce
		block = 7;
		fnum = 1023;
	} else
	{
		if(milliHertz > 3104215) block = 7;
		else if(milliHertz > 1552107) block = 6;
		else if(milliHertz > 776053) block = 5;
		else if(milliHertz > 388026) block = 4;
		else if(milliHertz > 194013) block = 3;
		else if(milliHertz > 97006) block = 2;
		else if(milliHertz > 48503) block = 1;
		else block = 0;

		fnum = static_cast<uint16>(Util::muldivr_unsigned(milliHertz, 1 << (20 - block), OPL_BASERATE * 1000));
		MPT_ASSERT(fnum < 1024);
	}

	uint16 channel = ChannelToRegister(oplCh);
	m_KeyOnBlock[oplCh] = (keyOff ? 0 : KEYON_BIT)   // Key on
	    | (block << 2)                               // Octave
	    | ((fnum >> 8) & FNUM_HIGH_MASK);            // F-number high 2 bits
	m_opl->Port(FNUM_LOW    | channel, fnum & 0xFF); // F-Number low 8 bits
	m_opl->Port(KEYON_BLOCK | channel, m_KeyOnBlock[oplCh]);

	isActive = true;
}


void OPL::Volume(CHANNELINDEX c, uint8 vol)
{
	uint8 oplCh = GetVoice(c);
	if(oplCh == OPL_CHANNEL_INVALID || m_opl == nullptr)
		return;

	const auto &patch = m_Patches[oplCh];
	m_opl->Port(KSL_LEVEL + OperatorToRegister(oplCh) + 3,
		(patch[3] & KSL_MASK) | (63 + ((patch[3] & TOTAL_LEVEL_MASK) * vol / 63) - vol));

}


void OPL::Pan(CHANNELINDEX c, int32 pan)
{
	if(pan < 85)
		m_Panning[c] = VOICE_TO_LEFT;
	else if(pan > 170)
		m_Panning[c] = VOICE_TO_RIGHT;
	else
		m_Panning[c] = VOICE_TO_LEFT | VOICE_TO_RIGHT;

	uint8 oplCh = GetVoice(c);
	if(oplCh == OPL_CHANNEL_INVALID || m_opl == nullptr)
		return;

	const auto &patch = m_Patches[oplCh];
	m_opl->Port(FEEDBACK_CONNECTION | ChannelToRegister(oplCh),
		(patch[10] & ~STEREO_BITS) | m_Panning[c]);
}


void OPL::Patch(CHANNELINDEX c, const OPLPatch &patch)
{
	uint8 oplCh = AllocateVoice(c);
	if(oplCh == OPL_CHANNEL_INVALID || m_opl == nullptr)
		return;

	m_Patches[oplCh] = patch;

	uint16 modulator = OperatorToRegister(oplCh);
	m_opl->Port(AM_VIB          | modulator, patch[0]);
	m_opl->Port(KSL_LEVEL       | modulator, patch[2]);
	m_opl->Port(ATTACK_DECAY    | modulator, patch[4]);
	m_opl->Port(SUSTAIN_RELEASE | modulator, patch[6]);
	m_opl->Port(WAVE_SELECT     | modulator, patch[8]);
	uint16 carrier = modulator + 3;
	m_opl->Port(AM_VIB          | carrier, patch[1]);
	m_opl->Port(KSL_LEVEL       | carrier, patch[3]);
	m_opl->Port(ATTACK_DECAY    | carrier, patch[5]);
	m_opl->Port(SUSTAIN_RELEASE | carrier, patch[7]);
	m_opl->Port(WAVE_SELECT     | carrier, patch[9]);

	// Keep panning bits from current channel panning
	m_opl->Port(FEEDBACK_CONNECTION | ChannelToRegister(oplCh),
		(patch[10] & ~STEREO_BITS) | m_Panning[c]);
}


void OPL::Reset()
{
	if(isActive)
	{
		for(CHANNELINDEX chn = 0; chn < MAX_CHANNELS; chn++)
		{
			NoteCut(chn);
		}
		isActive = false;
	}

	m_KeyOnBlock.fill(0);
	m_OPLtoChan.fill(CHANNELINDEX_INVALID);
	m_ChanToOPL.fill(OPL_CHANNEL_INVALID);
}

OPENMPT_NAMESPACE_END
