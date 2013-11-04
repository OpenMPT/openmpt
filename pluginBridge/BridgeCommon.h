#pragma once

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
#include "../common/mutex.h"

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


template<size_t queueSize>
struct MsgQueueProto
{
	uint32_t readIndex;
	uint32_t writeIndex;
	uint32_t offset[queueSize];

	MsgQueueProto() : readIndex(0), writeIndex(0) { }
};


struct MsgQueue
{
	MsgQueueProto<1024> toHost;
	MsgQueueProto<1024> toBridge;
};


// General message header
struct MsgHeader
{
	enum BridgeMessageType
	{
		// Management messages, host to bridge
		init = 0,
		close,
		// Management messages, bridge to host
		reallocate,
		errorMsg,

		// VST messages, common
		dispatch,
		// VST messages, host to bridge
		setParameter,
		getParameter,
	};

	uint32_t headerSize;		// sizeof(MsgHeader), only here because the struct would be padded anyway, and as extra verification during the intialization step.
	uint32_t size;				// Size of complete message, including this header
	uint32_t type;				// See BridgeMessageType
	uint32_t ackSignal;			// Signal to send when processing of this message is done
	uint32_t interruptSignal;	// Signal to send when this thread needs to send back a new message while handling this message
	uint32_t interruptOffset;	// Offset of message in buffer if interruptSignal has been fired (set by interrupting thread).
	int64_t  result;			// Result of this message (exact meaning depends on message type, not every message has a result)

	MsgHeader(uint32_t type, uint32_t size) : headerSize(sizeof(MsgHeader)), size(size), type(type), ackSignal(0), interruptSignal(0), interruptOffset(0), result(0) { }
};


// Host-to-bridge initialization message
struct InitMsg : public MsgHeader
{
	enum ProtocolVersion
	{
		protocolVersion = 1,
	};

	int32_t version;		// In: Protocol version used by host
	int32_t mapSize;		// Out: Current map size
	int32_t samplesOffset;	// Out: Offset into shared memory of sample buffers
	int32_t hostPtrSize;	// In: Size of VstIntPtr in host
	wchar_t str[_MAX_PATH];	// In: Plugin file to load. Out: Error message if result != 0.

	InitMsg(const wchar_t *pluginPath) : MsgHeader(MsgHeader::init, sizeof(InitMsg)),
		version(protocolVersion), hostPtrSize(sizeof(VstIntPtr))
	{
		wcsncpy(str, pluginPath, CountOf(str) - 1);
		str[CountOf(str) - 1] = 0;
	}
};


// Bridge-to-host memory reallocation message
struct ReallocMsg : public MsgHeader
{
	enum DefaultSizes
	{
		defaultMapSize = 1024 * 1024,
		defaultGlobalsSize = sizeof(AEffect64) + sizeof(ProcessMsg) + 512 * 16 * 4,	// 16 channels of floats
	};

	//wchar_t str[64];	// New map name
	int32_t  handle;	// New map handle
	uint32_t mapSize;	// New map size

	ReallocMsg(int32_t newHandle, uint32_t size) : MsgHeader(MsgHeader::reallocate, sizeof(ReallocMsg)), handle(newHandle), mapSize(size) { }
	/*ReallocMsg(wchar_t *newName, uint32_t size) : MsgHeader(MsgHeader::reallocate, sizeof(ReallocMsg)), mapSize(size)
	{
		wcsncpy(str, newName, CountOf(str) - 1);
		str[CountOf(str) - 1] = 0;
	}*/
};


// Host-to-bridge VST dispatch message
struct DispatchMsg : public MsgHeader
{
	int32_t opcode;
	int32_t index;
	int64_t value;
	int64_t ptr;	// Usually, this will be the size of ptr. In that case, the data itself is stored after this struct.
	float   opt;

	DispatchMsg(int32_t opcode, int32_t index, int64_t value, int64_t ptr, float opt, int32_t extraSize) : MsgHeader(MsgHeader::dispatch, sizeof(DispatchMsg) + extraSize),
		opcode(opcode), index(index), value(value), ptr(ptr), opt(opt) { }
};


// Host-to-bridge VST setParameter / getParameter message
struct ParameterMsg : public MsgHeader
{
	int32_t index;
	float value;

	ParameterMsg(int32_t index, float value) : MsgHeader(MsgHeader::setParameter, sizeof(ParameterMsg)), index(index), value(value) { }
	ParameterMsg(int32_t index) : MsgHeader(MsgHeader::getParameter, sizeof(ParameterMsg)), index(index), value(0) { }
};


// Bridge-to-host error message
struct ErrorMsg : public MsgHeader
{
	wchar_t str[64];

	ErrorMsg(const wchar_t *text) : MsgHeader(MsgHeader::errorMsg, sizeof(ErrorMsg))
	{
		wcsncpy(str, text, CountOf(str) - 1);
		str[CountOf(str) - 1] = 0;
	}
};

#pragma pack(pop)

// Internal data

struct Signal
{
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
	void Create(HANDLE s, HANDLE a)
	{
		send = s;
		ack = a;
	}

	// Create signal from existing handles
	void Create(const TCHAR *s, const TCHAR *a)
	{
		send = reinterpret_cast<HANDLE>(_ttoi(s));
		ack = reinterpret_cast<HANDLE>(_ttoi(a));
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


#include <map>

struct SharedMem
{
	Signal defaultSignal;
	Signal interruptSignal[8];
	std::map<uint32_t, Signal *> threadMap;	// Map thread IDs to signal pairs

	// Signals for host <-> bridge communication
	Signal sigToHost, sigToBridge, sigProcess;

	HANDLE otherProcess;	// Handle of "other" process (host handle in the bridge and vice versa)

	HANDLE sigThreadExit;	// Signal to kill helper thread

	HANDLE mapFile;			// Internal handle of memory-mapped file
	Util::mutex writeMutex;
	char *sharedPtr;		// Pointer to beginning of shared memory
	void *queuePtr;			// Pointer to shared memory queue
	AEffect *effectPtr;		// Pointer to shared AEffect struct (with host pointer format)
	void *sampleBufPtr;		// Pointer to shared memory area which contains sample buffers

	uint32_t queueSize;		// Size of shared memory queue
	uint32_t sampleBufSize;	// Size of shared sample memory
	uint32_t writeOffset;	// Message offset in shared memory

	int32_t otherPtrSize;

	SharedMem() : mapFile(nullptr), queuePtr(nullptr), effectPtr(nullptr), sampleBufPtr(nullptr), queueSize(ReallocMsg::defaultMapSize),
		sampleBufSize(ReallocMsg::defaultGlobalsSize), writeOffset(uint32_t(sizeof(MsgQueue))), otherPtrSize(0) { }
};
