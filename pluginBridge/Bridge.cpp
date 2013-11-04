// TODO
// Host <-> bridge communication of memory size
// Have independent process signal
// Thread safety: Keep multiple messages, when multiple threads are waiting for a signal, we need to figure out which one got its result!
// Translate VstIntPtr size in remaining structs!!! VstFileSelect, VstVariableIo, VstOfflineTask, VstAudioFile, VstWindow, VstFileSelect
// Properly handle sharedMem.otherProcess and ask threads to die properly in every situation
// => sigThreadExit might already be an invalid handle the time it arrives in the thread
// Resizing the mix buffer transfer area
// Bridge should tell host about memory layout, host shouldn't assume things
// Freeze during load: otium, electri-q
// => If a message occours while waiting for an ACK, send a custom SetEvent() handle with the message which should be triggered

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

#include "Bridge.h"

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

	if(!CreateMapping(argv[0]))
	{
		exit(-1);
	}

	// Store parent process handle so that we can terminate the bridge process when OpenMPT closes.
	sharedMem.otherProcess = OpenProcess(SYNCHRONIZE, FALSE, _ttoi(argv[1]));

	sharedMem.sigToHost.Create(argv[2], argv[3]);
	sharedMem.sigToBridge.Create(argv[4], argv[5]);
	sharedMem.sigProcess.Create(argv[6], argv[7]);

	sharedMem.sigThreadExit = CreateEvent(NULL, FALSE, FALSE, NULL);
	DWORD dummy;	// For Win9x
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&RenderThread, (LPVOID)this, 0, &dummy);

	// Tell the parent process that we've initialized the shared memory and are ready to go.
	sharedMem.sigToHost.Confirm();

	const HANDLE objects[] = { sharedMem.sigToBridge.send, sharedMem.otherProcess };
	DWORD result;
	do
	{
		result = WaitForMultipleObjects(CountOf(objects), objects, FALSE, /*100*/INFINITE);
		if(result == WAIT_OBJECT_0)
		{
			ParseNextMessage();
		}
		if(window)
		{
			//PeekMessage?
			MSG msg;
			GetMessage(&msg, window, 0, 0);
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	} while(result != WAIT_OBJECT_0 + 1);
	CloseHandle(sharedMem.otherProcess);
}


// Create our shared memory object. Returns true on success.
bool PluginBridge::CreateMapping(const TCHAR *memName)
{
	SECURITY_ATTRIBUTES secAttr;
	secAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	secAttr.lpSecurityDescriptor = nullptr;
	secAttr.bInheritHandle = TRUE;

	// Create the file mapping object.
	CloseMapping();
	sharedMem.mapFile = CreateFileMapping(INVALID_HANDLE_VALUE, &secAttr, PAGE_READWRITE,
		0, sharedMem.queueSize + sharedMem.sampleBufSize, memName);
	if(sharedMem.mapFile == NULL)
	{
		return false;
	}

	sharedMem.sharedPtr = static_cast<char *>(MapViewOfFile(sharedMem.mapFile, FILE_MAP_ALL_ACCESS,
		0, 0, sharedMem.queueSize + sharedMem.sampleBufSize));
	if(sharedMem.sharedPtr == nullptr)
	{
		CloseMapping();
		return false;
	}

	// Map a view of the file mapping into the address space of the current process.
	sharedMem.effectPtr = reinterpret_cast<AEffect *>(sharedMem.sharedPtr);
	sharedMem.sampleBufPtr = sharedMem.sharedPtr + sizeof(AEffect64);
	sharedMem.queuePtr = sharedMem.sharedPtr + sharedMem.sampleBufSize;
	sharedMem.sampleBufSize -= sizeof(AEffect64);

	return true;
}


// Destroy our shared memory object.
void PluginBridge::CloseMapping()
{
	if(sharedMem.mapFile)
	{
		if(sharedMem.queuePtr)
		{
			UnmapViewOfFile(sharedMem.queuePtr);
			sharedMem.queuePtr = nullptr;
		}
		CloseHandle(sharedMem.mapFile);
		sharedMem.mapFile = NULL;
	}
}


