#include "stdafx.h"
#include <uuids.h>
#include <dmoreg.h>
#include <medparam.h>
#include "mptrack.h"
#include "mainfrm.h"
#include "vstplug.h"


//#define VST_LOG
//#define ENABLE_BUZZ
#define DMO_LOG

#ifdef ENABLE_BUZZ
#define STATIC_BUILD
#include <machineinterface.h>	// Buzz
AEffect *Buzz2Vst(CMachineInterface *pBuzzMachine, const CMachineInfo *pBuzzInfo);
#endif // ENABLE_BUZZ

AEffect *DmoToVst(PVSTPLUGINLIB pLib);


long VSTCALLBACK CVstPluginManager::MasterCallBack(AEffect *effect,	long opcode, long index, long value, void *ptr, float opt)
//----------------------------------------------------------------------------------------------------------------------------
{
	CVstPluginManager *that = theApp.GetPluginManager();
	if (that)
	{
		return that->VstCallback(effect, opcode, index, value, ptr, opt);
	}
	return 0;
}


BOOL CVstPluginManager::CreateMixPluginProc(PSNDMIXPLUGIN pMixPlugin)
//-------------------------------------------------------------------
{
	CVstPluginManager *that = theApp.GetPluginManager();
	if (that)
	{
		return that->CreateMixPlugin(pMixPlugin);
	}
	return FALSE;
}


CVstPluginManager::CVstPluginManager()
//------------------------------------
{
	m_pVstHead = NULL;
	m_bNeedIdle = FALSE;
	CSoundFile::gpMixPluginCreateProc = CreateMixPluginProc;
	EnumerateDirectXDMOs();
}


CVstPluginManager::~CVstPluginManager()
//-------------------------------------
{
	CSoundFile::gpMixPluginCreateProc = NULL;
	while (m_pVstHead)
	{
		PVSTPLUGINLIB p = m_pVstHead;
		m_pVstHead = m_pVstHead->pNext;
		if (m_pVstHead) m_pVstHead->pPrev = NULL;
		p->pPrev = p->pNext = NULL;
		while ((volatile void *)(p->pPluginsList) != NULL)
		{
			p->pPluginsList->Release();
		}
		delete p;
	}
}


BOOL CVstPluginManager::IsValidPlugin(const VSTPLUGINLIB *pLib)
//-------------------------------------------------------------
{
	PVSTPLUGINLIB p = m_pVstHead;
	while (p)
	{
		if (p == pLib) return TRUE;
		p = p->pNext;
	}
	return FALSE;
}


