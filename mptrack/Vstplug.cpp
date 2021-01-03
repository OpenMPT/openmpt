/*
 * Vstplug.cpp
 * -----------
 * Purpose: VST Plugin handling / processing
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#ifndef NO_VST

#include "Vstplug.h"
#ifdef MODPLUG_TRACKER
#include "Moddoc.h"
#include "Mainfrm.h"
#include "AbstractVstEditor.h"
#include "VSTEditor.h"
#include "DefaultVstEditor.h"
#include "ExceptionHandler.h"
#endif // MODPLUG_TRACKER
#include "../soundlib/Sndfile.h"
#include "../soundlib/MIDIEvents.h"
#include "MIDIMappingDialog.h"
#include "../common/mptStringBuffer.h"
#include "FileDialog.h"
#include "../pluginBridge/BridgeWrapper.h"
#include "../pluginBridge/BridgeOpCodes.h"
#include "../soundlib/plugins/OpCodes.h"
#include "../soundlib/plugins/PluginManager.h"
#include "../common/mptOSException.h"

using namespace Vst;
DECLARE_FLAGSET(Vst::VstTimeInfoFlags)

OPENMPT_NAMESPACE_BEGIN

static VstTimeInfo g_timeInfoFallback = { 0 };

#ifdef MPT_ALL_LOGGING
#define VST_LOG
#endif


using VstCrash = Windows::SEH::Code;


bool CVstPlugin::MaskCrashes() noexcept
{
	return m_maskCrashes;
}


template <typename Tfn>
DWORD CVstPlugin::SETryOrError(bool maskCrashes, Tfn fn)
{
	DWORD exception = 0;
	if(maskCrashes)
	{
		exception = Windows::SEH::TryOrError(fn);
		if(exception)
		{
			ExceptionHandler::TaintProcess(ExceptionHandler::TaintReason::Plugin);
		}
	} else
	{
		fn();
	}
	return exception;
}


template <typename Tfn>
DWORD CVstPlugin::SETryOrError(Tfn fn)
{
	DWORD exception = 0;
	if(MaskCrashes())
	{
		exception = Windows::SEH::TryOrError(fn);
		if(exception)
		{
			ExceptionHandler::TaintProcess(ExceptionHandler::TaintReason::Plugin);
		}
	} else
	{
#ifdef MODPLUG_TRACKER
		ExceptionHandler::ContextSetter ectxguard{&m_Ectx};
#endif // MODPLUG_TRACKER
		fn();
	}
	return exception;
}


AEffect *CVstPlugin::LoadPlugin(bool maskCrashes, VSTPluginLib &plugin, HMODULE &library, bool forceBridge, bool forceLegacy)
{
	const mpt::PathString &pluginPath = plugin.dllPath;

	AEffect *effect = nullptr;
	library = nullptr;

	const bool isNative = plugin.IsNative(false);
	if(forceBridge || plugin.useBridge || !isNative)
	{
		try
		{
			effect = BridgeWrapper::Create(plugin, forceLegacy);
			if(effect != nullptr)
			{
				return effect;
			}
		} catch(BridgeWrapper::BridgeNotFoundException &)
		{
			// Try normal loading
			if(!isNative)
			{
				Reporting::Error("Could not locate the plugin bridge executable, which is required for running non-native plugins.", "OpenMPT Plugin Bridge");
				return nullptr;
			}
		} catch(BridgeWrapper::BridgeException &e)
		{
			// If there was some error, don't try normal loading as well... unless the user really wants it.
			if(isNative)
			{
				const CString msg =
					MPT_CFORMAT("The following error occurred while trying to load\n{}\n\n{}\n\nDo you want to try to load the plugin natively?")
					(plugin.dllPath, mpt::get_exception_text<mpt::ustring>(e));
				if(Reporting::Confirm(msg, _T("OpenMPT Plugin Bridge")) == cnfNo)
				{
					return nullptr;
				}
			} else
			{
				Reporting::Error(mpt::get_exception_text<mpt::ustring>(e), "OpenMPT Plugin Bridge");
				return nullptr;
			}
		}
		// If plugin was marked to use the plugin bridge but this somehow doesn't work (e.g. because the bridge is missing),
		// disable the plugin bridge for this plugin.
		plugin.useBridge = false;
	}

	{
#ifdef MODPLUG_TRACKER
		ExceptionHandler::Context ectx{ MPT_UFORMAT("VST Plugin: {}")(plugin.dllPath.ToUnicode()) };
		ExceptionHandler::ContextSetter ectxguard{&ectx};
#endif // MODPLUG_TRACKER
		DWORD exception = SETryOrError(maskCrashes, [&](){ library = LoadLibrary(pluginPath.AsNative().c_str()); });
		if(exception)
		{
			CVstPluginManager::ReportPlugException(MPT_UFORMAT("Exception caught while loading {}")(pluginPath));
			return nullptr;
		}
	}
	if(library == nullptr)
	{
		DWORD error = GetLastError();
		if(error == ERROR_MOD_NOT_FOUND)
		{
			return nullptr;
		} else if(error == ERROR_DLL_INIT_FAILED)
		{
			// A likely reason for this error is that Fiber Local Storage slots are exhausted, e.g. because too many plugins ship with a statically linked runtime.
			// Before Windows 10 1903, there was a limit of 128 FLS slots per process, and the VS2017 runtime uses two FLS slots, so this could cause a worst-case limit
			// of 62 different plugins per process (assuming they all use a statically-linked runtime).
			// In Windows 10 1903, the FLS limit was finally raised, so this message is mostly relevant for older systems.
			CVstPluginManager::ReportPlugException(U_("Plugin initialization failed. This may be caused by loading too many plugins.\nTry activating the Plugin Bridge for this plugin."));
		}

#ifdef _DEBUG
		mpt::ustring buf = MPT_UFORMAT("Warning: encountered problem when loading plugin dll. Error {}: {}")
			( mpt::ufmt::hex(error)
			, mpt::ToUnicode(mpt::Windows::GetErrorMessage(error))
			);
		Reporting::Error(buf, "DEBUG: Error when loading plugin dll");
#endif //_DEBUG
	}

	if(library != nullptr && library != INVALID_HANDLE_VALUE)
	{
		auto pMainProc = (Vst::MainProc)GetProcAddress(library, "VSTPluginMain");
		if(pMainProc == nullptr)
		{
			pMainProc = (Vst::MainProc)GetProcAddress(library, "main");
		}

		if(pMainProc != nullptr)
		{
#ifdef MODPLUG_TRACKER
			ExceptionHandler::Context ectx{ MPT_UFORMAT("VST Plugin: {}")(plugin.dllPath.ToUnicode()) };
			ExceptionHandler::ContextSetter ectxguard{&ectx};
#endif // MODPLUG_TRACKER
			DWORD exception = SETryOrError(maskCrashes, [&](){ effect = pMainProc(CVstPlugin::MasterCallBack); });
			if(exception)
			{
				return nullptr;
			}
		} else
		{
#ifdef VST_LOG
			MPT_LOG(LogDebug, "VST", MPT_UFORMAT("Entry point not found! (handle={})")(mpt::ufmt::PTR(library)));
#endif // VST_LOG
			return nullptr;
		}
	}

	return effect;
}

static void operator|= (Vst::VstTimeInfoFlags &lhs, Vst::VstTimeInfoFlags rhs)
{
	lhs = (lhs | rhs).as_enum();
}

intptr_t VSTCALLBACK CVstPlugin::MasterCallBack(AEffect *effect, VstOpcodeToHost opcode, int32 index, intptr_t value, void *ptr, float opt)
{
#ifdef VST_LOG
	MPT_LOG(LogDebug, "VST", MPT_UFORMAT("VST plugin to host: Eff: {}, Opcode = {}, Index = {}, Value = {}, PTR = {}, OPT = {}\n")(
		mpt::ufmt::PTR(effect), mpt::ufmt::val(opcode),
		mpt::ufmt::val(index), mpt::ufmt::PTR(value), mpt::ufmt::PTR(ptr), mpt::ufmt::flt(opt, 3)));
	MPT_TRACE();
#else
	MPT_UNREFERENCED_PARAMETER(opt);
#endif

	enum
	{
		HostDoNotKnow	= 0,
		HostCanDo		= 1,
		HostCanNotDo	= -1
	};

	CVstPlugin *pVstPlugin = nullptr;
	if(effect != nullptr)
	{
		pVstPlugin = static_cast<CVstPlugin *>(effect->reservedForHost1);
	}

	switch(opcode)
	{
	// Called when plugin param is changed via gui
	case audioMasterAutomate:
		// Strum Acoustic GS-1 and Strum Electric GS-1 send audioMasterAutomate during effOpen (WTF #1),
		// but when sending back effCanBeAutomated, they just crash (WTF #2).
		// As a consequence, just generally forbid this action while the plugin is not fully initialized yet.
		if(pVstPlugin != nullptr && pVstPlugin->m_isInitialized && pVstPlugin->CanAutomateParameter(index))
		{
			// This parameter can be automated. Ugo Motion constantly sends automation callback events for parameters that cannot be automated...
			pVstPlugin->AutomateParameter((PlugParamIndex)index);
		}
		return 0;

	// Called when plugin asks for VST version supported by host
	case audioMasterVersion:
		return kVstVersion;

	// Returns the unique id of a plugin that's currently loading
	// We don't support shell plugins currently, so we only support one effect ID as well.
	case audioMasterCurrentId:
		return (effect != nullptr) ? effect->uniqueID : 0;

	// Call application idle routine (this will call effEditIdle for all open editors too)
	case audioMasterIdle:
		theApp.GetPluginManager()->OnIdle();
		return 0;

	// Inquire if an input or output is beeing connected; index enumerates input or output counting from zero,
	// value is 0 for input and != 0 otherwise. note: the return value is 0 for <true> such that older versions
	// will always return true.
	case audioMasterPinConnected:
		if (value)	//input:
			return (index < 2) ? 0 : 1;		//we only support up to 2 inputs. Remember: 0 means yes.
		else		//output:
			return (index < 2) ? 0 : 1;		//2 outputs max too

	//---from here VST 2.0 extension opcodes------------------------------------------------------

	// <value> is a filter which is currently ignored - DEPRECATED in VST 2.4
	case audioMasterWantMidi:
		return 1;

	// returns const VstTimeInfo* (or 0 if not supported)
	// <value> should contain a mask indicating which fields are required
	case audioMasterGetTime:

		if(pVstPlugin)
		{
			VstTimeInfo &timeInfo = pVstPlugin->timeInfo;
			MemsetZero(timeInfo);

			timeInfo.sampleRate = pVstPlugin->m_nSampleRate;
			CSoundFile &sndFile = pVstPlugin->GetSoundFile();
			if(pVstPlugin->IsSongPlaying())
			{
				timeInfo.flags |= kVstTransportPlaying;
				if(pVstPlugin->GetSoundFile().m_SongFlags[SONG_PATTERNLOOP]) timeInfo.flags |= kVstTransportCycleActive;
				timeInfo.samplePos = sndFile.GetTotalSampleCount();
				if(pVstPlugin->m_positionChanged)
				{
					timeInfo.flags |= kVstTransportChanged;
					pVstPlugin->lastBarStartPos = -1.0;
				}
			} else
			{
				timeInfo.flags |= kVstTransportChanged; //just stopped.
				timeInfo.samplePos = 0;
				pVstPlugin->lastBarStartPos = -1.0;
			}
			if((value & kVstNanosValid))
			{
				timeInfo.flags |= kVstNanosValid;
				timeInfo.nanoSeconds = static_cast<double>(Util::mul32to64_unsigned(timeGetTime(), 1000000));
			}
			if((value & kVstPpqPosValid))
			{
				timeInfo.flags |= kVstPpqPosValid;
				if (timeInfo.flags & kVstTransportPlaying)
				{
					timeInfo.ppqPos = (timeInfo.samplePos / timeInfo.sampleRate) * (sndFile.GetCurrentBPM() / 60.0);
				} else
				{
					timeInfo.ppqPos = 0;
				}

				ROWINDEX rpm = pVstPlugin->GetSoundFile().m_PlayState.m_nCurrentRowsPerMeasure;
				if(!rpm)
					rpm = 4;
				if((pVstPlugin->GetSoundFile().m_PlayState.m_nRow % rpm) == 0)
				{
					pVstPlugin->lastBarStartPos = std::floor(timeInfo.ppqPos);
				}
				if(pVstPlugin->lastBarStartPos >= 0)
				{
					timeInfo.barStartPos = pVstPlugin->lastBarStartPos;
					timeInfo.flags |= kVstBarsValid;
				}
			}
			if((value & kVstTempoValid))
			{
				timeInfo.tempo = sndFile.GetCurrentBPM();
				if (timeInfo.tempo)
				{
					timeInfo.flags |= kVstTempoValid;
				}
			}
			if((value & kVstTimeSigValid))
			{
				timeInfo.flags |= kVstTimeSigValid;

				// Time signature. numerator = rows per beats / rows pear measure (should sound somewhat logical to you).
				// the denominator is a bit more tricky, since it cannot be set explicitely. so we just assume quarters for now.
				ROWINDEX rpb = std::max(sndFile.m_PlayState.m_nCurrentRowsPerBeat, ROWINDEX(1));
				timeInfo.timeSigNumerator = std::max(sndFile.m_PlayState.m_nCurrentRowsPerMeasure, rpb) / rpb;
				timeInfo.timeSigDenominator = 4; //std::gcd(pSndFile->m_nCurrentRowsPerMeasure, pSndFile->m_nCurrentRowsPerBeat);
			}
			return ToIntPtr(&timeInfo);
		} else
		{
			MemsetZero(g_timeInfoFallback);
			return ToIntPtr(&g_timeInfoFallback);
		}

	// Receive MIDI events from plugin
	case audioMasterProcessEvents:
		if(pVstPlugin != nullptr && ptr != nullptr)
		{
			pVstPlugin->ReceiveVSTEvents(static_cast<VstEvents *>(ptr));
			return 1;
		}
		break;

	// DEPRECATED in VST 2.4
	case audioMasterSetTime:
		MPT_LOG(LogDebug, "VST", U_("VST plugin to host: Set Time"));
		break;

	// returns tempo (in bpm * 10000) at sample frame location passed in <value> - DEPRECATED in VST 2.4
	case audioMasterTempoAt:
		// Screw it! Let's just return the tempo at this point in time (might be a bit wrong).
		if (pVstPlugin != nullptr)
		{
			return mpt::saturate_round<int32>(pVstPlugin->GetSoundFile().GetCurrentBPM() * 10000);
		}
		return (125 * 10000);

	// parameters - DEPRECATED in VST 2.4
	case audioMasterGetNumAutomatableParameters:
		//MPT_LOG(LogDebug, "VST", U_("VST plugin to host: Get Num Automatable Parameters"));
		if(pVstPlugin != nullptr)
		{
			return pVstPlugin->GetNumParameters();
		}
		break;

	// Apparently, this one is broken in VST SDK anyway. - DEPRECATED in VST 2.4
	case audioMasterGetParameterQuantization:
		MPT_LOG(LogDebug, "VST", U_("VST plugin to host: Audio Master Get Parameter Quantization"));
		break;

	// numInputs and/or numOutputs has changed
	case audioMasterIOChanged:
		if(pVstPlugin != nullptr)
		{
			CriticalSection cs;
			return pVstPlugin->InitializeIOBuffers() ? 1 : 0;
		}
		break;

	// Plugin needs idle calls (outside its editor window) - DEPRECATED in VST 2.4
	case audioMasterNeedIdle:
		if(pVstPlugin != nullptr)
		{
			pVstPlugin->m_needIdle = true;
		}

		return 1;

	// index: width, value: height
	case audioMasterSizeWindow:
		if(pVstPlugin != nullptr)
		{
			CAbstractVstEditor *pVstEditor = pVstPlugin->GetEditor();
			if (pVstEditor && pVstEditor->IsResizable())
			{
				pVstEditor->SetSize(index, static_cast<int>(value));
			}
		}
		MPT_LOG(LogDebug, "VST", U_("VST plugin to host: Size Window"));
		return 1;

	case audioMasterGetSampleRate:
		if(pVstPlugin)
		{
			return pVstPlugin->m_nSampleRate;
		} else
		{
			// HERCs Abakos queries the sample rate while the plugin is being created and then never again...
			return TrackerSettings::Instance().GetMixerSettings().gdwMixingFreq;
		}

	case audioMasterGetBlockSize:
		return MIXBUFFERSIZE;

	case audioMasterGetInputLatency:
		MPT_LOG(LogDebug, "VST", U_("VST plugin to host: Get Input Latency"));
		break;

	case audioMasterGetOutputLatency:
		if(pVstPlugin)
		{
			return mpt::saturate_round<intptr_t>(pVstPlugin->GetOutputLatency() * pVstPlugin->GetSoundFile().GetSampleRate());
		}
		break;

	// input pin in <value> (-1: first to come), returns cEffect* - DEPRECATED in VST 2.4
	case audioMasterGetPreviousPlug:
		if(pVstPlugin != nullptr)
		{
			std::vector<IMixPlugin *> list;
			if(pVstPlugin->GetInputPlugList(list) != 0)
			{
				// We don't assign plugins to pins...
				CVstPlugin *plugin = dynamic_cast<CVstPlugin *>(list[0]);
				if(plugin != nullptr)
				{
					return ToIntPtr(&plugin->m_Effect);
				}
			}
		}
		break;

	// output pin in <value> (-1: first to come), returns cEffect* - DEPRECATED in VST 2.4
	case audioMasterGetNextPlug:
		if(pVstPlugin != nullptr)
		{
			std::vector<IMixPlugin *> list;
			if(pVstPlugin->GetOutputPlugList(list) != 0)
			{
				// We don't assign plugins to pins...
				CVstPlugin *plugin = dynamic_cast<CVstPlugin *>(list[0]);
				if(plugin != nullptr)
				{
					return ToIntPtr(&plugin->m_Effect);
				}
			}
		}
		break;

	// realtime info
	// returns: 0: not supported, 1: replace, 2: accumulate - DEPRECATED in VST 2.4 (replace is default)
	case audioMasterWillReplaceOrAccumulate:
		return 1; //we replace.

	case audioMasterGetCurrentProcessLevel:
		if(pVstPlugin != nullptr && pVstPlugin->GetSoundFile().IsRenderingToDisc())
			return kVstProcessLevelOffline;
		else
			return kVstProcessLevelRealtime;
		break;

	// returns 0: not supported, 1: off, 2:read, 3:write, 4:read/write
	case audioMasterGetAutomationState:
		// Not entirely sure what this means. We can write automation TO the plug.
		// Is that "read" in this context?
		//MPT_LOG(LogDebug, "VST", U_("VST plugin to host: Get Automation State"));
		return kVstAutomationReadWrite;

	case audioMasterOfflineStart:
		MPT_LOG(LogDebug, "VST", U_("VST plugin to host: Offlinestart"));
		break;

	case audioMasterOfflineRead:
		MPT_LOG(LogDebug, "VST", U_("VST plugin to host: Offlineread"));
		break;

	case audioMasterOfflineWrite:
		MPT_LOG(LogDebug, "VST", U_("VST plugin to host: Offlinewrite"));
		break;

	case audioMasterOfflineGetCurrentPass:
		MPT_LOG(LogDebug, "VST", U_("VST plugin to host: OfflineGetcurrentpass"));
		break;

	case audioMasterOfflineGetCurrentMetaPass:
		MPT_LOG(LogDebug, "VST", U_("VST plugin to host: OfflineGetCurrentMetapass"));
		break;

	// for variable i/o, sample rate in <opt> - DEPRECATED in VST 2.4
	case audioMasterSetOutputSampleRate:
		MPT_LOG(LogDebug, "VST", U_("VST plugin to host: Set Output Sample Rate"));
		break;

	// result in ret - DEPRECATED in VST 2.4
	case audioMasterGetOutputSpeakerArrangement:
		MPT_LOG(LogDebug, "VST", U_("VST plugin to host: Get Output Speaker Arrangement"));
		break;

	case audioMasterGetVendorString:
		strcpy((char *) ptr, TrackerSettings::Instance().vstHostVendorString.Get().c_str());
		return 1;

	case audioMasterGetProductString:
		strcpy((char *) ptr, TrackerSettings::Instance().vstHostProductString.Get().c_str());
		return 1;

	case audioMasterGetVendorVersion:
		return TrackerSettings::Instance().vstHostVendorVersion;

	case audioMasterVendorSpecific:
		return 0;

	// void* in <ptr>, format not defined yet - DEPRECATED in VST 2.4
	case audioMasterSetIcon:
		MPT_LOG(LogDebug, "VST", U_("VST plugin to host: Set Icon"));
		break;

	// string in ptr, see below
	case audioMasterCanDo:
		//Other possible Can Do strings are:
		if(!strcmp((char*)ptr, HostCanDo::sendVstEvents)
		   || !strcmp((char *)ptr, HostCanDo::sendVstMidiEvent)
		   || !strcmp((char *)ptr, HostCanDo::sendVstTimeInfo)
		   || !strcmp((char *)ptr, HostCanDo::receiveVstEvents)
		   || !strcmp((char *)ptr, HostCanDo::receiveVstMidiEvent)
		   || !strcmp((char *)ptr, HostCanDo::supplyIdle)
		   || !strcmp((char *)ptr, HostCanDo::sizeWindow)
		   || !strcmp((char *)ptr, HostCanDo::openFileSelector)
		   || !strcmp((char *)ptr, HostCanDo::closeFileSelector)
		   || !strcmp((char *)ptr, HostCanDo::acceptIOChanges)
		   || !strcmp((char *)ptr, HostCanDo::reportConnectionChanges))
		{
			return HostCanDo;
		} else
		{
			return HostCanNotDo;
		}

	case audioMasterGetLanguage:
		return kVstLangEnglish;

	// returns platform specific ptr - DEPRECATED in VST 2.4
	case audioMasterOpenWindow:
		MPT_LOG(LogDebug, "VST", U_("VST plugin to host: Open Window"));
		break;

	// close window, platform specific handle in <ptr> - DEPRECATED in VST 2.4
	case audioMasterCloseWindow:
		MPT_LOG(LogDebug, "VST", U_("VST plugin to host: Close Window"));
		break;

	// get plugin directory, FSSpec on MAC, else char*
	case audioMasterGetDirectory:
		//MPT_LOG(LogDebug, "VST", U_("VST plugin to host: Get Directory"));
		// Need to allocate space for path only, but I guess noone relies on this anyway.
		//return ToVstPtr(pVstPlugin->GetPluginFactory().dllPath.GetPath().ToLocale());
		//return ToVstPtr(TrackerSettings::Instance().PathPlugins.GetDefaultDir());
		break;

	// something has changed, update 'multi-fx' display
	case audioMasterUpdateDisplay:
		if(pVstPlugin != nullptr)
		{
			// Note to self for testing: Electri-Q sends opcode. Korg M1 sends this when switching between Combi and Multi mode to update the preset names.
			CAbstractVstEditor *pVstEditor = pVstPlugin->GetEditor();
			if(pVstEditor && ::IsWindow(pVstEditor->m_hWnd))
			{
				pVstEditor->UpdateDisplay();
			}
		}
		return 0;

	//---from here VST 2.1 extension opcodes------------------------------------------------------

	// begin of automation session (when mouse down), parameter index in <index>
	case audioMasterBeginEdit:
		MPT_LOG(LogDebug, "VST", U_("VST plugin to host: Begin Edit"));
		break;

	// end of automation session (when mouse up),     parameter index in <index>
	case audioMasterEndEdit:
		MPT_LOG(LogDebug, "VST", U_("VST plugin to host: End Edit"));
		break;

	// open a fileselector window with VstFileSelect* in <ptr>
	case audioMasterOpenFileSelector:

	//---from here VST 2.2 extension opcodes------------------------------------------------------

	// close a fileselector operation with VstFileSelect* in <ptr>: Must be always called after an open !
	case audioMasterCloseFileSelector:
		if(pVstPlugin != nullptr && ptr != nullptr)
		{
			return pVstPlugin->VstFileSelector(opcode == audioMasterCloseFileSelector, *static_cast<VstFileSelect *>(ptr));
		}

	// open an editor for audio (defined by XML text in ptr) - DEPRECATED in VST 2.4
	case audioMasterEditFile:
		MPT_LOG(LogDebug, "VST", U_("VST plugin to host: Edit File"));
		break;

	// get the native path of currently loading bank or project
	// (called from writeChunk) void* in <ptr> (char[2048], or sizeof(FSSpec)) - DEPRECATED in VST 2.4
	// Note: The shortcircuit VSTi actually uses this feature.
	case audioMasterGetChunkFile:
#ifdef MODPLUG_TRACKER
		if(pVstPlugin && pVstPlugin->GetModDoc())
		{
			mpt::ustring pathStr = TrackerSettings::Instance().pluginProjectPath;
			if(pathStr.empty())
			{
				pathStr = U_("%1");
			}
			const mpt::PathString projectPath = pVstPlugin->GetModDoc()->GetPathNameMpt().GetPath();
			const mpt::PathString projectFile = pVstPlugin->GetModDoc()->GetPathNameMpt().GetFullFileName();
			pathStr = mpt::String::Replace(pathStr, U_("%1"), U_("?1?"));
			pathStr = mpt::String::Replace(pathStr, U_("%2"), U_("?2?"));
			pathStr = mpt::String::Replace(pathStr, U_("?1?"), projectPath.ToUnicode());
			pathStr = mpt::String::Replace(pathStr, U_("?2?"), projectFile.ToUnicode());
			mpt::PathString path = mpt::PathString::FromUnicode(pathStr);
			if(path.empty())
			{
				return 0;
			}
			path.EnsureTrailingSlash();
			::SHCreateDirectoryEx(NULL, path.AsNative().c_str(), nullptr);
			path += projectFile;
			strcpy(static_cast<char*>(ptr), path.ToLocale().c_str());
			return 1;
		}
#endif
		MPT_LOG(LogDebug, "VST", U_("VST plugin to host: Get Chunk File"));
		break;

	//---from here VST 2.3 extension opcodes------------------------------------------------------

	// result a VstSpeakerArrangement in ret - DEPRECATED in VST 2.4
	case audioMasterGetInputSpeakerArrangement:
		MPT_LOG(LogDebug, "VST", U_("VST plugin to host: Get Input Speaker Arrangement"));
		break;

	}

	// Unknown codes:

	return 0;
}


// Helper function for file selection dialog stuff.
intptr_t CVstPlugin::VstFileSelector(bool destructor, VstFileSelect &fileSel)
{
	if(!destructor)
	{
		fileSel.returnMultiplePaths = nullptr;
		fileSel.numReturnPaths = 0;
		fileSel.reserved = 0;

		std::string returnPath;
		if(fileSel.command != kVstDirectorySelect)
		{
			// Plugin wants to load or save a file.
			std::string extensions, workingDir;
			for(int32 i = 0; i < fileSel.numFileTypes; i++)
			{
				const VstFileType &type = fileSel.fileTypes[i];
				extensions += type.name;
				extensions += "|";
#if MPT_OS_WINDOWS
				extensions += "*.";
				extensions += type.dosType;
#elif MPT_OS_MACOSX_OR_IOS
				extensions += "*";
				extensions += type.macType;
#elif MPT_OS_GENERIC_UNIX
				extensions += "*.";
				extensions += type.unixType;
#else
#error Platform-specific code missing
#endif
				extensions += "|";
			}
			extensions += "|";

			if(fileSel.initialPath != nullptr)
			{
				workingDir = fileSel.initialPath;
			} else
			{
				// Plugins are probably looking for presets...?
				//workingDir = TrackerSettings::Instance().PathPluginPresets.GetWorkingDir();
			}

			FileDialog dlg = OpenFileDialog();
			if(fileSel.command == kVstFileSave)
			{
				dlg = SaveFileDialog();
			} else if(fileSel.command == kVstMultipleFilesLoad)
			{
				dlg = OpenFileDialog().AllowMultiSelect();
			}
			dlg.ExtensionFilter(extensions)
				.WorkingDirectory(mpt::PathString::FromLocale(workingDir))
				.AddPlace(GetPluginFactory().dllPath.GetPath());
			if(!dlg.Show(GetEditor()))
				return 0;

			if(fileSel.command == kVstMultipleFilesLoad)
			{
				// Multiple paths
				const auto &files = dlg.GetFilenames();
				fileSel.numReturnPaths = mpt::saturate_cast<int32>(files.size());
				fileSel.returnMultiplePaths = new (std::nothrow) char *[fileSel.numReturnPaths];
				if(!fileSel.returnMultiplePaths)
					return 0;
				for(int32 i = 0; i < fileSel.numReturnPaths; i++)
				{
					const std::string fname_ = files[i].ToLocale();
					char *fname = new (std::nothrow) char[fname_.length() + 1];
					if(fname)
						strcpy(fname, fname_.c_str());
					fileSel.returnMultiplePaths[i] = fname;
				}
				return 1;
			} else
			{
				// Single path

				// VOPM doesn't initialize required information properly (it doesn't memset the struct to 0)...
				if(FourCC("VOPM") == GetUID())
				{
					fileSel.sizeReturnPath = _MAX_PATH;
				}

				returnPath = dlg.GetFirstFile().ToLocale();
			}
		} else
		{
			// Plugin wants a directory
			BrowseForFolder dlg(mpt::PathString::FromLocale(fileSel.initialPath != nullptr ? fileSel.initialPath : ""), mpt::ToCString(mpt::Charset::Locale, fileSel.title != nullptr ? fileSel.title : ""));
			if(!dlg.Show(GetEditor()))
				return 0;

			returnPath = dlg.GetDirectory().ToLocale();
			if(FourCC("VSTr") == GetUID() && fileSel.returnPath != nullptr && fileSel.sizeReturnPath == 0)
			{
				// Old versions of reViSiT (which still relied on the host's file selector) seem to be dodgy.
				// They report a path size of 0, but when using an own buffer, they will crash.
				// So we'll just assume that reViSiT can handle long enough (_MAX_PATH) paths here.
				fileSel.sizeReturnPath = mpt::saturate_cast<int32>(returnPath.length() + 1);
			}
		}

		// Return single path (file or directory)
		if(fileSel.returnPath == nullptr || fileSel.sizeReturnPath == 0)
		{
			// Provide some memory for the return path.
			fileSel.sizeReturnPath = mpt::saturate_cast<int32>(returnPath.length() + 1);
			fileSel.returnPath = new(std::nothrow) char[fileSel.sizeReturnPath];
			if(fileSel.returnPath == nullptr)
			{
				return 0;
			}
			fileSel.reserved = 1;
		} else
		{
			fileSel.reserved = 0;
		}
		const auto len = std::min(returnPath.size(), static_cast<size_t>(fileSel.sizeReturnPath - 1));
		strncpy(fileSel.returnPath, returnPath.data(), len);
		fileSel.returnPath[len] = '\0';
		fileSel.numReturnPaths = 1;
		fileSel.returnMultiplePaths = nullptr;
		return 1;
	} else
	{
		// Close file selector - delete allocated strings.
		if(fileSel.command == kVstMultipleFilesLoad && fileSel.returnMultiplePaths != nullptr)
		{
			for(int32 i = 0; i < fileSel.numReturnPaths; i++)
			{
				if(fileSel.returnMultiplePaths[i] != nullptr)
				{
					delete[] fileSel.returnMultiplePaths[i];
				}
			}
			delete[] fileSel.returnMultiplePaths;
			fileSel.returnMultiplePaths = nullptr;
		} else
		{
			if(fileSel.reserved == 1 && fileSel.returnPath != nullptr)
			{
				delete[] fileSel.returnPath;
				fileSel.returnPath = nullptr;
			}
		}
		return 1;
	}
}


//////////////////////////////////////////////////////////////////////////////
//
// CVstPlugin
//

CVstPlugin::CVstPlugin(bool maskCrashes, HMODULE hLibrary, VSTPluginLib &factory, SNDMIXPLUGIN &mixStruct, AEffect &effect, CSoundFile &sndFile)
	: IMidiPlugin(factory, sndFile, &mixStruct)
	, m_maskCrashes(maskCrashes)
	, m_Effect(effect)
	, timeInfo{}
	, isBridged(!memcmp(&effect.reservedForHost2, "OMPT", 4))
	, m_hLibrary(hLibrary)
	, m_nSampleRate(sndFile.GetSampleRate())
	, m_isInitialized(false)
	, m_needIdle(false)
{
	// Open plugin and initialize data structures
	Initialize();
	InsertIntoFactoryList();

	m_isInitialized = true;
}


void CVstPlugin::Initialize()
{

	m_Ectx = { MPT_UFORMAT("VST Plugin: {}")(m_Factory.dllPath.ToUnicode()) };

	// If filename matched during load but plugin ID didn't, make sure it's updated.
	m_pMixStruct->Info.dwPluginId1 = m_Factory.pluginId1 = m_Effect.magic;
	m_pMixStruct->Info.dwPluginId2 = m_Factory.pluginId2 = m_Effect.uniqueID;

	// Store a pointer so we can get the CVstPlugin object from the basic VST effect object.
	m_Effect.reservedForHost1 = this;
	m_nSampleRate = m_SndFile.GetSampleRate();

	// First try to let the plugin know the render parameters.
	Dispatch(effSetSampleRate, 0, 0, nullptr, static_cast<float>(m_nSampleRate));
	Dispatch(effSetBlockSize, 0, MIXBUFFERSIZE, nullptr, 0.0f);

	Dispatch(effOpen, 0, 0, nullptr, 0.0f);

	// VST 2.0 plugins return 2 here, VST 2.4 plugins return 2400... Great!
	m_isVst2 = Dispatch(effGetVstVersion, 0,0, nullptr, 0.0f) >= 2;
	if(m_isVst2)
	{
		// Set VST speaker in/out setup to Stereo. Required for some plugins (e.g. Voxengo SPAN 2)
		// All this might get more interesting when adding sidechaining support...
		VstSpeakerArrangement sa{};
		sa.numChannels = 2;
		sa.type = kSpeakerArrStereo;
		for(std::size_t i = 0; i < std::size(sa.speakers); i++)
		{
			// For now, only left and right speaker are used.
			switch(i)
			{
			case 0:
				sa.speakers[i].type = kSpeakerL;
				mpt::String::WriteAutoBuf(sa.speakers[i].name) = "Left";
				break;
			case 1:
				sa.speakers[i].type = kSpeakerR;
				mpt::String::WriteAutoBuf(sa.speakers[i].name) = "Right";
				break;
			default:
				sa.speakers[i].type = kSpeakerUndefined;
				break;
			}
		}

		// For some reason, this call crashes in a call to free() in AdmiralQuality NaiveLPF / SCAMP 1.2 (newer versions are fine).
		// This does not happen when running the plugin in pretty much any host, or when running in OpenMPT 1.22 and older
		// (EXCEPT when recompiling those old versions with VS2010), so it sounds like an ASLR issue to me.
		// AdmiralQuality also doesn't know what to do.
		if(GetUID() != FourCC("CSI4"))
		{
			// For now, input setup = output setup.
			Dispatch(effSetSpeakerArrangement, 0, ToIntPtr(&sa), &sa, 0.0f);
		}

		// Dummy pin properties collection.
		// We don't use them but some plugs might do inits in here.
		VstPinProperties tempPinProperties;
		Dispatch(effGetInputProperties, 0, 0, &tempPinProperties, 0);
		Dispatch(effGetOutputProperties, 0, 0, &tempPinProperties, 0);

		Dispatch(effConnectInput, 0, 1, nullptr, 0.0f);
		if (m_Effect.numInputs > 1) Dispatch(effConnectInput, 1, 1, nullptr, 0.0f);
		Dispatch(effConnectOutput, 0, 1, nullptr, 0.0f);
		if (m_Effect.numOutputs > 1) Dispatch(effConnectOutput, 1, 1, nullptr, 0.0f);
		// Disable all inputs and outputs beyond stereo left and right:
		for(int32 i = 2; i < m_Effect.numInputs; i++)
			Dispatch(effConnectInput, i, 0, nullptr, 0.0f);
		for(int32 i = 2; i < m_Effect.numOutputs; i++)
			Dispatch(effConnectOutput, i, 0, nullptr, 0.0f);
	}

	// Second try to let the plugin know the render parameters.
	Dispatch(effSetSampleRate, 0, 0, nullptr, static_cast<float>(m_nSampleRate));
	Dispatch(effSetBlockSize, 0, MIXBUFFERSIZE, nullptr, 0.0f);
	if(m_Effect.numPrograms > 0)
	{
		BeginSetProgram(0);
		EndSetProgram();
	}

	InitializeIOBuffers();

	Dispatch(effSetProcessPrecision, 0, kVstProcessPrecision32, nullptr, 0.0f);

	m_isInstrument = IsInstrument();
	RecalculateGain();
	m_pProcessFP = (m_Effect.flags & effFlagsCanReplacing) ? m_Effect.processReplacing : m_Effect.process;

	// Issue samplerate again here, cos some plugs like it before the block size, other like it right at the end.
	Dispatch(effSetSampleRate, 0, 0, nullptr, static_cast<float>(m_nSampleRate));

	// Korg Wavestation GUI won't work until plugin was resumed at least once.
	// On the other hand, some other plugins (notably Synthedit plugins like Superwave P8 2.3 or Rez 3.0) don't like this
	// and won't load their stored plugin data instantly, so only do this for the troublesome plugins...
	// Also apply this fix for Korg's M1 plugin, as this will fixes older versions of said plugin, newer versions don't require the fix.
	// EZDrummer / Superior Drummer won't load their samples until playback has started.
	if(GetUID() == FourCC("KLWV")      // Wavestation
		|| GetUID() == FourCC("KLM1")  // M1
		|| GetUID() == FourCC("dfhe")  // EZDrummer
		|| GetUID() == FourCC("dfh2")) // Superior Drummer
	{
		Resume();
		Suspend();
	}
}


bool CVstPlugin::InitializeIOBuffers()
{
	// Input pointer array size must be >= 2 for now - the input buffer assignment might write to non allocated mem. otherwise
	// In case of a bridged plugin, the AEffect struct has been updated before calling this opcode, so we don't have to worry about it being up-to-date.
	return m_mixBuffer.Initialize(std::max(m_Effect.numInputs, int32(2)), m_Effect.numOutputs);
}


CVstPlugin::~CVstPlugin()
{
	CriticalSection cs;

	CloseEditor();
	if (m_isVst2)
	{
		Dispatch(effConnectInput, 0, 0, nullptr, 0);
		if (m_Effect.numInputs > 1) Dispatch(effConnectInput, 1, 0, nullptr, 0);
		Dispatch(effConnectOutput, 0, 0, nullptr, 0);
		if (m_Effect.numOutputs > 1) Dispatch(effConnectOutput, 1, 0, nullptr, 0);
	}
	CVstPlugin::Suspend();
	m_isInitialized = false;

	Dispatch(effClose, 0, 0, nullptr, 0);
	if(TrackerSettings::Instance().BrokenPluginsWorkaroundVSTNeverUnloadAnyPlugin)
	{
		// Buggy SynthEdit 1.4 plugins: Showing a SynthEdit 1.4 plugin's editor, fully unloading the plugin,
		// then loading another (unrelated) SynthEdit 1.4 plugin and showing its editor causes a crash.
	} else
	{
		if(m_hLibrary)
		{
			FreeLibrary(m_hLibrary);
		}
	}

}


void CVstPlugin::Release()
{
	delete this;
}


void CVstPlugin::Idle()
{
	if(m_needIdle)
	{
		if(!(Dispatch(effIdle, 0, 0, nullptr, 0.0f)))
			m_needIdle = false;
	}
	if (m_pEditor && m_pEditor->m_hWnd)
	{
		Dispatch(effEditIdle, 0, 0, nullptr, 0.0f);
	}
}


int32 CVstPlugin::GetNumPrograms() const
{
	return std::max(m_Effect.numPrograms, int32(0));
}


PlugParamIndex CVstPlugin::GetNumParameters() const
{
	return std::max(m_Effect.numParams, int32(0));
}


// Check whether a VST parameter can be automated
bool CVstPlugin::CanAutomateParameter(PlugParamIndex index)
{
	return (Dispatch(effCanBeAutomated, index, 0, nullptr, 0.0f) != 0);
}


int32 CVstPlugin::GetUID() const
{
	return m_Effect.uniqueID;
}


int32 CVstPlugin::GetVersion() const
{
	return m_Effect.version;
}


// Wrapper for VST dispatch call with structured exception handling.
intptr_t CVstPlugin::DispatchSEH(bool maskCrashes, AEffect *effect, VstOpcodeToPlugin opCode, int32 index, intptr_t value, void *ptr, float opt, unsigned long &exception)
{
	if(effect->dispatcher != nullptr)
	{
		intptr_t result = 0;
		DWORD e = SETryOrError(maskCrashes, [&](){ result = effect->dispatcher(effect, opCode, index, value, ptr, opt); });
		if(e)
		{
			exception = e;
		}
		return result;
	}
	return 0;
}


intptr_t CVstPlugin::Dispatch(VstOpcodeToPlugin opCode, int32 index, intptr_t value, void *ptr, float opt)
{
#ifdef VST_LOG
	{
		mpt::ustring codeStr;
		if(opCode < std::size(VstOpCodes))
		{
			codeStr = mpt::ToUnicode(mpt::Charset::ASCII, VstOpCodes[opCode]);
		} else
		{
			codeStr = mpt::ufmt::val(opCode);
		}
		MPT_LOG(LogDebug, "VST", MPT_UFORMAT("About to Dispatch({}) (Plugin=\"{}\"), index: {}, value: {}, ptr: {}, opt: {}!\n")(codeStr, m_Factory.libraryName, index, mpt::ufmt::PTR(value), mpt::ufmt::PTR(ptr), mpt::ufmt::flt(opt, 3)));
	}
#endif
	if(!m_Effect.dispatcher)
	{
		return 0;
	}
	intptr_t result = 0;
	{
		DWORD exception = SETryOrError([&](){ result = m_Effect.dispatcher(&m_Effect, opCode, index, value, ptr, opt); });
		if(exception)
		{
			mpt::ustring codeStr;
			if(opCode < mpt::saturate_cast<int32>(std::size(VstOpCodes)))
			{
				codeStr = mpt::ToUnicode(mpt::Charset::ASCII, VstOpCodes[opCode]);
			} else
			{
				codeStr = mpt::ufmt::val(opCode);
			}
			ReportPlugException(MPT_UFORMAT("Exception {} in Dispatch({})")(mpt::ufmt::HEX0<8>(exception), codeStr));
		}
	}
	return result;
}


int32 CVstPlugin::GetCurrentProgram()
{
	if(m_Effect.numPrograms > 0)
	{
		return static_cast<int32>(Dispatch(effGetProgram, 0, 0, nullptr, 0));
	}
	return 0;
}


CString CVstPlugin::GetCurrentProgramName()
{
	std::vector<char> s(256, 0);
	// kVstMaxProgNameLen is 24... too short for some plugins, so use at least 256 bytes.
	Dispatch(effGetProgramName, 0, 0, s.data(), 0);
	return mpt::ToCString(mpt::Charset::Locale, s.data());
}


void CVstPlugin::SetCurrentProgramName(const CString &name)
{
	Dispatch(effSetProgramName, 0, 0, const_cast<char *>(mpt::ToCharset(mpt::Charset::Locale, name.Left(kVstMaxProgNameLen)).c_str()), 0.0f);
}


CString CVstPlugin::GetProgramName(int32 program)
{
	// kVstMaxProgNameLen is 24... too short for some plugins, so use at least 256 bytes.
	std::vector<char> rawname(256, 0);
	if(program < m_Effect.numPrograms)
	{
		if(Dispatch(effGetProgramNameIndexed, program, -1 /*category*/, rawname.data(), 0) != 1)
		{
			// Fallback: Try to get current program name.
			rawname.assign(256, 0);
			int32 curProg = GetCurrentProgram();
			if(program != curProg)
			{
				SetCurrentProgram(program);
			}
			Dispatch(effGetProgramName, 0, 0, rawname.data(), 0);
			if(program != curProg)
			{
				SetCurrentProgram(curProg);
			}
		}
	}
	return mpt::ToCString(mpt::Charset::Locale, rawname.data());
}


