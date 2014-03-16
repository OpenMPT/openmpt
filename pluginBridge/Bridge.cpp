// TODO
// Replace Local\\foo :)
// Translate VstIntPtr size in remaining structs!!! VstFileSelect, VstVariableIo, VstOfflineTask, VstAudioFile, VstWindow, VstFileSelect
// Properly handle sharedMem.otherProcess and ask threads to die properly in every situation (random crashes)
// => sigThreadExit might already be an invalid handle the time it arrives in the thread

// Low priority:
// Speed up things like consecutive calls to CVstPlugin::GetFormattedProgramName by a custom opcode
// Re-enable DEP in OpenMPT?
// Module files supposedly opened in plugin wrapper => Does DuplicateHandle word on the host side? Otherwise, named events

#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#define VST_FORCE_DEPRECATED 0
#define MODPLUG_TRACKER
#define CountOf(x) _countof(x)
#include <Windows.h>
#include <tchar.h>
#include <cstdint>
#include <algorithm>
#include <assert.h>

#include <intrin.h>
#undef assert
#define assert(x) if(!(x)) { __debugbreak(); }

#include "Bridge.h"
#include "../common/thread.h"

#include <iostream>	// TODO DEBUG ONLY


// This is kind of a back-up pointer in case we couldn't sneak our pointer into the AEffect struct yet.
// It always points to the last intialized PluginBridge object. Right now there's just one, but we might
// have to initialize more than one plugin in a container at some point to get plugins like SideKickv3 to work.
PluginBridge *PluginBridge::latestInstance = nullptr;


int _tmain(int argc, TCHAR *argv[])
{
	if(argc < 8)
	{
		MessageBox(NULL, _T("This executable is part of OpenMPT. You do not need to run it by yourself."), _T("Plugin Bridge"), 0);
		return -1;
	}

	PluginBridge pb(argv);
	return 0;
}


