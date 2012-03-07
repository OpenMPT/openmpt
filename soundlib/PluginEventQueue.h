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
struct PluginEventQueue
{
	typedef VstMidiEvent BiggestVstEvent;
	static_assert(sizeof(BiggestVstEvent) >= sizeof(VstEvent)
		&& sizeof(BiggestVstEvent) >= sizeof(VstMidiEvent)
		&& sizeof(BiggestVstEvent) >= sizeof(VstMidiSysexEvent),
		"Check typedef above, BiggestVstEvent must be the biggest VstEvent struct.");

	VstInt32 numEvents;		///< number of Events in array
	VstIntPtr reserved;		///< zero (Reserved for future use)
	VstEvent* events[N];	///< event pointer array
	std::deque<BiggestVstEvent> eventQueue;	// Here we store our events.

	PluginEventQueue()
	{
		numEvents = 0;
		reserved = nullptr;
		MemsetZero(events);
	}

	// Add a VST event to the queue. Returns true on success.
	bool Enqueue(const VstEvent &event, bool insertFront = false)
	{
		VstMidiEvent midiEvent;
		memcpy(&midiEvent, &event, sizeof(event));
		ASSERT(event.byteSize == sizeof(event));
		return Enqueue(midiEvent, insertFront);
	}
	bool Enqueue(const VstMidiEvent &event, bool insertFront = false)
	{
		static_assert(sizeof(BiggestVstEvent) <= sizeof(VstMidiEvent), "Also check implementation here.");
		if(insertFront)
		{
			eventQueue.push_front(event);
		} else
		{
			eventQueue.push_back(event);
		}
		return true;
	}
	bool Enqueue(const VstMidiSysexEvent &event, bool insertFront = false)
	{
		VstMidiEvent midiEvent;
		memcpy(&midiEvent, &event, sizeof(event));
		ASSERT(event.byteSize == sizeof(event));
		return Enqueue(midiEvent, insertFront);
	}

	// Set up the queue for transmitting to the plugin. Returns number of elements that are going to be transmitted.
	VstInt32 Finalise()
	{
		numEvents = min(eventQueue.size(), N);
		for(VstInt32 i = 0; i < numEvents; i++)
		{
			events[i] = reinterpret_cast<VstEvent *>(&eventQueue[i]);
		}
		return numEvents;
	}

	// Remove transmitted events from the queue
	void Clear()
	{
		if(numEvents)
		{
			eventQueue.erase(eventQueue.begin(), eventQueue.begin() + numEvents);
			numEvents = 0;
		}
	}

};

#if TARGET_API_MAC_CARBON
	#pragma options align=reset
#elif defined(WIN32) || defined(__FLAT__) || defined(__GNUC__)
	#pragma pack(pop)
#elif defined __BORLANDC__
	#pragma -a-
#endif
