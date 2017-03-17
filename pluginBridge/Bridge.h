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

OPENMPT_NAMESPACE_BEGIN

class PluginBridge : protected BridgeCommon
{
public:
	static Event sigQuit;
	static bool fullMemDump;

protected:
	static LONG instanceCount;
	static PluginBridge *latestInstance;
	static WNDCLASSEX windowClass;

	// Plugin
	AEffect *nativeEffect;
	HMODULE library;
	ERect windowSize;
	HWND window, windowParent;
	LONG isProcessing;

	// Static memory for host-to-plugin pointers
	union
	{
		VstSpeakerArrangement speakerArrangement;
		char name[256];
	} host2PlugMem;
	std::vector<char> eventCache;	// Cached VST (MIDI) events

	// Pointers to sample data
	std::vector<void *> samplePointers;
	uint32 mixBufSize;

	bool needIdle : 1 ;	// Plugin needs idle time
	bool closeInstance : 1;

public:
	PluginBridge(const wchar_t *memName, HANDLE otherProcess);
	~PluginBridge();
	static void InitializeStaticVariables();

protected:
	void MessageThread();

	bool SendToHost(BridgeMessage &sendMsg);

	void UpdateEffectStruct();
	void CreateProcessingFile(std::vector<char> &dispatchData);

	void ParseNextMessage();
	void NewInstance(NewInstanceMsg *msg);
	void InitBridge(InitMsg *msg);
	void DispatchToPlugin(DispatchMsg *msg);
	void SetParameter(ParameterMsg *msg);
	void GetParameter(ParameterMsg *msg);
	void AutomateParameters();
	void Process();
	void ProcessReplacing();
	void ProcessDoubleReplacing();
	VstIntPtr DispatchToHost(VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt);
	VstIntPtr VstFileSelector(bool destructor, VstFileSelect *fileSel);
	void SendErrorMessage(const wchar_t *str);
	VstIntPtr Dispatch(VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt);

	template<typename buf_t>
	int32 BuildProcessPointers(buf_t **(&inPointers), buf_t **(&outPointers));

	void RenderThread();

	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	static VstIntPtr VSTCALLBACK MasterCallback(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt);
};


OPENMPT_NAMESPACE_END