PluginBridge::PluginBridge(TCHAR *argv[]) : window(NULL)
{
	PluginBridge::latestInstance = this;

	if(!sharedMem.queueMem.Open(argv[0])/* || !sharedMem.extraMemPipe.Connect(argv[1])*/)
	{
		MessageBox(NULL, _T("Could not connect to host."), _T("Plugin Bridge"), 0);
		exit(-1);
	}

	sharedMem.SetupPointers();

	// Store parent process handle so that we can terminate the bridge process when OpenMPT closes.
	sharedMem.otherProcess = OpenProcess(SYNCHRONIZE, FALSE, _ttoi(argv[1]));

	sharedMem.sigToHost.Create(argv[2], argv[3]);
	sharedMem.sigToBridge.Create(argv[4], argv[5]);
	sharedMem.sigProcess.Create(argv[6], argv[7]);

	sharedMem.sigThreadExit = CreateEvent(NULL, FALSE, FALSE, NULL);
	mpt::thread_member<PluginBridge, &PluginBridge::RenderThread>(this);
	sharedMem.msgThreadID = GetCurrentThreadId();

	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = DefWindowProc;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = GetModuleHandle(NULL);
	windowClass.hIcon = NULL;
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
	windowClass.lpszMenuName = NULL;
	windowClass.lpszClassName = _T("OpenMPTPluginBridge");
	windowClass.hIconSm = NULL;
	RegisterClassEx(&windowClass);

	// Tell the parent process that we've initialized the shared memory and are ready to go.
	sharedMem.sigToHost.Confirm();

	const HANDLE objects[] = { sharedMem.sigToBridge.send, sharedMem.sigToHost.ack, sharedMem.otherProcess };
	DWORD result;
	do
	{
		result = WaitForMultipleObjects(CountOf(objects), objects, FALSE, window ? 10 : INFINITE);
		if(result == WAIT_OBJECT_0)
		{
			ParseNextMessage();
		} else if(result == WAIT_OBJECT_0 + 1)
		{
			// Message got answered
			for(size_t i = 0; i < CountOf(sharedMem.queuePtr->toHost); i++)
			{
				BridgeMessage &msg = sharedMem.queuePtr->toHost[i];
				if(InterlockedExchangeAdd(&msg.header.status, 0) == MsgHeader::done)
				{
					InterlockedExchange(&msg.header.status, MsgHeader::empty);
					sharedMem.ackSignals[msg.header.signalID].Confirm();
				}
			}
		} else if(result == WAIT_OBJECT_0 + 2)
		{
			// Something failed
			for(size_t i = 0; i < CountOf(sharedMem.queuePtr->toHost); i++)
			{
				BridgeMessage &msg = sharedMem.queuePtr->toHost[i];
				sharedMem.ackSignals[msg.header.signalID].Send();
			}
		}
		if(window)
		{
			MSG msg;
			while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	} while(result != WAIT_OBJECT_0 + 2);
	CloseHandle(sharedMem.otherProcess);
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
			result = WaitForMultipleObjects(CountOf(objects), objects, FALSE, window ? 10 : INFINITE);
			if(result == WAIT_OBJECT_0)
			{
				// Message got answered
				const bool done = (InterlockedExchangeAdd(&addr->header.status, 0) == MsgHeader::done);
				for(size_t i = 0; i < CountOf(sharedMem.queuePtr->toHost); i++)
				{
					BridgeMessage &msg = sharedMem.queuePtr->toHost[i];
					if(InterlockedExchangeAdd(&msg.header.status, 0) == MsgHeader::done)
					{
						InterlockedExchange(&msg.header.status, MsgHeader::empty);
						if(msg.header.signalID != uint32_t(-1)) sharedMem.ackSignals[msg.header.signalID].Send();
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
			if(window)
			{
				MSG msg;
				while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
		} while(result != WAIT_OBJECT_0 + 2 && result != WAIT_FAILED);
		if(result == WAIT_OBJECT_0 + 2)
		{
			SetEvent(sharedMem.sigThreadExit);
		}
	} else
	{
		// Wait until the message thread notifies us.
		Signal &ackHandle = sharedMem.ackSignals[addr->header.signalID];
		const HANDLE objects[] = { ackHandle.ack, ackHandle.send, sharedMem.otherProcess };
		result = WaitForMultipleObjects(CountOf(objects), objects, FALSE, INFINITE);
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

	dispatchData.insert(dispatchData.end(), reinterpret_cast<char *>(mapName), reinterpret_cast<char *>(mapName) + sizeof(mapName));

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
		if(InterlockedExchangeAdd(&msg->header.status, 0) == MsgHeader::sent)
		{
			InterlockedExchange(&msg->header.status, MsgHeader::received);
			switch(msg->header.type)
			{
			case MsgHeader::init:
				InitBridge(&msg->init);
				break;
			case MsgHeader::close:
				CloseBridge();
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
			}

			InterlockedExchange(&msg->header.status, MsgHeader::done);
			sharedMem.sigToBridge.Confirm();
		}
	}
}


// Load the plugin.
void PluginBridge::InitBridge(InitMsg *msg)
{
	sharedMem.otherPtrSize = msg->hostPtrSize;
	mixBufSize = msg->mixBufSize;
	msg->result = 0;
	
	// TODO DEBUG
	SetConsoleTitleW(msg->str);

	if(msg->version != InitMsg::protocolVersion)
	{
		swprintf(msg->str, CountOf(msg->str), L"Host protocol version (%u) differs from expected version (%u)", msg->version, InitMsg::protocolVersion);
		return;
	}

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
	// Let's identify ourselves...
	sharedMem.effectPtr->resvd1 = 0;
	memcpy(&(sharedMem.effectPtr->resvd2), "OMPT", 4);

	DispatchToHost(audioMasterIOChanged, 0, 0, nullptr, 0.0f);
}


// Unload the plugin.
void PluginBridge::CloseBridge()
{
	CloseMapping();
	SetEvent(sharedMem.sigThreadExit);
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
		ptr = window = CreateWindow(
			_T("OpenMPTPluginBridge"),
			_T("OpenMPT Plugin Bridge"),
			WS_VISIBLE | WS_POPUP,
			CW_USEDEFAULT, CW_USEDEFAULT,
			windowSize.right - windowSize.left, windowSize.bottom- windowSize.top,
			/*reinterpret_cast<HWND>(msg->ptr)*/ NULL,
			NULL,
			windowClass.hInstance,
			NULL);
		SetParent(window, reinterpret_cast<HWND>(msg->ptr));
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
		TranslateBridgeToVSTEvents(extraData, ptr);
		ptr = &extraData[0];
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
			case 0:
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
		msg->result = nativeEffect->dispatcher(nativeEffect, msg->opcode, msg->index, static_cast<VstIntPtr>(msg->value), ptr, msg->opt);
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
			}
		}
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
			sharedMem.sigProcess.Confirm();
		}
	} while(result != WAIT_OBJECT_0 + 1 && result != WAIT_OBJECT_0 + 2 && result != WAIT_FAILED);
	CloseHandle(sharedMem.sigThreadExit);
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
	assert(sharedMem.processMem.Good());
	ProcessMsg *msg = static_cast<ProcessMsg *>(sharedMem.processMem.view);
	size_t numPtrs = msg->numInputs + msg->numOutputs;
	samplePointers.resize(numPtrs, 0);
	buf_t *offset = reinterpret_cast<buf_t *>(msg + 1);
	for(size_t i = 0; i < numPtrs; i++)
	{
		samplePointers[i] = offset;
		offset += msg->sampleFrames;
	}
	inPointers = reinterpret_cast<buf_t **>(&samplePointers[0]);
	outPointers = reinterpret_cast<buf_t **>(&samplePointers[msg->numInputs]);
	return msg->sampleFrames;
}


// Send a message to the host.
VstIntPtr PluginBridge::DispatchToHost(VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt)
{
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

	case audioMasterIOChanged:
		// We need to be sure that the new values are known to the master.
		UpdateEffectStruct();
		CreateProcessingFile(dispatchData);
		ptrOut = dispatchData.size() - sizeof(DispatchMsg);
		break;

	case audioMasterNeedIdle:
		break;

	case audioMasterSizeWindow:
		if(window)
		{
			SetWindowPos(window, NULL, 0, 0, index, value, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
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
	case audioMasterVendorSpecific:
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
		memcpy(&host2PlugMem.timeInfo, extraData, sizeof(VstTimeInfo));
		return ToVstPtr<VstTimeInfo>(&host2PlugMem.timeInfo);

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
