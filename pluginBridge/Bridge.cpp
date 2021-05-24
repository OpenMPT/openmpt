/*
 * Bridge.cpp
 * ----------
 * Purpose: VST plugin bridge (plugin side)
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


// TODO: Translate pointer-sized members in remaining structs: VstVariableIo, VstOfflineTask, VstAudioFile, VstWindow (all these are currently not supported by OpenMPT, so not urgent at all)

#include "openmpt/all/BuildSettings.hpp"
#include "../common/mptBaseMacros.h"
#include "../common/mptBaseTypes.h"
#include "../common/mptBaseUtils.h"
#include <Windows.h>
#include <ShellAPI.h>
#include <ShlObj.h>
#include <CommDlg.h>
#include <tchar.h>
#include <algorithm>
#include <string>

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
		const int ch = GetDateFormatW(LOCALE_SYSTEM_DEFAULT, 0, nullptr, L"'PluginBridge 'yyyy'-'MM'-'dd ", tempPath, mpt::saturate_cast<int>(std::size(tempPath)));
		if(ch)
			GetTimeFormatW(LOCALE_SYSTEM_DEFAULT, 0, nullptr, L"HH'.'mm'.'ss'.dmp'", tempPath + ch - 1, mpt::saturate_cast<int>(std::size(tempPath)) - ch + 1);
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
	// Note 1: Which plugins? This was added in r6459 on 2016-05-31 but with no remark whether it fixed a specific plugin,
	//         but the fix doesn't seem to make a lot of sense since back then no plugin code was ever running on the main thread.
	//         Could it have been for file dialogs, which were added a while before?
	// Note 2: M1 editor crashes if it runs on this thread and it was initialized with COINIT_MULTITHREADED
	const bool comInitialized = SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));

	OPENMPT_NAMESPACE::PluginBridge::MainLoop(argv);

	if(comInitialized)
		CoUninitialize();

	return 0;
}


int WINAPI WinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*nCmdShow*/)
{
	int argc = 0;
	auto argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	return _tmain(argc, argv);
}


OPENMPT_NAMESPACE_BEGIN

using namespace Vst;

std::vector<BridgeCommon *> BridgeCommon::m_plugins;
HWND BridgeCommon::m_communicationWindow = nullptr;
int BridgeCommon::m_instanceCount = 0;
thread_local bool BridgeCommon::m_isAudioThread = false;

// This is kind of a back-up pointer in case we couldn't sneak our pointer into the AEffect struct yet.
// It always points to the last initialized PluginBridge object.
PluginBridge *PluginBridge::m_latestInstance = nullptr;
ATOM PluginBridge::m_editorClassAtom = 0;
bool PluginBridge::m_fullMemDump = false;

