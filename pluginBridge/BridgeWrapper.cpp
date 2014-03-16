#include "stdafx.h"
#include "BridgeWrapper.h"
#include "misc_util.h"
#include "../mptrack/Mptrack.h"
#include "../mptrack/Vstplug.h"
#include "../common/mptFstream.h"
#include "../common/thread.h"


// Check whether we need to load a 32-bit or 64-bit wrapper.
BridgeWrapper::BinaryType BridgeWrapper::GetPluginBinaryType(const mpt::PathString &pluginPath)
{
	BinaryType type = binUnknown;
	mpt::ifstream file(pluginPath, std::ios::in | std::ios::binary);
	if(file.is_open())
	{
		IMAGE_DOS_HEADER dosHeader;
		IMAGE_NT_HEADERS ntHeader;
		file.read(reinterpret_cast<char *>(&dosHeader), sizeof(dosHeader));
		if(dosHeader.e_magic == IMAGE_DOS_SIGNATURE)
		{
			file.seekg(dosHeader.e_lfanew);
			file.read(reinterpret_cast<char *>(&ntHeader), sizeof(ntHeader));

			ASSERT((ntHeader.FileHeader.Characteristics & IMAGE_FILE_DLL) != 0);
			if(ntHeader.FileHeader.Machine == IMAGE_FILE_MACHINE_I386)
				type = bin32Bit;
			else if(ntHeader.FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64)
				type = bin64Bit;
		}
	}
	return type;
}


BridgeWrapper::BridgeWrapper(const mpt::PathString &pluginPath)
{
	BinaryType binType;
	if((binType = GetPluginBinaryType(pluginPath)) == binUnknown)
	{
		return;
	}

	static uint32 plugId = 0;
	plugId++;
	const DWORD procId = GetCurrentProcessId();
	const mpt::PathString exeName = theApp.GetAppDirPath() + (binType == bin64Bit ? MPT_PATHSTRING("PluginBridge64.exe") : MPT_PATHSTRING("PluginBridge32.exe"));
	const std::wstring mapName = L"Local\\openmpt-" + mpt::ToWString(procId) + L"-" + mpt::ToWString(plugId);

	// Create our shared memory object.
	if(!sharedMem.queueMem.Create(mapName.c_str(), sizeof(AEffect64) + sizeof(MsgQueue)))
	{
		Reporting::Error("Could not initialize plugin bridge memory.");
		return;
	}

	sharedMem.otherPtrSize = binType / 8;

	sharedMem.sigToHost.Create();
	sharedMem.sigToBridge.Create();
	sharedMem.sigProcess.Create();
	sharedMem.sigThreadExit = CreateEvent(NULL, FALSE, FALSE, NULL);

	//std::wstring pipeName = L"\\\\.\\pipe\\openmpt-" + mpt::ToWString(procId) + L"-" + mpt::ToWString(plugId);
	//sharedMem.extraMemPipe.Create((pipeName).c_str());

	// Command-line must be a modfiable string...
	wchar_t cmdLine[128];
	//wcscpy(cmdLine, mapName.c_str());
	//swprintf(cmdLine, CountOf(cmdLine), L"%s %s", pipeName.c_str(), mapName.c_str());
	// Parameters: Name for shared memory object, parent process ID, signals
	swprintf(cmdLine, CountOf(cmdLine), L"%s %d %d %d %d %d %d %d", mapName.c_str(), procId, sharedMem.sigToHost.send, sharedMem.sigToHost.ack, sharedMem.sigToBridge.send, sharedMem.sigToBridge.ack, sharedMem.sigProcess.send, sharedMem.sigProcess.ack);

	STARTUPINFOW info;
	MemsetZero(info);
	info.cb = sizeof(info);
	PROCESS_INFORMATION processInfo;
	MemsetZero(processInfo);

	// TODO un-share handles
	if(!CreateProcessW(exeName.AsNative().c_str(), cmdLine, NULL, NULL, TRUE, /*CREATE_NO_WINDOW */0, NULL, NULL, &info, &processInfo))
	{
		Reporting::Error("Failed to launch plugin bridge.");
		return;
	}

	sharedMem.otherProcess = processInfo.hProcess;

	// Initialize bridge
	sharedMem.SetupPointers();
	sharedMem.effectPtr->object = this;
	sharedMem.effectPtr->dispatcher = DispatchToPlugin;
	sharedMem.effectPtr->setParameter = SetParameter;
	sharedMem.effectPtr->getParameter = GetParameter;
	sharedMem.effectPtr->process = Process;

	if(WaitForSingleObject(sharedMem.sigToHost.ack, 5000) != WAIT_OBJECT_0)
	{
		Reporting::Error("Plugin bridge timed out.");
		return;
	}

	mpt::thread_member<BridgeWrapper, &BridgeWrapper::MessageThread>(this);

	BridgeMessage initMsg;
	initMsg.Init(pluginPath.ToWide().c_str(), MIXBUFFERSIZE);
	const BridgeMessage *initResult = SendToBridge(initMsg);

	if(initResult == nullptr)
	{
		Reporting::Error("Could not initialize plugin bridge, it probably crashed.");
	} else if(initResult->init.result != 1)
	{
		Reporting::Error(mpt::ToLocale(initResult->init.str).c_str());
	} else
	{
		if(sharedMem.effectPtr->flags & effFlagsCanReplacing) sharedMem.effectPtr->processReplacing = ProcessReplacing;
		if(sharedMem.effectPtr->flags & effFlagsCanDoubleReplacing) sharedMem.effectPtr->processDoubleReplacing = ProcessDoubleReplacing;
		return;
	}

	sharedMem.queueMem.Close();
	/*	{
		sharedMem.messagePipe.WaitForClient();
		//sharedMem.messagePipe.Write(&BridgeProtocolVersion, sizeof(BridgeProtocolVersion));

		sharedMem.otherProcess = processInfo.hProcess;
		const HANDLE objects[] = { sharedMem.sigToHost.ack, sharedMem.otherProcess };
		DWORD result = WaitForMultipleObjects(CountOf(objects), objects, FALSE, 10000);
		if(result == WAIT_OBJECT_0)
		{
			success = true;
		} else if(result == WAIT_OBJECT_0 + 1)
		{
			// Process died
		}
		CloseHandle(processInfo.hThread);
	}*/
}


