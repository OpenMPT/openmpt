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
#include "DMOPlugin.h"
#include "DigiBoosterEcho.h"
#include "../../mptrack/Vstplug.h"
#include "../../mptrack/Mptrack.h"
#include "../../mptrack/TrackerSettings.h"
#include "../../mptrack/AbstractVstEditor.h"
#include "../../common/AudioCriticalSection.h"
#include "../../common/StringFixer.h"
#include "../Sndfile.h"
#include "../../pluginBridge/BridgeWrapper.h"

// For CRC32 calculation (to tell plugins with same UID apart in our cache file)
#if !defined(NO_ZLIB)
#include <zlib/zlib.h>
#elif !defined(NO_MINIZ)
#define MINIZ_HEADER_FILE_ONLY
#include <miniz/miniz.c>
#endif

OPENMPT_NAMESPACE_BEGIN

char CVstPluginManager::s_szHostProductString[64] = "OpenMPT";
char CVstPluginManager::s_szHostVendorString[64] = "OpenMPT project";
VstInt32 CVstPluginManager::s_nHostVendorVersion = MptVersion::num;

typedef AEffect * (VSTCALLBACK * PVSTPLUGENTRY)(audioMasterCallback);

//#define VST_LOG

#ifndef NO_DMO
#define DMO_LOG
#endif // NO_DMO

static const MPT_UCHAR_TYPE *const cacheSection = MPT_ULITERAL("PluginCache");


