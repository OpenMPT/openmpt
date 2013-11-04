#include "stdafx.h"
#include "BridgeWrapper.h"
#include "misc_util.h"
#include "../mptrack/Mptrack.h"
#include "../mptrack/Vstplug.h"
#include <fstream>


// Check whether we need to load a 32-bit or 64-bit wrapper.
BridgeWrapper::BinaryType BridgeWrapper::GetPluginBinaryType(const char *pluginPath)
{
	BinaryType type = binUnknown;
	std::ifstream file(pluginPath, std::ios::in | std::ios::binary);
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


BridgeWrapper::BridgeWrapper(const char *pluginPath)
{
	BinaryType binType;
	if((binType = GetPluginBinaryType(pluginPath)) == binUnknown)
	{
		return;
	}

	static int32 plugId = 0;
	const DWORD procId = GetCurrentProcessId();
	std::string exeName = std::string(theApp.GetAppDirPath()) + (binType == bin64Bit ? "PluginBridge64.exe" : "PluginBridge32.exe");
	std::string mapName = "Local\\openmpt-" + Stringify(procId) + "-" + Stringify(plugId++);

	sharedMem.writeOffset = sizeof(MsgQueue) + 512 * 1024;
	sharedMem.otherPtrSize = binType / 8;

	sharedMem.sigToHost.Create();
	sharedMem.sigToBridge.Create();
	sharedMem.sigProcess.Create();
	sharedMem.sigThreadExit = CreateEvent(NULL, FALSE, FALSE, NULL);

	// Parameters: Name for shared memory object, parent process ID, signals
	char cmdLine[64];
	sprintf(cmdLine, "%s %d %d %d %d %d %d %d", mapName.c_str(), procId, sharedMem.sigToHost.send, sharedMem.sigToHost.ack, sharedMem.sigToBridge.send, sharedMem.sigToBridge.ack, sharedMem.sigProcess.send, sharedMem.sigProcess.ack);

	STARTUPINFO info;
	MemsetZero(info);
	info.cb = sizeof(info);
	PROCESS_INFORMATION processInfo;
	MemsetZero(processInfo);

	bool success = false;
	if(CreateProcess(exeName.c_str(), cmdLine, NULL, NULL, TRUE, /*CREATE_NO_WINDOW */0, NULL, NULL, &info, &processInfo))
	{
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
	}
	if(success)
	{
		sharedMem.mapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, TRUE, mapName.c_str());
		if(sharedMem.mapFile)
		{
			// TODO De-Dupe this code
			sharedMem.sharedPtr = static_cast<char *>(MapViewOfFile(sharedMem.mapFile, FILE_MAP_ALL_ACCESS, 0, 0, sharedMem.queueSize + sharedMem.sampleBufSize));
			sharedMem.effectPtr = reinterpret_cast<AEffect *>(sharedMem.sharedPtr);
			sharedMem.sampleBufPtr = sharedMem.sharedPtr + sizeof(AEffect64);
			sharedMem.queuePtr = sharedMem.sharedPtr + sharedMem.sampleBufSize;
			sharedMem.sampleBufSize -= sizeof(AEffect64);

			// Some opcodes might be dispatched before we get the chance to fill the struct out.
			sharedMem.effectPtr->object = this;

			// Initialize bridge
			struct InitEffect : public InitMsg
			{
				AEffect effectCopy;

				InitEffect(const wchar_t *pluginPath) : InitMsg(pluginPath) { }
			};

			const InitEffect *initEffect = static_cast<const InitEffect *>(SendToBridge(InitEffect(mpt::String::Decode(pluginPath, mpt::CharsetLocale).c_str())));

			if(initEffect == nullptr)
			{
				Reporting::Error("Could not initialize plugin bridge, it probably crashed.");
			} else if(initEffect->result != 1)
			{
				Reporting::Error(mpt::String::Encode(initEffect->str, mpt::CharsetLocale).c_str());
			} else
			{
				ASSERT(initEffect->type == MsgHeader::init);
				sharedMem.queueSize = initEffect->mapSize;

				sharedMem.effectPtr->object = this;
				sharedMem.effectPtr->dispatcher = DispatchToPlugin;
				sharedMem.effectPtr->setParameter = SetParameter;
				sharedMem.effectPtr->getParameter = GetParameter;
				sharedMem.effectPtr->process = Process;
				if(sharedMem.effectPtr->flags & effFlagsCanReplacing) sharedMem.effectPtr->processReplacing = ProcessReplacing;
				if(sharedMem.effectPtr->flags & effFlagsCanDoubleReplacing) sharedMem.effectPtr->processDoubleReplacing = ProcessDoubleReplacing;

				DWORD dummy;	// For Win9x
				CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&MessageThread, (LPVOID)this, 0, &dummy);
			}

		}
	}
}