BridgeWrapper::~BridgeWrapper()
{
	if(sharedMem.queueMem.Good())
	{
		BridgeMessage close;
		close.Close();
		SendToBridge(close);
	}

	CloseHandle(sharedMem.otherProcess);
	SetEvent(sharedMem.sigThreadExit);
}


void BridgeWrapper::MessageThread()
{
	sharedMem.msgThreadID = GetCurrentThreadId();

	const HANDLE objects[] = { sharedMem.sigToHost.send, sharedMem.sigToBridge.ack, sharedMem.otherProcess, sharedMem.sigThreadExit };
	DWORD result = 0;
	do
	{
		result = WaitForMultipleObjects(CountOf(objects), objects, FALSE, INFINITE);
		if(result == WAIT_OBJECT_0)
		{
			ParseNextMessage();
		} else if(result == WAIT_OBJECT_0 + 1)
		{
			// Message got answered
			for(size_t i = 0; i < CountOf(sharedMem.queuePtr->toBridge); i++)
			{
				BridgeMessage &msg = sharedMem.queuePtr->toBridge[i];
				if(InterlockedExchangeAdd(&msg.header.status, 0) == MsgHeader::done)
				{
					InterlockedExchange(&msg.header.status, MsgHeader::empty);
					sharedMem.ackSignals[msg.header.signalID].Confirm();
				}
			}
		} else if(result == WAIT_OBJECT_0 + 2)
		{
			// Something failed
			for(size_t i = 0; i < CountOf(sharedMem.queuePtr->toBridge); i++)
			{
				BridgeMessage &msg = sharedMem.queuePtr->toBridge[i];
				sharedMem.ackSignals[msg.header.signalID].Send();
			}
		}
	} while(result != WAIT_OBJECT_0 + 2 && result != WAIT_OBJECT_0 + 3 && result != WAIT_FAILED);
	ASSERT(result != WAIT_FAILED);
	CloseHandle(sharedMem.sigThreadExit);
}


