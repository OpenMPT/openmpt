/*
 * AEffectWrapper.h
 * ----------------
 * Purpose: Helper functions and structs for translating the VST AEffect struct between bit boundaries.
 * Notes  : (currently none)
 * Authors: Johannes Schultz (OpenMPT Devs)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include <vector>
#include "../mptrack/plugins/VstDefinitions.h"

OPENMPT_NAMESPACE_BEGIN

#pragma pack(push, 8)

template <typename ptr_t>
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
static void TranslateVstEventsToBridge(std::vector<char> &outData, const Vst::VstEvents &events, int32 targetPtrSize)
{
	outData.reserve(outData.size() + sizeof(int32) + sizeof(Vst::VstMidiEvent) * events.numEvents);
	// Write number of events
	PushToVector(outData, events.numEvents);
	// Write events
	for(const auto event : events)
	{
		if(event->type == Vst::kVstSysExType)
		{
			// This is going to be messy since the VstMidiSysexEvent event has a different size than other events on 64-bit platforms.
			// We are going to write the event using the target process pointer size.
			auto sysExEvent = *static_cast<const Vst::VstMidiSysexEvent *>(event);
			sysExEvent.byteSize = 4 * sizeof(int32) + 4 * targetPtrSize;  // It's 5 int32s and 3 pointers but that means that on 64-bit platforms, the fifth int32 is padded for alignment.
			PushToVector(outData, sysExEvent, 5 * sizeof(int32));         // Exclude the three pointers at the end for now
			if(targetPtrSize > sizeof(int32))                             // Padding for 64-bit required?
				outData.insert(outData.end(), targetPtrSize - sizeof(int32), 0);
			outData.insert(outData.end(), 3 * targetPtrSize, 0);  // Make space for pointer + two reserved intptr_ts
			// Embed SysEx dump as well...
			auto sysex = reinterpret_cast<const char *>(sysExEvent.sysexDump);
			outData.insert(outData.end(), sysex, sysex + sysExEvent.dumpBytes);
		} else if(event->type == Vst::kVstMidiType)
		{
			// randomid by Insert Piz Here sends events of type kVstMidiType, but with a claimed size of 24 bytes instead of 32.
			Vst::VstMidiEvent midiEvent;
			std::memcpy(&midiEvent, event, sizeof(midiEvent));
			midiEvent.byteSize = sizeof(midiEvent);
			PushToVector(outData, midiEvent, sizeof(midiEvent));
		} else
		{
			PushToVector(outData, *event, event->byteSize);
		}
	}
}


// Translate bridge format (void *ptr) back to VSTEvents struct (placed in data vector)
static void TranslateBridgeToVstEvents(std::vector<char> &outData, const void *inData)
{
	const int32 numEvents = *static_cast<const int32 *>(inData);

	// First element is really a int32, but in case of 64-bit builds, the next field gets aligned anyway.
	const size_t headerSize = sizeof(intptr_t) + sizeof(intptr_t) + sizeof(Vst::VstEvent *) * numEvents;
	outData.reserve(headerSize + sizeof(Vst::VstMidiEvent) * numEvents);
	outData.resize(headerSize, 0);
	if(numEvents == 0)
		return;

	// Copy over event data (this is required for dumb SynthEdit plugins that don't copy over the event data during effProcessEvents)
	const char *readOffset = static_cast<const char *>(inData) + sizeof(int32);
	for(int32 i = 0; i < numEvents; i++)
	{
		auto *event = reinterpret_cast<const Vst::VstEvent *>(readOffset);
		outData.insert(outData.end(), readOffset, readOffset + event->byteSize);
		readOffset += event->byteSize;

		if(event->type == Vst::kVstSysExType)
		{
			// Copy over sysex dump
			auto *sysExEvent = static_cast<const Vst::VstMidiSysexEvent *>(event);
			outData.insert(outData.end(), readOffset, readOffset + sysExEvent->dumpBytes);
			readOffset += sysExEvent->dumpBytes;
		}
	}

	// Write pointers
	auto events = reinterpret_cast<Vst::VstEvents *>(outData.data());
	events->numEvents = numEvents;
	char *offset = outData.data() + headerSize;
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
static size_t BridgeVstEventsSize(const void *ptr)
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


static void TranslateVstFileSelectToBridge(std::vector<char> &outData, const Vst::VstFileSelect &fileSelect, int32 targetPtrSize)
{
	outData.reserve(outData.size() + sizeof(Vst::VstFileSelect) + fileSelect.numFileTypes * sizeof(Vst::VstFileType));
	PushToVector(outData, fileSelect.command);
	PushToVector(outData, fileSelect.type);
	PushToVector(outData, fileSelect.macCreator);
	PushToVector(outData, fileSelect.numFileTypes);
	outData.insert(outData.end(), targetPtrSize, 0);  // fileTypes
	PushToVector(outData, fileSelect.title);
	outData.insert(outData.end(), 2 * targetPtrSize, 0);  // initialPath, returnPath
	PushToVector(outData, fileSelect.sizeReturnPath);
	if(targetPtrSize > sizeof(int32))
		outData.insert(outData.end(), targetPtrSize - sizeof(int32), 0);  // padding
	outData.insert(outData.end(), targetPtrSize, 0);                      // returnMultiplePaths
	PushToVector(outData, fileSelect.numReturnPaths);
	outData.insert(outData.end(), targetPtrSize, 0);  // reserved
	PushToVector(outData, fileSelect.reserved2);

	if(fileSelect.command != Vst::kVstDirectorySelect)
	{
		for(int32 i = 0; i < fileSelect.numFileTypes; i++)
		{
			PushToVector(outData, fileSelect.fileTypes[i]);
		}
	}

	if(fileSelect.command == Vst::kVstMultipleFilesLoad)
	{
		outData.insert(outData.end(), fileSelect.numReturnPaths * targetPtrSize, 0);
		for(int32 i = 0; i < fileSelect.numReturnPaths; i++)
		{
			PushZStringToVector(outData, fileSelect.returnMultiplePaths[i]);
		}
	}

	PushZStringToVector(outData, fileSelect.initialPath);
	PushZStringToVector(outData, fileSelect.returnPath);
}


static void TranslateBridgeToVstFileSelect(std::vector<char> &outData, const void *inData, size_t srcSize)
{
	outData.assign(static_cast<const char *>(inData), static_cast<const char *>(inData) + srcSize);

	// Fixup pointers
	Vst::VstFileSelect &fileSelect = *reinterpret_cast<Vst::VstFileSelect *>(outData.data());
	auto ptrOffset = outData.data() + sizeof(Vst::VstFileSelect);

	if(fileSelect.command != Vst::kVstDirectorySelect)
	{
		fileSelect.fileTypes = reinterpret_cast<Vst::VstFileType *>(ptrOffset);
		ptrOffset += fileSelect.numFileTypes * sizeof(Vst::VstFileType);
	} else
	{
		fileSelect.fileTypes = nullptr;
	}

	if(fileSelect.command == Vst::kVstMultipleFilesLoad)
	{
		fileSelect.returnMultiplePaths = reinterpret_cast<char **>(ptrOffset);
		ptrOffset += fileSelect.numReturnPaths * sizeof(char *);

		for(int32 i = 0; i < fileSelect.numReturnPaths; i++)
		{
			fileSelect.returnMultiplePaths[i] = ptrOffset;
			ptrOffset += strlen(fileSelect.returnMultiplePaths[i]) + 1;
		}
	} else
	{
		fileSelect.returnMultiplePaths = nullptr;
	}

	fileSelect.initialPath = ptrOffset;
	ptrOffset += strlen(fileSelect.initialPath) + 1;
	fileSelect.returnPath = ptrOffset;
	fileSelect.sizeReturnPath = static_cast<int32>(srcSize - std::distance(outData.data(), ptrOffset));
	ptrOffset += strlen(fileSelect.returnPath) + 1;
}


OPENMPT_NAMESPACE_END
