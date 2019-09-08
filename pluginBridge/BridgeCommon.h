/*
 * BridgeCommon.h
 * --------------
 * Purpose: Declarations of stuff that's common between the VST bridge and bridge wrapper.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

#include <map>
#include <shared_mutex>
#include <thread>
#include <vector>


OPENMPT_NAMESPACE_BEGIN

// Insert some object at the end of a char vector.
template <typename T>
static void PushToVector(std::vector<char> &data, const T &obj, size_t writeSize = sizeof(T))
{
	static_assert(!std::is_pointer<T>::value, "Won't push pointers to data vectors.");
	const char *objC = reinterpret_cast<const char *>(&obj);
	data.insert(data.end(), objC, objC + writeSize);
}

OPENMPT_NAMESPACE_END

#include "AEffectWrapper.h"
#include "BridgeOpCodes.h"
#include "../common/mptThread.h"


OPENMPT_NAMESPACE_BEGIN


// Internal data structures


// Event to notify other threads
class Event
{
protected:
	HANDLE handle;

public:
	Event()
	    : handle(nullptr) {}
	~Event() { Close(); }

	// Create a new event
	bool Create(bool manual = false, const wchar_t *name = nullptr)
	{
		Close();
		handle = CreateEventW(nullptr, manual ? TRUE : FALSE, FALSE, name);
		return handle != nullptr;
	}

	// Duplicate a local event
	bool DuplicateFrom(HANDLE source)
	{
		Close();
		return DuplicateHandle(GetCurrentProcess(), source, GetCurrentProcess(), &handle, 0, FALSE, DUPLICATE_SAME_ACCESS) != FALSE;
	}

	void Close()
	{
		CloseHandle(handle);
	}

	void Trigger()
	{
		SetEvent(handle);
	}

	void Reset()
	{
		ResetEvent(handle);
	}

	void operator=(const HANDLE other) { handle = other; }
	operator const HANDLE() const { return handle; }
};


// Two-way event
class Signal
{
public:
	Event send, ack;

	// Create new (local) signal
	bool Create()
	{
		return send.Create() && ack.Create();
	}

	// Create signal from name (for inter-process communication)
	bool Create(const wchar_t *name, const wchar_t *addendum)
	{
		wchar_t fullName[64];
		wcscpy(fullName, name);
		wcscat(fullName, addendum);
		size_t nameLen = wcslen(fullName);
		wcscpy(fullName + nameLen, L"-s");

		bool success = send.Create(false, fullName);
		wcscpy(fullName + nameLen, L"-a");
		return success && ack.Create(false, fullName);
	}

	// Create signal from other signal
	bool DuplicateFrom(const Signal &other)
	{
		return send.DuplicateFrom(other.send)
		       && ack.DuplicateFrom(other.ack);
	}

	void Send()
	{
		send.Trigger();
	}

	void Confirm()
	{
		ack.Trigger();
	}
};


// Memory that can be shared between processes
class MappedMemory
{
protected:
	struct Header
	{
		uint32 size;
	};

	HANDLE mapFile = nullptr;
	Header *view = nullptr;

public:
	MappedMemory() = default;
	~MappedMemory() { Close(); }

	// Create a shared memory object.
	bool Create(const wchar_t *name, uint32 size)
	{
		Close();

		mapFile = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, size + sizeof(Header), name);
		view = static_cast<Header *>(MapViewOfFile(mapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0));
		view->size = size;
		return Good();
	}

	// Open an existing shared memory object.
	bool Open(const wchar_t *name)
	{
		Close();

		mapFile = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, name);
		view = static_cast<Header *>(MapViewOfFile(mapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0));
		return Good();
	}

	// Close this shared memory object.
	void Close()
	{
		if(mapFile)
		{
			if(view)
			{
				UnmapViewOfFile(view);
				view = nullptr;
			}
			CloseHandle(mapFile);
			mapFile = nullptr;
		}
	}

	template <typename T = void>
	T *Data() const
	{
		if(view == nullptr)
			return nullptr;
		else
			return reinterpret_cast<T *>(view + 1);
	}

	size_t Size() const
	{
		if(!view)
			return 0;
		else
			return view->size;
	}

	bool Good() const { return view != nullptr; }

	// Make a copy and detach it from the other object
	void CopyFrom(MappedMemory &other)
	{
		Close();
		mapFile = other.mapFile;
		view = other.view;
		other.mapFile = nullptr;
		other.view = nullptr;
	}
};


// Bridge communication data

#pragma pack(push, 8)

// Host-to-bridge parameter automation message
struct AutomationQueue
{
	struct Parameter
	{
		uint32 index;
		float value;
	};

	LONG pendingEvents;    // Number of pending automation events
	Parameter params[64];  // Automation events
};


// Host-to-bridge message to initiate a process call.
struct ProcessMsg
{
	enum ProcessType : int32
	{
		process = 0,
		processReplacing,
		processDoubleReplacing,
	};

	int32 processType;
	int32 numInputs;
	int32 numOutputs;
	int32 sampleFrames;
	// Input and output buffers follow
};


// General message header
struct MsgHeader
{
	enum BridgeMessageType : uint32
	{
		// Management messages, host to bridge
		newInstance,
		init,
		// Management messages, bridge to host
		errorMsg,
		exceptionMsg,

		// VST messages, common
		dispatch,
		// VST messages, host to bridge
		setParameter,
		getParameter,
		automate,
	};

	// Message life-cycle
	enum BridgeMessageStatus : LONG
	{
		empty = 0,  // Slot is usable
		sent,       // Slot is ready to be sent
		received,   // Slot is being handled
		done,       // Slot got handled
		delivered,  // Slot got handed back to sender
	};

	LONG status;  // See BridgeMessageStatus
	uint32 size;  // Size of complete message, including this header
	uint32 type;  // See BridgeMessageType
};


// Host-to-bridge new instance message
struct NewInstanceMsg : public MsgHeader
{
	wchar_t memName[64];  // Shared memory object name;
};


// Host-to-bridge initialization message
struct InitMsg : public MsgHeader
{
	int32 result;
	int32 version;           // Protocol version used by host
	int32 hostPtrSize;       // Size of VstIntPtr in host
	uint32 mixBufSize;       // Interal mix buffer size (for shared memory audio buffers)
	uint32 fullMemDump;      // When crashing, create full memory dumps instead of stack dumps
	wchar_t str[_MAX_PATH];  // Plugin file to load. Out: Error message if result != 0.
};


// Host-to-bridge or bridge-to-host VST dispatch message
struct DispatchMsg : public MsgHeader
{
	int32 opcode;
	int32 index;
	int64 value;
	int64 ptr;  // Usually, this will be the size of whatever ptr points to. In that case, the data itself is stored after this struct.
	float opt;
	int32 result;
};


// Host-to-bridge VST setParameter / getParameter message
struct ParameterMsg : public MsgHeader
{
	int32 index;  // Parameter ID
	float value;  // Parameter value (in/out)
};


// Bridge-to-host error message
struct ErrorMsg : public MsgHeader
{
	wchar_t str[64];
};


// Universal bridge message format
union BridgeMessage
{
	MsgHeader header;
	NewInstanceMsg newInstance;
	InitMsg init;
	DispatchMsg dispatch;
	ParameterMsg parameter;
	ErrorMsg error;
	uint8 dummy[2048];  // Enough space for most default structs, e.g. 2x speaker negotiation struct

	void SetType(MsgHeader::BridgeMessageType msgType, uint32 size)
	{
		header.status = MsgHeader::empty;
		header.size = size;
		header.type = msgType;
	}

	void NewInstance(const wchar_t *memName)
	{
		SetType(MsgHeader::newInstance, sizeof(NewInstanceMsg));

		wcsncpy(newInstance.memName, memName, CountOf(newInstance.memName) - 1);
	}

	void Init(const wchar_t *pluginPath, uint32 mixBufSize, bool fullMemDump)
	{
		SetType(MsgHeader::init, sizeof(InitMsg));

		init.result = 0;
		init.hostPtrSize = sizeof(intptr_t);
		init.mixBufSize = mixBufSize;
		init.fullMemDump = fullMemDump;
		wcsncpy(init.str, pluginPath, CountOf(init.str) - 1);
	}

	void Dispatch(int32 opcode, int32 index, int64 value, int64 ptr, float opt, uint32 extraDataSize)
	{
		SetType(MsgHeader::dispatch, sizeof(DispatchMsg) + extraDataSize);

		dispatch.result = 0;
		dispatch.opcode = opcode;
		dispatch.index = index;
		dispatch.value = value;
		dispatch.ptr = ptr;
		dispatch.opt = opt;
	}

	void Dispatch(Vst::VstOpcodeToHost opcode, int32 index, int64 value, int64 ptr, float opt, uint32 extraDataSize)
	{
		Dispatch(static_cast<int32>(opcode), index, value, ptr, opt, extraDataSize);
	}

	void Dispatch(Vst::VstOpcodeToPlugin opcode, int32 index, int64 value, int64 ptr, float opt, uint32 extraDataSize)
	{
		Dispatch(static_cast<int32>(opcode), index, value, ptr, opt, extraDataSize);
	}

	void SetParameter(int32 index, float value)
	{
		SetType(MsgHeader::setParameter, sizeof(ParameterMsg));

		parameter.index = index;
		parameter.value = value;
	}

	void GetParameter(int32 index)
	{
		SetType(MsgHeader::getParameter, sizeof(ParameterMsg));

		parameter.index = index;
		parameter.value = 0.0f;
	}

	void Automate()
	{
		// Dummy message
		SetType(MsgHeader::automate, sizeof(MsgHeader));
	}

	void Error(const wchar_t *text)
	{
		SetType(MsgHeader::errorMsg, sizeof(ErrorMsg));

		wcsncpy(error.str, text, CountOf(error.str) - 1);
		error.str[CountOf(error.str) - 1] = 0;
	}

	// Copy message to target and clear delivery status
	void CopyFromSharedMemory(BridgeMessage &target)
	{
		std::memcpy(&target, this, std::min(static_cast<size_t>(header.size), sizeof(BridgeMessage)));
		InterlockedExchange(&header.status, MsgHeader::empty);
	}
};

// This is the maximum size of dispatch data that can be sent in a message. If you want to dispatch more data, use a secondary medium for sending it along.
inline constexpr size_t DISPATCH_DATA_SIZE = (sizeof(BridgeMessage) - sizeof(DispatchMsg));
static_assert(DISPATCH_DATA_SIZE >= 256, "There should be room for at least 256 bytes of dispatch data!");

struct MsgStack
{
	enum
	{
		kStackSize = 16u,  // Should be more than enough for any realistic scenario with nested dispatch calls
	};
	LONG readPos = 0, writePos = 0;
	std::array<BridgeMessage, kStackSize> msg;

	BridgeMessage &ReadNext()
	{
		auto pos = InterlockedIncrement(&readPos);
		MPT_ASSERT(pos > 0 && pos <= kStackSize);
		return msg[pos - 1];
	}
};

// Layout of the shared memory chunk
struct SharedMemLayout
{
	union
	{
		Vst::AEffect effect;  // Native layout from host perspective
		AEffect32 effect32;
		AEffect64 effect64;
	};
	MsgStack guiThreadMessages;
	MsgStack audioThreadMessages;
	MsgStack msgThreadMessages;
	AutomationQueue automationQueue;
	Vst::VstTimeInfo timeInfo;
	int32 tailSize;
};
static_assert(sizeof(Vst::AEffect) <= sizeof(AEffect64), "Something's going very wrong here.");


// For caching parameter information
struct ParameterInfo
{
	Vst::VstParameterProperties props;
	char name[64];
	char label[64];
	char display[64];
};


#pragma pack(pop)

// Common variables that we will find in both the host and plugin side of the bridge (this is not shared memory)
class BridgeCommon
{
public:
	BridgeCommon() = default;

protected:
	// Signals for host <-> bridge communication
	Signal m_sigToHostGui, m_sigToBridgeGui;
	Signal m_sigToHostAudio, m_sigToBridgeAudio;
	Signal m_sigToHostMsgThread, m_sigToBridgeMsgThread;
	Signal m_sigProcessAudio;

	// The thread context indicates which message stack to use and which signals to send / listen to
	struct ThreadContext
	{
		MsgStack *msgStack;
		Signal *signalToHost;
		Signal *signalToBridge;

		bool Valid() const { return msgStack && signalToHost && signalToBridge; }
	};

	mpt::UnmanagedThread m_otherThread;  // Handle of the "other" thread (host side: message thread, bridge side: process thread)
	Event m_otherProcess;                // Handle of "other" process (host handle in the bridge and vice versa)
	Event m_sigThreadExit;               // Signal to kill helper thread

	// Shared memory segments
	MappedMemory m_queueMem;     // AEffect, message, some fixed size VST structures
	MappedMemory m_processMem;   // Process message + sample buffer
	MappedMemory m_getChunkMem;  // effGetChunk temporary memory
	MappedMemory m_eventMem;     // VstEvents memory

	// Pointer into shared memory
	SharedMemLayout *volatile m_sharedMem = nullptr;

	// Pointer size of the "other" side of the bridge, in bytes
	int32 m_otherPtrSize = 0;

private:
	std::map<std::thread::id, ThreadContext> m_threadContext;
	mutable std::shared_mutex m_contextMutex;

protected:
	bool CreateSignals(const wchar_t *mapName)
	{
		return m_sigToHostGui.Create(mapName, L"hg")
		       && m_sigToBridgeGui.Create(mapName, L"bg")
		       && m_sigToHostAudio.Create(mapName, L"hp")
		       && m_sigToBridgeAudio.Create(mapName, L"bp")
		       && m_sigToHostMsgThread.Create(mapName, L"hm")
		       && m_sigToBridgeMsgThread.Create(mapName, L"bm")
		       && m_sigProcessAudio.Create(mapName, L"p");
	}

	// Set current thread context and returns the old context
	ThreadContext SetContext(MsgStack &msgStack, Signal &signalToHost, Signal &signalToBridge)
	{
		std::unique_lock lock(m_contextMutex);
		auto &context = m_threadContext[std::this_thread::get_id()];
		const auto oldContext = context;
		context = {&msgStack, &signalToHost, &signalToBridge};
		return oldContext;
	}

	// Clears current thread context
	void ClearContext()
	{
		std::unique_lock lock(m_contextMutex);
		m_threadContext.erase(std::this_thread::get_id());
	}

	// Returns current thread context, or falls back to GUI context if none exists
	std::tuple<MsgStack &, Signal &, Signal &> Context()
	{
		std::shared_lock lock(m_contextMutex);
		const auto ctx = m_threadContext.find(std::this_thread::get_id());
		if(ctx != m_threadContext.end())
			return {*ctx->second.msgStack, *ctx->second.signalToHost, *ctx->second.signalToBridge};
		else
			return {m_sharedMem->guiThreadMessages, m_sigToHostGui, m_sigToBridgeGui};
	}

	// Copy a message to shared memory and return relative position.
	BridgeMessage *CopyToSharedMemory(const BridgeMessage &msg, MsgStack &stack)
	{
		MPT_ASSERT(msg.header.status == MsgHeader::empty);
		MPT_ASSERT((intptr_t(&stack.writePos) & 3) == 0);

		const LONG writePos = InterlockedIncrement(&stack.writePos) - 1;
		if(writePos >= MsgStack::kStackSize)
		{
			// Stack full! Should never happen, unless there is a crazy amount of threads talking over the bridge or too many nested messages, which by itself shouldn't happen.
			InterlockedDecrement(&stack.writePos);
			return nullptr;
		}

		BridgeMessage &targetMsg = stack.msg[writePos];
		MPT_ASSERT((intptr_t(&targetMsg.header.status) & 3) == 0);  // InterlockedExchangeAdd operand should be aligned to 32 bits
		InterlockedExchange(&targetMsg.header.status, MsgHeader::empty);
		std::memcpy(&targetMsg, &msg, std::min(sizeof(BridgeMessage), size_t(msg.header.size)));
		InterlockedExchange(&targetMsg.header.status, MsgHeader::sent);
		return &targetMsg;
	}
};


OPENMPT_NAMESPACE_END