void CVstPlugin::SetCurrentProgram(int32 nIndex)
{
	if(m_Effect.numPrograms > 0)
	{
		if(nIndex < m_Effect.numPrograms)
		{
			BeginSetProgram(nIndex);
			EndSetProgram();
		}
	}
}


void CVstPlugin::BeginSetProgram(int32 program)
{
	Dispatch(effBeginSetProgram, 0, 0, nullptr, 0);
	if(program != -1)
		Dispatch(effSetProgram, 0, program, nullptr, 0);
}


void CVstPlugin::EndSetProgram()
{
	Dispatch(effEndSetProgram, 0, 0, nullptr, 0);
}


void CVstPlugin::BeginGetProgram(int32 program)
{
	if(program != -1)
		Dispatch(effSetProgram, 0, program, nullptr, 0);
	if(isBridged)
		Dispatch(effVendorSpecific, kVendorOpenMPT, kBeginGetProgram, nullptr, 0);
}


void CVstPlugin::EndGetProgram()
{
	if(isBridged)
		Dispatch(effVendorSpecific, kVendorOpenMPT, kEndGetProgram, nullptr, 0);
}


PlugParamValue CVstPlugin::GetParameter(PlugParamIndex nIndex)
{
	float fResult = 0;
	if(nIndex < m_Effect.numParams && m_Effect.getParameter != nullptr)
	{
		DWORD exception = SETryOrError([&](){ fResult = m_Effect.getParameter(&m_Effect, nIndex); });
		if(exception)
		{
			//ReportPlugException(U_("Exception in getParameter (Plugin=\"{}\")!\n"), m_Factory.szLibraryName);
		}
	}
	return fResult;
}


