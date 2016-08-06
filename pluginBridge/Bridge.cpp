/*
 * Bridge.cpp
 * ----------
 * Purpose: VST plugin bridge (plugin side)
 * Notes  : (currently none)
 * Authors: Johannes Schultz (OpenMPT Devs)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


// TODO
// Translate VstIntPtr size in remaining structs!!! VstFileSelect, VstVariableIo, VstOfflineTask, VstAudioFile, VstWindow (all but VstFileSelect are currently not supported by OpenMPT)
// Optimize out audioMasterProcessEvents the same way as effProcessEvents?
// Find a nice solution for audioMasterIdle that doesn't break TAL-Elek7ro-II
// Kirnu and Combo F GUI deadlocks during playback

// Low priority:
// Re-enable DEP in OpenMPT?
// Clean up code :)

#ifndef MODPLUG_TRACKER
#define MODPLUG_TRACKER
#endif
#include "../common/BuildSettings.h"
#include "../common/typedefs.h"
#include <Windows.h>
#include <ShellAPI.h>
#include <ShlObj.h>
#include <CommDlg.h>
#include <tchar.h>
#include <algorithm>


//#include <cassert>
#ifdef _DEBUG
#include <intrin.h>
#undef assert
#define assert(x) while(!(x)) { ::MessageBoxA(NULL, #x, "Debug Assertion Failed", MB_ICONERROR);  __debugbreak(); break; }
#else
#define assert(x)
#endif

#include "../common/WriteMemoryDump.h"
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
		const int ch = GetDateFormatW(LOCALE_SYSTEM_DEFAULT, 0, nullptr, L"'PluginBridge 'yyyy'-'MM'-'dd ", tempPath, CountOf(tempPath));
		if(ch) GetTimeFormatW(LOCALE_SYSTEM_DEFAULT, 0, nullptr, L"HH'.'mm'.'ss'.dmp'", tempPath + ch - 1, CountOf(tempPath) - ch + 1);
		filename += tempPath;
		static_assert(CountOf(tempPath) > 3, "");

		OPENMPT_NAMESPACE::WriteMemoryDump(pExceptionInfo, filename.c_str(), OPENMPT_NAMESPACE::PluginBridge::fullMemDump);
	}

	// Let Windows handle the exception...
	return EXCEPTION_CONTINUE_SEARCH;
}


int _tmain(int argc, TCHAR *argv[])
{
	if(argc != 2)
	{
		MessageBox(NULL, _T("This executable is part of OpenMPT. You do not need to run it by yourself."), _T("OpenMPT Plugin Bridge"), 0);
		return -1;
	}

	::SetUnhandledExceptionFilter(CrashHandler);

	// We don't need COM, but some plugins do and don't initialize it themselves.
	CoInitializeEx(NULL, COINIT_MULTITHREADED);

	OPENMPT_NAMESPACE::PluginBridge::InitializeStaticVariables();
	uint32 parentProcessId = _ttoi(argv[1]);
	new OPENMPT_NAMESPACE::PluginBridge(argv[0], OpenProcess(SYNCHRONIZE, FALSE, parentProcessId));

	WaitForSingleObject(OPENMPT_NAMESPACE::PluginBridge::sigQuit, INFINITE);
	return 0;
}


int WINAPI WinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*nCmdShow*/)
{
	LPWSTR *argv;
	int argc;
	argv = CommandLineToArgvW(GetCommandLine(), &argc);
	return _tmain(argc, argv);
}


OPENMPT_NAMESPACE_BEGIN

LONG PluginBridge::instanceCount = 0;
Event PluginBridge::sigQuit;
bool PluginBridge::fullMemDump = false;

// This is kind of a back-up pointer in case we couldn't sneak our pointer into the AEffect struct yet.
// It always points to the last intialized PluginBridge object.
PluginBridge *PluginBridge::latestInstance = nullptr;
WNDCLASSEX PluginBridge::windowClass;
#define WINDOWCLASSNAME _T("OpenMPTPluginBridge")


// Initialize static stuff like the editor window class
void PluginBridge::InitializeStaticVariables()
{
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = WindowProc;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = GetModuleHandle(NULL);
	windowClass.hIcon = NULL;
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.hbrBackground  = NULL;
	windowClass.lpszMenuName = NULL;
	windowClass.lpszClassName = WINDOWCLASSNAME;
	windowClass.hIconSm = NULL;
	RegisterClassEx(&windowClass);

	sigQuit.Create();
}


