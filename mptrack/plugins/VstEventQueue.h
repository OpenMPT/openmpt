/*
 * VstEventQueue.h
 * ---------------
 * Purpose: Event queue for VST events.
 * Notes  : Modelled after an idea from https://www.kvraudio.com/forum/viewtopic.php?p=3043807#p3043807
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include <deque>
#include "mpt/mutex/mutex.hpp"
#include "VstDefinitions.h"

OPENMPT_NAMESPACE_BEGIN

class VstEventQueue : public Vst::VstEvents
{
protected:

	// Although originally all event types were apparently supposed to be of the same size, this is not the case on 64-Bit systems.
	// VstMidiSysexEvent contains 3 pointers, so the struct size differs between 32-Bit and 64-Bit systems.
	union Event
	{
		Vst::VstEvent event;
		Vst::VstMidiEvent midi;
		Vst::VstMidiSysexEvent sysex;
	};

	// Here we store our events which are then inserted into the fixed-size event buffer sent to the plugin.
	std::deque<Event> eventQueue;
	// Since plugins can also add events to the queue (even from a different thread than the processing thread),
	// we need to ensure that reading and writing is never done in parallel.
	mpt::mutex criticalSection;

public:

	VstEventQueue()
	{
		numEvents = 0;
		reserved = 0;
		std::fill(std::begin(events), std::end(events), nullptr);
	}

	// Get the number of events that are currently queued, but not in the output buffer.
	size_t GetNumQueuedEvents()
	{
		return eventQueue.size() - numEvents;
	}

	// Add a VST event to the queue. Returns true on success.
	// Set insertFront to true to prioritise this event (i.e. add it at the front of the queue instead of the back)
	bool Enqueue(const Vst::VstEvent *event, bool insertFront = false)
	{
		MPT_ASSERT(event->type != Vst::kVstSysExType || event->byteSize == sizeof(Vst::VstMidiSysexEvent));
		MPT_ASSERT(event->type != Vst::kVstMidiType || event->byteSize == sizeof(Vst::VstMidiEvent));

		Event copyEvent;
		size_t copySize;
		// randomid by Insert Piz Here sends events of type kVstMidiType, but with a claimed size of 24 bytes instead of 32.
		// Hence, we enforce the size of known events.
		if(event->type == Vst::kVstSysExType)
			copySize = sizeof(Vst::VstMidiSysexEvent);
		else if(event->type == Vst::kVstMidiType)
			copySize = sizeof(Vst::VstMidiEvent);
		else
			copySize = std::min(size_t(event->byteSize), sizeof(copyEvent));
		memcpy(&copyEvent, event, copySize);

		if(event->type == Vst::kVstSysExType)
		{
			// SysEx messages need to be copied, as the space used for the dump might be freed in the meantime.
			auto &e = copyEvent.sysex;
			auto sysexDump = new (std::nothrow) std::byte[e.dumpBytes];
			if(sysexDump == nullptr)
				return false;
			memcpy(sysexDump, e.sysexDump, e.dumpBytes);
			e.sysexDump = sysexDump;
		}

		mpt::lock_guard<mpt::mutex> lock(criticalSection);
		if(insertFront)
			eventQueue.push_front(copyEvent);
		else
			eventQueue.push_back(copyEvent);
		return true;
	}

	// Set up the queue for transmitting to the plugin. Returns number of elements that are going to be transmitted.
	int32 Finalise()
	{
		mpt::lock_guard<mpt::mutex> lock(criticalSection);
		numEvents = static_cast<int32>(std::min(eventQueue.size(), MAX_EVENTS));
		for(int32 i = 0; i < numEvents; i++)
		{
			events[i] = &eventQueue[i].event;
		}
		return numEvents;
	}

	// Remove transmitted events from the queue
	void Clear()
	{
		mpt::lock_guard<mpt::mutex> lock(criticalSection);
		if(numEvents)
		{
			// Release temporarily allocated buffer for SysEx messages
			for(auto e = eventQueue.begin(); e != eventQueue.begin() + numEvents; ++e)
			{
				if(e->event.type == Vst::kVstSysExType)
				{
					delete[] e->sysex.sysexDump;
				}
			}
			eventQueue.erase(eventQueue.begin(), eventQueue.begin() + numEvents);
			numEvents = 0;
		}
	}

};

OPENMPT_NAMESPACE_END
