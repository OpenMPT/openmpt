/*
 * BridgeCommon.h
 * --------------
 * Purpose: Declarations of stuff that's common between the VST bridge and bridge wrapper.
 * Notes  : (currently none)
 * Authors: Johannes Schultz (OpenMPT Devs)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once


#if !defined(assert) && defined(ASSERT)
#define assert ASSERT
#endif

// Insert some object at the end of a char vector.
template<typename T>
static void PushToVector(std::vector<char> &data, const T &obj, size_t writeSize = sizeof(T))
{
	static_assert(std::is_pointer<T>::value == false, "Won't push pointers to data vectors.");
	const char *objC = reinterpret_cast<const char *>(&obj);
	data.insert(data.end(), objC, objC + writeSize);
}

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
	Event() : handle(nullptr) { }
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

	void operator = (const HANDLE other) { handle = other; }
	operator const HANDLE () const { return handle; }
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
	HANDLE mapFile;
public:
	void *view;

	MappedMemory() : mapFile(nullptr), view(nullptr) { }
	~MappedMemory() { Close(); }

	// Create a shared memory object.
	bool Create(const wchar_t *name, uint32 size)
	{
		Close();

		mapFile = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
			0, size, name);
		view = MapViewOfFile(mapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);
		return Good();
	}

	// Open an existing shared memory object.
	bool Open(const wchar_t *name)
	{
		Close();

		mapFile = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, name);
		view = MapViewOfFile(mapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);
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

	size_t Size() const
	{
		LARGE_INTEGER size;
		size.QuadPart = 0;
		GetFileSizeEx(mapFile, &size);
		return static_cast<size_t>(size.QuadPart);
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

	LONG pendingEvents;			// Number of pending automation events
	Parameter params[64];		// Automation events
};


// Host-to-bridge message to initiate a process call.
struct ProcessMsg
{
	enum ProcessType
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

	ProcessMsg(ProcessMsg::ProcessType processType, int32 numInputs, int32 numOutputs, int32 sampleFrames) :
		processType(processType), numInputs(numInputs), numOutputs(numOutputs), sampleFrames(sampleFrames) { }
};


// General message header
struct MsgHeader
{
	enum BridgeMessageType
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
	enum BridgeMessageStatus
	{
		empty = 0,	// Slot is usable
		prepared,	// Slot got acquired and is being prepared
		sent,		// Slot is ready to be sent 
		received,	// Slot is being handled
		done,		// Slot got handled
		delivered,	// Slot got handed back to sender
	};

	LONG status;			// See BridgeMessageStatus
	uint32 size;			// Size of complete message, including this header
	uint32 type;			// See BridgeMessageType
	LONG signalID;			// Signal that should be triggered to answer this message
};


// Host-to-bridge new instance message
struct NewInstanceMsg : public MsgHeader
{
	wchar_t memName[64];	// Shared memory object name;
};


// Host-to-bridge initialization message
struct InitMsg : public MsgHeader
{
	int32 result;
	int32 version;			// Protocol version used by host
	int32 hostPtrSize;		// Size of VstIntPtr in host
	uint32 mixBufSize;		// Interal mix buffer size (for shared memory audio buffers)
	uint32 fullMemDump;		// When crashing, create full memory dumps instead of stack dumps
	wchar_t str[_MAX_PATH];	// Plugin file to load. Out: Error message if result != 0.
};


// Host-to-bridge VST dispatch message
struct DispatchMsg : public MsgHeader
{
	int32 opcode;
	int32 index;
	int64 value;
	int64 ptr;	// Usually, this will be the size of whatever ptr points to. In that case, the data itself is stored after this struct.
	float opt;
	int32 result;
};


// Host-to-bridge VST setParameter / getParameter message
struct ParameterMsg : public MsgHeader
{
	int32 index;	// Parameter ID
	float value;	// Parameter value (in/out)
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
	uint8_t dummy[2048];	// Enough space for most default structs, e.g. 2x speaker negotiation struct

	void SetType(MsgHeader::BridgeMessageType msgType, uint32 size)
	{
		header.status = MsgHeader::empty;
		header.size = size;
		header.type = msgType;
		header.signalID = 0;
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
		init.hostPtrSize = sizeof(VstIntPtr);
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
		std::memcpy(&target, this, std::min<size_t>(header.size, sizeof(BridgeMessage)));
		InterlockedExchange(&header.status, MsgHeader::empty);
	}
};

// This is the maximum size of dispatch data that can be sent in a message. If you want to dispatch more data, use a secondary medium for sending it along.
#define DISPATCH_DATA_SIZE (sizeof(BridgeMessage) - sizeof(DispatchMsg))
static_assert(DISPATCH_DATA_SIZE >= 256, "There should be room for at least 256 bytes of dispatch data!");


// Layout of the shared memory chunk
struct SharedMemLayout
{
	enum
	{
		queueSize = 8u,	// Must be 2^n
	};
	typedef BridgeMessage MsgQueue[queueSize];

	union
	{
		AEffect effect;		// Native layout
		AEffect32 effect32;
		AEffect64 effect64;
	};
	MsgQueue toHost;
	MsgQueue toBridge;
	AutomationQueue automationQueue;
	VstTimeInfo timeInfo;
	int32 tailSize;
};
static_assert(sizeof(AEffect) <= sizeof(AEffect64), "Something's going very wrong here.");


// For caching parameter information
struct ParameterInfo
{
	VstParameterProperties props;
	char name[64];
	char label[64];
	char display[64];
};


#pragma pack(pop)

// Common variables that we will find in both the host and plugin side of the bridge (this is not shared memory)
class BridgeCommon
{
public:
	// Signals for host <-> bridge communication
	Signal sigToHost, sigToBridge, sigProcess;
	// Signals for internal communication (wake up waiting threads). Confirm() => OK, Send() => Failure
	Signal ackSignals[SharedMemLayout::queueSize];

	mpt::UnmanagedThread otherThread;	// Handle of the "other" thread (host side: message thread, bridge side: process thread)
	Event otherProcess;			// Handle of "other" process (host handle in the bridge and vice versa)
	Event sigThreadExit;		// Signal to kill helper thread

	// Shared memory segments
	MappedMemory queueMem;		// AEffect, message, some fixed size VST structures
	MappedMemory processMem;	// Process message + sample buffer
	MappedMemory getChunkMem;	// effGetChunk temporary memory
	MappedMemory eventMem;		// VstEvents memory

	// Pointer into shared memory
	SharedMemLayout * volatile sharedMem;

	// Thread ID for message thread
	uint32 msgThreadID;

	// Pointer size of the "other" side of the bridge, in bytes
	int32 otherPtrSize;

	BridgeCommon() : sharedMem(nullptr), otherPtrSize(0), msgThreadID(0)
	{
		for(size_t i = 0; i < CountOf(ackSignals); i++)
		{
			ackSignals[i].Create();
		}
	}

	bool CreateSignals(const wchar_t *mapName)
	{
		return sigToHost.Create(mapName, L"h")
			&& sigToBridge.Create(mapName, L"b")
			&& sigProcess.Create(mapName, L"p");
	}

	// Copy a message to shared memory and return relative position.
	BridgeMessage *CopyToSharedMemory(const BridgeMessage &msg, SharedMemLayout::MsgQueue &queue)
	{
		assert(msg.header.status == MsgHeader::empty);
		
		// Find a suitable slot to post the message into
		BridgeMessage *targetMsg = queue;
		for(size_t i = 0; i < CountOf(queue); i++, targetMsg++)
		{
			assert((intptr_t(&targetMsg->header.status) & 3) == 0);	// InterlockedExchangeAdd operand should be aligned to 32 bits
			if(InterlockedCompareExchange(&targetMsg->header.status, MsgHeader::prepared, MsgHeader::empty) == MsgHeader::empty)
			{
				memcpy(targetMsg, &msg, std::min(sizeof(BridgeMessage), size_t(msg.header.size)));
				InterlockedExchange(&targetMsg->header.signalID, LONG(i));
				InterlockedExchange(&targetMsg->header.status, MsgHeader::sent);
				return targetMsg;
			}
		}
		// Should never happen, unless there is a crazy amount of threads talking over the bridge, which by itself shouldn't happen.
		assert(false);
		return nullptr;
	}
};


OPENMPT_NAMESPACE_END