PluginBridge::PluginBridge(const wchar_t *memName, HANDLE otherProcess_)
	: window(NULL), isProcessing(0), nativeEffect(nullptr), needIdle(false), closeInstance(false)
{
	PluginBridge::latestInstance = this;
	InterlockedIncrement(&instanceCount);

	if(!queueMem.Open(memName)
		|| !CreateSignals(memName))
	{
		MessageBox(NULL, _T("Could not connect to OpenMPT."), _T("OpenMPT Plugin Bridge"), 0);
		delete this;
		return;
	}

	sharedMem = reinterpret_cast<SharedMemLayout *>(queueMem.view);

	// Store parent process handle so that we can terminate the bridge process when OpenMPT closes (e.g. through a crash).
	otherProcess.DuplicateFrom(otherProcess_);

	sigThreadExit.Create(true);
	otherThread = mpt::UnmanagedThreadMember<PluginBridge, &PluginBridge::RenderThread>(this);

	mpt::UnmanagedThreadMember<PluginBridge, &PluginBridge::MessageThread>(this);
}


PluginBridge::~PluginBridge()
{
	SignalObjectAndWait(sigThreadExit, otherThread, INFINITE, FALSE);

	sharedMem = nullptr;
	queueMem.Close();
	processMem.Close();

	if(InterlockedDecrement(&instanceCount) == 0)
	{
		sigQuit.Trigger();
	}
}


