/*
 * PluginEventQueue.h
 * ------------------
 * Purpose: Alternative, easy to use implementation of the VST event queue mechanism.
 * Notes  : Modelled after an idea from http://www.kvraudio.com/forum/viewtopic.php?p=3043807#3043807
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

// Copied packing options from affectx.h
#if TARGET_API_MAC_CARBON
	#ifdef __LP64__
	#pragma options align=power
	#else
	#pragma options align=mac68k
	#endif
#elif defined __BORLANDC__
	#pragma -a8
#elif defined(__GNUC__)
	#pragma pack(push,8)
#elif defined(WIN32) || defined(__FLAT__)
	#pragma pack(push)
	#pragma pack(8)
#endif

#include <deque>

// Alternative, easy to use implementation of VstEvents struct.
template <size_t N>
//====================
class PluginEventQueue
//====================
{
protected:

	// Although originally all event types were apparently supposed to be of the same size, this is not the case on 64-Bit systems.
	// VstMidiSysexEvent contains 3 pointers, so the struct size differs between 32-Bit and 64-Bit systems.
	typedef VstMidiSysexEvent BiggestVstEvent;
	static_assert(sizeof(BiggestVstEvent) >= sizeof(VstEvent)
		&& sizeof(BiggestVstEvent) >= sizeof(VstMidiEvent)
		&& sizeof(BiggestVstEvent) >= sizeof(VstMidiSysexEvent),
		"Check typedef above, BiggestVstEvent must be the biggest VstEvent struct.");

	// The first three member variables of this class mustn't be changed, to be compatible with the original VSTEvents struct.
	VstInt32 numEvents;		///< number of Events in array
	VstIntPtr reserved;		///< zero (Reserved for future use)
	VstEvent *events[N];	///< event pointer array
	// Here we store our events.
	std::deque<BiggestVstEvent> eventQueue;
	// Since plugins can also add events to the queue (even from a different thread than the processing thread),
	// we need to ensure that reading and writing is never done in parallel.
	CRITICAL_SECTION criticalSection;

public:

	PluginEventQueue()
	{
		numEvents = 0;
		reserved = 0;
		MemsetZero(events);
		MemsetZero(criticalSection);
		InitializeCriticalSection(&criticalSection);
	}

	~PluginEventQueue()
	{
		DeleteCriticalSection(&criticalSection);
	}

	// Get the number of events that are currently in the output buffer.
	// It is possible that there are more events left in the queue if the output buffer is full.
	size_t GetNumEvents()
	{
		return numEvents;
	}

	// Get the number of events that are currently queued, but not in the output buffer.
	size_t GetNumQueuedEvents()
	{
		return eventQueue.size() - numEvents;
	}

	// Add a VST event to the queue. Returns true on success.
	// Set insertFront to true to prioritise this event (i.e. add it at the front of the queue instead of the back)
	bool Enqueue(const VstEvent *event, bool insertFront = false)
	{
		ASSERT(event->type != kVstSysExType || event->byteSize == sizeof(VstMidiSysexEvent));
		ASSERT(event->type != kVstMidiType || event->byteSize == sizeof(VstMidiEvent));

		BiggestVstEvent copyEvent;
		memcpy(&copyEvent, event, Util::Min(size_t(event->byteSize), sizeof(copyEvent)));

		EnterCriticalSection(&criticalSection);
		if(insertFront)
		{
			eventQueue.push_front(copyEvent);
		} else
		{
			eventQueue.push_back(copyEvent);
		}
		LeaveCriticalSection(&criticalSection);
		return true;
	}

	// Set up the queue for transmitting to the plugin. Returns number of elements that are going to be transmitted.
	VstInt32 Finalise()
	{
		EnterCriticalSection(&criticalSection);
		numEvents = min(eventQueue.size(), N);
		for(VstInt32 i = 0; i < numEvents; i++)
		{
			events[i] = reinterpret_cast<VstEvent *>(&eventQueue[i]);
		}
		LeaveCriticalSection(&criticalSection);
		return numEvents;
	}

	// Remove transmitted events from the queue
	void Clear()
	{
		EnterCriticalSection(&criticalSection);
		if(numEvents)
		{
			eventQueue.erase(eventQueue.begin(), eventQueue.begin() + numEvents);
			numEvents = 0;
		}
		LeaveCriticalSection(&criticalSection);
	}

};

#if TARGET_API_MAC_CARBON
	#pragma options align=reset
#elif defined(WIN32) || defined(__FLAT__) || defined(__GNUC__)
	#pragma pack(pop)
#elif defined __BORLANDC__
	#pragma -a-
#endif
