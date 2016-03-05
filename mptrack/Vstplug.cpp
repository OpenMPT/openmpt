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
#include "VstEditor.h"
#include "DefaultVstEditor.h"
#endif // MODPLUG_TRACKER
#include "../soundlib/Sndfile.h"
#include "../soundlib/MIDIEvents.h"
#include "MIDIMappingDialog.h"
#include "../common/StringFixer.h"
#include "FileDialog.h"
#include "../pluginBridge/BridgeOpCodes.h"
#include "../soundlib/plugins/OpCodes.h"
#include "../soundlib/plugins/PluginManager.h"

OPENMPT_NAMESPACE_BEGIN


VstTimeInfo CVstPlugin::timeInfo = { 0 };

//#define VST_LOG

VstIntPtr VSTCALLBACK CVstPlugin::MasterCallBack(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float /*opt*/)
//-------------------------------------------------------------------------------------------------------------------------------------------
{
	#ifdef VST_LOG
	Log(mpt::format("VST plugin to host: Eff: %1, Opcode = %2, Index = %3, Value = %4, PTR = %5, OPT = %6\n")(
		mpt::fmt::Ptr(effect), mpt::ToString(opcode),
		mpt::ToString(index), mpt::fmt::HEX0<sizeof(VstIntPtr) * 2>(value), mpt::fmt::Ptr(ptr), mpt::fmt::flt(0.0f, 0, 3)/*opt*/));
	#endif

	enum enmHostCanDo
	{
		HostDoNotKnow	= 0,
		HostCanDo		= 1,
		HostCanNotDo	= -1
	};

	CVstPlugin *pVstPlugin = nullptr;
	if(effect != nullptr)
	{
		pVstPlugin = FromVstPtr<CVstPlugin>(effect->resvd1);
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

	// Returns the unique id of a plug that's currently loading
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
		MemsetZero(timeInfo);

		if(pVstPlugin)
		{
			timeInfo.sampleRate = pVstPlugin->m_nSampleRate;
			CSoundFile &sndFile = pVstPlugin->GetSoundFile();
			if(pVstPlugin->IsSongPlaying())
			{
				timeInfo.flags |= kVstTransportPlaying;
				if(pVstPlugin->GetSoundFile().m_SongFlags[SONG_PATTERNLOOP]) timeInfo.flags |= kVstTransportCycleActive;
				timeInfo.samplePos = sndFile.GetTotalSampleCount();
				if(sndFile.HasPositionChanged())
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
				timeInfo.nanoSeconds = timeGetTime() * 1000000;
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

				if((pVstPlugin->GetSoundFile().m_PlayState.m_nRow % pVstPlugin->GetSoundFile().m_PlayState.m_nCurrentRowsPerMeasure) == 0)
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
				timeInfo.timeSigDenominator = 4; //gcd(pSndFile->m_nCurrentRowsPerMeasure, pSndFile->m_nCurrentRowsPerBeat);
			}
		}
		return ToVstPtr(&timeInfo);

	// Receive MIDI events from plugin
	case audioMasterProcessEvents:
		if(pVstPlugin != nullptr && ptr != nullptr)
		{
			pVstPlugin->ReceiveVSTEvents(reinterpret_cast<VstEvents *>(ptr));
			return 1;
		}
		break;

	// DEPRECATED in VST 2.4
	case audioMasterSetTime:
		Log("VST plugin to host: Set Time\n");
		break;

	// returns tempo (in bpm * 10000) at sample frame location passed in <value> - DEPRECATED in VST 2.4
	case audioMasterTempoAt:
		//Screw it! Let's just return the tempo at this point in time (might be a bit wrong).
		if (pVstPlugin != nullptr)
		{
			return (VstInt32)(pVstPlugin->GetSoundFile().GetCurrentBPM() * 10000);
		}
		return (VstInt32)(125 * 10000);

	// parameters - DEPRECATED in VST 2.4
	case audioMasterGetNumAutomatableParameters:
		//Log("VST plugin to host: Get Num Automatable Parameters\n");
		if(pVstPlugin != nullptr)
		{
			return pVstPlugin->GetNumParameters();
		}
		break;

	// Apparently, this one is broken in VST SDK anyway. - DEPRECATED in VST 2.4
	case audioMasterGetParameterQuantization:
		Log("VST plugin to host: Audio Master Get Parameter Quantization\n");
		break;

	// numInputs and/or numOutputs has changed
	case audioMasterIOChanged:
		if(pVstPlugin != nullptr)
		{
			CriticalSection cs;
			return pVstPlugin->InitializeIOBuffers() ? 1 : 0;
		}
		break;

	// plug needs idle calls (outside its editor window) - DEPRECATED in VST 2.4
	case audioMasterNeedIdle:
		if(pVstPlugin != nullptr)
		{
			pVstPlugin->m_bNeedIdle = true;
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
		Log("VST plugin to host: Size Window\n");
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
		Log("VST plugin to host: Get Input Latency\n");
		break;

	case audioMasterGetOutputLatency:
		if(pVstPlugin)
		{
			if(pVstPlugin->GetSoundFile().IsRenderingToDisc())
			{
				return 0;
			} else
			{
				return Util::Round<VstIntPtr>(pVstPlugin->GetSoundFile().m_TimingInfo.OutputLatency * pVstPlugin->m_nSampleRate);
			}
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
					return ToVstPtr(&plugin->m_Effect);
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
					return ToVstPtr(&plugin->m_Effect);
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
		//Log("VST plugin to host: Get Automation State\n");
		return kVstAutomationReadWrite;

	case audioMasterOfflineStart:
		Log("VST plugin to host: Offlinestart\n");
		break;

	case audioMasterOfflineRead:
		Log("VST plugin to host: Offlineread\n");
		break;

	case audioMasterOfflineWrite:
		Log("VST plugin to host: Offlinewrite\n");
		break;

	case audioMasterOfflineGetCurrentPass:
		Log("VST plugin to host: OfflineGetcurrentpass\n");
		break;

	case audioMasterOfflineGetCurrentMetaPass:
		Log("VST plugin to host: OfflineGetCurrentMetapass\n");
		break;

	// for variable i/o, sample rate in <opt> - DEPRECATED in VST 2.4
	case audioMasterSetOutputSampleRate:
		Log("VST plugin to host: Set Output Sample Rate\n");
		break;

	// result in ret - DEPRECATED in VST 2.4
	case audioMasterGetOutputSpeakerArrangement:
		Log("VST plugin to host: Get Output Speaker Arrangement\n");
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
		Log("VST plugin to host: Set Icon\n");
		break;

	// string in ptr, see below
	case audioMasterCanDo:
		//Other possible Can Do strings are:
		//"receiveVstTimeInfo",
		//"asyncProcessing",
		//"offline",
		//"supportShell"
		//"shellCategory"
		//"editFile"
		//"startStopProcess"
		//"sendVstMidiEventFlagIsRealtime"

		if(!strcmp((char*)ptr,"sendVstEvents")
			|| !strcmp((char*)ptr,"sendVstMidiEvent")
			|| !strcmp((char*)ptr,"sendVstTimeInfo")
			|| !strcmp((char*)ptr,"receiveVstEvents")
			|| !strcmp((char*)ptr,"receiveVstMidiEvent")
			|| !strcmp((char*)ptr,"supplyIdle")
			|| !strcmp((char*)ptr,"sizeWindow")
			|| !strcmp((char*)ptr,"openFileSelector")
			|| !strcmp((char*)ptr,"closeFileSelector")
			|| !strcmp((char*)ptr,"acceptIOChanges")
			|| !strcmp((char*)ptr,"reportConnectionChanges"))
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
		Log("VST plugin to host: Open Window\n");
		break;

	// close window, platform specific handle in <ptr> - DEPRECATED in VST 2.4
	case audioMasterCloseWindow:
		Log("VST plugin to host: Close Window\n");
		break;

	// get plug directory, FSSpec on MAC, else char*
	case audioMasterGetDirectory:
		//Log("VST plugin to host: Get Directory\n");
		// Need to allocate space for path only, but I guess noone relies on this anyway.
		//return ToVstPtr(pVstPlugin->GetPluginFactory().szDllPath);
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
		Log("VST plugin to host: Begin Edit\n");
		break;

	// end of automation session (when mouse up),     parameter index in <index>
	case audioMasterEndEdit:
		Log("VST plugin to host: End Edit\n");
		break;

	// open a fileselector window with VstFileSelect* in <ptr>
	case audioMasterOpenFileSelector:

	//---from here VST 2.2 extension opcodes------------------------------------------------------

	// close a fileselector operation with VstFileSelect* in <ptr>: Must be always called after an open !
	case audioMasterCloseFileSelector:
		if(pVstPlugin != nullptr)
		{
			return pVstPlugin->VstFileSelector(opcode == audioMasterCloseFileSelector, static_cast<VstFileSelect *>(ptr));
		}

	// open an editor for audio (defined by XML text in ptr) - DEPRECATED in VST 2.4
	case audioMasterEditFile:
		Log("VST plugin to host: Edit File\n");
		break;

	// get the native path of currently loading bank or project
	// (called from writeChunk) void* in <ptr> (char[2048], or sizeof(FSSpec)) - DEPRECATED in VST 2.4
	// Note: The shortcircuit VSTi actually uses this feature.
	case audioMasterGetChunkFile:
#ifdef MODPLUG_TRACKER
		if(pVstPlugin && pVstPlugin->GetModDoc())
		{
			std::wstring formatStr = TrackerSettings::Instance().pluginProjectPath;
			if(formatStr.empty()) formatStr = L"%1";
			const mpt::PathString projectPath = pVstPlugin->GetModDoc()->GetPathNameMpt();
			const mpt::PathString projectFile = projectPath.GetFullFileName();
			mpt::PathString path = mpt::PathString::FromWide(mpt::String::Print(formatStr, projectPath.GetPath().ToWide(), projectFile.ToWide()));
			if(path.empty())
			{
				return 0;
			}
			path.EnsureTrailingSlash();
			::SHCreateDirectoryExW(NULL, path.AsNative().c_str(), nullptr);
			path += projectFile;
			strcpy(ptr, path.ToLocale().c_str());
			return 1;
		}
#endif
		Log("VST plugin to host: Get Chunk File\n");
		break;

	//---from here VST 2.3 extension opcodes------------------------------------------------------

	// result a VstSpeakerArrangement in ret - DEPRECATED in VST 2.4
	case audioMasterGetInputSpeakerArrangement:
		Log("VST plugin to host: Get Input Speaker Arrangement\n");
		break;

	}

	// Unknown codes:

	return 0;
}


// Helper function for file selection dialog stuff.
// Note: This function has been copied over to the Plugin Bridge. Ugly, but serializing this over the bridge would be even uglier.
VstIntPtr CVstPlugin::VstFileSelector(bool destructor, VstFileSelect *fileSel)
//----------------------------------------------------------------------------
{
	if(fileSel == nullptr)
	{
		return 0;
	}

	if(!destructor)
	{
		fileSel->nbReturnPath = 0;
		fileSel->reserved = 0;

		if(fileSel->command != kVstDirectorySelect)
		{
			// Plugin wants to load or save a file.
			std::string extensions, workingDir;
			for(VstInt32 i = 0; i < fileSel->nbFileTypes; i++)
			{
				VstFileType *pType = &(fileSel->fileTypes[i]);
				extensions += pType->name;
				extensions += "|";
#if (defined(WIN32) || (defined(WINDOWS) && WINDOWS == 1))
				extensions += "*.";
				extensions += pType->dosType;
#elif defined(MAC) && MAC == 1
				extensions += "*";
				extensions += pType->macType;
#elif defined(UNIX) && UNIX == 1
				extensions += "*.";
				extensions += pType->unixType;
#else
#error Platform-specific code missing
#endif
				extensions += "|";
			}
			extensions += "|";

			if(fileSel->initialPath != nullptr)
			{
				workingDir = fileSel->initialPath;
			} else
			{
				// Plugins are probably looking for presets...?
				//workingDir = TrackerSettings::Instance().PathPluginPresets.GetWorkingDir();
			}

			FileDialog dlg = OpenFileDialog();
			if(fileSel->command == kVstFileSave)
			{
				dlg = SaveFileDialog();
			} else if(fileSel->command == kVstMultipleFilesLoad)
			{
				dlg = OpenFileDialog().AllowMultiSelect();
			}
			dlg.ExtensionFilter(extensions)
				.WorkingDirectory(mpt::PathString::FromLocale(workingDir));
			if(!dlg.Show(GetEditor()))
			{
				return 0;
			}

			if(fileSel->command == kVstMultipleFilesLoad)
			{
				// Multiple paths
				const FileDialog::PathList &files = dlg.GetFilenames();
				fileSel->nbReturnPath = files.size();
				fileSel->returnMultiplePaths = new (std::nothrow) char *[fileSel->nbReturnPath];
				for(size_t i = 0; i < files.size(); i++)
				{
					const std::string fname_ = files[i].ToLocale();
					char *fname = new (std::nothrow) char[fname_.length() + 1];
					strcpy(fname, fname_.c_str());
					fileSel->returnMultiplePaths[i] = fname;
				}
			} else
			{
				// Single path

				// VOPM doesn't initialize required information properly (it doesn't memset the struct to 0)...
				if(CCONST('V', 'O', 'P', 'M') == GetUID())
				{
					fileSel->sizeReturnPath = _MAX_PATH;
				}

				if(fileSel->returnPath == nullptr || fileSel->sizeReturnPath == 0)
				{

					// Provide some memory for the return path.
					fileSel->sizeReturnPath = dlg.GetFirstFile().ToLocale().length() + 1;
					fileSel->returnPath = new (std::nothrow) char[fileSel->sizeReturnPath];
					if(fileSel->returnPath == nullptr)
					{
						return 0;
					}
					fileSel->returnPath[fileSel->sizeReturnPath - 1] = '\0';
					fileSel->reserved = 1;
				} else
				{
					fileSel->reserved = 0;
				}
				strncpy(fileSel->returnPath, dlg.GetFirstFile().ToLocale().c_str(), fileSel->sizeReturnPath - 1);
				fileSel->nbReturnPath = 1;
				fileSel->returnMultiplePaths = nullptr;
			}
			return 1;

		} else
		{
			// Plugin wants a directory
			BrowseForFolder dlg(mpt::PathString::FromLocale(fileSel->initialPath != nullptr ? fileSel->initialPath : ""), fileSel->title != nullptr ? fileSel->title : "");
			if(dlg.Show(GetEditor()))
			{
				const std::string dir = dlg.GetDirectory().ToLocale();
				if(CCONST('V', 'S', 'T', 'r') == GetUID() && fileSel->returnPath != nullptr && fileSel->sizeReturnPath == 0)
				{
					// old versions of reViSiT (which still relied on the host's file selection code) seem to be dodgy.
					// They report a path size of 0, but when using an own buffer, they will crash.
					// So we'll just assume that reViSiT can handle long enough (_MAX_PATH) paths here.
					fileSel->sizeReturnPath = dir.length() + 1;
					fileSel->returnPath[fileSel->sizeReturnPath - 1] = '\0';
				}
				if(fileSel->returnPath == nullptr || fileSel->sizeReturnPath == 0)
				{
					// Provide some memory for the return path.
					fileSel->sizeReturnPath = mpt::saturate_cast<VstInt32>(dir.length() + 1);
					fileSel->returnPath = new char[fileSel->sizeReturnPath];
					if(fileSel->returnPath == nullptr)
					{
						return 0;
					}
					fileSel->returnPath[fileSel->sizeReturnPath - 1] = '\0';
					fileSel->reserved = 1;
				} else
				{
					fileSel->reserved = 0;
				}
				strncpy(fileSel->returnPath, dir.c_str(), fileSel->sizeReturnPath - 1);
				fileSel->nbReturnPath = 1;
				return 1;
			} else
			{
				return 0;
			}
		}
	} else
	{
		// Close file selector - delete allocated strings.
		if(fileSel->command == kVstMultipleFilesLoad && fileSel->returnMultiplePaths != nullptr)
		{
			for(VstInt32 i = 0; i < fileSel->nbReturnPath; i++)
			{
				if(fileSel->returnMultiplePaths[i] != nullptr)
				{
					delete[] fileSel->returnMultiplePaths[i];
				}
			}
			delete[] fileSel->returnMultiplePaths;
			fileSel->returnMultiplePaths = nullptr;
		} else
		{
			if(fileSel->reserved == 1 && fileSel->returnPath != nullptr)
			{
				delete[] fileSel->returnPath;
				fileSel->returnPath = nullptr;
			}
		}
		return 1;
	}
}


//////////////////////////////////////////////////////////////////////////////
//
// CVstPlugin
//

CVstPlugin::CVstPlugin(HMODULE hLibrary, VSTPluginLib &factory, SNDMIXPLUGIN &mixStruct, AEffect &effect, CSoundFile &sndFile)
	: IMidiPlugin(factory, sndFile, &mixStruct)
	, m_Effect(effect)
	, isBridged(!memcmp(&effect.resvd2, "OMPT", 4))
	, m_hLibrary(hLibrary)
	, m_nSampleRate(sndFile.GetSampleRate())
	, m_isInitialized(false)
	, m_bNeedIdle(false)
//----------------------------------------------------------------------------------------------------------------------------
{
	m_pProcessFP = nullptr;

	// Open plugin and initialize data structures
	Initialize();
	// Now we should be ready to go
	m_pMixStruct->pMixPlugin = this;

	// Insert ourselves in the beginning of the list
	InsertIntoFactoryList();

	m_isInitialized = true;
}


void CVstPlugin::Initialize()
//---------------------------
{
	// Store a pointer so we can get the CVstPlugin object from the basic VST effect object.
	m_Effect.resvd1 = ToVstPtr(this);
	m_nSampleRate = m_SndFile.GetSampleRate();

	Dispatch(effOpen, 0, 0, nullptr, 0.0f);
	// VST 2.0 plugins return 2 here, VST 2.4 plugins return 2400... Great!
	m_bIsVst2 = Dispatch(effGetVstVersion, 0,0, nullptr, 0.0f) >= 2;
	if (m_bIsVst2)
	{
		// Set VST speaker in/out setup to Stereo. Required for some plugins (e.g. Voxengo SPAN 2)
		// All this might get more interesting when adding sidechaining support...
		VstSpeakerArrangement sa;
		MemsetZero(sa);
		sa.numChannels = 2;
		sa.type = kSpeakerArrStereo;
		for(int i = 0; i < CountOf(sa.speakers); i++)
		{
			sa.speakers[i].azimuth = 0.0f;
			sa.speakers[i].elevation = 0.0f;
			sa.speakers[i].radius = 0.0f;
			sa.speakers[i].reserved = 0.0f;
			// For now, only left and right speaker are used.
			switch(i)
			{
			case 0:
				sa.speakers[i].type = kSpeakerL;
				vst_strncpy(sa.speakers[i].name, "Left", kVstMaxNameLen - 1);
				break;
			case 1:
				sa.speakers[i].type = kSpeakerR;
				vst_strncpy(sa.speakers[i].name, "Right", kVstMaxNameLen - 1);
				break;
			default:
				sa.speakers[i].type = kSpeakerUndefined;
				break;
			}
		}
		// For now, input setup = output setup.
		Dispatch(effSetSpeakerArrangement, 0, ToVstPtr(&sa), &sa, 0.0f);

		// Dummy pin properties collection.
		// We don't use them but some plugs might do inits in here.
		VstPinProperties tempPinProperties;
		Dispatch(effGetInputProperties, 0, 0, &tempPinProperties, 0);
		Dispatch(effGetOutputProperties, 0, 0, &tempPinProperties, 0);

		Dispatch(effConnectInput, 0, 1, nullptr, 0.0f);
		if (m_Effect.numInputs > 1) Dispatch(effConnectInput, 1, 1, nullptr, 0.0f);
		Dispatch(effConnectOutput, 0, 1, nullptr, 0.0f);
		if (m_Effect.numOutputs > 1) Dispatch(effConnectOutput, 1, 1, nullptr, 0.0f);
		//rewbs.VSTCompliance: disable all inputs and outputs beyond stereo left and right:
		for (int i=2; i<m_Effect.numInputs; i++)
			Dispatch(effConnectInput, i, 0, nullptr, 0.0f);
		for (int i=2; i<m_Effect.numOutputs; i++)
			Dispatch(effConnectOutput, i, 0, nullptr, 0.0f);
		//end rewbs.VSTCompliance

	}

	Dispatch(effSetSampleRate, 0, 0, nullptr, static_cast<float>(m_nSampleRate));
	Dispatch(effSetBlockSize, 0, MIXBUFFERSIZE, nullptr, 0.0f);
	if(m_Effect.numPrograms > 0)
	{
		BeginSetProgram(0);
		EndSetProgram();
	}

	InitializeIOBuffers();

	Dispatch(effSetProcessPrecision, 0, kVstProcessPrecision32, nullptr, 0.0f);

#ifdef VST_LOG
	Log("%s: vst ver %d.0, flags=%04X, %d programs, %d parameters\n",
		m_Factory.libraryName, (m_bIsVst2) ? 2 : 1, m_Effect.flags,
		m_Effect.numPrograms, m_Effect.numParams);
#endif

	m_bIsInstrument = IsInstrument();
	RecalculateGain();
	m_pProcessFP = (m_Effect.flags & effFlagsCanReplacing) ? m_Effect.processReplacing : m_Effect.process;

	// issue samplerate again here, cos some plugs like it before the block size, other like it right at the end.
	Dispatch(effSetSampleRate, 0, 0, nullptr, static_cast<float>(m_nSampleRate));

	// Korg Wavestation GUI won't work until plugin was resumed at least once.
	// On the other hand, some other plugins (notably Synthedit plugins like Superwave P8 2.3 or Rez 3.0) don't like this
	// and won't load their stored plugin data instantly, so only do this for the troublesome plugins...
	// Also apply this fix for Korg's M1 plugin, as this will fixes older versions of said plugin, newer versions don't require the fix.
	// EZDrummer / Superior Drummer won't load their samples until playback has started.
	if(GetUID() == CCONST('K', 'L', 'W', 'V')			// Wavestation
		|| GetUID() == CCONST('K', 'L', 'M', '1')		// M1
		|| GetUID() == CCONST('d', 'f', 'h', 'e')		// EZDrummer
		|| GetUID() == CCONST('d', 'f', 'h', '2'))		// Superior Drummer
	{
		Resume();
		Suspend();
	}
}


bool CVstPlugin::InitializeIOBuffers()
//------------------------------------
{
	// Input pointer array size must be >= 2 for now - the input buffer assignment might write to non allocated mem. otherwise
	// In case of a bridged plugin, the AEffect struct has been updated before calling this opcode, so we don't have to worry about it being up-to-date.
	bool result = m_mixBuffer.Initialize(std::max<size_t>(m_Effect.numInputs, 2), m_Effect.numOutputs);
	m_MixState.pOutBufferL = m_mixBuffer.GetInputBuffer(0);
	m_MixState.pOutBufferR = m_mixBuffer.GetInputBuffer(1);

	return result;
}


CVstPlugin::~CVstPlugin()
//-----------------------
{
	CriticalSection cs;

	if (m_pEditor)
	{
		if (m_pEditor->m_hWnd) m_pEditor->OnClose();
		if ((volatile void *)m_pEditor) delete m_pEditor;
		m_pEditor = nullptr;
	}
	if (m_bIsVst2)
	{
		Dispatch(effConnectInput, 0, 0, nullptr, 0);
		if (m_Effect.numInputs > 1) Dispatch(effConnectInput, 1, 0, nullptr, 0);
		Dispatch(effConnectOutput, 0, 0, nullptr, 0);
		if (m_Effect.numOutputs > 1) Dispatch(effConnectOutput, 1, 0, nullptr, 0);
	}
	Suspend();
	m_isInitialized = false;

	// First thing to do, if we don't want to hang in a loop
	if (m_Factory.pPluginsList == this) m_Factory.pPluginsList = m_pNext;
	if (m_pMixStruct)
	{
		m_pMixStruct->pMixPlugin = nullptr;
		m_pMixStruct = nullptr;
	}

	Dispatch(effClose, 0, 0, nullptr, 0);
	if(m_hLibrary)
	{
		FreeLibrary(m_hLibrary);
		m_hLibrary = nullptr;
	}

}


void CVstPlugin::Release()
//------------------------
{
	try
	{
		delete this;
	} catch (...)
	{
		ReportPlugException(L"Exception while destroying plugin!");
	}
}


void CVstPlugin::Idle()
//---------------------
{
	if(m_bNeedIdle)
	{
		if(!(Dispatch(effIdle, 0, 0, nullptr, 0.0f)))
			m_bNeedIdle = false;
	}
	if (m_pEditor && m_pEditor->m_hWnd)
	{
		Dispatch(effEditIdle, 0, 0, nullptr, 0.0f);
	}
}


int32 CVstPlugin::GetNumPrograms() const
//--------------------------------------
{
	return std::max(m_Effect.numPrograms, VstInt32(0));
}


PlugParamIndex CVstPlugin::GetNumParameters() const
//-------------------------------------------------
{
	return std::max(m_Effect.numParams, VstInt32(0));
}


// Check whether a VST parameter can be automated
bool CVstPlugin::CanAutomateParameter(PlugParamIndex index)
//---------------------------------------------------------
{
	return (Dispatch(effCanBeAutomated, index, 0, nullptr, 0.0f) != 0);
}


VstInt32 CVstPlugin::GetUID() const
//---------------------------------
{
	return m_Effect.uniqueID;
}


VstInt32 CVstPlugin::GetVersion() const
//-------------------------------------
{
	return m_Effect.version;
}


VstIntPtr CVstPlugin::Dispatch(VstInt32 opCode, VstInt32 index, VstIntPtr value, void *ptr, float opt)
//----------------------------------------------------------------------------------------------------
{
	VstIntPtr result = 0;

	try
	{
		if(m_Effect.dispatcher != nullptr)
		{
			#ifdef VST_LOG
			Log("About to Dispatch(%d) (Plugin=\"%s\"), index: %d, value: %d, value: %h, value: %f!\n", opCode, m_Factory.libraryName, index, value, ptr, opt);
			#endif
			result = m_Effect.dispatcher(&m_Effect, opCode, index, value, ptr, opt);
		}
	} catch (...)
	{
		std::wstring codeStr;
		if(opCode < CountOf(VstOpCodes))
		{
			codeStr = mpt::ToWide(mpt::CharsetASCII, VstOpCodes[opCode]);
		} else
		{
			codeStr = mpt::ToWString(opCode);
		}
		ReportPlugException(mpt::String::Print(L"Exception in Dispatch(%1)!", codeStr));
	}

	return result;
}


int32 CVstPlugin::GetCurrentProgram()
//-----------------------------------
{
	if(m_Effect.numPrograms > 0)
	{
		return Dispatch(effGetProgram, 0, 0, nullptr, 0);
	}
	return 0;
}


CString CVstPlugin::GetCurrentProgramName()
//-----------------------------------------
{
	char rawname[MAX(kVstMaxProgNameLen + 1, 256)] = "";	// kVstMaxProgNameLen is 24...
	Dispatch(effGetProgramName, 0, 0, rawname, 0);
	mpt::String::SetNullTerminator(rawname);
	return mpt::ToCString(mpt::CharsetLocale, rawname);
}


void CVstPlugin::SetCurrentProgramName(const CString &name)
//---------------------------------------------------------
{
	Dispatch(effSetProgramName, 0, 0, (void *)mpt::ToCharset(mpt::CharsetLocale, name).c_str(), 0.0f);
}


CString CVstPlugin::GetProgramName(int32 program)
//---------------------------------------------
{
	char rawname[MAX(kVstMaxProgNameLen + 1, 256)] = "";	// kVstMaxProgNameLen is 24...
	if(m_Effect.numPrograms > 0)
	{
		if(Dispatch(effGetProgramNameIndexed, program, -1 /*category*/, rawname, 0) != 1)
		{
			// Fallback: Try to get current program name.
			strcpy(rawname, "");
			VstInt32 curProg = GetCurrentProgram();
			if(program != curProg)
			{
				SetCurrentProgram(program);
			}
			Dispatch(effGetProgramName, 0, 0, rawname, 0);
			if(program != curProg)
			{
				SetCurrentProgram(curProg);
			}
		}
	}
	mpt::String::SetNullTerminator(rawname);
	return mpt::ToCString(mpt::CharsetLocale, rawname);
}


void CVstPlugin::SetCurrentProgram(int32 nIndex)
//----------------------------------------------
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
//---------------------------------------------
{
	Dispatch(effBeginSetProgram, 0, 0, nullptr, 0);
	if(program != -1)
		Dispatch(effSetProgram, 0, program, nullptr, 0);
}


void CVstPlugin::EndSetProgram()
//------------------------------
{
	Dispatch(effEndSetProgram, 0, 0, nullptr, 0);
}


PlugParamValue CVstPlugin::GetParameter(PlugParamIndex nIndex)
//------------------------------------------------------------
{
	float fResult = 0;
	if(nIndex < m_Effect.numParams && m_Effect.getParameter != nullptr)
	{
		try
		{
			fResult = m_Effect.getParameter(&m_Effect, nIndex);
		} catch (...)
		{
			//ReportPlugException("Exception in getParameter (Plugin=\"%s\")!\n", m_Factory.szLibraryName);
		}
	}
	Limit(fResult, 0.0f, 1.0f);
	return fResult;
}


void CVstPlugin::SetParameter(PlugParamIndex nIndex, PlugParamValue fValue)
//-------------------------------------------------------------------------
{
	try
	{
		if(nIndex < m_Effect.numParams && m_Effect.setParameter)
		{
			if ((fValue >= 0.0f) && (fValue <= 1.0f))
				m_Effect.setParameter(&m_Effect, nIndex, fValue);
		}
		ResetSilence();
	} catch (...)
	{
		ReportPlugException(mpt::String::Print(L"Exception in SetParameter(%1, %2)!", nIndex, fValue));
	}
}


// Helper function for retreiving parameter name / label / display
CString CVstPlugin::GetParamPropertyString(VstInt32 param, VstInt32 opcode)
//-------------------------------------------------------------------------
{
	char s[MAX(kVstMaxParamStrLen + 1, 64)]; // Increased to 64 bytes since 32 bytes doesn't seem to suffice for all plugs. Kind of ridiculous if you consider that kVstMaxParamStrLen = 8...
	s[0] = '\0';

	if(m_Effect.numParams > 0 && param < m_Effect.numParams)
	{
		Dispatch(opcode, param, 0, s, 0);
		mpt::String::SetNullTerminator(s);
	}
	return mpt::ToCString(mpt::CharsetLocale, s);
}


CString CVstPlugin::GetParamName(PlugParamIndex param)
//----------------------------------------------------
{
	VstParameterProperties properties;
	MemsetZero(properties.label);
	if(Dispatch(effGetParameterProperties, param, 0, &properties, 0.0f) == 1)
	{
		mpt::String::SetNullTerminator(properties.label);
		return mpt::ToCString(mpt::CharsetLocale, properties.label);
	} else
	{
		return GetParamPropertyString(param, effGetParamName);
	}
}


CString CVstPlugin::GetDefaultEffectName()
//----------------------------------------
{
	char s[256] = "";
	if(m_bIsVst2)
	{
		Dispatch(effGetEffectName, 0, 0, s, 0);
	}
	return mpt::ToCString(mpt::CharsetLocale, s);
}


void CVstPlugin::Resume()
//-----------------------
{
	const uint32 sampleRate = m_SndFile.GetSampleRate();

	try
	{
		//reset some stuff
		m_MixState.nVolDecayL = 0;
		m_MixState.nVolDecayR = 0;
		if(m_bPlugResumed)
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
		m_bPlugResumed = true;
	} catch (...)
	{
		ReportPlugException(L"Exception in Resume()!");
	}
}


void CVstPlugin::Suspend()
//------------------------
{
	if(m_bPlugResumed)
	{
		try
		{
			Dispatch(effStopProcess, 0, 0, nullptr, 0.0f);
			Dispatch(effMainsChanged, 0, 0, nullptr, 0.0f); // calls plugin's suspend (theoretically, plugins should clean their buffers here, but oh well, the number of plugins which don't do this is surprisingly high.)
			m_bPlugResumed = false;
		} catch (...)
		{
			ReportPlugException(L"Exception in Suspend()!");
		}
	}
}


// Send events to plugin. Returns true if there are events left to be processed.
void CVstPlugin::ProcessVSTEvents()
//---------------------------------
{
	// Process VST events
	if(m_Effect.dispatcher != nullptr && vstEvents.Finalise() > 0)
	{
		try
		{
			m_Effect.dispatcher(&m_Effect, effProcessEvents, 0, 0, &vstEvents, 0);
			ResetSilence();
		} catch (...)
		{
			ReportPlugException(mpt::String::Print(L"Exception in ProcessVSTEvents(numEvents:%1)!",
				vstEvents.GetNumEvents()));
		}
	}
}


// Receive events from plugin and send them to the next plugin in the chain.
void CVstPlugin::ReceiveVSTEvents(const VstEvents *events)
//--------------------------------------------------------
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
		for(VstInt32 i = 0; i < events->numEvents; i++)
		{
			if(vstPlugin != nullptr)
			{
				// Directly enqueue the message and preserve as much of the event data as possible (e.g. delta frames, which are currently not used by OpenMPT but might be by plugins)
				vstPlugin->vstEvents.Enqueue(events->events[i]);
			} else if(plugin != nullptr)
			{
				if(events->events[i]->type == kVstMidiType)
				{
					VstMidiEvent *event = reinterpret_cast<VstMidiEvent *>(events->events[i]);
					plugin->MidiSend(reinterpret_cast<uint32>(event->midiData));
				} else if(events->events[i]->type == kVstSysExType)
				{
					VstMidiSysexEvent *event = reinterpret_cast<VstMidiSysexEvent *>(events->events[i]);
					plugin->MidiSysexSend(event->sysexDump, event->dumpBytes);
				}
			}
		}
	}

#ifdef MODPLUG_TRACKER
	if(m_bRecordMIDIOut)
	{
		// Spam MIDI data to all views
		for(VstInt32 i = 0; i < events->numEvents; i++)
		{
			if(events->events[i]->type == kVstMidiType)
			{
				VstMidiEvent *event = reinterpret_cast<VstMidiEvent *>(events->events[i]);
				::PostMessage(CMainFrame::GetMainFrame()->GetMidiRecordWnd(), WM_MOD_MIDIMSG, *reinterpret_cast<uint32 *>(event->midiData), reinterpret_cast<LPARAM>(this));
			}
		}
	}
#endif // MODPLUG_TRACKER
}


