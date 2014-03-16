#pragma once

#include "BridgeCommon.h"

class BridgeWrapper
{
protected:
	// Shared signals and memory
	SharedMem sharedMem;

public:
	enum BinaryType
	{
		binUnknown = 0,
		bin32Bit = 32,
		bin64Bit = 64,
	};

public:
	BridgeWrapper(const mpt::PathString &pluginPath);
	~BridgeWrapper();

	static BinaryType GetPluginBinaryType(const mpt::PathString &pluginPath);

	AEffect *GetEffect() { return sharedMem.queueMem.Good() ? sharedMem.effectPtr : nullptr; }

protected:

	void MessageThread();

	void ParseNextMessage();
	void DispatchToHost(DispatchMsg *msg);
	const BridgeMessage *SendToBridge(const BridgeMessage &msg);

	static VstIntPtr VSTCALLBACK DispatchToPlugin(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt);
	static void VSTCALLBACK SetParameter(AEffect *effect, VstInt32 index, float parameter);
	static float VSTCALLBACK GetParameter(AEffect *effect, VstInt32 index);
	static void VSTCALLBACK Process(AEffect *effect, float **inputs, float **outputs, VstInt32 sampleFrames);
	static void VSTCALLBACK ProcessReplacing(AEffect *effect, float **inputs, float **outputs, VstInt32 sampleFrames);
	static void VSTCALLBACK ProcessDoubleReplacing(AEffect *effect, double **inputs, double **outputs, VstInt32 sampleFrames);

	template<typename buf_t>
	void BuildProcessBuffer(ProcessMsg::ProcessType type, VstInt32 numInputs, VstInt32 numOutputs, buf_t **inputs, buf_t **outputs, VstInt32 sampleFrames);
};