// Send an arbitrary message to the bridge.
// Returns a pointer to the message, as processed by the bridge.
const BridgeMessage *BridgeWrapper::SendToBridge(const BridgeMessage &msg)
{
	const bool inMsgThread = GetCurrentThreadId() == sharedMem.msgThreadID;
	BridgeMessage *addr = sharedMem.CopyToSharedMemory(msg, sharedMem.queuePtr->toBridge, inMsgThread);
	sharedMem.sigToBridge.Send();

	// Wait until we get the result from the bridge.
	DWORD result;
	if(inMsgThread)
	{
		// Since this is the message thread, we must handle messages directly.
		const HANDLE objects[] = { sharedMem.sigToBridge.ack, sharedMem.sigToHost.send, sharedMem.otherProcess };
		do
		{
			result = WaitForMultipleObjects(CountOf(objects), objects, FALSE, INFINITE);
			if(result == WAIT_OBJECT_0)
			{
				// Message got answered
				const bool done = (InterlockedExchangeAdd(&addr->header.status, 0) == MsgHeader::done);
				for(size_t i = 0; i < CountOf(sharedMem.queuePtr->toBridge); i++)
				{
					BridgeMessage &msg = sharedMem.queuePtr->toBridge[i];
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
		} while(result != WAIT_OBJECT_0 + 2 && result != WAIT_FAILED);
		if(result == WAIT_OBJECT_0 + 2)
		{
			SetEvent(sharedMem.sigThreadExit);
		}
	} else
	{
		// Wait until the message thread notifies us.
		Signal &ackHandle = sharedMem.ackSignals[addr->header.signalID];
		const HANDLE objects[] = { ackHandle.ack, ackHandle.send };
		result = WaitForMultipleObjects(CountOf(objects), objects, FALSE, INFINITE);
	}

	return (result == WAIT_OBJECT_0) ? addr : nullptr;
}


// Receive a message from the host and translate it.
void BridgeWrapper::ParseNextMessage()
{
	ASSERT(GetCurrentThreadId() == sharedMem.msgThreadID);
	for(size_t i = 0; i < CountOf(sharedMem.queuePtr->toHost); i++)
	{
		BridgeMessage *msg = sharedMem.queuePtr->toHost + i;
		if(InterlockedExchangeAdd(&msg->header.status, 0) == MsgHeader::sent)
		{
			InterlockedExchange(&msg->header.status, MsgHeader::received);
			switch(msg->header.type)
			{
			case MsgHeader::dispatch:
				DispatchToHost(&msg->dispatch);
				break;

			case MsgHeader::errorMsg:
				// TODO will deadlock as the main thread is in a waiting state
				// To reproduce: setChunk with byte size = 0 for otiumfx basslane
				Reporting::Error(msg->error.str, L"OpenMPT Plugin Bridge");
				break;
			}

			InterlockedExchange(&msg->header.status, MsgHeader::done);
			sharedMem.sigToHost.Confirm();
		}
	}
}


void BridgeWrapper::DispatchToHost(DispatchMsg *msg)
{
	// Various dispatch data - depending on the opcode, one of those might be used.
	std::vector<char> extraData;
	size_t extraDataSize = 0;

	MappedMemory auxMem;

	// Content of ptr is usually stored right after the message header, ptr field indicates size.
	void *ptr = (msg->ptr != 0) ? (msg + 1) : nullptr;
	if(msg->size > sizeof(BridgeMessage))
	{
		if(!auxMem.Open(L"Local\\foo2"))
		{
			return;
		}
		ptr = auxMem.view;
	}

	switch(msg->opcode)
	{
	case audioMasterProcessEvents:
		// VstEvents* in [ptr]
		TranslateBridgeToVSTEvents(extraData, ptr);
		ptr = &extraData[0];
		break;

	case audioMasterIOChanged:
		// Set up new processing file
		sharedMem.processMem.Open(reinterpret_cast<wchar_t *>(ptr));
		break;

		//case audioMasterOpenFileSelector:
		//case audioMasterCloseFileSelector:
		// TODO: Translate the structs
	}

	if(extraDataSize != 0)
	{
		extraData.resize(extraDataSize, 0);
		ptr = &extraData[0];
	}

	msg->result = CVstPluginManager::MasterCallBack(sharedMem.effectPtr, msg->opcode, msg->index, static_cast<VstIntPtr>(msg->value), ptr, msg->opt);

	// Post-fix some opcodes
	switch(msg->opcode)
	{
	case audioMasterGetTime:
		// VstTimeInfo* in [return value]
		if(msg->result != 0)
		{
			ASSERT(static_cast<size_t>(msg->ptr) >= sizeof(VstTimeInfo));
			memcpy(ptr, reinterpret_cast<void *>(msg->result), std::min(sizeof(VstTimeInfo), static_cast<size_t>(msg->ptr)));
		}
		break;

	case audioMasterGetDirectory:
		// char* in [return value]
		if(msg->result != 0)
		{
			char *target = static_cast<char *>(ptr);
			strncpy(target, reinterpret_cast<const char *>(msg->result), static_cast<size_t>(msg->ptr - 1));
			target[msg->ptr - 1] = 0;
		}
		break;
	}
}


VstIntPtr VSTCALLBACK BridgeWrapper::DispatchToPlugin(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt)
{
	BridgeWrapper *that = static_cast<BridgeWrapper *>(effect->object);

	std::vector<char> dispatchData(sizeof(DispatchMsg), 0);
	int64_t ptrOut = 0;
	bool copyPtrBack = false, ptrIsSize = true;
	char *ptrC = static_cast<char *>(ptr);

	switch(opcode)
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
		ptrOut = 256;
		copyPtrBack = true;
		break;

	case effSetProgramName:
	case effCanDo:
		// char* in [ptr]
		ptrOut = strlen(ptrC) + 1;
		dispatchData.insert(dispatchData.end(), ptrC, ptrC + ptrOut);
		break;

	case effEditGetRect:
		// ERect** in [ptr]
		ptrOut = sizeof(ERect);
		copyPtrBack = true;
		break;

	case effEditOpen:
		// HWND in [ptr] - Note: Window handles are interoperable between 32-bit and 64-bit applications in Windows (http://msdn.microsoft.com/en-us/library/windows/desktop/aa384203%28v=vs.85%29.aspx)
		ptrOut = reinterpret_cast<int64_t>(ptr);
		ptrIsSize = false;
		break;

	case effGetChunk:
		// void** in [ptr] for chunk data address
		{
			static uint32_t chunkId = 0;
			const std::wstring mapName = L"Local\\openmpt-" + mpt::ToWString(GetCurrentProcessId()) + L"-chunkdata-" + mpt::ToWString(chunkId++);
			const char *str = reinterpret_cast<const char *>(mapName.c_str());
			dispatchData.insert(dispatchData.end(), str, str + mapName.length() * sizeof(wchar_t));
		}
		break;

	case effSetChunk:
		// void* in [ptr] for chunk data
		ptrOut = value;
		dispatchData.insert(dispatchData.end(), ptrC, ptrC + value);
		break;

	case effProcessEvents:
		// VstEvents* in [ptr]
		TranslateVSTEventsToBridge(dispatchData, static_cast<VstEvents *>(ptr), that->sharedMem.otherPtrSize);
		ptrOut = dispatchData.size() - sizeof(DispatchMsg);
		break;

	case effGetInputProperties:
	case effGetOutputProperties:
		// VstPinProperties* in [ptr]
		ptrOut = sizeof(VstPinProperties);
		copyPtrBack = true;
		break;

	case effOfflineNotify:
		// VstAudioFile* in [ptr]
		ptrOut = sizeof(VstAudioFile) * value;
		// TODO
		break;

	case effOfflinePrepare:
	case effOfflineRun:
		// VstOfflineTask* in [ptr]
		ptrOut = sizeof(VstOfflineTask) * value;
		// TODO
		break;

	case effProcessVarIo:
		// VstVariableIo* in [ptr]
		ptrOut = sizeof(VstVariableIo);
		// TODO
		break;

	case effSetSpeakerArrangement:
		// VstSpeakerArrangement* in [value] and [ptr]
		ptrOut = sizeof(VstSpeakerArrangement) * 2;
		PushToVector(dispatchData, *static_cast<VstSpeakerArrangement *>(ptr));
		PushToVector(dispatchData, *FromVstPtr<VstSpeakerArrangement>(value));
		break;

	case effGetParameterProperties:
		// VstParameterProperties* in [ptr]
		ptrOut = sizeof(VstParameterProperties);
		copyPtrBack = true;
		break;

	case effGetMidiProgramName:
	case effGetCurrentMidiProgram:
		// MidiProgramName* in [ptr]
		ptrOut = sizeof(MidiProgramName);
		copyPtrBack = true;
		break;

	case effGetMidiProgramCategory:
		// MidiProgramCategory* in [ptr]
		ptrOut = sizeof(MidiProgramCategory);
		copyPtrBack = true;
		break;

	case effGetMidiKeyName:
		// MidiKeyName* in [ptr]
		ptrOut = sizeof(MidiKeyName);
		copyPtrBack = true;
		break;

	case effGetSpeakerArrangement:
		// VstSpeakerArrangement* in [value] and [ptr]
		ptrOut = sizeof(VstSpeakerArrangement) * 2;
		copyPtrBack = true;
		break;

	case effBeginLoadBank:
	case effBeginLoadProgram:
		// VstPatchChunkInfo* in [ptr]
		ptrOut = sizeof(VstPatchChunkInfo);
		break;

	default:
		ASSERT(ptr == nullptr);
	}

	if(ptrOut != 0 && ptrIsSize)
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
		if(auxMem.Create(L"Local\\foo", extraSize))
		{
			memcpy(auxMem.view, &dispatchData[sizeof(DispatchMsg)], extraSize);
			dispatchData.resize(sizeof(DispatchMsg));
		} else
		{
			return 0;
		}
	}
	
	const DispatchMsg *resultMsg;
	{
		BridgeMessage *msg = reinterpret_cast<BridgeMessage *>(&dispatchData[0]);
		msg->Dispatch(opcode, index, value, ptrOut, opt, extraSize);
		resultMsg = &(that->SendToBridge(*msg)->dispatch);
	}

	if(resultMsg == nullptr)
	{
		return 0;
	}

	const char *extraData = dispatchData.size() == sizeof(DispatchMsg) ? static_cast<const char *>(auxMem.view) : reinterpret_cast<const char *>(resultMsg + 1);
	// Post-fix some opcodes
	switch(opcode)
	{
	case effClose:
		effect->object = nullptr;
		delete that;
		return 0;

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
		strcpy(ptrC, extraData);
		break;

	case effEditGetRect:
		// ERect** in [ptr]
		*static_cast<const ERect **>(ptr) = reinterpret_cast<const ERect *>(extraData);
		break;

	case effGetChunk:
		// void** in [ptr] for chunk data address
		{
			const wchar_t *str = reinterpret_cast<const wchar_t *>(extraData);
			if(that->sharedMem.getChunkMem.Open(str))
			{
				*static_cast<void **>(ptr) = that->sharedMem.getChunkMem.view;
			} else
			{
				return 0;
			}
		}
		break;

	case effGetSpeakerArrangement:
		// VstSpeakerArrangement* in [value] and [ptr]
		*static_cast<VstSpeakerArrangement *>(ptr) = *reinterpret_cast<const VstSpeakerArrangement *>(extraData);
		*FromVstPtr<VstSpeakerArrangement>(value) = *(reinterpret_cast<const VstSpeakerArrangement *>(extraData) + 1);
		break;

	default:
		// TODO: Translate VstVariableIo, offline tasks
		if(copyPtrBack)
		{
			memcpy(ptr, extraData, static_cast<size_t>(ptrOut));
		}
	}

	return static_cast<VstIntPtr>(resultMsg->result);
}


