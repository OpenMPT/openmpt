/*
 * Bridge.cpp
 * ----------
 * Purpose: VST plugin bridge (plugin side)
 * Notes  : (currently none)
 * Authors: Johannes Schultz (OpenMPT Devs)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


// TODO
// Replace Local\\foo :)
// Translate VstIntPtr size in remaining structs!!! VstFileSelect, VstVariableIo, VstOfflineTask, VstAudioFile, VstWindow, VstFileSelect
// Fix deadlocks in some plugin GUIs (triggering many notes through the M1 GUI)
// Ability to put all plugins (or all instances of the same plugin) in the same container. Necessary for plugins like SideKick v3.
// jBridged Rez3 GUI breaks
// ProteusVX, TriDirt sometimes flickers because of timed redraws
// ProteusVX GUI stops working after closing it once and the plugin crashes when unloading it when the GUI was open at least once
// Message handling isn't perfect yet: a message slot may be re-used for sending before its result is being read. Either copy back message, or manually reset message afterwards by caller
// Fix potential thread-safety problems int automation stuff

// Low priority:
// Speed up things like consecutive calls to CVstPlugin::GetFormattedProgramName by a custom opcode
// Re-enable DEP in OpenMPT?
// Module files supposedly opened in plugin wrapper => Does DuplicateHandle work on the host side? Otherwise, named events (can't repro anymore)
// Remove native jBridge support
// Clean up code :)

#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#define VST_FORCE_DEPRECATED 0
#define MODPLUG_TRACKER
#define CountOf(x) _countof(x)
#define MPT_FALLTHROUGH __fallthrough
#include <Windows.h>
#include <tchar.h>
#include <cstdint>
#include <algorithm>

//#include <assert.h>
#include <intrin.h>
#undef assert
#define assert(x) while(!(x)) { ::MessageBoxA(NULL, #x, "Debug Assertion Failed", MB_ICONERROR);  __debugbreak(); break; }

#include "Bridge.h"
#include "../common/thread.h"

//#include <iostream>	// TODO DEBUG ONLY


// This is kind of a back-up pointer in case we couldn't sneak our pointer into the AEffect struct yet.
// It always points to the last intialized PluginBridge object. Right now there's just one, but we might
// have to initialize more than one plugin in a container at some point to get plugins like SideKickv3 to work.
PluginBridge *PluginBridge::latestInstance = nullptr;
WNDCLASSEX PluginBridge::windowClass;
#define WINDOWCLASSNAME _T("OpenMPTPluginBridge")


int _tmain(int argc, TCHAR *argv[])
{
	if(argc < 8)
	{
		MessageBox(NULL, _T("This executable is part of OpenMPT. You do not need to run it by yourself."), _T("Plugin Bridge"), 0);
		return -1;
	}

	PluginBridge::RegisterWindowClass();

	Signal sigToHost, sigToBridge, sigProcess;
	sigToHost.Create(argv[2], argv[3]);
	sigToBridge.Create(argv[4], argv[5]);
	sigProcess.Create(argv[6], argv[7]);
	PluginBridge pb(argv[0], OpenProcess(SYNCHRONIZE, FALSE, _ttoi(argv[1])), sigToHost, sigToBridge, sigProcess);
	return 0;
}


int WINAPI WinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*nCmdShow*/)
{
	LPWSTR *argv;
	int argc;
	argv = CommandLineToArgvW(GetCommandLine(), &argc);
	return _tmain(argc, argv);
}


// Register the editor window class
void PluginBridge::RegisterWindowClass()
{
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = WindowProc;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = GetModuleHandle(NULL);
	windowClass.hIcon = NULL;
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
	windowClass.lpszMenuName = NULL;
	windowClass.lpszClassName = WINDOWCLASSNAME;
	windowClass.hIconSm = NULL;
	RegisterClassEx(&windowClass);
}


