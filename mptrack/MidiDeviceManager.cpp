/*
 * MidiDeviceManager.cpp
 * ---------------------
 * Purpose: MIDI device management, port sharing, and MIDI output thread handling
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "MidiDeviceManager.h"
#include "MidiInputCallback.h"
#include "Mptrack.h"
#include "TrackerSettings.h"
#include "../misc/mptClock.h"
#include "../soundlib/MIDIEvents.h"
#include "../src/openmpt/sounddevice/SoundDeviceManager.hpp"
#include "../src/openmpt/sounddevice/SoundDeviceUtilities.hpp"

#include <rtmidi/RtMidi.h>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <thread>


OPENMPT_NAMESPACE_BEGIN


static_assert(std::is_same_v<MidiPortID, decltype(RtMidiIn().getPortCount())>);


struct MidiDeviceManager::SharedInputPort
{
	RtMidiIn rtMidiIn;
	mpt::mutex callbackMutex;  // Guards callbacks vector
	std::vector<IMidiInputCallback *> callbacks;
	std::vector<std::byte> sysexBuffer;

	bool HasReferences() const noexcept { return !callbacks.empty(); }
};


struct MidiDeviceManager::SharedOutputPort
{
	RtMidiOut rtMidiOut;
	std::thread outputThread;
	mpt::mutex mutex;  // Guards queue vector
	std::vector<MidiMessage> queue;
	std::condition_variable_any cv;
	std::atomic<bool> running = false;
	int refCount = 0;

	bool HasReferences() const noexcept { return refCount > 0; }
};


MidiDeviceManager &MidiDeviceManager::Instance()
{
	static MidiDeviceManager instance;
	return instance;
}


MidiDeviceManager::~MidiDeviceManager()
{
	MPT_ASSERT(m_openInputs.empty());
	MPT_ASSERT(m_openOutputs.empty());
}


std::vector<MidiPortInfo> MidiDeviceManager::EnumeratePorts(RtMidi &rtMidi, bool isInput)
{
	std::vector<MidiPortInfo> ports;
	try
	{
		const MidiPortID count = rtMidi.getPortCount();
		ports.reserve(count);
		for(MidiPortID i = 0; i < count; i++)
		{
			try
			{
				std::string name = rtMidi.getPortName(i);
				mpt::ustring friendlyName = theApp.GetFriendlyMIDIPortName(mpt::ToUnicode(mpt::Charset::UTF8, name), isInput, false);
				ports.push_back({std::move(name), std::move(friendlyName), i});
			} catch(const RtMidiError &)
			{
			}
		}
	} catch(const RtMidiError &)
	{
	}
	return ports;
}


std::vector<MidiPortInfo> MidiDeviceManager::EnumerateInputPorts()
{
	RtMidiIn rtMidi;
	return EnumeratePorts(rtMidi, true);
}


std::vector<MidiPortInfo> MidiDeviceManager::EnumerateOutputPorts()
{
	RtMidiOut rtMidi;
	return EnumeratePorts(rtMidi, false);
}


MidiPortID MidiDeviceManager::FindPort(RtMidi &rtMidi, const MidiPortInfo &port, bool isInput)
{
	if(port.id == kNoMidiDevice)
		return port.id;

	MidiPortID candidate = port.id;
	bool foundFriendly = false;
	const MidiPortID numPorts = rtMidi.getPortCount();
	for(MidiPortID i = 0; i < numPorts; i++)
	{
		try
		{
			std::string portName = rtMidi.getPortName(i);
			bool deviceNameMatches = (portName == port.name);
			if(!port.friendlyName.empty() && port.friendlyName == theApp.GetFriendlyMIDIPortName(mpt::ToUnicode(mpt::Charset::UTF8, portName), isInput, false))
			{
				candidate = i;
				foundFriendly = true;
				if(deviceNameMatches)
					return candidate;
			}
			if(deviceNameMatches && !foundFriendly)
				candidate = i;
		} catch(const RtMidiError &)
		{
		}
	}
	return candidate;
}


MidiPortID MidiDeviceManager::FindInputPort(const MidiPortInfo &port)
{
	try
	{
		RtMidiIn rtMidi;
		return FindPort(rtMidi, port, true);
	} catch(const RtMidiError &)
	{
		return kNoMidiDevice;
	}
}


MidiPortID MidiDeviceManager::FindOutputPort(const MidiPortInfo &port)
{
	try
	{
		RtMidiOut rtMidi;
		return FindPort(rtMidi, port, false);
	} catch(const RtMidiError &)
	{
		return kNoMidiDevice;
	}
}


MidiPortID MidiDeviceManager::FindPort(const MidiPortInfo &port, bool asInputDevice)
{
	if(asInputDevice)
		return FindInputPort(port);
	else
		return FindOutputPort(port);
}


MidiPortInfo MidiDeviceManager::GetPortInfo(const MidiInputHandle &handle) const
{
	SharedInputPort &port = handle.m_port;
	auto it = std::find_if(m_openInputs.begin(), m_openInputs.end(), [&port](const auto &kv) { return &kv.second == &port; });
	if(it == m_openInputs.end())
		return {};

	std::string name = port.rtMidiIn.getPortName(it->first);
	mpt::ustring friendlyName = theApp.GetFriendlyMIDIPortName(mpt::ToUnicode(mpt::Charset::UTF8, name), false, false);
	return {std::move(name), std::move(friendlyName), it->first};
}


MidiPortInfo MidiDeviceManager::GetPortInfo(const MidiOutputHandle &handle) const
{
	SharedOutputPort &port = handle.m_port;
	auto it = std::find_if(m_openOutputs.begin(), m_openOutputs.end(), [&port](const auto &kv) { return &kv.second == &port; });
	if(it == m_openOutputs.end())
		return {};

	std::string name = port.rtMidiOut.getPortName(it->first);
	mpt::ustring friendlyName = theApp.GetFriendlyMIDIPortName(mpt::ToUnicode(mpt::Charset::UTF8, name), true, false);
	return {std::move(name), std::move(friendlyName), it->first};
}



// RtMidi input callback that dispatches to all registered listeners
void MidiDeviceManager::InputCallback(double /*deltatime*/, std::vector<unsigned char> *message, void *userData)
{
	if(!message || message->empty() || !userData)
		return;

	auto &port = *static_cast<SharedInputPort *>(userData);
	const auto span = mpt::byte_cast<mpt::const_byte_span>(mpt::as_span(*message));

	std::vector<IMidiInputCallback *> callbacks;
	{
		// Make a copy for thread-safety, in case the user opens this input device in another MIDI I/O plugin instance at the same time
		std::lock_guard<mpt::mutex> lock{port.callbackMutex};
		callbacks = port.callbacks;
	}

	if(!port.sysexBuffer.empty() && !(message->front() & 0x80))
	{
		// Continued SysEx message
		port.sysexBuffer.insert(port.sysexBuffer.end(), span.begin(), span.end());
		if(message->back() == MIDIEvents::sysExEnd)
		{
			for(auto *callback : callbacks)
				callback->OnMidiMessage(port.sysexBuffer);
			port.sysexBuffer.clear();
		}
		return;
	} else if(message->front() == MIDIEvents::sysExStart && message->back() != MIDIEvents::sysExEnd)
	{
		// Start of SysEx but not complete yet (PortAudio backends are inconsistent in whether they buffer incomplete messages or not)
		port.sysexBuffer.assign(span.begin(), span.end());
		return;
	}

	for(auto *callback : callbacks)
		callback->OnMidiMessage(span);
}


