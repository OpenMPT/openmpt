/*
 * view_com.cpp
 * ------------
 * Purpose: Song comments tab, lower panel.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Mptrack.h"
#include "Mainfrm.h"
#include "InputHandler.h"
#include "Childfrm.h"
#include "ImageLists.h"
#include "Moddoc.h"
#include "Globals.h"
#include "Ctrl_com.h"
#include "ChannelManagerDlg.h"
#include "../common/StringFixer.h"
#include "view_com.h"
#include "../soundlib/mod_specifications.h"
#include <set>


OPENMPT_NAMESPACE_BEGIN


#define DETAILS_TOOLBAR_CY	28

enum
{
	SMPLIST_SAMPLENAME = 0,
	SMPLIST_SAMPLENO,
	SMPLIST_SIZE,
	SMPLIST_TYPE,
	SMPLIST_MIDDLEC,
	SMPLIST_INSTR,
	SMPLIST_FILENAME,
	SMPLIST_PATH,
	SMPLIST_COLUMNS
};


enum
{
	INSLIST_INSTRUMENTNAME = 0,
	INSLIST_INSTRUMENTNO,
	INSLIST_SAMPLES,
	INSLIST_ENVELOPES,
	INSLIST_FILENAME,
	INSLIST_PLUGIN,
	INSLIST_COLUMNS
};


const CListCtrlEx::Header gSampleHeaders[SMPLIST_COLUMNS] =
{
	{ _T("Sample Name"),	192, LVCFMT_LEFT },
	{ _T("Num"),			45, LVCFMT_RIGHT },
	{ _T("Size"),			72, LVCFMT_RIGHT },
	{ _T("Type"),			45, LVCFMT_RIGHT },
	{ _T("C-5 Freq"),		80, LVCFMT_RIGHT },
	{ _T("Instr"),			64, LVCFMT_RIGHT },
	{ _T("File Name"),		128, LVCFMT_RIGHT },
	{ _T("Path"),			256, LVCFMT_LEFT },
};

const CListCtrlEx::Header gInstrumentHeaders[INSLIST_COLUMNS] =
{
	{ _T("Instrument Name"),	192, LVCFMT_LEFT },
	{ _T("Num"),				45, LVCFMT_RIGHT },
	{ _T("Samples"),			64, LVCFMT_RIGHT },
	{ _T("Envelopes"),			128, LVCFMT_RIGHT },
	{ _T("File Name"),			128, LVCFMT_RIGHT },
	{ _T("Plugin"),				128, LVCFMT_RIGHT },
};


IMPLEMENT_SERIAL(CViewComments, CModScrollView, 0)

BEGIN_MESSAGE_MAP(CViewComments, CModScrollView)
	//{{AFX_MSG_MAP(CViewComments)
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_MESSAGE(WM_MOD_MIDIMSG,			OnMidiMsg)
	ON_COMMAND(IDC_LIST_SAMPLES,		OnShowSamples)
	ON_COMMAND(IDC_LIST_INSTRUMENTS,	OnShowInstruments)
	ON_COMMAND(IDC_LIST_PATTERNS,		OnShowPatterns)
	ON_NOTIFY(LVN_ENDLABELEDIT,		IDC_LIST_DETAILS,	OnEndLabelEdit)
	ON_NOTIFY(LVN_BEGINLABELEDIT,	IDC_LIST_DETAILS,	OnBeginLabelEdit)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_DETAILS,	OnDblClickListItem)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


///////////////////////////////////////////////////////////////
// CViewComments operations

CViewComments::CViewComments()
//----------------------------
{
	m_nCurrentListId = 0;
	m_nListId = 0;//IDC_LIST_SAMPLES;
}


void CViewComments::OnInitialUpdate()
//-----------------------------------
{
	CModScrollView::OnInitialUpdate();
	if(m_nListId == 0)
	{
		m_nListId = IDC_LIST_SAMPLES;

		// For XM, set the instrument list as the default list
		CModDoc *pModDoc = GetDocument();
		if(pModDoc && (pModDoc->GetModType() & MOD_TYPE_XM) && pModDoc->GetNumInstruments() > 0)
		{
			m_nListId = IDC_LIST_INSTRUMENTS;
		}
	}

	CChildFrame *pFrame = (CChildFrame *)GetParentFrame();
	CRect rect;

	if (pFrame)
	{
		COMMENTVIEWSTATE &commentState = pFrame->GetCommentViewState();
		if (commentState.cbStruct == sizeof(COMMENTVIEWSTATE))
		{
			m_nListId = commentState.nId;
		}
	}
	GetClientRect(&rect);
	m_ToolBar.Create(WS_CHILD|WS_VISIBLE|CCS_NOPARENTALIGN, rect, this, IDC_TOOLBAR_DETAILS);
	m_ToolBar.Init(CMainFrame::GetMainFrame()->m_MiscIcons, CMainFrame::GetMainFrame()->m_MiscIconsDisabled);
	m_ItemList.Create(WS_CHILD|WS_VISIBLE|LVS_REPORT|LVS_SINGLESEL|LVS_EDITLABELS|LVS_NOSORTHEADER, rect, this, IDC_LIST_DETAILS);
	m_ItemList.ModifyStyleEx(0, WS_EX_STATICEDGE);
	// Add ToolBar Buttons
	m_ToolBar.AddButton(IDC_LIST_SAMPLES, IMAGE_SAMPLES);
	m_ToolBar.AddButton(IDC_LIST_INSTRUMENTS, IMAGE_INSTRUMENTS);
	//m_ToolBar.AddButton(IDC_LIST_PATTERNS, TIMAGE_TAB_PATTERNS);
	m_ToolBar.SetIndent(4);
	UpdateButtonState();
	UpdateView(UpdateHint().ModType());
}


void CViewComments::OnDestroy()
//-----------------------------
{
	CChildFrame *pFrame = (CChildFrame *)GetParentFrame();
	if (pFrame)
	{
		COMMENTVIEWSTATE &commentState = pFrame->GetCommentViewState();
		commentState.cbStruct = sizeof(COMMENTVIEWSTATE);
		commentState.nId = m_nListId;
	}
	CView::OnDestroy();
}


LRESULT CViewComments::OnMidiMsg(WPARAM midiData, LPARAM)
//-------------------------------------------------------
{
	// Handle MIDI messages assigned to shortcuts
	CInputHandler *ih = CMainFrame::GetInputHandler();
	ih->HandleMIDIMessage(kCtxViewComments, midiData) != kcNull
		|| ih->HandleMIDIMessage(kCtxAllContexts, midiData) != kcNull;
	return 1;
}


///////////////////////////////////////////////////////////////
// CViewComments drawing

void CViewComments::UpdateView(UpdateHint hint, CObject *)
//--------------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();

	if ((!pModDoc) || (!(m_ItemList.m_hWnd))) return;
	const FlagSet<HintType> hintType = hint.GetType();
	if (hintType[HINT_MPTOPTIONS])
	{
		m_ToolBar.UpdateStyle();
	}
	const SampleHint sampleHint = hint.ToType<SampleHint>();
	const InstrumentHint instrHint = hint.ToType<InstrumentHint>();
	const bool updateSamples = sampleHint.GetType()[HINT_SMPNAMES | HINT_SAMPLEINFO];
	const bool updateInstr = instrHint.GetType()[HINT_INSNAMES|HINT_INSTRUMENT];
	bool updateAll = hintType[HINT_MODTYPE];

	if(!updateSamples && !updateInstr && !updateAll) return;

	const CSoundFile &sndFile = pModDoc->GetrSoundFile();

	m_ToolBar.ChangeBitmap(IDC_LIST_INSTRUMENTS, sndFile.GetNumInstruments() ? IMAGE_INSTRUMENTS : IMAGE_INSTRMUTE);

	TCHAR s[512], stmp[256];
	LV_ITEM lvi, lvi2;

	m_ItemList.SetRedraw(FALSE);
	// Add sample headers
	if (m_nListId != m_nCurrentListId || updateAll)
	{
		UINT ichk = 0;
		m_ItemList.DeleteAllItems();
		while ((m_ItemList.DeleteColumn(0)) && (ichk < 25)) ichk++;
		m_nCurrentListId = m_nListId;
		if (m_nCurrentListId == IDC_LIST_SAMPLES)
		{
			// Add Sample Headers
			m_ItemList.SetHeaders(gSampleHeaders);
		} else if (m_nCurrentListId == IDC_LIST_INSTRUMENTS)
		{
			// Add Instrument Headers
			m_ItemList.SetHeaders(gInstrumentHeaders);
		} else
		updateAll = true;
	}
	// Add Items
	UINT nCount = m_ItemList.GetItemCount();
	// Add Samples
	if (m_nCurrentListId == IDC_LIST_SAMPLES && (updateAll || updateSamples))
	{
		SAMPLEINDEX nMax = static_cast<SAMPLEINDEX>(nCount);
		if (nMax < sndFile.GetNumSamples()) nMax = sndFile.GetNumSamples();
		for (SAMPLEINDEX iSmp = 0; iSmp < nMax; iSmp++)
		{
			if (iSmp < sndFile.GetNumSamples())
			{
				UINT nCol = 0;
				for (UINT iCol=0; iCol<SMPLIST_COLUMNS; iCol++)
				{
					const ModSample &sample = sndFile.GetSample(iSmp + 1);
					s[0] = 0;
					switch(iCol)
					{
					case SMPLIST_SAMPLENAME:
						mpt::String::Copy(s, sndFile.m_szNames[iSmp + 1]);
						break;
					case SMPLIST_SAMPLENO:
						wsprintf(s, "%02u", iSmp + 1);
						break;
					case SMPLIST_SIZE:
						if (sample.nLength)
						{
							if(sample.GetSampleSizeInBytes() >= 1024)
								wsprintf(s, "%u KB", sample.GetSampleSizeInBytes() >> 10);
							else
								wsprintf(s, "%u B", sample.GetSampleSizeInBytes());
						}
						break;
					case SMPLIST_TYPE:
						if(sample.nLength)
						{
							wsprintf(s, "%u Bit", sample.GetElementarySampleSize() * 8);
						}
						break;
					case SMPLIST_INSTR:
						if (sndFile.GetNumInstruments())
						{
							bool first = true;
							for (INSTRUMENTINDEX i = 1; i <= sndFile.GetNumInstruments(); i++)
							{
								if (sndFile.IsSampleReferencedByInstrument(iSmp + 1, i))
								{
									if (!first) strcat(s, ",");
									first = false;

									wsprintf(stmp, "%u", i);
									strcat(s, stmp);

									if (strlen(s) > sizeof(s) - 10)
									{
										strcat(s, "...");
										break;
									}
								}
							}
						}
						break;
					case SMPLIST_MIDDLEC:
						if (sample.nLength)
						{
							wsprintf(s, "%u Hz", sample.GetSampleRate(sndFile.GetType()));
						}
						break;
					case SMPLIST_FILENAME:
						mpt::String::Copy(s, sample.filename);
						s[CountOf(sample.filename)] = 0;
						break;
					case SMPLIST_PATH:
						mpt::String::Copy(s, sndFile.GetSamplePath(iSmp + 1).ToLocale());
						break;
					}
					lvi.mask = LVIF_TEXT;
					lvi.iItem = iSmp;
					lvi.iSubItem = nCol;
					lvi.pszText = (LPTSTR)s;
					if ((iCol) || (iSmp < nCount))
					{
						bool bOk = true;
						if (iSmp < nCount)
						{
							lvi2 = lvi;
							lvi2.pszText = (LPTSTR)stmp;
							lvi2.cchTextMax = sizeof(stmp);
							stmp[0] = 0;
							m_ItemList.GetItem(&lvi2);
							if (!strcmp(s, stmp)) bOk = false;
						}
						if (bOk) m_ItemList.SetItem(&lvi);
					} else
					{
						m_ItemList.InsertItem(&lvi);
					}
					nCol++;
				}
			} else
			{
				m_ItemList.DeleteItem(iSmp);
			}
		}
	} else
	// Add Instruments
	if ((m_nCurrentListId == IDC_LIST_INSTRUMENTS) && (updateAll || updateInstr))
	{
		INSTRUMENTINDEX nMax = static_cast<INSTRUMENTINDEX>(nCount);
		if (nMax < sndFile.GetNumInstruments()) nMax = sndFile.GetNumInstruments();
		for (INSTRUMENTINDEX iIns = 0; iIns < nMax; iIns++)
		{
			if (iIns < sndFile.GetNumInstruments())
			{
				UINT nCol = 0;
				for (UINT iCol=0; iCol<INSLIST_COLUMNS; iCol++)
				{
					ModInstrument *pIns = sndFile.Instruments[iIns+1];
					s[0] = 0;
					switch(iCol)
					{
					case INSLIST_INSTRUMENTNAME:
						if (pIns) mpt::String::Copy(s, pIns->name);
						break;
					case INSLIST_INSTRUMENTNO:
						wsprintf(s, "%02u", iIns+1);
						break;
					case INSLIST_SAMPLES:
						if (pIns)
						{
							const auto referencedSamples = pIns->GetSamples();

							bool first = true;
							for(auto sample = referencedSamples.cbegin(); sample != referencedSamples.cend(); sample++)
							{
								if(!first) strcat(s, ",");
								first = false;

								size_t l = strlen(s);
								if(l >= sizeof(s) - 8)
								{
									strcat(s, "...");
									break;
								}

								wsprintf(s + l, "%u", *sample);
							}
						}
						break;
					case INSLIST_ENVELOPES:
						if (pIns)
						{
							if (pIns->VolEnv.dwFlags[ENV_ENABLED]) strcat(s, "Vol");
							if (pIns->PanEnv.dwFlags[ENV_ENABLED]) { if (s[0]) strcat(s, ", "); strcat(s, "Pan"); }
							if (pIns->PitchEnv.dwFlags[ENV_ENABLED]) { if (s[0]) strcat(s, ", "); strcat(s, (pIns->PitchEnv.dwFlags[ENV_FILTER]) ? "Filter" : "Pitch"); }
						}
						break;
					case INSLIST_FILENAME:
						if (pIns)
						{
							memcpy(s, pIns->filename, sizeof(pIns->filename));
							s[CountOf(pIns->filename)] = 0;
						}
						break;
					case INSLIST_PLUGIN:
						if (pIns != nullptr && pIns->nMixPlug > 0 && sndFile.m_MixPlugins[pIns->nMixPlug - 1].pMixPlugin != nullptr)
						{
							wsprintf(s, "FX%02u: %s", pIns->nMixPlug, mpt::ToCharset(mpt::CharsetLocale, mpt::CharsetUTF8, sndFile.m_MixPlugins[pIns->nMixPlug - 1].GetLibraryName()).c_str());
						}
						break;
					}
					lvi.mask = LVIF_TEXT;
					lvi.iItem = iIns;
					lvi.iSubItem = nCol;
					lvi.pszText = (LPTSTR)s;
					if ((iCol) || (iIns < nCount))
					{
						BOOL bOk = TRUE;
						if (iIns < nCount)
						{
							lvi2 = lvi;
							lvi2.pszText = (LPTSTR)stmp;
							lvi2.cchTextMax = sizeof(stmp);
							stmp[0] = 0;
							m_ItemList.GetItem(&lvi2);
							if (!strcmp(s, stmp)) bOk = FALSE;
						}
						if (bOk) m_ItemList.SetItem(&lvi);
					} else
					{
						m_ItemList.InsertItem(&lvi);
					}
					nCol++;
				}
			} else
			{
				m_ItemList.DeleteItem(iIns);
			}
		}
	} else
	// Add Patterns
	//if ((m_nCurrentListId == IDC_LIST_PATTERNS) && (hintType & (HINT_MODTYPE|HINT_PATNAMES|HINT_PATTERNROW)))
	{
	}
	m_ItemList.SetRedraw(TRUE);
	m_ItemList.Invalidate(FALSE);
}


void CViewComments::RecalcLayout()
//--------------------------------
{
	CRect rect;

	if (!m_hWnd) return;
	GetClientRect(&rect);
	m_ToolBar.SetWindowPos(NULL, 0, 0, rect.Width(), DETAILS_TOOLBAR_CY, SWP_NOZORDER|SWP_NOACTIVATE);
	m_ItemList.SetWindowPos(NULL, -1, DETAILS_TOOLBAR_CY, rect.Width()+2, rect.Height() - DETAILS_TOOLBAR_CY + 1, SWP_NOZORDER|SWP_NOACTIVATE);
}


void CViewComments::UpdateButtonState()
//-------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		m_ToolBar.SetState(IDC_LIST_SAMPLES, ((m_nListId == IDC_LIST_SAMPLES) ? TBSTATE_CHECKED : 0)|TBSTATE_ENABLED);
		m_ToolBar.SetState(IDC_LIST_INSTRUMENTS, ((m_nListId == IDC_LIST_INSTRUMENTS) ? TBSTATE_CHECKED : 0)|TBSTATE_ENABLED);
		m_ToolBar.SetState(IDC_LIST_PATTERNS, ((m_nListId == IDC_LIST_PATTERNS) ? TBSTATE_CHECKED : 0)|TBSTATE_ENABLED);
		m_ToolBar.EnableButton(IDC_LIST_INSTRUMENTS, (pModDoc->GetNumInstruments()) ? TRUE : FALSE);
	}
}


void CViewComments::OnBeginLabelEdit(LPNMHDR, LRESULT *)
//------------------------------------------------------
{
	CEdit *editCtrl = m_ItemList.GetEditControl();
	if(editCtrl)
	{
		const CModSpecifications &specs = GetDocument()->GetSoundFile()->GetModSpecifications();
		const size_t maxStrLen = (m_nListId == IDC_LIST_SAMPLES) ? specs.sampleNameLengthMax : specs.instrNameLengthMax;
		editCtrl->LimitText(maxStrLen);
	}
}


void CViewComments::OnEndLabelEdit(LPNMHDR pnmhdr, LRESULT *)
//-----------------------------------------------------------
{
	LV_DISPINFO *plvDispInfo = (LV_DISPINFO *)pnmhdr;
	LV_ITEM &lvItem = plvDispInfo->item;
	CModDoc *pModDoc = GetDocument();

	if(lvItem.pszText != nullptr && !lvItem.iSubItem && pModDoc)
	{
		UINT iItem = lvItem.iItem;
		CSoundFile &sndFile = pModDoc->GetrSoundFile();

		if(m_nListId == IDC_LIST_SAMPLES)
		{
			if(iItem < sndFile.GetNumSamples())
			{
				mpt::String::CopyN(sndFile.m_szNames[iItem + 1], lvItem.pszText);
				pModDoc->UpdateAllViews(this, SampleHint(static_cast<SAMPLEINDEX>(iItem + 1)).Info().Names(), this);
				pModDoc->SetModified();
			}
		} else if(m_nListId == IDC_LIST_INSTRUMENTS)
		{
			if((iItem < sndFile.GetNumInstruments()) && (sndFile.Instruments[iItem + 1]))
			{
				ModInstrument *pIns = sndFile.Instruments[iItem + 1];
				mpt::String::CopyN(pIns->name, lvItem.pszText);
				pModDoc->UpdateAllViews(this, InstrumentHint(static_cast<INSTRUMENTINDEX>(iItem + 1)).Info().Names(), this);
				pModDoc->SetModified();
			}
		} else
		{
			return;
		}
		m_ItemList.SetItemText(iItem, lvItem.iSubItem, lvItem.pszText);
	}
}


///////////////////////////////////////////////////////////////
// CViewComments messages

// -> CODE#0015
// -> DESC="channels management dlg"
void CViewComments::OnDraw(CDC* pDC)
{
	CView::OnDraw(pDC);

	CMainFrame * pMainFrm = CMainFrame::GetMainFrame();
	BOOL activeDoc = pMainFrm ? pMainFrm->GetActiveDoc() == GetDocument() : FALSE;

	if(activeDoc && CChannelManagerDlg::sharedInstance(FALSE) && CChannelManagerDlg::sharedInstance()->IsDisplayed())
		CChannelManagerDlg::sharedInstance()->SetDocument((void*)this);
}
// -! NEW_FEATURE#0015


void CViewComments::OnSize(UINT nType, int cx, int cy)
//----------------------------------------------------
{
	CView::OnSize(nType, cx, cy);
	if (((nType == SIZE_RESTORED) || (nType == SIZE_MAXIMIZED)) && (cx > 0) && (cy > 0) && (m_hWnd))
	{
		RecalcLayout();
	}
}


void CViewComments::OnShowSamples()
//---------------------------------
{
	if (m_nListId != IDC_LIST_SAMPLES)
	{
		m_nListId = IDC_LIST_SAMPLES;
		UpdateButtonState();
		UpdateView(UpdateHint().ModType());
	}
}


void CViewComments::OnShowInstruments()
//-------------------------------------
{
	if (m_nListId != IDC_LIST_INSTRUMENTS)
	{
		CModDoc *pModDoc = GetDocument();
		if (pModDoc && pModDoc->GetNumInstruments())
		{
			m_nListId = IDC_LIST_INSTRUMENTS;
			UpdateButtonState();
			UpdateView(UpdateHint().ModType());
		}
	}
}


void CViewComments::OnShowPatterns()
//----------------------------------
{
	if (m_nListId != IDC_LIST_PATTERNS)
	{
		//m_nListId = IDC_LIST_PATTERNS;
		UpdateButtonState();
		UpdateView(UpdateHint().ModType());
	}
}

void CViewComments::OnDblClickListItem(NMHDR *, LRESULT *)
//--------------------------------------------------------
{
	// Double click -> switch to instrument or sample tab
	int nItem = m_ItemList.GetSelectionMark();
	if(nItem == -1) return;
	CModDoc *pModDoc = GetDocument();
	if(!pModDoc) return;
	nItem++;

	switch(m_nListId)
	{
	case IDC_LIST_SAMPLES:
		pModDoc->ViewSample(nItem);
		break;
	case IDC_LIST_INSTRUMENTS:
		pModDoc->ViewInstrument(nItem);
		break;
	case IDC_LIST_PATTERNS:
		pModDoc->ViewPattern(nItem, 0);
		break;
	}
}


LRESULT CViewComments::OnModViewMsg(WPARAM wParam, LPARAM lParam)
//-----------------------------------------------------------------
{
//	switch(wParam)
//	{
//		default:
			return CModScrollView::OnModViewMsg(wParam, lParam);
//	}
}

OPENMPT_NAMESPACE_END