PluginBridge::PluginBridge(TCHAR *memName, HANDLE otherProcess, Signal &sigToHost, Signal &sigToBridge, Signal &sigProcess)
	: window(NULL), isProcessing(0), nativeEffect(nullptr), needIdle(false)
{
	PluginBridge::latestInstance = this;

	if(!sharedMem.queueMem.Open(memName))
	{
		MessageBox(NULL, _T("Could not connect to host."), _T("Plugin Bridge"), 0);
		exit(-1);
	}

	sharedMem.SetupPointers();

	// Store parent process handle so that we can terminate the bridge process when OpenMPT closes.
	sharedMem.otherProcess = otherProcess;

	sharedMem.sigToHost.Create(sigToHost);
	sharedMem.sigToBridge.Create(sigToBridge);
	sharedMem.sigProcess.Create(sigProcess);

	sharedMem.sigThreadExit.Create(true);
	sharedMem.otherThread = mpt::thread_member<PluginBridge, &PluginBridge::RenderThread>(this);
	sharedMem.msgThreadID = GetCurrentThreadId();

	// Tell the parent process that we've initialized the shared memory and are ready to go.
	sharedMem.sigToHost.Confirm();

	const HANDLE objects[] = { sharedMem.sigToBridge.send, sharedMem.sigToHost.ack, sharedMem.otherProcess };
	DWORD result;
	do
	{
		// Wait for incoming messages, time out for window refresh
		result = WaitForMultipleObjects(CountOf(objects), objects, FALSE, window ? 15 : INFINITE);
		if(result == WAIT_OBJECT_0)
		{
			ParseNextMessage();
		} else if(result == WAIT_OBJECT_0 + 1)
		{
			// Message got answered
			for(size_t i = 0; i < CountOf(sharedMem.queuePtr->toHost); i++)
			{
				BridgeMessage &msg = sharedMem.queuePtr->toHost[i];
				if(InterlockedCompareExchange(&msg.header.status, MsgHeader::empty, MsgHeader::done) == MsgHeader::done)
				{
					sharedMem.ackSignals[msg.header.signalID].Confirm();
				}
			}
		}
		if(needIdle && nativeEffect)
		{
			nativeEffect->dispatcher(nativeEffect, effIdle, 0, 0, nullptr, 0.0f);
		}
		if(window)
		{
			MessageHandler();
		}
	} while(result != WAIT_OBJECT_0 + 2);
}


PluginBridge::~PluginBridge()
{
	CloseHandle(sharedMem.otherProcess);
	sharedMem.sigThreadExit.Trigger();
	WaitForSingleObject(sharedMem.otherThread, INFINITE);
}


// Destroy our shared memory object.
void PluginBridge::CloseMapping()
{
	sharedMem.queuePtr = nullptr;
	sharedMem.queueMem.Close();
	sharedMem.processMem.Close();
}