void CVstPlugin::Process(float *pOutL, float *pOutR, uint32 numFrames)
//--------------------------------------------------------------------
{
	ProcessVSTEvents();

	// If the plug is found & ok, continue
	if(m_pProcessFP != nullptr && (m_mixBuffer.GetInputBufferArray()) && m_mixBuffer.GetOutputBufferArray() && m_pMixStruct != nullptr)
	{
		VstInt32 numInputs = m_Effect.numInputs, numOutputs = m_Effect.numOutputs;
		//RecalculateGain();

		// Merge stereo input before sending to the plug if the plug can only handle one input.
		if (numInputs == 1)
		{
			for (uint32 i = 0; i < numFrames; i++)
			{
				m_MixState.pOutBufferL[i] = 0.5f * (m_MixState.pOutBufferL[i] + m_MixState.pOutBufferR[i]);
			}
		}

		float **outputBuffers = m_mixBuffer.GetOutputBufferArray();
		if(!isBridged)
		{
			m_mixBuffer.ClearOutputBuffers(numFrames);
		}

		// Do the VST processing magic
		try
		{
			ASSERT(numFrames <= MIXBUFFERSIZE);
			m_pProcessFP(&m_Effect, m_mixBuffer.GetInputBufferArray(), outputBuffers, numFrames);
		} catch (...)
		{
			Bypass();
			const wchar_t *processMethod = (m_Effect.flags & effFlagsCanReplacing) ? L"processReplacing" : L"process";
			ReportPlugException(mpt::String::Print(L"The plugin threw an exception in %1. It has automatically been set to \"Bypass\".", processMethod));
		}

		ASSERT(outputBuffers != nullptr);

		// Mix outputs of multi-output VSTs:
		if(numOutputs > 2)
		{
			// first, mix extra outputs on a stereo basis
			VstInt32 outs = numOutputs;
			// so if nOuts is not even, let process the last output later
			if((outs % 2u) == 1) outs--;

			// mix extra stereo outputs
			for(VstInt32 iOut = 2; iOut < outs; iOut++)
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
			ProcessMixOps(pOutL, pOutR, outputBuffers[0], outputBuffers[numOutputs > 1 ? 1 : 0], numFrames);
		}


		// If the I/O format of the bridge changed in the meanwhile, update it now.
		if(isBridged && Dispatch(effVendorSpecific, kVendorOpenMPT, kCloseOldProcessingMemory, nullptr, 0.0f) != 0)
		{
			InitializeIOBuffers();
		}
	}

	vstEvents.Clear();
}