void CVstPlugin::SetParameter(PlugParamIndex nIndex, PlugParamValue fValue)
{
	DWORD exception = 0;
	if(nIndex < m_Effect.numParams && m_Effect.setParameter)
	{
		exception = SETryOrError([&](){ m_Effect.setParameter(&m_Effect, nIndex, fValue); });
	}
	ResetSilence();
	if(exception)
	{
		//ReportPlugException(mpt::format(U_("Exception in SetParameter({}, {})!"))(nIndex, fValue));
	}
}


// Helper function for retreiving parameter name / label / display
CString CVstPlugin::GetParamPropertyString(int32 param, Vst::VstOpcodeToPlugin opcode)
{
	if(m_Effect.numParams > 0 && param < m_Effect.numParams)
	{
		// Increased to 256 bytes since SynthMaster 2.8 writes more than 64 bytes of 0-padding. Kind of ridiculous if you consider that kVstMaxParamStrLen = 8...
		std::vector<char> s(256, 0);
		Dispatch(opcode, param, 0, s.data(), 0);
		return mpt::ToCString(mpt::Charset::Locale, s.data());
	}
	return CString();
}


CString CVstPlugin::GetParamName(PlugParamIndex param)
{
	VstParameterProperties properties{};
	if(param < m_Effect.numParams && Dispatch(effGetParameterProperties, param, 0, &properties, 0.0f) == 1)
	{
		mpt::String::SetNullTerminator(properties.label);
		return mpt::ToCString(mpt::Charset::Locale, properties.label);
	} else
	{
		return GetParamPropertyString(param, effGetParamName);
	}
}