// Send an arbitrary message to the host.
// Returns a pointer to the message, as processed by the host.
const MsgHeader *PluginBridge::SendToHost(const MsgHeader &msg, MsgHeader *sourceHeader)
{
	if(msg.size > sharedMem.queueSize)
	{
		// Need to increase shared memory size
		static uint32_t pluginCounter = 0;
		/*wchar_t name[64];
		wsprintf(name, L"Local\\openmpt-%d-%d", GetCurrentProcessId(), pluginCounter++);*/
		int32_t newSize = msg.size + 1024;

		// TODO
		SendToHost(ReallocMsg(0, newSize));
		sharedMem.queueSize = newSize;
		CreateMapping(nullptr /*name*/);
	}

	sharedMem.writeMutex.lock();

	if(sharedMem.writeOffset + msg.size > sizeof(MsgQueue) + 512 * 1024 /*sharedMem.queueSize*/)
	{
		std::cout << "message wraparound" << std::endl;
		sharedMem.writeOffset = sizeof(MsgQueue);
	}
	MsgHeader *addr = reinterpret_cast<MsgHeader *>(static_cast<char *>(sharedMem.queuePtr) + sharedMem.writeOffset);
	memcpy(addr, &msg, msg.size);

	if(sourceHeader != nullptr)
	{
		// Interrupting a command..
		sourceHeader->interruptOffset = sharedMem.writeOffset;
		SetEvent(reinterpret_cast<HANDLE>(sourceHeader->interruptSignal));
	} else
	{
		// Simply sending a new command
		MsgQueue *queue = static_cast<MsgQueue *>(sharedMem.queuePtr);
		queue->toHost.offset[queue->toHost.writeIndex++] = sharedMem.writeOffset;
		if(queue->toHost.writeIndex >= CountOf(queue->toHost.offset))
		{
			queue->toHost.writeIndex = 0;
		}
		sharedMem.sigToHost.Send();
	}

	//sharedMem.writeMutex.unlock();

	// Wait until we get the result from the host.
	const HANDLE objects[] = { sharedMem.sigToHost.ack, sharedMem.otherProcess, sharedMem.sigToBridge.send };
	DWORD result;
	do
	{
		result = WaitForMultipleObjects(CountOf(objects), objects, FALSE, INFINITE);
		if(result == WAIT_OBJECT_0 + 2)
		{
			ParseNextMessage();
		}
	} while(result != WAIT_OBJECT_0 && result != WAIT_OBJECT_0 + 1 && result != WAIT_FAILED);

	sharedMem.writeMutex.unlock();
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
	}
}


// Receive a message from the host and translate it.
void PluginBridge::ParseNextMessage(MsgHeader *parentMessage)
{
	MsgHeader *msg;
	if(parentMessage == nullptr)
	{
		// Default: Grab next message
		MsgQueue *queue = static_cast<MsgQueue *>(sharedMem.queuePtr);
		uint32_t index = InterlockedExchangeAdd(&(queue->toBridge.readIndex), 1) % CountOf(queue->toBridge.offset);
		msg = reinterpret_cast<MsgHeader *>(static_cast<char *>(sharedMem.queuePtr) + queue->toBridge.offset[index]);
	} else
	{
		// Read interrupt message
		msg = reinterpret_cast<MsgHeader *>(static_cast<char *>(sharedMem.queuePtr) + parentMessage->interruptOffset);
	}

	switch(msg->type)
	{
	case MsgHeader::init:
		InitBridge(static_cast<InitMsg *>(msg));
		break;
	case MsgHeader::close:
		CloseBridge();
		break;
	case MsgHeader::dispatch:
		DispatchToPlugin(static_cast<DispatchMsg *>(msg));
		break;
	case MsgHeader::setParameter:
		SetParameter(static_cast<ParameterMsg *>(msg));
		break;
	case MsgHeader::getParameter:
		GetParameter(static_cast<ParameterMsg *>(msg));
		break;
	}
	sharedMem.sigToBridge.Confirm();
}


