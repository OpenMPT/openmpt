/*
 * Bridge.cpp
 * ----------
 * Purpose: VST plugin bridge (plugin side)
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


// TODO: Translate pointer-sized members in remaining structs: VstVariableIo, VstOfflineTask, VstAudioFile, VstWindow (all these are currently not supported by OpenMPT, so not urgent at all)

#include "../common/BuildSettings.h"
#include "../common/mptBaseMacros.h"
#include "../common/mptBaseTypes.h"
#include <Windows.h>
#include <ShellAPI.h>
#include <ShlObj.h>
#include <CommDlg.h>
#include <tchar.h>
#include <algorithm>

#if defined(MPT_BUILD_MSVC)
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#endif


#if MPT_BUILD_DEBUG
#include <intrin.h>
#define MPT_ASSERT(x) \
	MPT_MAYBE_CONSTANT_IF(!(x)) \
	{ \
		if(IsDebuggerPresent()) \
			__debugbreak(); \
		::MessageBoxA(nullptr, "Debug Assertion Failed:\n\n" #x, "OpenMPT Plugin Bridge", MB_ICONERROR); \
	}
#else
#define MPT_ASSERT(x)
#endif

#include "../misc/WriteMemoryDump.h"
#include "Bridge.h"
using namespace Vst;


// Crash handler for writing memory dumps
static LONG WINAPI CrashHandler(_EXCEPTION_POINTERS *pExceptionInfo)
{
	WCHAR tempPath[MAX_PATH + 2];
	DWORD result = GetTempPathW(MAX_PATH + 1, tempPath);
	if(result > 0 && result <= MAX_PATH + 1)
	{
		std::wstring filename = tempPath;
		filename += L"OpenMPT Crash Files\\";
		CreateDirectoryW(filename.c_str(), nullptr);

		tempPath[0] = 0;
		const int ch = GetDateFormatW(LOCALE_SYSTEM_DEFAULT, 0, nullptr, L"'PluginBridge 'yyyy'-'MM'-'dd ", tempPath, CountOf(tempPath));
		if(ch)
			GetTimeFormatW(LOCALE_SYSTEM_DEFAULT, 0, nullptr, L"HH'.'mm'.'ss'.dmp'", tempPath + ch - 1, CountOf(tempPath) - ch + 1);
		filename += tempPath;
		OPENMPT_NAMESPACE::WriteMemoryDump(pExceptionInfo, filename.c_str(), OPENMPT_NAMESPACE::PluginBridge::m_fullMemDump);
	}

	// Let Windows handle the exception...
	return EXCEPTION_CONTINUE_SEARCH;
}


int _tmain(int argc, TCHAR *argv[])
{
	if(argc != 2)
	{
		MessageBox(nullptr, _T("This executable is part of OpenMPT. You do not need to run it by yourself."), _T("OpenMPT Plugin Bridge"), 0);
		return -1;
	}

	::SetUnhandledExceptionFilter(CrashHandler);

	// We don't need COM, but some plugins do and don't initialize it themselves.
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	OPENMPT_NAMESPACE::PluginBridge::InitializeStaticVariables();
	uint32 parentProcessId = _ttoi(argv[1]);
	new OPENMPT_NAMESPACE::PluginBridge(argv[0], OpenProcess(SYNCHRONIZE, FALSE, parentProcessId));

	WaitForSingleObject(OPENMPT_NAMESPACE::BridgeInstanceCounter::m_sigQuit, INFINITE);
	return 0;
}


int WINAPI WinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*nCmdShow*/)
{
	int argc = 0;
	auto argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	return _tmain(argc, argv);
}


OPENMPT_NAMESPACE_BEGIN

Event BridgeInstanceCounter::m_sigQuit;
std::atomic<uint32> BridgeInstanceCounter::m_instanceCount = 0;

bool PluginBridge::m_fullMemDump = false;
thread_local int PluginBridge::m_inMessageStack = 0;

// This is kind of a back-up pointer in case we couldn't sneak our pointer into the AEffect struct yet.
// It always points to the last initialized PluginBridge object.
PluginBridge *PluginBridge::m_latestInstance = nullptr;
WNDCLASSEX PluginBridge::m_windowClass;
static const TCHAR WINDOWCLASSNAME[] = _T("OpenMPTPluginBridge");


// Initialize static stuff like the editor window class
void PluginBridge::InitializeStaticVariables()
{
	m_windowClass.cbSize = sizeof(WNDCLASSEX);
	m_windowClass.style = CS_HREDRAW | CS_VREDRAW;
	m_windowClass.lpfnWndProc = WindowProc;
	m_windowClass.cbClsExtra = 0;
	m_windowClass.cbWndExtra = 0;
	m_windowClass.hInstance = GetModuleHandle(nullptr);
	m_windowClass.hIcon = nullptr;
	m_windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	m_windowClass.hbrBackground = nullptr;
	m_windowClass.lpszMenuName = nullptr;
	m_windowClass.lpszClassName = WINDOWCLASSNAME;
	m_windowClass.hIconSm = nullptr;
	RegisterClassEx(&m_windowClass);

	BridgeInstanceCounter::m_sigQuit.Create();
}


BridgeInstanceCounter::BridgeInstanceCounter()
{
	++m_instanceCount;
}

BridgeInstanceCounter::~BridgeInstanceCounter()
{
	if((--m_instanceCount) == 0)
		m_sigQuit.Trigger();
}


PluginBridge::PluginBridge(const wchar_t *memName, HANDLE otherProcess)
{
	PluginBridge::m_latestInstance = this;

	if(!m_queueMem.Open(memName)
	   || !CreateSignals(memName))
	{
		MessageBox(nullptr, _T("Could not connect to OpenMPT."), _T("OpenMPT Plugin Bridge"), 0);
		delete this;
		return;
	}

	m_sharedMem = m_queueMem.Data<SharedMemLayout>();

	// Store parent process handle so that we can terminate the bridge process when OpenMPT closes (e.g. through a crash).
	m_otherProcess.DuplicateFrom(otherProcess);

	m_sigThreadExit.Create(true);
	m_otherThread = mpt::UnmanagedThreadMember<PluginBridge, &PluginBridge::AudioThread>(this);

	mpt::UnmanagedThreadMember<PluginBridge, &PluginBridge::MessageThread>(this);
}