CString CVstPlugin::GetDefaultEffectName()
{
	if(m_isVst2)
	{
		std::vector<char> s(256, 0);
		Dispatch(effGetEffectName, 0, 0, s.data(), 0);
		return mpt::ToCString(mpt::Charset::Locale, s.data());
	}
	return CString();
}


void CVstPlugin::Resume()
{
	const uint32 sampleRate = m_SndFile.GetSampleRate();

	//reset some stuff
	m_MixState.nVolDecayL = 0;
	m_MixState.nVolDecayR = 0;
	if(m_isResumed)
	{
		Dispatch(effStopProcess, 0, 0, nullptr, 0.0f);
		Dispatch(effMainsChanged, 0, 0, nullptr, 0.0f);	// calls plugin's suspend
	}
	if (sampleRate != m_nSampleRate)
	{
		m_nSampleRate = sampleRate;
		Dispatch(effSetSampleRate, 0, 0, nullptr, static_cast<float>(m_nSampleRate));
	}
	Dispatch(effSetBlockSize, 0, MIXBUFFERSIZE, nullptr, 0.0f);
	//start off some stuff
	Dispatch(effMainsChanged, 0, 1, nullptr, 0.0f);	// calls plugin's resume
	Dispatch(effStartProcess, 0, 0, nullptr, 0.0f);
	m_isResumed = true;
}


