/*
 * Bridge.h
 * --------
 * Purpose: VST plugin bridge (plugin side)
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"
#include <atomic>

#include "BridgeCommon.h"

OPENMPT_NAMESPACE_BEGIN

// This counter is not part of class PluginBridge to avoid triggering the quit signal before all heap memory allocated by members of class PluginBridge has been freed.
class BridgeInstanceCounter
{
public:
	BridgeInstanceCounter();
	~BridgeInstanceCounter();

	static Event m_sigQuit;

private:
	static std::atomic<uint32> m_instanceCount;
};


class PluginBridge : private BridgeInstanceCounter, private BridgeCommon
{
public:
	static bool m_fullMemDump;

protected:
	static PluginBridge *m_latestInstance;
	static WNDCLASSEX m_windowClass;
	thread_local static int m_inMessageStack;  // To determine if a message is a reply to another message, or if we need to send to the host's dedicated message thread

	// Plugin
	Vst::AEffect *m_nativeEffect = nullptr;
	HMODULE m_library = nullptr;
	HWND m_window = nullptr, m_windowParent = nullptr;
	int m_windowWidth = 0, m_windowHeight = 0;
	enum ProcessMode
	{
		kNotProcessing,
		kAboutToProcess,
		kProcessing,
	};
	std::atomic<ProcessMode> m_isProcessing = kNotProcessing;

	// Static memory for host-to-plugin pointers
	union
	{
		Vst::VstSpeakerArrangement speakerArrangement;
		char name[256];
	} m_host2PlugMem;
	std::vector<char> m_eventCache;  // Cached VST (MIDI) events

	// Pointers to sample data
	std::vector<void *> m_sampleBuffers;
	uint32 m_mixBufSize = 0;

	bool m_needIdle = false;  // Plugin needs idle time
	bool m_closeInstance = false;

public:
	PluginBridge(const wchar_t *memName, HANDLE otherProcess);
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

	void AudioThread();

	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	static intptr_t VSTCALLBACK MasterCallback(Vst::AEffect *effect, Vst::VstOpcodeToHost, int32 index, intptr_t value, void *ptr, float opt);
};


OPENMPT_NAMESPACE_END