// Load the plugin.
void PluginBridge::InitBridge(InitMsg *msg)
{
	msg->result = 0;
	msg->mapSize = sharedMem.queueSize;
	//msg->hostPtrSize = sizeof(VstIntPtr);
	// TODO
	msg->samplesOffset = sharedMem.queueSize - sharedMem.sampleBufSize;
	
	if(msg->headerSize != sizeof(MsgHeader))
	{
		wcscpy(msg->str, L"Host message format is not compatible with bridge");
		return;
	}
	if(msg->version != InitMsg::protocolVersion)
	{
		swprintf(msg->str, CountOf(msg->str), L"Host protocol version (%u) differs from expected version (%u)", msg->version, InitMsg::protocolVersion);
		return;
	}

	std::wcout << msg->str << std::endl;
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

	sharedMem.otherPtrSize = msg->hostPtrSize;
	msg->result = 1;

	UpdateEffectStruct();
	// Let's identify ourselves...
	memcpy(&(sharedMem.effectPtr->resvd1), "OMPT", 4);
}


// Unload the plugin.
void PluginBridge::CloseBridge()
{
	CloseMapping();
	SetEvent(sharedMem.sigThreadExit);
	exit(0);
}


// Re-initialize the shared memory object with a different amount of memory.
void PluginBridge::ReallocateBridge(ReallocMsg *msg)
{
	// TODO
	//sharedMem.size = msg->mapSize;
	//CreateMapping(msg->str);
}


// Host-to-plugin opcode dispatcher
void PluginBridge::DispatchToPlugin(DispatchMsg *msg)
{
	// Various dispatch data - depending on the opcode, one of those might be used.
	std::vector<char> extraData;
	size_t extraDataSize = 0;

	// Content of ptr is usually stored right after the message header, ptr field indicates size.
	void *ptr = (msg->ptr != 0) ? (msg + 1) : nullptr;

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
		std::cout << "open!" << std::endl;
		Sleep(1000);
		ptr = reinterpret_cast<void *>(msg->ptr);
		if(0)
		{
			WNDCLASSEX wcex;

			wcex.cbSize = sizeof(WNDCLASSEX);
			wcex.style = CS_HREDRAW | CS_VREDRAW;
			wcex.lpfnWndProc = DefWindowProc;
			wcex.cbClsExtra = 0;
			wcex.cbWndExtra = 0;
			wcex.hInstance = GetModuleHandle(NULL);
			wcex.hIcon = NULL;
			wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
			wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
			wcex.lpszMenuName = NULL;
			wcex.lpszClassName = _T("OpenMPTPluginBridge");
			wcex.hIconSm = NULL;

			if(!RegisterClassEx(&wcex))
			{
				msg->result = 0;
				return;
			}

			ptr = window = CreateWindow(
				_T("OpenMPTPluginBridge"),
				_T("OpenMPT Plugin Bridge"),
				WS_VISIBLE | WS_POPUP,
				CW_USEDEFAULT, CW_USEDEFAULT,
				windowSize.right - windowSize.left, windowSize.bottom- windowSize.top,
				/*reinterpret_cast<HWND>(msg->ptr)*/ NULL,
				NULL,
				wcex.hInstance,
				NULL);
			//SetParent(window, reinterpret_cast<HWND>(msg->ptr));
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
		SendToHost(ErrorMsg(L"Exception in dispatch()!"));
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
		vst_strncpy(reinterpret_cast<char *>(msg + 1), &extraData[0], static_cast<size_t>(msg->ptr - 1));
		break;

	case effEditGetRect:
		// ERect** in [ptr]
		{
			ERect *rectPtr = *reinterpret_cast<ERect **>(&extraData[0]);
			if(rectPtr != nullptr)
			{
				memcpy(msg + 1, rectPtr, std::min<size_t>(sizeof(ERect), static_cast<size_t>(msg->ptr)));
				windowSize = *rectPtr;
			}
		}
		break;

	case effGetChunk:
		// void** in [ptr] for chunk data address
		{
			void *chunkPtr = *reinterpret_cast<void **>(&extraData[0]);
			if(chunkPtr != nullptr)
			{
				memcpy(msg + 1, *reinterpret_cast<void **>(&extraData[0]), std::min(static_cast<size_t>(msg->result), static_cast<size_t>(msg->ptr)));
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
		SendToHost(ErrorMsg(L"Exception in setParameter()!"));
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
		SendToHost(ErrorMsg(L"Exception in getParameter()!"));
	}
}


// Audio rendering thread
DWORD WINAPI PluginBridge::RenderThread(LPVOID param)
{
	PluginBridge *that = static_cast<PluginBridge *>(param);

	const HANDLE objects[] = { that->sharedMem.sigProcess.send, that->sharedMem.sigThreadExit };
	DWORD result = 0;
	do
	{
		result = WaitForMultipleObjects(CountOf(objects), objects, FALSE, INFINITE);
		if(result == WAIT_OBJECT_0)
		{
			ProcessMsg *msg = static_cast<ProcessMsg *>(that->sharedMem.sampleBufPtr);
			switch(msg->processType)
			{
			case ProcessMsg::process:
				that->Process();
				break;
			case ProcessMsg::processReplacing:
				that->ProcessReplacing();
				break;
			case ProcessMsg::processDoubleReplacing:
				that->ProcessDoubleReplacing();
				break;
			}
			that->sharedMem.sigProcess.Confirm();
		}
	} while(result != WAIT_OBJECT_0 + 1 && result != WAIT_OBJECT_0 + 2 && result != WAIT_FAILED);
	CloseHandle(that->sharedMem.sigThreadExit);
	return 0;
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
			SendToHost(ErrorMsg(L"Exception in process()!"));
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
			SendToHost(ErrorMsg(L"Exception in processReplacing()!"));
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
			SendToHost(ErrorMsg(L"Exception in processDoubleReplacing()!"));
		}
	}
}