void CVstPlugin::Suspend()
{
	if(m_isResumed)
	{
		Dispatch(effStopProcess, 0, 0, nullptr, 0.0f);
		Dispatch(effMainsChanged, 0, 0, nullptr, 0.0f); // calls plugin's suspend (theoretically, plugins should clean their buffers here, but oh well, the number of plugins which don't do this is surprisingly high.)
		m_isResumed = false;
	}
}


// Send events to plugin. Returns true if there are events left to be processed.
void CVstPlugin::ProcessVSTEvents()
{
	// Process VST events
	if(m_Effect.dispatcher != nullptr && vstEvents.Finalise() > 0)
	{
		DWORD exception = SETryOrError([&](){ m_Effect.dispatcher(&m_Effect, effProcessEvents, 0, 0, &vstEvents, 0); });
		ResetSilence();
		if(exception)
		{
			ReportPlugException(MPT_UFORMAT("Exception {} in ProcessVSTEvents(numEvents:{})!")(
				mpt::ufmt::HEX0<8>(exception),
				vstEvents.size()));
		}
	}
}


// Receive events from plugin and send them to the next plugin in the chain.
void CVstPlugin::ReceiveVSTEvents(const VstEvents *events)
{
	if(m_pMixStruct == nullptr)
	{
		return;
	}

	ResetSilence();

	// I think we should only route events to plugins that are explicitely specified as output plugins of the current plugin.
	// This should probably use GetOutputPlugList here if we ever get to support multiple output plugins.
	PLUGINDEX receiver = m_pMixStruct->GetOutputPlugin();

	if(receiver != PLUGINDEX_INVALID)
	{
		IMixPlugin *plugin = m_SndFile.m_MixPlugins[receiver].pMixPlugin;
		CVstPlugin *vstPlugin = dynamic_cast<CVstPlugin *>(plugin);
		// Add all events to the plugin's queue.
		for(const auto &ev : *events)
		{
			if(vstPlugin != nullptr)
			{
				// Directly enqueue the message and preserve as much of the event data as possible (e.g. delta frames, which are currently not used by OpenMPT but might be by plugins)
				vstPlugin->vstEvents.Enqueue(ev);
			} else if(plugin != nullptr)
			{
				if(ev->type == kVstMidiType)
				{
					plugin->MidiSend(static_cast<const VstMidiEvent *>(ev)->midiData);
				} else if(ev->type == kVstSysExType)
				{
					auto event = static_cast<const VstMidiSysexEvent *>(ev);
					plugin->MidiSysexSend(mpt::as_span(mpt::byte_cast<const std::byte *>(event->sysexDump), event->dumpBytes));
				}
			}
		}
	}

#ifdef MODPLUG_TRACKER
	if(m_recordMIDIOut)
	{
		// Spam MIDI data to all views
		for(const auto &ev : *events)
		{
			if(ev->type == kVstMidiType)
			{
				VstMidiEvent *event = static_cast<VstMidiEvent *>(ev);
				::SendNotifyMessage(CMainFrame::GetMainFrame()->GetMidiRecordWnd(), WM_MOD_MIDIMSG, event->midiData, reinterpret_cast<LPARAM>(this));
			}
		}
	}
#endif // MODPLUG_TRACKER
}


