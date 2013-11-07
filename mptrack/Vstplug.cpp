/*
 * vstplug.cpp
 * -----------
 * Purpose: Plugin handling (loading and processing plugins)
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include <uuids.h>
#include <dmoreg.h>
#include <shlwapi.h>
#include <medparam.h>
#include "mainfrm.h"
#include "vstplug.h"
#include "VstPresets.h"
#include "moddoc.h"
#include "Sndfile.h"
#include "AbstractVstEditor.h"		//rewbs.defaultPlugGUI
#include "VstEditor.h"				//rewbs.defaultPlugGUI
#include "defaultvsteditor.h"		//rewbs.defaultPlugGUI
#include "../soundlib/MIDIEvents.h"
#include "../common/version.h"
#include "midimappingdialog.h"
#include "../common/StringFixer.h"
#include "MemoryMappedFile.h"
#include "../soundlib/FileReader.h"
#include "../soundlib/plugins/JBridge.h"
#include "../common/mptFstream.h"
#ifdef VST_USE_ALTERNATIVE_MAGIC	//Pelya's plugin ID fix. Breaks fx presets, so let's avoid it for now.
#include "../zlib/zlib.h"			//For CRC32 calculation (to detect plugins with same UID)
#endif // VST_USE_ALTERNATIVE_MAGIC

#ifndef NO_VST

char CVstPluginManager::s_szHostProductString[64] = "OpenMPT";
char CVstPluginManager::s_szHostVendorString[64] = "OpenMPT project";
VstIntPtr CVstPluginManager::s_nHostVendorVersion = MptVersion::num;

//#define VST_LOG
#define DMO_LOG

AEffect *DmoToVst(VSTPluginLib *pLib);

#ifdef VST_USE_ALTERNATIVE_MAGIC
UINT32 CalculateCRC32fromFilename(const char *s)
//----------------------------------------------
{
	char fn[_MAX_PATH];
	mpt::String::Copy(fn, s);
	int f;
	for(f = 0; fn[f] != 0; f++) fn[f] = toupper(fn[f]);
	return LittleEndian(crc32(0, (BYTE *)fn, f));

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
	//m_bNeedIdle = FALSE; //rewbs.VSTCompliance - now member of plugin class
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
	CHAR keyname[128];
	CHAR s[256];
	WCHAR w[100];
	LONG cr;
	DWORD index;

	cr = RegOpenKey(HKEY_LOCAL_MACHINE, "software\\classes\\DirectShow\\MediaObjects\\Categories\\f3602b3f-0592-48df-a4cd-674721e7ebeb", &hkEnum);
	index = 0;
	while (cr == ERROR_SUCCESS)
	{
		if ((cr = RegEnumKey(hkEnum, index, (LPTSTR)keyname, sizeof(keyname))) == ERROR_SUCCESS)
		{
			CLSID clsid;

			wsprintf(s, "{%s}", keyname);
			MultiByteToWideChar(CP_ACP, 0, (LPCSTR)s,-1,(LPWSTR)w,98);
			if (CLSIDFromString(w, &clsid) == S_OK)
			{
				HKEY hksub;

				wsprintf(s, "software\\classes\\DirectShow\\MediaObjects\\%s", keyname);
				if (RegOpenKey(HKEY_LOCAL_MACHINE, s, &hksub) == ERROR_SUCCESS)
				{
					DWORD datatype = REG_SZ;
					DWORD datasize = 64;

					if (ERROR_SUCCESS == RegQueryValueEx(hksub, nullptr, 0, &datatype, (LPBYTE)s, &datasize))
					{
						VSTPluginLib *p = new VSTPluginLib();
						p->pNext = m_pVstHead;
						p->dwPluginId1 = kDmoMagic;
						p->dwPluginId2 = clsid.Data1;
						p->category = VSTPluginLib::catDMO;
						mpt::String::Copy(p->szLibraryName, s);

						StringFromGUID2(clsid, w, 100);
						WideCharToMultiByte(CP_ACP, 0, w, -1, p->szDllPath, sizeof(p->szDllPath), nullptr, nullptr);
						mpt::String::SetNullTerminator(p->szDllPath);
					#ifdef DMO_LOG
						Log("Found \"%s\" clsid=%s\n", p->szLibraryName, p->szDllPath);
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


void CVstPluginManager::LoadPlugin(const char *pluginPath, AEffect *&effect, HINSTANCE &library)
//----------------------------------------------------------------------------------------------
{
	library = nullptr;
	effect = nullptr;

	try
	{
		library = LoadLibrary(pluginPath);

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
				return;
			}
		}
	} catch(...)
	{
		CVstPluginManager::ReportPlugException("Exception caught in LoadLibrary (%s)", pluginPath);
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

VSTPluginLib *CVstPluginManager::AddPlugin(LPCSTR pszDllPath, bool fromCache, const bool checkFileExistence, CString *const errStr)
//---------------------------------------------------------------------------------------------------------------------------------
{
	TCHAR szPath[_MAX_PATH];

	if(checkFileExistence && (PathFileExists(pszDllPath) == FALSE))
	{
		if(errStr)
		{
			*errStr += "\nUnable to find ";
			*errStr += pszDllPath;
		}
	}

	VSTPluginLib *pDup = m_pVstHead;
	while(pDup != nullptr)
	{
		if (!lstrcmpi(pszDllPath, pDup->szDllPath)) return pDup;
		pDup = pDup->pNext;
	}
	// Look if the plugin info is stored in the PluginCache
	if(fromCache)
	{
		const CString cacheSection = "PluginCache";
		const CString cacheFile = theApp.GetPluginCacheFileName();
		char fileName[_MAX_PATH];
		_splitpath(pszDllPath, nullptr, nullptr, fileName, nullptr);

		CString IDs = CMainFrame::GetPrivateProfileCString(cacheSection, fileName, "", cacheFile);

		if (IDs.GetLength() >= 16)
		{
			// Get path from cache file
			GetPrivateProfileString(cacheSection, IDs, "", szPath, CountOf(szPath), cacheFile);
			mpt::String::SetNullTerminator(szPath);
			theApp.RelativePathToAbsolute(szPath);

			if ((szPath[0]) && (!lstrcmpi(szPath, pszDllPath)))
			{
				VSTPluginLib *p = new (std::nothrow) VSTPluginLib(pszDllPath);
				if(p == nullptr)
				{
					return nullptr;
				}
				_splitpath(pszDllPath, nullptr, nullptr, p->szLibraryName, nullptr);
				p->szLibraryName[63] = '\0';
				p->pNext = m_pVstHead;
				if (m_pVstHead) m_pVstHead->pPrev = p;
				m_pVstHead = p;

				// Extract plugin Ids
				for (UINT i=0; i<16; i++)
				{
					UINT n = ((LPCTSTR)IDs)[i] - '0';
					if (n > 9) n = ((LPCTSTR)IDs)[i] + 10 - 'A';
					n &= 0x0f;
					if (i < 8)
					{
						p->dwPluginId1 = (p->dwPluginId1 << 4) | n;
					} else
					{
						p->dwPluginId2 = (p->dwPluginId2 << 4) | n;
					}
				}

				CString flagKey;
				flagKey.Format("%s.Flags", IDs);
				p->DecodeCacheFlags(CMainFrame::GetPrivateProfileLong(cacheSection, flagKey, 0, cacheFile));

			#ifdef VST_USE_ALTERNATIVE_MAGIC
				if( p->dwPluginId1 == kEffectMagic )
				{
					p->dwPluginId1 = CalculateCRC32fromFilename(p->szLibraryName); // Make Plugin ID unique for sure (for VSTs with same UID)
				};
			#endif // VST_USE_ALTERNATIVE_MAGIC
			#ifdef VST_LOG
				Log("Plugin \"%s\" found in PluginCache\n", p->szLibraryName);
			#endif // VST_LOG
				return p;
			} else
			{
			#ifdef VST_LOG
				Log("Plugin \"%s\" mismatch in PluginCache: \"%s\" [%s]=\"%s\"\n", s, pszDllPath, (LPCTSTR)IDs, (LPCTSTR)strFullPath);
			#endif // VST_LOG
			}
		}
	}

	// If this key contains a file name on program launch, a plugin previously crashed OpenMPT.
	WritePrivateProfileString("VST Plugins", "FailedPlugin", pszDllPath, theApp.GetConfigFileName());

	AEffect *pEffect;
	HINSTANCE hLib;
	bool validPlug = false;

	VSTPluginLib *p = new (std::nothrow) VSTPluginLib(pszDllPath);
	if(p == nullptr)
	{
		return nullptr;
	}
	_splitpath(pszDllPath, nullptr, nullptr, p->szLibraryName, nullptr);
	p->szLibraryName[63] = 0;
	p->pNext = m_pVstHead;

	try
	{
		LoadPlugin(pszDllPath, pEffect, hLib);

		if(pEffect != nullptr && pEffect->magic == kEffectMagic && pEffect->dispatcher != nullptr)
		{
			pEffect->dispatcher(pEffect, effOpen, 0, 0, 0, 0);

			if (m_pVstHead) m_pVstHead->pPrev = p;
			m_pVstHead = p;

#ifdef VST_USE_ALTERNATIVE_MAGIC
			p->dwPluginId1 = CalculateCRC32fromFilename(p->szLibraryName); // Make Plugin ID unique for sure
#else
			p->dwPluginId1 = pEffect->magic;
#endif // VST_USE_ALTERNATIVE_MAGIC
			p->dwPluginId2 = pEffect->uniqueID;

			GetPluginInformation(pEffect, *p);

#ifdef VST_LOG
			int nver = pEffect->dispatcher(pEffect, effGetVstVersion, 0,0, nullptr, 0);
			if (!nver) nver = pEffect->version;
			Log("%-20s: v%d.0, %d in, %d out, %2d programs, %2d params, flags=0x%04X realQ=%d offQ=%d\n",
				p->szLibraryName, nver,
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
		CVstPluginManager::ReportPlugException("Exception while trying to load plugin \"%s\"!\n", p->szLibraryName);
	}

	// Now it should be safe to assume that this plugin loaded properly. :)
	WritePrivateProfileString("VST Plugins", "FailedPlugin", nullptr, theApp.GetConfigFileName());

	// If OK, write the information in PluginCache
	if(validPlug)
	{
		const CString cacheSection = "PluginCache";
		const CString cacheFile = theApp.GetPluginCacheFileName();
		CString IDs, flagsKey;
		IDs.Format("%08X%08X", p->dwPluginId1, p->dwPluginId2);
		flagsKey.Format("%s.Flags", IDs);

		_tcsncpy(szPath, pszDllPath, CountOf(szPath) - 1);
		if(theApp.IsPortableMode())
		{
			theApp.AbsolutePathToRelative(szPath);
		}
		mpt::String::SetNullTerminator(szPath);

		WritePrivateProfileString(cacheSection, IDs, szPath, cacheFile);
		CMainFrame::WritePrivateProfileCString(cacheSection, IDs, pszDllPath, cacheFile);
		CMainFrame::WritePrivateProfileCString(cacheSection, p->szLibraryName, IDs, cacheFile);
		CMainFrame::WritePrivateProfileLong(cacheSection, flagsKey, p->EncodeCacheFlags(), cacheFile);
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
				CVstPluginManager::ReportPlugException("Exception while trying to release plugin \"%s\"!\n", pFactory->szLibraryName);
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
	UINT nMatch = 0;
	VSTPluginLib *pFound = nullptr;

	VSTPluginLib *p = m_pVstHead;

	while (p)
	{
		bool b1 = false, b2 = false;
		if ((p->dwPluginId1 == mixPlugin.Info.dwPluginId1)
			&& (p->dwPluginId2 == mixPlugin.Info.dwPluginId2))
		{
			b1 = true;
		}
		if (!mpt::strnicmp(p->szLibraryName, mixPlugin.GetLibraryName(), 64))
		{
			b2 = true;
		}
		if ((b1) && (b2))
		{
			nMatch = 3;
			pFound = p;
		} else
		if ((b1) && (nMatch < 2))
		{
			nMatch = 2;
			pFound = p;
		} else
		if ((b2) && (nMatch < 1))
		{
			nMatch = 1;
			pFound = p;
		}
		p = p->pNext;
	}

	if (mixPlugin.Info.dwPluginId1 == kDmoMagic)
	{
		if (!pFound) return FALSE;
		AEffect *pEffect = DmoToVst(pFound);
		if ((pEffect) && (pEffect->dispatcher) && (pEffect->magic == kDmoMagic))
		{
			bool result = false;
			CVstPlugin *pVstPlug = new (std::nothrow) CVstPlugin(NULL, *pFound, mixPlugin, *pEffect, sndFile);
			if(pVstPlug)
			{
				result = true;
			}
			return result;
		}
	}

	if ((!pFound) && strcmp(mixPlugin.GetLibraryName(), ""))
	{
		// Try finding the plugin DLL in the plugin directory instead.
		CHAR s[_MAX_PATH];
		mpt::String::CopyN(s, TrackerSettings::Instance().GetDefaultDirectory(DIR_PLUGINS));
		if(!s[0])
		{
			mpt::String::CopyN(s, theApp.GetAppDirPath());
		}
		size_t len = strlen(s);
		if((len > 0) && (s[len - 1] != '\\') && (s[len - 1] != '/'))
		{
			strcat(s, "\\");
		}
		strncat(s, mixPlugin.GetLibraryName(), CountOf(s));
		strncat(s, ".dll", CountOf(s));

		mpt::String::SetNullTerminator(s);

		pFound = AddPlugin(s);
		if (!pFound)
		{
			CString cacheSection = "PluginCache";
			CString cacheFile = theApp.GetPluginCacheFileName();
			CString IDs = CMainFrame::GetPrivateProfileCString(cacheSection, mixPlugin.GetLibraryName(), "", cacheFile);
			if (IDs.GetLength() >= 16)
			{
				CString strFullPath = CMainFrame::GetPrivateProfileCString(cacheSection, IDs, "", cacheFile);
				if ((strFullPath) && (strFullPath[0])) pFound = AddPlugin(strFullPath);
			}
		}
	}

	if (pFound)
	{

		AEffect *pEffect = nullptr;
		HINSTANCE hLibrary = nullptr;
		bool validPlugin = false;

		try
		{
			LoadPlugin(pFound->szDllPath, pEffect, hLibrary);

			if(pEffect != nullptr && pEffect->dispatcher != nullptr && pEffect->magic == kEffectMagic)
			{
				validPlugin = true;

				const bool oldIsInstrument = pFound->isInstrument;
				const VSTPluginLib::PluginCategory oldCategory = pFound->category;

				GetPluginInformation(pEffect, *pFound);

				if(oldIsInstrument != pFound->isInstrument || oldCategory != pFound->category)
				{
					// Update cached information
					CString cacheSection = "PluginCache";
					CString cacheFile = theApp.GetPluginCacheFileName();
					CString flagsKey;
					flagsKey.Format("%08X%08X.Flags", pFound->dwPluginId1, pFound->dwPluginId2);
					CMainFrame::WritePrivateProfileLong(cacheSection, flagsKey, pFound->EncodeCacheFlags(), cacheFile);
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
			CVstPluginManager::ReportPlugException("Exception while trying to create plugin \"%s\"!\n", pFound->szLibraryName);
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

//rewbs.VSTCompliance: Added support for lots of opcodes
VstIntPtr CVstPluginManager::VstCallback(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float /*opt*/)
//-----------------------------------------------------------------------------------------------------------------------------------
{
	#ifdef VST_LOG
	Log("VST plugin to host: Eff: 0x%.8X, Opcode = %d, Index = %d, Value = %d, PTR = %.8X, OPT = %.3f\n",(int)effect, opcode,index,value,(int)ptr,opt);
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
		if (pVstPlugin != nullptr && pVstPlugin->CanAutomateParameter(index))
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
		OnIdle();
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
				timeInfo.samplePos = sndFile.GetTotalSampleCount();
				if(sndFile.HasPositionChanged())
				{
					timeInfo.flags |= kVstTransportChanged;
				}
			} else
			{
				timeInfo.flags |= kVstTransportChanged; //just stopped.
				timeInfo.samplePos = 0;
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
				timeInfo.timeSigNumerator = sndFile.m_nCurrentRowsPerMeasure / MAX(sndFile.m_nCurrentRowsPerBeat, 1);
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
				pVstEditor->SetSize(index, value);
			}
		}
		Log("VST plugin to host: Size Window\n");
		return 1;

	case audioMasterGetSampleRate:
		if(pVstPlugin)
		{
			return pVstPlugin->m_nSampleRate;
		}

	case audioMasterGetBlockSize:
		return MIXBUFFERSIZE;

	case audioMasterGetInputLatency:
		Log("VST plugin to host: Get Input Latency\n");
		break;

	case audioMasterGetOutputLatency:
		if(pVstPlugin)
		{
			return Util::muldiv(TrackerSettings::Instance().m_LatencyMS, pVstPlugin->m_nSampleRate, 1000);
		}

	// input pin in <value> (-1: first to come), returns cEffect* - DEPRECATED in VST 2.4
	case audioMasterGetPreviousPlug:
		if(pVstPlugin != nullptr)
		{
			std::vector<CVstPlugin *> list;
			if(pVstPlugin->GetInputPlugList(list) != 0)
			{
				// We don't assign plugins to pins...
				return ToVstPtr(&list[0]->m_Effect);
			}
		}
		break;

	// output pin in <value> (-1: first to come), returns cEffect* - DEPRECATED in VST 2.4
	case audioMasterGetNextPlug:
		if(pVstPlugin != nullptr)
		{
			std::vector<CVstPlugin *> list;
			if(pVstPlugin->GetOutputPlugList(list) != 0)
			{
				// We don't assign plugins to pins...
				return ToVstPtr(&list[0]->m_Effect);
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
		strcpy((char *) ptr, s_szHostVendorString);
		return 1;

	case audioMasterGetProductString:
		strcpy((char *) ptr, s_szHostProductString);
		return 1;

	case audioMasterGetVendorVersion:
		return s_nHostVendorVersion;

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
		return ToVstPtr(TrackerSettings::Instance().GetDefaultDirectory(DIR_PLUGINS));

	// something has changed, update 'multi-fx' display
	case audioMasterUpdateDisplay:
		if(pVstPlugin != nullptr)
		{
			// Note to self for testing: Electri-Q sends opcode. Korg M1 sends this when switching between Combi and Multi mode to update the preset names.
			CAbstractVstEditor *pVstEditor = pVstPlugin->GetEditor();
			if(pVstEditor && ::IsWindow(pVstEditor->m_hWnd))
			{
				pVstEditor->SetupMenu(true);
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
		return VstFileSelector(opcode == audioMasterCloseFileSelector, static_cast<VstFileSelect *>(ptr), effect);

	// open an editor for audio (defined by XML text in ptr) - DEPRECATED in VST 2.4
	case audioMasterEditFile:
		Log("VST plugin to host: Edit File\n");
		break;

	// get the native path of currently loading bank or project
	// (called from writeChunk) void* in <ptr> (char[2048], or sizeof(FSSpec)) - DEPRECATED in VST 2.4
	case audioMasterGetChunkFile:
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
//end rewbs. VSTCompliance:


// Helper function for file selection dialog stuff.
VstIntPtr CVstPluginManager::VstFileSelector(bool destructor, VstFileSelect *fileSel, const AEffect *effect)
//----------------------------------------------------------------------------------------------------------
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

			if(fileSel->initialPath != nullptr)
			{
				workingDir = fileSel->initialPath;
			} else
			{
				// Plugins are probably looking for presets...?
				workingDir = ""; //TrackerSettings::Instance().GetWorkingDirectory(DIR_PLUGINPRESETS);
			}

			FileDlgResult files = CTrackApp::ShowOpenSaveFileDialog(
				(fileSel->command != kVstFileSave),
				"", "", extensions, workingDir,
				(fileSel->command == kVstMultipleFilesLoad)
				);

			if(files.abort)
			{
				return 0;
			}

			if(fileSel->command == kVstMultipleFilesLoad)
			{
				// Multiple paths
				fileSel->nbReturnPath = files.filenames.size();
				fileSel->returnMultiplePaths = new char *[fileSel->nbReturnPath];
				for(size_t i = 0; i < files.filenames.size(); i++)
				{
					char *fname = new char[files.filenames[i].length() + 1];
					strcpy(fname, files.filenames[i].c_str());
					fileSel->returnMultiplePaths[i] = fname;
				}
				return 1;
			} else
			{
				// Single path

				// VOPM doesn't initialize required information properly (it doesn't memset the struct to 0)...
				if(CCONST('V', 'O', 'P', 'M') == effect->uniqueID)
				{
					fileSel->sizeReturnPath = _MAX_PATH;
				}

				if(fileSel->returnPath == nullptr || fileSel->sizeReturnPath == 0)
				{

					// Provide some memory for the return path.
					fileSel->sizeReturnPath = files.first_file.length() + 1;
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
				strncpy(fileSel->returnPath, files.first_file.c_str(), fileSel->sizeReturnPath - 1);
				fileSel->nbReturnPath = 1;
				fileSel->returnMultiplePaths = nullptr;
			}
			return 1;

		} else
		{
			// Plugin wants a directory

			char szInitPath[_MAX_PATH] = { '\0' };
			if(fileSel->initialPath)
			{
				mpt::String::CopyN(szInitPath, fileSel->initialPath);
			}

			char szBuffer[_MAX_PATH];
			MemsetZero(szBuffer);

			BROWSEINFO bi;
			MemsetZero(bi);
			bi.hwndOwner = CMainFrame::GetMainFrame()->m_hWnd;
			bi.lpszTitle = fileSel->title;
			bi.pszDisplayName = szInitPath;
			bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
			LPITEMIDLIST pid = SHBrowseForFolder(&bi);
			if(pid != nullptr && SHGetPathFromIDList(pid, szBuffer))
			{
				if(CCONST('V', 'S', 'T', 'r') == effect->uniqueID && fileSel->returnPath != nullptr && fileSel->sizeReturnPath == 0)
				{
					// old versions of reViSiT (which still relied on the host's file selection code) seem to be dodgy.
					// They report a path size of 0, but when using an own buffer, they will crash.
					// So we'll just assume that reViSiT can handle long enough (_MAX_PATH) paths here.
					fileSel->sizeReturnPath = strlen(szBuffer) + 1;
					fileSel->returnPath[fileSel->sizeReturnPath - 1] = '\0';
				}
				if(fileSel->returnPath == nullptr || fileSel->sizeReturnPath == 0)
				{
					// Provide some memory for the return path.
					fileSel->sizeReturnPath = strlen(szBuffer) + 1;
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
				strncpy(fileSel->returnPath, szBuffer, fileSel->sizeReturnPath - 1);
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
/*	END_CRITICAL();
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
		if (pMainFrm) pMainFrm->StopMod();
*/
	va_end(va);
}


//////////////////////////////////////////////////////////////////////////////
//
// CVstPlugin
//

CVstPlugin::CVstPlugin(HMODULE hLibrary, VSTPluginLib &factory, SNDMIXPLUGIN &mixStruct, AEffect &effect, CSoundFile &sndFile) : m_SndFile(sndFile), m_Factory(factory), m_Effect(effect)
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
{
	m_hLibrary = hLibrary;
	m_nRefCount = 1;
	m_pPrev = nullptr;
	m_pNext = nullptr;
	m_pMixStruct = &mixStruct;
	m_pEditor = nullptr;
	m_nInputs = m_nOutputs = 0;
	m_nEditorX = m_nEditorY = -1;
	m_pProcessFP = nullptr;

	m_MixState.dwFlags = 0;
	m_MixState.nVolDecayL = 0;
	m_MixState.nVolDecayR = 0;
	m_MixState.pMixBuffer = (int *)((((intptr_t)m_MixBuffer) + 7) & ~7);
	m_MixState.pOutBufferL = mixBuffer.GetInputBuffer(0);
	m_MixState.pOutBufferR = mixBuffer.GetInputBuffer(1);

	m_bSongPlaying = false; //rewbs.VSTCompliance
	m_bPlugResumed = false;
	m_nSampleRate = uint32_max;

	MemsetZero(m_MidiCh);
	for(int ch = 0; ch < 16; ch++)
	{
		m_MidiCh[ch].midiPitchBendPos = EncodePitchBendParam(MIDIEvents::pitchBendCentre); // centre pitch bend on all channels
	}

	// Update Mix structure
	m_pMixStruct->pMixState = &m_MixState;
	// Open plugin and initialize data structures
	Initialize();
	// Now we should be ready to go
	m_pMixStruct->pMixPlugin = this;

	// Insert ourselves in the beginning of the list
	m_pNext = m_Factory.pPluginsList;
	if(m_Factory.pPluginsList)
	{
		m_Factory.pPluginsList->m_pPrev = this;
	}
	m_Factory.pPluginsList = this;
}


void CVstPlugin::Initialize()
//---------------------------
{
	m_bNeedIdle = false;
	m_bRecordAutomation = false;
	m_bPassKeypressesToPlug = false;
	m_bRecordMIDIOut = false;

	//rewbs.VSTcompliance
	//Store a pointer so we can get the CVstPlugin object from the basic VST effect object.
	m_Effect.resvd1 = ToVstPtr(this);
	m_nSlot = FindSlot();

	Dispatch(effOpen, 0, 0, nullptr, 0.0f);
	// VST 2.0 plugins return 2 here, VST 2.4 plugins return 2400... Great!
	m_bIsVst2 = Dispatch(effGetVstVersion, 0,0, nullptr, 0.0f) >= 2;
	if (m_bIsVst2)
	{
		// Set VST speaker in/out setup to Stereo. Required for some plugins (possibly all VST 2.4+ plugins?)
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

	m_nSampleRate = m_SndFile.GetSampleRate();
	Dispatch(effSetSampleRate, 0, 0, nullptr, static_cast<float>(m_SndFile.GetSampleRate()));
	Dispatch(effSetBlockSize, 0, MIXBUFFERSIZE, nullptr, 0.0f);
	if(m_Effect.numPrograms > 0)
	{
		Dispatch(effBeginSetProgram, 0, 0, nullptr, 0);
		Dispatch(effSetProgram, 0, 0, nullptr, 0);
		Dispatch(effEndSetProgram, 0, 0, nullptr, 0);
	}

	InitializeIOBuffers();

	Dispatch(effSetProcessPrecision, 0, kVstProcessPrecision32, nullptr, 0.0f);

#ifdef VST_LOG
	Log("%s: vst ver %d.0, flags=%04X, %d programs, %d parameters\n",
		m_Factory.szLibraryName, (m_bIsVst2) ? 2 : 1, m_Effect.flags,
		m_Effect.numPrograms, m_Effect.numParams);
#endif

	m_bIsInstrument = isInstrument();
	RecalculateGain();
	m_pProcessFP = (m_Effect.flags & effFlagsCanReplacing) ? m_Effect.processReplacing : m_Effect.process;

	// issue samplerate again here, cos some plugs like it before the block size, other like it right at the end.
	Dispatch(effSetSampleRate, 0, 0, nullptr, static_cast<float>(m_SndFile.GetSampleRate()));

	// Korg Wavestation GUI won't work until plugin was resumed at least once.
	// On the other hand, some other plugins (notably Synthedit plugins like Superwave P8 2.3 or Rez 3.0) don't like this
	// and won't load their stored plugin data instantly, so only do this for the troublesome plugins...
	// Also apply this fix for Korg's M1 plugin, as this will fixes older versions of said plugin, newer versions don't require the fix.
	// EZDrummer won't load its samples until playback has started.
	if(GetUID() == CCONST('K', 'L', 'W', 'V')			// Wavestation
		|| GetUID() == CCONST('K', 'L', 'M', '1')		// M1
		|| GetUID() == CCONST('d', 'f', 'h', 'e'))		// EZDrummer
	{
		Resume();
		Suspend();
	}
}


bool CVstPlugin::InitializeIOBuffers()
//------------------------------------
{
	m_nInputs = m_Effect.numInputs;
	m_nOutputs = m_Effect.numOutputs;

	// Input pointer array size must be >= 2 for now - the input buffer assignment might write to non allocated mem. otherwise
	bool result = mixBuffer.Initialize(MAX(m_nInputs, 2), m_nOutputs);
	m_MixState.pOutBufferL = mixBuffer.GetInputBuffer(0);
	m_MixState.pOutBufferR = mixBuffer.GetInputBuffer(1);

	return result;
}


CVstPlugin::~CVstPlugin()
//-----------------------
{
#ifdef VST_LOG
	Log("~CVstPlugin: m_nRefCount=%d\n", m_nRefCount);
#endif
	CriticalSection cs;

	// First thing to do, if we don't want to hang in a loop
	if (m_Factory.pPluginsList == this) m_Factory.pPluginsList = m_pNext;
	if (m_pMixStruct)
	{
		m_pMixStruct->pMixPlugin = nullptr;
		m_pMixStruct->pMixState = nullptr;
		m_pMixStruct = nullptr;
	}
	if (m_pNext) m_pNext->m_pPrev = m_pPrev;
	if (m_pPrev) m_pPrev->m_pNext = m_pNext;
	m_pPrev = nullptr;
	m_pNext = nullptr;
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
	CVstPlugin::Dispatch(effClose, 0, 0, nullptr, 0);
	if(m_hLibrary)
	{
		FreeLibrary(m_hLibrary);
		m_hLibrary = nullptr;
	}

}


size_t CVstPlugin::Release()
//--------------------------
{
	if(!(--m_nRefCount))
	{
		try
		{
			delete this;
		} catch (...)
		{
			CVstPluginManager::ReportPlugException("Exception while destroying plugin!\n");
		}

		return 0;
	}
	return m_nRefCount;
}


void CVstPlugin::GetPluginType(LPSTR pszType)
//-------------------------------------------
{
	pszType[0] = 0;
	if (m_Effect.numInputs < 1) strcpy(pszType, "No input"); else
	if (m_Effect.numInputs == 1) strcpy(pszType, "Mono-In"); else
	strcpy(pszType, "Stereo-In");
	strcat(pszType, ", ");
	if (m_Effect.numOutputs < 1) strcat(pszType, "No output"); else
	if (m_Effect.numInputs == 1) strcat(pszType, "Mono-Out"); else
	strcat(pszType, "Stereo-Out");
}


bool CVstPlugin::HasEditor()
//--------------------------
{
	return (m_Effect.flags & effFlagsHasEditor) != 0;
}


VstInt32 CVstPlugin::GetNumPrograms()
//-----------------------------------
{
	return std::max(m_Effect.numPrograms, VstInt32(0));
}


PlugParamIndex CVstPlugin::GetNumParameters()
//-------------------------------------------
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


bool CVstPlugin::GetParams(float *param, VstInt32 min, VstInt32 max)
//------------------------------------------------------------------
{
	LimitMax(max, m_Effect.numParams);

	for(VstInt32 p = min; p < max; p++)
		param[p - min] = GetParameter(p);

	return true;

}


bool CVstPlugin::RandomizeParams(PlugParamIndex minParam, PlugParamIndex maxParam)
//--------------------------------------------------------------------------------
{
	if (minParam == 0 && maxParam == 0)
	{
		maxParam = m_Effect.numParams;

	}
	LimitMax(maxParam, PlugParamIndex(m_Effect.numParams));

	for(PlugParamIndex p = minParam; p < maxParam; p++)
	{
		SetParameter(p, (float(rand()) / float(RAND_MAX)));
	}

	return true;
}


bool CVstPlugin::SaveProgram()
//----------------------------
{
	std::string defaultDir = TrackerSettings::Instance().GetWorkingDirectory(DIR_PLUGINPRESETS);
	bool useDefaultDir = !defaultDir.empty();
	if(!useDefaultDir)
	{
		defaultDir = m_Factory.szDllPath;
		defaultDir = defaultDir.substr(0, defaultDir.find_last_of("\\/"));
	}

	char rawname[MAX(kVstMaxProgNameLen + 1, 256)] = "";	// kVstMaxProgNameLen is 24...
	Dispatch(effGetProgramName, 0, 0, rawname, 0);
	SanitizeFilename(rawname);
	mpt::String::SetNullTerminator(rawname);
	FileDlgResult files = CTrackApp::ShowOpenSaveFileDialog(false, "fxp", rawname,
		"VST Plugin Programs (*.fxp)|*.fxp|"
		"VST Plugin Banks (*.fxb)|*.fxb||",
		defaultDir);
	if(files.abort) return false;

	if(useDefaultDir)
	{
		TrackerSettings::Instance().SetWorkingDirectory(files.workingDirectory.c_str(), DIR_PLUGINPRESETS, true);
	}

	bool bank = !mpt::strnicmp(files.first_file.substr(files.first_file.length() - 3).c_str(), "fxb", 3);

	mpt::fstream f(files.first_file.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);
	if(f.good() && VSTPresets::SaveFile(f, *this, bank))
	{
		return true;
	} else
	{
		Reporting::Error("Error saving preset.");
		return false;
	}

}


bool CVstPlugin::LoadProgram()
//----------------------------
{
	std::string defaultDir = TrackerSettings::Instance().GetWorkingDirectory(DIR_PLUGINPRESETS);
	bool useDefaultDir = !defaultDir.empty();
	if(!useDefaultDir)
	{
		defaultDir = m_Factory.szDllPath;
		defaultDir = defaultDir.substr(0, defaultDir.find_last_of("\\/"));
	}

	FileDlgResult files = CTrackApp::ShowOpenSaveFileDialog(true, "fxp", "",
		"VST Plugin Programs and Banks (*.fxp,*.fxb)|*.fxp;*.fxb|"
		"VST Plugin Programs (*.fxp)|*.fxp|"
		"VST Plugin Banks (*.fxb)|*.fxb|"
		"All Files|*.*||",
		defaultDir);
	if(files.abort) return false;

	if(useDefaultDir)
	{
		TrackerSettings::Instance().SetWorkingDirectory(files.workingDirectory.c_str(), DIR_PLUGINPRESETS, true);
	}

	CMappedFile f;
	const char *errorStr = nullptr;
	if(f.Open(files.first_file.c_str()))
	{
		FileReader file = f.GetFile();
		errorStr = VSTPresets::GetErrorMessage(VSTPresets::LoadFile(file, *this));
	} else
	{
		errorStr = "Can't open file.";
	}

	if(errorStr == nullptr)
	{
		if(GetModDoc() != nullptr && GetSoundFile().GetModSpecifications().supportsPlugins)
		{
			GetModDoc()->SetModified();
		}
		return true;
	} else
	{
		Reporting::Error(errorStr);
		return false;
	}
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
			Log("About to Dispatch(%d) (Plugin=\"%s\"), index: %d, value: %d, value: %h, value: %f!\n", opCode, m_Factory.szLibraryName, index, value, ptr, opt);
			#endif
			result = m_Effect.dispatcher(&m_Effect, opCode, index, value, ptr, opt);
		}
	} catch (...)
	{
		CVstPluginManager::ReportPlugException("Exception in Dispatch(%d) (Plugin=\"%s\")!\n", opCode, m_Factory.szLibraryName);
	}

	return result;
}


VstInt32 CVstPlugin::GetCurrentProgram()
//--------------------------------------
{
	if(m_Effect.numPrograms > 0)
	{
		return Dispatch(effGetProgram, 0, 0, nullptr, 0);
	}
	return 0;
}


bool CVstPlugin::GetProgramNameIndexed(VstInt32 index, VstIntPtr category, char *text)
//------------------------------------------------------------------------------------
{
	if(m_Effect.numPrograms > 0)
	{
		return (Dispatch(effGetProgramNameIndexed, index, category, text, 0) == 1);
	}
	return false;
}


CString CVstPlugin::GetFormattedProgramName(VstInt32 index)
//---------------------------------------------------------
{
	char rawname[MAX(kVstMaxProgNameLen + 1, 256)];	// kVstMaxProgNameLen is 24...
	if(!GetProgramNameIndexed(index, -1, rawname))
	{
		// Fallback: Try to get current program name.
		strcpy(rawname, "");
		VstInt32 curProg = GetCurrentProgram();
		if(index != curProg)
		{
			SetCurrentProgram(index);
		}
		Dispatch(effGetProgramName, 0, 0, rawname, 0);
		if(index != curProg)
		{
			SetCurrentProgram(curProg);
		}
	}
	mpt::String::SetNullTerminator(rawname);

	// Let's start counting at 1 for the program name (as most MIDI hardware / software does)
	index++;

	CString formattedName;
	if(rawname[0] < ' ')
	{
		formattedName.Format("%02d - Program %d", index, index);
	}
	else
	{
		formattedName.Format("%02d - %s", index, rawname);
	}

	return formattedName;
}


void CVstPlugin::SetCurrentProgram(VstInt32 nIndex)
//-------------------------------------------------
{
	if(m_Effect.numPrograms > 0)
	{
		if(nIndex < m_Effect.numPrograms)
		{
			Dispatch(effBeginSetProgram, 0, 0, nullptr, 0);
			Dispatch(effSetProgram, 0, nIndex, nullptr, 0);
			Dispatch(effEndSetProgram, 0, 0, nullptr, 0);
		}
	}
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
			//CVstPluginManager::ReportPlugException("Exception in getParameter (Plugin=\"%s\")!\n", m_Factory.szLibraryName);
		}
	}
	//rewbs.VSTcompliance
	if (fResult<0.0f)
		return 0.0f;
	else if (fResult>1.0f)
		return 1.0f;
	else
	//end rewbs.VSTcompliance
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
	} catch (...)
	{
		CVstPluginManager::ReportPlugException("Exception in SetParameter(%d, 0.%03d) (Plugin=%s)\n", nIndex, (int)(fValue*1000), m_Factory.szLibraryName);
	}
}


// Helper function for retreiving parameter name / label / display
CString CVstPlugin::GetParamPropertyString(VstInt32 param, VstInt32 opcode)
//-------------------------------------------------------------------------
{
	CHAR s[MAX(kVstMaxParamStrLen + 1, 64)]; // Increased to 64 bytes since 32 bytes doesn't seem to suffice for all plugs. Kind of ridiculous if you consider that kVstMaxParamStrLen = 8...
	s[0] = '\0';

	if(m_Effect.numParams > 0 && param < m_Effect.numParams)
	{
		Dispatch(opcode, param, 0, s, 0);
		mpt::String::SetNullTerminator(s);
	}
	return CString(s);
}


CString CVstPlugin::GetFormattedParamName(PlugParamIndex param)
//-------------------------------------------------------------
{
	CString paramName;

	VstParameterProperties properties;
	MemsetZero(properties.label);
	if(Dispatch(effGetParameterProperties, param, 0, &properties, 0.0f) == 1)
	{
		mpt::String::SetNullTerminator(properties.label);
		paramName = properties.label;
	} else
	{
		paramName = GetParamName(param);
	}

	CString name;
	if(paramName.IsEmpty())
	{
		name.Format("%02d: Parameter %02d", param, param);
	} else
	{
		name.Format("%02d: %s", param, paramName);
	}
	return name;
}


// Get a parameter's current value, represented by the plugin.
CString CVstPlugin::GetFormattedParamValue(PlugParamIndex param)
//--------------------------------------------------------------
{

	CString paramDisplay = GetParamDisplay(param);
	CString paramUnits = GetParamLabel(param);
	paramDisplay.Trim();
	paramUnits.Trim();
	paramDisplay += " " + paramUnits;

	return paramDisplay;
}


BOOL CVstPlugin::GetDefaultEffectName(LPSTR pszName)
//--------------------------------------------------
{
	pszName[0] = 0;
	if(m_bIsVst2)
	{
		Dispatch(effGetEffectName, 0, 0, pszName, 0);
		return TRUE;
	}
	return FALSE;
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
		CVstPluginManager::ReportPlugException("Exception in Resume() (Plugin=%s)\n", m_Factory.szLibraryName);
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
			CVstPluginManager::ReportPlugException("Exception in Suspend() (Plugin=%s)\n", m_Factory.szLibraryName);
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
		} catch (...)
		{
			CVstPluginManager::ReportPlugException("Exception in ProcessVSTEvents() (Plugin=%s, numEvents:%d)\n",
				m_Factory.szLibraryName, vstEvents.GetNumEvents());
		}
	}
}


