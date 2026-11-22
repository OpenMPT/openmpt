/*
 * MidiInputCallback.h
 * -------------------
 * Purpose: Callback interface class for MIDI recording
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"


OPENMPT_NAMESPACE_BEGIN

// Callback interface to notify that a MIDI input callback has been destroyed
class IMidiInputCallbackDestroyedNotifySink
{
public:
	virtual ~IMidiInputCallbackDestroyedNotifySink() = default;
	virtual void OnMidiInputCallbackDestroyed(void *) = 0;
};


// Callback interface for receiving MIDI input messages
class IMidiInputCallback
{
public:
	virtual ~IMidiInputCallback()
	{
		if(m_notifySink)
			m_notifySink->OnMidiInputCallbackDestroyed(this);
	}

	virtual bool PreFilterMidiMessage(mpt::const_byte_span /*midiData*/) { return false; }
	virtual void OnMidiMessage(mpt::const_byte_span midiData) = 0;

	void SetDestroyNotifySink(IMidiInputCallbackDestroyedNotifySink *notifySink)
	{
		m_notifySink = notifySink;
	}

private:
	IMidiInputCallbackDestroyedNotifySink *m_notifySink = nullptr;
};

OPENMPT_NAMESPACE_END
