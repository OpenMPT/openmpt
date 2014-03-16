/*
 * Bridge.h
 * --------
 * Purpose: VST plugin bridge (plugin side)
 * Notes  : (currently none)
 * Authors: Johannes Schultz (OpenMPT Devs)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


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
	WNDCLASSEX windowClass;
	uint32_t isProcessing;

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
	uint32_t mixBufSize;

public:
	PluginBridge(TCHAR *memName, HANDLE otherProcess, Signal &sigToHost, Signal &sigToBridge, Signal &sigProcess);

protected:
	//bool CreateMapping(const TCHAR *memName);
	void CloseMapping();
	const BridgeMessage *SendToHost(const BridgeMessage &msg);

	void UpdateEffectStruct();
	void CreateProcessingFile(std::vector<char> &dispatchData);

	void ParseNextMessage();
	void NewInstance(NewInstanceMsg *msg);
	void InitBridge(InitMsg *msg);
	void CloseBridge();
	void DispatchToPlugin(DispatchMsg *msg);
	void SetParameter(ParameterMsg *msg);
	void GetParameter(ParameterMsg *msg);
	void Process();
	void ProcessReplacing();
	void ProcessDoubleReplacing();
	VstIntPtr DispatchToHost(VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt);
	void SendErrorMessage(const wchar_t *str);

	template<typename buf_t>
	int32_t BuildProcessPointers(buf_t **(&inPointers), buf_t **(&outPointers));

	void RenderThread();

	static VstIntPtr VSTCALLBACK MasterCallback(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt);
};