/*
 * ModDocTemplate.h
 * ----------------
 * Purpose: CDocTemplate specialization for CModDoc.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include <unordered_set>

OPENMPT_NAMESPACE_BEGIN

class CModDoc;

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
	std::unordered_set<CModDoc *>::iterator begin() { return m_documents.begin(); }
	std::unordered_set<CModDoc *>::const_iterator begin() const { return m_documents.begin(); }
	std::unordered_set<CModDoc *>::const_iterator cbegin() const { return m_documents.cbegin(); }
	std::unordered_set<CModDoc *>::iterator end() { return m_documents.end(); }
	std::unordered_set<CModDoc *>::const_iterator end() const { return m_documents.end(); }
	std::unordered_set<CModDoc *>::const_iterator cend() const { return m_documents.cend(); }
};

OPENMPT_NAMESPACE_END
