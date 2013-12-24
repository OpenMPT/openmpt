/*
 * PluginManager.cpp
 * -----------------
 * Purpose: Implementation of the plugin manager, which keeps a list of known plugins and instantiates them.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#ifndef NO_VST
#include "../../common/version.h"
#include "../../mptrack/Vstplug.h"
#include "../../mptrack/Mptrack.h"
#include "../../mptrack/TrackerSettings.h"
#include "../../mptrack/AbstractVstEditor.h"
#include "../../common/AudioCriticalSection.h"
#include "../../common/StringFixer.h"
#include "../Sndfile.h"
#include "JBridge.h"

// For CRC32 calculation (to tell plugins with same UID apart in our cache file)
#if !defined(NO_ZLIB)
#include <zlib/zlib.h>
#elif !defined(NO_MINIZ)
#define MINIZ_HEADER_FILE_ONLY
#include <miniz/miniz.c>
#endif

char CVstPluginManager::s_szHostProductString[64] = "OpenMPT";
char CVstPluginManager::s_szHostVendorString[64] = "OpenMPT project";
VstIntPtr CVstPluginManager::s_nHostVendorVersion = MptVersion::num;

typedef AEffect * (VSTCALLBACK * PVSTPLUGENTRY)(audioMasterCallback);

//#define VST_LOG
#define DMO_LOG

AEffect *DmoToVst(VSTPluginLib &lib);

static const char *const cacheSection = "PluginCache";
static const wchar_t *const cacheSectionW = L"PluginCache";


// PluginCache format:
// LibraryName = <ID1><ID2><CRC32> (hex-encoded)
// <ID1><ID2><CRC32> = FullDllPath
// <ID1><ID2><CRC32>.Flags = Plugin Flags (set VSTPluginLib::DecodeCacheFlags).

static void WriteToCache(const VSTPluginLib &lib)
//-----------------------------------------------
{
	SettingsContainer &cacheFile = theApp.GetPluginCache();

	const std::string libName = lib.libraryName.ToUTF8();
	const uint32 crc = crc32(0, reinterpret_cast<const Bytef *>(&libName[0]), libName.length());
	const std::string IDs = mpt::String::Format("%08X%08X%08X", SwapBytesReturnLE(lib.pluginId1), SwapBytesReturnLE(lib.pluginId2), SwapBytesReturnLE(crc));
	const std::string flagsKey = IDs + ".Flags";

	mpt::PathString writePath = lib.dllPath;
	if(theApp.IsPortableMode())
	{
		writePath = theApp.AbsolutePathToRelative(writePath);
	}

	cacheFile.Write<std::string>(cacheSectionW, lib.libraryName.ToWide(), IDs);
	cacheFile.Write<mpt::PathString>(cacheSection, IDs, writePath);
	cacheFile.Write<int32>(cacheSection, flagsKey, lib.EncodeCacheFlags());
}


VstIntPtr VSTCALLBACK CVstPluginManager::MasterCallBack(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt)
//----------------------------------------------------------------------------------------------------------------------------------------------
{
	CVstPluginManager *that = theApp.GetPluginManager();
	if(that)
	{
		return that->VstCallback(effect, opcode, index, value, ptr, opt);
	}
	return 0;
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

	LONG cr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "software\\classes\\DirectShow\\MediaObjects\\Categories\\f3602b3f-0592-48df-a4cd-674721e7ebeb", 0, KEY_READ, &hkEnum);
	DWORD index = 0;
	while (cr == ERROR_SUCCESS)
	{
		if ((cr = RegEnumKeyW(hkEnum, index, keyname, CountOf(keyname))) == ERROR_SUCCESS)
		{
			CLSID clsid;
			std::wstring formattedKey = std::wstring(L"{") + std::wstring(keyname) + std::wstring(L"}");
			if(Util::StringToCLSID(formattedKey, clsid))
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
						StringFromGUID2(clsid, keyname, 100);

						VSTPluginLib *plug = new (std::nothrow) VSTPluginLib(mpt::PathString::FromNative(keyname), mpt::PathString::FromNative(name));
						if(plug != nullptr)
						{
							pluginList.push_back(plug);
							plug->pluginId1 = kDmoMagic;
							plug->pluginId2 = clsid.Data1;
							plug->category = VSTPluginLib::catDMO;
#ifdef DMO_LOG
							Log(mpt::String::PrintW(L"Found \"%1\" clsid=%2\n", plug->libraryName, plug->dllPath));
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


AEffect *CVstPluginManager::LoadPlugin(const mpt::PathString &pluginPath, HINSTANCE &library)
//-------------------------------------------------------------------------------------------
{
	AEffect *effect = nullptr;
	library = nullptr;

	try
	{
		library = LoadLibraryW(pluginPath.AsNative().c_str());

		if(library == nullptr)
		{
			DWORD error = GetLastError();

#ifdef _DEBUG
			if(error != ERROR_MOD_NOT_FOUND)	// "File not found errors" are annoying.
			{
				TCHAR szBuf[256];
				wsprintf(szBuf, "Warning: encountered problem when loading plugin dll. Error %x: %s", error, GetErrorMessage(error).c_str());
				Reporting::Error(szBuf, "DEBUG: Error when loading plugin dll");
			}
#endif //_DEBUG

			if(error == ERROR_MOD_NOT_FOUND)
			{
				// No point in trying with JBride, either...
				return nullptr;
			}
		}
	} catch(...)
	{
		CVstPluginManager::ReportPlugException(mpt::String::PrintW(L"Exception caught in LoadLibrary (%1)", pluginPath));
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
			effect = pMainProc(MasterCallBack);
		} else
		{
#ifdef VST_LOG
			Log("Entry point not found! (handle=%08X)\n", library);
#endif // VST_LOG
		}
	}

	if(effect == nullptr)
	{
		// Try loading the plugin using JBridge instead.
		FreeLibrary(library);
		library = nullptr;
		effect = JBridge::LoadBridgedPlugin(MasterCallBack, pluginPath);
	}
	return effect;
}


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
VSTPluginLib *CVstPluginManager::AddPlugin(const mpt::PathString &dllPath, bool fromCache, const bool checkFileExistence, std::wstring *const errStr)
//---------------------------------------------------------------------------------------------------------------------------------------------------
{
	const mpt::PathString fileName = dllPath.GetFileName();

	if(checkFileExistence && (PathFileExistsW(dllPath.AsNative().c_str()) == FALSE))
	{
		if(errStr)
		{
			*errStr += L"\nUnable to find ";
			*errStr += dllPath.ToWide();
		}
	}

	// Check if this is already a known plugin.
	for(const_iterator dupePlug = begin(); dupePlug != end(); dupePlug++)
	{
		if(!dllPath.CompareNoCase(dllPath, (**dupePlug).dllPath)) return *dupePlug;
	}

	// Look if the plugin info is stored in the PluginCache
	if(fromCache)
	{
		SettingsContainer & cacheFile = theApp.GetPluginCache();
		const std::string IDs = cacheFile.Read<std::string>(cacheSectionW, fileName.ToWide(), "");

		if(IDs.length() >= 16)
		{
			// Get path from cache file
			mpt::PathString realPath = cacheFile.Read<mpt::PathString>(cacheSection, IDs, MPT_PATHSTRING(""));
			realPath = theApp.RelativePathToAbsolute(realPath);

			if(!realPath.empty() && !dllPath.CompareNoCase(realPath, dllPath))
			{
				VSTPluginLib *plug = new (std::nothrow) VSTPluginLib(dllPath, fileName);
				if(plug == nullptr)
				{
					return nullptr;
				}
				pluginList.push_back(plug);

				// Extract plugin IDs
				for (int i = 0; i < 16; i++)
				{
					VstInt32 n = IDs[i] - '0';
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

				const std::string flagKey = IDs + ".Flags";
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
	theApp.GetSettings().Write<mpt::PathString>("VST Plugins", "FailedPlugin", dllPath);

	AEffect *pEffect;
	HINSTANCE hLib;
	bool validPlug = false;

	VSTPluginLib *plug = new (std::nothrow) VSTPluginLib(dllPath, fileName);
	if(plug == nullptr)
	{
		return nullptr;
	}

	try
	{
		pEffect = LoadPlugin(dllPath, hLib);

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
		CVstPluginManager::ReportPlugException(mpt::String::PrintW(L"Exception while trying to load plugin \"%1\"!\n", plug->libraryName));
	}

	// Now it should be safe to assume that this plugin loaded properly. :)
	theApp.GetSettings().Remove("VST Plugins", "FailedPlugin");

	// If OK, write the information in PluginCache
	if(validPlug)
	{
		pluginList.push_back(plug);
		WriteToCache(*plug);
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
				CVstPluginManager::ReportPlugException(mpt::String::PrintW(L"Exception while trying to release plugin \"%1\"!\n", pFactory->libraryName));
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

	// Find plugin in library
	int match = 0;
	for(const_iterator p = begin(); p != end(); p++)
	{
		VSTPluginLib *plug = *p;
		const bool matchID = (plug->pluginId1 == mixPlugin.Info.dwPluginId1)
			&& (plug->pluginId2 == mixPlugin.Info.dwPluginId2);
		const bool matchName = !mpt::PathString::CompareNoCase(plug->libraryName, mpt::PathString::FromUTF8(mixPlugin.GetLibraryName()));

		if(matchID && matchName)
		{
			pFound = plug;
			break;
		} else if(matchID && match < 2)
		{
			match = 2;
			pFound = plug;
		} else if(matchName && match < 1)
		{
			match = 1;
			pFound = plug;
		}
	}

	if(mixPlugin.Info.dwPluginId1 == kDmoMagic)
	{
		if (!pFound) return false;
		AEffect *pEffect = DmoToVst(*pFound);
		if(pEffect && pEffect->magic == kDmoMagic)
		{
			CVstPlugin *pVstPlug = new (std::nothrow) CVstPlugin(nullptr, *pFound, mixPlugin, *pEffect, sndFile);
			return pVstPlug != nullptr;
		}
	}

	if(!pFound && strcmp(mixPlugin.GetLibraryName(), ""))
	{
		// Try finding the plugin DLL in the plugin directory or plugin cache instead.
		mpt::PathString fullPath = TrackerDirectories::Instance().GetDefaultDirectory(DIR_PLUGINS);
		if(fullPath.empty())
		{
			fullPath = theApp.GetAppDirPath() + MPT_PATHSTRING("Plugins\\");
		}
		fullPath += mpt::PathString::FromUTF8(mixPlugin.GetLibraryName()) + MPT_PATHSTRING(".dll");

		pFound = AddPlugin(fullPath);
		if(!pFound)
		{
			// Try plugin cache (search for library name)
			SettingsContainer &cacheFile = theApp.GetPluginCache();
			std::string IDs = cacheFile.Read<std::string>(cacheSectionW, mpt::ToWide(mpt::CharsetUTF8, mixPlugin.GetLibraryName()), "");
			if(IDs.length() >= 16)
			{
				fullPath = cacheFile.Read<mpt::PathString>(cacheSection, IDs, MPT_PATHSTRING(""));
				if(!fullPath.empty())
				{
					fullPath = theApp.RelativePathToAbsolute(fullPath);
					if(PathFileExistsW(fullPath.AsNative().c_str()))
					{
						pFound = AddPlugin(fullPath);
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

		try
		{
			pEffect = LoadPlugin(pFound->dllPath, hLibrary);

			if(pEffect != nullptr && pEffect->dispatcher != nullptr && pEffect->magic == kEffectMagic)
			{
				validPlugin = true;

				const bool oldIsInstrument = pFound->isInstrument;
				const VSTPluginLib::PluginCategory oldCategory = pFound->category;

				GetPluginInformation(pEffect, *pFound);

				if(oldIsInstrument != pFound->isInstrument || oldCategory != pFound->category)
				{
					// Update cached information
					WriteToCache(*pFound);
				}

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
			CVstPluginManager::ReportPlugException(mpt::String::PrintW(L"Exception while trying to create plugin \"%1\"!\n", pFound->libraryName));
		}

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
		CVstPlugin *p = (**pFactory).pPluginsList;
		while (p)
		{
			//rewbs. VSTCompliance: A specific plug has requested indefinite periodic processing time.
			if (p->m_bNeedIdle)
			{
				if (!(p->Dispatch(effIdle, 0, 0, nullptr, 0.0f)))
					p->m_bNeedIdle=false;
			}
			//We need to update all open editors
			if ((p->m_pEditor) && (p->m_pEditor->m_hWnd))
			{
				p->m_pEditor->UpdateParamDisplays();
			}
			//end rewbs. VSTCompliance:

			p = p->m_pNext;
		}
	}

}


void CVstPluginManager::ReportPlugException(LPCSTR format,...)
//------------------------------------------------------------
{
	CHAR cBuf[1024];
	va_list va;
	va_start(va, format);
	wvsprintf(cBuf, format, va);
	Reporting::Notification(cBuf);
#ifdef VST_LOG
	Log(cBuf);
#endif

	//Stop song - TODO: figure out why this causes a kernel hang from IASIO->release();
/*
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
		if (pMainFrm) pMainFrm->StopMod();
*/
	va_end(va);
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
	Log("%s", mpt::ToLocale(msg).c_str());
#endif
}

#endif // NO_VST