void CVstPlugin::Process(float *pOutL, float *pOutR, uint32 numFrames)
{
	ProcessVSTEvents();

	// If the plugin is found & ok, continue
	if(m_pProcessFP != nullptr && m_mixBuffer.Ok())
	{
		int32 numInputs = m_Effect.numInputs, numOutputs = m_Effect.numOutputs;
		//RecalculateGain();

		// Merge stereo input before sending to the plugin if it can only handle one input.
		if (numInputs == 1)
		{
			float *plugInputL = m_mixBuffer.GetInputBuffer(0);
			float *plugInputR = m_mixBuffer.GetInputBuffer(1);
			for (uint32 i = 0; i < numFrames; i++)
			{
				plugInputL[i] = 0.5f * (plugInputL[i] + plugInputR[i]);
			}
		}

		float **outputBuffers = m_mixBuffer.GetOutputBufferArray();
		if(!isBridged)
		{
			m_mixBuffer.ClearOutputBuffers(numFrames);
		}

		// Do the VST processing magic
		MPT_ASSERT(numFrames <= MIXBUFFERSIZE);
		{
			DWORD exception = SETryOrError([&](){ m_pProcessFP(&m_Effect, m_mixBuffer.GetInputBufferArray(), outputBuffers, numFrames); });
			if(exception)
			{
				Bypass();
				mpt::ustring processMethod = (m_Effect.flags & effFlagsCanReplacing) ? U_("processReplacing") : U_("process");
				ReportPlugException(MPT_UFORMAT("The plugin threw an exception ({}) in {}. It has automatically been set to \"Bypass\".")(mpt::ufmt::HEX0<8>(exception), processMethod));
			}
		}

		// Mix outputs of multi-output VSTs:
		if(numOutputs > 2)
		{
			MPT_ASSERT(outputBuffers != nullptr);
			// first, mix extra outputs on a stereo basis
			int32 outs = numOutputs;
			// so if nOuts is not even, let process the last output later
			if((outs % 2u) == 1) outs--;

			// mix extra stereo outputs
			for(int32 iOut = 2; iOut < outs; iOut++)
			{
				for(uint32 i = 0; i < numFrames; i++)
				{
					outputBuffers[iOut % 2u][i] += outputBuffers[iOut][i]; // assumed stereo.
				}
			}

			// if m_Effect.numOutputs is odd, mix half the signal of last output to each channel
			if(outs != numOutputs)
			{
				// trick : if we are here, numOutputs = m_Effect.numOutputs - 1 !!!
				for(uint32 i = 0; i < numFrames; i++)
				{
					float v = 0.5f * outputBuffers[outs][i];
					outputBuffers[0][i] += v;
					outputBuffers[1][i] += v;
				}
			}
		}

		if(numOutputs != 0)
		{
			MPT_ASSERT(outputBuffers != nullptr);
			ProcessMixOps(pOutL, pOutR, outputBuffers[0], outputBuffers[numOutputs > 1 ? 1 : 0], numFrames);
		}

		// If the I/O format of the bridge changed in the meanwhile, update it now.
		if(isBridged && Dispatch(effVendorSpecific, kVendorOpenMPT, kCloseOldProcessingMemory, nullptr, 0.0f) != 0)
		{
			InitializeIOBuffers();
		}
	}

	vstEvents.Clear();
	m_positionChanged = false;
}


