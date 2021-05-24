/*
 * BridgeCommon.h
 * --------------
 * Purpose: Declarations of stuff that's common between the VST bridge and bridge wrapper.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include <vector>

#if (_WIN32_WINNT < _WIN32_WINNT_VISTA)
#include <intrin.h>
#endif


OPENMPT_NAMESPACE_BEGIN

// Insert some object at the end of a char vector.
template <typename T>
static void PushToVector(std::vector<char> &data, const T &obj, size_t writeSize = sizeof(T))
{
	static_assert(!std::is_pointer<T>::value, "Won't push pointers to data vectors.");
	const char *objC = reinterpret_cast<const char *>(&obj);
	data.insert(data.end(), objC, objC + writeSize);
}

static void PushZStringToVector(std::vector<char> &data, const char *str)
{
	if(str != nullptr)
		data.insert(data.end(), str, str + strlen(str));
	PushToVector(data, char(0));
}

OPENMPT_NAMESPACE_END

#include "AEffectWrapper.h"
#include "BridgeOpCodes.h"

OPENMPT_NAMESPACE_BEGIN


// Internal data structures


// Event to notify other threads
class Event
{
private:
	HANDLE handle = nullptr;

public:
	Event() = default;
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
	Event send, confirm;

	// Create new (local) signal
	bool Create()
	{
		return send.Create() && confirm.Create();
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
		return success && confirm.Create(false, fullName);
	}

	// Create signal from other signal
	bool DuplicateFrom(const Signal &other)
	{
		return send.DuplicateFrom(other.send)
		       && confirm.DuplicateFrom(other.confirm);
	}

	void Send()
	{
		send.Trigger();
	}

	void Confirm()
	{
		confirm.Trigger();
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

// Simple atomic value that has a guaranteed size and layout for the shared memory
template <typename T>
struct BridgeAtomic
{
public:
	BridgeAtomic() = default;
	BridgeAtomic<T> &operator=(const T value)
	{
		static_assert(sizeof(m_value) >= sizeof(T));
		MPT_ASSERT((intptr_t(&m_value) & 3) == 0);
#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
		InterlockedExchange(&m_value, static_cast<LONG>(value));
#else
		_InterlockedExchange(&m_value, static_cast<LONG>(value));
#endif
		return *this;
	}
	operator T() const
	{
#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
		return static_cast<T>(InterlockedAdd(&m_value, 0));
#else
		return static_cast<T>(_InterlockedExchangeAdd(&m_value, 0));
#endif
	}

	T exchange(T desired)
	{
#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
		return static_cast<T>(InterlockedExchange(&m_value, static_cast<LONG>(desired)));
#else
		return static_cast<T>(_InterlockedExchange(&m_value, static_cast<LONG>(desired)));
#endif
	}

	T fetch_add(T arg)
	{
#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
		return static_cast<T>(InterlockedExchangeAdd(&m_value, static_cast<LONG>(arg)));
#else
		return static_cast<T>(_InterlockedExchangeAdd(&m_value, static_cast<LONG>(arg)));
#endif
	}

	bool compare_exchange_strong(T &expected, T desired)
	{
#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
		return InterlockedCompareExchange(&m_value, static_cast<LONG>(desired), static_cast<LONG>(expected)) == static_cast<LONG>(expected);
#else
		return _InterlockedCompareExchange(&m_value, static_cast<LONG>(desired), static_cast<LONG>(expected)) == static_cast<LONG>(expected);
#endif
	}

private:
	mutable LONG m_value;
};


// Host-to-bridge parameter automation message
struct AutomationQueue
{
	struct Parameter
	{
		uint32 index;
		float value;
	};

	BridgeAtomic<int32> pendingEvents;  // Number of pending automation events
	Parameter params[64];               // Automation events
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

	BridgeAtomic<bool> isUsed;
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
	int32 hostPtrSize;       // Size of VstIntPtr in host
	uint32 mixBufSize;       // Interal mix buffer size (for shared memory audio buffers)
	int32 pluginID;          // ID to use when sending messages to host
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
		header.isUsed = true;
		header.size = size;
		header.type = msgType;
	}

	void NewInstance(const wchar_t *memName)
	{
		SetType(MsgHeader::newInstance, sizeof(NewInstanceMsg));

		wcsncpy(newInstance.memName, memName, std::size(newInstance.memName) - 1);
	}

	void Init(const wchar_t *pluginPath, uint32 mixBufSize, int32 pluginID, bool fullMemDump)
	{
		SetType(MsgHeader::init, sizeof(InitMsg));

		init.result = 0;
		init.hostPtrSize = sizeof(intptr_t);
		init.mixBufSize = mixBufSize;
		init.pluginID = pluginID;
		init.fullMemDump = fullMemDump;
		wcsncpy(init.str, pluginPath, std::size(init.str) - 1);
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

		wcsncpy(error.str, text, std::size(error.str) - 1);
		error.str[std::size(error.str) - 1] = 0;
	}

	// Copy message to target and clear delivery status
	void CopyTo(BridgeMessage &target)
	{
		std::memcpy(&target, this, std::min(static_cast<size_t>(header.size), sizeof(BridgeMessage)));
		header.isUsed = false;
	}
};

// This is the maximum size of dispatch data that can be sent in a message. If you want to dispatch more data, use a secondary medium for sending it along.
inline constexpr size_t DISPATCH_DATA_SIZE = (sizeof(BridgeMessage) - sizeof(DispatchMsg));
static_assert(DISPATCH_DATA_SIZE >= 256, "There should be room for at least 256 bytes of dispatch data!");


// The array size should be more than enough for any realistic scenario with nested and simultaneous dispatch calls
inline constexpr int MSG_STACK_SIZE = 16;
using MsgStack = std::array<BridgeMessage, MSG_STACK_SIZE>;


// Ensuring that our HWND looks the same to both 32-bit and 64-bit processes
struct BridgeHWND
{
public:
	void operator=(HWND handle) { m_handle = static_cast<int32>(reinterpret_cast<intptr_t>(handle)); }
	operator HWND() const { return reinterpret_cast<HWND>(static_cast<intptr_t>(m_handle)); }
protected:
	BridgeAtomic<int32> m_handle;
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
	MsgStack ipcMessages;
	AutomationQueue automationQueue;
	Vst::VstTimeInfo timeInfo;
	BridgeHWND hostCommWindow;
	BridgeHWND bridgeCommWindow;
	int32 bridgePluginID;
	BridgeAtomic<int32> tailSize;
	BridgeAtomic<int32> audioThreadToHostMsgID;
	BridgeAtomic<int32> audioThreadToBridgeMsgID;
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
	BridgeCommon()
	{
		m_instanceCount++;
	}

	~BridgeCommon()
	{
		m_instanceCount--;
	}

protected:
	enum WindowMessage : UINT
	{
		WM_BRIDGE_KEYFIRST = WM_USER + 4000,  // Must be consistent with VSTEditor.cpp!
		WM_BRIDGE_KEYLAST = WM_BRIDGE_KEYFIRST + WM_KEYLAST - WM_KEYFIRST,
		WM_BRIDGE_MESSAGE_TO_BRIDGE,
		WM_BRIDGE_MESSAGE_TO_HOST,
		WM_BRIDGE_DELETE_PLUGIN,

		WM_BRIDGE_SUCCESS = 1337,
	};

	static std::vector<BridgeCommon *> m_plugins;
	static HWND m_communicationWindow;
	static int m_instanceCount;
	static thread_local bool m_isAudioThread;

	// Signals for host <-> bridge communication
	Signal m_sigToHostAudio, m_sigToBridgeAudio;
	Signal m_sigProcessAudio;
	Event m_sigBridgeReady;

	Event m_otherProcess;  // Handle of "other" process (host handle in the bridge and vice versa)

	// Shared memory segments
	MappedMemory m_queueMem;     // AEffect, message, some fixed size VST structures
	MappedMemory m_processMem;   // Process message + sample buffer
	MappedMemory m_getChunkMem;  // effGetChunk temporary memory
	MappedMemory m_eventMem;     // VstEvents memory

	// Pointer into shared memory
	SharedMemLayout /*volatile*/ *m_sharedMem = nullptr;

	// Pointer size of the "other" side of the bridge, in bytes
	int32 m_otherPtrSize = 0;

	int32 m_thisPluginID = 0;
	int32 m_otherPluginID = 0;

	static void CreateCommunicationWindow(WNDPROC windowProc)
	{
		static constexpr TCHAR windowClassName[] = _T("OpenMPTPluginBridgeCommunication");
		static bool registered = false;
		if(!registered)
		{
			registered = true;
			WNDCLASSEX wndClass;
			wndClass.cbSize = sizeof(WNDCLASSEX);
			wndClass.style = CS_HREDRAW | CS_VREDRAW;
			wndClass.lpfnWndProc = windowProc;
			wndClass.cbClsExtra = 0;
			wndClass.cbWndExtra = 0;
			wndClass.hInstance = GetModuleHandle(nullptr);
			wndClass.hIcon = nullptr;
			wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
			wndClass.hbrBackground = nullptr;
			wndClass.lpszMenuName = nullptr;
			wndClass.lpszClassName = windowClassName;
			wndClass.hIconSm = nullptr;
			RegisterClassEx(&wndClass);
		}

		m_communicationWindow = CreateWindow(
		    windowClassName,
		    _T("OpenMPT Plugin Bridge Communication"),
		    WS_POPUP,
		    CW_USEDEFAULT,
		    CW_USEDEFAULT,
		    1,
		    1,
		    HWND_MESSAGE,
		    nullptr,
		    GetModuleHandle(nullptr),
		    nullptr);
	}

	bool CreateSignals(const wchar_t *mapName)
	{
		wchar_t readyName[64];
		wcscpy(readyName, mapName);
		wcscat(readyName, L"rdy");
		return m_sigToHostAudio.Create(mapName, L"sha")
		       && m_sigToBridgeAudio.Create(mapName, L"sba")
		       && m_sigProcessAudio.Create(mapName, L"prc")
		       && m_sigBridgeReady.Create(false, readyName);
	}

	// Copy a message to shared memory and return relative position.
	int CopyToSharedMemory(const BridgeMessage &msg, MsgStack &stack)
	{
		MPT_ASSERT(msg.header.isUsed);
		for(int i = 0; i < MSG_STACK_SIZE; i++)
		{
			BridgeMessage &targetMsg = stack[i];
			bool expected = false;
			if(targetMsg.header.isUsed.compare_exchange_strong(expected, true))
			{
				std::memcpy(&targetMsg, &msg, std::min(sizeof(BridgeMessage), size_t(msg.header.size)));
				return i;
			}
		}
		return -1;
	}
};


OPENMPT_NAMESPACE_END
