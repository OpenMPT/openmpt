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

// Internal data structures

typedef int32_t winhandle_t;	// Cross-bridge handle type (WinAPI handles are 32-bit, even on 64-bit systems, so they can be passed between processes of different bitness)

class Signal
{
public:
	HANDLE send, ack;

	Signal() : send(nullptr), ack(nullptr) { }
	~Signal()
	{
		CloseHandle(send);
		CloseHandle(ack);
	}

	// Create new signal
	bool Create()
	{
		// TODO un-inherit
		// We want to share our handles.
		SECURITY_ATTRIBUTES secAttr;
		secAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
		secAttr.lpSecurityDescriptor = nullptr;
		secAttr.bInheritHandle = TRUE;

		send = CreateEvent(&secAttr, FALSE, FALSE, NULL);
		ack = CreateEvent(&secAttr, FALSE, FALSE, NULL);
		return send != nullptr && ack != nullptr;
	}

	// Create signal from existing handles
	void Create(winhandle_t s, winhandle_t a)
	{
		send = reinterpret_cast<HANDLE>(s);
		ack = reinterpret_cast<HANDLE>(a);
	}

	// Create signal from existing handles
	void Create(const TCHAR *s, const TCHAR *a)
	{
		send = reinterpret_cast<HANDLE>(_ttoi(s));
		ack = reinterpret_cast<HANDLE>(_ttoi(a));
	}

	// Create signal from other signal
	void Create(const Signal &other)
	{
		send = other.send;
		ack = other.ack;
	}

	void Send()
	{
		SetEvent(send);
	}

	void Confirm()
	{
		SetEvent(ack);
	}
};


class MappedMemory
{
protected:
	HANDLE mapFile;
public:
	void *view;

	MappedMemory() : mapFile(nullptr), view(nullptr) { }
	~MappedMemory() { Close(); }

	// Create a shared memory object.
	bool Create(const wchar_t *name, uint32_t size)
	{
		Close();

		// TODO un-inherit
		SECURITY_ATTRIBUTES secAttr;
		secAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
		secAttr.lpSecurityDescriptor = nullptr;
		secAttr.bInheritHandle = TRUE;

		mapFile = CreateFileMappingW(INVALID_HANDLE_VALUE, &secAttr, PAGE_READWRITE,
			0, size, name);
		view = MapViewOfFile(mapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);
		return Good();
	}

	// Open an existing shared memory object.
	bool Open(const wchar_t *name)
	{
		Close();

		// TODO un-inherit
		mapFile = OpenFileMappingW(FILE_MAP_ALL_ACCESS, TRUE, name);
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

	bool Good() const { return view != nullptr; }
};


// Insert some object at the end of a char vector.
template<typename T>
static void PushToVector(std::vector<char> &data, const T &obj, size_t writeSize = sizeof(T))
{
#ifdef HAS_TYPE_TRAITS
	static_assert(std::is_pointer<T>::value == false, "Won't push pointers to data vectors.");
#endif // HAS_TYPE_TRAITS
	const char *objC = reinterpret_cast<const char *>(&obj);
	data.insert(data.end(), objC, objC + writeSize);
}

#include "AEffectWrapper.h"
//#include <map>

// Bridge communication data

#pragma pack(push, 8)

// Host-to-bridge message to initiate a process call.
struct ProcessMsg
{
	enum ProcessType
	{
		process = 0,
		processReplacing,
		processDoubleReplacing,
	};

	int32_t processType;
	int32_t numInputs;
	int32_t numOutputs;
	int32_t sampleFrames;
	// Input and output buffers follow

	ProcessMsg(ProcessMsg::ProcessType processType, int32_t numInputs, int32_t numOutputs, int32_t sampleFrames) :
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
		close,
		// Management messages, bridge to host
		errorMsg,

		// VST messages, common
		dispatch,
		// VST messages, host to bridge
		setParameter,
		getParameter,
	};

	// Message life-cycle
	enum BridgeMessageStatus
	{
		empty = 0,
		sent,
		received,
		done,
	};

	uint32_t status;		// See BridgeMessageStatus
	uint32_t size;			// Size of complete message, including this header
	uint32_t type;			// See BridgeMessageType
	uint32_t signalID;		// Signal that should be triggered to answer this message
};


// Host-to-bridge new instance message
struct NewInstanceMsg : public MsgHeader
{
	wchar_t memName[64];	// Shared memory object name;
	winhandle_t handles[6];	// Signal handles
};


// Host-to-bridge initialization message
struct InitMsg : public MsgHeader
{
	int32_t result;
	int32_t version;		// Protocol version used by host
	int32_t hostPtrSize;	// Size of VstIntPtr in host
	uint32_t mixBufSize;	// Interal mix buffer size (for shared memory audio buffers)
	wchar_t str[_MAX_PATH];	// Plugin file to load. Out: Error message if result != 0.
};


// Host-to-bridge VST dispatch message
struct DispatchMsg : public MsgHeader
{
	int32_t opcode;
	int32_t index;
	int64_t value;
	int64_t ptr;	// Usually, this will be the size of whatever ptr points to. In that case, the data itself is stored after this struct.
	float   opt;
	int32_t result;
};


// Host-to-bridge VST setParameter / getParameter message
struct ParameterMsg : public MsgHeader
{
	int32_t index;	// Parameter ID
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
	uint8_t dummy[1024];

	void SetType(MsgHeader::BridgeMessageType msgType, uint32_t size)
	{
		header.status = MsgHeader::empty;
		header.size = size;
		header.type = msgType;
		header.signalID = 0;
	}

