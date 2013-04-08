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
#include "mptrack.h"
#include "mainfrm.h"
#include "childfrm.h"
#include "moddoc.h"
#include "globals.h"
#include "ctrl_com.h"
#include "ChannelManagerDlg.h"
#include "../common/StringFixer.h"
#include "view_com.h"
#include <set>

#define DETAILS_TOOLBAR_CY	28

typedef struct LISTCOLHDR
{
	LPCSTR pszName;
	UINT cx;
} LISTCOLHDR, *PLISTCOLHDR;


enum {
	SMPLIST_SAMPLENAME=0,
	SMPLIST_SAMPLENO,
	SMPLIST_SIZE,
	SMPLIST_TYPE,
	SMPLIST_MIDDLEC,
	SMPLIST_INSTR,
	SMPLIST_FILENAME,
	SMPLIST_COLUMNS
};


enum {
	INSLIST_INSTRUMENTNAME=0,
	INSLIST_INSTRUMENTNO,
	INSLIST_SAMPLES,
	INSLIST_ENVELOPES,
	INSLIST_FILENAME,
	INSLIST_PLUGIN,
// -> CODE#0023
// -> DESC="IT project files (.itp)"
	INSLIST_PATH,
// -! NEW_FEATURE#0023
	INSLIST_COLUMNS
};


enum {
	PATLIST_PATTERNNAME=0,
	PATLIST_PATTERNNO,
	PATLIST_COLUMNS
};


LISTCOLHDR gSampleHeaders[SMPLIST_COLUMNS] =
{
	{"Sample Name", 192},
	{"Num", 36},
	{"Size", 72},
	{"Type", 40},
	{"C-5 Freq", 80},
	{"Instr", 64},
	{"File Name", 128},
};

LISTCOLHDR gInstrumentHeaders[INSLIST_COLUMNS] =
{
	{"Instrument Name", 192},
	{"Num", 36},
	{"Samples", 64},
	{"Envelopes", 128},
	{"File Name", 128},
	{"Plugin", 128},
// -> CODE#0023
// -> DESC="IT project files (.itp)"
	{"Path", 128},
// -! NEW_FEATURE#0023
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
	if(m_nListId == 0)
	{
		m_nListId = IDC_LIST_SAMPLES;

		// For XM, set the instrument list as the default list
		CModDoc *pModDoc = GetDocument();
		CSoundFile *pSndFile;
		if(pModDoc)
		{
			pSndFile= pModDoc->GetSoundFile();
			if(pSndFile && (pSndFile->m_nType & MOD_TYPE_XM) && pSndFile->m_nInstruments > 0)
			{
				m_nListId = IDC_LIST_INSTRUMENTS;
			}
		}
	}

	CChildFrame *pFrame = (CChildFrame *)GetParentFrame();
	CRect rect;

	if (pFrame)
	{
		COMMENTVIEWSTATE *pState = pFrame->GetCommentViewState();
		if (pState->cbStruct == sizeof(COMMENTVIEWSTATE))
		{
			m_nListId = pState->nId;
		}
	}
	GetClientRect(&rect);
	m_ToolBar.Create(WS_CHILD|WS_VISIBLE|CCS_NOPARENTALIGN, rect, this, IDC_TOOLBAR_DETAILS);
	m_ToolBar.Init();
	m_ItemList.Create(WS_CHILD|WS_VISIBLE|LVS_REPORT|LVS_SINGLESEL|LVS_EDITLABELS|LVS_NOSORTHEADER, rect, this, IDC_LIST_DETAILS);
	m_ItemList.ModifyStyleEx(0, WS_EX_STATICEDGE);
	// Add ToolBar Buttons
	m_ToolBar.AddButton(IDC_LIST_SAMPLES, TIMAGE_TAB_SAMPLES);
	m_ToolBar.AddButton(IDC_LIST_INSTRUMENTS, TIMAGE_TAB_INSTRUMENTS);
	//m_ToolBar.AddButton(IDC_LIST_PATTERNS, TIMAGE_TAB_PATTERNS);
	m_ToolBar.SetIndent(4);
	UpdateButtonState();
	OnUpdate(NULL, HINT_MODTYPE, NULL);
}


void CViewComments::OnDestroy()
//-----------------------------
{
	CChildFrame *pFrame = (CChildFrame *)GetParentFrame();
	if (pFrame)
	{
		COMMENTVIEWSTATE *pState = pFrame->GetCommentViewState();
		pState->cbStruct = sizeof(COMMENTVIEWSTATE);
		pState->nId = m_nListId;
	}
	CView::OnDestroy();
}


LRESULT CViewComments::OnMidiMsg(WPARAM midiData, LPARAM)
//-------------------------------------------------------
{
	// Handle MIDI messages assigned to shortcuts
	CInputHandler *ih = CMainFrame::GetMainFrame()->GetInputHandler();
	ih->HandleMIDIMessage(kCtxViewComments, midiData) != kcNull
		|| ih->HandleMIDIMessage(kCtxAllContexts, midiData) != kcNull;
	return 1;
}


///////////////////////////////////////////////////////////////
// CViewComments drawing

