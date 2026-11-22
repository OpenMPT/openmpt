/*
 * MidiDeviceManager.h
 * -------------------
 * Purpose: MIDI device management, port sharing, and MIDI output thread handling
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"
#include "mpt/mutex/mutex.hpp"

#include <array>
#include <map>
#include <memory>
#include <string>
#include <vector>


OPENMPT_NAMESPACE_BEGIN


using MidiPortID = unsigned int;
constexpr MidiPortID kNoMidiDevice = MidiPortID(-1);

class RtMidi;
class IMidiInputCallback;
class MidiInputHandle;
class MidiOutputHandle;


// MIDI queue entry with small storage optimiziation.
// This optimiziation is going to be used for all messages that OpenMPT can send internally,
// but SysEx messages received from plugins or via MIDI in may be longer.
class MidiMessage
{
public:
	int64 m_streamFrame = 0;
	size_t m_size = 0;
	const std::byte *m_message = nullptr;
	void *m_sender = nullptr;  // For feedback prevention
protected:
	std::array<std::byte, 32> m_msgSmall;

public:
	MidiMessage(int64 streamFrame, const mpt::const_byte_span data, void *sender = nullptr)
		: m_streamFrame{streamFrame}
		, m_size{data.size()}
		, m_sender{sender}
	{
		std::byte *message = m_msgSmall.data();
		if(m_size > m_msgSmall.size())
			message = new std::byte[m_size];
		std::memcpy(message, data.data(), m_size);
		m_message = message;
	}

	MidiMessage(const MidiMessage &) = delete;
	MidiMessage &operator=(const MidiMessage &) = delete;

	operator mpt::const_byte_span() const { return mpt::as_span(m_message, m_size); }

	MidiMessage(MidiMessage &&other) noexcept
		: m_streamFrame{other.m_streamFrame}
		, m_size{other.m_size}
		, m_message{other.m_message}
		, m_sender{other.m_sender}
		, m_msgSmall{other.m_msgSmall}
	{
		other.m_message = nullptr;
		if(m_size <= m_msgSmall.size())
			m_message = m_msgSmall.data();
	}

	~MidiMessage()
	{
		if(m_size > m_msgSmall.size())
			delete[] m_message;
	}

	MidiMessage &operator=(MidiMessage &&other) noexcept
	{
		m_streamFrame = other.m_streamFrame;
		m_size = other.m_size;
		m_sender = other.m_sender;
		m_message = (m_size <= m_msgSmall.size()) ? m_msgSmall.data() : other.m_message;
		m_msgSmall = other.m_msgSmall;
		other.m_message = nullptr;
		return *this;
	}

	bool operator<(const MidiMessage &rhs) const noexcept
	{
		return m_streamFrame < rhs.m_streamFrame;
	}
};


struct MidiPortInfo
{
	std::string name;  // UTF-8, raw device name from RtMidi
	mpt::ustring friendlyName;
	MidiPortID id = kNoMidiDevice;
};


class MidiDeviceManager
{
public:
	static MidiDeviceManager &Instance();

	struct SyncPoint
	{
		int64 streamFrames = 0;
		uint64 systemTimestampNs = 0;
		double speed = 1.0;
		uint32 sampleRate = 0;
	};

	// Enumerate available MIDI ports.
	static std::vector<MidiPortInfo> EnumerateInputPorts();
	static std::vector<MidiPortInfo> EnumerateOutputPorts();

	// Find a port by name/friendlyName, returning best-match ID.
	// Returns kNoMidiDevice if no match is found.
	static MidiPortID FindInputPort(const MidiPortInfo &port);
	static MidiPortID FindOutputPort(const MidiPortInfo &port);
	static MidiPortID FindPort(const MidiPortInfo &port, bool asInputDevice);
	
	MidiPortInfo GetPortInfo(const MidiInputHandle &handle) const;
	MidiPortInfo GetPortInfo(const MidiOutputHandle &handle) const;

	// Open an input or output port. The same port can be opened multiple times.
	// The port is closed by resetting the returned handle.
	// Returns nullptr on failure.
	std::unique_ptr<MidiInputHandle> OpenInput(MidiPortID port, IMidiInputCallback *callback);
	std::unique_ptr<MidiOutputHandle> OpenOutput(MidiPortID port);

	// Updates the current audio thread sync point to schedule MIDI output message timing against
	void UpdateSyncPoint(int64 SyncPointStreamFrames, uint64 SyncPointSystemTimestamp, double speed, uint32 sampleRate);
	void ResetSyncPoint() { m_syncPoint = SyncPoint{}; }

	// Check if a MIDI device has been disconnected
	static bool CheckDeviceStatus(const MidiInputHandle &handle);

private:
	friend class MidiInputHandle;
	friend class MidiOutputHandle;

	struct SharedInputPort;
	struct SharedOutputPort;

	MidiDeviceManager() = default;
	~MidiDeviceManager();

	// Internal helpers
	static std::vector<MidiPortInfo> EnumeratePorts(RtMidi &rtMidi, bool isInput);
	static MidiPortID FindPort(RtMidi &rtMidi, const MidiPortInfo &port, bool isInput);

	void CloseInput(MidiInputHandle &handle);
	void CloseOutput(MidiOutputHandle &handle);

	static void InputCallback(double deltatime, std::vector<unsigned char> *message, void *userData);
	void OutputThreadFunc(SharedOutputPort &port);

	mpt::mutex m_managerMutex;
	std::map<MidiPortID, SharedInputPort> m_openInputs;
	std::map<MidiPortID, SharedOutputPort> m_openOutputs;

	// TODO make this non-locking?
	std::atomic<SyncPoint> m_syncPoint;
};


// Handle to an open MIDI input port. Multiple handles can share the same physical port.
// Releasing the last handle for a port closes the physical device.
class MidiInputHandle final
{
	friend class MidiDeviceManager;

	MidiInputHandle(MidiDeviceManager::SharedInputPort &port, IMidiInputCallback &callback)
		: m_port{port}
		, m_callback{callback}
	{
	}

public:
	~MidiInputHandle();

private:
	MidiDeviceManager::SharedInputPort &m_port;
	IMidiInputCallback &m_callback;
};


// Handle to an open MIDI output port. Multiple handles can share the same physical port.
// Releasing the last handle for a port drains its messages and closes the device.
class MidiOutputHandle final
{
	friend class MidiDeviceManager;

	MidiOutputHandle(MidiDeviceManager::SharedOutputPort&port)
		: m_port{port}
	{
	}

public:
	~MidiOutputHandle();

	void Send(mpt::const_byte_span data, int64 streamFrame);

private:
	MidiDeviceManager::SharedOutputPort &m_port;
};


OPENMPT_NAMESPACE_END