std::unique_ptr<MidiInputHandle> MidiDeviceManager::OpenInput(MidiPortID port, IMidiInputCallback *callback)
{
	if(port == kNoMidiDevice || !callback)
		return nullptr;

	mpt::lock_guard<mpt::mutex> lock{m_managerMutex};

	auto &shared = m_openInputs[port];
	if(shared.rtMidiIn.isPortOpen())
	{
		// Port already open - add this callback as a listener
		mpt::lock_guard<mpt::mutex> callbackLock{shared.callbackMutex};
		shared.callbacks.push_back(callback);
	} else
	{
		// Need to open the port
		try
		{
			shared.sysexBuffer.clear();
			shared.rtMidiIn.openPort(port);
			shared.rtMidiIn.ignoreTypes(false, true, true);  // Accept SysEx, ignore timing / active sensing
			shared.callbacks.push_back(callback);
			shared.rtMidiIn.setCallback(InputCallback, &shared);
		} catch(const RtMidiError &)
		{
			m_openInputs.erase(port);
			return nullptr;
		}
	}

	return std::unique_ptr<MidiInputHandle>(new MidiInputHandle{shared, *callback});
}


void MidiDeviceManager::CloseInput(MidiInputHandle &handle)
{
	mpt::lock_guard<mpt::mutex> lock{m_managerMutex};
	SharedInputPort &port = handle.m_port;

	// Remove this specific callback
	auto &cbs = port.callbacks;
	cbs.erase(std::remove(cbs.begin(), cbs.end(), &handle.m_callback), cbs.end());

	if(port.HasReferences())
		return;

	// This was the last listener, close the port
	try
	{
		port.rtMidiIn.closePort();
	} catch(const RtMidiError &)
	{
	}
	auto it = std::find_if(m_openInputs.begin(), m_openInputs.end(), [&port](const auto &kv) { return &kv.second == &port; });
	if(it != m_openInputs.end())
		m_openInputs.erase(it);
}