	void NewInstance(const wchar_t *memName, const Signal &sigToHost, const Signal &sigToBridge, const Signal &sigProcess)
	{
		wcsncpy(newInstance.memName, memName, CountOf(newInstance.memName) - 1);
		newInstance.memName[CountOf(newInstance.memName) - 1] = 0;
		newInstance.handles[0] = reinterpret_cast<winhandle_t>(sigToHost.send);
		newInstance.handles[0] = reinterpret_cast<winhandle_t>(sigToHost.ack);
		newInstance.handles[0] = reinterpret_cast<winhandle_t>(sigToBridge.send);
		newInstance.handles[0] = reinterpret_cast<winhandle_t>(sigToBridge.ack);
		newInstance.handles[0] = reinterpret_cast<winhandle_t>(sigProcess.send);
		newInstance.handles[0] = reinterpret_cast<winhandle_t>(sigProcess.ack);
	}

	void Init(const wchar_t *pluginPath, uint32_t mixBufSize)
	{
		SetType(MsgHeader::init, sizeof(InitMsg));

		init.result = 0;
		init.hostPtrSize = sizeof(VstIntPtr);
		init.mixBufSize = mixBufSize;
		wcsncpy(init.str, pluginPath, CountOf(init.str) - 1);
		init.str[CountOf(init.str) - 1] = 0;
	}

	void Close()
	{
		SetType(MsgHeader::close, sizeof(MsgHeader));
	}

	void Dispatch(int32_t opcode, int32_t index, int64_t value, int64_t ptr, float opt, uint32_t extraDataSize)
	{
		SetType(MsgHeader::dispatch, sizeof(DispatchMsg) + extraDataSize);

		dispatch.result = 0;
		dispatch.opcode = opcode;
		dispatch.index = index;
		dispatch.value = value;
		dispatch.ptr = ptr;
		dispatch.opt = opt;
	}

	void SetParameter(int32_t index, float value)
	{
		SetType(MsgHeader::setParameter, sizeof(ParameterMsg));

		parameter.index = index;
		parameter.value = value;
	}

	void GetParameter(int32_t index)
	{
		SetType(MsgHeader::getParameter, sizeof(ParameterMsg));

		parameter.index = index;
		parameter.value = 0.0f;
	}

	void Error(const wchar_t *text)
	{
		SetType(MsgHeader::errorMsg, sizeof(ErrorMsg));

		wcsncpy(error.str, text, CountOf(error.str) - 1);
		error.str[CountOf(error.str) - 1] = 0;
	}
};

// This is the maximum size of dispatch data that can be sent in a message. If you want to dispatch more data, use a secondary medium for sending it along.
#define DISPATCH_DATA_SIZE (sizeof(BridgeMessage) - sizeof(DispatchMsg))
static_assert(DISPATCH_DATA_SIZE >= 256, "There should be room for at least 256 bytes of dispatch data!");


struct MsgQueue
{
	enum
	{
		queueSize = 8u,	// Must be 2^n
	};

	BridgeMessage toHost[queueSize];
	BridgeMessage toBridge[queueSize];
};


#pragma pack(pop)

class SharedMem
{
public:
	//std::map<winhandle_t, SignalSlot *> threadMap;	// Map thread IDs to signals

	// Signals for host <-> bridge communication
	Signal sigToHost, sigToBridge, sigProcess;
	// Signals for internal communication (wake up waiting threads). Ack() => OK, Send() => Failure
	Signal ackSignals[MsgQueue::queueSize];

	HANDLE otherProcess;	// Handle of "other" process (host handle in the bridge and vice versa)
	HANDLE sigThreadExit;	// Signal to kill helper thread

	// Shared memory segments
	MappedMemory queueMem, processMem, getChunkMem;

	// Various pointers into shared memory
	MsgQueue *queuePtr;				// Message queue indexes
	AEffect *effectPtr;				// Pointer to shared AEffect struct (with host pointer format)

	uint32_t msgThreadID;
	volatile uint32_t writeOffset;	// Message offset in shared memory

	int32_t otherPtrSize;

	SharedMem() : effectPtr(nullptr), queuePtr(nullptr), writeOffset(0), otherPtrSize(0), msgThreadID(0)
	{
		for(size_t i = 0; i < CountOf(ackSignals); i++)
		{
			ackSignals[i].Create();
		}
	}

	void SetupPointers()
	{
		effectPtr = reinterpret_cast<AEffect *>(queueMem.view);
		queuePtr = reinterpret_cast<MsgQueue *>(static_cast<AEffect64 *>(queueMem.view) + 1);
	}

	// Copy a message to shared memory and return relative position.
	BridgeMessage *CopyToSharedMemory(const BridgeMessage &msg, BridgeMessage *queue, bool fromMsgThread)
	{
		assert((reinterpret_cast<intptr_t>(&writeOffset) & 3) == 0);	// InterlockedExchangeAdd operand should be aligned to 32 bits
		uint32_t offset = InterlockedExchangeAdd(&writeOffset, 1) % MsgQueue::queueSize;

		assert(msg.header.status == MsgHeader::empty);
		assert(queue[offset].header.status == MsgHeader::empty);
		memcpy(queue + offset, &msg, std::min(sizeof(BridgeMessage), size_t(msg.header.size)));

		queue[offset].header.signalID = fromMsgThread ? uint32_t(-1) : offset;	// Don't send signal to ourselves

		assert((reinterpret_cast<intptr_t>(&queue[offset].header.status) & 3) == 0);	// InterlockedExchange operand should be aligned to 32 bits
		InterlockedExchange(&queue[offset].header.status, MsgHeader::sent);

		return queue + offset;
	}
};
