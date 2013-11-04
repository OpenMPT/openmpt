#pragma once

#include <vector>
#include "BridgeCommon.h"

class PluginBridge
{
protected:
	static PluginBridge *latestInstance;

	// Plugin
	AEffect *nativeEffect;
	HINSTANCE library;
	ERect windowSize;
	HWND window;

	// Static memory for host-to-plugin pointers
	union
	{
		VstSpeakerArrangement speakerArrangement;
		VstTimeInfo timeInfo;
		char name[256];
	} host2PlugMem;

	// Shared memory
	SharedMem sharedMem;

	// Pointers to sample data
	std::vector<void *> samplePointers;

public:
	PluginBridge(TCHAR *argv[]);

protected:
	bool CreateMapping(const TCHAR *memName);
	void CloseMapping();
	const MsgHeader *SendToHost(const MsgHeader &msg, MsgHeader *sourceHeader = nullptr);

	void UpdateEffectStruct();

	void ParseNextMessage(MsgHeader *parentMessage = nullptr);
	void InitBridge(InitMsg *msg);
	void CloseBridge();
	void ReallocateBridge(ReallocMsg *msg);
	void DispatchToPlugin(DispatchMsg *msg);
	void SetParameter(ParameterMsg *msg);
	void GetParameter(ParameterMsg *msg);
	void Process();
	void ProcessReplacing();
	void ProcessDoubleReplacing();
	VstIntPtr DispatchToHost(VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt);

	template<typename buf_t>
	int32_t BuildProcessPointers(buf_t **(&inPointers), buf_t **(&outPointers));

	static DWORD WINAPI RenderThread(LPVOID param);

	static VstIntPtr VSTCALLBACK MasterCallback(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt);
};