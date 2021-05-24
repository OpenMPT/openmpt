/*
 * ModDocTemplate.h
 * ----------------
 * Purpose: CDocTemplate and CModDocManager specializations for CModDoc.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include <unordered_set>

OPENMPT_NAMESPACE_BEGIN

class CModDoc;
namespace mpt { class PathString; }

class CModDocTemplate: public CMultiDocTemplate
{
	std::unordered_set<CModDoc *> m_documents;	// Allow faster lookup of open documents than MFC's linear search allows for

public:
	CModDocTemplate(UINT nIDResource, CRuntimeClass* pDocClass, CRuntimeClass* pFrameClass, CRuntimeClass* pViewClass):
		CMultiDocTemplate(nIDResource, pDocClass, pFrameClass, pViewClass) {}

	CDocument* OpenTemplateFile(const mpt::PathString &filename, bool isExampleTune = false);

	CDocument* OpenDocumentFile(LPCTSTR lpszPathName, BOOL addToMru = TRUE, BOOL makeVisible = TRUE) override;

	void AddDocument(CDocument *doc) override;
	void RemoveDocument(CDocument *doc) override;
	bool DocumentExists(const CModDoc *doc) const;

	size_t size() const { return m_documents.size(); }
	bool empty() const { return m_documents.empty(); }
	auto begin() { return m_documents.begin(); }
	auto begin() const { return m_documents.begin(); }
	auto cbegin() const { return m_documents.cbegin(); }
	auto end() { return m_documents.end(); }
	auto end() const { return m_documents.end(); }
	auto cend() const { return m_documents.cend(); }
};


class CModDocManager : public CDocManager
{
public:
	CDocument *OpenDocumentFile(LPCTSTR lpszFileName, BOOL bAddToMRU = TRUE) override;
	BOOL OnDDECommand(LPTSTR lpszCommand) override;
};


OPENMPT_NAMESPACE_END