VOID CVstPluginManager::EnumerateDirectXDMOs()
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

					if (ERROR_SUCCESS == RegQueryValueEx(hksub, NULL, 0, &datatype, (LPBYTE)s, &datasize))
					{
						PVSTPLUGINLIB p = new VSTPLUGINLIB;

						p->pPrev = NULL;
						p->pNext = m_pVstHead;
						p->dwPluginId1 = kDmoMagic;
						p->dwPluginId2 = clsid.Data1;
						p->pPluginsList = NULL;
						p->bIsInstrument = FALSE;
						lstrcpyn(p->szLibraryName, s, sizeof(p->szLibraryName));
						StringFromGUID2(clsid, w, 100);
						WideCharToMultiByte(CP_ACP, 0, w, -1, p->szDllPath, sizeof(p->szDllPath), NULL, NULL);
					#ifdef DMO_LOG
						if (theApp.IsDebug()) Log("Found \"%s\" clsid=%s\n", p->szLibraryName, p->szDllPath);
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

//
// PluginCache format:
// LibraryName = ID100000ID200000
// ID100000ID200000 = FullDllPath


PVSTPLUGINLIB CVstPluginManager::AddPlugin(LPCSTR pszDllPath, BOOL bCache)
//------------------------------------------------------------------------
{
	CHAR s[_MAX_PATH];
	PVSTPLUGINLIB pDup = m_pVstHead;
	while (pDup)
	{
		if (!lstrcmpi(pszDllPath, pDup->szDllPath)) return pDup;
		pDup = pDup->pNext;
	}
	// Look if the plugin info is stored in the PluginCache
	if (bCache)
	{
		LPCSTR pszSection = "PluginCache";
		_splitpath(pszDllPath, NULL, NULL, s, NULL);
		s[63] = 0;
		CString strIds = theApp.GetProfileString(pszSection, s);
		if (strIds.GetLength() >= 16)
		{
			CString strFullPath = theApp.GetProfileString(pszSection, strIds);
			if ((strFullPath) && (strFullPath[0]) && (!lstrcmpi(strFullPath, pszDllPath)))
			{
				PVSTPLUGINLIB p = new VSTPLUGINLIB;
				if (!p) return NULL;
				p->dwPluginId1 = 0;
				p->dwPluginId2 = 0;
				p->bIsInstrument = FALSE;
				p->pPluginsList = NULL;
				lstrcpyn(p->szDllPath, pszDllPath, sizeof(p->szDllPath));
				_splitpath(pszDllPath, NULL, NULL, p->szLibraryName, NULL);
				p->szLibraryName[63] = 0;
				p->pNext = m_pVstHead;
				p->pPrev = NULL;
				if (m_pVstHead) m_pVstHead->pPrev = p;
				m_pVstHead = p;
				// Extract plugin Ids
				for (UINT i=0; i<16; i++)
				{
					UINT n = ((LPCTSTR)strIds)[i] - '0';
					if (n > 9) n = ((LPCTSTR)strIds)[i] + 10 - 'A';
					n &= 0x0f;
					if (i < 8)
					{
						p->dwPluginId1 = (p->dwPluginId1<<4) | n;
					} else
					{
						p->dwPluginId2 = (p->dwPluginId2<<4) | n;
					}
				}
				wsprintf(s, "%s.Flags", (const char *)strIds);
				int infoex = theApp.GetProfileInt(pszSection, s, 0);
				if (infoex&1) p->bIsInstrument = TRUE;
			#ifdef VST_LOG
				Log("Plugin \"%s\" found in PluginCache\n", p->szLibraryName);
			#endif
				return p;
			} else
			{
			#ifdef VST_LOG
				Log("Plugin \"%s\" mismatch in PluginCache: \"%s\" [%s]=\"%s\"\n", s, pszDllPath, (LPCTSTR)strIds, (LPCTSTR)strFullPath);
			#endif
			}
		}
	}

	HINSTANCE hLib = NULL;

	#ifndef _DEBUG
	__try {
		__try {
	#endif
			hLib = LoadLibrary(pszDllPath);
	#ifndef _DEBUG
		} __finally {}
	} __except(EXCEPTION_EXECUTE_HANDLER)
	{
	#ifdef VST_LOG
		Log("Exception in LoadLibrary(\"%s\")!\n", pszDllPath);
	#endif
	}
	#endif
	if ((hLib) && (hLib != INVALID_HANDLE_VALUE))
	{
		BOOL bOk = FALSE;
		PVSTPLUGENTRY pMainProc = (PVSTPLUGENTRY)GetProcAddress(hLib, "main");
	#ifdef ENABLE_BUZZ
		GET_INFO pBuzzGetInfo = (GET_INFO)GetProcAddress(hLib, "GetInfo");
	#endif
		if (pMainProc)
		{
			PVSTPLUGINLIB p = new VSTPLUGINLIB;
			if (!p) return NULL;
			p->dwPluginId1 = 0;
			p->dwPluginId2 = 0;
			p->bIsInstrument = FALSE;
			p->pPluginsList = NULL;
			lstrcpyn(p->szDllPath, pszDllPath, sizeof(p->szDllPath));
			_splitpath(pszDllPath, NULL, NULL, p->szLibraryName, NULL);
			p->szLibraryName[63] = 0;
			p->pNext = m_pVstHead;
			p->pPrev = NULL;
			if (m_pVstHead) m_pVstHead->pPrev = p;
			m_pVstHead = p;
		#ifndef _DEBUG
			__try {
			__try {
		#endif
				AEffect *pEffect = pMainProc(MasterCallBack);
				if ((pEffect) && (pEffect->magic == kEffectMagic)
				 && (pEffect->dispatcher))
				{
					pEffect->dispatcher(pEffect, effOpen, 0,0,0,0);
					p->dwPluginId1 = pEffect->magic;
					p->dwPluginId2 = pEffect->uniqueID;
					if ((pEffect->flags & effFlagsIsSynth) || (!pEffect->numInputs)) p->bIsInstrument = TRUE;
				#ifdef VST_LOG
					int nver = pEffect->dispatcher(pEffect, effGetVstVersion, 0,0, NULL, 0);
					if (!nver) nver = pEffect->version;
					Log("%-20s: v%d.0, %d in, %d out, %2d programs, %2d params, flags=0x%04X realQ=%d offQ=%d\n",
						p->szLibraryName, nver,
						pEffect->numInputs, pEffect->numOutputs,
						pEffect->numPrograms, pEffect->numParams,
						pEffect->flags, pEffect->realQualities, pEffect->offQualities);
				#endif
					pEffect->dispatcher(pEffect, effClose, 0,0,0,0);
					bOk = TRUE;
				}
			#ifndef _DEBUG
				} __finally {}
			} __except(EXCEPTION_EXECUTE_HANDLER)
			{
			#ifdef VST_LOG
				Log("Exception while trying to load plugin \"%s\"!\n", p->szLibraryName);
			#endif
			}
			#endif
			// If OK, write the information in PluginCache
			if (bOk)
			{
				LPCSTR pszSection = "PluginCache";
				wsprintf(s, "%08X%08X", p->dwPluginId1, p->dwPluginId2);
				theApp.WriteProfileString(pszSection, s, pszDllPath);
				theApp.WriteProfileString(pszSection, p->szLibraryName, s);
				strcat(s, ".Flags");
				theApp.WriteProfileInt(pszSection, s, p->bIsInstrument);
			}
		} else
	#ifdef ENABLE_BUZZ
		if (pBuzzGetInfo)
		{
			PVSTPLUGINLIB p = new VSTPLUGINLIB;
			if (!p) return NULL;
			p->dwPluginId1 = kBuzzMagic;
			p->dwPluginId2 = 0;
			p->pPluginsList = NULL;
			lstrcpyn(p->szDllPath, pszDllPath, sizeof(p->szDllPath));
			_splitpath(pszDllPath, NULL, NULL, p->szLibraryName, NULL);
			p->szLibraryName[63] = 0;
			p->pNext = m_pVstHead;
			p->pPrev = NULL;
			if (m_pVstHead) m_pVstHead->pPrev = p;
			m_pVstHead = p;
		#ifndef _DEBUG
			__try {
			__try {
		#endif
			const CMachineInfo *pInfo = pBuzzGetInfo();
			if (pInfo)
			{
				p->dwPluginId2 = pInfo->Version;
				if (pInfo->Name)
				{
					lstrcpyn(p->szLibraryName, pInfo->Name, sizeof(p->szLibraryName));
				}
				bOk = TRUE;
			}
		#ifndef _DEBUG
				} __finally {}
			} __except(EXCEPTION_EXECUTE_HANDLER)
			{
			#ifdef VST_LOG
				Log("Exception while trying to load buzz plugin \"%s\"!\n", p->szLibraryName);
			#endif
			}
		#endif
		} else
	#endif // ENABLE_BUZZ
		{
		#ifdef VST_LOG
			Log("Entry point not found!\n");
		#endif
		}
		#ifndef _DEBUG
		__try {
			__try {
		#endif
			FreeLibrary(hLib);
		#ifndef _DEBUG
			} __finally {}
		} __except(EXCEPTION_EXECUTE_HANDLER)
		{
		#ifdef VST_LOG
			Log("Exception in FreeLibrary(\"%s\")!\n", pszDllPath);
		#endif
		}
		#endif
		return (bOk) ? m_pVstHead : NULL;
	} else
	{
	#ifdef VST_LOG
		Log("LoadLibrary(%s) failed!\n", pszDllPath);
	#endif
	}
	return NULL;
}


BOOL CVstPluginManager::RemovePlugin(PVSTPLUGINLIB pFactory)
//----------------------------------------------------------
{
	PVSTPLUGINLIB p = m_pVstHead;

	while (p)
	{
		if (p == pFactory)
		{
			if (p == m_pVstHead) m_pVstHead = p->pNext;
			if (p->pPrev) p->pPrev->pNext = p->pNext;
			if (p->pNext) p->pNext->pPrev = p->pPrev;
			p->pPrev = p->pNext = NULL;
			BEGIN_CRITICAL();
			__try {
				while ((volatile void *)(p->pPluginsList) != NULL)
				{
					p->pPluginsList->Release();
				}
				delete p;
			} __except(EXCEPTION_EXECUTE_HANDLER)
			{
			#ifdef VST_LOG
				Log("Exception while trying to release plugin \"%s\"!\n", pFactory->szLibraryName);
			#endif
			}
			END_CRITICAL();
			return TRUE;
		}
		p = p->pNext;
	}
	return FALSE;
}


BOOL CVstPluginManager::CreateMixPlugin(PSNDMIXPLUGIN pMixPlugin)
//---------------------------------------------------------------
{
	UINT nMatch=0;
	PVSTPLUGINLIB pFound = NULL;

	if (pMixPlugin)
	{
		PVSTPLUGINLIB p = m_pVstHead;
	
		while (p)
		{
			if (pMixPlugin)
			{
				BOOL b1=FALSE, b2=FALSE;
				if ((p->dwPluginId1 == pMixPlugin->Info.dwPluginId1)
				 && (p->dwPluginId2 == pMixPlugin->Info.dwPluginId2))
				{
					b1 = TRUE;
				}
				if (!strnicmp(p->szLibraryName, pMixPlugin->Info.szLibraryName, 64))
				{
					b2 = TRUE;
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
			}
			p = p->pNext;
		}
	}
	if (pMixPlugin->Info.dwPluginId1 == kDmoMagic)
	{
		if (!pFound) return FALSE;
		AEffect *pEffect = DmoToVst(pFound);
		if ((pEffect) && (pEffect->dispatcher) && (pEffect->magic == kDmoMagic))
		{
			BOOL bOk = FALSE;
			BEGIN_CRITICAL();
			CVstPlugin *pVstPlug = new CVstPlugin(NULL, pFound, pMixPlugin, pEffect);
			if (pVstPlug)
			{
				pVstPlug->Initialize();
				bOk = TRUE;
			}
			END_CRITICAL();
			return bOk;
		}
	}
	if ((!pFound) && (pMixPlugin->Info.szLibraryName[0]))
	{
		CHAR s[_MAX_PATH], dir[_MAX_PATH];
		_splitpath(theApp.GetConfigFileName(), s, dir, NULL, NULL);
		strcat(s, dir);
		int len = strlen(s);
		if ((len > 0) && (s[len-1] != '\\')) strcat(s, "\\");
		strncat(s, "plugins\\", _MAX_PATH-1);
		strncat(s, pMixPlugin->Info.szLibraryName, _MAX_PATH-1);
		strncat(s, ".dll", _MAX_PATH-1);
		pFound = AddPlugin(s);
		if (!pFound)
		{
			LPCSTR pszSection = "PluginCache";
			CString strIds = theApp.GetProfileString(pszSection, pMixPlugin->Info.szLibraryName);
			if (strIds.GetLength() >= 16)
			{
				CString strFullPath = theApp.GetProfileString(pszSection, strIds);
				if ((strFullPath) && (strFullPath[0])) pFound = AddPlugin(strFullPath);
			}
		}
	}
	if (pFound)
	{
		HINSTANCE hLibrary = NULL;
		BOOL bOk = FALSE;

		BEGIN_CRITICAL();
		#ifndef _DEBUG
		__try { __try {
		#endif
		hLibrary = LoadLibrary(pFound->szDllPath);
		if (hLibrary != NULL)
		{
			AEffect *pEffect = NULL;
			PVSTPLUGENTRY pEntryPoint = (PVSTPLUGENTRY)GetProcAddress(hLibrary, "main");
		#ifdef ENABLE_BUZZ
			GET_INFO pBuzzGetInfo = (GET_INFO)GetProcAddress(hLibrary, "GetInfo");
			CREATE_MACHINE pBuzzCreateMachine = (CREATE_MACHINE)GetProcAddress(hLibrary, "CreateMachine");
		#endif
			if (pEntryPoint)
			{
				pEffect = pEntryPoint(MasterCallBack);
			} else
		#ifdef ENABLE_BUZZ
			if ((pBuzzCreateMachine) && (pBuzzGetInfo))
			{
				CMachineInterface *pBuzzMachine = pBuzzCreateMachine();
				const CMachineInfo *pBuzzInfo = pBuzzGetInfo();
				if ((pBuzzMachine) && (pBuzzInfo) && (pBuzzInfo->Type == MT_EFFECT))
				{
					pEffect = Buzz2Vst(pBuzzMachine, pBuzzInfo);
				}
			} else
		#endif
			{
			#ifdef VST_LOG
				Log("Entry point not found! (handle=%08X)\n", hLibrary);
			#endif
			}
			if ((pEffect) && (pEffect->dispatcher)
			 && ((pEffect->magic == kEffectMagic) || (pEffect->magic == kBuzzMagic)))
			{
				bOk = TRUE;
				if ((pEffect->flags & effFlagsIsSynth) || (!pEffect->numInputs))
				{
					if (!pFound->bIsInstrument)
					{
						LPCSTR pszSection = "PluginCache";
						CHAR s[64];
						pFound->bIsInstrument = TRUE;
						wsprintf(s, "%08X%08X.Flags", pFound->dwPluginId1, pFound->dwPluginId2);
						theApp.WriteProfileInt(pszSection, s, 1);
					}
				}
				CVstPlugin *pVstPlug = new CVstPlugin(hLibrary, pFound, pMixPlugin, pEffect);
				if (pVstPlug) pVstPlug->Initialize();
			}
		} else
		{
		#ifdef VST_LOG
			Log("LoadLibrary(\"%s\") failed!\n", pFound->szDllPath);
		#endif
		}
		if ((!bOk) && (hLibrary)) FreeLibrary(hLibrary);
		#ifndef _DEBUG
		} __finally {}
		} __except(EXCEPTION_EXECUTE_HANDLER)
		{
		#ifdef VST_LOG
			Log("Exception while trying to create plugin \"%s\"!\n", pFound->szLibraryName);
		#endif
		}
		#endif
		END_CRITICAL();
		return bOk;
	} else
	{
	#ifdef VST_LOG
		Log("Unknown plugin\n");
	#endif
	}
	return FALSE;
}


VOID CVstPluginManager::OnIdle()
//------------------------------
{
	m_bNeedIdle = FALSE;
	PVSTPLUGINLIB pFactory = m_pVstHead;
	while (pFactory)
	{
		CVstPlugin *p = pFactory->pPluginsList;
		while (p)
		{
			if ((p->m_pEditor) && (p->m_pEditor->m_hWnd))
			{
				p->Dispatch(effEditIdle, 0,0, NULL, 0);
			}
			p = p->m_pNext;
		}
		pFactory = pFactory->pNext;
	}
	m_bNeedIdle = FALSE;
}


long CVstPluginManager::VstCallback(AEffect *effect, long opcode, long index, long value, void *ptr, float opt)
//-------------------------------------------------------------------------------------------------------------
{
	switch(opcode)
	{
	case audioMasterAutomate:	m_bNeedIdle = TRUE; return 0;
	case audioMasterVersion:	return 2;
	case audioMasterIdle:
		m_bNeedIdle = TRUE;
		break;
	case audioMasterPinConnected:
	#ifdef VST_LOG
		Log("audioMasterPinConnected(ndx=%d, value=%d)\n", index, value);
	#endif
		if (effect)
		{
			OnIdle();
			if (value)
			{
				return ((index < effect->numOutputs) && (index < 2)) ? 0 : 1;
			} else
			{
				return ((index < effect->numInputs) && (index < 2)) ? 0 : 1;
			}
		}
		break;

	case audioMasterWantMidi:	return 1;

	// Unknown codes:
	#ifdef VST_LOG
	default:
		Log("VstCallback(op=%d, index=%d, value=%d)\n", opcode, index, value);
	#endif
	}
	return 0;
}


/////////////////////////////////////////////////////////////////////////////////
//
// CSelectPluginDlg
//

BEGIN_MESSAGE_MAP(CSelectPluginDlg, CDialog)
	ON_NOTIFY(TVN_SELCHANGED,	IDC_TREE1, OnSelChanged)
	ON_NOTIFY(NM_DBLCLK,		IDC_TREE1, OnSelDblClk)
	ON_COMMAND(IDC_BUTTON1,		OnAddPlugin)
	ON_COMMAND(IDC_BUTTON2,		OnRemovePlugin)
END_MESSAGE_MAP()

void CSelectPluginDlg::DoDataExchange(CDataExchange* pDX)
//-------------------------------------------------------
{
	DDX_Control(pDX, IDC_TREE1, m_treePlugins);
}


CSelectPluginDlg::CSelectPluginDlg(PSNDMIXPLUGIN pPlugin, CWnd *parent):CDialog(IDD_SELECTMIXPLUGIN, parent)
//----------------------------------------------------------------------------------------------------------
{
	m_pPlugin = pPlugin;
}


BOOL CSelectPluginDlg::OnInitDialog()
//-----------------------------------
{
	DWORD dwRemove = TVS_EDITLABELS|TVS_SINGLEEXPAND;
	DWORD dwAdd = TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS;

	CDialog::OnInitDialog();
	m_treePlugins.ModifyStyle(dwRemove, dwAdd);
	m_treePlugins.SetImageList(CMainFrame::GetMainFrame()->GetImageList(), TVSIL_NORMAL);
	UpdatePluginsList();
	OnSelChanged(NULL, NULL);
	return TRUE;
}


VOID CSelectPluginDlg::OnOK()
//---------------------------
{
	BOOL bChanged = FALSE;
	CVstPluginManager *pManager = theApp.GetPluginManager();
	PVSTPLUGINLIB pNewPlug = (PVSTPLUGINLIB)m_treePlugins.GetItemData(m_treePlugins.GetSelectedItem());
	PVSTPLUGINLIB pFactory = NULL;
	CVstPlugin *pCurrentPlugin = NULL;
	if (m_pPlugin) pCurrentPlugin = (CVstPlugin *)m_pPlugin->pMixPlugin;
	if ((pManager) && (pManager->IsValidPlugin(pNewPlug))) pFactory = pNewPlug;
	// Plugin selected
	if (pFactory)
	{
		if ((!pCurrentPlugin) || (pCurrentPlugin->GetPluginFactory() != pFactory))
		{
			BEGIN_CRITICAL();
			if (pCurrentPlugin) pCurrentPlugin->Release();
			// Just in case...
			m_pPlugin->pMixPlugin = NULL;
			m_pPlugin->pMixState = NULL;
			// Remove old state
			m_pPlugin->nPluginDataSize = 0;
			if (m_pPlugin->pPluginData) delete m_pPlugin->pPluginData;
			m_pPlugin->pPluginData = NULL;
			// Initialize plugin info
			memset(&m_pPlugin->Info, 0, sizeof(m_pPlugin->Info));
			m_pPlugin->Info.dwPluginId1 = pFactory->dwPluginId1;
			m_pPlugin->Info.dwPluginId2 = pFactory->dwPluginId2;
			switch(m_pPlugin->Info.dwPluginId2)
			{
			// Enable drymix by default for these known plugins
			case 'Scop':
				m_pPlugin->Info.dwInputRouting |= MIXPLUG_INPUTF_WETMIX;
				break;
			}
			lstrcpyn(m_pPlugin->Info.szName, pFactory->szLibraryName, 32);
			lstrcpyn(m_pPlugin->Info.szLibraryName, pFactory->szLibraryName, 64);
			END_CRITICAL();
			// Now, create the new plugin
			if (pManager)
			{
				pManager->CreateMixPlugin(m_pPlugin);
				if (m_pPlugin->pMixPlugin)
				{
					CHAR s[128];
					CVstPlugin *p = (CVstPlugin *)m_pPlugin->pMixPlugin;
					s[0] = 0;
					if ((p->GetDefaultEffectName(s)) && (s[0]))
					{
						s[31] = 0;
						lstrcpyn(m_pPlugin->Info.szName, s, 32);
					}
				}
			}
			bChanged = TRUE;
		}
	} else
	// No effect
	{
		BEGIN_CRITICAL();
		if (pCurrentPlugin)
		{
			pCurrentPlugin->Release();
			bChanged = TRUE;
		}
		// Just in case...
		m_pPlugin->pMixPlugin = NULL;
		m_pPlugin->pMixState = NULL;
		// Remove old state
		m_pPlugin->nPluginDataSize = 0;
		if (m_pPlugin->pPluginData) delete m_pPlugin->pPluginData;
		m_pPlugin->pPluginData = NULL;
		// Clear plugin info
		memset(&m_pPlugin->Info, 0, sizeof(m_pPlugin->Info));
		END_CRITICAL();
	}
	if (bChanged)
		CDialog::OnOK();
	else
		CDialog::OnCancel();
}


VOID CSelectPluginDlg::UpdatePluginsList()
//----------------------------------------
{
	TVINSERTSTRUCT tvis;
	CVstPluginManager *pManager = theApp.GetPluginManager();
	HTREEITEM cursel, hDmo, hVst, hSynth;

	m_treePlugins.SetRedraw(FALSE);
	m_treePlugins.DeleteAllItems();
	memset(&tvis, 0, sizeof(tvis));
	tvis.hParent = TVI_ROOT;
	tvis.hInsertAfter = TVI_FIRST;
	tvis.item.mask = TVIF_IMAGE | TVIF_PARAM | TVIF_TEXT | TVIF_SELECTEDIMAGE | TVIF_TEXT;
	tvis.item.pszText = "VST Instruments";
	tvis.item.iImage = IMAGE_FOLDER;
	tvis.item.iSelectedImage = IMAGE_FOLDER;
	tvis.item.lParam = NULL;
	hSynth = m_treePlugins.InsertItem(&tvis);
	tvis.item.pszText = "DirectX Media Audio Effects";
	hDmo = m_treePlugins.InsertItem(&tvis);
	tvis.item.pszText = "VST Audio Effects";
	hVst = m_treePlugins.InsertItem(&tvis);
	tvis.item.pszText = " No effect (default)";
	tvis.item.iImage = IMAGE_WAVEOUT;
	tvis.item.iSelectedImage = IMAGE_WAVEOUT;
	cursel = m_treePlugins.InsertItem(&tvis);
	if (pManager)
	{
		PVSTPLUGINLIB pCurrent = NULL;
		PVSTPLUGINLIB p = pManager->GetFirstPlugin();
		while (p)
		{
			if (p->dwPluginId1 == kDmoMagic)
			{
				tvis.hParent = hDmo;
			} else
			{
				tvis.hParent = (p->bIsInstrument) ? hSynth : hVst;
			}
			tvis.hInsertAfter = TVI_SORT;
			tvis.item.pszText = p->szLibraryName;
			tvis.item.lParam = (LPARAM)p;
			HTREEITEM h = m_treePlugins.InsertItem(&tvis);
			if (m_pPlugin)
			{
				if (m_pPlugin->pMixPlugin)
				{
					CVstPlugin *pVstPlug = (CVstPlugin *)m_pPlugin->pMixPlugin;
					if (pVstPlug->GetPluginFactory() == p) pCurrent = p;
				} else
				if (!pCurrent)
				{
					if ((p->dwPluginId1 == m_pPlugin->Info.dwPluginId1)
					 && (p->dwPluginId2 == m_pPlugin->Info.dwPluginId2))
					{
						pCurrent = p;
					}
				}
			}
			if (pCurrent == p) cursel = h;
			p = p->pNext;
		}
	}
	m_treePlugins.SetRedraw(TRUE);
	if (cursel)
	{
		m_treePlugins.SelectItem(cursel);
		m_treePlugins.SetItemState(cursel, TVIS_BOLD, TVIS_BOLD);
		m_treePlugins.EnsureVisible(cursel);
	}
}


VOID CSelectPluginDlg::OnSelDblClk(NMHDR *, LRESULT *result)
//----------------------------------------------------------
{
	HTREEITEM hSel = m_treePlugins.GetSelectedItem();
	int nImage, nSelectedImage;
	m_treePlugins.GetItemImage(hSel, nImage, nSelectedImage);
	if ((hSel) && (nImage == IMAGE_WAVEOUT)) OnOK();
	if (result) *result = 0;
}


VOID CSelectPluginDlg::OnSelChanged(NMHDR *, LRESULT *result)
//-----------------------------------------------------------
{
	CVstPluginManager *pManager = theApp.GetPluginManager();
	PVSTPLUGINLIB pPlug = (PVSTPLUGINLIB)m_treePlugins.GetItemData(m_treePlugins.GetSelectedItem());
	if ((pManager) && (pManager->IsValidPlugin(pPlug)))
	{
		SetDlgItemText(IDC_TEXT1, pPlug->szDllPath);
	} else
	{
		SetDlgItemText(IDC_TEXT1, "");
	}
	if (result) *result = 0;
}

#define MAX_FILEOPEN_BUFSIZE	32000

VOID CSelectPluginDlg::OnAddPlugin()
//----------------------------------
{
	CHAR *pszFileNames;
	CFileDialog dlg(TRUE, ".dll", NULL, 
					OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_PATHMUSTEXIST|OFN_FORCESHOWHIDDEN|OFN_ALLOWMULTISELECT,
					"VST Plugins (*.dll)|*.dll||",
					this);
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if (pMainFrm)
	{
		LPCSTR pszDir = pMainFrm->GetPluginsDir();
		if (pszDir[0])
		{
			dlg.m_ofn.lpstrInitialDir = pszDir;
		}
	}
	pszFileNames = new CHAR[MAX_FILEOPEN_BUFSIZE];
	if (!pszFileNames) return;
	pszFileNames[0] = 0;
	pszFileNames[1] = 0;
	dlg.m_ofn.lpstrFile = pszFileNames;
	dlg.m_ofn.nMaxFile = MAX_FILEOPEN_BUFSIZE;
	if (dlg.DoModal() == IDOK)
	{
		CHAR s[_MAX_PATH], sdir[_MAX_PATH];
		CVstPluginManager *pManager = theApp.GetPluginManager();
		pszFileNames[MAX_FILEOPEN_BUFSIZE-1] = 0;
		POSITION pos = dlg.GetStartPosition();
		BOOL bOk = FALSE;
		int n = 0;
		while (pos != NULL)
		{
			CString str = dlg.GetNextPathName(pos);
			if (!n)
			{
				_splitpath(str, s, sdir, NULL, NULL);
				strcat(s, sdir);
				if (pMainFrm) pMainFrm->SetPluginsDir(s);
			}
			n++;
			if ((pManager) && (pManager->AddPlugin(str, FALSE)))
			{
				bOk = TRUE;
			}
		}
		if (bOk)
		{
			UpdatePluginsList();
		} else
		{
			MessageBox("This file is not a valid VST-Plugin", NULL, MB_ICONERROR|MB_OK);
		}
	}
	dlg.m_ofn.lpstrFile = NULL;
	dlg.m_ofn.nMaxFile = 0;
	delete pszFileNames;
}


VOID CSelectPluginDlg::OnRemovePlugin()
//-------------------------------------
{
	CVstPluginManager *pManager = theApp.GetPluginManager();
	PVSTPLUGINLIB pPlug = (PVSTPLUGINLIB)m_treePlugins.GetItemData(m_treePlugins.GetSelectedItem());
	if ((pManager) && (pPlug))
	{
		pManager->RemovePlugin(pPlug);
		UpdatePluginsList();
	}
}


//////////////////////////////////////////////////////////////////////////////
//
// CVstPlugin
//

CVstPlugin::CVstPlugin(HMODULE hLibrary, PVSTPLUGINLIB pFactory, PSNDMIXPLUGIN pMixStruct, AEffect *pEffect)
//----------------------------------------------------------------------------------------------------------
{
	m_hLibrary = hLibrary;
	m_nRefCount = 1;
	m_pPrev = NULL;
	m_pNext = NULL;
	m_pFactory = pFactory;
	m_pMixStruct = pMixStruct;
	m_pEffect = pEffect;
	m_pInputs = NULL;
	m_pOutputs = NULL;
	m_pEditor = NULL;
	m_nInputs = m_nOutputs = 0;
	m_nEditorX = m_nEditorY = -1;
	m_pEvList = NULL;
	// Insert ourselves in the beginning of the list
	if (m_pFactory)
	{
		m_pNext = m_pFactory->pPluginsList;
		if (m_pFactory->pPluginsList)
		{
			m_pFactory->pPluginsList->m_pPrev = this;
		}
		m_pFactory->pPluginsList = this;
	}
	m_MixState.dwFlags = 0;
	m_MixState.nVolDecayL = 0;
	m_MixState.nVolDecayR = 0;
	m_MixState.pMixBuffer = (int *)((((DWORD)m_MixBuffer)+7)&~7);
	m_MixState.pOutBufferL = (float *)((((DWORD)&m_FloatBuffer[0])+7)&~7);
	m_MixState.pOutBufferR = (float *)((((DWORD)&m_FloatBuffer[MIXBUFFERSIZE])+7)&~7);
	m_pTempBuffer = (float *)((((DWORD)&m_FloatBuffer[MIXBUFFERSIZE*2])+7)&~7);
	m_nSampleRate = CSoundFile::gdwMixingFreq;
	memset(m_MidiCh, 0, sizeof(m_MidiCh));
}


void CVstPlugin::Initialize()
//---------------------------
{
	if (!m_pEvList)
	{
		m_pEvList = (VstEvents *)new char[sizeof(VstEvents)+sizeof(VstEvent*)*VSTEVENT_QUEUE_LEN];
	}
	if (m_pEvList)
	{
		m_pEvList->numEvents = 0;
		m_pEvList->reserved = 0;
	}
	memset(m_MidiCh, 0, sizeof(m_MidiCh));
	Dispatch(effOpen, 0, 0, NULL, 0);
	CVstPlugin::Init(m_nSampleRate, TRUE);
	m_bIsVst2 = (CVstPlugin::Dispatch(effGetVstVersion, 0,0, NULL, 0) >= 2) ? TRUE : FALSE;
	if (m_bIsVst2)
	{
		Dispatch(effConnectInput, 0, 1, NULL, 0);
		if (m_pEffect->numInputs > 1) Dispatch(effConnectInput, 1, 1, NULL, 0);
		Dispatch(effConnectOutput, 0, 1, NULL, 0);
		if (m_pEffect->numOutputs > 1) Dispatch(effConnectOutput, 1, 1, NULL, 0);
	}
	if (m_pEffect->numPrograms > 0)
	{
		Dispatch(effSetProgram, 0, 0, NULL, 0);
	}
	m_nInputs = (m_pEffect->numInputs > 1) ? m_pEffect->numInputs : 2;
	m_nOutputs = (m_pEffect->numOutputs > 1) ? m_pEffect->numOutputs : 2;
	m_pInputs = (float **)new char[m_nInputs*sizeof(float *)];
	m_pOutputs = (float **)new char[m_nOutputs*sizeof(float *)];
	for (UINT iIn=0; iIn<m_nInputs; iIn++)
	{
		m_pInputs[iIn] = (iIn & 1) ? m_MixState.pOutBufferR : m_MixState.pOutBufferL;
	}
#ifdef VST_LOG
	Log("%s: vst ver %d.0, flags=%04X, %d programs, %d parameters\n",
		m_pFactory->szLibraryName, (m_bIsVst2) ? 2 : 1, m_pEffect->flags,
		m_pEffect->numPrograms, m_pEffect->numParams);
#endif
	// Update Mix structure
	if (m_pMixStruct)
	{
		m_pMixStruct->pMixPlugin = this;
		m_pMixStruct->pMixState = &m_MixState;
	}
}


CVstPlugin::~CVstPlugin()
//-----------------------
{
#ifdef VST_LOG
	Log("~CVstPlugin: m_nRefCount=%d\n", m_nRefCount);
#endif
	// First thing to do, if we don't want to hang in a loop
	if ((m_pFactory) && (m_pFactory->pPluginsList == this)) m_pFactory->pPluginsList = m_pNext;
	if (m_pMixStruct)
	{
		m_pMixStruct->pMixPlugin = NULL;
		m_pMixStruct->pMixState = NULL;
		m_pMixStruct = NULL;
	}
	if (m_pNext) m_pNext->m_pPrev = m_pPrev;
	if (m_pPrev) m_pPrev->m_pNext = m_pNext;
	m_pPrev = NULL;
	m_pNext = NULL;
	if (m_pEditor)
	{
		if (m_pEditor->m_hWnd) m_pEditor->OnClose();
		if ((volatile void *)m_pEditor) delete m_pEditor;
		m_pEditor = NULL;
	}
	if (m_bIsVst2)
	{
		Dispatch(effConnectInput, 0, 0, NULL, 0);
		if (m_pEffect->numInputs > 1) Dispatch(effConnectInput, 1, 0, NULL, 0);
		Dispatch(effConnectOutput, 0, 0, NULL, 0);
		if (m_pEffect->numOutputs > 1) Dispatch(effConnectOutput, 1, 0, NULL, 0);
	}
	CVstPlugin::Dispatch(effMainsChanged, 0,0, NULL, 0);
	CVstPlugin::Dispatch(effClose, 0, 0, NULL, 0);
	m_pFactory = NULL;
	if (m_pInputs)
	{
		delete m_pInputs;
		m_pInputs = NULL;
	}
	if (m_pOutputs)
	{
		delete m_pOutputs;
		m_pOutputs = NULL;
	}
	if (m_pEvList)
	{
		delete (char *)m_pEvList;
		m_pEvList = NULL;
	}
	if (m_hLibrary)
	{
		FreeLibrary(m_hLibrary);
		m_hLibrary = NULL;
	}
}


int CVstPlugin::Release()
//-----------------------
{
	if (!(--m_nRefCount))
	{
	__try { __try{
		delete this;
		} __finally {}
		} __except(EXCEPTION_EXECUTE_HANDLER)
		{
		#ifdef VST_LOG
			Log("Exception while destroying plugin!\n");
		#endif
		}
		return 0;
	}
	return m_nRefCount;
}


VOID CVstPlugin::GetPluginType(LPSTR pszType)
//-------------------------------------------
{
	pszType[0] = 0;
	if (m_pEffect)
	{
		if (m_pEffect->numInputs < 1) strcpy(pszType, "No input"); else
		if (m_pEffect->numInputs == 1) strcpy(pszType, "Mono-In"); else
		strcpy(pszType, "Stereo-In");
		strcat(pszType, ", ");
		if (m_pEffect->numOutputs < 1) strcat(pszType, "No output"); else
		if (m_pEffect->numInputs == 1) strcat(pszType, "Mono-Out"); else
		strcat(pszType, "Stereo-Out");
	}
}


BOOL CVstPlugin::HasEditor()
//--------------------------
{
	if ((m_pEffect) && (m_pEffect->flags & effFlagsHasEditor))
	{
		return TRUE;
	}
	return FALSE;
}


UINT CVstPlugin::GetNumPrograms()
//-------------------------------
{
	if ((m_pEffect) && (m_pEffect->numPrograms > 0))
	{
		return m_pEffect->numPrograms;
	}
	return 0;
}


UINT CVstPlugin::GetNumParameters()
//---------------------------------
{
	if ((m_pEffect) && (m_pEffect->numParams > 0))
	{
		return m_pEffect->numParams;
	}
	return 0;
}


long CVstPlugin::Dispatch(long opCode, long index, long value, void *ptr, float opt)
//----------------------------------------------------------------------------------
{
	long lresult = 0;

	__try { __try {
	if ((m_pEffect) && (m_pEffect->dispatcher))
	{
		lresult = m_pEffect->dispatcher(m_pEffect, opCode, index, value, ptr, opt);
	}
	} __finally {}
	} __except(EXCEPTION_EXECUTE_HANDLER)
	{
	#ifdef VST_LOG
		Log("Exception Dispatch(%d) (Plugin=\"%s\")!\n", opCode, m_pFactory->szLibraryName);
	#endif
	}
	return lresult;
}


UINT CVstPlugin::GetCurrentProgram()
//----------------------------------
{
	if ((m_pEffect) && (m_pEffect->numPrograms > 0))
	{
		return Dispatch(effGetProgram, 0, 0, NULL, 0);
	}
	return 0;
}


VOID CVstPlugin::SetCurrentProgram(UINT nIndex)
//---------------------------------------------
{
	if ((m_pEffect) && (m_pEffect->numPrograms > 0))
	{
		if (nIndex < (UINT)m_pEffect->numPrograms)
		{
			Dispatch(effSetProgram, 0, nIndex, NULL, 0);
		}
	}
}


FLOAT CVstPlugin::GetParameter(UINT nIndex)
//-----------------------------------------
{
	FLOAT fResult = 0;
	if ((m_pEffect) && ((long)nIndex < m_pEffect->numParams) && (m_pEffect->getParameter))
	{
		__try { __try {
			fResult = m_pEffect->getParameter(m_pEffect, nIndex);
		} __finally {}
		} __except(EXCEPTION_EXECUTE_HANDLER) {}
	}
	return fResult;
}


VOID CVstPlugin::SetParameter(UINT nIndex, FLOAT fValue)
//------------------------------------------------------
{
	__try { __try {
	if ((m_pEffect) && ((long)nIndex < m_pEffect->numParams) && (m_pEffect->setParameter))
	{
		if ((fValue >= 0.0f) && (fValue <= 1.0f))
			m_pEffect->setParameter(m_pEffect, nIndex, fValue);
	}
	} __finally {}
	} __except(EXCEPTION_EXECUTE_HANDLER)
	{
	#ifdef VST_LOG
		Log("GPF in SetParameter(%d, 0.%03d) (Plugin=%s)\n", 
			nIndex, (int)(fValue*1000), m_pFactory->szLibraryName);
	#endif
	}
}


VOID CVstPlugin::GetParamName(UINT nIndex, LPSTR pszName, UINT cbSize)
//--------------------------------------------------------------------
{
	if ((!pszName) || (!cbSize)) return;
	pszName[0] = 0;
	if ((m_pEffect) && (m_pEffect->numParams > 0) && (nIndex < (UINT)m_pEffect->numParams))
	{
		CHAR s[32];
		s[0] = 0;
		Dispatch(effGetParamName, nIndex, 0, s, 0);
		s[31] = 0;
		lstrcpyn(pszName, s, cbSize);
	}
}


VOID CVstPlugin::GetParamLabel(UINT nIndex, LPSTR pszLabel)
//---------------------------------------------------------
{
	pszLabel[0] = 0;
	if ((m_pEffect) && (m_pEffect->numParams > 0) && (nIndex < (UINT)m_pEffect->numParams))
	{
		Dispatch(effGetParamLabel, nIndex, 0, pszLabel, 0);
	}
}


VOID CVstPlugin::GetParamDisplay(UINT nIndex, LPSTR pszDisplay)
//-------------------------------------------------------------
{
	pszDisplay[0] = 0;
	if ((m_pEffect) && (m_pEffect->numParams > 0) && (nIndex < (UINT)m_pEffect->numParams))
	{
		Dispatch(effGetParamDisplay, nIndex, 0, pszDisplay, 0);
	}
}


BOOL CVstPlugin::GetDefaultEffectName(LPSTR pszName)
//--------------------------------------------------
{
	pszName[0] = 0;
	if (m_bIsVst2)
	{
		Dispatch(effGetEffectName, 0,0, pszName, 0);
		return TRUE;
	}
	return FALSE;
}


void CVstPlugin::Init(unsigned long nFreq, int bReset)
//----------------------------------------------------
{
	if ((bReset) || (nFreq != m_nSampleRate))
	{
		__try { __try {
		m_nSampleRate = nFreq;
		m_MixState.nVolDecayL = 0;
		m_MixState.nVolDecayR = 0;
		CVstPlugin::Dispatch(effMainsChanged, 0, FALSE, NULL, 0);
		CVstPlugin::Dispatch(effSetSampleRate, 0, 0, NULL, (float)m_nSampleRate);
		CVstPlugin::Dispatch(effSetBlockSize, 0, MIXBUFFERSIZE, NULL, 0);
		CVstPlugin::Dispatch(effMainsChanged, 0, TRUE, NULL, 0);
		} __finally {}
		} __except(EXCEPTION_EXECUTE_HANDLER)
		{
		#ifdef VST_LOG
			Log("GPF in Init() (Plugin=%s)\n", m_pFactory->szLibraryName);
		#endif
		}
	}
}


void CVstPlugin::Process(float *pOutL, float *pOutR, unsigned long nSamples)
//--------------------------------------------------------------------------
{
	if ((m_pEffect) && (m_pEffect->dispatcher) && (m_pEvList) && (m_pEvList->numEvents > 0))
	{
		m_pEffect->dispatcher(m_pEffect, effProcessEvents, 0, 0, m_pEvList, 0);
	}
	if ((m_pEffect) && (m_pEffect->process) && (m_pInputs) && (m_pOutputs) && (m_pMixStruct))
	{
		if (m_pEffect->numInputs == 1)
		{
			for (UINT i=0; i<nSamples; i++)
			{
				m_MixState.pOutBufferL[i] = 0.5f*m_MixState.pOutBufferL[i] + 0.5f*m_MixState.pOutBufferR[i];
			}
		}
		if (m_pEffect->numOutputs == 1)
		{
			memset(m_pTempBuffer, 0, nSamples*sizeof(float));
			m_pOutputs[0] = m_pTempBuffer;
		} else
		{
			for (UINT iOut=0; iOut<m_nOutputs; iOut++)
			{
				m_pOutputs[iOut] = (iOut & 1) ? pOutR : pOutL;
			}
		}
		m_pEffect->process(m_pEffect, m_pInputs, m_pOutputs, nSamples);
		if (m_pEffect->numOutputs == 1)
		{
			for (UINT i=0; i<nSamples; i++)
			{
				pOutL[i] += m_pTempBuffer[i];
				pOutR[i] += m_pTempBuffer[i];
			}
		}
		if ((m_pEffect->numInputs < 1) || (m_pMixStruct->Info.dwInputRouting & MIXPLUG_INPUTF_WETMIX))
		{
			for (UINT i=0; i<nSamples; i++)
			{
				pOutL[i] += m_MixState.pOutBufferL[i];
				pOutR[i] += m_MixState.pOutBufferR[i];
			}
		}
	}
	// Clear VST events
	if ((m_pEvList) && (m_pEvList->numEvents > 0))
	{
		m_pEvList->numEvents = 0;
	}
}


void CVstPlugin::MidiSend(DWORD dwMidiCode)
//-----------------------------------------
{
	if ((m_pEvList) && (m_pEvList->numEvents < VSTEVENT_QUEUE_LEN))
	{
		VstMidiEvent *pev = &m_ev_queue[m_pEvList->numEvents];
		m_pEvList->events[m_pEvList->numEvents] = (VstEvent *)pev;
		pev->type = kVstMidiType;
		pev->byteSize = 24;
		pev->deltaFrames = 0;
		pev->flags = 0;
		pev->noteLength = 0;
		pev->noteOffset = 0;
		pev->detune = 0;
		pev->noteOffVelocity = 0;
		pev->reserved1 = 0;
		pev->reserved2 = 0;
		*(DWORD *)pev->midiData = dwMidiCode;
		m_pEvList->numEvents++;
	#ifdef VST_LOG
		Log("Sending Midi %02X.%02X.%02X\n", pev->midiData[0]&0xff, pev->midiData[1]&0xff, pev->midiData[2]&0xff);
	#endif
	}
#ifdef VST_LOG
	else Log("VST Event queue overflow!\n");
#endif
}


void CVstPlugin::MidiCommand(UINT nMidiCh, UINT nMidiProg, UINT note, UINT vol)
//-----------------------------------------------------------------------------
{
	UINT nCh = (nMidiCh-1) & 0x0f;
	PVSTINSTCH pCh = &m_MidiCh[nCh];
	DWORD dwMidiCode = 0x90|nCh;

	// Program change ?
	if ((pCh->nProgram != nMidiProg) && (nMidiCh != 10) && (nMidiProg < 0x80))
	{
		pCh->nProgram = nMidiProg;
		MidiSend((nMidiProg<<8)|(0xC0|nCh));
	}
	// Note Off
	if (note > 0x80)
	{
		for (UINT i=0; i<128; i++)
		{
			UINT mask = 1<<(i&7);
			if (pCh->uNoteOnMap[i>>3] & mask)
			{
				pCh->uNoteOnMap[i>>3] &= ~mask;
				MidiSend(dwMidiCode|(i<<8));
			}
		}
	} else
	// Note On
	if (note > 0)
	{
		note--;
		vol *= 2;
		if (vol < 1) vol = 1;
		if (vol > 0x7f) vol = 0x7f;
		pCh->uNoteOnMap[note>>3] |= 1<<(note&7);
		MidiSend(dwMidiCode|(note<<8)|(vol<<16));
	}
}


void CVstPlugin::SetZxxParameter(UINT nParam, UINT nValue)
//--------------------------------------------------------
{
	FLOAT fValue = nValue / 127.0f;
	SetParameter(nParam, fValue);
}


void CVstPlugin::SaveAllParameters()
//----------------------------------
{
	if ((m_pEffect) && (m_pMixStruct))
	{
		if ((m_pEffect->flags & effFlagsProgramChunks)
		 && (Dispatch(effIdentify, 0,0, NULL, 0) == 'NvEf'))
		{
			PVOID p = NULL;
			LONG nByteSize = Dispatch(effGetChunk, 0,0, &p, 0);
			if ((nByteSize > 0) && (p))
			{
				if ((m_pMixStruct->pPluginData) && (m_pMixStruct->nPluginDataSize >= (UINT)(nByteSize+4)))
				{
					m_pMixStruct->nPluginDataSize = nByteSize+4;
				} else
				{
					if (m_pMixStruct->pPluginData) delete m_pMixStruct->pPluginData;
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
		// Chunks not supported: save parameters
		UINT nParams = (m_pEffect->numParams > 0) ? m_pEffect->numParams : 0;
		UINT nLen = nParams * sizeof(FLOAT);
		if (!nLen) return;
		nLen += 4;
		if ((m_pMixStruct->pPluginData) && (m_pMixStruct->nPluginDataSize >= nLen))
		{
			m_pMixStruct->nPluginDataSize = nLen;
		} else
		{
			if (m_pMixStruct->pPluginData) delete m_pMixStruct->pPluginData;
			m_pMixStruct->nPluginDataSize = 0;
			m_pMixStruct->pPluginData = new char[nLen];
			if (m_pMixStruct->pPluginData)
			{
				m_pMixStruct->nPluginDataSize = nLen;
			}
		}
		if (m_pMixStruct->pPluginData)
		{
			FLOAT *p = (FLOAT *)m_pMixStruct->pPluginData;
			*(ULONG *)p = 0;
			p++;
			for (UINT i=0; i<nParams; i++)
			{
				p[i] = GetParameter(i);
			}
		}
	}
}


void CVstPlugin::RestoreAllParameters()
//-------------------------------------
{
	if ((m_pEffect) && (m_pMixStruct) && (m_pMixStruct->pPluginData) && (m_pMixStruct->nPluginDataSize >= 4))
	{
		UINT nParams = (m_pEffect->numParams > 0) ? m_pEffect->numParams : 0;
		UINT nLen = nParams * sizeof(FLOAT);
		ULONG nType = *(ULONG *)m_pMixStruct->pPluginData;

		if ((Dispatch(effIdentify, 0,0, NULL, 0) == 'NvEf') && (nType == 'NvEf'))
		{
			PVOID p = NULL;
			Dispatch(effGetChunk, 0,0, &p, 0);
			Dispatch(effSetChunk, 0, m_pMixStruct->nPluginDataSize-4, ((BYTE *)m_pMixStruct->pPluginData)+4, 0);
		} else
		{
			FLOAT *p = (FLOAT *)m_pMixStruct->pPluginData;
			if (m_pMixStruct->nPluginDataSize >= nLen+4) p++;
			if (m_pMixStruct->nPluginDataSize >= nLen)
			{
				for (UINT i=0; i<nParams; i++)
				{
					SetParameter(i, p[i]);
				}
			}
		}
	}
}


VOID CVstPlugin::ToggleEditor()
//-----------------------------
{
	if ((!m_pEffect) || (!(m_pEffect->flags & effFlagsHasEditor))) return;
	#ifndef _DEBUG
	__try {
	#endif
	if ((m_pEditor) && (!m_pEditor->m_hWnd))
	{
		delete m_pEditor;
		m_pEditor = NULL;
	}
	if (m_pEditor)
	{
		if (m_pEditor->m_hWnd) m_pEditor->DoClose();
		if ((volatile void *)m_pEditor) delete m_pEditor;
		m_pEditor = NULL;
	} else
	{
		m_pEditor = new CVstEditor(this);
		if (m_pEditor)
		{
			m_pEditor->OpenEditor(CMainFrame::GetMainFrame());
		}
	}
	#ifndef _DEBUG
	} __except(EXCEPTION_EXECUTE_HANDLER)
	{
	#ifdef VST_LOG
		Log("GPF in ToggleEditor() (Plugin=%s)\n", m_pFactory->szLibraryName);
	#endif
	}
	#endif
}


UINT CVstPlugin::GetNumCommands()
//-------------------------------
{
	if ((m_pEffect) && (m_pEffect->magic == kBuzzMagic))
	{
		return Dispatch(effBuzzGetNumCommands, 0,0,NULL,0);
	}
	return 0;
}


BOOL CVstPlugin::GetCommandName(UINT nIndex, LPSTR pszName)
//---------------------------------------------------------
{
	if ((m_pEffect) && (m_pEffect->magic == kBuzzMagic))
	{
		return Dispatch(effBuzzGetCommandName, nIndex,0,pszName,0);
	}
	return 0;
}


BOOL CVstPlugin::ExecuteCommand(UINT nIndex)
//------------------------------------------
{
	if ((m_pEffect) && (m_pEffect->magic == kBuzzMagic))
	{
		return Dispatch(effBuzzExecuteCommand, nIndex,0,NULL,0);
	}
	return 0;
}


//////////////////////////////////////////////////////////////////////////////
//
// CVstEditor
//

BEGIN_MESSAGE_MAP(CVstEditor, CDialog)
	ON_WM_CLOSE()
END_MESSAGE_MAP()


CVstEditor::CVstEditor(CVstPlugin *pPlugin)
//-----------------------------------------
{
	m_pVstPlugin = pPlugin;
}


CVstEditor::~CVstEditor()
//-----------------------
{
#ifdef VST_LOG
	Log("~CVstEditor()\n");
#endif
	if (m_pVstPlugin)
	{
		m_pVstPlugin->m_pEditor = NULL;
		m_pVstPlugin = NULL;
	}
}

#pragma pack(push, 1)
typedef struct _ERect
{
	short top;
	short left;
	short bottom;
	short right;
} ERect, *PERect;
#pragma pack(pop)


BOOL CVstEditor::OpenEditor(CWnd *parent)
//---------------------------------------
{
	Create(IDD_PLUGINEDITOR, parent);
	if (m_pVstPlugin)
	{
		CRect rcWnd, rcClient;
		ERect *pRect;

		pRect = NULL;
		m_pVstPlugin->Dispatch(effEditOpen, 0, 0, (void *)m_hWnd, 0);
		m_pVstPlugin->Dispatch(effEditGetRect, 0, 0, (LPRECT)&pRect, 0);
		if ((pRect) && (pRect->right > pRect->left) && (pRect->bottom > pRect->top))
		{
			GetWindowRect(&rcWnd);
			GetClientRect(&rcClient);
			SetWindowPos(NULL, 0,0, 
				(rcWnd.right-rcWnd.left) - (rcClient.right-rcClient.left) + (pRect->right-pRect->left),
				(rcWnd.bottom-rcWnd.top) - (rcClient.bottom-rcClient.top) + (pRect->bottom-pRect->top),
				SWP_NOZORDER|SWP_NOMOVE|SWP_NOACTIVATE);
		}
		if ((m_pVstPlugin->m_nEditorX >= 0) && (m_pVstPlugin->m_nEditorY >= 0))
		{
			int cxScreen = GetSystemMetrics(SM_CXSCREEN);
			int cyScreen = GetSystemMetrics(SM_CYSCREEN);
			if ((m_pVstPlugin->m_nEditorX+8 < cxScreen) && (m_pVstPlugin->m_nEditorY+8 < cyScreen))
			{
				SetWindowPos(NULL, m_pVstPlugin->m_nEditorX, m_pVstPlugin->m_nEditorY, 0,0,
					SWP_NOZORDER|SWP_NOSIZE|SWP_NOACTIVATE);
			}
		}
		if (m_pVstPlugin->m_pMixStruct)
		{
			const char *pszTitle = m_pVstPlugin->m_pMixStruct->Info.szName;
			if (!pszTitle[0]) pszTitle = m_pVstPlugin->m_pMixStruct->Info.szLibraryName;
			SetWindowText(pszTitle);
		}
		m_pVstPlugin->Dispatch(effEditTop, 0,0, NULL, 0);
		m_pVstPlugin->Dispatch(effEditIdle, 0,0, NULL, 0);
	}
	ShowWindow(SW_SHOW);
	return TRUE;
}


VOID CVstEditor::OnClose()
//------------------------
{
	DoClose();
}


VOID CVstEditor::OnOK()
//---------------------
{
	OnClose();
}


VOID CVstEditor::OnCancel()
//-------------------------
{
	OnClose();
}


VOID CVstEditor::DoClose()
//------------------------
{
#ifdef VST_LOG
	Log("CVstEditor::DoClose()\n");
#endif
	if ((m_pVstPlugin) && (m_hWnd))
	{
		CRect rect;
		GetWindowRect(&rect);
		m_pVstPlugin->m_nEditorX = rect.left;
		m_pVstPlugin->m_nEditorY = rect.top;
	}
	if (m_hWnd)
	{
	#ifdef VST_LOG
		Log("Destroying window...\n");
	#endif
		DestroyWindow();
	}
	if (m_pVstPlugin)
	{
	#ifdef VST_LOG
		Log("Dispatching effEditClose...\n");
	#endif
		m_pVstPlugin->Dispatch(effEditClose, 0, 0, NULL, 0);
	}
}


//////////////////////////////////////////////////////////////////////////////////////
//
// BUZZ -> VST converter
//

#ifdef ENABLE_BUZZ

class CBuzz2Vst;

//===========================================
class CVSTDataInput: public CMachineDataInput
//===========================================
{
protected:
	CBuzz2Vst *m_pParent;

public:
	CVSTDataInput(CBuzz2Vst *p) { m_pParent = p; }
	virtual void Read(void *pbuf, int const numbytes) { Log("Read(%08X, %d)\n", pbuf, numbytes);}
};


//==================================
class CBuzz2Vst: public CMICallbacks
//==================================
{
protected:
	AEffect m_Effect;
	const CMachineInfo *m_pMachineInfo;
	CMachineInterface *m_pMachineInterface;
	CVSTDataInput *m_pDataInput;
	CMasterInfo m_MasterInfo;
	CWaveLevel m_WaveLevel;
	short m_Waveform[32];

public:
	CBuzz2Vst(CMachineInterface *, const CMachineInfo *);
	~CBuzz2Vst();
	AEffect *GetEffect() { return &m_Effect; }
	long Dispatcher(long opCode, long index, long value, void *ptr, float opt);
	void SetParameter(long index, float parameter);
	float GetParameter(long index);
	void Process(float **inputs, float **outputs, long sampleframes);

public:
	static long VSTCALLBACK BuzzDispatcher(AEffect *effect, long opCode, long index, long value, void *ptr, float opt);
	static void VSTCALLBACK BuzzSetParameter(AEffect *effect, long index, float parameter);
	static float VSTCALLBACK BuzzGetParameter(AEffect *effect, long index);
	static void VSTCALLBACK BuzzProcess(AEffect *effect, float **inputs, float **outputs, long sampleframes);

	// CMICallbacks interface
public:
	virtual CWaveInfo const *GetWave(int const) { Log("GetWave\n"); return NULL; }
	virtual CWaveLevel const *GetWaveLevel(int const i, int const level) { Log("GetWaveLevel\n"); return NULL; }
	virtual void MessageBox(char const *txt) { ::MessageBox(CMainFrame::GetMainFrame()->m_hWnd, txt, m_pMachineInfo->Name, MB_OK); }
	virtual void Lock() { Log("Lock\n"); }
	virtual void Unlock() { Log("Unlock\n"); }
	virtual int GetWritePos() { Log("GetWritePos\n"); return 0; }
	virtual int GetPlayPos() { Log("GetPlayPos\n"); return 0; }
	virtual float *GetAuxBuffer() { Log("GetAuxBuffer\n"); return NULL; }
	virtual void ClearAuxBuffer() { Log("ClearAuxBuffer\n"); }
	virtual int GetFreeWave() { Log("GetFreeWave\n"); return 0; }
	virtual bool AllocateWave(int const i, int const size, char const *name) { Log("AllocateWave\n"); return false; }
	virtual void ScheduleEvent(int const time, dword const data) { Log("ScheduleEvent\n"); }
	virtual void MidiOut(int const dev, dword const data) { Log("MidiOut\n"); }
	virtual short const *GetOscillatorTable(int const waveform) { Log("GetOscillatorTable\n"); return NULL; }

	// envelopes
	virtual int GetEnvSize(int const wave, int const env) { Log("GetEnvSize\n"); return 0; }
	virtual bool GetEnvPoint(int const wave, int const env, int const i, word &x, word &y, int &flags) { Log("GetEnvPoint\n"); return false; }
	virtual CWaveLevel const *GetNearestWaveLevel(int const i, int const note) { Log("GetNearestWaveLevel(%d, %d)\n", i, note); return &m_WaveLevel; }
	
	// pattern editing
	virtual void SetNumberOfTracks(int const n) { Log("SetNumberOfTracks\n"); }
	virtual CPattern *CreatePattern(char const *name, int const length) { Log("CreatePattern\n"); return NULL; }
	virtual CPattern *GetPattern(int const index) { Log("GetPattern\n"); return NULL; }
	virtual char const *GetPatternName(CPattern *ppat) { Log("GetPatternName\n"); return ""; }
	virtual void RenamePattern(char const *oldname, char const *newname) { Log("RenamePattern\n"); }
	virtual void DeletePattern(CPattern *ppat) { Log("DeletePattern\n"); }
	virtual int GetPatternData(CPattern *ppat, int const row, int const group, int const track, int const field) { Log("GetPatternData\n"); return 0; }
	virtual void SetPatternData(CPattern *ppat, int const row, int const group, int const track, int const field, int const value) { Log("SetPatternData\n"); }
 		
	// sequence editing
	virtual CSequence *CreateSequence() { Log("CreateSequence\n"); return NULL; }
	virtual void DeleteSequence(CSequence *pseq) { Log("DeleteSequence\n"); }

	// special ppat values for GetSequenceData and SetSequenceData 
	// empty = NULL
	// <break> = (CPattern *)1
	// <mute> = (CPattern *)2
	// <thru> = (CPattern *)3
	virtual CPattern *GetSequenceData(int const row) { Log("GetSequenceData\n"); return NULL; }
	virtual void SetSequenceData(int const row, CPattern *ppat) { Log("SetSequenceData\n"); }

	// buzz v1.2 (MI_VERSION 15) additions start here
	virtual void SetMachineInterfaceEx(CMachineInterfaceEx *pex) { Log("SetMachineInterfaceEx\n"); }
	// group 1=global, 2=track
	virtual void ControlChange__obsolete__(int group, int track, int param, int value) { Log("ControlChange__obsolete__\n"); }
	
	// direct calls to audiodriver, used by WaveInput and WaveOutput
	// shouldn't be used for anything else
	virtual int ADGetnumChannels(bool input) { Log("ADGetNumChannels\n"); return 2; }
	virtual void ADWrite(int channel, float *psamples, int numsamples) { Log("ADWrite\n"); }
	virtual void ADRead(int channel, float *psamples, int numsamples) { Log("ADRead\n"); }

	virtual CMachine *GetThisMachine() { Log("GetThisMachine\n"); return NULL; }
	virtual void ControlChange(CMachine *pmac, int group, int track, int param, int value) { Log("ControlChange\n"); }		// set value of parameter (group & 16 == don't record)

	// returns pointer to the sequence if there is a pattern playing
	virtual CSequence *GetPlayingSequence(CMachine *pmac) { Log("GetPlayingSequence\n"); return NULL; }

	// gets ptr to raw pattern data for row of a track of a currently playing pattern (or something like that)
	virtual void *GetPlayingRow(CSequence *pseq, int group, int track) { Log("GetPlayingRow\n"); return NULL; }
	virtual int GetStateFlags() { Log("GetStateFlags\n"); return 0; }
	virtual void SetnumOutputChannels(CMachine *pmac, int n) { Log("SetNumOutputChannels\n"); }	// if n=1 Work(), n=2 WorkMonoToStereo()
	virtual void SetEventHandler(CMachine *pmac, BEventType et, EVENT_HANDLER_PTR p, void *param) { Log("SetEventHandler\n"); }
	virtual char const *GetWaveName(int const i) { Log("GetWavename\n"); return ""; }
	virtual void SetInternalWaveName(CMachine *pmac, int const i, char const *name) { Log("SetInternalWaveName\n"); }	// i >= 1, NULL name to clear
	virtual void GetMachineNames(CMachineDataOutput *pout) { Log("GetMachineNames\n"); }		// *pout will get one name per Write()
	virtual CMachine *GetMachine(char const *name) { Log("GetMachine\n"); return NULL; }
	virtual CMachineInfo const *GetMachineInfo(CMachine *pmac) { Log("GetMachineInfo\n"); return NULL; }
	virtual char const *GetMachineName(CMachine *pmac) { Log("GetMachineName\n"); return ""; }
	virtual bool GetInput(int index, float *psamples, int numsamples, bool stereo, float *extrabuffer) { Log("GetInput\n"); return false; }
};


CBuzz2Vst::CBuzz2Vst(CMachineInterface *pBuzzMachine, const CMachineInfo *pBuzzInfo)
//----------------------------------------------------------------------------------
{
	m_pMachineInfo = pBuzzInfo;
	m_pMachineInterface = pBuzzMachine;
	m_pDataInput = new CVSTDataInput(this);

	memset(&m_Effect, 0, sizeof(AEffect));
	m_Effect.magic = kBuzzMagic;
	m_Effect.numPrograms = 1;
	m_Effect.numParams = pBuzzInfo->numGlobalParameters;
	m_Effect.numInputs = 2;
	m_Effect.numOutputs = 2;
	m_Effect.flags = 0;		// see constants
	m_Effect.ioRatio = 1.0f;
	m_Effect.object = this;
	m_Effect.uniqueID = pBuzzInfo->Version;
	m_Effect.version = 2;
	// Callbacks
	m_Effect.dispatcher = BuzzDispatcher;
	m_Effect.setParameter = BuzzSetParameter;
	m_Effect.getParameter = BuzzGetParameter;
	m_Effect.process = BuzzProcess;
	m_Effect.processReplacing = NULL;
	// Buzz Info
	m_pMachineInterface->pMasterInfo = &m_MasterInfo;
	m_pMachineInterface->pCB = this;
	m_MasterInfo.BeatsPerMin = 125;
	m_MasterInfo.TicksPerBeat = 6;
	m_MasterInfo.SamplesPerSec = CSoundFile::gdwMixingFreq;
	m_MasterInfo.SamplesPerTick = (int)((60*m_MasterInfo.SamplesPerSec) / (125*6));
	m_MasterInfo.PosInTick = 0;
	m_MasterInfo.TicksPerSec = (float)m_MasterInfo.SamplesPerSec / (float)m_MasterInfo.SamplesPerTick;
	// Dummy Wave Level
	memset(m_Waveform, 0, sizeof(m_Waveform));
	m_WaveLevel.numSamples = 16;
	m_WaveLevel.pSamples = m_Waveform;
	m_WaveLevel.RootNote = 60;
	m_WaveLevel.SamplesPerSec = 44100;
	m_WaveLevel.LoopStart = 0;
	m_WaveLevel.LoopEnd = 0;
}


CBuzz2Vst::~CBuzz2Vst()
//---------------------
{
	m_Effect.object = NULL;
	if (m_pMachineInfo)
	{
		m_pMachineInfo = NULL;
	}
	if (m_pMachineInterface)
	{
		delete m_pMachineInterface;
		m_pMachineInterface = NULL;
	}
	if (m_pDataInput)
	{
		delete m_pDataInput;
		m_pDataInput = NULL;
	}
}


long CBuzz2Vst::BuzzDispatcher(AEffect *effect, long opCode, long index, long value, void *ptr, float opt)
//--------------------------------------------------------------------------------------------------------
{
	CBuzz2Vst *pBuzz2Vst = (CBuzz2Vst *)effect->object;
	if (pBuzz2Vst) return pBuzz2Vst->Dispatcher(opCode, index, value, ptr, opt);
	return 0;
}


void CBuzz2Vst::BuzzSetParameter(AEffect *effect, long index, float parameter)
//----------------------------------------------------------------------------
{
	CBuzz2Vst *pBuzz2Vst = (CBuzz2Vst *)effect->object;
	if (pBuzz2Vst) pBuzz2Vst->SetParameter(index, parameter);
}


float CBuzz2Vst::BuzzGetParameter(AEffect *effect, long index)
//------------------------------------------------------------
{
	CBuzz2Vst *pBuzz2Vst = (CBuzz2Vst *)effect->object;
	if (pBuzz2Vst) return pBuzz2Vst->GetParameter(index);
	return 0;
}


void CBuzz2Vst::BuzzProcess(AEffect *effect, float **inputs, float **outputs, long sampleframes)
//----------------------------------------------------------------------------------------------
{
	CBuzz2Vst *pBuzz2Vst = (CBuzz2Vst *)effect->object;
	if (pBuzz2Vst) pBuzz2Vst->Process(inputs, outputs, sampleframes);
}


long CBuzz2Vst::Dispatcher(long opCode, long index, long value, void *ptr, float opt)
//-----------------------------------------------------------------------------------
{
	if (!m_pMachineInterface) return 0;
	switch(opCode)
	{
	case effOpen:
		Log("Initializing machine...\n");
		m_pMachineInterface->Init(m_pDataInput);
		Log("Done.\n");
		break;

	case effClose:
		m_Effect.object = NULL;
		delete this;
		return 0;

	case effIdentify:
		return 'NvEf';

	case effGetParamName:
		if (index < m_pMachineInfo->numGlobalParameters)
		{
			const CMachineParameter *p = m_pMachineInfo->Parameters[index];
			if ((p) && (ptr) && (p->Name))
			{
				lstrcpyn((char *)ptr, p->Name, 9);
			}
		}
		break;

	case effGetParamDisplay:
		if (index < m_pMachineInfo->numGlobalParameters)
		{
			const CMachineParameter *p = m_pMachineInfo->Parameters[index];
			if (p)
			{
				int value = p->DefValue;
				const char *s = m_pMachineInterface->DescribeValue(index, value);
				if ((s) && (ptr))
				{
					lstrcpyn((char *)ptr, s, 9);
				}
			}
		}
		break;

	// Buzz extensions
	case effBuzzGetNumCommands:
		{
			const char *p = m_pMachineInfo->Commands;
			UINT n = 0;
			while (p)
			{
				if (!*p)
				{
					n++;
					break;
				}
				if (*p == '\n') n++;
				p++;
			}
			return n;
		}

	case effBuzzGetCommandName:
		if (ptr)
		{
			const char *p = m_pMachineInfo->Commands;
			char *s = (char *)ptr;
			UINT n = 0, len = 0;
			while ((p) && (n <= (UINT)index))
			{
				if (!*p)
				{
					n++;
					break;
				}
				if (*p == '\n') n++;
				if ((n == (UINT)index) && (len < 10))
				{
					*s++ = *p;
					len++;
				}
				p++;
			}
			*s = 0;
			return 0;
		}

	case effBuzzExecuteCommand:
		{
			m_pMachineInterface->Command(index);
		}
	}
	return 0;
}


void CBuzz2Vst::SetParameter(long index, float parameter)
//-------------------------------------------------------
{
	
}


float CBuzz2Vst::GetParameter(long index)
//---------------------------------------
{
	if (index < m_pMachineInfo->numGlobalParameters)
	{
		const CMachineParameter *p = m_pMachineInfo->Parameters[index];
		if (p)
		{
			float fmin = (float)p->MinValue;
			float fmax = (float)p->MaxValue;
			float f = (float)p->DefValue;
			if (p->MinValue < p->MaxValue)
			{
				return (f - fmin) / (fmax - fmin);
			}
		}
	}
	return 0;
}


void X86_Interleave(const float *pinL, const float *pinR, float *pout, int nsamples)
//----------------------------------------------------------------------------------
{
	for (int i=0; i<nsamples; i++)
	{
		pout[i*2] = pinL[i];
		pout[i*2+1] = pinR[i];
	}
}


void X86_AddAndDeinterleave(const float *pin, float *poutL, float *poutR, int nsamples)
//-------------------------------------------------------------------------------------
{
	for (int i=0; i<nsamples; i++)
	{
		poutL[i] += pin[i*2];
		poutR[i] += pin[i*2+1];
	}
}


void CBuzz2Vst::Process(float **inputs, float **outputs, long sampleframes)
//-------------------------------------------------------------------------
{
	float *pinL = inputs[0], *pinR = inputs[1];
	float *poutL = outputs[0], *poutR = outputs[1];
	float Buffer[MIXBUFFERSIZE*2];		// Stereo interleaved
	
	// Re-interleave the stereo mix
	X86_Interleave(pinL, pinR, Buffer, sampleframes);
	// Process
	m_pMachineInterface->Work(Buffer, sampleframes, WM_READWRITE);
	// De-interleave and add the result
	X86_AddAndDeinterleave(Buffer, poutL, poutR, sampleframes);
}


AEffect *Buzz2Vst(CMachineInterface *pBuzzMachine, const CMachineInfo *pBuzzInfo)
//-------------------------------------------------------------------------------
{
	CBuzz2Vst *p;

	if ((!pBuzzMachine) || (!pBuzzInfo)) return NULL;
	p = new CBuzz2Vst(pBuzzMachine, pBuzzInfo);
	return (p) ? p->GetEffect() : NULL;
}

#endif // ENABLE_BUZZ


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
	short int *m_pMixBuffer;
	short int m_MixBuffer[MIXBUFFERSIZE*2+16];		// 16-bit Stereo interleaved

public:
	CDmo2Vst(IMediaObject *pMO, IMediaObjectInPlace *pMOIP, DWORD uid);
	~CDmo2Vst();
	AEffect *GetEffect() { return &m_Effect; }
	long Dispatcher(long opCode, long index, long value, void *ptr, float opt);
	void SetParameter(long index, float parameter);
	float GetParameter(long index);
	void Process(float **inputs, float **outputs, long sampleframes);

public:
	static long VSTCALLBACK DmoDispatcher(AEffect *effect, long opCode, long index, long value, void *ptr, float opt);
	static void VSTCALLBACK DmoSetParameter(AEffect *effect, long index, float parameter);
	static float VSTCALLBACK DmoGetParameter(AEffect *effect, long index);
	static void VSTCALLBACK DmoProcess(AEffect *effect, float **inputs, float **outputs, long sampleframes);
};


CDmo2Vst::CDmo2Vst(IMediaObject *pMO, IMediaObjectInPlace *pMOIP, DWORD uid)
//--------------------------------------------------------------------------
{
	m_pMediaObject = pMO;
	m_pMediaProcess = pMOIP;
	m_pParamInfo = NULL;
	m_pMediaParams = NULL;
	memset(&m_Effect, 0, sizeof(m_Effect));
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
	if (FAILED(m_pMediaObject->QueryInterface(IID_IMediaParamInfo, (void **)&m_pParamInfo))) m_pParamInfo = NULL;
	if (m_pParamInfo)
	{
		DWORD dwParamCount = 0;
		m_pParamInfo->GetParamCount(&dwParamCount);
		m_Effect.numParams = dwParamCount;
	}
	if (FAILED(m_pMediaObject->QueryInterface(IID_IMediaParams, (void **)&m_pMediaParams))) m_pMediaParams = NULL;
	m_pMixBuffer = (short int *)((((int)m_MixBuffer)+15)&~15);
	// Callbacks
	m_Effect.dispatcher = DmoDispatcher;
	m_Effect.setParameter = DmoSetParameter;
	m_Effect.getParameter = DmoGetParameter;
	m_Effect.process = DmoProcess;
	m_Effect.processReplacing = NULL;
}


CDmo2Vst::~CDmo2Vst()
//-------------------
{
	if (m_pMediaParams)
	{
		m_pMediaParams->Release();
		m_pMediaParams = NULL;
	}
	if (m_pParamInfo)
	{
		m_pParamInfo->Release();
		m_pParamInfo = NULL;
	}
	if (m_pMediaProcess)
	{
		m_pMediaProcess->Release();
		m_pMediaProcess = NULL;
	}
	if (m_pMediaObject)
	{
		m_pMediaObject->Release();
		m_pMediaObject = NULL;
	}
}


long CDmo2Vst::DmoDispatcher(AEffect *effect, long opCode, long index, long value, void *ptr, float opt)
//------------------------------------------------------------------------------------------------------
{
	if (effect)
	{
		CDmo2Vst *that = (CDmo2Vst *)effect->object;
		if (that) return that->Dispatcher(opCode, index, value, ptr, opt);
	}
	return 0;
}


void CDmo2Vst::DmoSetParameter(AEffect *effect, long index, float parameter)
//--------------------------------------------------------------------------
{
	if (effect)
	{
		CDmo2Vst *that = (CDmo2Vst *)effect->object;
		if (that) that->SetParameter(index, parameter);
	}
}


float CDmo2Vst::DmoGetParameter(AEffect *effect, long index)
//----------------------------------------------------------
{
	if (effect)
	{
		CDmo2Vst *that = (CDmo2Vst *)effect->object;
		if (that) return that->GetParameter(index);
	}
	return 0;
}


void CDmo2Vst::DmoProcess(AEffect *effect, float **inputs, float **outputs, long sampleframes)
//--------------------------------------------------------------------------------------------
{
	if (effect)
	{
		CDmo2Vst *that = (CDmo2Vst *)effect->object;
		if (that) that->Process(inputs, outputs, sampleframes);
	}
}


long CDmo2Vst::Dispatcher(long opCode, long index, long value, void *ptr, float opt)
//----------------------------------------------------------------------------------
{
	switch(opCode)
	{
	case effOpen:
		break;

	case effClose:
		m_Effect.object = NULL;
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
						// TODO: case MPT_ENUM: -> GetParamText
						default:
							wsprintf(pszName, "%d", (int)md);
							break;
						}
					}
				} else
				{
					WideCharToMultiByte(CP_ACP, 0, (opCode == effGetParamName) ? mpi.szLabel : mpi.szUnitText, -1, pszName, 32, NULL, NULL);
				}
			}
		}
		break;

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
			mt.pUnk = NULL;
			mt.pbFormat = (LPBYTE)&wfx;
			mt.cbFormat = sizeof(WAVEFORMATEX);
			mt.lSampleSize = 4;
			wfx.wFormatTag = WAVE_FORMAT_PCM;
			wfx.nChannels = 2;
			wfx.nSamplesPerSec = m_nSamplesPerSec;
			wfx.wBitsPerSample = 16;
			wfx.nBlockAlign = wfx.nChannels * (wfx.wBitsPerSample >> 3);
			wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
			wfx.cbSize = 0;
			if ((FAILED(m_pMediaObject->SetInputType(0, &mt, 0)))
			 || (FAILED(m_pMediaObject->SetOutputType(0, &mt, 0))))
			{
			#ifdef DMO_LOG
				Log("DMO: Failed to set I/O media type\n");
			#endif
				return -1;
			}
		} else
		{
			m_pMediaObject->Flush();
			m_pMediaObject->SetInputType(0, NULL, DMO_SET_TYPEF_CLEAR);
			m_pMediaObject->SetOutputType(0, NULL, DMO_SET_TYPEF_CLEAR);
			m_DataTime = 0;
		}
		break;
	}
	return 0;
}