BridgeWrapper::~BridgeWrapper()
{
	if(sharedMem.queuePtr != nullptr)
	{
		SendToBridge(MsgHeader(MsgHeader::close, sizeof(MsgHeader)));
	}

	CloseHandle(sharedMem.otherProcess);
	SetEvent(sharedMem.sigThreadExit);
}


DWORD WINAPI BridgeWrapper::MessageThread(LPVOID param)
{
	BridgeWrapper *that = static_cast<BridgeWrapper *>(param);

	const HANDLE objects[] = { that->sharedMem.sigToHost.send, that->sharedMem.otherProcess, that->sharedMem.sigThreadExit };
	DWORD result = 0;
	do
	{
		result = WaitForMultipleObjects(CountOf(objects), objects, FALSE, INFINITE);
		if(result == WAIT_OBJECT_0)
		{
			that->ParseNextMessage();
		}
	} while(result != WAIT_OBJECT_0 + 1 && result != WAIT_OBJECT_0 + 2 && result != WAIT_FAILED);
	ASSERT(result != WAIT_FAILED);
	CloseHandle(that->sharedMem.sigThreadExit);
	return 0;
}


// Receive a message from the host and translate it.
void BridgeWrapper::ParseNextMessage(MsgHeader *parentMessage)
{
	MsgHeader *msg;
	if(parentMessage == nullptr)
	{
		// Default: Grab next message
		MsgQueue *queue = static_cast<MsgQueue *>(sharedMem.queuePtr);
		uint32_t index = InterlockedExchangeAdd(&(queue->toHost.readIndex), 1) % CountOf(queue->toHost.offset);
		msg = reinterpret_cast<MsgHeader *>(static_cast<char *>(sharedMem.queuePtr) + queue->toHost.offset[index]);
	} else
	{
		// Read interrupt message
		msg = reinterpret_cast<MsgHeader *>(static_cast<char *>(sharedMem.queuePtr) + parentMessage->interruptOffset);
	}

	switch(msg->type)
	{
	case MsgHeader::dispatch:
		DispatchToHost(static_cast<DispatchMsg *>(msg));
		break;

	case MsgHeader::errorMsg:
		MessageBoxW(theApp.GetMainWnd()->m_hWnd, static_cast<ErrorMsg *>(msg)->str, L"OpenMPT Plugin Bridge", MB_ICONERROR);
		break;
	}

	if(parentMessage == nullptr)
		sharedMem.sigToHost.Confirm();
	else
		SetEvent(reinterpret_cast<HANDLE>(parentMessage->interruptSignal));	// TODO
}