// Receive events from plugin and send them to the next plugin in the chain.
void CVstPlugin::ReceiveVSTEvents(const VstEvents *events) const
//--------------------------------------------------------------
{
	if(m_pMixStruct == nullptr)
	{
		return;
	}

	// I think we should only route events to plugins that are explicitely specified as output plugins of the current plugin.
	// This should probably use GetOutputPlugList here if we ever get to support multiple output plugins.
	PLUGINDEX receiver = m_pMixStruct->GetOutputPlugin();

	if(receiver != PLUGINDEX_INVALID)
	{
		SNDMIXPLUGIN &mixPlug = m_SndFile.m_MixPlugins[receiver];
		CVstPlugin *vstPlugin = dynamic_cast<CVstPlugin *>(mixPlug.pMixPlugin);
		if(vstPlugin != nullptr)
		{
			// Add all events to the plugin's queue.
			for(VstInt32 i = 0; i < events->numEvents; i++)
			{
				vstPlugin->vstEvents.Enqueue(events->events[i]);
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


void CVstPlugin::RecalculateGain()
//--------------------------------
{
	float gain = 0.1f * static_cast<float>(m_pMixStruct ? m_pMixStruct->GetGain() : 10);
	if(gain < 0.1f) gain = 1.0f;

	if(m_bIsInstrument)
	{
		gain /= m_SndFile.GetPlayConfig().getVSTiAttenuation();
		gain = static_cast<float>(gain * (m_SndFile.m_nVSTiVolume / m_SndFile.GetPlayConfig().getNormalVSTiVol()));
	}
	m_fGain = gain;
}


void CVstPlugin::SetDryRatio(UINT param)
//--------------------------------------
{
	param = MIN(param, 127);
	m_pMixStruct->fDryRatio = static_cast<float>(1.0-(static_cast<double>(param)/127.0));
}


// Render some silence and return maximum level returned by the plugin.
float CVstPlugin::RenderSilence(size_t numSamples)
//------------------------------------------------
{
	// The JUCE framework doesn't like processing while being suspended.
	const bool wasSuspended = !IsResumed();
	if(wasSuspended)
	{
		Resume();
	}

	float out[2][MIXBUFFERSIZE]; // scratch buffers
	float maxVal = 0.0f;
	mixBuffer.ClearInputBuffers(MIXBUFFERSIZE);

	while(numSamples > 0)
	{
		size_t renderSamples = numSamples;
		LimitMax(renderSamples, CountOf(out[0]));
		MemsetZero(out);

		Process(out[0], out[1], renderSamples);
		for(size_t i = 0; i < renderSamples; i++)
		{
			maxVal = std::max(maxVal, fabs(out[0][i]));
			maxVal = std::max(maxVal, fabs(out[1][i]));
		}

		numSamples -= renderSamples;
	}

	if(wasSuspended)
	{
		Suspend();
	}

	return maxVal;
}


void CVstPlugin::Process(float *pOutL, float *pOutR, size_t nSamples)
//-------------------------------------------------------------------
{
	ProcessVSTEvents();

	//If the plug is found & ok, continue
	if(m_pProcessFP != nullptr && (mixBuffer.GetInputBufferArray()) && mixBuffer.GetOutputBufferArray() && m_pMixStruct != nullptr)
	{

		//RecalculateGain();

		// Merge stereo input before sending to the plug if the plug can only handle one input.
		if (m_Effect.numInputs == 1)
		{
			for (size_t i = 0; i < nSamples; i++)
			{
				m_MixState.pOutBufferL[i] = 0.5f * m_MixState.pOutBufferL[i] + 0.5f * m_MixState.pOutBufferR[i];
			}
		}

		float **outputBuffers = mixBuffer.GetOutputBufferArray();
		mixBuffer.ClearOutputBuffers(nSamples);

		// Do the VST processing magic
		try
		{
			ASSERT(nSamples <= MIXBUFFERSIZE);
			m_pProcessFP(&m_Effect, mixBuffer.GetInputBufferArray(), outputBuffers, nSamples);
		} catch (...)
		{
			Bypass();
			CString processMethod = (m_Effect.flags & effFlagsCanReplacing) ? "processReplacing" : "process";
			CVstPluginManager::ReportPlugException("The plugin %s threw an exception in %s. It has automatically been set to \"Bypass\".", m_pMixStruct->GetName(), processMethod);
		}

		ASSERT(outputBuffers != nullptr);

		// Mix outputs of multi-output VSTs:
		if(m_nOutputs > 2)
		{
			// first, mix extra outputs on a stereo basis
			uint32 numOutputs = m_nOutputs;
			// so if nOuts is not even, let process the last output later
			if((numOutputs % 2u) == 1) numOutputs--;

			// mix extra stereo outputs
			for(uint32 iOut = 2; iOut < numOutputs; iOut++)
			{
				for(size_t i = 0; i < nSamples; i++)
				{
					outputBuffers[iOut % 2u][i] += outputBuffers[iOut][i]; // assumed stereo.
				}
			}

			// if m_nOutputs is odd, mix half the signal of last output to each channel
			if(numOutputs != m_nOutputs)
			{
				// trick : if we are here, nOuts = m_nOutputs - 1 !!!
				for(size_t i = 0; i < nSamples; i++)
				{
					float v = 0.5f * outputBuffers[numOutputs][i];
					outputBuffers[0][i] += v;
					outputBuffers[1][i] += v;
				}
			}
		}

		ProcessMixOps(pOutL, pOutR, nSamples);

		// If dry mix is ticked, we add the unprocessed buffer,
		// except if this is an instrument since this it has already been done:
		if(m_pMixStruct->IsWetMix() && !m_bIsInstrument)
		{
			for(size_t i = 0; i < nSamples; i++)
			{
				pOutL[i] += m_MixState.pOutBufferL[i];
				pOutR[i] += m_MixState.pOutBufferR[i];
			}
		}
	}

	vstEvents.Clear();
}


void CVstPlugin::ProcessMixOps(float *pOutL, float *pOutR, size_t nSamples)
//-------------------------------------------------------------------------
{
	float *leftPlugOutput;
	float *rightPlugOutput;

	if(m_Effect.numOutputs == 1)
	{
		// If there was just the one plugin output we copy it into our 2 outputs
		leftPlugOutput = rightPlugOutput = mixBuffer.GetOutputBuffer(0);
	} else if(m_Effect.numOutputs > 1)
	{
		// Otherwise we actually only cater for two outputs max (outputs > 2 have been mixed together already).
		leftPlugOutput = mixBuffer.GetOutputBuffer(0);
		rightPlugOutput = mixBuffer.GetOutputBuffer(1);
	} else
	{
		return;
	}

	// -> mixop == 0 : normal processing
	// -> mixop == 1 : MIX += DRY - WET * wetRatio
	// -> mixop == 2 : MIX += WET - DRY * dryRatio
	// -> mixop == 3 : MIX -= WET - DRY * wetRatio
	// -> mixop == 4 : MIX -= middle - WET * wetRatio + middle - DRY
	// -> mixop == 5 : MIX_L += wetRatio * (WET_L - DRY_L) + dryRatio * (DRY_R - WET_R)
	//                 MIX_R += dryRatio * (WET_L - DRY_L) + wetRatio * (DRY_R - WET_R)

	int mixop;
	if(m_bIsInstrument || m_pMixStruct == nullptr)
	{
		// Force normal mix mode for instruments
		mixop = 0;
	} else
	{
		mixop = m_pMixStruct->GetMixMode();
	}

	float wetRatio = 1 - m_pMixStruct->fDryRatio;
	float dryRatio = m_bIsInstrument ? 1 : m_pMixStruct->fDryRatio; // Always mix full dry if this is an instrument

	// Wet / Dry range expansion [0,1] -> [-1,1]
	if(m_Effect.numInputs > 0 && m_pMixStruct->IsExpandedMix())
	{
		wetRatio = 2.0f * wetRatio - 1.0f;
		dryRatio = -wetRatio;
	}

	wetRatio *= m_fGain;
	dryRatio *= m_fGain;

	// Mix operation
	switch(mixop)
	{

	// Default mix
	case 0:
		for(size_t i = 0; i < nSamples; i++)
		{
			//rewbs.wetratio - added the factors. [20040123]
			pOutL[i] += leftPlugOutput[i] * wetRatio + m_MixState.pOutBufferL[i] * dryRatio;
			pOutR[i] += rightPlugOutput[i] * wetRatio + m_MixState.pOutBufferR[i] * dryRatio;
		}
		break;

	// Wet subtract
	case 1:
		for(size_t i = 0; i < nSamples; i++)
		{
			pOutL[i] += m_MixState.pOutBufferL[i] - leftPlugOutput[i] * wetRatio;
			pOutR[i] += m_MixState.pOutBufferR[i] - rightPlugOutput[i] * wetRatio;
		}
		break;

	// Dry subtract
	case 2:
		for(size_t i = 0; i < nSamples; i++)
		{
			pOutL[i] += leftPlugOutput[i] - m_MixState.pOutBufferL[i] * dryRatio;
			pOutR[i] += rightPlugOutput[i] - m_MixState.pOutBufferR[i] * dryRatio;
		}
		break;

	// Mix subtract
	case 3:
		for(size_t i = 0; i < nSamples; i++)
		{
			pOutL[i] -= leftPlugOutput[i] - m_MixState.pOutBufferL[i] * wetRatio;
			pOutR[i] -= rightPlugOutput[i] - m_MixState.pOutBufferR[i] * wetRatio;
		}
		break;

	// Middle subtract
	case 4:
		for(size_t i = 0; i < nSamples; i++)
		{
			float middle = (pOutL[i] + m_MixState.pOutBufferL[i] + pOutR[i] + m_MixState.pOutBufferR[i]) / 2.0f;
			pOutL[i] -= middle - leftPlugOutput[i] * wetRatio + middle - m_MixState.pOutBufferL[i];
			pOutR[i] -= middle - rightPlugOutput[i] * wetRatio + middle - m_MixState.pOutBufferR[i];
		}
		break;

	// Left / Right balance
	case 5:
		if(m_pMixStruct->IsExpandedMix())
		{
			wetRatio /= 2.0f;
			dryRatio /= 2.0f;
		}

		for(size_t i = 0; i < nSamples; i++)
		{
			pOutL[i] += wetRatio * (leftPlugOutput[i] - m_MixState.pOutBufferL[i]) + dryRatio * (m_MixState.pOutBufferR[i] - rightPlugOutput[i]);
			pOutR[i] += dryRatio * (leftPlugOutput[i] - m_MixState.pOutBufferL[i]) + wetRatio * (m_MixState.pOutBufferR[i] - rightPlugOutput[i]);
		}
		break;
	}
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
		VSTInstrChannel &channel = m_MidiCh[mc];

		MidiPitchBend(mc, EncodePitchBendParam(MIDIEvents::pitchBendCentre));		// centre pitch bend
		MidiSend(MIDIEvents::CC(MIDIEvents::MIDICC_AllControllersOff, mc, 0));		// reset all controllers
		MidiSend(MIDIEvents::CC(MIDIEvents::MIDICC_AllNotesOff, mc, 0));			// all notes off
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


void CVstPlugin::MidiCC(uint8 nMidiCh, MIDIEvents::MidiCC nController, uint8 nParam, CHANNELINDEX /*trackChannel*/)
//-----------------------------------------------------------------------------------------------------------------
{
	//Error checking
	LimitMax(nController, MIDIEvents::MIDICC_end);
	LimitMax(nParam, uint8(127));

	if(m_SndFile.GetModFlag(MSF_MIDICC_BUGEMULATION))
		MidiSend(MIDIEvents::Event(MIDIEvents::evControllerChange, nMidiCh, nParam, static_cast<uint8>(nController)));	// param and controller are swapped (old broken implementation)
	else
		MidiSend(MIDIEvents::CC(nController, nMidiCh, nParam));
}


void CVstPlugin::ApplyPitchWheelDepth(int32 &value, int8 pwd)
//-----------------------------------------------------------
{
	if(pwd != 0)
	{
		value = (value * ((MIDIEvents::pitchBendMax - MIDIEvents::pitchBendCentre + 1) / 64)) / pwd;
	} else
	{
		value = 0;
	}
}


// Bend MIDI pitch for given MIDI channel using fine tracker param (one unit = 1/64th of a note step)
void CVstPlugin::MidiPitchBend(uint8 nMidiCh, int32 increment, int8 pwd)
//----------------------------------------------------------------------
{
	if(m_SndFile.GetModFlag(MSF_OLD_MIDI_PITCHBENDS))
	{
		// OpenMPT Legacy: Old pitch slides never were really accurate, but setting the PWD to 13 in plugins would give the closest results.
		increment = (increment * 0x800 * 13) / (0xFF * pwd);
		increment = EncodePitchBendParam(increment);
	} else
	{
		increment = EncodePitchBendParam(increment);
		ApplyPitchWheelDepth(increment, pwd);
	}

	int32 newPitchBendPos = (increment + m_MidiCh[nMidiCh].midiPitchBendPos) & vstPitchBendMask;
	Limit(newPitchBendPos, EncodePitchBendParam(MIDIEvents::pitchBendMin), EncodePitchBendParam(MIDIEvents::pitchBendMax));

	MidiPitchBend(nMidiCh, newPitchBendPos);
}


// Set MIDI pitch for given MIDI channel using fixed point pitch bend value (converted back to 0-16383 MIDI range)
void CVstPlugin::MidiPitchBend(uint8 nMidiCh, int32 newPitchBendPos)
//------------------------------------------------------------------
{
	ASSERT(EncodePitchBendParam(MIDIEvents::pitchBendMin) <= newPitchBendPos && newPitchBendPos <= EncodePitchBendParam(MIDIEvents::pitchBendMax));
	m_MidiCh[nMidiCh].midiPitchBendPos = newPitchBendPos;
	MidiSend(MIDIEvents::PitchBend(nMidiCh, DecodePitchBendParam(newPitchBendPos)));
}


// Apply vibrato effect through pitch wheel commands on a given MIDI channel.
void CVstPlugin::MidiVibrato(uint8 nMidiCh, int32 depth, int8 pwd)
//----------------------------------------------------------------
{
	depth = EncodePitchBendParam(depth);
	if(depth != 0 || (m_MidiCh[nMidiCh].midiPitchBendPos & vstVibratoFlag))
	{
		ApplyPitchWheelDepth(depth, pwd);

		// Temporarily add vibrato offset to current pitch
		int32 newPitchBendPos = (depth + m_MidiCh[nMidiCh].midiPitchBendPos) & vstPitchBendMask;
		Limit(newPitchBendPos, EncodePitchBendParam(MIDIEvents::pitchBendMin), EncodePitchBendParam(MIDIEvents::pitchBendMax));

		MidiSend(MIDIEvents::PitchBend(nMidiCh, DecodePitchBendParam(newPitchBendPos)));
	}

	// Update vibrato status
	if(depth != 0)
	{
		m_MidiCh[nMidiCh].midiPitchBendPos |= vstVibratoFlag;
	} else
	{
		m_MidiCh[nMidiCh].midiPitchBendPos &= ~vstVibratoFlag;
	}
}


//rewbs.introVST - many changes to MidiCommand, still to be refined.
void CVstPlugin::MidiCommand(uint8 nMidiCh, uint8 nMidiProg, uint16 wMidiBank, uint16 note, uint16 vol, CHANNELINDEX trackChannel)
//--------------------------------------------------------------------------------------------------------------------------------
{
	VSTInstrChannel &channel = m_MidiCh[nMidiCh];

	bool bankChanged = (channel.currentBank != --wMidiBank) && (wMidiBank < 0x4000);
	bool progChanged = (channel.currentProgram != --nMidiProg) && (nMidiProg < 0x80);
	//get vol in [0,128[
	uint8 volume = static_cast<uint8>(std::min(vol / 2, 127));

	// Bank change
	if(wMidiBank < 0x4000 && bankChanged)
	{
		uint8 high = static_cast<uint8>(wMidiBank >> 7);
		uint8 low = static_cast<uint8>(wMidiBank & 0x7F);

		if((channel.currentBank >> 7) != high)
		{
			// High byte changed
			MidiSend(MIDIEvents::CC(MIDIEvents::MIDICC_BankSelect_Coarse, nMidiCh, high));
		}
		// Low byte
		//GetSoundFile()->ProcessMIDIMacro(trackChannel, false, GetSoundFile()->m_MidiCfg.szMidiGlb[MIDIOUT_BANKSEL], 0);
		MidiSend(MIDIEvents::CC(MIDIEvents::MIDICC_BankSelect_Fine, nMidiCh, low));

		channel.currentBank = wMidiBank;
	}

	// Program change
	// According to the MIDI specs, a bank change alone doesn't have to change the active program - it will only change the bank of subsequent program changes.
	// Thus we send program changes also if only the bank has changed.
	if(nMidiProg < 0x80 && (progChanged || bankChanged))
	{
		channel.currentProgram = nMidiProg;
		//GetSoundFile()->ProcessMIDIMacro(trackChannel, false, GetSoundFile()->m_MidiCfg.szMidiGlb[MIDIOUT_PROGRAM], 0);
		MidiSend(MIDIEvents::ProgramChange(nMidiCh, nMidiProg));
	}


	// Specific Note Off
	if(note > NOTE_MAX_SPECIAL)
	{
		uint8 i = static_cast<uint8>(note - NOTE_MAX_SPECIAL - NOTE_MIN);
		if(channel.noteOnMap[i][trackChannel])
		{
			channel.noteOnMap[i][trackChannel]--;
			MidiSend(MIDIEvents::NoteOff(nMidiCh, i, 0));
		}
	}

	// "Hard core" All Sounds Off on this midi and tracker channel
	// This one doesn't check the note mask - just one note off per note.
	// Also less likely to cause a VST event buffer overflow.
	else if(note == NOTE_NOTECUT)	// ^^
	{
		MidiSend(MIDIEvents::CC(MIDIEvents::MIDICC_AllNotesOff, nMidiCh, 0));
		MidiSend(MIDIEvents::CC(MIDIEvents::MIDICC_AllSoundOff, nMidiCh, 0));

		// Turn off all notes
		for(uint8 i = 0; i < CountOf(channel.noteOnMap); i++)
		{
			channel.noteOnMap[i][trackChannel] = 0;
			MidiSend(MIDIEvents::NoteOff(nMidiCh, i, volume));
		}

	}

	// All "active" notes off on this midi and tracker channel
	// using note mask.
	else if(note == NOTE_KEYOFF || note == NOTE_FADE) // ==, ~~
	{
		for(uint8 i = 0; i < CountOf(channel.noteOnMap); i++)
		{
			// Some VSTis need a note off for each instance of a note on, e.g. fabfilter.
			while(channel.noteOnMap[i][trackChannel])
			{
				MidiSend(MIDIEvents::NoteOff(nMidiCh, i, volume));
				channel.noteOnMap[i][trackChannel]--;
			}
		}
	}

	// Note On
	else if(ModCommand::IsNote(static_cast<ModCommand::NOTE>(note)))
	{
		note -= NOTE_MIN;

		// Reset pitch bend on each new note, tracker style.
		// This is done if the pitch wheel has been moved or there was a vibrato on the previous row (in which case the "vstVibratoFlag" bit of the pitch bend memory is set)
		if(m_MidiCh[nMidiCh].midiPitchBendPos != EncodePitchBendParam(MIDIEvents::pitchBendCentre))
		{
			MidiPitchBend(nMidiCh, EncodePitchBendParam(MIDIEvents::pitchBendCentre));
		}

		// count instances of active notes.
		// This is to send a note off for each instance of a note, for plugs like Fabfilter.
		// Problem: if a note dies out naturally and we never send a note off, this counter
		// will block at max until note off. Is this a problem?
		// Safe to assume we won't need more than 16 note offs max on a given note?
		if(channel.noteOnMap[note][trackChannel] < 17)
			channel.noteOnMap[note][trackChannel]++;

		MidiSend(MIDIEvents::NoteOn(nMidiCh, static_cast<uint8>(note), volume));
	}
}


bool CVstPlugin::isPlaying(UINT note, UINT midiChn, UINT trackerChn)
//------------------------------------------------------------------
{
	note -= NOTE_MIN;
	return (m_MidiCh[midiChn].noteOnMap[note][trackerChn] != 0);
}


bool CVstPlugin::MoveNote(UINT note, UINT midiChn, UINT sourceTrackerChn, UINT destTrackerChn)
//---------------------------------------------------------------------------------------------
{
	note -= NOTE_MIN;
	VSTInstrChannel *pMidiCh = &m_MidiCh[midiChn & 0x0f];

	if (!(pMidiCh->noteOnMap[note][sourceTrackerChn]))
		return false;

	pMidiCh->noteOnMap[note][sourceTrackerChn]--;
	pMidiCh->noteOnMap[note][destTrackerChn]++;
	return true;
}
//end rewbs.introVST


void CVstPlugin::SetZxxParameter(UINT nParam, UINT nValue)
//--------------------------------------------------------
{
	PlugParamValue fValue = (PlugParamValue)nValue / 127.0f;
	SetParameter(nParam, fValue);
}

//rewbs.smoothVST
UINT CVstPlugin::GetZxxParameter(UINT nParam)
//-------------------------------------------
{
	return (UINT) (GetParameter(nParam) * 127.0f + 0.5f);
}
//end rewbs.smoothVST


// Automate a parameter from the plugin GUI (both custom and default plugin GUI)
void CVstPlugin::AutomateParameter(PlugParamIndex param)
//------------------------------------------------------
{
#ifdef MODPLUG_TRACKER
	CModDoc* pModDoc = GetModDoc();
	if(pModDoc == nullptr)
	{
		return;
	}

	if (m_bRecordAutomation)
	{
		// Record parameter change
		pModDoc->RecordParamChange(GetSlot(), param);
	}

	CAbstractVstEditor *pVstEditor = GetEditor();

	if(pVstEditor && pVstEditor->m_hWnd)
	{
		// Mark track modified if GUI is open and format supports plugins
		if(pModDoc->GetrSoundFile().GetModSpecifications().supportsPlugins)
		{
			CMainFrame::GetMainFrame()->ThreadSafeSetModified(pModDoc);
		}

		// TODO: Could be used to update general tab in real time, but causes flickers in treeview
		// Better idea: add an update hint just for plugin params?
		//pModDoc->UpdateAllViews(nullptr, HINT_MIXPLUGINS, nullptr);

		if (CMainFrame::GetInputHandler()->ShiftPressed())
		{
			// Shift pressed -> Open MIDI mapping dialog
			CMainFrame::GetInputHandler()->SetModifierMask(0); // Make sure that the dialog will open only once.

			CMIDIMappingDialog dlg(pVstEditor, pModDoc->GetrSoundFile());
			dlg.m_Setting.SetParamIndex(param);
			dlg.m_Setting.SetPlugIndex(GetSlot() + 1);
			dlg.DoModal();
		}

		// Learn macro
		int macroToLearn = pVstEditor->GetLearnMacro();
		if (macroToLearn > -1)
		{
			pModDoc->LearnMacro(macroToLearn, param);
			pVstEditor->SetLearnMacro(-1);
		}
	}
#endif // MODPLUG_TRACKER
}


void CVstPlugin::SaveAllParameters()
//----------------------------------
{
	if (m_pMixStruct)
	{
		m_pMixStruct->defaultProgram = -1;

		if(ProgramsAreChunks() && Dispatch(effIdentify, 0,0, nullptr, 0.0f) == 'NvEf')
		{
			void *p = nullptr;
			LONG nByteSize = 0;

			// Try to get whole bank
			nByteSize = Dispatch(effGetChunk, 0,0, &p, 0);

			if (!p)
			{
				nByteSize = Dispatch(effGetChunk, 1,0, &p, 0); 	// Getting bank failed, try to get get just preset
			} else
			{
				m_pMixStruct->defaultProgram = GetCurrentProgram();	//we managed to get the bank, now we need to remember which program we're on.
			}
			if (p != nullptr)
			{
				if ((m_pMixStruct->pPluginData) && (m_pMixStruct->nPluginDataSize >= (UINT)(nByteSize+4)))
				{
					m_pMixStruct->nPluginDataSize = nByteSize+4;
				} else
				{
					delete[] m_pMixStruct->pPluginData;
					m_pMixStruct->nPluginDataSize = 0;
					m_pMixStruct->pPluginData = new char[nByteSize+4];
					if (m_pMixStruct->pPluginData)
					{
						m_pMixStruct->nPluginDataSize = nByteSize+4;
					}
				}
				if (m_pMixStruct->pPluginData)
				{
					*(ULONG *)m_pMixStruct->pPluginData = 'NvEf';
					memcpy(((BYTE*)m_pMixStruct->pPluginData)+4, p, nByteSize);
					return;
				}
			}
		}
		// This plug doesn't support chunks: save parameters
		PlugParamIndex nParams = (m_Effect.numParams > 0) ? m_Effect.numParams : 0;
		UINT nLen = nParams * sizeof(float);
		if (!nLen) return;
		nLen += 4;
		if ((m_pMixStruct->pPluginData) && (m_pMixStruct->nPluginDataSize >= nLen))
		{
			m_pMixStruct->nPluginDataSize = nLen;
		} else
		{
			if (m_pMixStruct->pPluginData) delete[] m_pMixStruct->pPluginData;
			m_pMixStruct->nPluginDataSize = 0;
			m_pMixStruct->pPluginData = new char[nLen];
			if (m_pMixStruct->pPluginData)
			{
				m_pMixStruct->nPluginDataSize = nLen;
			}
		}
		if (m_pMixStruct->pPluginData)
		{
			float *p = (float *)m_pMixStruct->pPluginData;
			*(ULONG *)p = 0;
			p++;
			for(PlugParamIndex i = 0; i < nParams; i++)
			{
				p[i] = GetParameter(i);
			}
		}
	}
	return;
}


void CVstPlugin::RestoreAllParameters(long nProgram)
//--------------------------------------------------
{
	if(m_pMixStruct != nullptr && m_pMixStruct->pPluginData != nullptr && m_pMixStruct->nPluginDataSize >= 4)
	{
		UINT nParams = (m_Effect.numParams > 0) ? m_Effect.numParams : 0;
		UINT nLen = nParams * sizeof(float);
		ULONG nType = *(ULONG *)m_pMixStruct->pPluginData;

		if ((Dispatch(effIdentify, 0, 0, nullptr, 0) == 'NvEf') && (nType == 'NvEf'))
		{
			void *p = nullptr;
			Dispatch(effGetChunk, 0,0, &p, 0); //init plug for chunk reception

			if ((nProgram>=0) && (nProgram < m_Effect.numPrograms))
			{
				// Bank
				Dispatch(effSetChunk, 0, m_pMixStruct->nPluginDataSize - 4, ((BYTE *)m_pMixStruct->pPluginData) + 4, 0);
				SetCurrentProgram(nProgram);
			} else
			{
				// Program
				Dispatch(effSetChunk, 1, m_pMixStruct->nPluginDataSize - 4, ((BYTE *)m_pMixStruct->pPluginData) + 4, 0);
			}

		} else
		{
			float *p = (float *)m_pMixStruct->pPluginData;
			if (m_pMixStruct->nPluginDataSize >= nLen + 4) p++;
			if (m_pMixStruct->nPluginDataSize >= nLen)
			{
				for (UINT i = 0; i < nParams; i++)
				{
					SetParameter(i, p[i]);
				}
			}
		}
	}
}


void CVstPlugin::ToggleEditor()
//-----------------------------
{
	try
	{
		if ((m_pEditor) && (!m_pEditor->m_hWnd))
		{
			delete m_pEditor;
			m_pEditor = nullptr;
		}
		if (m_pEditor)
		{
			if (m_pEditor->m_hWnd) m_pEditor->DoClose();
			if ((volatile void *)m_pEditor) delete m_pEditor;
			m_pEditor = nullptr;
		} else
		{
			if (HasEditor())
				m_pEditor =  new COwnerVstEditor(*this);
			else
				m_pEditor = new CDefaultVstEditor(*this);

			if (m_pEditor)
				m_pEditor->OpenEditor(CMainFrame::GetMainFrame());
		}
	} catch (...)
	{
		CVstPluginManager::ReportPlugException("Exception in ToggleEditor() (Plugin=%s)\n", m_Factory.szLibraryName);
	}
}


CAbstractVstEditor* CVstPlugin::GetEditor()
//-----------------------------------------
{
	return m_pEditor;
}


void CVstPlugin::Bypass(bool bypass)
//----------------------------------
{
	m_pMixStruct->Info.SetBypass(bypass);

	Dispatch(effSetBypass, bypass ? 1 : 0, 0, nullptr, 0.0f);

#ifdef MODPLUG_TRACKER
	if(m_SndFile.GetpModDoc())
		m_SndFile.GetpModDoc()->UpdateAllViews(nullptr, HINT_MIXPLUGINS, nullptr);
#endif // MODPLUG_TRACKER
}


void CVstPlugin::NotifySongPlaying(bool playing)
//----------------------------------------------
{
	m_bSongPlaying = playing;
}


PLUGINDEX CVstPlugin::FindSlot()
//------------------------------
{
	PLUGINDEX slot = 0;
	while(m_pMixStruct != &(m_SndFile.m_MixPlugins[slot]) && slot < MAX_MIXPLUGINS - 1)
	{
		slot++;
	}
	return slot;
}


void CVstPlugin::SetSlot(PLUGINDEX slot)
//--------------------------------------
{
	m_nSlot = slot;
}


PLUGINDEX CVstPlugin::GetSlot()
//-----------------------------
{
	return m_nSlot;
}


void CVstPlugin::UpdateMixStructPtr(SNDMIXPLUGIN *p)
//--------------------------------------------------
{
	m_pMixStruct = p;
}


bool CVstPlugin::isInstrument()
//-----------------------------
{
	return ((m_Effect.flags & effFlagsIsSynth) || (!m_Effect.numInputs));
}


bool CVstPlugin::CanRecieveMidiEvents()
//-------------------------------------
{
	return (CVstPlugin::Dispatch(effCanDo, 0, 0, "receiveVstMidiEvent", 0.0f) != 0);
}


// Get list of plugins to which output is sent. A nullptr indicates master output.
size_t CVstPlugin::GetOutputPlugList(std::vector<CVstPlugin *> &list)
//-------------------------------------------------------------------
{
	// At the moment we know there will only be 1 output.
	// Returning nullptr means plugin outputs directly to master.
	list.clear();

	CVstPlugin *outputPlug = nullptr;
	if(!m_pMixStruct->IsOutputToMaster())
	{
		PLUGINDEX nOutput = m_pMixStruct->GetOutputPlugin();
		if(nOutput > m_nSlot && nOutput != PLUGINDEX_INVALID)
		{
			outputPlug = dynamic_cast<CVstPlugin *>(m_SndFile.m_MixPlugins[nOutput].pMixPlugin);
		}
	}
	list.push_back(outputPlug);

	return 1;
}


// Get a list of plugins that send data to this plugin.
size_t CVstPlugin::GetInputPlugList(std::vector<CVstPlugin *> &list)
//------------------------------------------------------------------
{
	std::vector<CVstPlugin *> candidatePlugOutputs;
	list.clear();

	for(PLUGINDEX plug = 0; plug < MAX_MIXPLUGINS; plug++)
	{
		CVstPlugin *candidatePlug = dynamic_cast<CVstPlugin *>(m_SndFile.m_MixPlugins[plug].pMixPlugin);
		if(candidatePlug)
		{
			candidatePlug->GetOutputPlugList(candidatePlugOutputs);

			for(std::vector<CVstPlugin *>::iterator iter = candidatePlugOutputs.begin(); iter != candidatePlugOutputs.end(); iter++)
			{
				if(*iter == this)
				{
					list.push_back(candidatePlug);
					break;
				}
			}
		}
	}

	return list.size();
}


// Get a list of instruments that send data to this plugin.
size_t CVstPlugin::GetInputInstrumentList(std::vector<INSTRUMENTINDEX> &list)
//---------------------------------------------------------------------------
{
	list.clear();
	const PLUGINDEX nThisMixPlug = m_nSlot + 1;		//m_nSlot is position in mixplug array.

	for(INSTRUMENTINDEX ins = 0; ins <= m_SndFile.GetNumInstruments(); ins++)
	{
		if(m_SndFile.Instruments[ins] != nullptr && m_SndFile.Instruments[ins]->nMixPlug == nThisMixPlug)
		{
			list.push_back(ins);
		}
	}

	return list.size();
}


size_t CVstPlugin::GetInputChannelList(std::vector<CHANNELINDEX> &list)
//---------------------------------------------------------------------
{
	list.clear();

	PLUGINDEX nThisMixPlug = m_nSlot + 1;		//m_nSlot is position in mixplug array.
	const CHANNELINDEX chnCount = m_SndFile.GetNumChannels();
	for(CHANNELINDEX nChn=0; nChn<chnCount; nChn++)
	{
		if(m_SndFile.ChnSettings[nChn].nMixPlugin == nThisMixPlug)
		{
			list.push_back(nChn);
		}
	}

	return list.size();

}


//////////////////////////////////////////////////////////////////////////////////////////////////
//
// DirectX Media Object support
//

//============
class CDmo2Vst
//============
{
protected:
	IMediaObject *m_pMediaObject;
	IMediaObjectInPlace *m_pMediaProcess;
	IMediaParamInfo *m_pParamInfo;
	IMediaParams *m_pMediaParams;
	ULONG m_nSamplesPerSec;
	AEffect m_Effect;
	REFERENCE_TIME m_DataTime;
	int16 *m_pMixBuffer;
	int16 m_MixBuffer[MIXBUFFERSIZE * 2 + 16];		// 16-bit Stereo interleaved

public:
	CDmo2Vst(IMediaObject *pMO, IMediaObjectInPlace *pMOIP, DWORD uid);
	~CDmo2Vst();
	AEffect *GetEffect() { return &m_Effect; }
	VstIntPtr Dispatcher(VstInt32 opCode, VstInt32 index, VstIntPtr value, void *ptr, float opt);
	void SetParameter(VstInt32 index, float parameter);
	float GetParameter(VstInt32 index);
	void Process(float * const *inputs, float **outputs, int samples);

public:
	static VstIntPtr VSTCALLBACK DmoDispatcher(AEffect *effect, VstInt32 opCode, VstInt32 index, VstIntPtr value, void *ptr, float opt);
	static void VSTCALLBACK DmoSetParameter(AEffect *effect, VstInt32 index, float parameter);
	static float VSTCALLBACK DmoGetParameter(AEffect *effect, VstInt32 index);
	static void VSTCALLBACK DmoProcess(AEffect *effect, float **inputs, float **outputs, VstInt32 sampleframes);

protected:
	// Stream conversion functions
	void InterleaveFloatToInt16(const float *inLeft, const float *inRight, int nsamples);
	void DeinterleaveInt16ToFloat(float *outLeft, float *outRight, int nsamples) const;
#ifdef ENABLE_SSE
	void SSEInterleaveFloatToInt16(const float *inLeft, const float *inRight, int nsamples);
	void SSEDeinterleaveInt16ToFloat(float *outLeft, float *outRight, int nsamples) const;
#endif // ENABLE_SSE
};


CDmo2Vst::CDmo2Vst(IMediaObject *pMO, IMediaObjectInPlace *pMOIP, DWORD uid)
//--------------------------------------------------------------------------
{
	m_pMediaObject = pMO;
	m_pMediaProcess = pMOIP;
	m_pParamInfo = nullptr;
	m_pMediaParams = nullptr;
	MemsetZero(m_Effect);
	m_Effect.magic = kDmoMagic;
	m_Effect.numPrograms = 0;
	m_Effect.numParams = 0;
	m_Effect.numInputs = 2;
	m_Effect.numOutputs = 2;
	m_Effect.flags = 0;		// see constants
	m_Effect.ioRatio = 1.0f;
	m_Effect.object = this;
	m_Effect.uniqueID = uid;
	m_Effect.version = 2;
	m_nSamplesPerSec = 44100;
	m_DataTime = 0;
	if (FAILED(m_pMediaObject->QueryInterface(IID_IMediaParamInfo, (void **)&m_pParamInfo))) m_pParamInfo = nullptr;
	if (m_pParamInfo)
	{
		DWORD dwParamCount = 0;
		m_pParamInfo->GetParamCount(&dwParamCount);
		m_Effect.numParams = dwParamCount;
	}
	if (FAILED(m_pMediaObject->QueryInterface(IID_IMediaParams, (void **)&m_pMediaParams))) m_pMediaParams = nullptr;
	m_pMixBuffer = (int16 *)((((intptr_t)m_MixBuffer) + 15) & ~15);
	// Callbacks
	m_Effect.dispatcher = DmoDispatcher;
	m_Effect.setParameter = DmoSetParameter;
	m_Effect.getParameter = DmoGetParameter;
	m_Effect.process = DmoProcess;
	m_Effect.processReplacing = nullptr;
}


CDmo2Vst::~CDmo2Vst()
//-------------------
{
	if (m_pMediaParams)
	{
		m_pMediaParams->Release();
		m_pMediaParams = nullptr;
	}
	if (m_pParamInfo)
	{
		m_pParamInfo->Release();
		m_pParamInfo = nullptr;
	}
	if (m_pMediaProcess)
	{
		m_pMediaProcess->Release();
		m_pMediaProcess = nullptr;
	}
	if (m_pMediaObject)
	{
		m_pMediaObject->Release();
		m_pMediaObject = nullptr;
	}
}


VstIntPtr CDmo2Vst::DmoDispatcher(AEffect *effect, VstInt32 opCode, VstInt32 index, VstIntPtr value, void *ptr, float opt)
//------------------------------------------------------------------------------------------------------------------------
{
	if (effect)
	{
		CDmo2Vst *that = (CDmo2Vst *)effect->object;
		if (that) return that->Dispatcher(opCode, index, value, ptr, opt);
	}
	return 0;
}


void CDmo2Vst::DmoSetParameter(AEffect *effect, VstInt32 index, float parameter)
//------------------------------------------------------------------------------
{
	if (effect)
	{
		CDmo2Vst *that = (CDmo2Vst *)effect->object;
		if (that) that->SetParameter(index, parameter);
	}
}


float CDmo2Vst::DmoGetParameter(AEffect *effect, VstInt32 index)
//----------------------------------------------------------
{
	if (effect)
	{
		CDmo2Vst *that = (CDmo2Vst *)effect->object;
		if (that) return that->GetParameter(index);
	}
	return 0;
}


void CDmo2Vst::DmoProcess(AEffect *effect, float **inputs, float **outputs, VstInt32 sampleframes)
//------------------------------------------------------------------------------------------------
{
	if (effect)
	{
		CDmo2Vst *that = (CDmo2Vst *)effect->object;
		if (that) that->Process(inputs, outputs, sampleframes);
	}
}


VstIntPtr CDmo2Vst::Dispatcher(VstInt32 opCode, VstInt32 index, VstIntPtr value, void *ptr, float opt)
//----------------------------------------------------------------------------------------------------
{
	switch(opCode)
	{
	case effOpen:
		break;

	case effClose:
		m_Effect.object = nullptr;
		delete this;
		return 0;

	case effGetParamName:
	case effGetParamLabel:
	case effGetParamDisplay:
		if ((index < m_Effect.numParams) && (m_pParamInfo))
		{
			MP_PARAMINFO mpi;
			LPSTR pszName = (LPSTR)ptr;

			mpi.mpType = MPT_INT;
			mpi.szUnitText[0] = 0;
			mpi.szLabel[0] = 0;
			pszName[0] = 0;
			if (m_pParamInfo->GetParamInfo(index, &mpi) == S_OK)
			{
				if (opCode == effGetParamDisplay)
				{
					MP_DATA md;

					if ((m_pMediaParams) && (m_pMediaParams->GetParam(index, &md) == S_OK))
					{
						switch(mpi.mpType)
						{
						case MPT_FLOAT:
							{
								int nValue = (int)(md * 100.0f + 0.5f);
								bool bNeg = false;
								if (nValue < 0) { bNeg = true; nValue = -nValue; }
								wsprintf(pszName, (bNeg) ? "-%d.%02d" : "%d.%02d", nValue/100, nValue%100);
							}
							break;

						case MPT_BOOL:
							strcpy(pszName, ((int)md) ? "Yes" : "No");
							break;

						case MPT_ENUM:
							{
								WCHAR *text = nullptr;
								m_pParamInfo->GetParamText(index, &text);

								const int nValue = (int)(md * (mpi.mpdMaxValue - mpi.mpdMinValue) + 0.5f);
								// Always skip first two strings (param name, unit name)
								for(int i = 0; i < nValue + 2; i++)
								{
									text += wcslen(text) + 1;
								}
								WideCharToMultiByte(CP_ACP, 0, text, -1, pszName, 32, nullptr, nullptr);
							}
							break;

						case MPT_INT:
						default:
							wsprintf(pszName, "%d", (int)md);
							break;
						}
					}
				} else
				{
					WideCharToMultiByte(CP_ACP, 0, (opCode == effGetParamName) ? mpi.szLabel : mpi.szUnitText, -1, pszName, 32, nullptr, nullptr);
				}
			}
		}
		break;

	case effCanBeAutomated:
		return (index < m_Effect.numParams);

	case effSetSampleRate:
		m_nSamplesPerSec = (int)opt;
		break;

	case effMainsChanged:
		if (value)
		{
			DMO_MEDIA_TYPE mt;
			WAVEFORMATEX wfx;

			mt.majortype = MEDIATYPE_Audio;
			mt.subtype = MEDIASUBTYPE_PCM;
			mt.bFixedSizeSamples = TRUE;
			mt.bTemporalCompression = FALSE;
			mt.formattype = FORMAT_WaveFormatEx;
			mt.pUnk = nullptr;
			mt.pbFormat = (LPBYTE)&wfx;
			mt.cbFormat = sizeof(WAVEFORMATEX);
			mt.lSampleSize = 2 * sizeof(int16);
			wfx.wFormatTag = WAVE_FORMAT_PCM;
			wfx.nChannels = 2;
			wfx.nSamplesPerSec = m_nSamplesPerSec;
			wfx.wBitsPerSample = sizeof(int16) * 8;
			wfx.nBlockAlign = wfx.nChannels * (wfx.wBitsPerSample / 8);
			wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
			wfx.cbSize = 0;
			if (FAILED(m_pMediaObject->SetInputType(0, &mt, 0))
				|| FAILED(m_pMediaObject->SetOutputType(0, &mt, 0)))
			{
			#ifdef DMO_LOG
				Log("DMO: Failed to set I/O media type\n");
			#endif
				return -1;
			}
		} else
		{
			m_pMediaObject->Flush();
			m_pMediaObject->SetInputType(0, nullptr, DMO_SET_TYPEF_CLEAR);
			m_pMediaObject->SetOutputType(0, nullptr, DMO_SET_TYPEF_CLEAR);
			m_DataTime = 0;
		}
		break;
	}
	return 0;
}


void CDmo2Vst::SetParameter(VstInt32 index, float fValue)
//-------------------------------------------------------
{
	MP_PARAMINFO mpi;

	if ((index < m_Effect.numParams) && (m_pParamInfo) && (m_pMediaParams))
	{
		MemsetZero(mpi);
		if (m_pParamInfo->GetParamInfo(index, &mpi) == S_OK)
		{
			float fMin = mpi.mpdMinValue;
			float fMax = mpi.mpdMaxValue;

			if (mpi.mpType == MPT_BOOL)
			{
				fMin = 0;
				fMax = 1;
				fValue = (fValue > 0.5f) ? 1.0f : 0.0f;
			}
			if (fMax > fMin) fValue *= (fMax - fMin);
			fValue += fMin;
			if (fValue < fMin) fValue = fMin;
			if (fValue > fMax) fValue = fMax;
			if (mpi.mpType != MPT_FLOAT) fValue = (float)(int)fValue;
			m_pMediaParams->SetParam(index, fValue);
		}
	}
}


float CDmo2Vst::GetParameter(VstInt32 index)
//------------------------------------------
{
	if ((index < m_Effect.numParams) && (m_pParamInfo) && (m_pMediaParams))
	{
		MP_PARAMINFO mpi;
		MP_DATA md;

		MemsetZero(mpi);
		md = 0;
		if (m_pParamInfo->GetParamInfo(index, &mpi) == S_OK
			&& m_pMediaParams->GetParam(index, &md) == S_OK)
		{
			float fValue, fMin, fMax, fDefault;

			fValue = md;
			fMin = mpi.mpdMinValue;
			fMax = mpi.mpdMaxValue;
			fDefault = mpi.mpdNeutralValue;
			if (mpi.mpType == MPT_BOOL)
			{
				fMin = 0;
				fMax = 1;
			}
			fValue -= fMin;
			if (fMax > fMin) fValue /= (fMax - fMin);
			return fValue;
		}
	}
	return 0;
}


static const float _f2si = 32768.0f;
static const float _si2f = 1.0f / 32768.0f;

// Interleave two float streams into one int16 stereo stream.
void CDmo2Vst::InterleaveFloatToInt16(const float *inLeft, const float *inRight, int samples)
//-------------------------------------------------------------------------------------------
{
	int16 *outBuf = m_pMixBuffer;
	for(int i = samples; i != 0; i--)
	{
		*(outBuf++) = static_cast<int16>(Clamp(*(inLeft++) * _f2si, static_cast<float>(int16_min), static_cast<float>(int16_max)));
		*(outBuf++) = static_cast<int16>(Clamp(*(inRight++) * _f2si, static_cast<float>(int16_min), static_cast<float>(int16_max)));
	}
}


// Deinterleave an int16 stereo stream into two float streams.
void CDmo2Vst::DeinterleaveInt16ToFloat(float *outLeft, float *outRight, int samples) const
//-----------------------------------------------------------------------------------------
{
	const int16 *inBuf = m_pMixBuffer;
	for(int i = samples; i != 0; i--)
	{
		*outLeft++ += _si2f * static_cast<float>(*inBuf++);
		*outRight++ += _si2f * static_cast<float>(*inBuf++);
	}
}


#ifdef ENABLE_SSE
// Interleave two float streams into one int16 stereo stream using SSE magic.
void CDmo2Vst::SSEInterleaveFloatToInt16(const float *inLeft, const float *inRight, int samples)
//----------------------------------------------------------------------------------------------
{
	int16 *outBuf = m_pMixBuffer;
	_asm
	{
		mov eax, inLeft
		mov edx, inRight
		mov ebx, outBuf
		mov ecx, samples
		movss xmm2, _f2si
		xorps xmm0, xmm0
		xorps xmm1, xmm1
		shufps xmm2, xmm2, 0x00
		pxor mm0, mm0
		inc ecx
		shr ecx, 1
mainloop:
		movlps xmm0, [eax]
		movlps xmm1, [edx]
		mulps xmm0, xmm2
		mulps xmm1, xmm2
		add ebx, 8
		cvtps2pi mm0, xmm0	// mm0 = [ x2l | x1l ]
		add eax, 8
		cvtps2pi mm1, xmm1	// mm1 = [ x2r | x1r ]
		add edx, 8
		packssdw mm0, mm1	// mm0 = [x2r|x1r|x2l|x1l]
		pshufw mm0, mm0, 0xD8
		dec ecx
		movq [ebx-8], mm0
		jnz mainloop
		emms
	}
}


// Deinterleave an int16 stereo stream into two float streams using SSE magic.
void CDmo2Vst::SSEDeinterleaveInt16ToFloat(float *outLeft, float *outRight, int samples) const
//--------------------------------------------------------------------------------------------
{
	const int16 *inBuf = m_pMixBuffer;
	_asm {
		mov ebx, inBuf
		mov eax, outLeft
		mov edx, outRight
		mov ecx, samples
		movss xmm7, _si2f
		inc ecx
		shr ecx, 1
		shufps xmm7, xmm7, 0x00
		xorps xmm0, xmm0
		xorps xmm1, xmm1
		xorps xmm2, xmm2
mainloop:
		movq mm0, [ebx]		// mm0 = [x2r|x2l|x1r|x1l]
		add ebx, 8
		pxor mm1, mm1
		pxor mm2, mm2
		punpcklwd mm1, mm0	// mm1 = [x1r|0|x1l|0]
		punpckhwd mm2, mm0	// mm2 = [x2r|0|x2l|0]
		psrad mm1, 16		// mm1 = [x1r|x1l]
		movlps xmm2, [eax]
		psrad mm2, 16		// mm2 = [x2r|x2l]
		cvtpi2ps xmm0, mm1	// xmm0 = [ ? | ? |x1r|x1l]
		dec ecx
		cvtpi2ps xmm1, mm2	// xmm1 = [ ? | ? |x2r|x2l]
		movhps xmm2, [edx]	// xmm2 = [y2r|y1r|y2l|y1l]
		movlhps xmm0, xmm1	// xmm0 = [x2r|x2l|x1r|x1l]
		shufps xmm0, xmm0, 0xD8
		lea eax, [eax+8]
		mulps xmm0, xmm7
		addps xmm0, xmm2
		lea edx, [edx+8]
		movlps [eax-8], xmm0
		movhps [edx-8], xmm0
		jnz mainloop
		emms
	}
}

#endif // ENABLE_SSE


void CDmo2Vst::Process(float * const *inputs, float **outputs, int samples)
//-------------------------------------------------------------------------
{
	if(m_pMixBuffer == nullptr || samples <= 0)
	{
		return;
	}

#ifdef ENABLE_SSE
	if(GetProcSupport() & PROCSUPPORT_SSE)
	{
		SSEInterleaveFloatToInt16(inputs[0], inputs[1], samples);
		m_pMediaProcess->Process(samples * 2 * sizeof(int16), reinterpret_cast<BYTE *>(m_pMixBuffer), m_DataTime, DMO_INPLACE_NORMAL);
		SSEDeinterleaveInt16ToFloat(outputs[0], outputs[1], samples);
	} else
#endif // ENABLE_SSE
	{
		InterleaveFloatToInt16(inputs[0], inputs[1], samples);
		m_pMediaProcess->Process(samples * 2 * sizeof(int16), reinterpret_cast<BYTE *>(m_pMixBuffer), m_DataTime, DMO_INPLACE_NORMAL);
		DeinterleaveInt16ToFloat(outputs[0], outputs[1], samples);
	}

	m_DataTime += Util::muldiv(samples, 10000000, m_nSamplesPerSec);
}


AEffect *DmoToVst(VSTPluginLib *pLib)
//-----------------------------------
{
	WCHAR w[100];
	CLSID clsid;

	MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pLib->szDllPath, -1, (LPWSTR)w, CountOf(w));
	w[99] = 0;
	if (CLSIDFromString(w, &clsid) == S_OK)
	{
		IMediaObject *pMO = nullptr;
		IMediaObjectInPlace *pMOIP = nullptr;
		if ((CoCreateInstance(clsid, nullptr, CLSCTX_INPROC_SERVER, IID_IMediaObject, (VOID **)&pMO) == S_OK) && (pMO))
		{
			if (pMO->QueryInterface(IID_IMediaObjectInPlace, (void **)&pMOIP) != S_OK) pMOIP = nullptr;
		} else pMO = nullptr;
		if ((pMO) && (pMOIP))
		{
			DWORD dwInputs, dwOutputs;

			dwInputs = dwOutputs = 0;
			pMO->GetStreamCount(&dwInputs, &dwOutputs);
			if (dwInputs == 1 && dwOutputs == 1)
			{
				CDmo2Vst *p = new CDmo2Vst(pMO, pMOIP, clsid.Data1);
				return (p) ? p->GetEffect() : nullptr;
			}
		#ifdef DMO_LOG
			Log("%s: Unable to use this DMO\n", pLib->szLibraryName);
		#endif
		}
	#ifdef DMO_LOG
		else Log("%s: Failed to get IMediaObject & IMediaObjectInPlace interfaces\n", pLib->szLibraryName);
	#endif
		if (pMO) pMO->Release();
		if (pMOIP) pMOIP->Release();
	}
	return nullptr;
}


#endif // NO_VST


std::string SNDMIXPLUGIN::GetParamName(PlugParamIndex index) const
//------------------------------------------------------------
{
	CVstPlugin *vstPlug = dynamic_cast<CVstPlugin *>(pMixPlugin);
	if(vstPlug != nullptr)
	{
		return vstPlug->GetParamName(index).GetString();
	} else
	{
		return std::string();
	}
}