uint8 VSTPluginLib::GetDllBits(bool fromCache) const
//--------------------------------------------------
{
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

void VSTPluginLib::WriteToCache() const
//-------------------------------------
{
	SettingsContainer &cacheFile = theApp.GetPluginCache();

	const std::string libName = libraryName.ToUTF8();
	const uint32 crc = crc32(0, reinterpret_cast<const Bytef *>(&libName[0]), libName.length());
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


bool CVstPluginManager::CreateMixPluginProc(SNDMIXPLUGIN &mixPlugin, CSoundFile &sndFile)
//---------------------------------------------------------------------------------------
{
	CVstPluginManager *that = theApp.GetPluginManager();
	if(that)
	{
		return that->CreateMixPlugin(mixPlugin, sndFile);
	}
	return false;
}


CVstPluginManager::CVstPluginManager()
//------------------------------------
{
	CSoundFile::gpMixPluginCreateProc = CreateMixPluginProc;
	EnumerateDirectXDMOs();
}


CVstPluginManager::~CVstPluginManager()
//-------------------------------------
{
	CSoundFile::gpMixPluginCreateProc = nullptr;
	for(const_iterator p = begin(); p != end(); p++)
	{
		while((**p).pPluginsList != nullptr)
		{
			(**p).pPluginsList->Release();
		}
		delete *p;
	}
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

						VSTPluginLib *plug = new (std::nothrow) VSTPluginLib(mpt::PathString::FromNative(Util::GUIDToString(clsid)), mpt::PathString::FromNative(name), mpt::ustring());
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
}


#ifndef NO_VST

AEffect *CVstPluginManager::LoadPlugin(VSTPluginLib &plugin, HINSTANCE &library, bool forceBridge)
//------------------------------------------------------------------------------------------------
{
	const mpt::PathString &pluginPath = plugin.dllPath;

	AEffect *effect = nullptr;
	library = nullptr;

	const bool isNative = plugin.IsNative(false);
	if(forceBridge || plugin.useBridge || !isNative)
	{
		try
		{
			effect = BridgeWrapper::Create(plugin);
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
				return false;
			}
		} catch(BridgeWrapper::BridgeException &e)
		{
			// If there was some error, don't try normal loading as well... unless the user really wants it.
			if(isNative)
			{
				const std::wstring msg = L"The following error occured while trying to load\n" + plugin.dllPath.ToWide() + L"\n\n" + mpt::ToWide(mpt::CharsetUTF8, e.what())
					+ L"\n\nDo you want to try to load the plugin natively?";
				if(Reporting::Confirm(msg, L"OpenMPT Plugin Bridge") == cnfNo)
				{
					return nullptr;
				}
			} else
			{
				Reporting::Error(e.what(), "OpenMPT Plugin Bridge");
				return nullptr;
			}
		}
		// If plugin was marked to use the plugin bridge but this somehow doesn't work (e.g. because the bridge is missing),
		// disable the plugin bridge for this plugin.
		plugin.useBridge = false;
	}

	try
	{
		library = LoadLibraryW(pluginPath.AsNative().c_str());

		if(library == nullptr)
		{
			DWORD error = GetLastError();

#ifdef _DEBUG
			if(error != ERROR_MOD_NOT_FOUND)	// "File not found errors" are annoying.
			{
				mpt::ustring buf = mpt::String::Print(MPT_USTRING("Warning: encountered problem when loading plugin dll. Error %1: %2")
					, mpt::ufmt::hex(error)
					, mpt::ToUnicode(GetErrorMessage(error))
					);
				Reporting::Error(buf, "DEBUG: Error when loading plugin dll");
			}
#endif //_DEBUG

			if(error == ERROR_MOD_NOT_FOUND)
			{
				// No point in trying bridging, either...
				return nullptr;
			}
		}
	} catch(...)
	{
		CVstPluginManager::ReportPlugException(mpt::String::Print(L"Exception caught in LoadLibrary (%1)", pluginPath));
	}

	if(library != nullptr && library != INVALID_HANDLE_VALUE)
	{
		// Try loading the VST plugin.
		PVSTPLUGENTRY pMainProc = (PVSTPLUGENTRY)GetProcAddress(library, "VSTPluginMain");
		if(pMainProc == nullptr)
		{
			pMainProc = (PVSTPLUGENTRY)GetProcAddress(library, "main");
		}

		if(pMainProc != nullptr)
		{
			effect = pMainProc(CVstPlugin::MasterCallBack);
		} else
		{
#ifdef VST_LOG
			Log("Entry point not found! (handle=%08X)\n", library);
#endif // VST_LOG
		}
	}

	return effect;
}

#endif // NO_VST



// Extract instrument and category information from plugin.
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
				VSTPluginLib *plug = new (std::nothrow) VSTPluginLib(dllPath, fileName, tags);
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

	AEffect *pEffect;
	HINSTANCE hLib;
	bool validPlug = false;

	VSTPluginLib *plug = new (std::nothrow) VSTPluginLib(dllPath, fileName, tags);
	if(plug == nullptr)
	{
		return nullptr;
	}

	try
	{
		// Always scan plugins in a separate process
		pEffect = LoadPlugin(*plug, hLib, true);

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


// Create an instance of a plugin.
bool CVstPluginManager::CreateMixPlugin(SNDMIXPLUGIN &mixPlugin, CSoundFile &sndFile)
//-----------------------------------------------------------------------------------
{
	VSTPluginLib *pFound = nullptr;
	mixPlugin.SetAutoSuspend(TrackerSettings::Instance().enableAutoSuspend);

	// Find plugin in library
	int8 match = 0;	// "Match quality" of found plugin. Higher value = better match.
	for(const_iterator p = begin(); p != end(); p++)
	{
		VSTPluginLib *plug = *p;
		const bool matchID = (plug->pluginId1 == mixPlugin.Info.dwPluginId1)
			&& (plug->pluginId2 == mixPlugin.Info.dwPluginId2);
		const bool matchName = !mpt::PathString::CompareNoCase(plug->libraryName, mpt::PathString::FromUTF8(mixPlugin.GetLibraryName()));

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

	if(!memcmp(&mixPlugin.Info.dwPluginId1, "DBM0", 4) && !memcmp(&mixPlugin.Info.dwPluginId2, "Echo", 4))
	{
		// DigiBooster Pro Echo DSP
		IMixPlugin *pVstPlug = new (std::nothrow) DigiBoosterEcho(*pFound, sndFile, &mixPlugin);
		return pVstPlug != nullptr;
	}

	if(mixPlugin.Info.dwPluginId1 == kDmoMagic)
	{
#ifndef NO_DMO
		if (!pFound) return false;
		IMixPlugin *plugin = DMOPlugin::Create(*pFound, sndFile, &mixPlugin);
		return plugin != nullptr;
#else
		return nullptr;
#endif // NO_DMO
	}

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

	if(pFound)
	{
		AEffect *pEffect = nullptr;
		HINSTANCE hLibrary = nullptr;
		bool validPlugin = false;

#ifndef NO_VST
		try
		{
			pEffect = LoadPlugin(*pFound, hLibrary, TrackerSettings::Instance().bridgeAllPlugins);

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
#endif

		return validPlugin;
	} else
	{
		// "plug not found" notification code MOVED to CSoundFile::Create
#ifdef VST_LOG
		Log("Unknown plugin\n");
#endif
	}
	return false;
}


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
{
	Reporting::Notification(msg.c_str());
#ifdef VST_LOG
	Log("%s", msg.c_str());
#endif
}

void CVstPluginManager::ReportPlugException(const std::wstring &msg)
{
	Reporting::Notification(msg);
#ifdef VST_LOG
	Log(mpt::ToUnicode(msg));
#endif
}

#endif // NO_PLUGINS


OPENMPT_NAMESPACE_END