void VSTCALLBACK BridgeWrapper::SetParameter(AEffect *effect, VstInt32 index, float parameter)
{
	BridgeWrapper *that = static_cast<BridgeWrapper *>(effect->object);
	if(that)
	{
		BridgeMessage msg;
		msg.SetParameter(index, parameter);
		that->SendToBridge(msg);
	}
}


float VSTCALLBACK BridgeWrapper::GetParameter(AEffect *effect, VstInt32 index)
{
	BridgeWrapper *that = static_cast<BridgeWrapper *>(effect->object);
	if(that)
	{
		BridgeMessage msg;
		msg.GetParameter(index);
		const ParameterMsg *resultMsg = &(that->SendToBridge(msg)->parameter);
		if(resultMsg != nullptr) return resultMsg->value;
	}
	return 0.0f;
}


void VSTCALLBACK BridgeWrapper::Process(AEffect *effect, float **inputs, float **outputs, VstInt32 sampleFrames)
{
	BridgeWrapper *that = static_cast<BridgeWrapper *>(effect->object);
	if(sampleFrames != 0 && that != nullptr)
	{
		that->BuildProcessBuffer(ProcessMsg::process, effect->numInputs, effect->numOutputs, inputs, outputs, sampleFrames);
	}
}


void VSTCALLBACK BridgeWrapper::ProcessReplacing(AEffect *effect, float **inputs, float **outputs, VstInt32 sampleFrames)
{
	BridgeWrapper *that = static_cast<BridgeWrapper *>(effect->object);
	if(sampleFrames != 0 && that != nullptr)
	{
		that->BuildProcessBuffer(ProcessMsg::processReplacing, effect->numInputs, effect->numOutputs, inputs, outputs, sampleFrames);
	}
}


