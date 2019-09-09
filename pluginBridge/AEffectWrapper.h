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
	int32 magic;
	ptr_t dispatcher;
	ptr_t process;
	ptr_t setParameter;
	ptr_t getParameter;

	int32 numPrograms;
	int32 numParams;
	int32 numInputs;
	int32 numOutputs;

	int32 flags;
	
	ptr_t resvd1;
	ptr_t resvd2;
	
	int32 initialDelay;
	
	int32 realQualities;
	int32 offQualities;
	float ioRatio;

	ptr_t object;
	ptr_t user;

	int32 uniqueID;
	int32 version;

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
			flags &= ~Vst::effFlagsCanReplacing;
		if(in.processDoubleReplacing == nullptr)
			flags &= ~Vst::effFlagsCanDoubleReplacing;
	}

};

using AEffect32 = AEffectProto<int32>;
using AEffect64 = AEffectProto<int64>;

#pragma pack(pop)


// Translate a VSTEvents struct to bridge format (placed in data vector)
static void TranslateVSTEventsToBridge(std::vector<char> &data, const Vst::VstEvents *events, int32 targetPtrSize)
{
	data.reserve(data.size() + sizeof(int32) + sizeof(Vst::VstMidiEvent) * events->numEvents);
	// Write number of events
	PushToVector(data, events->numEvents);
	// Write events
	for(const auto event : *events)
	{
		if(event->type == Vst::kVstSysExType)
		{
			// This is going to be messy since the VstMidiSysexEvent event has a different size than other events on 64-bit platforms.
			// We are going to write the event using the target process pointer size.
			auto sysExEvent = *static_cast<const Vst::VstMidiSysexEvent *>(event);
			sysExEvent.byteSize = 4 * sizeof(int32) + 4 * targetPtrSize;  // It's 5 int32s and 3 pointers but that means that on 64-bit platforms, the fifth int32 is padded for alignment.
			PushToVector(data, sysExEvent, 5 * sizeof(int32));            // Exclude the three pointers at the end for now
			if(targetPtrSize > sizeof(int32))                             // Padding for 64-bit required?
				data.insert(data.end(), targetPtrSize - sizeof(int32), 0);
			data.insert(data.end(), 3 * targetPtrSize, 0);  // Make space for pointer + two reserved intptr_ts
			// Embed SysEx dump as well...
			auto sysex = reinterpret_cast<const char *>(sysExEvent.sysexDump);
			data.insert(data.end(), sysex, sysex + sysExEvent.dumpBytes);
		} else if(event->type == Vst::kVstMidiType)
		{
			// randomid by Insert Piz Here sends events of type kVstMidiType, but with a claimed size of 24 bytes instead of 32.
			Vst::VstMidiEvent midiEvent;
			std::memcpy(&midiEvent, event, sizeof(midiEvent));
			midiEvent.byteSize = sizeof(midiEvent);
			PushToVector(data, midiEvent, sizeof(midiEvent));
		} else
		{
			PushToVector(data, *event, event->byteSize);
		}
	}
}


// Translate bridge format (void *ptr) back to VSTEvents struct (placed in data vector)
static void TranslateBridgeToVSTEvents(std::vector<char> &data, void *ptr)
{
	const int32 numEvents = *static_cast<const int32 *>(ptr);
	
	// First element is really a int32, but in case of 64-bit builds, the next field gets aligned anyway.
	const size_t headerSize = sizeof(intptr_t) + sizeof(intptr_t) + sizeof(Vst::VstEvent *) * numEvents;
	data.reserve(headerSize + sizeof(Vst::VstEvent) * numEvents);
	data.resize(headerSize, 0);
	if(numEvents == 0)
		return;

	// Copy over event data (this is required for dumb SynthEdit plugins that don't copy over the event data during effProcessEvents)
	char *offset = static_cast<char *>(ptr) + sizeof(int32);
	for(int32 i = 0; i < numEvents; i++)
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
	for(int32 i = 0; i < numEvents; i++)
	{
		events->events[i] = reinterpret_cast<Vst::VstEvent *>(offset);
		offset += events->events[i]->byteSize;
		if(events->events[i]->type == Vst::kVstSysExType)
		{
			auto sysExEvent = static_cast<Vst::VstMidiSysexEvent *>(events->events[i]);
			sysExEvent->sysexDump = reinterpret_cast<const std::byte *>(offset);
			offset += sysExEvent->dumpBytes;
		}
	}
}


// Calculate the size total of the VSTEvents (without header) in bridge format
static size_t BridgeVSTEventsSize(const void *ptr)
{
	const int32 numEvents = *static_cast<const int32 *>(ptr);
	size_t size = 0;
	for(int32 i = 0; i < numEvents; i++)
	{
		const auto event = reinterpret_cast<const Vst::VstEvent *>(static_cast<const char *>(ptr) + sizeof(int32) + size);
		size += event->byteSize;
		if(event->type == Vst::kVstSysExType)
		{
			size += static_cast<const Vst::VstMidiSysexEvent *>(event)->dumpBytes;
		}
	}
	return size;
}


OPENMPT_NAMESPACE_END
