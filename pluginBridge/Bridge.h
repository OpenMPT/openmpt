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
#include "../common/mptThread.h"

OPENMPT_NAMESPACE_BEGIN

class PluginBridge : private BridgeCommon
{
public:
	static bool m_fullMemDump;

protected:
	enum TimerID : UINT
	{
		TIMER_IDLE = 1,
	};

	static PluginBridge *m_latestInstance;
	static ATOM m_editorClassAtom;

	// Plugin
	Vst::AEffect *m_nativeEffect = nullptr;
	HMODULE m_library = nullptr;
	HWND m_window = nullptr;
	int m_windowWidth = 0, m_windowHeight = 0;
	std::atomic<bool> m_isProcessing = false;

	// Static memory for host-to-plugin pointers
	union
	{
		Vst::VstSpeakerArrangement speakerArrangement;
		char name[256];
	} m_host2PlugMem;
	std::vector<char> m_eventCache;       // Cached VST (MIDI) events
	std::vector<char> m_fileSelectCache;  // Cached VstFileSelect data

	// Pointers to sample data
	std::vector<void *> m_sampleBuffers;
	uint32 m_mixBufSize = 0;

	HANDLE m_audioThread = nullptr;
	Event m_sigThreadExit;  // Signal to kill audio thread

	bool m_needIdle = false;  // Plugin needs idle time

public:
	PluginBridge(const wchar_t *memName, HANDLE otherProcess);
	~PluginBridge();
	
	PluginBridge(const PluginBridge&) = delete;
	PluginBridge &operator=(const PluginBridge&) = delete;

	static void MainLoop(TCHAR *argv[]);

protected:
	void RequestDelete();

	bool SendToHost(BridgeMessage &sendMsg);

	void UpdateEffectStruct();
	void CreateProcessingFile(std::vector<char> &dispatchData);

	void ParseNextMessage(int msgID);
	void NewInstance(NewInstanceMsg &msg);
	void InitBridge(InitMsg &msg);
	void DispatchToPlugin(DispatchMsg &msg);
	void SetParameter(ParameterMsg &msg);
	void GetParameter(ParameterMsg &msg);
	void AutomateParameters();
	void Process();
	void ProcessReplacing();
	void ProcessDoubleReplacing();
	intptr_t DispatchToHost(Vst::VstOpcodeToHost opcode, int32 index, intptr_t value, void *ptr, float opt);
	void SendErrorMessage(const wchar_t *str);
	intptr_t Dispatch(Vst::VstOpcodeToPlugin opcode, int32 index, intptr_t value, void *ptr, float opt);

	template<typename buf_t>
	int32 BuildProcessPointers(buf_t **(&inPointers), buf_t **(&outPointers));

	static DWORD WINAPI AudioThread(LPVOID param);
	void AudioThread();

	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static void CALLBACK IdleTimerProc(HWND hwnd, UINT message, UINT_PTR idTimer, DWORD dwTime);

	static intptr_t VSTCALLBACK MasterCallback(Vst::AEffect *effect, Vst::VstOpcodeToHost, int32 index, intptr_t value, void *ptr, float opt);
};


OPENMPT_NAMESPACE_END
