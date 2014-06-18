/*
 * BridgeWrapper.h
 * ---------------
 * Purpose: VST plugin bridge wrapper (host side)
 * Notes  : (currently none)
 * Authors: Johannes Schultz (OpenMPT Devs)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BridgeCommon.h"

OPENMPT_NAMESPACE_BEGIN

struct VSTPluginLib;

class BridgeWrapper : protected BridgeCommon
{
protected:
	Event sigAutomation;
	bool isSettingProgram;

	ERect editRect;
	VstSpeakerArrangement speakers[2];

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
	static bool IsPluginNative(const mpt::PathString &pluginPath) { return GetPluginBinaryType(pluginPath) == sizeof(void *) * 8; }
	static uint64 GetFileVersion(const WCHAR *exePath);

	static AEffect *Create(const VSTPluginLib &plugin);

protected:
	BridgeWrapper() : isSettingProgram(false) { }
	~BridgeWrapper();

	bool Init(const mpt::PathString &pluginPath, BridgeWrapper *sharedInstace);

	void MessageThread();

	void ParseNextMessage();
	void DispatchToHost(DispatchMsg *msg);
	bool SendToBridge(BridgeMessage &sendMsg);
	void SendAutomationQueue();

	static VstIntPtr VSTCALLBACK DispatchToPlugin(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt);
	static void VSTCALLBACK SetParameter(AEffect *effect, VstInt32 index, float parameter);
	static float VSTCALLBACK GetParameter(AEffect *effect, VstInt32 index);
	static void VSTCALLBACK Process(AEffect *effect, float **inputs, float **outputs, VstInt32 sampleFrames);
	static void VSTCALLBACK ProcessReplacing(AEffect *effect, float **inputs, float **outputs, VstInt32 sampleFrames);
	static void VSTCALLBACK ProcessDoubleReplacing(AEffect *effect, double **inputs, double **outputs, VstInt32 sampleFrames);

	template<typename buf_t>
	void BuildProcessBuffer(ProcessMsg::ProcessType type, VstInt32 numInputs, VstInt32 numOutputs, buf_t **inputs, buf_t **outputs, VstInt32 sampleFrames);
};

OPENMPT_NAMESPACE_END
