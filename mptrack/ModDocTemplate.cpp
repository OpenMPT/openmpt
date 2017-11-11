/*
 * ModDocTemplate.cpp
 * ------------------
 * Purpose: CDocTemplate specialization for CModDoc.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Mainfrm.h"
#include "Moddoc.h"
#include "ModDocTemplate.h"
#include "Reporting.h"
#include "../soundlib/plugins/PluginManager.h"

OPENMPT_NAMESPACE_BEGIN

CDocument *CModDocTemplate::OpenDocumentFile(LPCTSTR lpszPathName, BOOL addToMru, BOOL makeVisible)
{
	const mpt::PathString filename = (lpszPathName ? mpt::PathString::FromCString(lpszPathName) : mpt::PathString());

	if(filename.IsDirectory())
	{
		CDocument *pDoc = nullptr;
		mpt::PathString path = filename;
		path.EnsureTrailingSlash();
		HANDLE hFind;
		WIN32_FIND_DATAW wfd;
		MemsetZero(wfd);
		if((hFind = FindFirstFileW((path + MPT_PATHSTRING("*.*")).AsNative().c_str(), &wfd)) != INVALID_HANDLE_VALUE)
		{
			do
			{
				if(wcscmp(wfd.cFileName, L"..") && wcscmp(wfd.cFileName, L"."))
				{
					pDoc = OpenDocumentFile((path + mpt::PathString::FromNative(wfd.cFileName)).ToCString(), addToMru, makeVisible);
				}
			} while (FindNextFileW(hFind, &wfd));
			FindClose(hFind);
		}
		return pDoc;
	}

	if(!mpt::PathString::CompareNoCase(filename.GetFileExt(), MPT_PATHSTRING(".dll")))
	{
		CVstPluginManager *pPluginManager = theApp.GetPluginManager();
		if(pPluginManager && pPluginManager->AddPlugin(filename) != nullptr)
		{
			return nullptr;
		}
	}

	// First, remove document from MRU list.
	if(addToMru)
	{
		theApp.RemoveMruItem(filename);
	}

	CDocument *pDoc = CMultiDocTemplate::OpenDocumentFile(filename.empty() ? NULL : filename.ToCString().GetString(), addToMru, makeVisible);
	if(pDoc)
	{
		CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
		if (pMainFrm) pMainFrm->OnDocumentCreated(static_cast<CModDoc *>(pDoc));
	} else //Case: pDoc == 0, opening document failed.
	{
		if(!filename.empty())
		{
			if(CMainFrame::GetMainFrame() && addToMru)
			{
				CMainFrame::GetMainFrame()->UpdateMRUList();
			}
			if(!filename.IsFile())
			{
				Reporting::Error(_T("Unable to open \"") + filename.ToCString() + _T("\": file does not exist."));
			} else
			{
				// Case: Valid path but opening failed.
				const int numDocs = theApp.GetOpenDocumentCount();
				Reporting::Notification(mpt::cformat(_T("Opening \"%1\" failed. This can happen if ")
					_T("no more modules can be opened or if the file type was not ")
					_T("recognised. If the former is true, it is ")
					_T("recommended to close some modules as otherwise a crash is likely")
					_T(" (currently there %2 %3 document%4 open)."))(
					filename.ToCString(), (numDocs == 1) ? _T("is") : _T("are"), numDocs, (numDocs == 1) ? _T("") : _T("s")));
			}
		}
	}
	return pDoc;
}


CDocument* CModDocTemplate::OpenTemplateFile(const mpt::PathString &filename, bool isExampleTune)
{
	CDocument *doc = OpenDocumentFile(filename.ToCString(), isExampleTune ? TRUE : FALSE, TRUE);
	if(doc)
	{
		CModDoc *modDoc = static_cast<CModDoc *>(doc);
		// Clear path so that saving will not take place in templates/examples folder.
		modDoc->ClearFilePath();
		if(!isExampleTune)
		{
			CMultiDocTemplate::SetDefaultTitle(modDoc);
			m_nUntitledCount++;
			// Name has changed...
			CMainFrame::GetMainFrame()->UpdateTree(modDoc, GeneralHint().General());

			// Reset edit history for template files
			CSoundFile &sndFile = modDoc->GetSoundFile();
			sndFile.GetFileHistory().clear();
			sndFile.m_dwCreatedWithVersion = MptVersion::num;
			sndFile.m_dwLastSavedWithVersion = 0;
			sndFile.m_madeWithTracker.clear();
			sndFile.m_songArtist = TrackerSettings::Instance().defaultArtist;
			sndFile.m_playBehaviour = sndFile.GetDefaultPlaybackBehaviour(sndFile.GetType());
			doc->UpdateAllViews(nullptr, UpdateHint().ModType().AsLPARAM());
		} else
		{
			// Remove extension from title, so that saving the file will not suggest a filename like e.g. "example.it.it".
			const CString title = modDoc->GetTitle();
			const int dotPos = title.ReverseFind(_T('.'));
			if(dotPos >= 0)
			{
				modDoc->SetTitle(title.Left(dotPos));
			}
		}
	}
	return doc;
}


void CModDocTemplate::AddDocument(CDocument *doc)
{
	CMultiDocTemplate::AddDocument(doc);
	m_documents.insert(static_cast<CModDoc *>(doc));
}


void CModDocTemplate::RemoveDocument(CDocument *doc)
{
	CMultiDocTemplate::RemoveDocument(doc);
	m_documents.erase(static_cast<CModDoc *>(doc));
}


bool CModDocTemplate::DocumentExists(const CModDoc *doc) const
{
	return m_documents.count(const_cast<CModDoc *>(doc)) != 0;
}

OPENMPT_NAMESPACE_END