bool CVstPlugin::MidiSend(uint32 dwMidiCode)
{
	// Note-Offs go at the start of the queue (since OpenMPT 1.17). Needed for situations like this:
	// ... ..|C-5 01
	// C-5 01|=== ..
	// TODO: Should not be used with real-time notes! Letting the key go too quickly
	// (e.g. while output device is being initalized) will cause the note to be stuck!
	bool insertAtFront = (MIDIEvents::GetTypeFromEvent(dwMidiCode) == MIDIEvents::evNoteOff);

	VstMidiEvent event{};
	event.type = kVstMidiType;
	event.byteSize = sizeof(event);
	event.midiData = dwMidiCode;

	ResetSilence();
	return vstEvents.Enqueue(&event, insertAtFront);
}


bool CVstPlugin::MidiSysexSend(mpt::const_byte_span sysex)
{
	VstMidiSysexEvent event{};
	event.type = kVstSysExType;
	event.byteSize = sizeof(event);
	event.dumpBytes = mpt::saturate_cast<int32>(sysex.size());
	event.sysexDump = sysex.data();	// We will make our own copy in VstEventQueue::Enqueue

	ResetSilence();
	return vstEvents.Enqueue(&event);
}


void CVstPlugin::HardAllNotesOff()
{
	constexpr uint32 SCRATCH_BUFFER_SIZE = 64;
	float out[2][SCRATCH_BUFFER_SIZE]; // scratch buffers

	// The JUCE framework doesn't like processing while being suspended.
	const bool wasSuspended = !IsResumed();
	if(wasSuspended)
	{
		Resume();
	}

	const bool isWavestation = GetUID() == FourCC("KLWV");
	const bool isSawer = GetUID() == FourCC("SaWR");
	for(uint8 mc = 0; mc < m_MidiCh.size(); mc++)
	{
		PlugInstrChannel &channel = m_MidiCh[mc];
		channel.ResetProgram();

		MidiPitchBend(mc, EncodePitchBendParam(MIDIEvents::pitchBendCentre));		// centre pitch bend

		if(!isWavestation && !isSawer)
		{
			// Korg Wavestation doesn't seem to like this CC, it can introduce ghost notes or
			// prevent new notes from being played.
			// Image-Line Sawer does not like it either and resets some parameters so that the plugin is all
			// distorted afterwards.
			MidiSend(MIDIEvents::CC(MIDIEvents::MIDICC_AllControllersOff, mc, 0));		// reset all controllers
		}
		if(!isSawer)
		{
			// Image-Line Sawer takes ages to execute this CC.
			MidiSend(MIDIEvents::CC(MIDIEvents::MIDICC_AllNotesOff, mc, 0));			// all notes off
		}
		MidiSend(MIDIEvents::CC(MIDIEvents::MIDICC_AllSoundOff, mc, 0));			// all sounds off

		for(std::size_t i = 0; i < std::size(channel.noteOnMap); i++)	//all notes
		{
			for(auto &c : channel.noteOnMap[i])
			{
				while(c != 0)
				{
					MidiSend(MIDIEvents::NoteOff(mc, static_cast<uint8>(i), 0));
					c--;
				}
			}
		}
	}
	// Let plugin process events
	while(vstEvents.GetNumQueuedEvents() > 0)
	{
		Process(out[0], out[1], SCRATCH_BUFFER_SIZE);
	}

	if(wasSuspended)
	{
		Suspend();
	}
}