bool CVstPlugin::MidiSend(uint32 dwMidiCode)
//------------------------------------------
{
	// Note-Offs go at the start of the queue.
	bool insertAtFront = (MIDIEvents::GetTypeFromEvent(dwMidiCode) == MIDIEvents::evNoteOff);

	VstMidiEvent event;
	event.type = kVstMidiType;
	event.byteSize = sizeof(event);
	event.deltaFrames = 0;
	event.flags = 0;
	event.noteLength = 0;
	event.noteOffset = 0;
	event.detune = 0;
	event.noteOffVelocity = 0;
	event.reserved1 = 0;
	event.reserved2 = 0;
	memcpy(event.midiData, &dwMidiCode, 4);

	#ifdef VST_LOG
		Log("Sending Midi %02X.%02X.%02X\n", event.midiData[0]&0xff, event.midiData[1]&0xff, event.midiData[2]&0xff);
	#endif

	ResetSilence();
	return vstEvents.Enqueue(reinterpret_cast<VstEvent *>(&event), insertAtFront);
}


bool CVstPlugin::MidiSysexSend(const char *message, uint32 length)
//----------------------------------------------------------------
{
	VstMidiSysexEvent event;
	event.type = kVstSysExType;
	event.byteSize = sizeof(event);
	event.deltaFrames = 0;
	event.flags = 0;
	event.dumpBytes = length;
	event.resvd1 = 0;
	event.sysexDump = const_cast<char *>(message);	// We will make our own copy in VstEventQueue::Enqueue
	event.resvd2 = 0;

	ResetSilence();
	return vstEvents.Enqueue(reinterpret_cast<VstEvent *>(&event));
}