// Helper function to build the pointer arrays required by the VST process functions.
template<typename buf_t>
int32_t PluginBridge::BuildProcessPointers(buf_t **(&inPointers), buf_t **(&outPointers))
{
	ProcessMsg *msg = static_cast<ProcessMsg *>(sharedMem.sampleBufPtr);
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
	case audioMasterGetTime:
		// VstTimeInfo* in [return value]
		ptrOut = sizeof(VstTimeInfo);
		break;

	case audioMasterProcessEvents:
		// VstEvents* in [ptr]
		TranslateVSTEventsToBridge(dispatchData, static_cast<VstEvents *>(ptr), sharedMem.otherPtrSize);
		ptrOut = dispatchData.size() - sizeof(DispatchMsg);
		break;

	case audioMasterIOChanged:
		// We need to be sure that the new values are known to the master.
		UpdateEffectStruct();
		break;

	case audioMasterSizeWindow:
		if(window)
		{
			SetWindowPos(window, NULL, 0, 0, index, value, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
		}
		break;

	case audioMasterGetPreviousPlug:
	case audioMasterGetNextPlug:
		// Don't even bother, this would explode :)
		return 0;

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

	case audioMasterCanDo:
		// Name in [ptr]
		{
			ptrOut = strlen(ptrC) + 1;
			dispatchData.insert(dispatchData.end(), ptrC, ptrC + ptrOut);
		}

	case audioMasterGetDirectory:
		// Name in [return value]
		ptrOut = 256;
		break;

	case audioMasterOpenFileSelector:
	case audioMasterCloseFileSelector:
		// VstFileSelect* in [ptr]
		// TODO
		ptrOut = sizeof(VstFileSelect);
		break;

	default:
		if(ptr != nullptr) DebugBreak();
	}

	if(ptrOut != 0)
	{
		// In case we only reserve space and don't copy stuff over...
		dispatchData.resize(sizeof(DispatchMsg) + static_cast<size_t>(ptrOut), 0);
	}
	
	//std::cout << "about to dispatch " << opcode << " to host...";
	//std::flush(std::cout);
	const DispatchMsg *resultMsg;
	{
		DispatchMsg *msg = reinterpret_cast<DispatchMsg *>(&dispatchData[0]);
		new (msg) DispatchMsg(opcode, index, value, ptrOut, opt, static_cast<int32_t>(dispatchData.size() - sizeof(DispatchMsg)));
		resultMsg = static_cast<const DispatchMsg *>(SendToHost(*msg));
	}
	//std::cout << "done." << std::endl;

	if(resultMsg == nullptr)
	{
		return 0;
	}

	const char *extraData = reinterpret_cast<const char*>(resultMsg + 1);
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