void VSTCALLBACK BridgeWrapper::ProcessDoubleReplacing(AEffect *effect, double **inputs, double **outputs, VstInt32 sampleFrames)
{
	BridgeWrapper *that = static_cast<BridgeWrapper *>(effect->object);
	if(sampleFrames != 0 && that != nullptr)
	{
		that->BuildProcessBuffer(ProcessMsg::processDoubleReplacing, effect->numInputs, effect->numOutputs, inputs, outputs, sampleFrames);
	}
}


template<typename buf_t>
void BridgeWrapper::BuildProcessBuffer(ProcessMsg::ProcessType type, VstInt32 numInputs, VstInt32 numOutputs, buf_t **inputs, buf_t **outputs, VstInt32 sampleFrames)
{
	ASSERT(sharedMem.processMem.Good());
	new (sharedMem.processMem.view) ProcessMsg(type, numInputs, numOutputs, sampleFrames);

	buf_t *ptr = reinterpret_cast<buf_t *>(static_cast<ProcessMsg *>(sharedMem.processMem.view) + 1);
	for(VstInt32 i = 0; i < numInputs; i++)
	{
		memcpy(ptr, inputs[i], sampleFrames * sizeof(buf_t));
		ptr += sampleFrames;
	}
	// Theoretically, we should memcpy() instead of memset() here in process(), but OpenMPT always clears the output buffer before processing so it doesn't matter.
	memset(ptr, 0, numOutputs * sampleFrames * sizeof(buf_t));

	sharedMem.sigProcess.Send();
	const HANDLE objects[] = { sharedMem.sigProcess.ack, sharedMem.otherProcess };
	WaitForMultipleObjects(CountOf(objects), objects, FALSE, INFINITE);

	for(VstInt32 i = 0; i < numOutputs; i++)
	{
		//memcpy(outputs[i], ptr, sampleFrames * sizeof(buf_t));
		outputs[i] = ptr;	// Exactly what you don't want plugins to do usually (bend your output pointers)... muahahaha!
		ptr += sampleFrames;
	}
}