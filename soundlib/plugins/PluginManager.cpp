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
#include "../Sndfile.h"
#include "JBridge.h"


#ifdef VST_USE_ALTERNATIVE_MAGIC		// Pelya's plugin ID fix. Breaks fx presets, so let's avoid it for now.
#include "../../include/zlib/zlib.h"	// For CRC32 calculation (to detect plugins with same UID)
#endif // VST_USE_ALTERNATIVE_MAGIC

char CVstPluginManager::s_szHostProductString[64] = "OpenMPT";
char CVstPluginManager::s_szHostVendorString[64] = "OpenMPT project";
VstIntPtr CVstPluginManager::s_nHostVendorVersion = MptVersion::num;

//#define VST_LOG
#define DMO_LOG

AEffect *DmoToVst(VSTPluginLib &lib);

#ifdef VST_USE_ALTERNATIVE_MAGIC
uint32 CalculateCRC32fromFilename(const char *s)
//----------------------------------------------
{
	char fn[_MAX_PATH];
	mpt::String::Copy(fn, s);
	int f;
	for(f = 0; fn[f] != 0; f++) fn[f] = toupper(fn[f]);
	return LittleEndian(crc32(0, (uint8 *)fn, f));

}
#endif // VST_USE_ALTERNATIVE_MAGIC


VstIntPtr VSTCALLBACK CVstPluginManager::MasterCallBack(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt)
//----------------------------------------------------------------------------------------------------------------------------------------------
{
	CVstPluginManager *that = theApp.GetPluginManager();
	if (that)
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
	m_pVstHead = nullptr;
	CSoundFile::gpMixPluginCreateProc = CreateMixPluginProc;
	EnumerateDirectXDMOs();
}


CVstPluginManager::~CVstPluginManager()
//-------------------------------------
{
	CSoundFile::gpMixPluginCreateProc = nullptr;
	while (m_pVstHead)
	{
		VSTPluginLib *p = m_pVstHead;
		m_pVstHead = m_pVstHead->pNext;
		if (m_pVstHead) m_pVstHead->pPrev = nullptr;
		p->pPrev = p->pNext = nullptr;
		while ((volatile void *)(p->pPluginsList) != nullptr)
		{
			p->pPluginsList->Release();
		}
		delete p;
	}
}


bool CVstPluginManager::IsValidPlugin(const VSTPluginLib *pLib)
//-------------------------------------------------------------
{
	VSTPluginLib *p = m_pVstHead;
	while (p)
	{
		if (p == pLib) return true;
		p = p->pNext;
	}
	return false;
}


