/*
 * BridgeWrapper.cpp
 * -----------------
 * Purpose: VST plugin bridge wrapper (host side)
 * Notes  : (currently none)
 * Authors: Johannes Schultz (OpenMPT Devs)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#ifndef NO_VST
#include "BridgeWrapper.h"
#include "../soundlib/plugins/PluginManager.h"
#include "../mptrack/Mptrack.h"
#include "../mptrack/Vstplug.h"
#include "../mptrack/ExceptionHandler.h"
#include "../common/mptFileIO.h"
#include "../common/thread.h"
#include "../common/StringFixer.h"
#include "../common/misc_util.h"


OPENMPT_NAMESPACE_BEGIN


ComponentPluginBridge::ComponentPluginBridge(int bitness)
	: ComponentBase(ComponentTypeBundled)
	, bitness(bitness)
	, availability(AvailabilityUnknown)
{
	return;
}


bool ComponentPluginBridge::DoInitialize()
{
	if(bitness != 32 && bitness != 64)
	{
		return false;
	}
	exeName = theApp.GetAppDirPath();
	if(bitness == 32)
	{
		exeName += MPT_PATHSTRING("PluginBridge32.exe");
	}
	if(bitness == 64)
	{
		exeName += MPT_PATHSTRING("PluginBridge64.exe");
	}
	// First, check for validity of the bridge executable.
	if(!exeName.IsFile())
	{
		availability = AvailabilityMissing;
		return false;
	}
	std::vector<WCHAR> exePath(MAX_PATH);
	while(GetModuleFileNameW(0, &exePath[0], exePath.size()) >= exePath.size())
	{
		exePath.resize(exePath.size() * 2);
	}
	uint64 mptVersion = BridgeWrapper::GetFileVersion(&exePath[0]);
	uint64 bridgeVersion = BridgeWrapper::GetFileVersion(exeName.AsNative().c_str());
	if(bridgeVersion != mptVersion)
	{
		availability = AvailabilityWrongVersion;
		return false;
	}
	availability = AvailabilityOK;
	return true;
}


MPT_REGISTERED_COMPONENT(ComponentPluginBridge32, "PluginBridge32")

MPT_REGISTERED_COMPONENT(ComponentPluginBridge64, "PluginBridge64")


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


uint64 BridgeWrapper::GetFileVersion(const WCHAR *exePath)
{
	DWORD verHandle = 0;
	DWORD verSize = GetFileVersionInfoSizeW(exePath, &verHandle);
	uint64 result = 0;
	if(verSize != 0)
	{
		LPSTR verData = new (std::nothrow) char[verSize];
		if(verData && GetFileVersionInfoW(exePath, verHandle, verSize, verData))
		{
			UINT size = 0;
			BYTE *lpBuffer = nullptr;
			if(VerQueryValue(verData, _T("\\"), (void **)&lpBuffer, &size) && size != 0)
			{
				VS_FIXEDFILEINFO *verInfo = (VS_FIXEDFILEINFO *)lpBuffer;
				if (verInfo->dwSignature == 0xfeef04bd)
				{
					result = (uint64(HIWORD(verInfo->dwFileVersionMS)) << 48)
					       | (uint64(LOWORD(verInfo->dwFileVersionMS)) << 32)
					       | (uint64(HIWORD(verInfo->dwFileVersionLS)) << 16)
					       | uint64(LOWORD(verInfo->dwFileVersionLS));
				}
			}
		}
		delete[] verData;
	}
	return result;
}


// Create a plugin bridge object
AEffect *BridgeWrapper::Create(const VSTPluginLib &plugin)
{
	BridgeWrapper *wrapper = new (std::nothrow) BridgeWrapper();
	BridgeWrapper *sharedInstance = nullptr;

	// Should we share instances?
	if(plugin.shareBridgeInstance)
	{
		// Well, then find some instance to share with!
		CVstPlugin *vstPlug = dynamic_cast<CVstPlugin *>(plugin.pPluginsList);
		while(vstPlug != nullptr)
		{
			if(vstPlug->isBridged)
			{
				sharedInstance = reinterpret_cast<BridgeWrapper *>(vstPlug->Dispatch(effVendorSpecific, kVendorOpenMPT, kGetWrapperPointer, nullptr, 0.0f));
				break;
			}
			vstPlug = dynamic_cast<CVstPlugin *>(vstPlug->GetNextInstance());
		}
	}

	try
	{
		if(wrapper != nullptr && wrapper->Init(plugin.dllPath, sharedInstance) && wrapper->queueMem.Good())
		{
			return &wrapper->sharedMem->effect;
		}
		delete wrapper;
		return nullptr;
	} catch(BridgeException &)
	{
		delete wrapper;
		throw;
	}
}


// Initialize and launch bridge
bool BridgeWrapper::Init(const mpt::PathString &pluginPath, BridgeWrapper *sharedInstace)
{
	static uint32 plugId = 0;
	plugId++;
	const DWORD procId = GetCurrentProcessId();

	const std::wstring mapName = L"Local\\openmpt-" + mpt::ToWString(procId) + L"-" + mpt::ToWString(plugId);

	// Create our shared memory object.
	if(!queueMem.Create(mapName.c_str(), sizeof(SharedMemLayout))
		|| !CreateSignals(mapName.c_str()))
	{
		throw BridgeException("Could not initialize plugin bridge memory.");
	}
	sharedMem = reinterpret_cast<SharedMemLayout *>(queueMem.view);

	if(sharedInstace == nullptr)
	{
		// Create a new bridge instance
		BinaryType binType;
		if((binType = GetPluginBinaryType(pluginPath)) == binUnknown)
		{
			return false;
		}

		const bool available = (binType == bin32Bit) ? IsComponentAvailable(pluginBridge32) : IsComponentAvailable(pluginBridge64);
		if(!available)
		{
			ComponentPluginBridge::Availability availability = (binType == bin32Bit) ? pluginBridge32->GetAvailability() : pluginBridge64->GetAvailability();
			switch(availability)
			{
				case ComponentPluginBridge::AvailabilityMissing:
					// Silently fail if bridge is missing.
					throw BridgeNotFoundException();
					break;
				case ComponentPluginBridge::AvailabilityWrongVersion:
					throw BridgeException("The plugin bridge version does not match your OpenMPT version.");
					break;
				default:
					throw BridgeNotFoundException();
					break;
			}
		}

		const mpt::PathString exeName = (binType == bin32Bit) ? pluginBridge32->GetFileName() : pluginBridge64->GetFileName();
	
		otherPtrSize = binType;

		// Command-line must be a modifiable string...
		wchar_t cmdLine[128];
		swprintf(cmdLine, CountOf(cmdLine), L"%s %d", mapName.c_str(), procId);

		STARTUPINFOW info;
		MemsetZero(info);
		info.cb = sizeof(info);
		PROCESS_INFORMATION processInfo;
		MemsetZero(processInfo);

		if(!CreateProcessW(exeName.AsNative().c_str(), cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &info, &processInfo))
		{
			throw BridgeException("Failed to launch plugin bridge.");
		}
		CloseHandle(processInfo.hThread);
		otherProcess = processInfo.hProcess;
	} else
	{
		// Re-use existing bridge instance
		otherPtrSize = sharedInstace->otherPtrSize;
		otherProcess.DuplicateFrom(sharedInstace->otherProcess);

		BridgeMessage msg;
		msg.NewInstance(mapName.c_str());
		if(!sharedInstace->SendToBridge(msg))
		{
			// Something went wrong, try a new instance
			return Init(pluginPath, nullptr);
		}
	}

	// Initialize bridge
	sharedMem->effect.object = this;
	sharedMem->effect.dispatcher = DispatchToPlugin;
	sharedMem->effect.setParameter = SetParameter;
	sharedMem->effect.getParameter = GetParameter;
	sharedMem->effect.process = Process;
	memcpy(&(sharedMem->effect.resvd2), "OMPT", 4);

	sigThreadExit.Create(true);
	sigAutomation.Create(true);

	otherThread = mpt::UnmanagedThreadMember<BridgeWrapper, &BridgeWrapper::MessageThread>(this);

	BridgeMessage initMsg;
	initMsg.Init(pluginPath.ToWide().c_str(), MIXBUFFERSIZE, ExceptionHandler::fullMemDump);

	if(!SendToBridge(initMsg))
	{
		throw BridgeException("Could not initialize plugin bridge, it probably crashed.");
	} else if(initMsg.init.result != 1)
	{
		throw BridgeException(mpt::ToCharset(mpt::CharsetUTF8, initMsg.init.str).c_str());
	} else
	{
		if(sharedMem->effect.flags & effFlagsCanReplacing) sharedMem->effect.processReplacing = ProcessReplacing;
		if(sharedMem->effect.flags & effFlagsCanDoubleReplacing) sharedMem->effect.processDoubleReplacing = ProcessDoubleReplacing;
		return true;
	}

	return false;
}


BridgeWrapper::~BridgeWrapper()
{
	SignalObjectAndWait(sigThreadExit, otherThread, INFINITE, FALSE);
	CloseHandle(otherThread);
}


void BridgeWrapper::MessageThread()
{
	msgThreadID = GetCurrentThreadId();

	const HANDLE objects[] = { sigToHost.send, sigToBridge.ack, otherProcess, sigThreadExit };
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
			for(size_t i = 0; i < CountOf(sharedMem->toBridge); i++)
			{
				BridgeMessage &msg = sharedMem->toBridge[i];
				LONG signalID = msg.header.signalID;
				if(InterlockedCompareExchange(&msg.header.status, MsgHeader::delivered, MsgHeader::done) == MsgHeader::done)
				{
					ackSignals[signalID].Confirm();
				}
			}
		}
	} while(result != WAIT_OBJECT_0 + 2 && result != WAIT_OBJECT_0 + 3 && result != WAIT_FAILED);

	// Close any possible waiting queries
	for(size_t i = 0; i < CountOf(ackSignals); i++)
	{
		ackSignals[i].Send();
	}
}


// Send an arbitrary message to the bridge.
// Returns a pointer to the message, as processed by the bridge.
bool BridgeWrapper::SendToBridge(BridgeMessage &sendMsg)
{
	const bool inMsgThread = GetCurrentThreadId() == msgThreadID;
	BridgeMessage *addr = CopyToSharedMemory(sendMsg, sharedMem->toBridge);
	if(addr == nullptr)
	{
		return false;
	}
	sigToBridge.Send();

	// Wait until we get the result from the bridge.
	DWORD result;
	if(inMsgThread)
	{
		// Since this is the message thread, we must handle messages directly.
		const HANDLE objects[] = { sigToBridge.ack, sigToHost.send, otherProcess, sigThreadExit };
		do
		{
			result = WaitForMultipleObjects(CountOf(objects), objects, FALSE, INFINITE);
			if(result == WAIT_OBJECT_0)
			{
				// Message got answered
				bool done = false;
				for(size_t i = 0; i < CountOf(sharedMem->toBridge); i++)
				{
					BridgeMessage &msg = sharedMem->toBridge[i];
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
		} while(result != WAIT_OBJECT_0 + 2 && result != WAIT_OBJECT_0 + 3 && result != WAIT_FAILED);
		if(result == WAIT_OBJECT_0 + 2)
		{
			sigThreadExit.Trigger();
		}
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
			}
		} while (result == WAIT_TIMEOUT);
		addr->CopyFromSharedMemory(sendMsg);

		// Bridge caught an exception while processing this request
		if(sendMsg.header.type == MsgHeader::exceptionMsg)
		{
			throw BridgeException();
		}

	}

	return (result == WAIT_OBJECT_0);
}


// Receive a message from the host and translate it.
void BridgeWrapper::ParseNextMessage()
{
	ASSERT(GetCurrentThreadId() == msgThreadID);

	BridgeMessage *msg = &sharedMem->toHost[0];
	for(size_t i = 0; i < CountOf(sharedMem->toHost); i++, msg++)
	{
		if(InterlockedCompareExchange(&msg->header.status, MsgHeader::received, MsgHeader::sent) == MsgHeader::sent)
		{
			switch(msg->header.type)
			{
			case MsgHeader::dispatch:
				DispatchToHost(&msg->dispatch);
				break;

			case MsgHeader::errorMsg:
				// TODO Showing a message box here will deadlock as the main thread can be in a waiting state
				//throw BridgeErrorException(msg->error.str);
				break;
			}

			InterlockedExchange(&msg->header.status, MsgHeader::done);
			sigToHost.Confirm();
		}
	}
}


void BridgeWrapper::DispatchToHost(DispatchMsg *msg)
{
	// Various dispatch data - depending on the opcode, one of those might be used.
	std::vector<char> extraData;

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

	switch(msg->opcode)
	{
	case audioMasterProcessEvents:
		// VstEvents* in [ptr]
		TranslateBridgeToVSTEvents(extraData, ptr);
		ptr = &extraData[0];
		break;

	case audioMasterVendorSpecific:
		if(msg->index != kVendorOpenMPT || msg->value != kUpdateProcessingBuffer)
		{
			break;
		}
		MPT_FALLTHROUGH;
	case audioMasterIOChanged:
		{
			// If the song is playing, the rendering thread might be active at the moment,
			// so we should keep the current processing memory alive until it is done for sure.
			const CVstPlugin *plug = FromVstPtr<CVstPlugin>(sharedMem->effect.resvd1);
			const bool isPlaying = plug != nullptr && plug->IsSongPlaying();
			if(isPlaying)
			{
				oldProcessMem.CopyFrom(processMem);
			}
			// Set up new processing file
			processMem.Open(reinterpret_cast<wchar_t *>(ptr));
			if(isPlaying)
			{
				msg->result = 1;
				return;
			}
		}
		break;

	case audioMasterUpdateDisplay:
		cachedProgNames.clear();
		cachedParamInfo.clear();
		break;

	case audioMasterOpenFileSelector:
	case audioMasterCloseFileSelector:
		// TODO: Translate the structs
		msg->result = 0;
		return;
	}

	VstIntPtr result = CVstPlugin::MasterCallBack(&sharedMem->effect, msg->opcode, msg->index, static_cast<VstIntPtr>(msg->value), ptr, msg->opt);
	msg->result = static_cast<int32>(result);

	// Post-fix some opcodes
	switch(msg->opcode)
	{
	case audioMasterGetTime:
		// VstTimeInfo* in [return value]
		if(msg->result != 0)
		{
			MemCopy(sharedMem->timeInfo, *FromVstPtr<VstTimeInfo>(result));
		}
		break;

	case audioMasterGetDirectory:
		// char* in [return value]
		if(msg->result != 0)
		{
			char *target = static_cast<char *>(ptr);
			strncpy(target, FromVstPtr<const char>(result), static_cast<size_t>(msg->ptr - 1));
			target[msg->ptr - 1] = 0;
		}
		break;
	}
}


VstIntPtr VSTCALLBACK BridgeWrapper::DispatchToPlugin(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt)
{
	BridgeWrapper *that = static_cast<BridgeWrapper *>(effect->object);
	if(that == nullptr)
	{
		return 0;
	}

	std::vector<char> dispatchData(sizeof(DispatchMsg), 0);
	int64 ptrOut = 0;
	bool copyPtrBack = false, ptrIsSize = true;
	char *ptrC = static_cast<char *>(ptr);

	switch(opcode)
	{
	case effGetParamLabel:
	case effGetParamDisplay:
	case effGetParamName:
		if(index >= that->cachedParamInfoStart && index < that->cachedParamInfoStart + mpt::saturate_cast<int32>(that->cachedParamInfo.size()))
		{
			if(opcode == effGetParamLabel)
				strcpy(ptr, that->cachedParamInfo[index - that->cachedParamInfoStart].label);
			else if(opcode == effGetParamDisplay)
				strcpy(ptr, that->cachedParamInfo[index - that->cachedParamInfoStart].display);
			else if(opcode == effGetParamName)
				strcpy(ptr, that->cachedParamInfo[index - that->cachedParamInfoStart].name);
			return 1;
		}
		MPT_FALLTHROUGH;
	case effGetProgramName:
	case effString2Parameter:
	case effGetProgramNameIndexed:
	case effGetEffectName:
	case effGetErrorText:
	case effGetVendorString:
	case effGetProductString:
	case effShellGetNextPlugin:
		// Name in [ptr]
		if(opcode == effGetProgramNameIndexed && !that->cachedProgNames.empty())
		{
			// First check if we have cached this program name
			if(index >= that->cachedProgNameStart && index < that->cachedProgNameStart + mpt::saturate_cast<int32>(that->cachedProgNames.size() / kCachedProgramNameLength))
			{
				strcpy(ptr, &that->cachedProgNames[(index - that->cachedProgNameStart) * kCachedProgramNameLength]);
				return 1;
			}
		}
		ptrOut = 256;
		copyPtrBack = true;
		break;

	case effSetProgramName:
		that->cachedProgNames.clear();
		MPT_FALLTHROUGH;
	case effCanDo:
		// char* in [ptr]
		ptrOut = strlen(ptrC) + 1;
		dispatchData.insert(dispatchData.end(), ptrC, ptrC + ptrOut);
		break;

	case effIdle:
		// The plugin bridge will generate these messages by itself
		break;

	case effEditGetRect:
		// ERect** in [ptr]
		ptrOut = sizeof(ERect);
		copyPtrBack = true;
		break;

	case effEditOpen:
		// HWND in [ptr] - Note: Window handles are interoperable between 32-bit and 64-bit applications in Windows (http://msdn.microsoft.com/en-us/library/windows/desktop/aa384203%28v=vs.85%29.aspx)
		ptrOut = reinterpret_cast<int64>(ptr);
		ptrIsSize = false;
		that->cachedProgNames.clear();
		that->cachedParamInfo.clear();
		break;

	case effEditIdle:
		// The plugin bridge will generate these messages by itself
		return 0;

	case effGetChunk:
		// void** in [ptr] for chunk data address
		{
			static uint32 chunkId = 0;
			const std::wstring mapName = L"Local\\openmpt-" + mpt::ToWString(GetCurrentProcessId()) + L"-chunkdata-" + mpt::ToWString(chunkId++);
			ptrOut = (mapName.length() + 1) * sizeof(wchar_t);
			PushToVector(dispatchData, *mapName.c_str(), static_cast<size_t>(ptrOut));
		}
		break;

	case effSetChunk:
		// void* in [ptr] for chunk data
		ptrOut = value;
		dispatchData.insert(dispatchData.end(), ptrC, ptrC + value);
		that->cachedProgNames.clear();
		that->cachedParamInfo.clear();
		break;

	case effProcessEvents:
		// VstEvents* in [ptr]
		// We process in a separate memory segment to save a bridge communication message.
		{
			std::vector<char> events;
			TranslateVSTEventsToBridge(events, static_cast<VstEvents *>(ptr), that->otherPtrSize);
			if(that->eventMem.Size() < events.size())
			{
				// Resize memory
				static uint32 chunkId = 0;
				const std::wstring mapName = L"Local\\openmpt-" + mpt::ToWString(GetCurrentProcessId()) + L"-events-" + mpt::ToWString(chunkId++);
				ptrOut = (mapName.length() + 1) * sizeof(wchar_t);
				PushToVector(dispatchData, *mapName.c_str(), static_cast<size_t>(ptrOut));
				that->eventMem.Create(mapName.c_str(), static_cast<uint32>(events.size() + 1024));

				opcode = effVendorSpecific;
				index = kVendorOpenMPT;
				value = kUpdateEventMemName;
			}
			memcpy(that->eventMem.view, &events[0], events.size());
		}
		if(opcode != effVendorSpecific)
		{
			return 1;
		}
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
		return 0;
		break;

	case effOfflinePrepare:
	case effOfflineRun:
		// VstOfflineTask* in [ptr]
		ptrOut = sizeof(VstOfflineTask) * value;
		// TODO
		return 0;
		break;

	case effProcessVarIo:
		// VstVariableIo* in [ptr]
		ptrOut = sizeof(VstVariableIo);
		// TODO
		return 0;
		break;

	case effSetSpeakerArrangement:
		// VstSpeakerArrangement* in [value] and [ptr]
		ptrOut = sizeof(VstSpeakerArrangement) * 2;
		PushToVector(dispatchData, *static_cast<VstSpeakerArrangement *>(ptr));
		PushToVector(dispatchData, *FromVstPtr<VstSpeakerArrangement>(value));
		break;

	case effVendorSpecific:
		if(index == kVendorOpenMPT)
		{
			switch(value)
			{
			case kGetWrapperPointer:
				return ToVstPtr<BridgeWrapper>(that);

			case kCloseOldProcessingMemory:
				{
					VstIntPtr result = that->oldProcessMem.Good();
					that->oldProcessMem.Close();
					return result;
				}

			case kCacheProgramNames:
				{
					int32 *prog = static_cast<int32 *>(ptr);
					that->cachedProgNameStart = prog[0];
					ptrOut = std::max<int64>(sizeof(int32) * 2, (prog[1] - prog[0]) * kCachedProgramNameLength);
					dispatchData.insert(dispatchData.end(), ptrC, ptrC + 2 * sizeof(int32));
				}
				break;

			case kCacheParameterInfo:
				{
					int32 *param = static_cast<int32 *>(ptr);
					that->cachedParamInfoStart = param[0];
					ptrOut = std::max<int64>(sizeof(int32) * 2, (param[1] - param[0]) * sizeof(ParameterInfo));
					dispatchData.insert(dispatchData.end(), ptrC, ptrC + 2 * sizeof(int32));
				}
			}
		}
		break;

	case effGetParameterProperties:
		// VstParameterProperties* in [ptr]
		if(index >= that->cachedParamInfoStart && index < that->cachedParamInfoStart + mpt::saturate_cast<int32>(that->cachedParamInfo.size()))
		{
			*static_cast<VstParameterProperties *>(ptr) = that->cachedParamInfo[index - that->cachedParamInfoStart].props;
			return 1;
		}
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

	case effBeginSetProgram:
		that->isSettingProgram = true;
		break;

	case effEndSetProgram:
		that->isSettingProgram = false;
		if(that->sharedMem->automationQueue.pendingEvents)
		{
			that->SendAutomationQueue();
		}
		that->cachedProgNames.clear();
		that->cachedParamInfo.clear();
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
		that->cachedProgNames.clear();
		that->cachedParamInfo.clear();
		break;

	default:
		ASSERT(ptr == nullptr);
	}

	if(ptrOut != 0 && ptrIsSize)
	{
		// In case we only reserve space and don't copy stuff over...
		dispatchData.resize(sizeof(DispatchMsg) + static_cast<size_t>(ptrOut), 0);
	}

	uint32 extraSize = static_cast<uint32>(dispatchData.size() - sizeof(DispatchMsg));
	
	// Create message header
	BridgeMessage *msg = reinterpret_cast<BridgeMessage *>(&dispatchData[0]);
	msg->Dispatch(opcode, index, value, ptrOut, opt, extraSize);

	const bool useAuxMem = dispatchData.size() > sizeof(BridgeMessage);
	AuxMem *auxMem = nullptr;
	if(useAuxMem)
	{
		// Extra data doesn't fit in message - use secondary memory
		auxMem = that->GetAuxMemory(dispatchData.size());
		if(auxMem == nullptr)
			return 0;

		// First, move message data to shared memory...
		memcpy(auxMem->memory.view, &dispatchData[sizeof(DispatchMsg)], extraSize);
		// ...Now put the shared memory name in the message instead.
		memcpy(&dispatchData[sizeof(DispatchMsg)], auxMem->name, sizeof(auxMem->name));
	}

	//std::cout << "about to dispatch " << opcode << " to host...";
	//std::flush(std::cout);
	if(!that->SendToBridge(*msg) && opcode != effClose)
	{
		return 0;
	}
	//std::cout << "done." << std::endl;
	const DispatchMsg *resultMsg = &msg->dispatch;

	const char *extraData = useAuxMem ? static_cast<const char *>(auxMem->memory.view) : reinterpret_cast<const char *>(resultMsg + 1);
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
		that->editRect = *reinterpret_cast<const ERect *>(extraData);
		*static_cast<const ERect **>(ptr) = &that->editRect;
		break;

	case effGetChunk:
		// void** in [ptr] for chunk data address
		{
			const wchar_t *str = reinterpret_cast<const wchar_t *>(extraData);
			if(that->getChunkMem.Open(str))
			{
				*static_cast<void **>(ptr) = that->getChunkMem.view;
			} else
			{
				return 0;
			}
		}
		break;

	case effVendorSpecific:
		if(index == kVendorOpenMPT)
		{
			switch(value)
			{
			case kCacheProgramNames:
				if(resultMsg->result == 1)
				{
					that->cachedProgNames.assign(extraData, extraData + ptrOut);
				}
				break;
			case kCacheParameterInfo:
				if(resultMsg->result == 1)
				{
					const ParameterInfo *params = reinterpret_cast<const ParameterInfo *>(extraData);
					that->cachedParamInfo.assign(params, params + ptrOut / sizeof(ParameterInfo));
				}
				break;
			}
		}
		break;

	case effGetSpeakerArrangement:
		// VstSpeakerArrangement* in [value] and [ptr]
		that->speakers[0] = *reinterpret_cast<const VstSpeakerArrangement *>(extraData);
		that->speakers[1] = *(reinterpret_cast<const VstSpeakerArrangement *>(extraData) + 1);
		*static_cast<VstSpeakerArrangement *>(ptr) = that->speakers[0];
		*FromVstPtr<VstSpeakerArrangement>(value) = that->speakers[1];
		break;

	default:
		// TODO: Translate VstVariableIo, offline tasks
		if(copyPtrBack)
		{
			memcpy(ptr, extraData, static_cast<size_t>(ptrOut));
		}
	}

	if(auxMem != nullptr)
	{
		InterlockedExchange(&auxMem->used, 0);
	}

	return static_cast<VstIntPtr>(resultMsg->result);
}


// Allocate auxiliary shared memory for too long bridge messages
BridgeWrapper::AuxMem *BridgeWrapper::GetAuxMemory(uint32 size)
{
	size_t index = CountOf(auxMems);
	for(int pass = 0; pass < 2; pass++)
	{
		for(size_t i = 0; i < CountOf(auxMems); i++)
		{
			if(auxMems[i].size >= size || pass == 1)
			{
				// Good candidate - is it taken yet?
				assert((intptr_t(&auxMems[i].used) & 3) == 0);	// InterlockedExchangeAdd operand should be aligned to 32 bits
				if(InterlockedCompareExchange(&auxMems[i].used, 1, 0) == 0)
				{
					index = i;
					break;
				}
			}
		}
		if(index != CountOf(auxMems))
			break;
	}
	if(index == CountOf(auxMems))
		return nullptr;

	AuxMem &auxMem = auxMems[index];
	if(auxMem.size >= size && auxMem.memory.Good())
	{
		// Re-use as-is
		return &auxMem;
	}
	// Create new memory with appropriate size
	static_assert(sizeof(DispatchMsg) + sizeof(auxMem.name) <= sizeof(BridgeMessage), "Check message sizes, this will crash!");
	static int auxMemCount = 0;
	swprintf(auxMem.name, CountOf(auxMem.name), L"Local\\openmpt-%d-auxmem-%d", GetCurrentProcessId(), auxMemCount++);
	if(auxMem.memory.Create(auxMem.name, size))
	{
		auxMem.size = size;
		return &auxMem;
	} else
	{
		InterlockedExchange(&auxMem.used, 0);
		return nullptr;
	}
}


// Send any pending automation events
void BridgeWrapper::SendAutomationQueue()
{
	sigAutomation.Reset();
	BridgeMessage msg;
	msg.Automate();
	if(!SendToBridge(msg))
	{
		// Failed (plugin probably crashed) - auto-fix event count
		sharedMem->automationQueue.pendingEvents = 0;
	}
	sigAutomation.Trigger();

}

void VSTCALLBACK BridgeWrapper::SetParameter(AEffect *effect, VstInt32 index, float parameter)
{
	BridgeWrapper *that = static_cast<BridgeWrapper *>(effect->object);
	const CVstPlugin *plug = FromVstPtr<CVstPlugin>(effect->resvd1);
	if(that)
	{
		AutomationQueue &autoQueue = that->sharedMem->automationQueue;
		if(that->isSettingProgram || (plug && plug->IsSongPlaying()))
		{
			// Queue up messages while rendering to reduce latency introduced by every single bridge call
			uint32 i;
			while((i = InterlockedExchangeAdd(&autoQueue.pendingEvents, 1)) >= CountOf(autoQueue.params))
			{
				// Queue full!
				if(i == CountOf(autoQueue.params))
				{
					// We're the first to notice that it's full
					that->SendAutomationQueue();
				} else
				{
					// Wait until queue is emptied by someone else (this branch is very unlikely to happen)
					WaitForSingleObject(that->sigAutomation, INFINITE);
				}
			}

			autoQueue.params[i].index = index;
			autoQueue.params[i].value = parameter;
			return;
		} else if(autoQueue.pendingEvents)
		{
			// Actually, this should never happen as pending events are cleared before processing and at the end of a set program event.
			that->SendAutomationQueue();
		}

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
		if(that->SendToBridge(msg))
		{
			return msg.parameter.value;
		}
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
	if(!processMem.Good())
	{
		ASSERT(false);
		return;
	}

	ProcessMsg *msg = static_cast<ProcessMsg *>(processMem.view);
	new (msg) ProcessMsg(type, numInputs, numOutputs, sampleFrames);

	// Anticipate that many plugins will query the play position in a process call and send it along the process call
	// to save some valuable inter-process calls.
	sharedMem->timeInfo = *reinterpret_cast<VstTimeInfo *>(CVstPlugin::MasterCallBack(&sharedMem->effect, audioMasterGetTime, 0, kVstNanosValid | kVstPpqPosValid | kVstTempoValid | kVstBarsValid | kVstCyclePosValid | kVstTimeSigValid | kVstSmpteValid | kVstClockValid, nullptr, 0.0f));

	buf_t *ptr = reinterpret_cast<buf_t *>(msg + 1);
	for(VstInt32 i = 0; i < numInputs; i++)
	{
		memcpy(ptr, inputs[i], sampleFrames * sizeof(buf_t));
		ptr += sampleFrames;
	}
	// Theoretically, we should memcpy() instead of memset() here in process(), but OpenMPT always clears the output buffer before processing so it doesn't matter.
	memset(ptr, 0, numOutputs * sampleFrames * sizeof(buf_t));

	sigProcess.Send();
	const HANDLE objects[] = { sigProcess.ack, otherProcess };
	WaitForMultipleObjects(CountOf(objects), objects, FALSE, INFINITE);

	for(VstInt32 i = 0; i < numOutputs; i++)
	{
		//memcpy(outputs[i], ptr, sampleFrames * sizeof(buf_t));
		outputs[i] = ptr;	// Exactly what you don't want plugins to do usually (bend your output pointers)... muahahaha!
		ptr += sampleFrames;
	}
}

#endif //NO_VST


OPENMPT_NAMESPACE_END