void CViewComments::OnUpdate(CView *pSender, LPARAM lHint, CObject *)
//-------------------------------------------------------------------
{
	//CHAR s[256], stmp[256];
	CHAR s[512], stmp[256]; //rewbs.fix3082
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;
	LV_COLUMN lvc;
	LV_ITEM lvi, lvi2;

	if ((!pModDoc) || (pSender == this) || (!(m_ItemList.m_hWnd))) return;
	if (lHint & HINT_MPTOPTIONS)
	{
		m_ToolBar.UpdateStyle();
		lHint &= ~HINT_MPTOPTIONS;
	}
	lHint &= (HINT_MODTYPE
		|HINT_SMPNAMES|HINT_SAMPLEINFO
		|HINT_INSNAMES|HINT_INSTRUMENT
		/*|HINT_PATNAMES|HINT_PATTERNROW*/); // pattern stuff currently unused
	if (!lHint) return;
	m_ItemList.SetRedraw(FALSE);
	// Add sample headers
	if ((m_nListId != m_nCurrentListId) || (lHint & HINT_MODTYPE))
	{
		UINT ichk = 0;
		m_ItemList.DeleteAllItems();
		while ((m_ItemList.DeleteColumn(0)) && (ichk < 25)) ichk++;
		m_nCurrentListId = m_nListId;
		// Add Sample Headers
		if (m_nCurrentListId == IDC_LIST_SAMPLES)
		{
			UINT nCol = 0;
			for (UINT iSmp=0; iSmp<SMPLIST_COLUMNS; iSmp++)
			{
				MemsetZero(lvc);
				lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
				lvc.fmt = (iSmp) ? LVCFMT_RIGHT : LVCFMT_LEFT;
				lvc.pszText = (LPSTR)gSampleHeaders[iSmp].pszName;
				lvc.cx = gSampleHeaders[iSmp].cx;
				lvc.iSubItem = iSmp;
				m_ItemList.InsertColumn(nCol, &lvc);
				nCol++;
			}
		} else
		// Add Instrument Headers
		if (m_nCurrentListId == IDC_LIST_INSTRUMENTS)
		{
			UINT nCol = 0;
			for (UINT i=0; i<INSLIST_COLUMNS; i++)
			{
				MemsetZero(lvc);
				lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
				lvc.fmt = (i) ? LVCFMT_RIGHT : LVCFMT_LEFT;
				lvc.pszText = (LPSTR)gInstrumentHeaders[i].pszName;
				lvc.cx = gInstrumentHeaders[i].cx;
				lvc.iSubItem = i;
				m_ItemList.InsertColumn(nCol, &lvc);
				nCol++;
			}
		} else
		lHint |= HINT_MODTYPE;
	}
	// Add Items
	UINT nCount = m_ItemList.GetItemCount();
	pSndFile = pModDoc->GetSoundFile();
	// Add Samples
	if ((m_nCurrentListId == IDC_LIST_SAMPLES) && (lHint & (HINT_MODTYPE|HINT_SMPNAMES|HINT_SAMPLEINFO)))
	{
		SAMPLEINDEX nMax = static_cast<SAMPLEINDEX>(nCount);
		if (nMax < pSndFile->GetNumSamples()) nMax = pSndFile->GetNumSamples();
		for (SAMPLEINDEX iSmp = 0; iSmp < nMax; iSmp++)
		{
			if (iSmp < pSndFile->GetNumSamples())
			{
				UINT nCol = 0;
				for (UINT iCol=0; iCol<SMPLIST_COLUMNS; iCol++)
				{
					const ModSample &sample = pSndFile->GetSample(iSmp + 1);
					s[0] = 0;
					switch(iCol)
					{
					case SMPLIST_SAMPLENAME:
						StringFixer::Copy(s, pSndFile->m_szNames[iSmp + 1]);
						break;
					case SMPLIST_SAMPLENO:
						wsprintf(s, "%02d", iSmp + 1);
						break;
					case SMPLIST_SIZE:
						if (sample.nLength)
						{
							if(sample.GetSampleSizeInBytes() >= 1024)
								wsprintf(s, "%d KB", sample.GetSampleSizeInBytes() >> 10);
							else
								wsprintf(s, "%d B", sample.GetSampleSizeInBytes());
						}
						break;
					case SMPLIST_TYPE:
						if(sample.nLength)
						{
							wsprintf(s, "%d Bit", sample.GetElementarySampleSize() * 8);
						}
						break;
					case SMPLIST_INSTR:
						if (pSndFile->GetNumInstruments())
						{
							bool first = true;
							for (INSTRUMENTINDEX i = 1; i <= pSndFile->GetNumInstruments(); i++)
							{
								if (pSndFile->IsSampleReferencedByInstrument(iSmp + 1, i))
								{
									if (!first) strcat(s, ",");
									first = false;

									wsprintf(stmp, "%d", i);
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
							wsprintf(s, "%d Hz", sample.GetSampleRate(pSndFile->GetType()));
						}
						break;
					case SMPLIST_FILENAME:
						memcpy(s, sample.filename, sizeof(sample.filename));
						s[CountOf(sample.filename)] = 0;
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
	if ((m_nCurrentListId == IDC_LIST_INSTRUMENTS) && (lHint & (HINT_MODTYPE|HINT_INSNAMES|HINT_INSTRUMENT)))
	{
		INSTRUMENTINDEX nMax = static_cast<INSTRUMENTINDEX>(nCount);
		if (nMax < pSndFile->GetNumInstruments()) nMax = pSndFile->GetNumInstruments();
		for (INSTRUMENTINDEX iIns = 0; iIns < nMax; iIns++)
		{
			if (iIns < pSndFile->GetNumInstruments())
			{
				UINT nCol = 0;
				for (UINT iCol=0; iCol<INSLIST_COLUMNS; iCol++)
				{
					ModInstrument *pIns = pSndFile->Instruments[iIns+1];
					s[0] = 0;
					switch(iCol)
					{
					case INSLIST_INSTRUMENTNAME:
						if (pIns) StringFixer::Copy(s, pIns->name);
						break;
					case INSLIST_INSTRUMENTNO:
						wsprintf(s, "%02d", iIns+1);
						break;
					case INSLIST_SAMPLES:
						if (pIns)
						{
							const std::set<SAMPLEINDEX> referencedSamples = pIns->GetSamples();

							bool first = true;
							for(std::set<SAMPLEINDEX>::const_iterator sample = referencedSamples.begin(); sample != referencedSamples.end(); sample++)
							{
								if(!first) strcat(s, ",");
								first = false;

								size_t l = strlen(s);
								if(l >= sizeof(s) - 8)
								{
									strcat(s, "...");
									break;
								}

								wsprintf(s + l, "%d", *sample);
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
						if (pIns != nullptr && pIns->nMixPlug > 0 && pSndFile->m_MixPlugins[pIns->nMixPlug - 1].pMixPlugin != nullptr)
						{
							wsprintf(s, "FX%02d: %s", pIns->nMixPlug, pSndFile->m_MixPlugins[pIns->nMixPlug - 1].GetLibraryName());
						}
						break;
// -> CODE#0023
// -> DESC="IT project files (.itp)"
					case INSLIST_PATH:
						if (pIns)
						{
							strncpy(s, pSndFile->m_szInstrumentPath[iIns], _MAX_PATH);
							s[_MAX_PATH] = 0;
						}
						break;
// -! NEW_FEATURE#0023
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
	if ((m_nCurrentListId == IDC_LIST_PATTERNS) && (lHint & (HINT_MODTYPE|HINT_PATNAMES|HINT_PATTERNROW)))
	{
	}
	m_ItemList.SetRedraw(TRUE);
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
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		m_ToolBar.SetState(IDC_LIST_SAMPLES, ((m_nListId == IDC_LIST_SAMPLES) ? TBSTATE_CHECKED : 0)|TBSTATE_ENABLED);
		m_ToolBar.SetState(IDC_LIST_INSTRUMENTS, ((m_nListId == IDC_LIST_INSTRUMENTS) ? TBSTATE_CHECKED : 0)|TBSTATE_ENABLED);
		m_ToolBar.SetState(IDC_LIST_PATTERNS, ((m_nListId == IDC_LIST_PATTERNS) ? TBSTATE_CHECKED : 0)|TBSTATE_ENABLED);
		m_ToolBar.EnableButton(IDC_LIST_INSTRUMENTS, (pSndFile->m_nInstruments) ? TRUE : FALSE);
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
		CSoundFile *pSndFile = pModDoc->GetSoundFile();

		if(m_nListId == IDC_LIST_SAMPLES)
		{
			if(iItem < pSndFile->GetNumSamples())
			{
				StringFixer::CopyN(pSndFile->m_szNames[iItem + 1], lvItem.pszText);
				pModDoc->UpdateAllViews(this, ((iItem + 1) << HINT_SHIFT_SMP) | (HINT_SMPNAMES | HINT_SAMPLEINFO), this);
				pModDoc->SetModified();
			}
		} else if(m_nListId == IDC_LIST_INSTRUMENTS)
		{
			if((iItem < pSndFile->GetNumInstruments()) && (pSndFile->Instruments[iItem + 1]))
			{
				ModInstrument *pIns = pSndFile->Instruments[iItem + 1];
				StringFixer::CopyN(pIns->name, lvItem.pszText);
				pModDoc->UpdateAllViews(this, ((iItem + 1) << HINT_SHIFT_INS) | (HINT_INSNAMES | HINT_INSTRUMENT), this);
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
		OnUpdate(NULL, HINT_MODTYPE, NULL);
	}
}


void CViewComments::OnShowInstruments()
//-------------------------------------
{
	if (m_nListId != IDC_LIST_INSTRUMENTS)
	{
		CModDoc *pModDoc = GetDocument();
		if (pModDoc)
		{
			CSoundFile *pSndFile = pModDoc->GetSoundFile();
			if (pSndFile->m_nInstruments)
			{
				m_nListId = IDC_LIST_INSTRUMENTS;
				UpdateButtonState();
				OnUpdate(NULL, HINT_MODTYPE, NULL);
			}
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
		OnUpdate(NULL, HINT_MODTYPE, NULL);
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