void CDmo2Vst::SetParameter(long index, float fValue)
//---------------------------------------------------
{
	MP_PARAMINFO mpi;

	if ((index < m_Effect.numParams) && (m_pParamInfo) && (m_pMediaParams))
	{
		memset(&mpi, 0, sizeof(mpi));
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


float CDmo2Vst::GetParameter(long index)
//--------------------------------------
{
	if ((index < m_Effect.numParams) && (m_pParamInfo) && (m_pMediaParams))
	{
		MP_PARAMINFO mpi;
		MP_DATA md;

		memset(&mpi, 0, sizeof(mpi));
		md = 0;
		if ((m_pParamInfo->GetParamInfo(index, &mpi) == S_OK)
		 && (m_pMediaParams->GetParam(index, &md) == S_OK))
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
			if (fMax > fMin) fValue /= (fMax-fMin);
			return fValue;
		}
	}
	return 0;
}

static const float _f2si = 32768.0f;
static const float _si2f = 1.0f / 32768.0f;

void X86_FloatToStereo16Mix(const float *pIn1, const float *pIn2, short int *pOut, int sampleframes)
//--------------------------------------------------------------------------------------------------
{
	for (int i=0; i<sampleframes; i++)
	{
		int xl = (int)(pIn1[i] * _f2si);
		int xr = (int)(pIn2[i] * _f2si);
		if (xl < -32768) xl = -32768;
		if (xl > 32767) xl = 32767;
		if (xr < -32768) xr = -32768;
		if (xr > 32767) xr = 32767;
		pOut[i*2] = xl;
		pOut[i*2+1] = xr;
	}
}


void X86_Stereo16AddMixToFloat(const short int *pIn, float *pOut1, float *pOut2, int sampleframes)
//------------------------------------------------------------------------------------------------
{
	for (int i=0; i<sampleframes; i++)
	{
		float xl = _si2f * (float)pIn[i*2];
		float xr = _si2f * (float)pIn[i*2+1];
		pOut1[i] += xl;
		pOut2[i] += xr;
	}
}


void SSE_FloatToStereo16Mix(const float *pIn1, const float *pIn2, short int *pOut, int sampleframes)
//--------------------------------------------------------------------------------------------------
{
	_asm {
	mov eax, pIn1
	mov edx, pIn2
	mov ebx, pOut
	mov ecx, sampleframes
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


void SSE_Stereo16AddMixToFloat(const short int *pIn, float *pOut1, float *pOut2, int sampleframes)
//------------------------------------------------------------------------------------------------
{
	_asm {
	mov ebx, pIn
	mov eax, pOut1
	mov edx, pOut2
	mov ecx, sampleframes
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


void CDmo2Vst::Process(float **inputs, float **outputs, long sampleframes)
//------------------------------------------------------------------------
{
	if ((!m_pMixBuffer) || (sampleframes <= 0)) return;
	if ((CSoundFile::gdwSysInfo & SYSMIX_SSE) && (CSoundFile::gdwSoundSetup & SNDMIX_ENABLEMMX))
	{
		SSE_FloatToStereo16Mix(inputs[0], inputs[1], m_pMixBuffer, sampleframes);
		m_pMediaProcess->Process(sampleframes*4, (LPBYTE)m_pMixBuffer, m_DataTime, DMO_INPLACE_NORMAL);
		SSE_Stereo16AddMixToFloat(m_pMixBuffer, outputs[0], outputs[1], sampleframes);
	} else
	{
		X86_FloatToStereo16Mix(inputs[0], inputs[1], m_pMixBuffer, sampleframes);
		m_pMediaProcess->Process(sampleframes*4, (LPBYTE)m_pMixBuffer, m_DataTime, DMO_INPLACE_NORMAL);
		X86_Stereo16AddMixToFloat(m_pMixBuffer, outputs[0], outputs[1], sampleframes);
	}
	m_DataTime += _muldiv(sampleframes, 10000000, m_nSamplesPerSec);
}


AEffect *DmoToVst(PVSTPLUGINLIB pLib)
//-----------------------------------
{
	WCHAR w[100];
	CLSID clsid;

	MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pLib->szDllPath,-1,(LPWSTR)w,98);
	if (CLSIDFromString(w, &clsid) == S_OK)
	{
		IMediaObject *pMO = NULL;
		IMediaObjectInPlace *pMOIP = NULL;
		if ((CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IMediaObject, (VOID **)&pMO) == S_OK) && (pMO))
		{
			if (pMO->QueryInterface(IID_IMediaObjectInPlace, (void **)&pMOIP) != S_OK) pMOIP = NULL;
		} else pMO = NULL;
		if ((pMO) && (pMOIP))
		{
			DWORD dwInputs, dwOutputs;
			BOOL bError;

			dwInputs = dwOutputs = 0;
			pMO->GetStreamCount(&dwInputs, &dwOutputs);
			bError = ((dwInputs == 1) && (dwOutputs == 1)) ? FALSE : TRUE;
			if (!bError)
			{
				CDmo2Vst *p = new CDmo2Vst(pMO, pMOIP, clsid.Data1);
				return (p) ? p->GetEffect() : NULL;
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
	return NULL;
}