void PluginBridge::MainLoop(TCHAR *argv[])
{
	WNDCLASSEX editorWndClass;
	editorWndClass.cbSize = sizeof(WNDCLASSEX);
	editorWndClass.style = CS_HREDRAW | CS_VREDRAW;
	editorWndClass.lpfnWndProc = WindowProc;
	editorWndClass.cbClsExtra = 0;
	editorWndClass.cbWndExtra = 0;
	editorWndClass.hInstance = GetModuleHandle(nullptr);
	editorWndClass.hIcon = nullptr;
	editorWndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	editorWndClass.hbrBackground = nullptr;
	editorWndClass.lpszMenuName = nullptr;
	editorWndClass.lpszClassName = _T("OpenMPTPluginBridgeEditor");
	editorWndClass.hIconSm = nullptr;
	m_editorClassAtom = RegisterClassEx(&editorWndClass);

	CreateCommunicationWindow(WindowProc);
	SetTimer(m_communicationWindow, TIMER_IDLE, 20, IdleTimerProc);

	uint32 parentProcessId = _ttoi(argv[1]);
	new PluginBridge(argv[0], OpenProcess(SYNCHRONIZE, FALSE, parentProcessId));

	MSG msg;
	while(::GetMessage(&msg, nullptr, 0, 0))
	{
		// Let host pre-process key messages like it does for non-bridged plugins
		if(msg.message >= WM_KEYFIRST && msg.message <= WM_KEYLAST)
		{
			HWND owner = nullptr;
			for(HWND hwnd = msg.hwnd; hwnd != nullptr; hwnd = GetParent(hwnd))
			{
				// Does it come from a child window? (e.g. Kirnu editor)
				if(GetClassWord(hwnd, GCW_ATOM) == m_editorClassAtom)
				{
					owner = GetParent(GetParent(hwnd));
					break;
				}
				// Does the message come from a top-level window? This is required e.g. for the slider pop-up windows and patch browser in Synth1.
				if(!(GetWindowLong(hwnd, GWL_STYLE) & WS_CHILD))
				{
					owner = GetWindow(hwnd, GW_OWNER);
					break;
				}
			}
			// Send to top-level VST editor window in host
			if(owner && SendMessage(owner, msg.message + WM_BRIDGE_KEYFIRST - WM_KEYFIRST, msg.wParam, msg.lParam))
				continue;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	DestroyWindow(m_communicationWindow);
}


PluginBridge::PluginBridge(const wchar_t *memName, HANDLE otherProcess)
{
	PluginBridge::m_latestInstance = this;

	m_thisPluginID = static_cast<int32>(m_plugins.size());
	m_plugins.push_back(this);

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
	DWORD dummy = 0;	// For Win9x
	m_audioThread = CreateThread(NULL, 0, &PluginBridge::AudioThread, this, 0, &dummy);

	m_sharedMem->bridgeCommWindow = m_communicationWindow;
	m_sharedMem->bridgePluginID = m_thisPluginID;
	m_sigBridgeReady.Trigger();
}


PluginBridge::~PluginBridge()
{
	SignalObjectAndWait(m_sigThreadExit, m_audioThread, INFINITE, FALSE);
	CloseHandle(m_audioThread);
	BridgeMessage dispatchMsg;
	dispatchMsg.Dispatch(effClose, 0, 0, 0, 0.0f, 0);
	DispatchToPlugin(dispatchMsg.dispatch);
	m_plugins[m_thisPluginID] = nullptr;
	if(m_instanceCount == 1)
		PostQuitMessage(0);
}


void PluginBridge::RequestDelete()
{
	PostMessage(m_communicationWindow, WM_BRIDGE_DELETE_PLUGIN, m_thisPluginID, 0);
}


// Send an arbitrary message to the host.
// Returns true if the message was processed by the host.
bool PluginBridge::SendToHost(BridgeMessage &sendMsg)
{
	auto &messages = m_sharedMem->ipcMessages;
	const auto msgID = CopyToSharedMemory(sendMsg, messages);
	if(msgID < 0)
		return false;
	BridgeMessage &sharedMsg = messages[msgID];

	if(!m_isAudioThread)
	{
		if(SendMessage(m_sharedMem->hostCommWindow, WM_BRIDGE_MESSAGE_TO_HOST, m_otherPluginID, msgID) == WM_BRIDGE_SUCCESS)
		{
			sharedMsg.CopyTo(sendMsg);
			return true;
		}
		return false;
	}

	// Audio thread: Use signals instead of window messages
	m_sharedMem->audioThreadToHostMsgID = msgID;
	m_sigToHostAudio.Send();

	// Wait until we get the result from the host.
	DWORD result;
	const HANDLE objects[] = {m_sigToHostAudio.confirm, m_sigToBridgeAudio.send, m_otherProcess};
	do
	{
		result = WaitForMultipleObjects(mpt::saturate_cast<DWORD>(std::size(objects)), objects, FALSE, INFINITE);
		if(result == WAIT_OBJECT_0)
		{
			// Message got answered
			sharedMsg.CopyTo(sendMsg);
			break;
		} else if(result == WAIT_OBJECT_0 + 1)
		{
			ParseNextMessage(m_sharedMem->audioThreadToBridgeMsgID);
			m_sigToBridgeAudio.Confirm();
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
	swprintf(mapName, std::size(mapName), L"Local\\openmpt-%u-%u", GetCurrentProcessId(), plugId++);

	PushToVector(dispatchData, mapName[0], sizeof(mapName));

	if(!m_processMem.Create(mapName, sizeof(ProcessMsg) + m_mixBufSize * (m_nativeEffect->numInputs + m_nativeEffect->numOutputs) * sizeof(double)))
	{
		SendErrorMessage(L"Could not initialize plugin bridge audio memory.");
		return;
	}
}


// Receive a message from the host and translate it.
void PluginBridge::ParseNextMessage(int msgID)
{
	auto &msg = m_sharedMem->ipcMessages[msgID];
	switch(msg.header.type)
	{
	case MsgHeader::newInstance:
		NewInstance(msg.newInstance);
		break;
	case MsgHeader::init:
		InitBridge(msg.init);
		break;
	case MsgHeader::dispatch:
		DispatchToPlugin(msg.dispatch);
		break;
	case MsgHeader::setParameter:
		SetParameter(msg.parameter);
		break;
	case MsgHeader::getParameter:
		GetParameter(msg.parameter);
		break;
	case MsgHeader::automate:
		AutomateParameters();
		break;
	}
}


// Create a new bridge instance within this one (creates a new thread).
void PluginBridge::NewInstance(NewInstanceMsg &msg)
{
	msg.memName[mpt::array_size<decltype(msg.memName)>::size - 1] = 0;
	new PluginBridge(msg.memName, m_otherProcess);
}


// Load the plugin.
void PluginBridge::InitBridge(InitMsg &msg)
{
	m_otherPtrSize = msg.hostPtrSize;
	m_mixBufSize = msg.mixBufSize;
	m_otherPluginID = msg.pluginID;
	m_fullMemDump = msg.fullMemDump != 0;
	msg.result = 0;
	msg.str[mpt::array_size<decltype(msg.str)>::size - 1] = 0;

#ifdef _CONSOLE
	SetConsoleTitleW(msg->str);
#endif

	m_nativeEffect = nullptr;
	__try
	{
		m_library = LoadLibraryW(msg.str);
	} __except(EXCEPTION_EXECUTE_HANDLER)
	{
		m_library = nullptr;
	}

	if(m_library == nullptr)
	{
		FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), msg.str, mpt::saturate_cast<DWORD>(std::size(msg.str)), nullptr);
		RequestDelete();
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

		wcscpy(msg.str, L"File is not a valid plugin");
		RequestDelete();
		return;
	}

	m_nativeEffect->reservedForHost1 = this;

	msg.result = 1;

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
void PluginBridge::DispatchToPlugin(DispatchMsg &msg)
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
	void *ptr = (msg.ptr != 0) ? (&msg + 1) : nullptr;
	if(msg.size > sizeof(BridgeMessage))
	{
		if(!auxMem.Open(static_cast<const wchar_t *>(ptr)))
		{
			return;
		}
		ptr = auxMem.Data();
	}
	void *origPtr = ptr;

	switch(msg.opcode)
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
		::SetThreadPriority(m_audioThread, msg.value ? THREAD_PRIORITY_ABOVE_NORMAL : THREAD_PRIORITY_NORMAL);
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
			GetModuleFileName(m_library, str, mpt::saturate_cast<DWORD>(std::size(str)));

			const auto parentWindow = reinterpret_cast<HWND>(msg.ptr);
			ptr = m_window = CreateWindow(
			    MAKEINTATOM(m_editorClassAtom),
			    str,
			    WS_CHILD | WS_VISIBLE,
			    CW_USEDEFAULT, CW_USEDEFAULT,
			    1, 1,
			    parentWindow,
			    nullptr,
			    GetModuleHandle(nullptr),
			    nullptr);
			SetWindowLongPtr(m_window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
		}
		break;

	case effGetChunk:
		// void** in [ptr] for chunk data address
		extraDataSize = sizeof(void *);
		break;

	case effProcessEvents:
		// VstEvents* in [ptr]
		TranslateBridgeToVstEvents(m_eventCache, ptr);
		ptr = m_eventCache.data();
		break;

	case effOfflineNotify:
		// VstAudioFile* in [ptr]
		extraData.resize(sizeof(VstAudioFile *) * static_cast<size_t>(msg.value));
		ptr = extraData.data();
		for(int64 i = 0; i < msg.value; i++)
		{
			// TODO create pointers
		}
		break;

	case effOfflinePrepare:
	case effOfflineRun:
		// VstOfflineTask* in [ptr]
		extraData.resize(sizeof(VstOfflineTask *) * static_cast<size_t>(msg.value));
		ptr = extraData.data();
		for(int64 i = 0; i < msg.value; i++)
		{
			// TODO create pointers
		}
		break;

	case effSetSpeakerArrangement:
	case effGetSpeakerArrangement:
		// VstSpeakerArrangement* in [value] and [ptr]
		msg.value = reinterpret_cast<int64>(ptr) + sizeof(VstSpeakerArrangement);
		break;

	case effVendorSpecific:
		// Let's implement some custom opcodes!
		if(msg.index == kVendorOpenMPT)
		{
			msg.result = 1;
			switch(msg.value)
			{
			case kUpdateEffectStruct:
				UpdateEffectStruct();
				break;
			case kUpdateEventMemName:
				if(ptr)
					m_eventMem.Open(static_cast<const wchar_t *>(ptr));
				break;
			case kCacheProgramNames:
				if(ptr)
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
				}
				break;
			case kCacheParameterInfo:
				if(ptr)
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
						param->name[mpt::array_size<decltype(param->label)>::size - 1] = '\0';
						param->label[mpt::array_size<decltype(param->label)>::size - 1] = '\0';
						param->display[mpt::array_size<decltype(param->display)>::size - 1] = '\0';

						if(Dispatch(effGetParameterProperties, i, 0, &param->props, 0.0f) != 1)
						{
							memset(&param->props, 0, sizeof(param->props));
							strncpy(param->props.label, param->name, std::size(param->props.label));
						}
					}
				}
				break;
			case kBeginGetProgram:
				if(ptr)
				{
					int32 numParams = static_cast<int32>((msg.size - sizeof(DispatchMsg)) / sizeof(float));
					float *params = static_cast<float *>(ptr);
					for(int32 i = 0; i < numParams; i++)
					{
						params[i] = m_nativeEffect->getParameter(m_nativeEffect, i);
					}
				}
				break;
			default:
				msg.result = 0;
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

	//std::cout << "about to dispatch " << msg.opcode << " to effect...";
	//std::flush(std::cout);
	bool exception = false;
	msg.result = static_cast<int32>(DispatchSEH(m_nativeEffect, static_cast<VstOpcodeToPlugin>(msg.opcode), msg.index, static_cast<intptr_t>(msg.value), ptr, msg.opt, exception));
	if(exception && msg.opcode != effClose)
	{
		msg.type = MsgHeader::exceptionMsg;
		return;
	}
	//std::cout << "done" << std::endl;

	// Post-fix some opcodes
	switch(msg.opcode)
	{
	case effClose:
		m_nativeEffect = nullptr;
		FreeLibrary(m_library);
		m_library = nullptr;
		RequestDelete();
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
			size_t length = static_cast<size_t>(msg.ptr - 1);
			strncpy(dst, extraData.data(), length);
			dst[length] = 0;
			break;
		}

	case effEditGetRect:
		// ERect** in [ptr]
		{
			ERect *rectPtr = *reinterpret_cast<ERect **>(extraData.data());
			if(rectPtr != nullptr && origPtr != nullptr)
			{
				MPT_ASSERT(static_cast<size_t>(msg.ptr) >= sizeof(ERect));
				std::memcpy(origPtr, rectPtr, std::min(sizeof(ERect), static_cast<size_t>(msg.ptr)));
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

	case effEditClose:
		DestroyWindow(m_window);
		m_window = nullptr;
		break;

	case effGetChunk:
		// void** in [ptr] for chunk data address
		if(m_getChunkMem.Create(static_cast<const wchar_t *>(origPtr), msg.result))
		{
			std::memcpy(m_getChunkMem.Data(), *reinterpret_cast<void **>(extraData.data()), msg.result);
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
void PluginBridge::SetParameter(ParameterMsg &msg)
{
	__try
	{
		m_nativeEffect->setParameter(m_nativeEffect, msg.index, msg.value);
	} __except(EXCEPTION_EXECUTE_HANDLER)
	{
		msg.type = MsgHeader::exceptionMsg;
	}
}


// Get a plugin parameter.
void PluginBridge::GetParameter(ParameterMsg &msg)
{
	__try
	{
		msg.value = m_nativeEffect->getParameter(m_nativeEffect, msg.index);
	} __except(EXCEPTION_EXECUTE_HANDLER)
	{
		msg.type = MsgHeader::exceptionMsg;
	}
}


// Execute received parameter automation messages
void PluginBridge::AutomateParameters()
{
	__try
	{
		const AutomationQueue::Parameter *param = m_sharedMem->automationQueue.params;
		const AutomationQueue::Parameter *paramEnd = param + std::min(m_sharedMem->automationQueue.pendingEvents.exchange(0), static_cast<int32>(std::size(m_sharedMem->automationQueue.params)));
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
DWORD WINAPI PluginBridge::AudioThread(LPVOID param)
{
	static_cast<PluginBridge*>(param)->AudioThread();
	return 0;
}
void PluginBridge::AudioThread()
{
	m_isAudioThread = true;

	const HANDLE objects[] = {m_sigProcessAudio.send, m_sigToBridgeAudio.send, m_sigThreadExit, m_otherProcess};
	DWORD result = 0;
	do
	{
		result = WaitForMultipleObjects(mpt::saturate_cast<DWORD>(std::size(objects)), objects, FALSE, INFINITE);
		if(result == WAIT_OBJECT_0)
		{
			ProcessMsg *msg = m_processMem.Data<ProcessMsg>();
			AutomateParameters();

			m_sharedMem->tailSize = static_cast<int32>(Dispatch(effGetTailSize, 0, 0, nullptr, 0.0f));
			m_isProcessing = true;

			// Prepare VstEvents.
			if(auto *events = m_eventMem.Data<int32>(); events != nullptr && *events != 0)
			{
				TranslateBridgeToVstEvents(m_eventCache, events);
				*events = 0;
				Dispatch(effProcessEvents, 0, 0, m_eventCache.data(), 0.0f);
			}

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

			m_isProcessing = false;
			m_sigProcessAudio.Confirm();
		} else if(result == WAIT_OBJECT_0 + 1)
		{
			ParseNextMessage(m_sharedMem->audioThreadToBridgeMsgID);
			m_sigToBridgeAudio.Confirm();
		} else if(result == WAIT_OBJECT_0 + 2)
		{
			// Main thread asked for termination
			break;
		} else if(result == WAIT_OBJECT_0 + 3)
		{
			// Host process died
			RequestDelete();
			break;
		}
	} while(result != WAIT_FAILED);
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
	ProcessMsg &msg = *m_processMem.Data<ProcessMsg>();

	const size_t numPtrs = msg.numInputs + msg.numOutputs;
	m_sampleBuffers.resize(numPtrs, 0);

	if(numPtrs)
	{
		buf_t *offset = reinterpret_cast<buf_t *>(&msg + 1);
		for(size_t i = 0; i < numPtrs; i++)
		{
			m_sampleBuffers[i] = offset;
			offset += msg.sampleFrames;
		}
		inPointers = reinterpret_cast<buf_t **>(m_sampleBuffers.data());
		outPointers = inPointers + msg.numInputs;
	}

	return msg.sampleFrames;
}


// Send a message to the host.
intptr_t PluginBridge::DispatchToHost(VstOpcodeToHost opcode, int32 index, intptr_t value, void *ptr, float opt)
{
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
		if(m_isProcessing)
		{
			// During processing, read the cached time info. It won't change during the call
			// and we can save some valuable inter-process calls that way.
			return ToIntPtr<VstTimeInfo>(&m_sharedMem->timeInfo);
		}
		break;

	case audioMasterProcessEvents:
		// VstEvents* in [ptr]
		if(ptr == nullptr)
			return 0;
		TranslateVstEventsToBridge(dispatchData, *static_cast<VstEvents *>(ptr), m_otherPtrSize);
		ptrOut = dispatchData.size() - sizeof(DispatchMsg);
		// If we are currently processing, try to return the events as part of the process call
		if(m_isAudioThread && m_isProcessing && m_eventMem.Good())
		{
			auto *memBytes = m_eventMem.Data<char>();
			auto *eventBytes = m_eventMem.Data<char>() + sizeof(int32);
			int32 &memNumEvents = *m_eventMem.Data<int32>();
			const auto memEventsSize = BridgeVstEventsSize(memBytes);
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
				*reinterpret_cast<int32 *>(dispatchData.data() + sizeof(DispatchMsg)) += memNumEvents;
				memNumEvents = 0;
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
		// Currently not supported in OpenMPT
		return 0;

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
		// VstFileSelect* in [ptr]
		if(ptr != nullptr)
		{
			auto &fileSel = *static_cast<VstFileSelect *>(ptr);
			fileSel.returnMultiplePaths = nullptr;
			fileSel.numReturnPaths = 0;
			TranslateVstFileSelectToBridge(dispatchData, fileSel, m_otherPtrSize);
			ptrOut = dispatchData.size() - sizeof(DispatchMsg) + 65536; // enough space for return paths
		}
		break;

	case audioMasterCloseFileSelector:
		// VstFileSelect* in [ptr]
		if(auto *fileSel = static_cast<VstFileSelect *>(ptr); fileSel != nullptr && fileSel->reserved == 1)
		{
			fileSel->returnPath = nullptr;
			fileSel->returnMultiplePaths = nullptr;
		}
		m_fileSelectCache.clear();
		m_fileSelectCache.shrink_to_fit();
		return 1;

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
		swprintf(auxMemName, std::size(auxMemName), L"Local\\openmpt-%u-auxmem-%u", GetCurrentProcessId(), GetCurrentThreadId());
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

	case audioMasterOpenFileSelector:
		if(resultMsg->result != 0 && ptr != nullptr)
		{
			TranslateBridgeToVstFileSelect(m_fileSelectCache, extraData, static_cast<size_t>(ptrOut));
			auto &fileSel = *static_cast<VstFileSelect *>(ptr);
			const auto &fileSelResult = *reinterpret_cast<const VstFileSelect *>(m_fileSelectCache.data());

			if((fileSel.command == kVstFileLoad || fileSel.command == kVstFileSave))
			{
				if(FourCC("VOPM") == m_nativeEffect->uniqueID)
				{
					fileSel.sizeReturnPath = _MAX_PATH;
				}
			} else if(fileSel.command == kVstDirectorySelect)
			{
				if(FourCC("VSTr") == m_nativeEffect->uniqueID && fileSel.returnPath != nullptr && fileSel.sizeReturnPath == 0)
				{
					// Old versions of reViSiT (which still relied on the host's file selector) seem to be dodgy.
					// They report a path size of 0, but when using an own buffer, they will crash.
					// So we'll just assume that reViSiT can handle long enough (_MAX_PATH) paths here.
					fileSel.sizeReturnPath = static_cast<int32>(strlen(fileSelResult.returnPath) + 1);
				}
			}
			fileSel.numReturnPaths = fileSelResult.numReturnPaths;
			fileSel.reserved = 1;
			if(fileSel.command == kVstMultipleFilesLoad)
			{
				fileSel.returnMultiplePaths = fileSelResult.returnMultiplePaths;
			} else if(fileSel.returnPath != nullptr && fileSel.sizeReturnPath != 0)
			{
				// Plugin provides memory
				const auto len = strnlen(fileSelResult.returnPath, fileSel.sizeReturnPath - 1);
				strncpy(fileSel.returnPath, fileSelResult.returnPath, len);
				fileSel.returnPath[len] = '\0';
				fileSel.reserved = 0;
			} else
			{
				fileSel.returnPath = fileSelResult.returnPath;
				fileSel.sizeReturnPath = fileSelResult.sizeReturnPath;
			}
		}
		break;

	case audioMasterGetChunkFile:
		// Name in [ptr]
		strcpy(ptrC, extraData);
		break;
	}

	return static_cast<intptr_t>(resultMsg->result);
}


// Helper function for sending messages to the host.
intptr_t VSTCALLBACK PluginBridge::MasterCallback(AEffect *effect, VstOpcodeToHost opcode, int32 index, intptr_t value, void *ptr, float opt)
{
	PluginBridge *instance = (effect != nullptr && effect->reservedForHost1 != nullptr) ? static_cast<PluginBridge *>(effect->reservedForHost1) : PluginBridge::m_latestInstance;
	return instance->DispatchToHost(opcode, index, value, ptr, opt);
}


LRESULT CALLBACK PluginBridge::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PluginBridge *that = nullptr;
	if(hwnd == m_communicationWindow && wParam < m_plugins.size())
		that = static_cast<PluginBridge *>(m_plugins[wParam]);
	else // Editor windows
		that = reinterpret_cast<PluginBridge *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

	if(that == nullptr)
		return DefWindowProc(hwnd, uMsg, wParam, lParam);

	switch(uMsg)
	{
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

	case WM_BRIDGE_MESSAGE_TO_BRIDGE:
		that->ParseNextMessage(static_cast<int>(lParam));
		return WM_BRIDGE_SUCCESS;

	case WM_BRIDGE_DELETE_PLUGIN:
		delete that;
		return 0;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


void PluginBridge::IdleTimerProc(HWND, UINT, UINT_PTR idTimer, DWORD)
{
	if(idTimer != TIMER_IDLE)
		return;
	for(auto *plugin : m_plugins)
	{
		auto *that = static_cast<PluginBridge *>(plugin);
		if(that == nullptr)
			continue;
		if(that->m_needIdle)
		{
			that->Dispatch(effIdle, 0, 0, nullptr, 0.0f);
			that->m_needIdle = false;
		}
		if(that->m_window)
		{
			that->Dispatch(effEditIdle, 0, 0, nullptr, 0.0f);
		}
	}
}


OPENMPT_NAMESPACE_END