void CVstPluginManager::EnumerateDirectXDMOs()
//--------------------------------------------
{
	HKEY hkEnum;
	WCHAR keyname[128];

	LONG cr = RegOpenKey(HKEY_LOCAL_MACHINE, "software\\classes\\DirectShow\\MediaObjects\\Categories\\f3602b3f-0592-48df-a4cd-674721e7ebeb", &hkEnum);
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
						StringFromGUID2(clsid, keyname, 100);

						VSTPluginLib *p = new VSTPluginLib(mpt::PathString::FromNative(keyname), mpt::PathString::FromWide(name));
						p->pNext = m_pVstHead;
						p->pluginId1 = kDmoMagic;
						p->pluginId2 = clsid.Data1;
						p->category = VSTPluginLib::catDMO;
#ifdef DMO_LOG
						Log("Found \"%s\" clsid=%s\n", p->libraryName.AsNative().c_str(), p->dllPath.AsNative().c_str());
#endif
						if (m_pVstHead) m_pVstHead->pPrev = p;
						m_pVstHead = p;
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
void GetPluginInformation(AEffect *effect, VSTPluginLib &library)
//---------------------------------------------------------------
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


//
// PluginCache format:
// LibraryName = ID100000ID200000
// ID100000ID200000 = FullDllPath
// ID100000ID200000.Flags = Plugin Flags (isInstrument + category).

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

	VSTPluginLib *pDup = m_pVstHead;
	while(pDup != nullptr)
	{
		if(!dllPath.CompareNoCase(dllPath, pDup->dllPath)) return pDup;
		pDup = pDup->pNext;
	}
	// Look if the plugin info is stored in the PluginCache
	if(fromCache)
	{
		SettingsContainer & cacheFile = theApp.GetPluginCache();
		const char *const cacheSection = "PluginCache";
		const wchar_t *const cacheSectionW = L"PluginCache";
		const std::string IDs = cacheFile.Read<std::string>(cacheSectionW, fileName.ToWide(), "");

		if(IDs.length() >= 16)
		{
			// Get path from cache file
			mpt::PathString realPath = cacheFile.Read<mpt::PathString>(cacheSection, IDs, MPT_PATHSTRING(""));
			realPath = theApp.RelativePathToAbsolute(realPath);

			if(!realPath.empty() && !dllPath.CompareNoCase(realPath, dllPath))
			{
				VSTPluginLib *p = new (std::nothrow) VSTPluginLib(dllPath, fileName);
				if(p == nullptr)
				{
					return nullptr;
				}
				p->pNext = m_pVstHead;
				if (m_pVstHead) m_pVstHead->pPrev = p;
				m_pVstHead = p;

				// Extract plugin Ids
				for (UINT i=0; i<16; i++)
				{
					UINT n = IDs[i] - '0';
					if (n > 9) n = IDs[i] + 10 - 'A';
					n &= 0x0f;
					if (i < 8)
					{
						p->pluginId1 = (p->pluginId1 << 4) | n;
					} else
					{
						p->pluginId2 = (p->pluginId2 << 4) | n;
					}
				}

				std::string flagKey = mpt::String::Format("%s.Flags", IDs.c_str());
				p->DecodeCacheFlags(cacheFile.Read<int32>(cacheSection, flagKey, 0));

#ifdef VST_USE_ALTERNATIVE_MAGIC
				if( p->pluginId1 == kEffectMagic )
				{
					p->pluginId1 = CalculateCRC32fromFilename(p->libraryName); // Make Plugin ID unique for sure (for VSTs with same UID)
				};
#endif // VST_USE_ALTERNATIVE_MAGIC
#ifdef VST_LOG
				Log("Plugin \"%s\" found in PluginCache\n", p->libraryName);
#endif // VST_LOG
				return p;
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

	VSTPluginLib *p = new (std::nothrow) VSTPluginLib(dllPath, fileName);
	if(p == nullptr)
	{
		return nullptr;
	}
	p->pNext = m_pVstHead;

	try
	{
		pEffect = LoadPlugin(dllPath, hLib);

		if(pEffect != nullptr && pEffect->magic == kEffectMagic && pEffect->dispatcher != nullptr)
		{
			pEffect->dispatcher(pEffect, effOpen, 0, 0, 0, 0);

			if (m_pVstHead) m_pVstHead->pPrev = p;
			m_pVstHead = p;

#ifdef VST_USE_ALTERNATIVE_MAGIC
			p->pluginId1 = CalculateCRC32fromFilename(p->libraryName); // Make Plugin ID unique for sure
#else
			p->pluginId1 = pEffect->magic;
#endif // VST_USE_ALTERNATIVE_MAGIC
			p->pluginId2 = pEffect->uniqueID;

			GetPluginInformation(pEffect, *p);

#ifdef VST_LOG
			int nver = pEffect->dispatcher(pEffect, effGetVstVersion, 0,0, nullptr, 0);
			if (!nver) nver = pEffect->version;
			Log("%-20s: v%d.0, %d in, %d out, %2d programs, %2d params, flags=0x%04X realQ=%d offQ=%d\n",
				p->libraryName, nver,
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
		CVstPluginManager::ReportPlugException(mpt::String::PrintW(L"Exception while trying to load plugin \"%1\"!\n", p->libraryName));
	}

	// Now it should be safe to assume that this plugin loaded properly. :)
	theApp.GetSettings().Write<std::string>("VST Plugins", "FailedPlugin", "");

	// If OK, write the information in PluginCache
	if(validPlug)
	{
		SettingsContainer &cacheFile = theApp.GetPluginCache();
		const char *const cacheSection = "PluginCache";
		const wchar_t *const cacheSectionW = L"PluginCache";
		const std::string IDs = mpt::String::Format("%08X%08X", p->pluginId1, p->pluginId2);
		const std::string flagsKey = mpt::String::Format("%s.Flags", IDs);

		mpt::PathString writePath = dllPath;
		if(theApp.IsPortableMode())
		{
			writePath = theApp.AbsolutePathToRelative(writePath);
		}

		cacheFile.Write<mpt::PathString>(cacheSection, IDs, writePath);
		cacheFile.Write<mpt::PathString>(cacheSection, IDs, dllPath);
		cacheFile.Write<std::string>(cacheSectionW, p->libraryName.ToWide(), IDs);
		cacheFile.Write<int32>(cacheSection, flagsKey, p->EncodeCacheFlags());
	} else
	{
		delete p;
	}

	return (validPlug ? m_pVstHead : nullptr);
}


bool CVstPluginManager::RemovePlugin(VSTPluginLib *pFactory)
//----------------------------------------------------------
{
	VSTPluginLib *p = m_pVstHead;

	while (p)
	{
		if (p == pFactory)
		{
			if (p == m_pVstHead) m_pVstHead = p->pNext;
			if (p->pPrev) p->pPrev->pNext = p->pNext;
			if (p->pNext) p->pNext->pPrev = p->pPrev;
			p->pPrev = p->pNext = nullptr;

			try
			{
				CriticalSection cs;

				while ((volatile void *)(p->pPluginsList) != nullptr)
				{
					p->pPluginsList->Release();
				}
				delete p;
			} catch (...)
			{
				CVstPluginManager::ReportPlugException(mpt::String::PrintW(L"Exception while trying to release plugin \"%1\"!\n", pFactory->libraryName));
			}

			return true;
		}
		p = p->pNext;
	}
	return false;
}


bool CVstPluginManager::CreateMixPlugin(SNDMIXPLUGIN &mixPlugin, CSoundFile &sndFile)
//-----------------------------------------------------------------------------------
{
	VSTPluginLib *pFound = nullptr;

	// Find plugin in library
	VSTPluginLib *p = m_pVstHead;
	int match = 0;
	while(p)
	{
		const bool matchID = (p->pluginId1 == mixPlugin.Info.dwPluginId1)
			&& (p->pluginId2 == mixPlugin.Info.dwPluginId2);
		const bool matchName = !mpt::strnicmp(p->libraryName.ToLocale().c_str(), mixPlugin.GetLibraryName(), CountOf(mixPlugin.Info.szLibraryName));

		if(matchID && matchName)
		{
			pFound = p;
			break;
		} else if(matchID && match < 2)
		{
			match = 2;
			pFound = p;
		} else if(matchName && match < 1)
		{
			match = 1;
			pFound = p;
		}
		p = p->pNext;
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
		// Try finding the plugin DLL in the plugin directory instead.
		mpt::PathString fullPath;
		fullPath = TrackerDirectories::Instance().GetDefaultDirectory(DIR_PLUGINS);
		if(fullPath.empty())
		{
			fullPath = theApp.GetAppDirPath() + MPT_PATHSTRING("Plugins\\");
		}
		if(!fullPath.HasTrailingSlash())
		{
			fullPath += MPT_PATHSTRING("\\");
		}
		fullPath += mpt::PathString::FromLocale(mixPlugin.GetLibraryName()) + MPT_PATHSTRING(".dll");

		pFound = AddPlugin(fullPath);
		if(!pFound)
		{
			SettingsContainer &cacheFile = theApp.GetPluginCache();
			const char *cacheSection = "PluginCache";
			std::string IDs = cacheFile.Read<std::string>(cacheSection, mixPlugin.GetLibraryName(), "");
			if(IDs.length() >= 16)
			{
				fullPath = cacheFile.Read<mpt::PathString>(cacheSection, IDs, MPT_PATHSTRING(""));
				if(!fullPath.empty()) pFound = AddPlugin(fullPath);
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
					SettingsContainer &cacheFile = theApp.GetPluginCache();
					const char *cacheSection = "PluginCache";
					std::string flagsKey = mpt::String::Format("%08X%08X.Flags", pFound->pluginId1, pFound->pluginId2);
					cacheFile.Write<int32>(cacheSection, flagsKey, pFound->EncodeCacheFlags());
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
	VSTPluginLib *pFactory = m_pVstHead;

	while (pFactory)
	{
		CVstPlugin *p = pFactory->pPluginsList;
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
		pFactory = pFactory->pNext;
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