void CVstPlugin::SaveAllParameters()
{
	if(m_pMixStruct == nullptr)
	{
		return;
	}
	m_pMixStruct->defaultProgram = -1;

	if(ProgramsAreChunks())
	{
		void *p = nullptr;

		// Try to get whole bank
		intptr_t byteSize = Dispatch(effGetChunk, 0, 0, &p, 0);

		if (!p)
		{
			// Getting bank failed, try to get just preset
			byteSize = Dispatch(effGetChunk, 1, 0, &p, 0);
		} else
		{
			// We managed to get the bank, now we need to remember which program we're on.
			m_pMixStruct->defaultProgram = GetCurrentProgram();
		}
		if (p != nullptr)
		{
			LimitMax(byteSize, Util::MaxValueOfType(byteSize) - 4);
			try
			{
				m_pMixStruct->pluginData.resize(byteSize + 4);
				auto data = m_pMixStruct->pluginData.data();
				memcpy(data, "fEvN", 4);	// 'NvEf', return value of deprecated effIdentify call
				memcpy(data + 4, p, byteSize);
				return;
			} catch(mpt::out_of_memory e)
			{
				mpt::delete_out_of_memory(e);
			}
		}
	}
	// This plugin doesn't support chunks: save parameters
	IMixPlugin::SaveAllParameters();
}


void CVstPlugin::RestoreAllParameters(int32 program)
{
	if(m_pMixStruct != nullptr && m_pMixStruct->pluginData.size() >= 4)
	{
		auto data = m_pMixStruct->pluginData.data();
		if (!memcmp(data, "fEvN", 4))	// 'NvEf', return value of deprecated effIdentify call
		{
			if ((program>=0) && (program < m_Effect.numPrograms))
			{
				// Bank
				Dispatch(effSetChunk, 0, m_pMixStruct->pluginData.size() - 4, data + 4, 0);
				SetCurrentProgram(program);
			} else
			{
				// Program
				BeginSetProgram(-1);
				Dispatch(effSetChunk, 1, m_pMixStruct->pluginData.size() - 4, data + 4, 0);
				EndSetProgram();
			}
		} else
		{
			IMixPlugin::RestoreAllParameters(program);
		}
	}
}


CAbstractVstEditor *CVstPlugin::OpenEditor()
{
	try
	{
		if(HasEditor())
			return new COwnerVstEditor(*this);
		else
			return new CDefaultVstEditor(*this);
	} catch(mpt::out_of_memory e)
	{
		mpt::delete_out_of_memory(e);
		ReportPlugException(U_("Exception in OpenEditor()"));
		return nullptr;
	}
}


void CVstPlugin::Bypass(bool bypass)
{
	Dispatch(effSetBypass, bypass ? 1 : 0, 0, nullptr, 0.0f);
	IMixPlugin::Bypass(bypass);
}


void CVstPlugin::NotifySongPlaying(bool playing)
{
	m_isSongPlaying = playing;
}


bool CVstPlugin::IsInstrument() const
{
	return ((m_Effect.flags & effFlagsIsSynth) || (!m_Effect.numInputs));
}


bool CVstPlugin::CanRecieveMidiEvents()
{
	return Dispatch(effCanDo, 0, 0, const_cast<char *>(PluginCanDo::receiveVstMidiEvent), 0.0f) != 0;
}


void CVstPlugin::ReportPlugException(const mpt::ustring &text) const
{
	CVstPluginManager::ReportPlugException(MPT_UFORMAT("{} (Plugin: {})")(text, m_Factory.libraryName));
}


// Cache program names for plugin bridge
void CVstPlugin::CacheProgramNames(int32 firstProg, int32 lastProg)
{
	if(isBridged)
	{
		int32 offsets[2] = { firstProg, lastProg };
		Dispatch(effVendorSpecific, kVendorOpenMPT, kCacheProgramNames, offsets, 0.0f);
	}
}


// Cache parameter names for plugin bridge
void CVstPlugin::CacheParameterNames(int32 firstParam, int32 lastParam)
{
	if(isBridged)
	{
		int32 offsets[2] = { firstParam, lastParam };
		Dispatch(effVendorSpecific, kVendorOpenMPT, kCacheParameterInfo, offsets, 0.0f);
	}
}


IMixPlugin::ChunkData CVstPlugin::GetChunk(bool isBank)
{
	std::byte *chunk = nullptr;
	auto size = Dispatch(effGetChunk, isBank ? 0 : 1, 0, &chunk, 0);
	if(chunk == nullptr)
	{
		size = 0;
	}
	return ChunkData(chunk, size);
}


void CVstPlugin::SetChunk(const ChunkData &chunk, bool isBank)
{
	Dispatch(effSetChunk, isBank ? 0 : 1, chunk.size(), const_cast<std::byte *>(chunk.data()), 0);
}


OPENMPT_NAMESPACE_END

#endif // NO_VST
