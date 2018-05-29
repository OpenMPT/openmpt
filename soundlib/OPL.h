/*
 * OPL.h
 * -----
 * Purpose: Translate data coming from OpenMPT's mixer into OPL commands to be sent to the Opal emulator.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "Snd_defs.h"

class Opal;

OPENMPT_NAMESPACE_BEGIN

class OPL
{
public:
	OPL();
	~OPL();

	void Initialize(uint32 samplerate);
	void Mix(int *buffer, size_t count);

	void NoteOff(CHANNELINDEX c);
	void NoteCut(CHANNELINDEX c);
	void Frequency(CHANNELINDEX c, uint32 milliHertz, bool keyOff);
	void Volume(CHANNELINDEX c, uint8 vol);
	void Pan(CHANNELINDEX c, int32 pan);
	void Patch(CHANNELINDEX c, const OPLPatch &patch);
	void Reset();

protected:
	static uint16 ChannelToRegister(uint8 oplCh);
	static uint16 OperatorToRegister(uint8 oplCh);
	uint8 GetVoice(CHANNELINDEX c) const;
	uint8 AllocateVoice(CHANNELINDEX c);

	enum
	{
		OPL_CHANNELS = 18,	// 9 for OPL2 or 18 for OPL3
		OPL_CHANNEL_INVALID = 0xFF,
		OPL_BASERATE = 49716,
	};

	std::unique_ptr<Opal> m_opl;

	std::array<uint8, OPL_CHANNELS> m_KeyOnBlock;
	std::array<CHANNELINDEX, OPL_CHANNELS> m_OPLtoChan;
	std::array<uint8, MAX_CHANNELS> m_ChanToOPL;
	std::array<uint8, MAX_CHANNELS> m_Panning;
	std::array<OPLPatch, OPL_CHANNELS> m_Patches;

	bool isActive = false;
};

OPENMPT_NAMESPACE_END
