/*
 * BridgeWrapper.cpp
 * -----------------
 * Purpose: VST plugin bridge wrapper (host side)
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#ifndef NO_VST
#include "BridgeWrapper.h"
#include "../soundlib/plugins/PluginManager.h"
#include "../mptrack/Mainfrm.h"
#include "../mptrack/Mptrack.h"
#include "../mptrack/Vstplug.h"
#include "../mptrack/ExceptionHandler.h"
#include "../common/mptFileIO.h"
#include "../common/mptStringBuffer.h"
#include "../common/misc_util.h"

using namespace Vst;


OPENMPT_NAMESPACE_BEGIN

std::vector<BridgeCommon *> BridgeCommon::m_plugins;
HWND BridgeCommon::m_communicationWindow = nullptr;
int BridgeCommon::m_instanceCount = 0;
thread_local bool BridgeCommon::m_isAudioThread = false;

std::size_t GetPluginArchPointerSize(PluginArch arch)
{
	std::size_t result = 0;
	switch(arch)
	{
	case PluginArch_x86:
		result = 4;
		break;
	case PluginArch_amd64:
		result = 8;
		break;
	case PluginArch_arm:
		result = 4;
		break;
	case PluginArch_arm64:
		result = 8;
		break;
	default:
		result = 0;
		break;
	}
	return result;
}


ComponentPluginBridge::ComponentPluginBridge(PluginArch arch, Generation generation)
	: ComponentBase(ComponentTypeBundled)
	, arch(arch)
	, generation(generation)
{
}


bool ComponentPluginBridge::DoInitialize()
{
	mpt::PathString archName;
	switch(arch)
	{
	case PluginArch_x86:
		if(mpt::OS::Windows::HostCanRun(mpt::OS::Windows::GetHostArchitecture(), mpt::OS::Windows::Architecture::x86) == mpt::OS::Windows::EmulationLevel::NA)
		{
			return false;
		}
		archName = P_("x86");
		break;
	case PluginArch_amd64:
		if(mpt::OS::Windows::HostCanRun(mpt::OS::Windows::GetHostArchitecture(), mpt::OS::Windows::Architecture::amd64) == mpt::OS::Windows::EmulationLevel::NA)
		{
			return false;
		}
		archName = P_("amd64");
		break;
	case PluginArch_arm:
		if(mpt::OS::Windows::HostCanRun(mpt::OS::Windows::GetHostArchitecture(), mpt::OS::Windows::Architecture::arm) == mpt::OS::Windows::EmulationLevel::NA)
		{
			return false;
		}
		archName = P_("arm");
		break;
	case PluginArch_arm64:
		if(mpt::OS::Windows::HostCanRun(mpt::OS::Windows::GetHostArchitecture(), mpt::OS::Windows::Architecture::arm64) == mpt::OS::Windows::EmulationLevel::NA)
		{
			return false;
		}
		archName = P_("arm64");
		break;
	default:
		break;
	}
	if(archName.empty())
	{
		return false;
	}
	exeName = mpt::PathString();
	const mpt::PathString generationSuffix = (generation == Generation::Legacy) ? P_("Legacy") : P_("");
	const mpt::PathString exeNames[] =
	{
		theApp.GetInstallPath() + P_("PluginBridge") + generationSuffix + P_("-") + archName + P_(".exe"),                          // Local
		theApp.GetInstallBinPath() + archName + P_("\\") + P_("PluginBridge") + generationSuffix + P_(".exe"),                      // Multi-arch
		theApp.GetInstallBinPath() + archName + P_("\\") + P_("PluginBridge") + generationSuffix + P_("-") + archName + P_(".exe")  // Multi-arch transitional
	};
	for(const auto &candidate : exeNames)
	{
		if(candidate.IsFile())
		{
			exeName = candidate;
			break;
		}
	}
	if(exeName.empty())
	{
		availability = AvailabilityMissing;
		return false;
	}
	std::vector<WCHAR> exePath(MAX_PATH);
	while(GetModuleFileNameW(0, exePath.data(), mpt::saturate_cast<DWORD>(exePath.size())) >= exePath.size())
	{
		exePath.resize(exePath.size() * 2);
	}
	uint64 mptVersion = BridgeWrapper::GetFileVersion(exePath.data());
	uint64 bridgeVersion = BridgeWrapper::GetFileVersion(exeName.ToWide().c_str());
	if(bridgeVersion != mptVersion)
	{
		availability = AvailabilityWrongVersion;
		return false;
	}
	availability = AvailabilityOK;
	return true;
}


MPT_REGISTERED_COMPONENT(ComponentPluginBridge_x86, "PluginBridge-x86")
MPT_REGISTERED_COMPONENT(ComponentPluginBridgeLegacy_x86, "PluginBridgeLegacy-x86")

MPT_REGISTERED_COMPONENT(ComponentPluginBridge_amd64, "PluginBridge-amd64")
MPT_REGISTERED_COMPONENT(ComponentPluginBridgeLegacy_amd64, "PluginBridgeLegacy-amd64")

#if defined(MPT_WITH_WINDOWS10)

MPT_REGISTERED_COMPONENT(ComponentPluginBridge_arm, "PluginBridge-arm")
MPT_REGISTERED_COMPONENT(ComponentPluginBridgeLegacy_arm, "PluginBridgeLegacy-arm")

MPT_REGISTERED_COMPONENT(ComponentPluginBridge_arm64, "PluginBridge-arm64")
MPT_REGISTERED_COMPONENT(ComponentPluginBridgeLegacy_arm64, "PluginBridgeLegacy-arm64")

#endif  // MPT_WITH_WINDOWS10


PluginArch BridgeWrapper::GetNativePluginBinaryType()
{
	PluginArch result = PluginArch_unknown;
	switch(mpt::OS::Windows::GetProcessArchitecture())
	{
	case mpt::OS::Windows::Architecture::x86:
		result = PluginArch_x86;
		break;
	case mpt::OS::Windows::Architecture::amd64:
		result = PluginArch_amd64;
		break;
	case mpt::OS::Windows::Architecture::arm:
		result = PluginArch_arm;
		break;
	case mpt::OS::Windows::Architecture::arm64:
		result = PluginArch_arm64;
		break;
	default:
		result = PluginArch_unknown;
		break;
	}
	return result;
}


// Check whether we need to load a 32-bit or 64-bit wrapper.
PluginArch BridgeWrapper::GetPluginBinaryType(const mpt::PathString &pluginPath)
{
	PluginArch type = PluginArch_unknown;
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

			MPT_ASSERT((ntHeader.FileHeader.Characteristics & IMAGE_FILE_DLL) != 0);
			switch(ntHeader.FileHeader.Machine)
			{
			case IMAGE_FILE_MACHINE_I386:
				type = PluginArch_x86;
				break;
			case IMAGE_FILE_MACHINE_AMD64:
				type = PluginArch_amd64;
				break;
#if defined(MPT_WITH_WINDOWS10)
			case IMAGE_FILE_MACHINE_ARM:
				type = PluginArch_arm;
				break;
			case IMAGE_FILE_MACHINE_ARM64:
				type = PluginArch_arm64;
				break;
#endif  // MPT_WITH_WINDOWS10
			default:
				type = PluginArch_unknown;
				break;
			}
		}
	}
	return type;
}


uint64 BridgeWrapper::GetFileVersion(const WCHAR *exePath)
{
	DWORD verHandle = 0;
	DWORD verSize = GetFileVersionInfoSizeW(exePath, &verHandle);
	uint64 result = 0;
	if(verSize == 0)
		return result;

	char *verData = new(std::nothrow) char[verSize];
	if(verData && GetFileVersionInfoW(exePath, verHandle, verSize, verData))
	{
		UINT size = 0;
		void *lpBuffer = nullptr;
		if(VerQueryValue(verData, _T("\\"), &lpBuffer, &size) && size != 0)
		{
			auto *verInfo = static_cast<const VS_FIXEDFILEINFO *>(lpBuffer);
			if(verInfo->dwSignature == 0xfeef04bd)
			{
				result = (uint64(HIWORD(verInfo->dwFileVersionMS)) << 48)
				         | (uint64(LOWORD(verInfo->dwFileVersionMS)) << 32)
				         | (uint64(HIWORD(verInfo->dwFileVersionLS)) << 16)
				         | uint64(LOWORD(verInfo->dwFileVersionLS));
			}
		}
	}
	delete[] verData;
	return result;
}


// Create a plugin bridge object
AEffect *BridgeWrapper::Create(const VSTPluginLib &plugin, bool forceLegacy)
{
	BridgeWrapper *wrapper = new(std::nothrow) BridgeWrapper();
	BridgeWrapper *sharedInstance = nullptr;
	const Generation wantedGeneration = (plugin.modernBridge && !forceLegacy) ? Generation::Modern : Generation::Legacy;

	// Should we share instances?
	if(plugin.shareBridgeInstance)
	{
		// Well, then find some instance to share with!
		CVstPlugin *vstPlug = dynamic_cast<CVstPlugin *>(plugin.pPluginsList);
		while(vstPlug != nullptr)
		{
			if(vstPlug->isBridged)
			{
				BridgeWrapper *instance = FromIntPtr<BridgeWrapper>(vstPlug->Dispatch(effVendorSpecific, kVendorOpenMPT, kGetWrapperPointer, nullptr, 0.0f));
				if(wantedGeneration == instance->m_Generation)
				{
					sharedInstance = instance;
					break;
				}
			}
			vstPlug = dynamic_cast<CVstPlugin *>(vstPlug->GetNextInstance());
		}
	}

	try
	{
		if(wrapper != nullptr && wrapper->Init(plugin.dllPath, wantedGeneration, sharedInstance) && wrapper->m_queueMem.Good())
		{
			return &wrapper->m_sharedMem->effect;
		}
		delete wrapper;
		return nullptr;
	} catch(BridgeException &)
	{
		delete wrapper;
		throw;
	}
}


BridgeWrapper::BridgeWrapper()
{
	m_thisPluginID = static_cast<int32>(m_plugins.size());
	m_plugins.push_back(this);

	if(m_instanceCount == 1)
		CreateCommunicationWindow(WindowProc);
}


BridgeWrapper::~BridgeWrapper()
{
	if(m_instanceCount == 1)
		DestroyWindow(m_communicationWindow);
}


// Initialize and launch bridge
bool BridgeWrapper::Init(const mpt::PathString &pluginPath, Generation bridgeGeneration, BridgeWrapper *sharedInstace)
{
	static uint32 plugId = 0;
	plugId++;
	const DWORD procId = GetCurrentProcessId();

	const std::wstring mapName = MPT_WFORMAT("Local\\openmpt-{}-{}")(procId, plugId);

	// Create our shared memory object.
	if(!m_queueMem.Create(mapName.c_str(), sizeof(SharedMemLayout))
	   || !CreateSignals(mapName.c_str()))
	{
		throw BridgeException("Could not initialize plugin bridge memory.");
	}
	m_sharedMem = m_queueMem.Data<SharedMemLayout>();

	if(sharedInstace == nullptr)
	{
		// Create a new bridge instance
		const PluginArch arch = GetPluginBinaryType(pluginPath);
		bool available = false;
		ComponentPluginBridge::Availability availability = ComponentPluginBridge::AvailabilityUnknown;
		switch(arch)
		{
		case PluginArch_x86:
			if(available = (bridgeGeneration == Generation::Modern) ? IsComponentAvailable(pluginBridge_x86) : IsComponentAvailable(pluginBridgeLegacy_x86); !available)
				availability = (bridgeGeneration == Generation::Modern) ? pluginBridge_x86->GetAvailability() : pluginBridgeLegacy_x86->GetAvailability();
			break;
		case PluginArch_amd64:
			if(available = (bridgeGeneration == Generation::Modern) ? IsComponentAvailable(pluginBridge_amd64) : IsComponentAvailable(pluginBridgeLegacy_amd64); !available)
				availability = (bridgeGeneration == Generation::Modern) ? pluginBridge_amd64->GetAvailability() : pluginBridgeLegacy_amd64->GetAvailability();
			break;
#if defined(MPT_WITH_WINDOWS10)
		case PluginArch_arm:
			if(available = (bridgeGeneration == Generation::Modern) ? IsComponentAvailable(pluginBridge_arm) : IsComponentAvailable(pluginBridgeLegacy_arm); !available)
				availability = (bridgeGeneration == Generation::Modern) ? pluginBridge_arm->GetAvailability() : pluginBridgeLegacy_arm->GetAvailability();
			break;
		case PluginArch_arm64:
			if(available = (bridgeGeneration == Generation::Modern) ? IsComponentAvailable(pluginBridge_arm64) : IsComponentAvailable(pluginBridgeLegacy_arm64); !available)
				availability = (bridgeGeneration == Generation::Modern) ? pluginBridge_arm64->GetAvailability() : pluginBridgeLegacy_arm64->GetAvailability();
			break;
#endif  // MPT_WITH_WINDOWS10
		default:
			break;
		}
		if(arch == PluginArch_unknown)
		{
			return false;
		}
		if(!available)
		{
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
		const ComponentPluginBridge *const pluginBridge =
				(arch == PluginArch_x86 && bridgeGeneration == Generation::Modern) ? static_cast<const ComponentPluginBridge *>(pluginBridge_x86.get()) :
				(arch == PluginArch_x86 && bridgeGeneration == Generation::Legacy) ? static_cast<const ComponentPluginBridge *>(pluginBridgeLegacy_x86.get()) :
				(arch == PluginArch_amd64 && bridgeGeneration == Generation::Modern) ? static_cast<const ComponentPluginBridge *>(pluginBridge_amd64.get()) :
				(arch == PluginArch_amd64 && bridgeGeneration == Generation::Legacy) ? static_cast<const ComponentPluginBridge *>(pluginBridgeLegacy_amd64.get()) :
#if defined(MPT_WITH_WINDOWS10)
				(arch == PluginArch_arm && bridgeGeneration == Generation::Modern) ? static_cast<const ComponentPluginBridge *>(pluginBridge_arm.get()) :
				(arch == PluginArch_arm && bridgeGeneration == Generation::Legacy) ? static_cast<const ComponentPluginBridge *>(pluginBridgeLegacy_arm.get()) :
				(arch == PluginArch_arm64 && bridgeGeneration == Generation::Modern) ? static_cast<const ComponentPluginBridge *>(pluginBridge_arm64.get()) :
				(arch == PluginArch_arm64 && bridgeGeneration == Generation::Legacy) ? static_cast<const ComponentPluginBridge *>(pluginBridgeLegacy_arm64.get()) :
#endif  // MPT_WITH_WINDOWS10
		    nullptr;
		m_Generation = bridgeGeneration;
		const mpt::PathString exeName = pluginBridge->GetFileName();

		m_otherPtrSize = static_cast<int32>(GetPluginArchPointerSize(arch));

		std::wstring cmdLine = MPT_WFORMAT("{} {}")(mapName, procId);

		STARTUPINFOW info;
		MemsetZero(info);
		info.cb = sizeof(info);
		PROCESS_INFORMATION processInfo;
		MemsetZero(processInfo);

		if(!CreateProcessW(exeName.ToWide().c_str(), cmdLine.data(), NULL, NULL, FALSE, 0, NULL, NULL, &info, &processInfo))
		{
			throw BridgeException("Failed to launch plugin bridge.");
		}
		CloseHandle(processInfo.hThread);
		m_otherProcess = processInfo.hProcess;
	} else
	{
		// Re-use existing bridge instance
		m_otherPtrSize = sharedInstace->m_otherPtrSize;
		m_otherProcess.DuplicateFrom(sharedInstace->m_otherProcess);

		BridgeMessage msg;
		msg.NewInstance(mapName.c_str());
		if(!sharedInstace->SendToBridge(msg))
		{
			// Something went wrong, try a new instance
			return Init(pluginPath, bridgeGeneration, nullptr);
		}
	}

	// Initialize bridge
	m_sharedMem->effect.object = this;
	m_sharedMem->effect.dispatcher = DispatchToPlugin;
	m_sharedMem->effect.setParameter = SetParameter;
	m_sharedMem->effect.getParameter = GetParameter;
	m_sharedMem->effect.process = Process;
	std::memcpy(&(m_sharedMem->effect.reservedForHost2), "OMPT", 4);

	m_sigAutomation.Create(true);

	m_sharedMem->hostCommWindow = m_communicationWindow;

	const HANDLE objects[] = {m_sigBridgeReady, m_otherProcess};
	if(WaitForMultipleObjects(mpt::saturate_cast<DWORD>(std::size(objects)), objects, FALSE, 10000) != WAIT_OBJECT_0)
	{
		throw BridgeException("Could not connect to plugin bridge, it probably crashed.");
	}
	m_otherPluginID = m_sharedMem->bridgePluginID;

	BridgeMessage initMsg;
	initMsg.Init(pluginPath.ToWide().c_str(), MIXBUFFERSIZE, m_thisPluginID, ExceptionHandler::fullMemDump);

	if(!SendToBridge(initMsg))
	{
		throw BridgeException("Could not initialize plugin bridge, it probably crashed.");
	} else if(initMsg.init.result != 1)
	{
		throw BridgeException(mpt::ToCharset(mpt::Charset::UTF8, initMsg.init.str).c_str());
	}

	if(m_sharedMem->effect.flags & effFlagsCanReplacing)
		m_sharedMem->effect.processReplacing = ProcessReplacing;
	if(m_sharedMem->effect.flags & effFlagsCanDoubleReplacing)
		m_sharedMem->effect.processDoubleReplacing = ProcessDoubleReplacing;
	return true;
}


// Send an arbitrary message to the bridge.
// Returns true if the message was processed by the bridge.
bool BridgeWrapper::SendToBridge(BridgeMessage &sendMsg)
{
	const bool inAudioThread = m_isAudioThread || CMainFrame::GetMainFrame()->InAudioThread();
	auto &messages = m_sharedMem->ipcMessages;
	const auto msgID = CopyToSharedMemory(sendMsg, messages);
	if(msgID < 0)
		return false;
	BridgeMessage &sharedMsg = messages[msgID];

	if(!inAudioThread)
	{
		if(SendMessage(m_sharedMem->bridgeCommWindow, WM_BRIDGE_MESSAGE_TO_BRIDGE, m_otherPluginID, msgID) == WM_BRIDGE_SUCCESS)
		{
			sharedMsg.CopyTo(sendMsg);
			return true;
		}
		return false;
	}

	// Audio thread: Use signals instead of window messages
	m_sharedMem->audioThreadToBridgeMsgID = msgID;
	m_sigToBridgeAudio.Send();

	// Wait until we get the result from the bridge
	DWORD result;
	const HANDLE objects[] = {m_sigToBridgeAudio.confirm, m_sigToHostAudio.send, m_otherProcess};
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
			ParseNextMessage(m_sharedMem->audioThreadToHostMsgID);
			m_sigToHostAudio.Confirm();
		}
	} while(result != WAIT_OBJECT_0 + 2 && result != WAIT_FAILED);

	return (result == WAIT_OBJECT_0);
}


// Receive a message from the host and translate it.
void BridgeWrapper::ParseNextMessage(int msgID)
{
	auto &msg = m_sharedMem->ipcMessages[msgID];
	switch(msg.header.type)
	{
	case MsgHeader::dispatch:
		DispatchToHost(msg.dispatch);
		break;

	case MsgHeader::errorMsg:
		// TODO Showing a message box here will deadlock as the main thread can be in a waiting state
		//throw BridgeErrorException(msg.error.str);
		break;
	}
}


void BridgeWrapper::DispatchToHost(DispatchMsg &msg)
{
	// Various dispatch data - depending on the opcode, one of those might be used.
	std::vector<char> extraData;

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
	case audioMasterProcessEvents:
		// VstEvents* in [ptr]
		TranslateBridgeToVstEvents(extraData, ptr);
		ptr = extraData.data();
		break;

	case audioMasterVendorSpecific:
		if(msg.index != kVendorOpenMPT || msg.value != kUpdateProcessingBuffer)
		{
			break;
		}
		[[fallthrough]];
	case audioMasterIOChanged:
	{
		// If the song is playing, the rendering thread might be active at the moment,
		// so we should keep the current processing memory alive until it is done for sure.
		const CVstPlugin *plug = static_cast<CVstPlugin *>(m_sharedMem->effect.reservedForHost1);
		const bool isPlaying = plug != nullptr && plug->IsResumed();
		if(isPlaying)
		{
			m_oldProcessMem.CopyFrom(m_processMem);
		}
		// Set up new processing file
		m_processMem.Open(static_cast<wchar_t *>(ptr));
		if(isPlaying)
		{
			msg.result = 1;
			return;
		}
	}
	break;

	case audioMasterUpdateDisplay:
		m_cachedProgNames.clear();
		m_cachedParamInfo.clear();
		break;

	case audioMasterOpenFileSelector:
		TranslateBridgeToVstFileSelect(extraData, ptr, static_cast<size_t>(msg.ptr));
		ptr = extraData.data();
		break;
	}

	intptr_t result = CVstPlugin::MasterCallBack(&m_sharedMem->effect, static_cast<VstOpcodeToHost>(msg.opcode), msg.index, static_cast<intptr_t>(msg.value), ptr, msg.opt);
	msg.result = static_cast<int32>(result);

	// Post-fix some opcodes
	switch(msg.opcode)
	{
	case audioMasterGetTime:
		// VstTimeInfo* in [return value]
		if(msg.result != 0)
		{
			m_sharedMem->timeInfo = *FromIntPtr<VstTimeInfo>(result);
		}
		break;

	case audioMasterGetDirectory:
		// char* in [return value]
		if(msg.result != 0)
		{
			char *target = static_cast<char *>(ptr);
			strncpy(target, FromIntPtr<const char>(result), static_cast<size_t>(msg.ptr - 1));
			target[msg.ptr - 1] = 0;
		}
		break;

	case audioMasterOpenFileSelector:
		if(msg.result != 0)
		{
			std::vector<char> fileSelect;
			TranslateVstFileSelectToBridge(fileSelect, *static_cast<const VstFileSelect *>(ptr), m_otherPtrSize);
			std::memcpy(origPtr, fileSelect.data(), std::min(fileSelect.size(), static_cast<size_t>(msg.ptr)));
			// Directly free memory on host side, we don't need it anymore
			CVstPlugin::MasterCallBack(&m_sharedMem->effect, audioMasterCloseFileSelector, msg.index, static_cast<intptr_t>(msg.value), ptr, msg.opt);
		}
		break;
	}
}


intptr_t VSTCALLBACK BridgeWrapper::DispatchToPlugin(AEffect *effect, VstOpcodeToPlugin opcode, int32 index, intptr_t value, void *ptr, float opt)
{
	BridgeWrapper *that = static_cast<BridgeWrapper *>(effect->object);
	if(that != nullptr)
	{
		return that->DispatchToPlugin(opcode, index, value, ptr, opt);
	}
	return 0;
}


intptr_t BridgeWrapper::DispatchToPlugin(VstOpcodeToPlugin opcode, int32 index, intptr_t value, void *ptr, float opt)
{
	std::vector<char> dispatchData(sizeof(DispatchMsg), 0);
	int64 ptrOut = 0;
	bool copyPtrBack = false, ptrIsSize = true;
	char *ptrC = static_cast<char *>(ptr);

	switch(opcode)
	{
	case effGetParamLabel:
	case effGetParamDisplay:
	case effGetParamName:
		if(index >= m_cachedParamInfoStart && index < m_cachedParamInfoStart + mpt::saturate_cast<int32>(m_cachedParamInfo.size()))
		{
			if(opcode == effGetParamLabel)
				strcpy(ptrC, m_cachedParamInfo[index - m_cachedParamInfoStart].label);
			else if(opcode == effGetParamDisplay)
				strcpy(ptrC, m_cachedParamInfo[index - m_cachedParamInfoStart].display);
			else if(opcode == effGetParamName)
				strcpy(ptrC, m_cachedParamInfo[index - m_cachedParamInfoStart].name);
			return 1;
		}
		[[fallthrough]];
	case effGetProgramName:
	case effString2Parameter:
	case effGetProgramNameIndexed:
	case effGetEffectName:
	case effGetErrorText:
	case effGetVendorString:
	case effGetProductString:
	case effShellGetNextPlugin:
		// Name in [ptr]
		if(opcode == effGetProgramNameIndexed && !m_cachedProgNames.empty())
		{
			// First check if we have cached this program name
			if(index >= m_cachedProgNameStart && index < m_cachedProgNameStart + mpt::saturate_cast<int32>(m_cachedProgNames.size() / kCachedProgramNameLength))
			{
				strcpy(ptrC, &m_cachedProgNames[(index - m_cachedProgNameStart) * kCachedProgramNameLength]);
				return 1;
			}
		}
		ptrOut = 256;
		copyPtrBack = true;
		break;

	case effSetProgramName:
		m_cachedProgNames.clear();
		[[fallthrough]];
	case effCanDo:
		// char* in [ptr]
		ptrOut = strlen(ptrC) + 1;
		dispatchData.insert(dispatchData.end(), ptrC, ptrC + ptrOut);
		break;

	case effIdle:
		// The plugin bridge will generate these messages by itself
		return 0;

	case effEditGetRect:
		// ERect** in [ptr]
		ptrOut = sizeof(ERect);
		copyPtrBack = true;
		break;

	case effEditOpen:
		// HWND in [ptr] - Note: Window handles are interoperable between 32-bit and 64-bit applications in Windows (http://msdn.microsoft.com/en-us/library/windows/desktop/aa384203%28v=vs.85%29.aspx)
		ptrOut = reinterpret_cast<int64>(ptr);
		ptrIsSize = false;
		m_cachedProgNames.clear();
		m_cachedParamInfo.clear();
		break;

	case effEditIdle:
		// The plugin bridge will generate these messages by itself
		return 0;

	case effGetChunk:
		// void** in [ptr] for chunk data address
		{
			static uint32 chunkId = 0;
			const std::wstring mapName = L"Local\\openmpt-" + mpt::wfmt::val(GetCurrentProcessId()) + L"-chunkdata-" + mpt::wfmt::val(chunkId++);
			ptrOut = (mapName.length() + 1) * sizeof(wchar_t);
			PushToVector(dispatchData, *mapName.c_str(), static_cast<size_t>(ptrOut));
		}
		break;

	case effSetChunk:
		// void* in [ptr] for chunk data
		ptrOut = value;
		dispatchData.insert(dispatchData.end(), ptrC, ptrC + value);
		m_cachedProgNames.clear();
		m_cachedParamInfo.clear();
		break;

	case effProcessEvents:
		// VstEvents* in [ptr]
		// We process in a separate memory segment to save a bridge communication message.
		{
			std::vector<char> events;
			TranslateVstEventsToBridge(events, *static_cast<VstEvents *>(ptr), m_otherPtrSize);
			if(m_eventMem.Size() < events.size())
			{
				// Resize memory
				static uint32 chunkId = 0;
				const std::wstring mapName = L"Local\\openmpt-" + mpt::wfmt::val(GetCurrentProcessId()) + L"-events-" + mpt::wfmt::val(chunkId++);
				ptrOut = (mapName.length() + 1) * sizeof(wchar_t);
				PushToVector(dispatchData, *mapName.c_str(), static_cast<size_t>(ptrOut));
				m_eventMem.Create(mapName.c_str(), static_cast<uint32>(events.size() + 1024));

				opcode = effVendorSpecific;
				index = kVendorOpenMPT;
				value = kUpdateEventMemName;
			}
			std::memcpy(m_eventMem.Data(), events.data(), events.size());
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
		PushToVector(dispatchData, *FromIntPtr<VstSpeakerArrangement>(value));
		break;

	case effVendorSpecific:
		if(index == kVendorOpenMPT)
		{
			switch(value)
			{
			case kGetWrapperPointer:
				return ToIntPtr<BridgeWrapper>(this);

			case kCloseOldProcessingMemory:
			{
				intptr_t result = m_oldProcessMem.Good();
				m_oldProcessMem.Close();
				return result;
			}

			case kCacheProgramNames:
			{
				int32 *prog = static_cast<int32 *>(ptr);
				m_cachedProgNameStart = prog[0];
				ptrOut = std::max(static_cast<int64>(sizeof(int32) * 2), static_cast<int64>((prog[1] - prog[0]) * kCachedProgramNameLength));
				dispatchData.insert(dispatchData.end(), ptrC, ptrC + 2 * sizeof(int32));
			}
			break;

			case kCacheParameterInfo:
			{
				int32 *param = static_cast<int32 *>(ptr);
				m_cachedParamInfoStart = param[0];
				ptrOut = std::max(static_cast<int64>(sizeof(int32) * 2), static_cast<int64>((param[1] - param[0]) * sizeof(ParameterInfo)));
				dispatchData.insert(dispatchData.end(), ptrC, ptrC + 2 * sizeof(int32));
			}
			break;

			case kBeginGetProgram:
				ptrOut = m_sharedMem->effect.numParams * sizeof(float);
				break;

			case kEndGetProgram:
				m_cachedParamValues.clear();
				return 1;
			}
		}
		break;

	case effGetTailSize:
		return m_sharedMem->tailSize;

	case effGetParameterProperties:
		// VstParameterProperties* in [ptr]
		if(index >= m_cachedParamInfoStart && index < m_cachedParamInfoStart + mpt::saturate_cast<int32>(m_cachedParamInfo.size()))
		{
			*static_cast<VstParameterProperties *>(ptr) = m_cachedParamInfo[index - m_cachedParamInfoStart].props;
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
		m_isSettingProgram = true;
		break;

	case effEndSetProgram:
		m_isSettingProgram = false;
		if(m_sharedMem->automationQueue.pendingEvents)
		{
			SendAutomationQueue();
		}
		m_cachedProgNames.clear();
		m_cachedParamInfo.clear();
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
		m_cachedProgNames.clear();
		m_cachedParamInfo.clear();
		break;

	default:
		MPT_ASSERT(ptr == nullptr);
	}

	if(ptrOut != 0 && ptrIsSize)
	{
		// In case we only reserve space and don't copy stuff over...
		dispatchData.resize(sizeof(DispatchMsg) + static_cast<size_t>(ptrOut), 0);
	}

	uint32 extraSize = static_cast<uint32>(dispatchData.size() - sizeof(DispatchMsg));

	// Create message header
	BridgeMessage &msg = *reinterpret_cast<BridgeMessage *>(dispatchData.data());
	msg.Dispatch(opcode, index, value, ptrOut, opt, extraSize);

	const bool useAuxMem = dispatchData.size() > sizeof(BridgeMessage);
	AuxMem *auxMem = nullptr;
	if(useAuxMem)
	{
		// Extra data doesn't fit in message - use secondary memory
		if(dispatchData.size() > std::numeric_limits<uint32>::max())
			return 0;
		auxMem = GetAuxMemory(mpt::saturate_cast<uint32>(dispatchData.size()));
		if(auxMem == nullptr)
			return 0;

		// First, move message data to shared memory...
		std::memcpy(auxMem->memory.Data(), &dispatchData[sizeof(DispatchMsg)], extraSize);
		// ...Now put the shared memory name in the message instead.
		std::memcpy(&dispatchData[sizeof(DispatchMsg)], auxMem->name, sizeof(auxMem->name));
	}

	try
	{
		if(!SendToBridge(msg) && opcode != effClose)
		{
			return 0;
		}
	} catch(...)
	{
		// Don't do anything for now.
#if 0
		if(opcode != effClose)
		{
			throw;
		}
#endif
	}

	const DispatchMsg &resultMsg = msg.dispatch;

	const void *extraData = useAuxMem ? auxMem->memory.Data<const char>() : reinterpret_cast<const char *>(&resultMsg + 1);
	// Post-fix some opcodes
	switch(opcode)
	{
	case effClose:
		m_sharedMem->effect.object = nullptr;
		delete this;
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
		strcpy(ptrC, static_cast<const char *>(extraData));
		break;

	case effEditGetRect:
		// ERect** in [ptr]
		m_editRect = *static_cast<const ERect *>(extraData);
		*static_cast<const ERect **>(ptr) = &m_editRect;
		break;

	case effGetChunk:
		// void** in [ptr] for chunk data address
		if(const wchar_t *str = static_cast<const wchar_t *>(extraData); m_getChunkMem.Open(str))
			*static_cast<void **>(ptr) = m_getChunkMem.Data();
		else
			return 0;
		break;

	case effVendorSpecific:
		if(index == kVendorOpenMPT && resultMsg.result == 1)
		{
			switch(value)
			{
			case kCacheProgramNames:
				m_cachedProgNames.assign(static_cast<const char *>(extraData), static_cast<const char *>(extraData) + ptrOut);
				break;
			case kCacheParameterInfo:
			{
				const ParameterInfo *params = static_cast<const ParameterInfo *>(extraData);
				m_cachedParamInfo.assign(params, params + ptrOut / sizeof(ParameterInfo));
				break;
			}
			case kBeginGetProgram:
				m_cachedParamValues.assign(static_cast<const float *>(extraData), static_cast<const float *>(extraData) + ptrOut / sizeof(float));
				break;
			}
		}
		break;

	case effGetSpeakerArrangement:
		// VstSpeakerArrangement* in [value] and [ptr]
		m_speakers[0] = *static_cast<const VstSpeakerArrangement *>(extraData);
		m_speakers[1] = *(static_cast<const VstSpeakerArrangement *>(extraData) + 1);
		*static_cast<VstSpeakerArrangement *>(ptr) = m_speakers[0];
		*FromIntPtr<VstSpeakerArrangement>(value) = m_speakers[1];
		break;

	default:
		// TODO: Translate VstVariableIo, offline tasks
		if(copyPtrBack)
		{
			std::memcpy(ptr, extraData, static_cast<size_t>(ptrOut));
		}
	}

	if(auxMem != nullptr)
	{
		auxMem->used = false;
	}

	return static_cast<intptr_t>(resultMsg.result);
}


// Allocate auxiliary shared memory for too long bridge messages
BridgeWrapper::AuxMem *BridgeWrapper::GetAuxMemory(uint32 size)
{
	std::size_t index = std::size(m_auxMems);
	for(int pass = 0; pass < 2; pass++)
	{
		for(std::size_t i = 0; i < std::size(m_auxMems); i++)
		{
			if(m_auxMems[i].size >= size || pass == 1)
			{
				// Good candidate - is it taken yet?
				bool expected = false;
				if(m_auxMems[i].used.compare_exchange_strong(expected, true))
				{
					index = i;
					break;
				}
			}
		}
		if(index != std::size(m_auxMems))
			break;
	}
	if(index == std::size(m_auxMems))
		return nullptr;

	AuxMem &auxMem = m_auxMems[index];
	if(auxMem.size >= size && auxMem.memory.Good())
	{
		// Re-use as-is
		return &auxMem;
	}
	// Create new memory with appropriate size
	static_assert(sizeof(DispatchMsg) + sizeof(auxMem.name) <= sizeof(BridgeMessage), "Check message sizes, this will crash!");
	static unsigned int auxMemCount = 0;
	mpt::String::WriteAutoBuf(auxMem.name) = MPT_WFORMAT("Local\\openmpt-{}-auxmem-{}")(GetCurrentProcessId(), auxMemCount++);
	if(auxMem.memory.Create(auxMem.name, size))
	{
		auxMem.size = size;
		return &auxMem;
	} else
	{
		auxMem.used = false;
		return nullptr;
	}
}


// Send any pending automation events
void BridgeWrapper::SendAutomationQueue()
{
	m_sigAutomation.Reset();
	BridgeMessage msg;
	msg.Automate();
	if(!SendToBridge(msg))
	{
		// Failed (plugin probably crashed) - auto-fix event count
		m_sharedMem->automationQueue.pendingEvents = 0;
	}
	m_sigAutomation.Trigger();
}

void VSTCALLBACK BridgeWrapper::SetParameter(AEffect *effect, int32 index, float parameter)
{
	BridgeWrapper *that = static_cast<BridgeWrapper *>(effect->object);
	if(that)
	{
		try
		{
			that->SetParameter(index, parameter);
		} catch(...)
		{
			// Be quiet about exceptions here
		}
	}
}


void BridgeWrapper::SetParameter(int32 index, float parameter)
{
	const CVstPlugin *plug = static_cast<CVstPlugin *>(m_sharedMem->effect.reservedForHost1);
	AutomationQueue &autoQueue = m_sharedMem->automationQueue;
	if(m_isSettingProgram || (plug && plug->IsResumed()))
	{
		// Queue up messages while rendering to reduce latency introduced by every single bridge call
		uint32 i;
		while((i = autoQueue.pendingEvents.fetch_add(1)) >= std::size(autoQueue.params))
		{
			// Queue full!
			if(i == std::size(autoQueue.params))
			{
				// We're the first to notice that it's full
				SendAutomationQueue();
			} else
			{
				// Wait until queue is emptied by someone else (this branch is very unlikely to happen)
				WaitForSingleObject(m_sigAutomation, INFINITE);
			}
		}

		autoQueue.params[i].index = index;
		autoQueue.params[i].value = parameter;
		return;
	} else if(autoQueue.pendingEvents)
	{
		// Actually, this should never happen as pending events are cleared before processing and at the end of a set program event.
		SendAutomationQueue();
	}

	BridgeMessage msg;
	msg.SetParameter(index, parameter);
	SendToBridge(msg);
}


float VSTCALLBACK BridgeWrapper::GetParameter(AEffect *effect, int32 index)
{
	BridgeWrapper *that = static_cast<BridgeWrapper *>(effect->object);
	if(that)
	{
		if(static_cast<size_t>(index) < that->m_cachedParamValues.size())
			return that->m_cachedParamValues[index];

		try
		{
			return that->GetParameter(index);
		} catch(...)
		{
			// Be quiet about exceptions here
		}
	}
	return 0.0f;
}


float BridgeWrapper::GetParameter(int32 index)
{
	BridgeMessage msg;
	msg.GetParameter(index);
	if(SendToBridge(msg))
	{
		return msg.parameter.value;
	}
	return 0.0f;
}


void VSTCALLBACK BridgeWrapper::Process(AEffect *effect, float **inputs, float **outputs, int32 sampleFrames)
{
	BridgeWrapper *that = static_cast<BridgeWrapper *>(effect->object);
	if(sampleFrames != 0 && that != nullptr)
	{
		that->BuildProcessBuffer(ProcessMsg::process, effect->numInputs, effect->numOutputs, inputs, outputs, sampleFrames);
	}
}


void VSTCALLBACK BridgeWrapper::ProcessReplacing(AEffect *effect, float **inputs, float **outputs, int32 sampleFrames)
{
	BridgeWrapper *that = static_cast<BridgeWrapper *>(effect->object);
	if(sampleFrames != 0 && that != nullptr)
	{
		that->BuildProcessBuffer(ProcessMsg::processReplacing, effect->numInputs, effect->numOutputs, inputs, outputs, sampleFrames);
	}
}


void VSTCALLBACK BridgeWrapper::ProcessDoubleReplacing(AEffect *effect, double **inputs, double **outputs, int32 sampleFrames)
{
	BridgeWrapper *that = static_cast<BridgeWrapper *>(effect->object);
	if(sampleFrames != 0 && that != nullptr)
	{
		that->BuildProcessBuffer(ProcessMsg::processDoubleReplacing, effect->numInputs, effect->numOutputs, inputs, outputs, sampleFrames);
	}
}


template <typename buf_t>
void BridgeWrapper::BuildProcessBuffer(ProcessMsg::ProcessType type, int32 numInputs, int32 numOutputs, buf_t **inputs, buf_t **outputs, int32 sampleFrames)
{
	if(!m_processMem.Good())
	{
		MPT_ASSERT_NOTREACHED();
		return;
	}

	ProcessMsg *processMsg = m_processMem.Data<ProcessMsg>();
	new(processMsg) ProcessMsg{type, numInputs, numOutputs, sampleFrames};

	// Anticipate that many plugins will query the play position in a process call and send it along the process call
	// to save some valuable inter-process calls.
	m_sharedMem->timeInfo = *FromIntPtr<VstTimeInfo>(CVstPlugin::MasterCallBack(&m_sharedMem->effect, audioMasterGetTime, 0, kVstNanosValid | kVstPpqPosValid | kVstTempoValid | kVstBarsValid | kVstCyclePosValid | kVstTimeSigValid | kVstSmpteValid | kVstClockValid, nullptr, 0.0f));

	buf_t *ptr = reinterpret_cast<buf_t *>(processMsg + 1);
	for(int32 i = 0; i < numInputs; i++)
	{
		std::memcpy(ptr, inputs[i], sampleFrames * sizeof(buf_t));
		ptr += sampleFrames;
	}
	// Theoretically, we should memcpy() instead of memset() here in process(), but OpenMPT always clears the output buffer before processing so it doesn't matter.
	memset(ptr, 0, numOutputs * sampleFrames * sizeof(buf_t));

	// In case we called Process() from the GUI thread (panic button or song stop => CSoundFile::SuspendPlugins)
	m_isAudioThread = true;

	m_sigProcessAudio.Send();
	const HANDLE objects[] = {m_sigProcessAudio.confirm, m_sigToHostAudio.send, m_otherProcess};
	DWORD result;
	do
	{
		result = WaitForMultipleObjects(mpt::saturate_cast<DWORD>(std::size(objects)), objects, FALSE, INFINITE);
		if(result == WAIT_OBJECT_0 + 1)
		{
			ParseNextMessage(m_sharedMem->audioThreadToHostMsgID);
			m_sigToHostAudio.Confirm();
		}
	} while(result != WAIT_OBJECT_0 && result != WAIT_OBJECT_0 + 2 && result != WAIT_FAILED);

	m_isAudioThread = false;

	for(int32 i = 0; i < numOutputs; i++)
	{
		//std::memcpy(outputs[i], ptr, sampleFrames * sizeof(buf_t));
		outputs[i] = ptr;  // Exactly what you don't want plugins to do usually (bend your output pointers)... muahahaha!
		ptr += sampleFrames;
	}

	// Did we receive any audioMasterProcessEvents data?
	if(auto *events = m_eventMem.Data<int32>(); events != nullptr && *events != 0)
	{
		std::vector<char> eventCache;
		TranslateBridgeToVstEvents(eventCache, events);
		*events = 0;
		CVstPlugin::MasterCallBack(&m_sharedMem->effect, audioMasterProcessEvents, 0, 0, eventCache.data(), 0.0f);
	}
}


LRESULT CALLBACK BridgeWrapper::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if(hwnd == m_communicationWindow && wParam < m_plugins.size())
	{
		auto *that = static_cast<BridgeWrapper *>(m_plugins[wParam]);
		if(that != nullptr && uMsg == WM_BRIDGE_MESSAGE_TO_HOST)
		{
			that->ParseNextMessage(static_cast<int>(lParam));
			return WM_BRIDGE_SUCCESS;
		}
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

#endif  // NO_VST


OPENMPT_NAMESPACE_END