MidiInputHandle::~MidiInputHandle()
{
	MidiDeviceManager::Instance().CloseInput(*this);
}


void MidiDeviceManager::OutputThreadFunc(SharedOutputPort &port)
{
	const auto *sdm = theApp.GetSoundDevicesManager();
	const auto sysInfo = sdm->GetSysInfo();
	const auto appInfo = sdm->GetAppInfo();
	SoundDevice::CPriorityBooster priorityBooster{sysInfo, true, mpt::ToWin(appInfo.BoostedThreadMMCSSClassVista), appInfo.BoostedThreadPriorityXP};
	Util::MultimediaClock clock;
	clock.SetResolution(1);

	static constexpr double MIN_WAIT_TIME_MS = 1.0;

	const auto WaitTimeMs = [this, &clock](int64 eventStreamFrame)
	{
		const SyncPoint sp = m_syncPoint;
		// Sync point not valid yet? Wait some more.
		if(sp.sampleRate == 0 || sp.speed <= 0.0)
			return MIN_WAIT_TIME_MS;
		
		const double srFactor = sp.speed * sp.sampleRate;
		int64 actualCurrentFrame = mpt::saturate_round<int64>(
			sp.streamFrames
			+ (static_cast<double>(static_cast<int64>(clock.NowNanoseconds() - sp.systemTimestampNs)) * srFactor * (1.0 / 1'000'000'000.0)));

		double frameDelta = static_cast<double>(eventStreamFrame - actualCurrentFrame);
		double waitMs = 1000.0 * frameDelta / srFactor;
		return std::min(waitMs, 10.0);  // In case of large buffer, only wait a smaller amount of time to decrease chance of overshooting
	};

	std::vector<MidiMessage> localQueue;

	while(true)
	{
		{
			std::unique_lock<mpt::mutex> lock{port.mutex};
			port.cv.wait(lock, [&]() { return !port.queue.empty() || !port.running; });
			if(!port.running && port.queue.empty())
				break;
			localQueue.assign(std::make_move_iterator(port.queue.begin()), std::make_move_iterator(port.queue.end()));
			port.queue.clear();
			// If two I/O plugin instances with different latency settings use the same output device, the timestamps are not going to be ordered correctly
			std::stable_sort(localQueue.begin(), localQueue.end());
		}

		for(size_t i = 0; i < localQueue.size(); i++)
		{
			double waitMs = WaitTimeMs(localQueue[i].m_streamFrame);

			// Wait until it's time to send this message (unless we're shutting down)
			while(waitMs > 0.0 && port.running)
			{
				if(waitMs >= MIN_WAIT_TIME_MS)
				{
					// Sleep until ~1ms before target, then spin-wait for more precise timing
					std::unique_lock<mpt::mutex> lock{port.mutex};
					port.cv.wait_for(lock, std::chrono::nanoseconds{static_cast<int64>((waitMs - 1.0) * 1'000'000.0)});
					if(!port.queue.empty())
					{
						// Move any newly queued messages into our local queue
						localQueue.insert(localQueue.end(), std::make_move_iterator(port.queue.begin()), std::make_move_iterator(port.queue.end()));
						port.queue.clear();
						// If two I/O plugin instances with different latency settings use the same output device, the timestamps are not going to be ordered correctly.
						std::stable_sort(localQueue.begin() + i, localQueue.end());
					}
				}
				waitMs = WaitTimeMs(localQueue[i].m_streamFrame);
			}

			// If we're draining messages, don't send any new note-on messages
			if(MIDIEvents::IsNoteOnEvent(localQueue[i]) && !port.running)
				continue;

			try
			{
				port.rtMidiOut.sendMessage(mpt::byte_cast<const unsigned char *>(localQueue[i].m_message), localQueue[i].m_size);
			} catch(const RtMidiError &)
			{
			}
		}
	}

	clock.SetResolution(0);
}


std::unique_ptr<MidiOutputHandle> MidiDeviceManager::OpenOutput(MidiPortID port)
{
	if(port == kNoMidiDevice)
		return nullptr;

	mpt::lock_guard<mpt::mutex> lock{m_managerMutex};

	auto &shared = m_openOutputs[port];
	if(shared.rtMidiOut.isPortOpen())
	{
		// Port already open, share it
	} else
	{
		// Need to open the port and start the output thread
		try
		{
			shared.queue.reserve(256);
			shared.rtMidiOut.openPort(port);
			shared.running = true;
			shared.outputThread = std::thread([this, &shared]() { OutputThreadFunc(shared); });
		} catch(const RtMidiError &)
		{
			m_openOutputs.erase(port);
			return nullptr;
		}
	}
	shared.refCount++;

	return std::unique_ptr<MidiOutputHandle>(new MidiOutputHandle{shared});
}


void MidiDeviceManager::CloseOutput(MidiOutputHandle &handle)
{
	std::thread thread;
	SharedOutputPort &port = handle.m_port;

	mpt::lock_guard<mpt::mutex> lock{m_managerMutex};

	port.refCount--;
	if(port.HasReferences())
		return;

	// This was the last user of the port, signal thread to drain remaining messages and close the port
	port.running = false;
	port.cv.notify_all();

	if(port.outputThread.joinable())
		port.outputThread.join();

	try
	{
		port.rtMidiOut.closePort();
	} catch(const RtMidiError &)
	{
	}
	auto it = std::find_if(m_openOutputs.begin(), m_openOutputs.end(), [&port](const auto &kv) { return &kv.second == &port; });
	if(it != m_openOutputs.end())
		m_openOutputs.erase(it);
}


MidiOutputHandle::~MidiOutputHandle()
{
	MidiDeviceManager::Instance().CloseOutput(*this);
}


void MidiOutputHandle::Send(mpt::const_byte_span data, int64 streamFrame)
{
	// TODO: make lock-free in the common case (ring buffer)
	{
		mpt::lock_guard<mpt::mutex> lock{m_port.mutex};
		m_port.queue.emplace_back(streamFrame, data);
	}
	m_port.cv.notify_one();
}


void MidiDeviceManager::UpdateSyncPoint(int64 syncPointStreamFrames, uint64 syncPointSystemTimestamp, double speed, uint32 sampleRate)
{
	m_syncPoint = {syncPointStreamFrames, syncPointSystemTimestamp, speed, sampleRate};
}


bool MidiDeviceManager::CheckDeviceStatus(const MidiInputHandle &handle)
{
	// This is really dirty, but I don't think there is a neater way, short of rewriting RtMidi.
	// WinMM does not report on its own if a MIDI device was disconnected.
	// One possibility is to call any API such as midiInGetNumDevs, which will then trigger a MIM_CLOSE message.
	// But RtMidi does not handle MIM_CLOSE, and even if it did, it has no callback mechanism to notify us of such an event.
	// So our only other option really is to try to do something non-destructive with the MIDI In handle and see if it's still valid.
	// midiInGetID is not enough (it will return MMSYSERR_NOERROR), so midiInMessage is our next-best bet.
	class MidiApiEx : public MidiApi
	{
	public:
		bool IsValid()
		{
			if(isPortOpen())
				return false;
			if(getCurrentApi() != RtMidi::WINDOWS_MM)
				return true;
			HMIDIIN handle = *static_cast<HMIDIIN *>(apiData_);  // First member of WinMidiData
			ULONG size = 0;
			return midiInMessage(handle, 0x813 /*DRV_QUERYDEVICEINTERFACESIZE*/, reinterpret_cast<DWORD_PTR>(&size), 0) != MMSYSERR_NODRIVER;
		}
	};

	class RtMidiInEx : public RtMidiIn
	{
	public:
		bool IsValid() const
		{
			return static_cast<MidiApiEx *>(rtapi_)->IsValid();
		}
	};

	return static_cast<const RtMidiInEx &>(handle.m_port.rtMidiIn).IsValid();
}


OPENMPT_NAMESPACE_END