void PluginBridge::MessageThread()
{
	const HANDLE objects[] = {m_sigToBridgeGui.send, m_otherProcess};
	DWORD result;
	do
	{
		// Wait for incoming messages, time out periodically for idle messages and window refresh
		result = WaitForMultipleObjects(CountOf(objects), objects, FALSE, 15);
		if(result == WAIT_OBJECT_0)
		{
			SetContext(m_sharedMem->guiThreadMessages, m_sigToHostGui, m_sigToBridgeGui);
			ParseNextMessage();
		}
		// Normally we would only need this block if there's an editor window, but the 4klang VSTi creates
		// its custom window in the message thread, and it will freeze if we don't dispatch messages here...
		MSG msg;
		while(::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
		if(result == WAIT_TIMEOUT && m_nativeEffect)
		{
			if(m_needIdle)
			{
				Dispatch(effIdle, 0, 0, nullptr, 0.0f);
				m_needIdle = false;
			}
			if(m_window)
			{
				Dispatch(effEditIdle, 0, 0, nullptr, 0.0f);
			}
		}
	} while(result != WAIT_OBJECT_0 + 1 && result != WAIT_FAILED && !m_closeInstance);

	if(!m_closeInstance)
	{
		BridgeMessage msg;
		msg.Dispatch(effClose, 0, 0, 0, 0.0f, 0);
		DispatchToPlugin(&msg.dispatch);
	}
	ClearContext();
	SignalObjectAndWait(m_sigThreadExit, m_otherThread, INFINITE, FALSE);
	delete this;
}


// Send an arbitrary message to the host.
// Returns a pointer to the message, as processed by the host.
bool PluginBridge::SendToHost(BridgeMessage &sendMsg)
{
	if(!m_inMessageStack)
	{
		// We are coming from nowhere! This means the plugin must be sending a dispatch call that is not a reply to one of our own calls, so route it to the host's message thread
		SetContext(m_sharedMem->msgThreadMessages, m_sigToHostMsgThread, m_sigToBridgeMsgThread);
	}
	const auto [msgStack, signalToHost, signalToBridge] = Context();

	BridgeMessage *addr = CopyToSharedMemory(sendMsg, msgStack);
	if(addr == nullptr)
		return false;

	signalToHost.Send();

	// Wait until we get the result from the host.
	DWORD result;
	const HANDLE objects[] = {signalToHost.ack, signalToBridge.send, m_otherProcess};
	do
	{
		result = WaitForMultipleObjects(CountOf(objects), objects, FALSE, INFINITE);
		if(result == WAIT_OBJECT_0)
		{
			// Message got answered
			MPT_ASSERT(InterlockedAdd(&addr->header.status, 0) == MsgHeader::done);
			addr->CopyFromSharedMemory(sendMsg);
			InterlockedDecrement(&msgStack.readPos);
			InterlockedDecrement(&msgStack.writePos);
			break;
		} else if(result == WAIT_OBJECT_0 + 1)
		{
			ParseNextMessage();
		}
	} while(result != WAIT_OBJECT_0 + 2 && result != WAIT_FAILED);

	return (result == WAIT_OBJECT_0);
}


// Copy AEffect to shared memory.
void PluginBridge::UpdateEffectStruct()
{
	if(m_nativeEffect == nullptr)
		return;
	else if(m_otherPtrSize == 4)
		m_sharedMem->effect32.FromNative(*m_nativeEffect);
	else if(m_otherPtrSize == 8)
		m_sharedMem->effect64.FromNative(*m_nativeEffect);
	else
		MPT_ASSERT(false);
}


// Create the memory-mapped file containing the processing message and audio buffers
void PluginBridge::CreateProcessingFile(std::vector<char> &dispatchData)
{
	static uint32 plugId = 0;
	wchar_t mapName[64];
	swprintf(mapName, CountOf(mapName), L"Local\\openmpt-%u-%u", GetCurrentProcessId(), plugId++);

	PushToVector(dispatchData, mapName[0], sizeof(mapName));

	if(!m_processMem.Create(mapName, sizeof(ProcessMsg) + m_mixBufSize * (m_nativeEffect->numInputs + m_nativeEffect->numOutputs) * sizeof(double)))
	{
		SendErrorMessage(L"Could not initialize plugin bridge audio memory.");
		return;
	}
}


// Receive a message from the host and translate it.
void PluginBridge::ParseNextMessage()
{
	m_inMessageStack++;
	const auto [msgStack, signalToHost, signalToBridge] = Context();
	auto &msg = msgStack.ReadNext();
	// In case another thread manage to fire the signal before the thread that wrote the first message finished... we will be there soon!
	while(InterlockedCompareExchange(&msg.header.status, MsgHeader::received, MsgHeader::sent) != MsgHeader::sent)
		;
	switch(msg.header.type)
	{
	case MsgHeader::newInstance:
		NewInstance(&msg.newInstance);
		break;
	case MsgHeader::init:
		InitBridge(&msg.init);
		break;
	case MsgHeader::dispatch:
		DispatchToPlugin(&msg.dispatch);
		break;
	case MsgHeader::setParameter:
		SetParameter(&msg.parameter);
		break;
	case MsgHeader::getParameter:
		GetParameter(&msg.parameter);
		break;

	case MsgHeader::automate:
		AutomateParameters();
		break;
	}

	InterlockedExchange(&msg.header.status, MsgHeader::done);
	m_inMessageStack--;
	signalToBridge.Confirm();
}


// Create a new bridge instance within this one (creates a new thread).
void PluginBridge::NewInstance(NewInstanceMsg *msg)
{
	msg->memName[CountOf(msg->memName) - 1] = 0;
	new PluginBridge(msg->memName, m_otherProcess);
}


// Load the plugin.
void PluginBridge::InitBridge(InitMsg *msg)
{
	m_otherPtrSize = msg->hostPtrSize;
	m_mixBufSize = msg->mixBufSize;
	m_fullMemDump = msg->fullMemDump != 0;
	msg->result = 0;
	msg->str[CountOf(msg->str) - 1] = 0;

#ifdef _CONSOLE
	SetConsoleTitleW(msg->str);
#endif

	m_nativeEffect = nullptr;
	__try
	{
		m_library = LoadLibraryW(msg->str);
	} __except(EXCEPTION_EXECUTE_HANDLER)
	{
		m_library = nullptr;
	}

	if(m_library == nullptr)
	{
		FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), msg->str, CountOf(msg->str), nullptr);
		return;
	}

	auto mainProc = (Vst::MainProc)GetProcAddress(m_library, "VSTPluginMain");
	if(mainProc == nullptr)
	{
		mainProc = (Vst::MainProc)GetProcAddress(m_library, "main");
	}

	if(mainProc != nullptr)
	{
		__try
		{
			m_nativeEffect = mainProc(MasterCallback);
		} __except(EXCEPTION_EXECUTE_HANDLER)
		{
			m_nativeEffect = nullptr;
		}
	}

	if(m_nativeEffect == nullptr || m_nativeEffect->dispatcher == nullptr || m_nativeEffect->magic != kEffectMagic)
	{
		FreeLibrary(m_library);
		m_library = nullptr;

		wcscpy(msg->str, L"File is not a valid plugin");
		m_closeInstance = true;
		return;
	}

	m_nativeEffect->reservedForHost1 = this;

	msg->result = 1;

	UpdateEffectStruct();

	// Init process buffer
	DispatchToHost(audioMasterVendorSpecific, kVendorOpenMPT, kUpdateProcessingBuffer, nullptr, 0.0f);
}