// Send an arbitrary message to the host.
// Returns a pointer to the message, as processed by the host.
const BridgeMessage *PluginBridge::SendToHost(const BridgeMessage &msg)
{
	const bool inMsgThread = GetCurrentThreadId() == sharedMem.msgThreadID;
	BridgeMessage *addr = sharedMem.CopyToSharedMemory(msg, sharedMem.queuePtr->toHost, inMsgThread);
	sharedMem.sigToHost.Send();

	// Wait until we get the result from the host.
	DWORD result;
	if(inMsgThread)
	{
		// Since this is the message thread, we must handle messages directly.
		const HANDLE objects[] = { sharedMem.sigToHost.ack, sharedMem.sigToBridge.send, sharedMem.otherProcess };
		do
		{
			result = WaitForMultipleObjects(CountOf(objects), objects, FALSE, INFINITE);
			if(result == WAIT_OBJECT_0)
			{
				// Some message got answered - is it our message?
				const bool done = (InterlockedExchangeAdd(&addr->header.status, 0) == MsgHeader::done);
				for(size_t i = 0; i < CountOf(sharedMem.queuePtr->toHost); i++)
				{
					BridgeMessage &msg = sharedMem.queuePtr->toHost[i];
					if(InterlockedCompareExchange(&msg.header.status, MsgHeader::empty, MsgHeader::done) == MsgHeader::done)
					{
						if(msg.header.signalID != uint32_t(-1)) sharedMem.ackSignals[msg.header.signalID].Confirm();
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
		if(result == WAIT_OBJECT_0 + 2)
		{
			sharedMem.sigThreadExit.Trigger();
		}
	} else
	{
		// Wait until the message thread notifies us.
		Signal &ackHandle = sharedMem.ackSignals[addr->header.signalID];
		const HANDLE objects[] = { ackHandle.ack, ackHandle.send, sharedMem.otherProcess };
		result = WaitForMultipleObjects(CountOf(objects), objects, FALSE, INFINITE);
		if(result != WAIT_OBJECT_0)
		{
			InterlockedExchange(&addr->header.status, MsgHeader::empty);
		}
	}

	return (result == WAIT_OBJECT_0) ? addr : nullptr;
}


// Copy AEffect to shared memory.
void PluginBridge::UpdateEffectStruct()
{
	if(sharedMem.otherPtrSize == 4)
	{
		reinterpret_cast<AEffect32 *>(sharedMem.effectPtr)->FromNative(*nativeEffect);
	} else if(sharedMem.otherPtrSize == 8)
	{
		reinterpret_cast<AEffect64 *>(sharedMem.effectPtr)->FromNative(*nativeEffect);
	} else
	{
		assert(false);
	}
}


// Create the memory-mapped file containing the processing message and audio buffers
void PluginBridge::CreateProcessingFile(std::vector<char> &dispatchData)
{
	static uint32_t plugId = 0;
	wchar_t mapName[64];
	swprintf(mapName, CountOf(mapName), L"Local\\openmpt-%d-%d", GetCurrentProcessId(), plugId++);

	PushToVector(dispatchData, mapName[0], sizeof(mapName));

	if(!sharedMem.processMem.Create(mapName, sizeof(ProcessMsg) + mixBufSize * (nativeEffect->numInputs + nativeEffect->numOutputs) * sizeof(double)))
	{
		SendErrorMessage(L"Could not initialize plugin bridge audio memory.");
		return;
	}
}


// Receive a message from the host and translate it.
void PluginBridge::ParseNextMessage()
{
	assert(GetCurrentThreadId() == sharedMem.msgThreadID);
	for(size_t i = 0; i < CountOf(sharedMem.queuePtr->toBridge); i++)
	{
		BridgeMessage *msg = sharedMem.queuePtr->toBridge + i;
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
			sharedMem.sigToBridge.Confirm();
		}
	}
}


// Create a new bridge instance within this one.
void PluginBridge::NewInstance(NewInstanceMsg *msg)
{
	// TODO launch a new thread
	Signal sigToHost, sigToBridge, sigProcess;
	sigToHost.Create(msg->handles[0], msg->handles[1]);
	sigToBridge.Create(msg->handles[2], msg->handles[3]);
	sigProcess.Create(msg->handles[4], msg->handles[5]);
	new PluginBridge(msg->memName, latestInstance->sharedMem.otherProcess, sigToHost, sigToBridge, sigProcess);
}


// Load the plugin.
void PluginBridge::InitBridge(InitMsg *msg)
{
	sharedMem.otherPtrSize = msg->hostPtrSize;
	mixBufSize = msg->mixBufSize;
	msg->result = 0;
	
	// TODO DEBUG
	//SetConsoleTitleW(msg->str);

	nativeEffect = nullptr;
	library = LoadLibraryW(msg->str);

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
		try
		{
			nativeEffect = pMainProc(MasterCallback);
		} catch(...)
		{
			nativeEffect = nullptr;
		}
	}

	if(nativeEffect == nullptr || nativeEffect->dispatcher == nullptr || nativeEffect->magic != kEffectMagic)
	{
		FreeLibrary(library);
		library = nullptr;
		
		wcscpy(msg->str, L"File is not a valid plugin");
		return;
	}

	nativeEffect->resvd1 = ToVstPtr(this);

	msg->result = 1;

	UpdateEffectStruct();

	// Init process buffer
	DispatchToHost(audioMasterVendorSpecific, CCONST('O', 'M', 'P', 'T'), kUpdateProcessingBuffer, nullptr, 0.0f);
}


// Unload the plugin.
void PluginBridge::CloseBridge()
{
	CloseMapping();
	sharedMem.sigThreadExit.Trigger();
	exit(0);
}


void PluginBridge::SendErrorMessage(const wchar_t *str)
{
	BridgeMessage msg;
	msg.Error(str);
	SendToHost(msg);
}


// Host-to-plugin opcode dispatcher
void PluginBridge::DispatchToPlugin(DispatchMsg *msg)
{
	// Various dispatch data - depending on the opcode, one of those might be used.
	std::vector<char> extraData;
	size_t extraDataSize = 0;

	MappedMemory auxMem;

	// Content of ptr is usually stored right after the message header, ptr field indicates size.
	void *ptr = (msg->ptr != 0) ? (msg + 1) : nullptr;
	if(msg->size > sizeof(BridgeMessage))
	{
		if(!auxMem.Open(L"Local\\foo"))
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

	case effEditGetRect:
		// ERect** in [ptr]
		extraDataSize = sizeof(void *);
		break;

	case effEditOpen:
		// HWND in [ptr] - Note: Window handles are interoperable between 32-bit and 64-bit applications in Windows (http://msdn.microsoft.com/en-us/library/windows/desktop/aa384203%28v=vs.85%29.aspx)
		{
			TCHAR str[_MAX_PATH];
			GetModuleFileName(library, str, CountOf(str));

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

			// Periodically update the window, in case redrawing failed.
			SetTimer(window, 0x1337, 1000, nullptr);
			windowParent = reinterpret_cast<HWND>(msg->ptr);
		}
		break;

	case effEditClose:
		DestroyWindow(window);
		window = NULL;
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
		for(int64_t i = 0; i < msg->value; i++)
		{
			// TODO create pointers
		}
		break;

	case effOfflinePrepare:
	case effOfflineRun:
		// VstOfflineTask* in [ptr]
		extraData.resize(sizeof(VstOfflineTask *) * static_cast<size_t>(msg->value));
		ptr = &extraData[0];
		for(int64_t i = 0; i < msg->value; i++)
		{
			// TODO create pointers
		}
		break;

	case effSetSpeakerArrangement:
	case effGetSpeakerArrangement:
		// VstSpeakerArrangement* in [value] and [ptr]
		msg->value = reinterpret_cast<int64_t>(ptr) + sizeof(VstSpeakerArrangement);
		break;

	case effVendorSpecific:
		// Let's implement some custom opcodes!
		if(msg->index == CCONST('O', 'M', 'P', 'T'))
		{
			msg->result = 1;
			switch(msg->value)
			{
			case kUpdateEffectStruct:
				UpdateEffectStruct();
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
	try
	{
		msg->result = static_cast<int32_t>(nativeEffect->dispatcher(nativeEffect, msg->opcode, msg->index, static_cast<VstIntPtr>(msg->value), ptr, msg->opt));
	} catch(...)
	{
		SendErrorMessage(L"Exception in dispatch()!");
	}
	//std::cout << "done" << std::endl;

	// Post-fix some opcodes
	switch(msg->opcode)
	{
	case effClose:
		nativeEffect = nullptr;
		FreeLibrary(library);
		library = nullptr;
		CloseBridge();
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
		// Need to do this after creating. Otherwise, we'll freeze. We also need to do this after the open call, or else ProteusVX will freeze in a SetParent call.
		SetParent(window, windowParent);
		SetProp(window, _T("MPT"), this);
		ShowWindow(window, SW_SHOW);
		break;

	case effGetChunk:
		// void** in [ptr] for chunk data address
		{
			void *chunkPtr = *reinterpret_cast<void **>(&extraData[0]);
			if(sharedMem.getChunkMem.Create(static_cast<const wchar_t *>(origPtr), msg->result))
			{
				memcpy(sharedMem.getChunkMem.view, *reinterpret_cast<void **>(&extraData[0]), msg->result);
			}
		}
		break;
	}

	UpdateEffectStruct();	// Regularly update the struct
}


// Set a plugin parameter.
void PluginBridge::SetParameter(ParameterMsg *msg)
{
	try
	{
		nativeEffect->setParameter(nativeEffect, msg->index, msg->value);
	} catch(...)
	{
		SendErrorMessage(L"Exception in setParameter()!");
	}
}


// Get a plugin parameter.
void PluginBridge::GetParameter(ParameterMsg *msg)
{
	try
	{
		msg->value = nativeEffect->getParameter(nativeEffect, msg->index);
	} catch(...)
	{
		SendErrorMessage(L"Exception in getParameter()!");
	}
}


// Execute received parameter automation messages
void PluginBridge::AutomateParameters()
{
	try
	{
		uint32_t numEvents = InterlockedExchange(&sharedMem.automationPtr->pendingEvents, 0);
		const AutomationQueue::Parameter *param = sharedMem.automationPtr->params;
		for(uint32_t i = 0; i < numEvents; i++, param++)
		{
			nativeEffect->setParameter(nativeEffect, param->index, param->value);
		}
	} catch(...)
	{
		SendErrorMessage(L"Exception in setParameter()!");
	}
}


// Audio rendering thread
void PluginBridge::RenderThread()
{
	const HANDLE objects[] = { sharedMem.sigProcess.send, sharedMem.sigThreadExit };
	DWORD result = 0;
	do
	{
		result = WaitForMultipleObjects(CountOf(objects), objects, FALSE, INFINITE);
		if(result == WAIT_OBJECT_0)
		{
			ProcessMsg *msg = static_cast<ProcessMsg *>(sharedMem.processMem.view);
			InterlockedExchange(&isProcessing, 1);
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
			sharedMem.sigProcess.Confirm();
		}
	} while(result != WAIT_OBJECT_0 + 1 && result != WAIT_OBJECT_0 + 2 && result != WAIT_FAILED);
	sharedMem.otherThread = NULL;
}


// Process audio.
void PluginBridge::Process()
{
	if(nativeEffect->process)
	{
		float **inPointers, **outPointers;
		int32_t sampleFrames = BuildProcessPointers(inPointers, outPointers);
		try
		{
			nativeEffect->process(nativeEffect, inPointers, outPointers, sampleFrames);
		} catch(...)
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
		int32_t sampleFrames = BuildProcessPointers(inPointers, outPointers);
		try
		{
			nativeEffect->processReplacing(nativeEffect, inPointers, outPointers, sampleFrames);
		} catch(...)
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
		int32_t sampleFrames = BuildProcessPointers(inPointers, outPointers);
		try
		{
			nativeEffect->processDoubleReplacing(nativeEffect, inPointers, outPointers, sampleFrames);
		} catch(...)
		{
			SendErrorMessage(L"Exception in processDoubleReplacing()!");
		}
	}
}


// Helper function to build the pointer arrays required by the VST process functions.
template<typename buf_t>
int32_t PluginBridge::BuildProcessPointers(buf_t **(&inPointers), buf_t **(&outPointers))
{
	AutomateParameters();

	assert(sharedMem.processMem.Good());
	ProcessMsg *msg = static_cast<ProcessMsg *>(sharedMem.processMem.view);

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
		outPointers = reinterpret_cast<buf_t **>(&samplePointers[msg->numInputs]);
	}

	return msg->sampleFrames;
}


// Send a message to the host.
VstIntPtr PluginBridge::DispatchToHost(VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt)
{
	const bool processing = InterlockedExchangeAdd(&isProcessing, 0) != 0;

	std::vector<char> dispatchData(sizeof(DispatchMsg), 0);
	int64_t ptrOut = 0;
	char *ptrC = static_cast<char *>(ptr);

	switch(opcode)
	{
	case audioMasterAutomate:
	case audioMasterVersion:
	case audioMasterCurrentId:
	case audioMasterIdle:
	case audioMasterPinConnected:
		break;

	case audioMasterGetTime:
		// VstTimeInfo* in [return value]
		if(processing)
		{
			// During processing, read the cached time info if possible
			VstTimeInfo &timeInfo = static_cast<ProcessMsg *>(sharedMem.processMem.view)->timeInfo;
			if((timeInfo.flags & value) == value)
			{
				return reinterpret_cast<VstIntPtr>(&static_cast<ProcessMsg *>(sharedMem.processMem.view)->timeInfo);
			}
		}
		ptrOut = sizeof(VstTimeInfo);
		break;

	case audioMasterProcessEvents:
		// VstEvents* in [ptr]
		TranslateVSTEventsToBridge(dispatchData, static_cast<VstEvents *>(ptr), sharedMem.otherPtrSize);
		ptrOut = dispatchData.size() - sizeof(DispatchMsg);
		break;

	case audioMasterSetTime:
	case audioMasterTempoAt:
	case audioMasterGetNumAutomatableParameters:
	case audioMasterGetParameterQuantization:
		break;

	case audioMasterVendorSpecific:
		if(index != CCONST('O', 'M', 'P', 'T') || value != kUpdateProcessingBuffer)
		{
			break;
		}
		MPT_FALLTHROUGH;
	case audioMasterIOChanged:
		// We need to be sure that the new values are known to the master.
		if(!processing)
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
		// TODO
		ptrOut = sizeof(VstFileSelect);
		break;

	case audioMasterEditFile:
	case audioMasterGetChunkFile:
		break;

	default:
		if(ptr != nullptr) __debugbreak();
	}

	if(ptrOut != 0)
	{
		// In case we only reserve space and don't copy stuff over...
		dispatchData.resize(sizeof(DispatchMsg) + static_cast<size_t>(ptrOut), 0);
	}
	
	uint32_t extraSize = static_cast<uint32_t>(dispatchData.size() - sizeof(DispatchMsg));
	MappedMemory auxMem;
	if(dispatchData.size() > sizeof(BridgeMessage))
	{
		//that->sharedMem.extraMemPipe.Write(&dispatchData[sizeof(DispatchMsg)], extraSize);
		//dispatchData.resize(sizeof(DispatchMsg));
		if(auxMem.Create(L"Local\\foo2", extraSize))
		{
			memcpy(auxMem.view, &dispatchData[sizeof(DispatchMsg)], extraSize);
			dispatchData.resize(sizeof(DispatchMsg));
		} else
		{
			return 0;
		}
	}

	//std::cout << "about to dispatch " << opcode << " to host...";
	//std::flush(std::cout);
	const DispatchMsg *resultMsg;
	{
		BridgeMessage *msg = reinterpret_cast<BridgeMessage *>(&dispatchData[0]);
		msg->Dispatch(opcode, index, value, ptrOut, opt, extraSize);
		resultMsg = &(SendToHost(*msg)->dispatch);
	}
	//std::cout << "done." << std::endl;

	if(resultMsg == nullptr)
	{
		return 0;
	}

	const char *extraData = dispatchData.size() == sizeof(DispatchMsg) ? static_cast<const char *>(auxMem.view) : reinterpret_cast<const char *>(resultMsg + 1);
	// Post-fix some opcodes
	switch(opcode)
	{
	case audioMasterGetTime:
		// VstTimeInfo* in [return value]
		if(sharedMem.processMem.Good())
		{
			VstTimeInfo *timeInfo = &static_cast<ProcessMsg *>(sharedMem.processMem.view)->timeInfo;
			memcpy(timeInfo, extraData, sizeof(VstTimeInfo));
			return ToVstPtr<VstTimeInfo>(timeInfo);
		}
		return 0;

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

	case audioMasterOpenFileSelector:
	case audioMasterCloseFileSelector:
		// VstFileSelect* in [ptr]
		// TODO
		break;
	}

	return static_cast<VstIntPtr>(resultMsg->result);
}


// Helper function for sending messages to the host.
VstIntPtr VSTCALLBACK PluginBridge::MasterCallback(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt)
{
	PluginBridge *instance = (effect != nullptr && effect->resvd1 != 0) ? FromVstPtr<PluginBridge>(effect->resvd1) : PluginBridge::latestInstance;
	return instance->DispatchToHost(opcode, index, value, ptr, opt);
}


LRESULT CALLBACK PluginBridge::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_NCACTIVATE:
		if(wParam == TRUE)
		{
			// Window is activated - put the plugin window into foreground
			PluginBridge *that = reinterpret_cast<PluginBridge *>(GetProp(hwnd, _T("MPT")));
			if(that != nullptr)
			{
				SetForegroundWindow(that->windowParent);
				SetForegroundWindow(that->window);
			}
		}
		break;

	case WM_TIMER:
		if(wParam == 0x1337)
		{
			// Fix failed plugin window redrawing by periodically forcing a redraw
			RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN);
		}
		break;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


// WinAPI message handler for plugin GUI
void PluginBridge::MessageHandler()
{
	nativeEffect->dispatcher(nativeEffect, effEditIdle, 0, 0, nullptr, 0.0f);

	MSG msg;
	while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}