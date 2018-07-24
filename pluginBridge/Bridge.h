/*
 * Bridge.h
 * --------
 * Purpose: VST plugin bridge (plugin side)
 * Notes  : (currently none)
 * Authors: Johannes Schultz (OpenMPT Devs)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

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
	Vst::AEffect *nativeEffect = nullptr;
	HMODULE library = nullptr;
	HWND window = nullptr, windowParent = nullptr;
	int windowWidth = 0, windowHeight = 0;
	LONG isProcessing = 0;

	// Static memory for host-to-plugin pointers
	union
	{
		Vst::VstSpeakerArrangement speakerArrangement;
		char name[256];
	} host2PlugMem;
	std::vector<char> eventCache;	// Cached VST (MIDI) events

	// Pointers to sample data
	std::vector<void *> samplePointers;
	uint32 mixBufSize = 0;

	bool needIdle = false;	// Plugin needs idle time
	bool closeInstance = false;

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
	intptr_t DispatchToHost(Vst::VstOpcodeToHost opcode, int32 index, intptr_t value, void *ptr, float opt);
	intptr_t VstFileSelector(bool destructor, Vst::VstFileSelect &fileSel);
	void SendErrorMessage(const wchar_t *str);
	intptr_t Dispatch(Vst::VstOpcodeToPlugin opcode, int32 index, intptr_t value, void *ptr, float opt);

	template<typename buf_t>
	int32 BuildProcessPointers(buf_t **(&inPointers), buf_t **(&outPointers));

	void RenderThread();

	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	static intptr_t VSTCALLBACK MasterCallback(Vst::AEffect *effect, Vst::VstOpcodeToHost, int32 index, intptr_t value, void *ptr, float opt);
};


OPENMPT_NAMESPACE_END