void PluginBridge::SendErrorMessage(const wchar_t *str)
{
	BridgeMessage msg;
	msg.Error(str);
	SendToHost(msg);
}


// Wrapper for VST dispatch call with structured exception handling.
static intptr_t DispatchSEH(AEffect *effect, VstOpcodeToPlugin opCode, int32 index, intptr_t value, void *ptr, float opt, bool &exception)
{
	__try
	{
		if(effect->dispatcher != nullptr)
		{
			return effect->dispatcher(effect, opCode, index, value, ptr, opt);
		}
	} __except(EXCEPTION_EXECUTE_HANDLER)
	{
		exception = true;
	}
	return 0;
}


// Host-to-plugin opcode dispatcher
void PluginBridge::DispatchToPlugin(DispatchMsg *msg)
{
	if(m_nativeEffect == nullptr)
	{
		return;
	}

	// Various dispatch data - depending on the opcode, one of those might be used.
	std::vector<char> extraData;
	size_t extraDataSize = 0;

	MappedMemory auxMem;

	// Content of ptr is usually stored right after the message header, ptr field indicates size.
	void *ptr = (msg->ptr != 0) ? (msg + 1) : nullptr;
	if(msg->size > sizeof(BridgeMessage))
	{
		if(!auxMem.Open(static_cast<const wchar_t *>(ptr)))
		{
			return;
		}
		ptr = auxMem.Data();
	}
	void *origPtr = ptr;

	switch(msg->opcode)
	{
	case effGetProgramName:
	case effGetParamLabel:
	case effGetParamDisplay:
	case effGetParamName:
	case effString2Parameter:
	case effGetProgramNameIndexed:
	case effGetEffectName:
	case effGetErrorText:
	case effGetVendorString:
	case effGetProductString:
	case effShellGetNextPlugin:
		// Name in [ptr]
		extraDataSize = 256;
		break;

	case effMainsChanged:
		// [value]: 0 means "turn off", 1 means "turn on"
		SetThreadPriority(m_otherThread, msg->value ? THREAD_PRIORITY_ABOVE_NORMAL : THREAD_PRIORITY_NORMAL);
		m_sharedMem->tailSize = static_cast<int32>(Dispatch(effGetTailSize, 0, 0, nullptr, 0.0f));
		break;

	case effEditGetRect:
		// ERect** in [ptr]
		extraDataSize = sizeof(void *);
		break;

	case effEditOpen:
		// HWND in [ptr] - Note: Window handles are interoperable between 32-bit and 64-bit applications in Windows (http://msdn.microsoft.com/en-us/library/windows/desktop/aa384203%28v=vs.85%29.aspx)
		{
			TCHAR str[_MAX_PATH];
			GetModuleFileName(m_library, str, CountOf(str));

			m_windowParent = reinterpret_cast<HWND>(msg->ptr);
			ptr = m_window = CreateWindow(
			    WINDOWCLASSNAME,
			    str,
			    WS_POPUP,
			    CW_USEDEFAULT, CW_USEDEFAULT,
			    1, 1,
			    nullptr,
			    nullptr,
			    m_windowClass.hInstance,
			    nullptr);
		}
		break;

	case effGetChunk:
		// void** in [ptr] for chunk data address
		extraDataSize = sizeof(void *);
		break;

	case effProcessEvents:
		// VstEvents* in [ptr]
		TranslateBridgeToVSTEvents(m_eventCache, ptr);
		ptr = m_eventCache.data();
		break;

	case effOfflineNotify:
		// VstAudioFile* in [ptr]
		extraData.resize(sizeof(VstAudioFile *) * static_cast<size_t>(msg->value));
		ptr = extraData.data();
		for(int64 i = 0; i < msg->value; i++)
		{
			// TODO create pointers
		}
		break;

	case effOfflinePrepare:
	case effOfflineRun:
		// VstOfflineTask* in [ptr]
		extraData.resize(sizeof(VstOfflineTask *) * static_cast<size_t>(msg->value));
		ptr = extraData.data();
		for(int64 i = 0; i < msg->value; i++)
		{
			// TODO create pointers
		}
		break;

	case effSetSpeakerArrangement:
	case effGetSpeakerArrangement:
		// VstSpeakerArrangement* in [value] and [ptr]
		msg->value = reinterpret_cast<int64>(ptr) + sizeof(VstSpeakerArrangement);
		break;

	case effVendorSpecific:
		// Let's implement some custom opcodes!
		if(msg->index == kVendorOpenMPT)
		{
			msg->result = 1;
			switch(msg->value)
			{
			case kUpdateEffectStruct:
				UpdateEffectStruct();
				break;
			case kUpdateEventMemName:
				m_eventMem.Open(static_cast<const wchar_t *>(ptr));
				break;
			case kCacheProgramNames:
			{
				int32 progMin = static_cast<const int32 *>(ptr)[0];
				int32 progMax = static_cast<const int32 *>(ptr)[1];
				char *name = static_cast<char *>(ptr);
				for(int32 i = progMin; i < progMax; i++)
				{
					strcpy(name, "");
					if(m_nativeEffect->numPrograms <= 0 || Dispatch(effGetProgramNameIndexed, i, -1, name, 0) != 1)
					{
						// Fallback: Try to get current program name.
						strcpy(name, "");
						int32 curProg = static_cast<int32>(Dispatch(effGetProgram, 0, 0, nullptr, 0.0f));
						if(i != curProg)
						{
							Dispatch(effSetProgram, 0, i, nullptr, 0.0f);
						}
						Dispatch(effGetProgramName, 0, 0, name, 0);
						if(i != curProg)
						{
							Dispatch(effSetProgram, 0, curProg, nullptr, 0.0f);
						}
					}
					name[kCachedProgramNameLength - 1] = '\0';
					name += kCachedProgramNameLength;
				}
				break;
			}
			case kCacheParameterInfo:
			{
				int32 paramMin = static_cast<const int32 *>(ptr)[0];
				int32 paramMax = static_cast<const int32 *>(ptr)[1];
				ParameterInfo *param = static_cast<ParameterInfo *>(ptr);
				for(int32 i = paramMin; i < paramMax; i++, param++)
				{
					strcpy(param->name, "");
					strcpy(param->label, "");
					strcpy(param->display, "");
					Dispatch(effGetParamName, i, 0, param->name, 0.0f);
					Dispatch(effGetParamLabel, i, 0, param->label, 0.0f);
					Dispatch(effGetParamDisplay, i, 0, param->display, 0.0f);
					param->name[CountOf(param->label) - 1] = '\0';
					param->label[CountOf(param->label) - 1] = '\0';
					param->display[CountOf(param->display) - 1] = '\0';

					if(Dispatch(effGetParameterProperties, i, 0, &param->props, 0.0f) != 1)
					{
						memset(&param->props, 0, sizeof(param->props));
						strncpy(param->props.label, param->name, CountOf(param->props.label));
					}
				}
				break;
			}
			default:
				msg->result = 0;
			}
			return;
		}
		break;
	}

	if(extraDataSize != 0)
	{
		extraData.resize(extraDataSize, 0);
		ptr = extraData.data();
	}

	//std::cout << "about to dispatch " << msg->opcode << " to effect...";
	//std::flush(std::cout);
	bool exception = false;
	msg->result = static_cast<int32>(DispatchSEH(m_nativeEffect, static_cast<VstOpcodeToPlugin>(msg->opcode), msg->index, static_cast<intptr_t>(msg->value), ptr, msg->opt, exception));
	if(exception)
	{
		msg->type = MsgHeader::exceptionMsg;
		return;
	}
	//std::cout << "done" << std::endl;

	// Post-fix some opcodes
	switch(msg->opcode)
	{
	case effClose:
		m_nativeEffect = nullptr;
		FreeLibrary(m_library);
		m_library = nullptr;
		m_closeInstance = true;
		return;

	case effGetProgramName:
	case effGetParamLabel:
	case effGetParamDisplay:
	case effGetParamName:
	case effString2Parameter:
	case effGetProgramNameIndexed:
	case effGetEffectName:
	case effGetErrorText:
	case effGetVendorString:
	case effGetProductString:
	case effShellGetNextPlugin:
		// Name in [ptr]
		{
			extraData.back() = 0;
			char *dst = static_cast<char *>(origPtr);
			size_t length = static_cast<size_t>(msg->ptr - 1);
			strncpy(dst, extraData.data(), length);
			dst[length] = 0;
			break;
		}

	case effEditGetRect:
		// ERect** in [ptr]
		{
			ERect *rectPtr = *reinterpret_cast<ERect **>(extraData.data());
			if(rectPtr != nullptr)
			{
				MPT_ASSERT(static_cast<size_t>(msg->ptr) >= sizeof(ERect));
				std::memcpy(origPtr, rectPtr, std::min<size_t>(sizeof(ERect), static_cast<size_t>(msg->ptr)));
				m_windowWidth = rectPtr->right - rectPtr->left;
				m_windowHeight = rectPtr->bottom - rectPtr->top;

				// For plugins that don't know their size until after effEditOpen is done.
				if(m_window)
				{
					SetWindowPos(m_window, nullptr, 0, 0, m_windowWidth, m_windowHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
				}
			}
			break;
		}

	case effEditOpen:
		SetProp(m_window, _T("MPT"), this);
		SetParent(m_window, m_windowParent);
		ShowWindow(m_window, SW_SHOW);
		break;

	case effEditClose:
		DestroyWindow(m_window);
		m_window = nullptr;
		break;

	case effGetChunk:
		// void** in [ptr] for chunk data address
		if(m_getChunkMem.Create(static_cast<const wchar_t *>(origPtr), msg->result))
		{
			std::memcpy(m_getChunkMem.Data(), *reinterpret_cast<void **>(extraData.data()), msg->result);
		}
		break;
	}

	UpdateEffectStruct();  // Regularly update the struct
}


intptr_t PluginBridge::Dispatch(VstOpcodeToPlugin opcode, int32 index, intptr_t value, void *ptr, float opt)
{
	__try
	{
		return m_nativeEffect->dispatcher(m_nativeEffect, opcode, index, value, ptr, opt);
	} __except(EXCEPTION_EXECUTE_HANDLER)
	{
		SendErrorMessage(L"Exception in dispatch()!");
	}
	return 0;
}


// Set a plugin parameter.
void PluginBridge::SetParameter(ParameterMsg *msg)
{
	__try
	{
		m_nativeEffect->setParameter(m_nativeEffect, msg->index, msg->value);
	} __except(EXCEPTION_EXECUTE_HANDLER)
	{
		msg->type = MsgHeader::exceptionMsg;
	}
}


// Get a plugin parameter.
void PluginBridge::GetParameter(ParameterMsg *msg)
{
	__try
	{
		msg->value = m_nativeEffect->getParameter(m_nativeEffect, msg->index);
	} __except(EXCEPTION_EXECUTE_HANDLER)
	{
		msg->type = MsgHeader::exceptionMsg;
	}
}


// Execute received parameter automation messages
void PluginBridge::AutomateParameters()
{
	__try
	{
		const AutomationQueue::Parameter *param = m_sharedMem->automationQueue.params;
		const AutomationQueue::Parameter *paramEnd = param + std::min<LONG>(InterlockedExchange(&m_sharedMem->automationQueue.pendingEvents, 0), CountOf(m_sharedMem->automationQueue.params));
		while(param != paramEnd)
		{
			m_nativeEffect->setParameter(m_nativeEffect, param->index, param->value);
			param++;
		}
	} __except(EXCEPTION_EXECUTE_HANDLER)
	{
		SendErrorMessage(L"Exception in setParameter()!");
	}
}


// Audio rendering thread
void PluginBridge::AudioThread()
{
	SetContext(m_sharedMem->audioThreadMessages, m_sigToHostAudio, m_sigToBridgeAudio);

	const HANDLE objects[] = {m_sigProcessAudio.send, m_sigToBridgeAudio.send, m_sigThreadExit};
	DWORD result = 0;
	do
	{
		result = WaitForMultipleObjects(CountOf(objects), objects, FALSE, INFINITE);
		if(result == WAIT_OBJECT_0)
		{
			m_inMessageStack++;
			ProcessMsg *msg = m_processMem.Data<ProcessMsg>();
			m_isProcessing = kAboutToProcess;
			AutomateParameters();

			m_sharedMem->tailSize = static_cast<int32>(Dispatch(effGetTailSize, 0, 0, nullptr, 0.0f));

			// Prepare VstEvents.
			if(auto *events = m_eventMem.Data<int32>(); events != nullptr && *events != 0)
			{
				TranslateBridgeToVSTEvents(m_eventCache, events);
				*events = 0;
				Dispatch(effProcessEvents, 0, 0, m_eventCache.data(), 0.0f);
			}
			m_isProcessing = kProcessing;  // Now it's safe to overwrite shared event memory

			switch(msg->processType)
			{
			case ProcessMsg::process:
				Process();
				break;
			case ProcessMsg::processReplacing:
				ProcessReplacing();
				break;
			case ProcessMsg::processDoubleReplacing:
				ProcessDoubleReplacing();
				break;
			}

			m_isProcessing = kNotProcessing;
			m_sigProcessAudio.Confirm();
			m_inMessageStack--;
		} else if(result == WAIT_OBJECT_0 + 1)
		{
			ParseNextMessage();
		}
	} while(result != WAIT_OBJECT_0 + 2 && result != WAIT_FAILED);

	ClearContext();
}


// Process audio.
void PluginBridge::Process()
{
	if(m_nativeEffect->process)
	{
		float **inPointers, **outPointers;
		int32 sampleFrames = BuildProcessPointers(inPointers, outPointers);
		__try
		{
			m_nativeEffect->process(m_nativeEffect, inPointers, outPointers, sampleFrames);
		} __except(EXCEPTION_EXECUTE_HANDLER)
		{
			SendErrorMessage(L"Exception in process()!");
		}
	}
}


// Process audio.
void PluginBridge::ProcessReplacing()
{
	if(m_nativeEffect->processReplacing)
	{
		float **inPointers, **outPointers;
		int32 sampleFrames = BuildProcessPointers(inPointers, outPointers);
		__try
		{
			m_nativeEffect->processReplacing(m_nativeEffect, inPointers, outPointers, sampleFrames);
		} __except(EXCEPTION_EXECUTE_HANDLER)
		{
			SendErrorMessage(L"Exception in processReplacing()!");
		}
	}
}


// Process audio.
void PluginBridge::ProcessDoubleReplacing()
{
	if(m_nativeEffect->processDoubleReplacing)
	{
		double **inPointers, **outPointers;
		int32 sampleFrames = BuildProcessPointers(inPointers, outPointers);
		__try
		{
			m_nativeEffect->processDoubleReplacing(m_nativeEffect, inPointers, outPointers, sampleFrames);
		} __except(EXCEPTION_EXECUTE_HANDLER)
		{
			SendErrorMessage(L"Exception in processDoubleReplacing()!");
		}
	}
}


// Helper function to build the pointer arrays required by the VST process functions.
template <typename buf_t>
int32 PluginBridge::BuildProcessPointers(buf_t **(&inPointers), buf_t **(&outPointers))
{
	MPT_ASSERT(m_processMem.Good());
	ProcessMsg *msg = m_processMem.Data<ProcessMsg>();

	const size_t numPtrs = msg->numInputs + msg->numOutputs;
	m_sampleBuffers.resize(numPtrs, 0);

	if(numPtrs)
	{
		buf_t *offset = reinterpret_cast<buf_t *>(msg + 1);
		for(size_t i = 0; i < numPtrs; i++)
		{
			m_sampleBuffers[i] = offset;
			offset += msg->sampleFrames;
		}
		inPointers = reinterpret_cast<buf_t **>(m_sampleBuffers.data());
		outPointers = inPointers + msg->numInputs;
	}

	return msg->sampleFrames;
}


// Send a message to the host.
intptr_t PluginBridge::DispatchToHost(VstOpcodeToHost opcode, int32 index, intptr_t value, void *ptr, float opt)
{
	const auto processing = m_isProcessing.load();

	std::vector<char> dispatchData(sizeof(DispatchMsg), 0);
	int64 ptrOut = 0;
	char *ptrC = static_cast<char *>(ptr);

	switch(opcode)
	{
	case audioMasterAutomate:
	case audioMasterVersion:
	case audioMasterCurrentId:
	case audioMasterIdle:
	case audioMasterPinConnected:
		break;

	case audioMasterWantMidi:
		return 1;

	case audioMasterGetTime:
		// VstTimeInfo* in [return value]
		if(processing != kNotProcessing)
		{
			// During processing, read the cached time info. It won't change during the call
			// and we can save some valuable inter-process calls that way.
			return ToIntPtr<VstTimeInfo>(&m_sharedMem->timeInfo);
		}
		break;

	case audioMasterProcessEvents:
		// VstEvents* in [ptr]
		TranslateVSTEventsToBridge(dispatchData, static_cast<VstEvents *>(ptr), m_otherPtrSize);
		ptrOut = dispatchData.size() - sizeof(DispatchMsg);
		// If we are currently processing, try to return the events as part of the process call
		if(processing == kProcessing && m_eventMem.Good())
		{
			auto *memBytes = m_eventMem.Data<char>();
			auto *eventBytes = m_eventMem.Data<char>() + sizeof(int32);
			int32 &memNumEvents = *m_eventMem.Data<int32>();
			const auto memEventsSize = BridgeVSTEventsSize(memBytes);
			if(m_eventMem.Size() >= static_cast<size_t>(ptrOut) + memEventsSize)
			{
				// Enough shared memory for possibly pre-existing and new events; add new events at the end
				memNumEvents += static_cast<VstEvents *>(ptr)->numEvents;
				std::memcpy(eventBytes + memEventsSize, dispatchData.data() + sizeof(DispatchMsg) + sizeof(int32), static_cast<size_t>(ptrOut) - sizeof(int32));
				return 1;
			} else if(memNumEvents)
			{
				// Not enough memory; merge what we have and what we want to add so that it arrives in the correct order
				dispatchData.insert(dispatchData.begin() + sizeof(DispatchMsg) + sizeof(int32), eventBytes, eventBytes + memEventsSize);
				ptrOut += memEventsSize;
			}
		}
		break;

	case audioMasterSetTime:
	case audioMasterTempoAt:
	case audioMasterGetNumAutomatableParameters:
	case audioMasterGetParameterQuantization:
		break;

	case audioMasterVendorSpecific:
		if(index != kVendorOpenMPT || value != kUpdateProcessingBuffer)
		{
			if(ptr != 0)
			{
				// Cannot translate this.
				return 0;
			}
			break;
		}
		[[fallthrough]];
	case audioMasterIOChanged:
		// We need to be sure that the new values are known to the master.
		if(m_nativeEffect != nullptr)
		{
			UpdateEffectStruct();
			CreateProcessingFile(dispatchData);
			ptrOut = dispatchData.size() - sizeof(DispatchMsg);
		}
		break;

	case audioMasterNeedIdle:
		m_needIdle = true;
		return 1;

	case audioMasterSizeWindow:
		if(m_window)
		{
			SetWindowPos(m_window, nullptr, 0, 0, index, static_cast<int>(value), SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
		}
		break;

	case audioMasterGetSampleRate:
	case audioMasterGetBlockSize:
	case audioMasterGetInputLatency:
	case audioMasterGetOutputLatency:
		break;

	case audioMasterGetPreviousPlug:
	case audioMasterGetNextPlug:
		// Don't even bother, this would explode :)
		return 0;

	case audioMasterWillReplaceOrAccumulate:
	case audioMasterGetCurrentProcessLevel:
	case audioMasterGetAutomationState:
		break;

	case audioMasterOfflineStart:
	case audioMasterOfflineRead:
	case audioMasterOfflineWrite:
	case audioMasterOfflineGetCurrentPass:
	case audioMasterOfflineGetCurrentMetaPass:
		// TODO
		MPT_ASSERT(false);
		return 0;
		break;

	case audioMasterSetOutputSampleRate:
		break;

	case audioMasterGetOutputSpeakerArrangement:
	case audioMasterGetInputSpeakerArrangement:
		// VstSpeakerArrangement* in [return value]
		ptrOut = sizeof(VstSpeakerArrangement);
		break;

	case audioMasterGetVendorString:
	case audioMasterGetProductString:
		// Name in [ptr]
		ptrOut = 256;
		break;

	case audioMasterGetVendorVersion:
	case audioMasterSetIcon:
		break;

	case audioMasterCanDo:
		// Name in [ptr]
		ptrOut = strlen(ptrC) + 1;
		dispatchData.insert(dispatchData.end(), ptrC, ptrC + ptrOut);
		break;

	case audioMasterGetLanguage:
	case audioMasterOpenWindow:
	case audioMasterCloseWindow:
		break;

	case audioMasterGetDirectory:
		// Name in [return value]
		ptrOut = 256;
		break;

	case audioMasterUpdateDisplay:
	case audioMasterBeginEdit:
	case audioMasterEndEdit:
		break;

	case audioMasterOpenFileSelector:
	case audioMasterCloseFileSelector:
		// VstFileSelect* in [ptr]
		if(ptr != nullptr)
		{
			return VstFileSelector(opcode == audioMasterCloseFileSelector, *static_cast<VstFileSelect *>(ptr));
		}
		break;

	case audioMasterEditFile:
		break;

	case audioMasterGetChunkFile:
		// Name in [ptr]
		ptrOut = 256;
		dispatchData.insert(dispatchData.end(), ptrC, ptrC + ptrOut);
		break;

	default:
#ifdef MPT_BUILD_DEBUG
		if(ptr != nullptr)
			__debugbreak();
#endif
		break;
	}

	if(ptrOut != 0)
	{
		// In case we only reserve space and don't copy stuff over...
		dispatchData.resize(sizeof(DispatchMsg) + static_cast<size_t>(ptrOut), 0);
	}

	uint32 extraSize = static_cast<uint32>(dispatchData.size() - sizeof(DispatchMsg));

	// Create message header
	BridgeMessage *msg = reinterpret_cast<BridgeMessage *>(dispatchData.data());
	msg->Dispatch(opcode, index, value, ptrOut, opt, extraSize);

	const bool useAuxMem = dispatchData.size() > sizeof(BridgeMessage);
	MappedMemory auxMem;
	if(useAuxMem)
	{
		// Extra data doesn't fit in message - use secondary memory
		wchar_t auxMemName[64];
		static_assert(sizeof(DispatchMsg) + sizeof(auxMemName) <= sizeof(BridgeMessage), "Check message sizes, this will crash!");
		swprintf(auxMemName, CountOf(auxMemName), L"Local\\openmpt-%u-auxmem-%u", GetCurrentProcessId(), GetCurrentThreadId());
		if(auxMem.Create(auxMemName, extraSize))
		{
			// Move message data to shared memory and then move shared memory name to message data
			std::memcpy(auxMem.Data(), &dispatchData[sizeof(DispatchMsg)], extraSize);
			std::memcpy(&dispatchData[sizeof(DispatchMsg)], auxMemName, sizeof(auxMemName));
		} else
		{
			return 0;
		}
	}

	//std::cout << "about to dispatch " << opcode << " to host...";
	//std::flush(std::cout);
	if(!SendToHost(*msg))
	{
		return 0;
	}
	//std::cout << "done." << std::endl;
	const DispatchMsg *resultMsg = &msg->dispatch;

	const char *extraData = useAuxMem ? auxMem.Data<const char>() : reinterpret_cast<const char *>(resultMsg + 1);
	// Post-fix some opcodes
	switch(opcode)
	{
	case audioMasterGetTime:
		// VstTimeInfo* in [return value]
		return ToIntPtr<VstTimeInfo>(&m_sharedMem->timeInfo);

	case audioMasterGetOutputSpeakerArrangement:
	case audioMasterGetInputSpeakerArrangement:
		// VstSpeakerArrangement* in [return value]
		std::memcpy(&m_host2PlugMem.speakerArrangement, extraData, sizeof(VstSpeakerArrangement));
		return ToIntPtr<VstSpeakerArrangement>(&m_host2PlugMem.speakerArrangement);

	case audioMasterGetVendorString:
	case audioMasterGetProductString:
		// Name in [ptr]
		strcpy(ptrC, extraData);
		break;

	case audioMasterGetDirectory:
		// Name in [return value]
		strncpy(m_host2PlugMem.name, extraData, std::size(m_host2PlugMem.name) - 1);
		m_host2PlugMem.name[std::size(m_host2PlugMem.name) - 1] = 0;
		return ToIntPtr<char>(m_host2PlugMem.name);

	case audioMasterGetChunkFile:
		// Name in [ptr]
		strcpy(ptrC, extraData);
		break;
	}

	return static_cast<intptr_t>(resultMsg->result);
}


// Helper function for file selection dialog stuff.
// Note: This has been mostly copied from Vstplug.cpp. Ugly, but serializing this over the bridge would be even uglier.
intptr_t PluginBridge::VstFileSelector(bool destructor, VstFileSelect &fileSel)
{
	if(!destructor)
	{
		fileSel.returnMultiplePaths = nullptr;
		fileSel.numReturnPaths = 0;
		fileSel.reserved = 0;

		if(fileSel.command != kVstDirectorySelect)
		{
			// Plugin wants to load or save a file.
			std::string extensions, workingDir;
			for(int32 i = 0; i < fileSel.numFileTypes; i++)
			{
				const VstFileType &type = fileSel.fileTypes[i];
				extensions += type.name;
				extensions.push_back(0);
#if MPT_OS_WINDOWS
				extensions += "*.";
				extensions += type.dosType;
#elif MPT_OS_MACOSX_OR_IOS
				extensions += "*";
				extensions += type.macType;
#elif MPT_OS_GENERIC_UNIX
				extensions += "*.";
				extensions += type.unixType;
#else
#error Platform-specific code missing
#endif
				extensions.push_back(0);
			}
			extensions.push_back(0);

			if(fileSel.initialPath != nullptr)
			{
				workingDir = fileSel.initialPath;
			} else
			{
				// Plugins are probably looking for presets...?
				//workingDir = TrackerSettings::Instance().PathPluginPresets.GetWorkingDir();
			}

			std::string filenameBuffer(uint16_max, 0);

			const bool multiSelect = (fileSel.command == kVstMultipleFilesLoad);
			const bool load = (fileSel.command != kVstFileSave);
			OPENFILENAMEA ofn{};
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = m_window;
			ofn.hInstance = NULL;
			ofn.lpstrFilter = extensions.c_str();
			ofn.lpstrCustomFilter = NULL;
			ofn.nMaxCustFilter = 0;
			ofn.nFilterIndex = 0;
			ofn.lpstrFile = filenameBuffer.data();
			ofn.nMaxFile = static_cast<DWORD>(filenameBuffer.size());
			ofn.lpstrFileTitle = NULL;
			ofn.nMaxFileTitle = 0;
			ofn.lpstrInitialDir = workingDir.empty() ? nullptr : workingDir.c_str();
			ofn.lpstrTitle = NULL;
			ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_ENABLESIZING | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | (multiSelect ? OFN_ALLOWMULTISELECT : 0) | (load ? 0 : (OFN_OVERWRITEPROMPT | OFN_NOREADONLYRETURN));
			ofn.nFileOffset = 0;
			ofn.nFileExtension = 0;
			ofn.lpstrDefExt = NULL;
			ofn.lCustData = reinterpret_cast<LPARAM>(this);
			ofn.lpfnHook = NULL;
			ofn.lpTemplateName = NULL;
			ofn.pvReserved = NULL;
			ofn.dwReserved = 0;
			ofn.FlagsEx = 0;

			BOOL result = load ? GetOpenFileNameA(&ofn) : GetSaveFileNameA(&ofn);

			if(!result)
			{
				return 0;
			}

			if(multiSelect)
			{
				// Multiple files might have been selected
				size_t numFiles = 0;
				const char *currentFile = ofn.lpstrFile + ofn.nFileOffset;
				while(currentFile[0] != 0)
				{
					numFiles++;
					currentFile += lstrlenA(currentFile) + 1;
				}

				fileSel.numReturnPaths = static_cast<int32>(numFiles);
				fileSel.returnMultiplePaths = new(std::nothrow) char *[fileSel.numReturnPaths];

				currentFile = ofn.lpstrFile + ofn.nFileOffset;
				for(size_t i = 0; i < numFiles; i++)
				{
					int len = lstrlenA(currentFile);
					char *fname = new char[ofn.nFileOffset + len + 1];
					lstrcpyA(fname, ofn.lpstrFile);
					lstrcatA(fname, "\\");
					lstrcpyA(fname + ofn.nFileOffset, currentFile);
					fileSel.returnMultiplePaths[i] = fname;

					currentFile += len + 1;
				}
			} else
			{
				// Single path

				// VOPM doesn't initialize required information properly (it doesn't memset the struct to 0)...
				if(FourCC("VOPM") == m_nativeEffect->uniqueID)
				{
					fileSel.sizeReturnPath = _MAX_PATH;
				}

				if(fileSel.returnPath == nullptr || fileSel.sizeReturnPath == 0)
				{
					// Provide some memory for the return path.
					fileSel.sizeReturnPath = lstrlenA(ofn.lpstrFile) + 1;
					fileSel.returnPath = new(std::nothrow) char[fileSel.sizeReturnPath];
					if(fileSel.returnPath == nullptr)
					{
						return 0;
					}
					fileSel.returnPath[fileSel.sizeReturnPath - 1] = '\0';
					fileSel.reserved = 1;
				} else
				{
					fileSel.reserved = 0;
				}
				strncpy(fileSel.returnPath, ofn.lpstrFile, fileSel.sizeReturnPath - 1);
				fileSel.numReturnPaths = 1;
				fileSel.returnMultiplePaths = nullptr;
			}
			return 1;
		} else
		{
			// Plugin wants a directory
			CHAR path[MAX_PATH];

			BROWSEINFOA bi{};
			bi.hwndOwner = m_window;
			bi.lpszTitle = fileSel.title;
			bi.pszDisplayName = path;
			bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
			bi.lpfn = NULL;
			bi.lParam = NULL;
			LPITEMIDLIST pid = SHBrowseForFolderA(&bi);
			if(pid != nullptr && SHGetPathFromIDListA(pid, path))
			{
				if(Vst::FourCC("VSTr") == m_nativeEffect->uniqueID && fileSel.returnPath != nullptr && fileSel.sizeReturnPath == 0)
				{
					// old versions of reViSiT (which still relied on the host's file selection code) seem to be dodgy.
					// They report a path size of 0, but when using an own buffer, they will crash.
					// So we'll just assume that reViSiT can handle long enough (_MAX_PATH) paths here.
					fileSel.sizeReturnPath = lstrlenA(path) + 1;
					fileSel.returnPath[fileSel.sizeReturnPath - 1] = '\0';
				}
				if(fileSel.returnPath == nullptr || fileSel.sizeReturnPath == 0)
				{
					// Provide some memory for the return path.
					fileSel.sizeReturnPath = lstrlenA(path) + 1;
					fileSel.returnPath = new char[fileSel.sizeReturnPath];
					if(fileSel.returnPath == nullptr)
					{
						return 0;
					}
					fileSel.returnPath[fileSel.sizeReturnPath - 1] = '\0';
					fileSel.reserved = 1;
				} else
				{
					fileSel.reserved = 0;
				}
				strncpy(fileSel.returnPath, path, fileSel.sizeReturnPath - 1);
				fileSel.numReturnPaths = 1;
				return 1;
			}
			return 0;
		}
	} else
	{
		// Close file selector - delete allocated strings.
		if(fileSel.command == kVstMultipleFilesLoad && fileSel.returnMultiplePaths != nullptr)
		{
			for(int32 i = 0; i < fileSel.numReturnPaths; i++)
			{
				if(fileSel.returnMultiplePaths[i] != nullptr)
				{
					delete[] fileSel.returnMultiplePaths[i];
				}
			}
			delete[] fileSel.returnMultiplePaths;
			fileSel.returnMultiplePaths = nullptr;
		} else
		{
			if(fileSel.reserved == 1 && fileSel.returnPath != nullptr)
			{
				delete[] fileSel.returnPath;
				fileSel.returnPath = nullptr;
			}
		}
		return 1;
	}
}


// Helper function for sending messages to the host.
intptr_t VSTCALLBACK PluginBridge::MasterCallback(AEffect *effect, VstOpcodeToHost opcode, int32 index, intptr_t value, void *ptr, float opt)
{
	PluginBridge *instance = (effect != nullptr && effect->reservedForHost1 != nullptr) ? static_cast<PluginBridge *>(effect->reservedForHost1) : PluginBridge::m_latestInstance;
	return instance->DispatchToHost(opcode, index, value, ptr, opt);
}


LRESULT CALLBACK PluginBridge::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PluginBridge *that = reinterpret_cast<PluginBridge *>(GetProp(hwnd, _T("MPT")));
	if(that == nullptr)
	{
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	switch(uMsg)
	{
	case WM_NCACTIVATE:
		if(wParam == TRUE)
		{
			// Window is activated - put the plugin window into foreground
			SetWindowPos(GetParent(that->m_windowParent), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_ASYNCWINDOWPOS);
		}
		break;

	case WM_SYSKEYUP:
	case WM_SYSKEYDOWN:
	case WM_KEYUP:
	case WM_KEYDOWN:
		// Let the host handle these keys, too.
		// Allow focus stealing as the host might show a message box as a response to a key shortcut.
		AllowSetForegroundWindow(ASFW_ANY);
		PostMessage(GetParent(that->m_windowParent), uMsg, wParam, lParam);
		break;

	case WM_ERASEBKGND:
		// Pretend that we erased the background
		return 1;

	case WM_SIZE:
	{
		// For plugins that change their size but do not notify the host, e.g. Roland D-50
		RECT rect{0, 0, 0, 0};
		GetClientRect(hwnd, &rect);
		const int width = rect.right - rect.left, height = rect.bottom - rect.top;
		if(width > 0 && height > 0 && (width != that->m_windowWidth || height != that->m_windowHeight))
		{
			that->m_windowWidth = width;
			that->m_windowHeight = height;
			that->DispatchToHost(audioMasterSizeWindow, width, height, nullptr, 0.0f);
		}
		break;
	}

	case WM_ENABLE:
		// If the plugin tries to disable us (e.g. by showing a modal dialog), pass it on to the main window
		EnableWindow(GetAncestor(that->m_windowParent, GA_ROOTOWNER), wParam ? TRUE : FALSE);
		break;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


OPENMPT_NAMESPACE_END
