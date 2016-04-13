/*
 * PluginManager.cpp
 * -----------------
 * Purpose: Implementation of the plugin manager, which keeps a list of known plugins and instantiates them.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#ifndef NO_PLUGINS

#include "../../common/version.h"
#include "PluginManager.h"
#include "PlugInterface.h"
#include "DigiBoosterEcho.h"
#include "dmo/DMOPlugin.h"
#include "dmo/Distortion.h"
#include "dmo/Echo.h"
#include "dmo/Gargle.h"
#include "dmo/ParamEq.h"
#include "dmo/WavesReverb.h"
#include "../../common/StringFixer.h"
#include "../Sndfile.h"
#include "../Loaders.h"

#ifndef NO_VST
#include "../../mptrack/Vstplug.h"
#include "../../pluginBridge/BridgeWrapper.h"
#endif // NO_VST

#ifndef NO_DMO
#include <winreg.h>
#include <strmif.h>
#include <tchar.h>
#endif // NO_DMO

#ifdef MODPLUG_TRACKER
#include "../../mptrack/Mptrack.h"
#include "../../mptrack/TrackerSettings.h"
#include "../../mptrack/AbstractVstEditor.h"
#include "../../soundlib/AudioCriticalSection.h"
#include "../common/mptCRC.h"
#endif // MODPLUG_TRACKER

OPENMPT_NAMESPACE_BEGIN

//#define VST_LOG
//#define DMO_LOG

#ifdef MODPLUG_TRACKER
static const MPT_UCHAR_TYPE *const cacheSection = MPT_ULITERAL("PluginCache");
#endif // MODPLUG_TRACKER


uint8 VSTPluginLib::GetDllBits(bool fromCache) const
//--------------------------------------------------
{
	// Built-in plugins are always native.
	if(dllPath.empty())
		return sizeof(void *) * CHAR_BIT;
#ifndef NO_VST
	if(!dllBits || !fromCache)
	{
		dllBits = static_cast<uint8>(BridgeWrapper::GetPluginBinaryType(dllPath));
	}
#else
	MPT_UNREFERENCED_PARAMETER(fromCache);
#endif // NO_VST
	return dllBits;
}


// PluginCache format:
// LibraryName = <ID1><ID2><CRC32> (hex-encoded)
// <ID1><ID2><CRC32> = FullDllPath
// <ID1><ID2><CRC32>.Flags = Plugin Flags (set VSTPluginLib::DecodeCacheFlags).

#ifdef MODPLUG_TRACKER
void VSTPluginLib::WriteToCache() const
//-------------------------------------
{
	SettingsContainer &cacheFile = theApp.GetPluginCache();

	const std::string libName = libraryName.ToUTF8();
	const uint32 crc = mpt::crc32(libName);
	const mpt::ustring IDs = mpt::ufmt::HEX0<8>(SwapBytesReturnLE(pluginId1)) + mpt::ufmt::HEX0<8>(SwapBytesReturnLE(pluginId2)) + mpt::ufmt::HEX0<8>(SwapBytesReturnLE(crc));
	const mpt::ustring flagsKey = IDs + MPT_USTRING(".Flags");

	mpt::PathString writePath = dllPath;
	if(theApp.IsPortableMode())
	{
		writePath = theApp.AbsolutePathToRelative(writePath);
	}

	cacheFile.Write<mpt::ustring>(cacheSection, libraryName.ToUnicode(), IDs);
	cacheFile.Write<mpt::PathString>(cacheSection, IDs, writePath);
	cacheFile.Write<int32>(cacheSection, flagsKey, EncodeCacheFlags());
}
#endif // MODPLUG_TRACKER


bool CreateMixPluginProc(SNDMIXPLUGIN &mixPlugin, CSoundFile &sndFile)
//--------------------------------------------------------------------
{
#ifdef MODPLUG_TRACKER
	CVstPluginManager *that = theApp.GetPluginManager();
	if(that)
	{
		return that->CreateMixPlugin(mixPlugin, sndFile);
	}
	return false;
#else
	if(!sndFile.m_PluginManager)
	{
		sndFile.m_PluginManager = mpt::make_shared<CVstPluginManager>();
	}
	return sndFile.m_PluginManager->CreateMixPlugin(mixPlugin, sndFile);
#endif // MODPLUG_TRACKER
}


CVstPluginManager::CVstPluginManager()
//------------------------------------
#if MPT_OS_WINDOWS && !defined(NO_DMO)
	: MustUnInitilizeCOM(false)
#endif
{

	#if MPT_OS_WINDOWS && !defined(NO_DMO)
		HRESULT COMinit = CoInitializeEx(NULL, COINIT_MULTITHREADED);
		if(COMinit == S_OK || COMinit == S_FALSE)
		{
			MustUnInitilizeCOM = true;
		}
	#endif

	// DirectX Media Objects
	EnumerateDirectXDMOs();

	// Hard-coded "plugins"
	VSTPluginLib *plug;

	// DigiBooster Pro Echo DSP
	plug = new (std::nothrow) VSTPluginLib(DigiBoosterEcho::Create, mpt::PathString(), MPT_PATHSTRING("DigiBooster Pro Echo"), mpt::ustring());
	if(plug != nullptr)
	{
		pluginList.push_back(plug);
		memcpy(&plug->pluginId1, "DBM0", 4);
		memcpy(&plug->pluginId2, "Echo", 4);
		plug->category = VSTPluginLib::catRoomFx;
	}

#ifdef NO_DMO
	// DirectX Media Objects Emulation
	plug = new (std::nothrow) VSTPluginLib(DMO::Distortion::Create, MPT_PATHSTRING("{EF114C90-CD1D-484E-96E5-09CFAF912A21}"), MPT_PATHSTRING("Distortion"), mpt::ustring());
	if(plug != nullptr)
	{
		pluginList.push_back(plug);
		plug->pluginId1 = kDmoMagic;
		plug->pluginId2 = 0xEF114C90;
		plug->category = VSTPluginLib::catDMO;
	}

	plug = new (std::nothrow) VSTPluginLib(DMO::Echo::Create, MPT_PATHSTRING("{EF3E932C-D40B-4F51-8CCF-3F98F1B29D5D}"), MPT_PATHSTRING("Echo"), mpt::ustring());
	if(plug != nullptr)
	{
		pluginList.push_back(plug);
		plug->pluginId1 = kDmoMagic;
		plug->pluginId2 = 0xEF3E932C;
		plug->category = VSTPluginLib::catDMO;
	}

	plug = new (std::nothrow) VSTPluginLib(DMO::Gargle::Create, MPT_PATHSTRING("{DAFD8210-5711-4B91-9FE3-F75B7AE279BF}"), MPT_PATHSTRING("Gargle"), mpt::ustring());
	if(plug != nullptr)
	{
		pluginList.push_back(plug);
		plug->pluginId1 = kDmoMagic;
		plug->pluginId2 = 0xDAFD8210;
		plug->category = VSTPluginLib::catDMO;
	}

	plug = new (std::nothrow) VSTPluginLib(DMO::ParamEq::Create, MPT_PATHSTRING("{120CED89-3BF4-4173-A132-3CB406CF3231}"), MPT_PATHSTRING("ParamEq"), mpt::ustring());
	if(plug != nullptr)
	{
		pluginList.push_back(plug);
		plug->pluginId1 = kDmoMagic;
		plug->pluginId2 = 0x120CED89;
		plug->category = VSTPluginLib::catDMO;
	}

	plug = new (std::nothrow) VSTPluginLib(DMO::WavesReverb::Create, MPT_PATHSTRING("{87FC0268-9A55-4360-95AA-004A1D9DE26C}"), MPT_PATHSTRING("WavesReverb"), mpt::ustring());
	if(plug != nullptr)
	{
		pluginList.push_back(plug);
		plug->pluginId1 = kDmoMagic;
		plug->pluginId2 = 0x87FC0268;
		plug->category = VSTPluginLib::catDMO;
	}
#endif // NO_DMO
}


CVstPluginManager::~CVstPluginManager()
//-------------------------------------
{
	for(const_iterator p = begin(); p != end(); p++)
	{
		while((**p).pPluginsList != nullptr)
		{
			(**p).pPluginsList->Release();
		}
		delete *p;
	}
	#if MPT_OS_WINDOWS && !defined(NO_DMO)
		if(MustUnInitilizeCOM)
		{
			CoUninitialize();
			MustUnInitilizeCOM = false;
		}
	#endif
}


bool CVstPluginManager::IsValidPlugin(const VSTPluginLib *pLib) const
//-------------------------------------------------------------------
{
	for(const_iterator p = begin(); p != end(); p++)
	{
		if(*p == pLib) return true;
	}
	return false;
}


void CVstPluginManager::EnumerateDirectXDMOs()
//--------------------------------------------
{
#ifndef NO_DMO
	HKEY hkEnum;
	WCHAR keyname[128];

	LONG cr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("software\\classes\\DirectShow\\MediaObjects\\Categories\\f3602b3f-0592-48df-a4cd-674721e7ebeb"), 0, KEY_READ, &hkEnum);
	DWORD index = 0;
	while (cr == ERROR_SUCCESS)
	{
		if ((cr = RegEnumKeyW(hkEnum, index, keyname, CountOf(keyname))) == ERROR_SUCCESS)
		{
			CLSID clsid;
			std::wstring formattedKey = std::wstring(L"{") + std::wstring(keyname) + std::wstring(L"}");
			if(Util::VerifyStringToCLSID(formattedKey, clsid))
			{
				HKEY hksub;
				formattedKey = std::wstring(L"software\\classes\\DirectShow\\MediaObjects\\") + std::wstring(keyname);
				if (RegOpenKeyW(HKEY_LOCAL_MACHINE, formattedKey.c_str(), &hksub) == ERROR_SUCCESS)
				{
					WCHAR name[64];
					DWORD datatype = REG_SZ;
					DWORD datasize = sizeof(name);

					if(ERROR_SUCCESS == RegQueryValueExW(hksub, nullptr, 0, &datatype, (LPBYTE)name, &datasize))
					{
						mpt::String::SetNullTerminator(name);

						VSTPluginLib *plug = new (std::nothrow) VSTPluginLib(DMOPlugin::Create, mpt::PathString::FromNative(Util::GUIDToString(clsid)), mpt::PathString::FromNative(name), mpt::ustring());
						if(plug != nullptr)
						{
							pluginList.push_back(plug);
							plug->pluginId1 = kDmoMagic;
							plug->pluginId2 = clsid.Data1;
							plug->category = VSTPluginLib::catDMO;
#ifdef DMO_LOG
							Log(mpt::String::Print(L"Found \"%1\" clsid=%2\n", plug->libraryName, plug->dllPath));
#endif
						}
					}
					RegCloseKey(hksub);
				}
			}
		}
		index++;
	}
	if (hkEnum) RegCloseKey(hkEnum);
#endif // NO_DMO
}


// Extract instrument and category information from plugin.
#ifndef NO_VST
static void GetPluginInformation(AEffect *effect, VSTPluginLib &library)
//----------------------------------------------------------------------
{
	library.category = static_cast<VSTPluginLib::PluginCategory>(effect->dispatcher(effect, effGetPlugCategory, 0, 0, nullptr, 0.0f));
	library.isInstrument = ((effect->flags & effFlagsIsSynth) || !effect->numInputs);

	if(library.isInstrument)
	{
		library.category = VSTPluginLib::catSynth;
	} else if(library.category >= VSTPluginLib::numCategories)
	{
		library.category = VSTPluginLib::catUnknown;
	}
}
#endif // NO_VST


#ifdef MODPLUG_TRACKER
// Add a plugin to the list of known plugins.
VSTPluginLib *CVstPluginManager::AddPlugin(const mpt::PathString &dllPath, const mpt::ustring &tags, bool fromCache, const bool checkFileExistence, std::wstring *const errStr)
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
{
	const mpt::PathString fileName = dllPath.GetFileName();

	// Check if this is already a known plugin.
	for(const_iterator dupePlug = begin(); dupePlug != end(); dupePlug++)
	{
		if(!dllPath.CompareNoCase(dllPath, (**dupePlug).dllPath)) return *dupePlug;
	}

	if(checkFileExistence && errStr != nullptr && !dllPath.IsFile())
	{
		*errStr += L"\nUnable to find " + dllPath.ToWide();
	}

	// Look if the plugin info is stored in the PluginCache
	if(fromCache)
	{
		SettingsContainer & cacheFile = theApp.GetPluginCache();
		const mpt::ustring IDs = cacheFile.Read<mpt::ustring>(cacheSection, fileName.ToUnicode(), MPT_USTRING(""));

		if(IDs.length() >= 16)
		{
			// Get path from cache file
			mpt::PathString realPath = cacheFile.Read<mpt::PathString>(cacheSection, IDs, MPT_PATHSTRING(""));
			realPath = theApp.RelativePathToAbsolute(realPath);

			if(!realPath.empty() && !dllPath.CompareNoCase(realPath, dllPath))
			{
				VSTPluginLib *plug = new (std::nothrow) VSTPluginLib(nullptr, dllPath, fileName, tags);
				if(plug == nullptr)
				{
					return nullptr;
				}
				pluginList.push_back(plug);

				// Extract plugin IDs
				for (int i = 0; i < 16; i++)
				{
					int32 n = IDs[i] - '0';
					if (n > 9) n = IDs[i] + 10 - 'A';
					n &= 0x0f;
					if (i < 8)
					{
						plug->pluginId1 = (plug->pluginId1 << 4) | n;
					} else
					{
						plug->pluginId2 = (plug->pluginId2 << 4) | n;
					}
				}

				const mpt::ustring flagKey = IDs + MPT_USTRING(".Flags");
				plug->DecodeCacheFlags(cacheFile.Read<int32>(cacheSection, flagKey, 0));

#ifdef VST_LOG
				Log("Plugin \"%s\" found in PluginCache\n", plug->libraryName.ToLocale().c_str());
#endif // VST_LOG
				return plug;
			} else
			{
#ifdef VST_LOG
				Log("Plugin \"%s\" mismatch in PluginCache: \"%s\" [%s]=\"%s\"\n", s, dllPath, (LPCTSTR)IDs, (LPCTSTR)strFullPath);
#endif // VST_LOG
			}
		}
	}

	// If this key contains a file name on program launch, a plugin previously crashed OpenMPT.
	theApp.GetSettings().Write<mpt::PathString>(MPT_USTRING("VST Plugins"), MPT_USTRING("FailedPlugin"), dllPath, SettingWriteThrough);

	bool validPlug = false;

	VSTPluginLib *plug = new (std::nothrow) VSTPluginLib(nullptr, dllPath, fileName, tags);
	if(plug == nullptr)
	{
		return nullptr;
	}

#ifndef NO_VST
	try
	{
		// Always scan plugins in a separate process
		HINSTANCE hLib = NULL;
		AEffect *pEffect = CVstPlugin::LoadPlugin(*plug, hLib, true);

		if(pEffect != nullptr && pEffect->magic == kEffectMagic && pEffect->dispatcher != nullptr)
		{
			pEffect->dispatcher(pEffect, effOpen, 0, 0, 0, 0);

			plug->pluginId1 = pEffect->magic;
			plug->pluginId2 = pEffect->uniqueID;

			GetPluginInformation(pEffect, *plug);

#ifdef VST_LOG
			int nver = pEffect->dispatcher(pEffect, effGetVstVersion, 0,0, nullptr, 0);
			if (!nver) nver = pEffect->version;
			Log("%-20s: v%d.0, %d in, %d out, %2d programs, %2d params, flags=0x%04X realQ=%d offQ=%d\n",
				plug->libraryName.ToLocale().c_str(), nver,
				pEffect->numInputs, pEffect->numOutputs,
				pEffect->numPrograms, pEffect->numParams,
				pEffect->flags, pEffect->realQualities, pEffect->offQualities);
#endif // VST_LOG

			pEffect->dispatcher(pEffect, effClose, 0, 0, 0, 0);

			validPlug = true;
		}

		FreeLibrary(hLib);
	} catch(...)
	{
		CVstPluginManager::ReportPlugException(mpt::String::Print(L"Exception while trying to load plugin \"%1\"!\n", plug->libraryName));
	}
#endif // NO_VST

	// Now it should be safe to assume that this plugin loaded properly. :)
	theApp.GetSettings().Remove(MPT_USTRING("VST Plugins"), MPT_USTRING("FailedPlugin"));

	// If OK, write the information in PluginCache
	if(validPlug)
	{
		pluginList.push_back(plug);
		plug->WriteToCache();
	} else
	{
		delete plug;
	}

	return (validPlug ? plug : nullptr);
}


// Remove a plugin from the list of known plugins and release any remaining instances of it.
bool CVstPluginManager::RemovePlugin(VSTPluginLib *pFactory)
//----------------------------------------------------------
{
	for(const_iterator p = begin(); p != end(); p++)
	{
		VSTPluginLib *plug = *p;
		if(plug == pFactory)
		{
			// Kill all instances of this plugin
			try
			{
				CriticalSection cs;

				while(plug->pPluginsList != nullptr)
				{
					plug->pPluginsList->Release();
				}
				pluginList.erase(p);
				delete plug;
			} catch (...)
			{
				CVstPluginManager::ReportPlugException(mpt::String::Print(L"Exception while trying to release plugin \"%1\"!\n", pFactory->libraryName));
			}

			return true;
		}
	}
	return false;
}
#endif // MODPLUG_TRACKER


// Create an instance of a plugin.
bool CVstPluginManager::CreateMixPlugin(SNDMIXPLUGIN &mixPlugin, CSoundFile &sndFile)
//-----------------------------------------------------------------------------------
{
	VSTPluginLib *pFound = nullptr;
#ifdef MODPLUG_TRACKER
	mixPlugin.SetAutoSuspend(TrackerSettings::Instance().enableAutoSuspend);
#endif // MODPLUG_TRACKER

	// Find plugin in library
	int8 match = 0;	// "Match quality" of found plugin. Higher value = better match.
	for(const_iterator p = begin(); p != end(); p++)
	{
		VSTPluginLib *plug = *p;
		const bool matchID = (plug->pluginId1 == mixPlugin.Info.dwPluginId1)
			&& (plug->pluginId2 == mixPlugin.Info.dwPluginId2);
#if MPT_OS_WINDOWS
		const bool matchName = !mpt::PathString::CompareNoCase(plug->libraryName, mpt::PathString::FromUTF8(mixPlugin.GetLibraryName()));
#else
		const bool matchName = (mpt::ToLowerCaseAscii(plug->libraryName.ToUTF8()) == mpt::ToLowerCaseAscii(mixPlugin.GetLibraryName()));
#endif

		if(matchID && matchName)
		{
			pFound = plug;
			if(plug->IsNative(false))
			{
				break;
			}
			// If the plugin isn't native, first check if a native version can be found.
			match = 3;
		} else if(matchID && match < 2)
		{
			pFound = plug;
			match = 2;
		} else if(matchName && match < 1)
		{
			pFound = plug;
			match = 1;
		}
	}

	if(pFound != nullptr && pFound->Create != nullptr)
	{
		IMixPlugin *plugin = pFound->Create(*pFound, sndFile, &mixPlugin);
		return plugin != nullptr;
	}

#ifdef MODPLUG_TRACKER
	if(!pFound && strcmp(mixPlugin.GetLibraryName(), ""))
	{
		// Try finding the plugin DLL in the plugin directory or plugin cache instead.
		mpt::PathString fullPath = TrackerSettings::Instance().PathPlugins.GetDefaultDir();
		if(fullPath.empty())
		{
			fullPath = theApp.GetAppDirPath() + MPT_PATHSTRING("Plugins\\");
		}
		fullPath += mpt::PathString::FromUTF8(mixPlugin.GetLibraryName()) + MPT_PATHSTRING(".dll");

		pFound = AddPlugin(fullPath, mpt::ustring());
		if(!pFound)
		{
			// Try plugin cache (search for library name)
			SettingsContainer &cacheFile = theApp.GetPluginCache();
			mpt::ustring IDs = cacheFile.Read<mpt::ustring>(cacheSection, mpt::ToUnicode(mpt::CharsetUTF8, mixPlugin.GetLibraryName()), MPT_USTRING(""));
			if(IDs.length() >= 16)
			{
				fullPath = cacheFile.Read<mpt::PathString>(cacheSection, IDs, MPT_PATHSTRING(""));
				if(!fullPath.empty())
				{
					fullPath = theApp.RelativePathToAbsolute(fullPath);
					if(fullPath.IsFile())
					{
						pFound = AddPlugin(fullPath, mpt::ustring());
					}
				}
			}
		}
	}

#ifndef NO_VST
	if(pFound && mixPlugin.Info.dwPluginId1 == kEffectMagic)
	{
		AEffect *pEffect = nullptr;
		HINSTANCE hLibrary = nullptr;
		bool validPlugin = false;

		try
		{
			pEffect = CVstPlugin::LoadPlugin(*pFound, hLibrary, TrackerSettings::Instance().bridgeAllPlugins);

			if(pEffect != nullptr && pEffect->dispatcher != nullptr && pEffect->magic == kEffectMagic)
			{
				validPlugin = true;

				GetPluginInformation(pEffect, *pFound);

				// Update cached information
				pFound->WriteToCache();

				CVstPlugin *pVstPlug = new (std::nothrow) CVstPlugin(hLibrary, *pFound, mixPlugin, *pEffect, sndFile);
				if(pVstPlug == nullptr)
				{
					validPlugin = false;
				}
			}

			if(!validPlugin)
			{
				FreeLibrary(hLibrary);
			}
		} catch(...)
		{
			CVstPluginManager::ReportPlugException(mpt::String::Print(L"Exception while trying to create plugin \"%1\"!\n", pFound->libraryName));
		}
		return validPlugin;
	} else
	{
		// "plug not found" notification code MOVED to CSoundFile::Create
#ifdef VST_LOG
		Log("Unknown plugin\n");
#endif
	}
#endif // NO_VST

#endif // MODPLUG_TRACKER
	return false;
}


#ifdef MODPLUG_TRACKER
void CVstPluginManager::OnIdle()
//------------------------------
{
	for(const_iterator pFactory = begin(); pFactory != end(); pFactory++)
	{
		// Note: bridged plugins won't receive these messages and generate their own idle messages.
		IMixPlugin *p = (**pFactory).pPluginsList;
		while (p)
		{
			//rewbs. VSTCompliance: A specific plug has requested indefinite periodic processing time.
			p->Idle();
			//We need to update all open editors
			CAbstractVstEditor *editor = p->GetEditor();
			if (editor && editor->m_hWnd)
			{
				editor->UpdateParamDisplays();
			}
			//end rewbs. VSTCompliance:

			p = p->GetNextInstance();
		}
	}
}


void CVstPluginManager::ReportPlugException(const std::string &msg)
//-----------------------------------------------------------------
{
	Reporting::Notification(msg.c_str());
#ifdef VST_LOG
	Log("%s", msg.c_str());
#endif
}

void CVstPluginManager::ReportPlugException(const std::wstring &msg)
//--------------------------------------------------------------
{
	Reporting::Notification(msg);
#ifdef VST_LOG
	Log(mpt::ToUnicode(msg));
#endif
}
#endif // MODPLUG_TRACKER

OPENMPT_NAMESPACE_END

#endif // NO_PLUGINS