void BridgeWrapper::DispatchToHost(DispatchMsg *msg)
{
	// Various dispatch data - depending on the opcode, one of those might be used.
	std::vector<char> extraData;
	size_t extraDataSize = 0;

	void *ptr = msg + 1;		// Content of ptr is usually stored right after the message header.

	switch(msg->opcode)
	{
	case audioMasterProcessEvents:
		// VstEvents* in [ptr]
		TranslateBridgeToVSTEvents(extraData, ptr);
		ptr = &extraData[0];
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


// Send an arbitrary message to the bridge.
// Returns a pointer to the message, as processed by the bridge.
const MsgHeader *BridgeWrapper::SendToBridge(const MsgHeader &msg)
{
	if(msg.size <= sharedMem.queueSize)
	{
		sharedMem.writeMutex.lock();
		if(sharedMem.writeOffset + msg.size > sharedMem.queueSize)
		{
			sharedMem.writeOffset = sizeof(MsgQueue) + 512 * 1024;
		}
		MsgHeader *addr = reinterpret_cast<MsgHeader *>(static_cast<char *>(sharedMem.queuePtr) + sharedMem.writeOffset);
		memcpy(addr, &msg, msg.size);

		MsgQueue *queue = static_cast<MsgQueue *>(sharedMem.queuePtr);
		queue->toBridge.offset[queue->toBridge.writeIndex++] = sharedMem.writeOffset;
		if(queue->toBridge.writeIndex >= CountOf(queue->toBridge.offset))
		{
			queue->toBridge.writeIndex = 0;
		}

		//sharedMem.writeMutex.unlock();

		sharedMem.sigToBridge.Send();

		if(msg.type == MsgHeader::dispatch && static_cast<const DispatchMsg &>(msg).opcode == effEditOpen)
		{sharedMem.writeMutex.unlock();
		return addr;}

		// Wait until we get the result from the bridge.
		const HANDLE objects[] = { sharedMem.sigToBridge.ack, sharedMem.otherProcess, sharedMem.sigToHost.send };
		DWORD result;
		do
		{
			result = WaitForMultipleObjects(CountOf(objects), objects, FALSE, INFINITE);
			if(result == WAIT_OBJECT_0 + 2)
			{
				ParseNextMessage();
			}
		} while(result != WAIT_OBJECT_0 && result != WAIT_OBJECT_0 + 1);
		if(result == WAIT_OBJECT_0 + 1)
		{
			SetEvent(sharedMem.sigThreadExit);
		}
		sharedMem.writeMutex.unlock();
		return (result == WAIT_OBJECT_0) ? addr : nullptr;
	}
	return nullptr;
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

	const DispatchMsg *resultMsg;
	{
		DispatchMsg *msg = reinterpret_cast<DispatchMsg *>(&dispatchData[0]);
		new (msg) DispatchMsg(opcode, index, value, ptrOut, opt, static_cast<int32_t>(dispatchData.size() - sizeof(DispatchMsg)));
		resultMsg = static_cast<const DispatchMsg *>(that->SendToBridge(*msg));
	}

	if(resultMsg == nullptr)
	{
		return 0;
	}

	const char *extraData = reinterpret_cast<const char *>(resultMsg + 1);
	// Post-fix some opcodes
	switch(opcode)
	{
	case effClose:
		effect->object = nullptr;
		delete that;
		break;

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
			// Now that we know the chunk size, allocate enough memory for it.
			dispatchData.resize(sizeof(DispatchMsg) + static_cast<size_t>(resultMsg->result));
			DispatchMsg *msg = reinterpret_cast<DispatchMsg *>(&dispatchData[0]);
			new (msg) DispatchMsg(opcode, index, value, resultMsg->result, opt, static_cast<int32_t>(resultMsg->result));
			resultMsg = static_cast<const DispatchMsg *>(that->SendToBridge(*msg));
			if(resultMsg != nullptr)
			{
				*static_cast<void **>(ptr) = const_cast<DispatchMsg *>(resultMsg + 1);
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
		that->SendToBridge(ParameterMsg(index, parameter));
	}
}


float VSTCALLBACK BridgeWrapper::GetParameter(AEffect *effect, VstInt32 index)
{
	BridgeWrapper *that = static_cast<BridgeWrapper *>(effect->object);
	if(that)
	{
		const ParameterMsg *msg = static_cast<const ParameterMsg *>(that->SendToBridge(ParameterMsg(index)));
		if(msg != nullptr) return msg->value;
	}
	return 0.0f;
}


template<typename buf_t>
void BridgeWrapper::BuildProcessBuffer(ProcessMsg::ProcessType type, VstInt32 numInputs, VstInt32 numOutputs, buf_t **inputs, buf_t **outputs, VstInt32 sampleFrames)
{
	new (sharedMem.sampleBufPtr) ProcessMsg(type, numInputs, numOutputs, sampleFrames);

	buf_t *ptr = reinterpret_cast<buf_t *>(static_cast<ProcessMsg *>(sharedMem.sampleBufPtr) + 1);
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


void VSTCALLBACK BridgeWrapper::Process(AEffect *effect, float **inputs, float **outputs, VstInt32 sampleFrames)
{
	// TODO memory size must probably grow!
	BridgeWrapper *that = static_cast<BridgeWrapper *>(effect->object);
	if(sampleFrames != 0 && that != nullptr)
	{
		that->BuildProcessBuffer(ProcessMsg::process, effect->numInputs, effect->numOutputs, inputs, outputs, sampleFrames);
	}
}


void VSTCALLBACK BridgeWrapper::ProcessReplacing(AEffect *effect, float **inputs, float **outputs, VstInt32 sampleFrames)
{
	// TODO memory size must probably grow!
	BridgeWrapper *that = static_cast<BridgeWrapper *>(effect->object);
	if(sampleFrames != 0 && that != nullptr)
	{
		that->BuildProcessBuffer(ProcessMsg::processReplacing, effect->numInputs, effect->numOutputs, inputs, outputs, sampleFrames);
	}
}


void VSTCALLBACK BridgeWrapper::ProcessDoubleReplacing(AEffect *effect, double **inputs, double **outputs, VstInt32 sampleFrames)
{
	// TODO memory size must probably grow!
	BridgeWrapper *that = static_cast<BridgeWrapper *>(effect->object);
	if(sampleFrames != 0 && that != nullptr)
	{
		that->BuildProcessBuffer(ProcessMsg::processDoubleReplacing, effect->numInputs, effect->numOutputs, inputs, outputs, sampleFrames);
	}
}