void CVstPlugin::HardAllNotesOff()
//--------------------------------
{
	float out[2][SCRATCH_BUFFER_SIZE]; // scratch buffers

	// The JUCE framework doesn't like processing while being suspended.
	const bool wasSuspended = !IsResumed();
	if(wasSuspended)
	{
		Resume();
	}

	for(uint8 mc = 0; mc < CountOf(m_MidiCh); mc++)		//all midi chans
	{
		PlugInstrChannel &channel = m_MidiCh[mc];
		channel.ResetProgram();

		MidiPitchBend(mc, EncodePitchBendParam(MIDIEvents::pitchBendCentre));		// centre pitch bend

		const bool isWavestation = GetUID() == CCONST('K', 'L', 'W', 'V');
		const bool isSawer = GetUID() == CCONST('S', 'a', 'W', 'R');
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

		for(size_t i = 0; i < CountOf(channel.noteOnMap); i++)	//all notes
		{
			for(CHANNELINDEX c = 0; c < CountOf(channel.noteOnMap[i]); c++)
			{
				while(channel.noteOnMap[i][c])
				{
					MidiSend(MIDIEvents::NoteOff(mc, static_cast<uint8>(i), 0));
					channel.noteOnMap[i][c]--;
				}
			}
		}
	}
	// let plug process events
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
//----------------------------------
{
	if(m_pMixStruct == nullptr)
	{
		return;
	}
	m_pMixStruct->defaultProgram = -1;

	if(ProgramsAreChunks() && Dispatch(effIdentify, 0,0, nullptr, 0.0f) == 'NvEf')
	{
		void *p = nullptr;

		// Try to get whole bank
		uint32 nByteSize = mpt::saturate_cast<uint32>(Dispatch(effGetChunk, 0, 0, &p, 0));

		if (!p)
		{
			// Getting bank failed, try to get just preset
			nByteSize = mpt::saturate_cast<uint32>(Dispatch(effGetChunk, 1, 0, &p, 0));
		} else
		{
			// We managed to get the bank, now we need to remember which program we're on.
			m_pMixStruct->defaultProgram = GetCurrentProgram();
		}
		if (p != nullptr)
		{
			LimitMax(nByteSize, Util::MaxValueOfType(nByteSize) - 4);
			if ((m_pMixStruct->pPluginData) && (m_pMixStruct->nPluginDataSize >= nByteSize + 4))
			{
				m_pMixStruct->nPluginDataSize = nByteSize + 4;
			} else
			{
				delete[] m_pMixStruct->pPluginData;
				m_pMixStruct->nPluginDataSize = 0;
				m_pMixStruct->pPluginData = new char[nByteSize + 4];
				if (m_pMixStruct->pPluginData)
				{
					m_pMixStruct->nPluginDataSize = nByteSize + 4;
				}
			}
			if (m_pMixStruct->pPluginData)
			{
				*reinterpret_cast<uint32 *>(m_pMixStruct->pPluginData) = 'NvEf';
				memcpy(m_pMixStruct->pPluginData + 4, p, nByteSize);
				return;
			}
		}
	}
	// This plug doesn't support chunks: save parameters
	IMixPlugin::SaveAllParameters();
}


void CVstPlugin::RestoreAllParameters(int32 program)
//--------------------------------------------------
{
	if(m_pMixStruct != nullptr && m_pMixStruct->pPluginData != nullptr && m_pMixStruct->nPluginDataSize >= 4)
	{
		uint32 type = *reinterpret_cast<const uint32 *>(m_pMixStruct->pPluginData);

		if ((type == 'NvEf') && (Dispatch(effIdentify, 0, 0, nullptr, 0) == 'NvEf'))
		{
			if ((program>=0) && (program < m_Effect.numPrograms))
			{
				// Bank
				Dispatch(effSetChunk, 0, m_pMixStruct->nPluginDataSize - 4, m_pMixStruct->pPluginData + 4, 0);
				SetCurrentProgram(program);
			} else
			{
				// Program
				BeginSetProgram(-1);
				Dispatch(effSetChunk, 1, m_pMixStruct->nPluginDataSize - 4, m_pMixStruct->pPluginData + 4, 0);
				EndSetProgram();
			}
		} else
		{
			IMixPlugin::RestoreAllParameters(program);
		}
	}
}


CAbstractVstEditor *CVstPlugin::OpenEditor()
//------------------------------------------
{
	try
	{
		if (HasEditor())
			return new COwnerVstEditor(*this);
		else
			return new CDefaultVstEditor(*this);
	} catch (...)
	{
		ReportPlugException(L"Exception in OpenEditor()");
		return nullptr;
	}
}


void CVstPlugin::Bypass(bool bypass)
//----------------------------------
{
	Dispatch(effSetBypass, bypass ? 1 : 0, 0, nullptr, 0.0f);
	IMixPlugin::Bypass(bypass);
}


void CVstPlugin::NotifySongPlaying(bool playing)
//----------------------------------------------
{
	m_bSongPlaying = playing;
}


bool CVstPlugin::IsInstrument() const
//-----------------------------------
{
	return ((m_Effect.flags & effFlagsIsSynth) || (!m_Effect.numInputs));
}


bool CVstPlugin::CanRecieveMidiEvents()
//-------------------------------------
{
	return (CVstPlugin::Dispatch(effCanDo, 0, 0, "receiveVstMidiEvent", 0.0f) != 0);
}


void CVstPlugin::ReportPlugException(std::wstring text) const
//-----------------------------------------------------------
{
	text += L" (Plugin=" + m_Factory.libraryName.ToWide() + L")";
	CVstPluginManager::ReportPlugException(text);
}


// Cache program names for plugin bridge
void CVstPlugin::CacheProgramNames(int32 firstProg, int32 lastProg)
//-----------------------------------------------------------------
{
	if(isBridged)
	{
		VstInt32 offsets[2] = { firstProg, lastProg };
		Dispatch(effVendorSpecific, kVendorOpenMPT, kCacheProgramNames, offsets, 0.0f);
	}
}


// Cache parameter names for plugin bridge
void CVstPlugin::CacheParameterNames(int32 firstParam, int32 lastParam)
//---------------------------------------------------------------------
{
	if(isBridged)
	{
		VstInt32 offsets[2] = { firstParam, lastParam };
		Dispatch(effVendorSpecific, kVendorOpenMPT, kCacheParameterInfo, offsets, 0.0f);
	}
}


size_t CVstPlugin::GetChunk(char *(&chunk), bool isBank)
//------------------------------------------------------
{
	return Dispatch(effGetChunk, isBank ? 0 : 1, 0, &chunk, 0);
}


void CVstPlugin::SetChunk(size_t size, char *chunk, bool isBank)
//--------------------------------------------------------------
{
	Dispatch(effSetChunk, isBank ? 0 : 1, size, chunk, 0);
}


OPENMPT_NAMESPACE_END

#endif // NO_VST