void PluginBridge::MessageThread()
{
	msgThreadID = GetCurrentThreadId();

	const HANDLE objects[] = { sigToBridge.send, sigToHost.ack, otherProcess };
	DWORD result;
	do
	{
		// Wait for incoming messages, time out periodically for idle messages and window refresh
		result = WaitForMultipleObjects(CountOf(objects), objects, FALSE, 15);
		if(result == WAIT_OBJECT_0)
		{
			ParseNextMessage();
		} else if(result == WAIT_OBJECT_0 + 1)
		{
			// Message got answered
			for(size_t i = 0; i < CountOf(sharedMem->toHost); i++)
			{
				BridgeMessage &msg = sharedMem->toHost[i];
				if(InterlockedCompareExchange(&msg.header.status, MsgHeader::delivered, MsgHeader::done) == MsgHeader::done)
				{
					ackSignals[msg.header.signalID].Confirm();
				}
			}
		}
		if(needIdle && nativeEffect)
		{
			Dispatch(effIdle, 0, 0, nullptr, 0.0f);
			needIdle = false;
		}
		if(window)
		{
			Dispatch(effEditIdle, 0, 0, nullptr, 0.0f);
		}
		// Normally we would only need this block if there's an editor window, but the 4klang VSTi creates
		// its custom window in the message thread, and it will freeze if we don't dispatch messages here...
		MSG msg;
		while(::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	} while(result != WAIT_OBJECT_0 + 2 && result != WAIT_FAILED && !closeInstance);

	if(!closeInstance)
	{
		// Close any possible waiting queries
		for(size_t i = 0; i < CountOf(ackSignals); i++)
		{
			ackSignals[i].Send();
		}
		BridgeMessage msg;
		msg.Dispatch(effClose, 0, 0, 0, 0.0f, 0);
		DispatchToPlugin(&msg.dispatch);
	}
	delete this;
}


// Send an arbitrary message to the host.
// Returns a pointer to the message, as processed by the host.
bool PluginBridge::SendToHost(BridgeMessage &sendMsg)
{
	const bool inMsgThread = GetCurrentThreadId() == msgThreadID;
	BridgeMessage *addr = CopyToSharedMemory(sendMsg, sharedMem->toHost);
	if(addr == nullptr)
	{
		return false;
	}

	sigToHost.Send();

	// Wait until we get the result from the host.
	DWORD result;
	if(inMsgThread)
	{
		// Since this is the message thread, we must handle messages directly.
		const HANDLE objects[] = { sigToHost.ack, sigToBridge.send, otherProcess };
		do
		{
			result = WaitForMultipleObjects(CountOf(objects), objects, FALSE, INFINITE);
			if(result == WAIT_OBJECT_0)
			{
				// Some message got answered - is it our message?
				bool done = false;
				for(size_t i = 0; i < CountOf(sharedMem->toHost); i++)
				{
					BridgeMessage &msg = sharedMem->toHost[i];
					if(InterlockedCompareExchange(&msg.header.status, MsgHeader::delivered, MsgHeader::done) == MsgHeader::done)
					{
						if(&msg != addr)
						{
							ackSignals[msg.header.signalID].Confirm();
						} else
						{
							// This is our message!
							addr->CopyFromSharedMemory(sendMsg);
							done = true;
						}
					}
				}
				if(done)
				{
					break;
				}
			} else if(result == WAIT_OBJECT_0 + 1)
			{
				ParseNextMessage();
			}
		} while(result != WAIT_OBJECT_0 + 2 && result != WAIT_FAILED);
	} else
	{
		// Wait until the message thread notifies us.
		Signal &ackHandle = ackSignals[addr->header.signalID];
		const HANDLE objects[] = { ackHandle.ack, ackHandle.send, otherProcess };
		do
		{
			result = WaitForMultipleObjects(CountOf(objects), objects, FALSE, 100);
			if(result == WAIT_TIMEOUT)
			{
				MSG msg;
				while(::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
				{
					::TranslateMessage(&msg);
					::DispatchMessage(&msg);
				}
				continue;
			}
		} while (result == WAIT_TIMEOUT);
		addr->CopyFromSharedMemory(sendMsg);
	}

	return (result == WAIT_OBJECT_0);
}


// Copy AEffect to shared memory.
void PluginBridge::UpdateEffectStruct()
{
	if(nativeEffect == nullptr)
	{
		return;
	} else if(otherPtrSize == 4)
	{
		sharedMem->effect32.FromNative(*nativeEffect);
	} else if(otherPtrSize == 8)
	{
		sharedMem->effect64.FromNative(*nativeEffect);
	} else
	{
		assert(false);
	}
}


// Create the memory-mapped file containing the processing message and audio buffers
void PluginBridge::CreateProcessingFile(std::vector<char> &dispatchData)
{
	static uint32 plugId = 0;
	wchar_t mapName[64];
	swprintf(mapName, CountOf(mapName), L"Local\\openmpt-%d-%d", GetCurrentProcessId(), plugId++);

	PushToVector(dispatchData, mapName[0], sizeof(mapName));

	if(!processMem.Create(mapName, sizeof(ProcessMsg) + mixBufSize * (nativeEffect->numInputs + nativeEffect->numOutputs) * sizeof(double)))
	{
		SendErrorMessage(L"Could not initialize plugin bridge audio memory.");
		return;
	}
}


// Receive a message from the host and translate it.
void PluginBridge::ParseNextMessage()
{
	assert(GetCurrentThreadId() == msgThreadID);

	BridgeMessage *msg = &sharedMem->toBridge[0];
	for(size_t i = 0; i < CountOf(sharedMem->toBridge); i++, msg++)
	{
		if(InterlockedCompareExchange(&msg->header.status, MsgHeader::received, MsgHeader::sent) == MsgHeader::sent)
		{
			switch(msg->header.type)
			{
			case MsgHeader::newInstance:
				NewInstance(&msg->newInstance);
				break;
			case MsgHeader::init:
				InitBridge(&msg->init);
				break;
			case MsgHeader::dispatch:
				DispatchToPlugin(&msg->dispatch);
				break;
			case MsgHeader::setParameter:
				SetParameter(&msg->parameter);
				break;
			case MsgHeader::getParameter:
				GetParameter(&msg->parameter);
				break;

			case MsgHeader::automate:
				AutomateParameters();
				break;
			}

			InterlockedExchange(&msg->header.status, MsgHeader::done);
			sigToBridge.Confirm();
		}
	}
}


// Create a new bridge instance within this one (creates a new thread).
void PluginBridge::NewInstance(NewInstanceMsg *msg)
{
	msg->memName[CountOf(msg->memName) - 1] = 0;
	new PluginBridge(msg->memName, otherProcess);
}


// Load the plugin.
void PluginBridge::InitBridge(InitMsg *msg)
{
	otherPtrSize = msg->hostPtrSize;
	mixBufSize = msg->mixBufSize;
	fullMemDump = msg->fullMemDump != 0;
	msg->result = 0;
	msg->str[CountOf(msg->str) - 1] = 0;

#ifdef _CONSOLE
	SetConsoleTitleW(msg->str);
#endif

	nativeEffect = nullptr;
	__try
	{
		library = LoadLibraryW(msg->str);
	} __except(EXCEPTION_EXECUTE_HANDLER)
	{
		library = nullptr;
	}

	if(library == nullptr)
	{
		FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			msg->str,
			CountOf(msg->str),
			NULL);
		return;
	}

	typedef AEffect * (VSTCALLBACK * PVSTPLUGENTRY)(audioMasterCallback);
	PVSTPLUGENTRY pMainProc = (PVSTPLUGENTRY)GetProcAddress(library, "VSTPluginMain");
	if(pMainProc == nullptr)
	{
		pMainProc = (PVSTPLUGENTRY)GetProcAddress(library, "main");
	}

	if(pMainProc != nullptr)
	{
		__try
		{
			nativeEffect = pMainProc(MasterCallback);
		} __except(EXCEPTION_EXECUTE_HANDLER)
		{
			nativeEffect = nullptr;
		}
	}

	if(nativeEffect == nullptr || nativeEffect->dispatcher == nullptr || nativeEffect->magic != kEffectMagic)
	{
		FreeLibrary(library);
		library = nullptr;
		
		wcscpy(msg->str, L"File is not a valid plugin");
		closeInstance = true;
		return;
	}

	nativeEffect->resvd1 = ToVstPtr(this);

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
static VstIntPtr DispatchSEH(AEffect *effect, VstInt32 opCode, VstInt32 index, VstIntPtr value, void *ptr, float opt, bool &exception)
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
	if(nativeEffect == nullptr)
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
		ptr = auxMem.view;
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
		SetThreadPriority(otherThread, msg->value ? THREAD_PRIORITY_ABOVE_NORMAL : THREAD_PRIORITY_NORMAL);
		break;

	case effEditGetRect:
		// ERect** in [ptr]
		extraDataSize = sizeof(void *);
		break;

	case effEditOpen:
		// HWND in [ptr] - Note: Window handles are interoperable between 32-bit and 64-bit applications in Windows (http://msdn.microsoft.com/en-us/library/windows/desktop/aa384203%28v=vs.85%29.aspx)
		{
			TCHAR str[_MAX_PATH];
			GetModuleFileName(library, str, CountOf(str));

			windowParent = reinterpret_cast<HWND>(msg->ptr);
			ptr = window = CreateWindow(
				WINDOWCLASSNAME,
				str,
				WS_POPUP,
				CW_USEDEFAULT, CW_USEDEFAULT,
				1, 1,
				NULL,
				NULL,
				windowClass.hInstance,
				NULL);

			// Opening two shared KORG M1 editor instances makes the second instance crash the bridge if there is no parent at this point.
			SetParent(window, windowParent);
		}
		break;

	case effGetChunk:
		// void** in [ptr] for chunk data address
		extraDataSize = sizeof(void *);
		break;

	case effProcessEvents:
		// VstEvents* in [ptr]
		TranslateBridgeToVSTEvents(eventCache, ptr);
		ptr = &eventCache[0];

		break;

	case effOfflineNotify:
		// VstAudioFile* in [ptr]
		extraData.resize(sizeof(VstAudioFile *) * static_cast<size_t>(msg->value));
		ptr = &extraData[0];
		for(int64 i = 0; i < msg->value; i++)
		{
			// TODO create pointers
		}
		break;

	case effOfflinePrepare:
	case effOfflineRun:
		// VstOfflineTask* in [ptr]
		extraData.resize(sizeof(VstOfflineTask *) * static_cast<size_t>(msg->value));
		ptr = &extraData[0];
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
				eventMem.Open(static_cast<const wchar_t *>(ptr));
				break;
			case kCacheProgramNames:
				{
					int32 progMin = static_cast<const int32 *>(ptr)[0];
					int32 progMax  = static_cast<const int32 *>(ptr)[1];
					char *name = static_cast<char *>(ptr);
					for(int32 i = progMin; i < progMax; i++)
					{
						strcpy(name, "");
						if(nativeEffect->numPrograms <= 0 || Dispatch(effGetProgramNameIndexed, i, -1, name, 0) != 1)
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
				{
					int32 paramMin = static_cast<const int32 *>(ptr)[0];
					int32 paramMax  = static_cast<const int32 *>(ptr)[1];
					ParameterInfo *param = static_cast<ParameterInfo *>(ptr);
					for(int32 i = paramMin; i < paramMax; i++)
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
						param++;
					}
				}
				break;
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
		ptr = &extraData[0];
	}

	//std::cout << "about to dispatch " << msg->opcode << " to effect...";
	//std::flush(std::cout);
	bool exception = false;
	msg->result = static_cast<int32>(DispatchSEH(nativeEffect, msg->opcode, msg->index, static_cast<VstIntPtr>(msg->value), ptr, msg->opt, exception));
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
		nativeEffect = nullptr;
		FreeLibrary(library);
		library = nullptr;
		closeInstance = true;
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
		extraData.back() = 0;
		vst_strncpy(static_cast<char *>(origPtr), &extraData[0], static_cast<size_t>(msg->ptr - 1));
		break;

	case effEditGetRect:
		// ERect** in [ptr]
		{
			ERect *rectPtr = *reinterpret_cast<ERect **>(&extraData[0]);
			if(rectPtr != nullptr)
			{
				assert(static_cast<size_t>(msg->ptr) >= sizeof(ERect));
				memcpy(origPtr, rectPtr, std::min<size_t>(sizeof(ERect), static_cast<size_t>(msg->ptr)));
				windowSize = *rectPtr;

				// For plugins that don't know their size until after effEditOpen is done.
				if(window)
				{
					SetWindowPos(window, NULL, 0, 0, windowSize.right - windowSize.left, windowSize.bottom - windowSize.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
				}
			}
		}
		break;

	case effEditOpen:
		SetProp(window, _T("MPT"), this);
		ShowWindow(window, SW_SHOW);
		break;

	case effEditClose:
		DestroyWindow(window);
		window = NULL;
		break;

	case effGetChunk:
		// void** in [ptr] for chunk data address
		if(getChunkMem.Create(static_cast<const wchar_t *>(origPtr), msg->result))
		{
			memcpy(getChunkMem.view, *reinterpret_cast<void **>(&extraData[0]), msg->result);
		}
		break;
	}

	UpdateEffectStruct();	// Regularly update the struct
}


VstIntPtr PluginBridge::Dispatch(VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt)
{
	__try
	{
		return nativeEffect->dispatcher(nativeEffect, opcode, index, value, ptr, opt);
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
		nativeEffect->setParameter(nativeEffect, msg->index, msg->value);
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
		msg->value = nativeEffect->getParameter(nativeEffect, msg->index);
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
		uint32 numEvents = InterlockedExchange(&sharedMem->automationQueue.pendingEvents, 0);
		const AutomationQueue::Parameter *param = sharedMem->automationQueue.params;
		for(uint32 i = 0; i < numEvents; i++, param++)
		{
			nativeEffect->setParameter(nativeEffect, param->index, param->value);
		}
	} __except(EXCEPTION_EXECUTE_HANDLER)
	{
		SendErrorMessage(L"Exception in setParameter()!");
	}
}


// Audio rendering thread
void PluginBridge::RenderThread()
{
	const HANDLE objects[] = { sigProcess.send, sigThreadExit };
	DWORD result = 0;
	do
	{
		result = WaitForMultipleObjects(CountOf(objects), objects, FALSE, INFINITE);
		if(result == WAIT_OBJECT_0)
		{
			ProcessMsg *msg = static_cast<ProcessMsg *>(processMem.view);
			InterlockedExchange(&isProcessing, 1);
			AutomateParameters();

			// Prepare VstEvents.
			if(eventMem.Good())
			{
				VstEvents *events = reinterpret_cast<VstEvents *>(eventMem.view);
				if(events->numEvents)
				{
					TranslateBridgeToVSTEvents(eventCache, events);
					events->numEvents = 0;
					Dispatch(effProcessEvents, 0, 0, &eventCache[0], 0.0f);
				}
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

			InterlockedExchange(&isProcessing, 0);
			sigProcess.Confirm();
		}
	} while(result != WAIT_OBJECT_0 + 1 && result != WAIT_OBJECT_0 + 2 && result != WAIT_FAILED);
}


// Process audio.
void PluginBridge::Process()
{
	if(nativeEffect->process)
	{
		float **inPointers, **outPointers;
		int32 sampleFrames = BuildProcessPointers(inPointers, outPointers);
		__try
		{
			nativeEffect->process(nativeEffect, inPointers, outPointers, sampleFrames);
		} __except(EXCEPTION_EXECUTE_HANDLER)
		{
			SendErrorMessage(L"Exception in process()!");
		}
	}
}


// Process audio.
void PluginBridge::ProcessReplacing()
{
	if(nativeEffect->processReplacing)
	{
		float **inPointers, **outPointers;
		int32 sampleFrames = BuildProcessPointers(inPointers, outPointers);
		__try
		{
			nativeEffect->processReplacing(nativeEffect, inPointers, outPointers, sampleFrames);
		} __except(EXCEPTION_EXECUTE_HANDLER)
		{
			SendErrorMessage(L"Exception in processReplacing()!");
		}
	}
}


// Process audio.
void PluginBridge::ProcessDoubleReplacing()
{
	if(nativeEffect->processDoubleReplacing)
	{
		double **inPointers, **outPointers;
		int32 sampleFrames = BuildProcessPointers(inPointers, outPointers);
		__try
		{
			nativeEffect->processDoubleReplacing(nativeEffect, inPointers, outPointers, sampleFrames);
		} __except(EXCEPTION_EXECUTE_HANDLER)
		{
			SendErrorMessage(L"Exception in processDoubleReplacing()!");
		}
	}
}


// Helper function to build the pointer arrays required by the VST process functions.
template<typename buf_t>
int32 PluginBridge::BuildProcessPointers(buf_t **(&inPointers), buf_t **(&outPointers))
{
	assert(processMem.Good());
	ProcessMsg *msg = static_cast<ProcessMsg *>(processMem.view);

	size_t numPtrs = msg->numInputs + msg->numOutputs;
	samplePointers.resize(numPtrs, 0);

	if(numPtrs)
	{
		buf_t *offset = reinterpret_cast<buf_t *>(msg + 1);
		for(size_t i = 0; i < numPtrs; i++)
		{
			samplePointers[i] = offset;
			offset += msg->sampleFrames;
		}
		inPointers = reinterpret_cast<buf_t **>(&samplePointers[0]);
		outPointers = inPointers + msg->numInputs;
	}

	return msg->sampleFrames;
}


// Send a message to the host.
VstIntPtr PluginBridge::DispatchToHost(VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt)
{
	const bool processing = InterlockedExchangeAdd(&isProcessing, 0) != 0;

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
		if(processing)
		{
			// During processing, read the cached time info. It won't change during the call
			// and we can save some valuable inter-process calls that way.
			return ToVstPtr<VstTimeInfo>(&sharedMem->timeInfo);
		}
		break;

	case audioMasterProcessEvents:
		// VstEvents* in [ptr]
		TranslateVSTEventsToBridge(dispatchData, static_cast<VstEvents *>(ptr), otherPtrSize);
		ptrOut = dispatchData.size() - sizeof(DispatchMsg);
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
		MPT_FALLTHROUGH;
	case audioMasterIOChanged:
		// We need to be sure that the new values are known to the master.
		if(nativeEffect != nullptr)
		{
			UpdateEffectStruct();
			CreateProcessingFile(dispatchData);
			ptrOut = dispatchData.size() - sizeof(DispatchMsg);
		}
		break;

	case audioMasterNeedIdle:
		needIdle = true;
		return 1;

	case audioMasterSizeWindow:
		if(window)
		{
			SetWindowPos(window, NULL, 0, 0, index, static_cast<int>(value), SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
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
		assert(false);
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
		return VstFileSelector(opcode == audioMasterCloseFileSelector, static_cast<VstFileSelect *>(ptr));
		break;

	case audioMasterEditFile:
		break;

	case audioMasterGetChunkFile:
		// Name in [ptr]
		ptrOut = 256;
		dispatchData.insert(dispatchData.end(), ptrC, ptrC + ptrOut);
		break;

	default:
#ifdef _DEBUG
		if(ptr != nullptr) __debugbreak();
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
	BridgeMessage *msg = reinterpret_cast<BridgeMessage *>(&dispatchData[0]);
	msg->Dispatch(opcode, index, value, ptrOut, opt, extraSize);

	const bool useAuxMem = dispatchData.size() > sizeof(BridgeMessage);
	MappedMemory auxMem;
	if(useAuxMem)
	{
		// Extra data doesn't fit in message - use secondary memory
		wchar_t auxMemName[64];
		static_assert(sizeof(DispatchMsg) + sizeof(auxMemName) <= sizeof(BridgeMessage), "Check message sizes, this will crash!");
		swprintf(auxMemName, CountOf(auxMemName), L"Local\\openmpt-%d-auxmem-%d", GetCurrentProcessId(), GetCurrentThreadId());
		if(auxMem.Create(auxMemName, extraSize))
		{
			// Move message data to shared memory and then move shared memory name to message data
			memcpy(auxMem.view, &dispatchData[sizeof(DispatchMsg)], extraSize);
			memcpy(&dispatchData[sizeof(DispatchMsg)], auxMemName, sizeof(auxMemName));
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

	const char *extraData = useAuxMem ? static_cast<const char *>(auxMem.view) : reinterpret_cast<const char *>(resultMsg + 1);
	// Post-fix some opcodes
	switch(opcode)
	{
	case audioMasterGetTime:
		// VstTimeInfo* in [return value]
		return ToVstPtr<VstTimeInfo>(&sharedMem->timeInfo);

	case audioMasterGetOutputSpeakerArrangement:
	case audioMasterGetInputSpeakerArrangement:
		// VstSpeakerArrangement* in [return value]
		memcpy(&host2PlugMem.speakerArrangement, extraData, sizeof(VstSpeakerArrangement));
		return ToVstPtr<VstSpeakerArrangement>(&host2PlugMem.speakerArrangement);

	case audioMasterGetVendorString:
	case audioMasterGetProductString:
		// Name in [ptr]
		strcpy(ptrC, extraData);
		break;
	
	case audioMasterGetDirectory:
		// Name in [return value]
		vst_strncpy(host2PlugMem.name, extraData, CountOf(host2PlugMem.name) - 1);
		return ToVstPtr<char>(host2PlugMem.name);

	case audioMasterGetChunkFile:
		// Name in [ptr]
		strcpy(ptrC, extraData);
		break;
	}

	return static_cast<VstIntPtr>(resultMsg->result);
}


// Helper function for file selection dialog stuff.
// Note: This has been mostly copied from Vstplug.cpp. Ugly, but serializing this over the bridge would be even uglier.
VstIntPtr PluginBridge::VstFileSelector(bool destructor, VstFileSelect *fileSel)
{
	if(fileSel == nullptr)
	{
		return 0;
	}

	if(!destructor)
	{
		fileSel->returnMultiplePaths = nullptr;
		fileSel->nbReturnPath = 0;
		fileSel->reserved = 0;

		if(fileSel->command != kVstDirectorySelect)
		{
			// Plugin wants to load or save a file.
			std::string extensions, workingDir;
			for(VstInt32 i = 0; i < fileSel->nbFileTypes; i++)
			{
				VstFileType *pType = &(fileSel->fileTypes[i]);
				extensions += pType->name;
				extensions.push_back(0);
#if (defined(WIN32) || (defined(WINDOWS) && WINDOWS == 1))
				extensions += "*.";
				extensions += pType->dosType;
#elif defined(MAC) && MAC == 1
				extensions += "*";
				extensions += pType->macType;
#elif defined(UNIX) && UNIX == 1
				extensions += "*.";
				extensions += pType->unixType;
#else
#error Platform-specific code missing
#endif
				extensions.push_back(0);
			}
			extensions.push_back(0);

			if(fileSel->initialPath != nullptr)
			{
				workingDir = fileSel->initialPath;
			} else
			{
				// Plugins are probably looking for presets...?
				//workingDir = TrackerSettings::Instance().PathPluginPresets.GetWorkingDir();
			}

			std::string filenameBuffer(uint16_max, 0);

			const bool multiSelect = (fileSel->command == kVstMultipleFilesLoad);
			const bool load = (fileSel->command != kVstFileSave);
			OPENFILENAMEA ofn;
			memset(&ofn, 0, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = window;
			ofn.hInstance = NULL;
			ofn.lpstrFilter = extensions.c_str();
			ofn.lpstrCustomFilter = NULL;
			ofn.nMaxCustFilter = 0;
			ofn.nFilterIndex = 0;
			ofn.lpstrFile = &filenameBuffer[0];
			ofn.nMaxFile = static_cast<DWORD>(filenameBuffer.size());
			ofn.lpstrFileTitle = NULL;
			ofn.nMaxFileTitle = 0;
			ofn.lpstrInitialDir = workingDir.empty() ? NULL : workingDir.c_str();
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

				fileSel->nbReturnPath = static_cast<VstInt32>(numFiles);
				fileSel->returnMultiplePaths = new (std::nothrow) char *[fileSel->nbReturnPath];

				currentFile = ofn.lpstrFile + ofn.nFileOffset;
				for(size_t i = 0; i < numFiles; i++)
				{
					int len = lstrlenA(currentFile);
					char *fname = new (std::nothrow) char[ofn.nFileOffset + len + 1];
					lstrcpyA(fname, ofn.lpstrFile);
					lstrcatA(fname, "\\");
					lstrcpyA(fname + ofn.nFileOffset, currentFile);
					fileSel->returnMultiplePaths[i] = fname;

					currentFile += len + 1;
				}
				
			} else
			{
				// Single path

				// VOPM doesn't initialize required information properly (it doesn't memset the struct to 0)...
				if(CCONST('V', 'O', 'P', 'M') == nativeEffect->uniqueID)
				{
					fileSel->sizeReturnPath = _MAX_PATH;
				}

				if(fileSel->returnPath == nullptr || fileSel->sizeReturnPath == 0)
				{

					// Provide some memory for the return path.
					fileSel->sizeReturnPath = lstrlenA(ofn.lpstrFile) + 1;
					fileSel->returnPath = new (std::nothrow) char[fileSel->sizeReturnPath];
					if(fileSel->returnPath == nullptr)
					{
						return 0;
					}
					fileSel->returnPath[fileSel->sizeReturnPath - 1] = '\0';
					fileSel->reserved = 1;
				} else
				{
					fileSel->reserved = 0;
				}
				strncpy(fileSel->returnPath, ofn.lpstrFile, fileSel->sizeReturnPath - 1);
				fileSel->nbReturnPath = 1;
				fileSel->returnMultiplePaths = nullptr;
			}
			return 1;
		} else
		{
			// Plugin wants a directory
			CHAR path[MAX_PATH];

			BROWSEINFOA bi;
			memset(&bi, 0, sizeof(bi));
			bi.hwndOwner = window;
			bi.lpszTitle = fileSel->title;
			bi.pszDisplayName = path;
			bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
			bi.lpfn = NULL;
			bi.lParam = NULL;
			LPITEMIDLIST pid = SHBrowseForFolderA(&bi);
			if(pid != NULL && SHGetPathFromIDListA(pid, path))
			{
				if(CCONST('V', 'S', 'T', 'r') == nativeEffect->uniqueID && fileSel->returnPath != nullptr && fileSel->sizeReturnPath == 0)
				{
					// old versions of reViSiT (which still relied on the host's file selection code) seem to be dodgy.
					// They report a path size of 0, but when using an own buffer, they will crash.
					// So we'll just assume that reViSiT can handle long enough (_MAX_PATH) paths here.
					fileSel->sizeReturnPath = lstrlenA(path) + 1;
					fileSel->returnPath[fileSel->sizeReturnPath - 1] = '\0';
				}
				if(fileSel->returnPath == nullptr || fileSel->sizeReturnPath == 0)
				{
					// Provide some memory for the return path.
					fileSel->sizeReturnPath = lstrlenA(path) + 1;
					fileSel->returnPath = new char[fileSel->sizeReturnPath];
					if(fileSel->returnPath == nullptr)
					{
						return 0;
					}
					fileSel->returnPath[fileSel->sizeReturnPath - 1] = '\0';
					fileSel->reserved = 1;
				} else
				{
					fileSel->reserved = 0;
				}
				strncpy(fileSel->returnPath, path, fileSel->sizeReturnPath - 1);
				fileSel->nbReturnPath = 1;
				return 1;
			}
			return 0;
		}
	} else
	{
		// Close file selector - delete allocated strings.
		if(fileSel->command == kVstMultipleFilesLoad && fileSel->returnMultiplePaths != nullptr)
		{
			for(VstInt32 i = 0; i < fileSel->nbReturnPath; i++)
			{
				if(fileSel->returnMultiplePaths[i] != nullptr)
				{
					delete[] fileSel->returnMultiplePaths[i];
				}
			}
			delete[] fileSel->returnMultiplePaths;
			fileSel->returnMultiplePaths = nullptr;
		} else
		{
			if(fileSel->reserved == 1 && fileSel->returnPath != nullptr)
			{
				delete[] fileSel->returnPath;
				fileSel->returnPath = nullptr;
			}
		}
		return 1;
	}
}


// Helper function for sending messages to the host.
VstIntPtr VSTCALLBACK PluginBridge::MasterCallback(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt)
{
	PluginBridge *instance = (effect != nullptr && effect->resvd1 != 0) ? FromVstPtr<PluginBridge>(effect->resvd1) : PluginBridge::latestInstance;
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
			SetForegroundWindow(that->windowParent);
			SetForegroundWindow(that->window);
		}
		break;

	case WM_SYSKEYUP:
	case WM_SYSKEYDOWN:
	case WM_KEYUP:
	case WM_KEYDOWN:
		// Let the host handle these keys, too.
		// Allow focus stealing as the host might show a message box as a response to a key shortcut.
		AllowSetForegroundWindow(ASFW_ANY);
		PostMessage(GetParent(that->windowParent), uMsg, wParam, lParam);
		break;

	case WM_ERASEBKGND:
		// Pretend that we erased the background
		return 1;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


OPENMPT_NAMESPACE_END
