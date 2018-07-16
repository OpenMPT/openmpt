/*
 * BridgeWrapper.h
 * ---------------
 * Purpose: VST plugin bridge wrapper (host side)
 * Notes  : (currently none)
 * Authors: Johannes Schultz (OpenMPT Devs)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

#ifndef NO_VST

#include "BridgeCommon.h"
#include "../common/ComponentManager.h"

OPENMPT_NAMESPACE_BEGIN

struct VSTPluginLib;

class ComponentPluginBridge
	: public ComponentBase
{
public:
	enum Availability
	{
		AvailabilityUnknown      =  0,
		AvailabilityOK           =  1,
		AvailabilityMissing      = -1,
		AvailabilityWrongVersion = -2,
	};
private:
	const int bitness;
	mpt::PathString exeName;
	Availability availability;
protected:
	ComponentPluginBridge(int bitness);
protected:
	bool DoInitialize() override;
public:
	Availability GetAvailability() const { return availability; }
	mpt::PathString GetFileName() const { return exeName; }
};

class ComponentPluginBridge32
	: public ComponentPluginBridge
{
	MPT_DECLARE_COMPONENT_MEMBERS
public:
	ComponentPluginBridge32() : ComponentPluginBridge(32) { }
};

class ComponentPluginBridge64
	: public ComponentPluginBridge
{
	MPT_DECLARE_COMPONENT_MEMBERS
public:
	ComponentPluginBridge64() : ComponentPluginBridge(64) { }
};

class BridgeWrapper : protected BridgeCommon
{
protected:
	Event sigAutomation;
	MappedMemory oldProcessMem;

	// Helper struct for keeping track of auxiliary shared memory
	struct AuxMem
	{
		LONG used;
		uint32 size;
		MappedMemory memory;
		wchar_t name[64];

		AuxMem() : used(0), size(0) { }
	};
	AuxMem auxMems[SharedMemLayout::queueSize];
	
	std::vector<char> cachedProgNames;
	std::vector<ParameterInfo> cachedParamInfo;
	int32 cachedProgNameStart, cachedParamInfoStart;
	
	bool isSettingProgram;

	Vst::ERect editRect;
	Vst::VstSpeakerArrangement speakers[2];

	ComponentHandle<ComponentPluginBridge32> pluginBridge32;
	ComponentHandle<ComponentPluginBridge64> pluginBridge64;

public:
	enum BinaryType
	{
		binUnknown = 0,
		bin32Bit = 32,
		bin64Bit = 64,
	};

	// Generic bridge exception
	class BridgeException : public std::exception
	{
	public:
		BridgeException(const char *str) : std::exception(str) { }
		BridgeException() { }
	};
	class BridgeNotFoundException : public BridgeException { };

	// Exception from bridge process
	class BridgeRemoteException
	{
	protected:
		wchar_t *str;
	public:
		BridgeRemoteException(const wchar_t *str_) : str(_wcsdup(str_)) { }
		~BridgeRemoteException() { free(str); }
		const wchar_t *what() const { return str; }
	};

public:
	static BinaryType GetPluginBinaryType(const mpt::PathString &pluginPath);
	static bool IsPluginNative(const mpt::PathString &pluginPath) { return GetPluginBinaryType(pluginPath) == mpt::arch_bits; }
	static uint64 GetFileVersion(const WCHAR *exePath);

	static Vst::AEffect *Create(const VSTPluginLib &plugin);

protected:
	BridgeWrapper() : isSettingProgram(false) { }
	~BridgeWrapper();

	bool Init(const mpt::PathString &pluginPath, BridgeWrapper *sharedInstace);

	void MessageThread();

	void ParseNextMessage();
	void DispatchToHost(DispatchMsg *msg);
	bool SendToBridge(BridgeMessage &sendMsg);
	void SendAutomationQueue();
	AuxMem *GetAuxMemory(uint32 size);

	static intptr_t VSTCALLBACK DispatchToPlugin(Vst::AEffect *effect, Vst::VstOpcodeToPlugin opcode, int32 index, intptr_t value, void *ptr, float opt);
	intptr_t DispatchToPlugin(Vst::VstOpcodeToPlugin opcode, int32 index, intptr_t value, void *ptr, float opt);
	static void VSTCALLBACK SetParameter(Vst::AEffect *effect, int32 index, float parameter);
	void SetParameter(int32 index, float parameter);
	static float VSTCALLBACK GetParameter(Vst::AEffect *effect, int32 index);
	float GetParameter(int32 index);
	static void VSTCALLBACK Process(Vst::AEffect *effect, float **inputs, float **outputs, int32 sampleFrames);
	static void VSTCALLBACK ProcessReplacing(Vst::AEffect *effect, float **inputs, float **outputs, int32 sampleFrames);
	static void VSTCALLBACK ProcessDoubleReplacing(Vst::AEffect *effect, double **inputs, double **outputs, int32 sampleFrames);

	template<typename buf_t>
	void BuildProcessBuffer(ProcessMsg::ProcessType type, int32 numInputs, int32 numOutputs, buf_t **inputs, buf_t **outputs, int32 sampleFrames);
};

OPENMPT_NAMESPACE_END

#endif // NO_VST
