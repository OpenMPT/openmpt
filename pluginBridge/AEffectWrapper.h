/*
 * AEffectWrapper.h
 * ----------------
 * Purpose: Helper functions and structs for translating the VST AEffect struct between bit boundaries.
 * Notes  : (currently none)
 * Authors: Johannes Schultz (OpenMPT Devs)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

#include <vector>
#include "../mptrack/plugins/VstDefinitions.h"

OPENMPT_NAMESPACE_BEGIN

#pragma pack(push, 8)

template<typename ptr_t>
struct AEffectProto
{
	int32_t magic;
	ptr_t dispatcher;
	ptr_t process;
	ptr_t setParameter;
	ptr_t getParameter;

	int32_t numPrograms;
	int32_t numParams;
	int32_t numInputs;
	int32_t numOutputs;

	int32_t flags;
	
	ptr_t resvd1;
	ptr_t resvd2;
	
	int32_t initialDelay;
	
	int32_t realQualities;
	int32_t offQualities;
	float ioRatio;

	ptr_t object;
	ptr_t user;

	int32_t uniqueID;
	int32_t version;

	ptr_t processReplacing;
	ptr_t processDoubleReplacing;
	char future[56];

	// Convert native representation to bridge representation.
	// Don't overwrite any values managed by the bridge wrapper or host in general.
	void FromNative(const Vst::AEffect &in)
	{
		magic = in.magic;

		numPrograms = in.numPrograms;
		numParams = in.numParams;
		numInputs = in.numInputs;
		numOutputs = in.numOutputs;

		flags = in.flags;

		initialDelay = in.initialDelay;

		realQualities = in.realQualities;
		offQualities = in.offQualities;
		ioRatio = in.ioRatio;

		uniqueID = in.uniqueID;
		version = in.version;

		if(in.processReplacing == nullptr)
		{
			flags &= ~Vst::effFlagsCanReplacing;
		}
		if(in.processDoubleReplacing == nullptr)
		{
			flags &= ~Vst::effFlagsCanDoubleReplacing;
		}
	}

};

typedef AEffectProto<int32_t> AEffect32;
typedef AEffectProto<int64_t> AEffect64;

#pragma pack(pop)


// Translate a VSTEvents struct to bridge format (placed in data vector)
static void TranslateVSTEventsToBridge(std::vector<char> &data, const Vst::VstEvents *events, int32_t targetPtrSize)
{
	data.reserve(data.size() + sizeof(int32) + sizeof(Vst::VstEvent) * events->numEvents);
	// Write number of events
	if(data.size() >= sizeof(int32))
	{
		int32 &numEvents = reinterpret_cast<int32 &>(data[0]);
		numEvents += events->numEvents;
	} else
	{
		PushToVector(data, events->numEvents);
	}

	// Write events
	for(decltype(events->numEvents) i = 0; i < events->numEvents; i++)
	{
		if(events->events[i]->type == Vst::kVstSysExType)
		{
			// This is going to be messy since the VstMidiSysexEvent event has a different size than other events on 64-bit platforms.
			// We are going to write the event using the target process pointer size.
			auto sysExEvent = *static_cast<const Vst::VstMidiSysexEvent *>(events->events[i]);
			sysExEvent.byteSize = 5 * sizeof(int32) + 3 * targetPtrSize;
			PushToVector(data, sysExEvent, 5 * sizeof(int32));	// Exclude the three pointers at the end for now
			data.resize(data.size() + 3 * targetPtrSize);		// Make space for pointer + two reserved intptr_ts
			// Embed SysEx dump as well...
			auto sysex = reinterpret_cast<const char *>(sysExEvent.sysexDump);
			data.insert(data.end(), sysex, sysex + sysExEvent.dumpBytes);
		} else if(events->events[i]->type == Vst::kVstMidiType)
		{
			// randomid by Insert Piz Here sends events of type kVstMidiType, but with a claimed size of 24 bytes instead of 32.
			Vst::VstMidiEvent event;
			std::memcpy(&event, events->events[i], sizeof(event));
			event.byteSize = sizeof(event);
			PushToVector(data, event, sizeof(event));
		} else
		{
			PushToVector(data, *events->events[i], events->events[i]->byteSize);
		}
	}
}


// Translate bridge format (void *ptr) back to VSTEvents struct (placed in data vector)
static void TranslateBridgeToVSTEvents(std::vector<char> &data, void *ptr)
{
	const int32_t numEvents = *static_cast<const int32_t *>(ptr);
	
	// First element is really a int32, but in case of 64-bit builds, the next field gets aligned anyway.
	const size_t headerSize = sizeof(intptr_t) + sizeof(intptr_t) + sizeof(Vst::VstEvent *) * numEvents;
	data.reserve(headerSize + sizeof(Vst::VstEvent) * numEvents);
	data.resize(headerSize, 0);
	if(numEvents == 0)
	{
		return;
	}

	// Copy over event data (this is required for dumb SynthEdit plugins that don't copy over the event data during effProcessEvents)
	char *offset = static_cast<char *>(ptr) + sizeof(int32_t);
	for(int32_t i = 0; i < numEvents; i++)
	{
		Vst::VstEvent *event = reinterpret_cast<Vst::VstEvent *>(offset);
		data.insert(data.end(), offset, offset + event->byteSize);
		offset += event->byteSize;

		if(event->type == Vst::kVstSysExType)
		{
			// Copy over sysex dump
			auto sysExEvent = static_cast<Vst::VstMidiSysexEvent *>(event);
			data.insert(data.end(), offset, offset + sysExEvent->dumpBytes);
			offset += sysExEvent->dumpBytes;
		}
	}

	// Write pointers
	auto events = reinterpret_cast<Vst::VstEvents *>(data.data());
	events->numEvents = numEvents;
	offset = data.data() + headerSize;
	for(int32_t i = 0; i < numEvents; i++)
	{
		events->events[i] = reinterpret_cast<Vst::VstEvent *>(offset);
		offset += events->events[i]->byteSize;
		if(events->events[i]->type == Vst::kVstSysExType)
		{
			auto sysExEvent = static_cast<Vst::VstMidiSysexEvent *>(events->events[i]);
			sysExEvent->sysexDump = reinterpret_cast<const mpt::byte *>(offset);
			offset += sysExEvent->dumpBytes;
		}
	}
}


OPENMPT_NAMESPACE_END
