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
#include "view_com.h"
#include "Childfrm.h"
#include "Clipboard.h"
#include "Ctrl_com.h"
#include "Globals.h"
#include "ImageLists.h"
#include "InputHandler.h"
#include "Mainfrm.h"
#include "Moddoc.h"
#include "Mptrack.h"
#include "Reporting.h"
#include "resource.h"
#include "TrackerSettings.h"
#include "WindowMessages.h"
#include "../common/mptStringBuffer.h"
#include "../soundlib/mod_specifications.h"

OPENMPT_NAMESPACE_BEGIN


#define DETAILS_TOOLBAR_CY HighDPISupport::ScalePixels(28, m_hWnd)

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


static constexpr CListCtrlEx::Header SampleHeaders[SMPLIST_COLUMNS] =
{
	{ _T("Sample Name"), 212, LVCFMT_LEFT },
	{ _T("Num"),         45,  LVCFMT_RIGHT },
	{ _T("Size"),        72,  LVCFMT_RIGHT },
	{ _T("Type"),        80,  LVCFMT_RIGHT },
	{ _T("C-5 Freq"),    80,  LVCFMT_RIGHT },
	{ _T("Instr"),       64,  LVCFMT_RIGHT },
	{ _T("File Name"),   160, LVCFMT_RIGHT },
	{ _T("Path"),        256, LVCFMT_LEFT },
};

static constexpr CListCtrlEx::Header InstrumentHeaders[INSLIST_COLUMNS] =
{
	{ _T("Instrument Name"), 212, LVCFMT_LEFT },
	{ _T("Num"),             45,  LVCFMT_RIGHT },
	{ _T("Samples"),         64,  LVCFMT_RIGHT },
	{ _T("Envelopes"),       128, LVCFMT_RIGHT },
	{ _T("File Name"),       160, LVCFMT_RIGHT },
	{ _T("Plugin"),          128, LVCFMT_RIGHT },
};


IMPLEMENT_SERIAL(CViewComments, CModScrollView, 0)

BEGIN_MESSAGE_MAP(CViewComments, CModScrollView)
	//{{AFX_MSG_MAP(CViewComments)
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_MESSAGE(WM_MOD_KEYCOMMAND,       &CViewComments::OnCustomKeyMsg)
	ON_MESSAGE(WM_MOD_MIDIMSG,          &CViewComments::OnMidiMsg)
	ON_COMMAND(IDC_LIST_SAMPLES,        &CViewComments::OnShowSamples)
	ON_COMMAND(IDC_LIST_INSTRUMENTS,    &CViewComments::OnShowInstruments)
	ON_COMMAND(IDC_LIST_PATTERNS,       &CViewComments::OnShowPatterns)
	ON_COMMAND(ID_EDIT_COPY,            &CViewComments::OnCopyNames)
	ON_COMMAND(ID_EDIT_PASTE,           &CViewComments::OnPasteNames)
	ON_NOTIFY(LVN_ENDLABELEDIT,   IDC_LIST_DETAILS, &CViewComments::OnEndLabelEdit)
	ON_NOTIFY(LVN_BEGINLABELEDIT, IDC_LIST_DETAILS, &CViewComments::OnBeginLabelEdit)
	ON_NOTIFY(NM_DBLCLK,          IDC_LIST_DETAILS, &CViewComments::OnDblClickListItem)
	ON_NOTIFY(NM_RCLICK,          IDC_LIST_DETAILS, &CViewComments::OnRClickListItem)
	ON_NOTIFY(NM_CUSTOMDRAW,      IDC_LIST_DETAILS, &CViewComments::OnCustomDrawList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CViewComments::OnInitialUpdate()
{
	CModScrollView::OnInitialUpdate();
	if(m_nListId == 0)
	{
		m_nListId = IDC_LIST_SAMPLES;

		// For XM, set the instrument list as the default list
		const CModDoc *pModDoc = GetDocument();
		if(pModDoc && pModDoc->GetSoundFile().GetMessageHeuristic() == ModMessageHeuristicOrder::InstrumentsSamples && pModDoc->GetNumInstruments() > 0)
		{
			m_nListId = IDC_LIST_INSTRUMENTS;
		}
	}

	CChildFrame *pFrame = (CChildFrame *)GetParentFrame();
	CRect rect;

	if (pFrame)
	{
		CommentsViewState &commentState = pFrame->GetCommentViewState();
		if (commentState.initialized)
		{
			m_nListId = commentState.nId;
		}
	}
	GetClientRect(&rect);
	m_ToolBar.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | CCS_NOPARENTALIGN, rect, this, IDC_TOOLBAR_DETAILS);
	m_ToolBar.Init(CMainFrame::GetMainFrame()->m_MiscIcons, CMainFrame::GetMainFrame()->m_MiscIconsDisabled);
	m_ItemList.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_EDITLABELS | LVS_NOSORTHEADER, rect, this, IDC_LIST_DETAILS);
	m_ItemList.ModifyStyleEx(0, WS_EX_STATICEDGE);

	// Add ToolBar Buttons
	m_ToolBar.AddButton(IDC_LIST_SAMPLES, IMAGE_SAMPLES);
	m_ToolBar.AddButton(IDC_LIST_INSTRUMENTS, IMAGE_INSTRUMENTS);
	//m_ToolBar.AddButton(IDC_LIST_PATTERNS, TIMAGE_TAB_PATTERNS);
	UpdateButtonState();
	OnDPIChanged();
}


void CViewComments::OnDPIChanged()
{
	UpdateView(GeneralHint().MPTOptions().ModType());
	m_ToolBar.SetIndent(HighDPISupport::ScalePixels(4, m_hWnd));
	const int imgSize = HighDPISupport::ScalePixels(16, m_hWnd), btnSizeX = HighDPISupport::ScalePixels(26, m_hWnd), btnSizeY = HighDPISupport::ScalePixels(24, m_hWnd);
	m_ToolBar.SetButtonSize(CSize(btnSizeX, btnSizeY));
	m_ToolBar.SetBitmapSize(CSize(imgSize, imgSize));
	RecalcLayout();
	CModScrollView::OnDPIChanged();
}


// cppcheck-suppress duplInheritedMember
void CViewComments::OnDestroy()
{
	if(m_lastNote != NOTE_NONE)
		GetDocument()->NoteOff(m_lastNote, true, m_noteInstr, m_noteChannel);

	CChildFrame *pFrame = (CChildFrame *)GetParentFrame();
	if (pFrame)
	{
		CommentsViewState &commentState = pFrame->GetCommentViewState();
		commentState.initialized = true;
		commentState.nId = m_nListId;
	}
	m_fixedFont.DeleteObject();
	CModScrollView::OnDestroy();
}


LRESULT CViewComments::OnModViewMsg(WPARAM wParam, LPARAM lParam)
{
	switch(wParam)
	{
		case VIEWMSG_SETFOCUS:
		case VIEWMSG_SETACTIVE:
			GetParentFrame()->SetActiveView(this);
			m_ItemList.SetFocus();
			return 0;
		default:
			return CModScrollView::OnModViewMsg(wParam, lParam);
	}
}


LRESULT CViewComments::OnMidiMsg(WPARAM midiData_, LPARAM)
{
	uint32 midiData = static_cast<uint32>(midiData_);
	INSTRUMENTINDEX ins = 0;
	SAMPLEINDEX smp = 0;

	const int item = m_ItemList.GetSelectionMark() + 1;
	if(item > 0 && m_nListId == IDC_LIST_SAMPLES)
		smp = static_cast<SAMPLEINDEX>(item);
	else if(item > 0 && m_nListId == IDC_LIST_INSTRUMENTS)
		ins = static_cast<INSTRUMENTINDEX>(item);

	GetDocument()->ProcessMIDI(midiData, smp, ins, GetDocument()->GetSoundFile().GetInstrumentPlugin(ins), kCtxViewComments);
	return 1;
}


LRESULT CViewComments::OnCustomKeyMsg(WPARAM wParam, LPARAM)
{
	const int item = m_ItemList.GetSelectionMark() + 1;
	if(item == 0)
		return kcNull;

	auto modDoc = GetDocument();
	if(wParam >= kcCommentsStartNotes && wParam <= kcCommentsEndNotes)
	{
		const auto lastInstr = m_noteInstr;
		m_noteInstr = (m_nListId == IDC_LIST_SAMPLES) ? INSTRUMENTINDEX_INVALID : static_cast<INSTRUMENTINDEX>(item);
		const auto note = modDoc->GetNoteWithBaseOctave(static_cast<int>(wParam - kcCommentsStartNotes), m_noteInstr);
		PlayNoteParam params(note);
		if(m_nListId == IDC_LIST_SAMPLES)
			params.Sample(static_cast<SAMPLEINDEX>(item));
		else if(m_nListId == IDC_LIST_INSTRUMENTS)
			params.Instrument(m_noteInstr);
		else
			return kcNull;
		if(m_lastNote != NOTE_NONE)
			modDoc->NoteOff(m_lastNote, true, lastInstr, m_noteChannel);
		m_noteChannel = modDoc->PlayNote(params);
		m_lastNote = note;
		return wParam;
	} else if(wParam >= kcCommentsStartNoteStops && wParam <= kcCommentsEndNoteStops)
	{
		const auto note = modDoc->GetNoteWithBaseOctave(static_cast<int>(wParam - kcCommentsStartNoteStops), m_noteInstr);
		modDoc->NoteOff(note, false, m_noteInstr, m_noteChannel);
		return wParam;
	} else if(wParam == kcToggleSmpInsList)
	{
		bool ok = SwitchToList(m_nListId == IDC_LIST_SAMPLES ? IDC_LIST_INSTRUMENTS : IDC_LIST_SAMPLES);
		if(ok)
		{
			int newItem = 0;
			switch(m_nListId)
			{
			case IDC_LIST_SAMPLES:
				// Switch to a sample belonging to previously selected instrument
				if(SAMPLEINDEX smp = modDoc->FindInstrumentChild(static_cast<INSTRUMENTINDEX>(item)); smp != 0 && smp != SAMPLEINDEX_INVALID)
					newItem = smp - 1;
				break;

			case IDC_LIST_INSTRUMENTS:
				// Switch to parent instrument of previously selected sample
				if(INSTRUMENTINDEX ins = modDoc->FindSampleParent(static_cast<SAMPLEINDEX>(item)); ins != 0 && ins != INSTRUMENTINDEX_INVALID)
					newItem = ins - 1;
				break;
			}
			m_ItemList.SetItemState(newItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
			m_ItemList.SetSelectionMark(newItem);
			m_ItemList.EnsureVisible(newItem, FALSE);
			m_ItemList.SetFocus();
		}
		return wParam;
	} else if(wParam == kcExecuteSmpInsListItem)
	{
		OnDblClickListItem(nullptr, nullptr);
		return wParam;
	} else if(wParam == kcRenameSmpInsListItem)
	{
		m_ItemList.EditLabel(item - 1);
		return wParam;
	} else if(wParam == kcEditCopy)
	{
		OnCopyNames();
		return wParam;
	} else if(wParam == kcEditPaste)
	{
		OnPasteNames();
		return wParam;
	}
	return kcNull;
}


BOOL CViewComments::PreTranslateMessage(MSG *pMsg)
{
	if(pMsg)
	{
		if(m_editLabel && pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE)
		{
			m_editLabel = false;
			m_ItemList.SetFocus();
			return TRUE;
		}

		if((pMsg->message == WM_SYSKEYUP) || (pMsg->message == WM_KEYUP)
			|| (pMsg->message == WM_SYSKEYDOWN) || (pMsg->message == WM_KEYDOWN))
		{
			CInputHandler *ih = CMainFrame::GetInputHandler();
			if(!ih->IsBypassed() && ih->KeyEvent(kCtxViewComments, ih->Translate(*pMsg)) != kcNull)
			{
				return TRUE;  // Mapped to a command, no need to pass message on.
			}
		} else if(pMsg->message == WM_CHAR)
		{
			// Avoid Windows warning sound when holding note key
			if(!CMainFrame::GetInputHandler()->IsBypassed() && CInputHandler::GetKeyEventType(*pMsg) == kKeyEventRepeat)
				return TRUE;
		}
	}

	return CModScrollView::PreTranslateMessage(pMsg);
}


///////////////////////////////////////////////////////////////
// CViewComments drawing

void CViewComments::UpdateView(UpdateHint hint, CObject *)
{
	const CModDoc *pModDoc = GetDocument();

	if ((!pModDoc) || (!(m_ItemList.m_hWnd))) return;
	const FlagSet<HintType> hintType = hint.GetType();
	if (hintType[HINT_MPTOPTIONS])
	{
		m_ToolBar.UpdateStyle();
		
		// Font for sample / instrument names
		CFont *font = m_ItemList.GetFont();
		LOGFONT lf;
		font->GetLogFont(&lf);
		mpt::WriteWinBuf(lf.lfFaceName) = mpt::ToWin(TrackerSettings::Instance().commentsFont.Get().name);
		m_fixedFont.DeleteObject();
		m_fixedFont.CreateFontIndirect(&lf);

		m_ItemList.Invalidate(FALSE);
	}
	const SampleHint sampleHint = hint.ToType<SampleHint>();
	const InstrumentHint instrHint = hint.ToType<InstrumentHint>();
	const bool updateSamples = sampleHint.GetType()[HINT_SMPNAMES | HINT_SAMPLEINFO];
	const bool updateInstr = instrHint.GetType()[HINT_INSNAMES|HINT_INSTRUMENT];
	bool updateAll = hintType[HINT_MODTYPE];

	if(!updateSamples && !updateInstr && !updateAll) return;

	const CSoundFile &sndFile = pModDoc->GetSoundFile();

	m_ToolBar.ChangeBitmap(IDC_LIST_INSTRUMENTS, sndFile.GetNumInstruments() ? IMAGE_INSTRUMENTS : IMAGE_INSTRMUTE);

	CString s;
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
			m_ItemList.SetHeaders(SampleHeaders);
		} else if (m_nCurrentListId == IDC_LIST_INSTRUMENTS)
		{
			// Add Instrument Headers
			m_ItemList.SetHeaders(InstrumentHeaders);
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
					s.Empty();
					switch(iCol)
					{
					case SMPLIST_SAMPLENAME:
						s = mpt::ToCString(sndFile.GetCharsetInternal(), sndFile.m_szNames[iSmp + 1]);
						break;
					case SMPLIST_SAMPLENO:
						s = mpt::cfmt::dec0<2>(iSmp + 1);
						break;
					case SMPLIST_SIZE:
						if(sample.nLength && !sample.uFlags[CHN_ADLIB])
							s = FormatFileSize(sample.GetSampleSizeInBytes());
						break;
					case SMPLIST_TYPE:
						if(sample.uFlags[CHN_ADLIB])
							s = _T("OPL");
						else if(sample.HasSampleData())
							s = MPT_CFORMAT("{}-bit {}")(sample.GetElementarySampleSize() * 8, (sample.GetNumChannels() == 2) ? CString(_T("stereo")) : CString(_T("mono")));
						break;
					case SMPLIST_INSTR:
						if (sndFile.GetNumInstruments())
						{
							bool first = true;
							for (INSTRUMENTINDEX i = 1; i <= sndFile.GetNumInstruments(); i++)
							{
								if (sndFile.IsSampleReferencedByInstrument(iSmp + 1, i))
								{
									if (!first) s.AppendChar(_T(','));
									first = false;

									s.AppendFormat(_T("%u"), i);
								}
							}
						}
						break;
					case SMPLIST_MIDDLEC:
						if (sample.nLength)
						{
							s.Format(_T("%u Hz"), sample.GetSampleRate(sndFile.GetType()));
						}
						break;
					case SMPLIST_FILENAME:
						s = mpt::ToCString(sndFile.GetCharsetInternal(), sample.filename);
						break;
					case SMPLIST_PATH:
						s = sndFile.GetSamplePath(iSmp + 1).ToCString();
						break;
					}
					lvi.mask = LVIF_TEXT;
					lvi.iItem = iSmp;
					lvi.iSubItem = nCol;
					lvi.pszText = const_cast<TCHAR *>(s.GetString());
					if ((iCol) || (iSmp < nCount))
					{
						bool update = true;
						if (iSmp < nCount)
						{
							TCHAR stmp[512];
							lvi2 = lvi;
							lvi2.pszText = stmp;
							lvi2.cchTextMax = mpt::saturate_cast<int>(std::size(stmp));
							stmp[0] = 0;
							m_ItemList.GetItem(&lvi2);
							if (s == stmp) update = false;
						}
						if (update) m_ItemList.SetItem(&lvi);
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
					s.Empty();
					switch(iCol)
					{
					case INSLIST_INSTRUMENTNAME:
						if (pIns) s = mpt::ToCString(sndFile.GetCharsetInternal(), pIns->name);
						break;
					case INSLIST_INSTRUMENTNO:
						s = mpt::cfmt::dec0<2>(iIns + 1);
						break;
					case INSLIST_SAMPLES:
						if (pIns)
						{
							bool first = true;
							for(auto sample : pIns->GetSamples())
							{
								if(!first) s.AppendChar(_T(','));
								first = false;
								s.AppendFormat(_T("%u"), sample);
							}
						}
						break;
					case INSLIST_ENVELOPES:
						if (pIns)
						{
							if (pIns->VolEnv.dwFlags[ENV_ENABLED]) s += _T("Vol");
							if (pIns->PanEnv.dwFlags[ENV_ENABLED]) { if (!s.IsEmpty()) s += _T(", "); s += _T("Pan"); }
							if (pIns->PitchEnv.dwFlags[ENV_ENABLED]) { if (!s.IsEmpty()) s += _T(", "); s += (pIns->PitchEnv.dwFlags[ENV_FILTER] ? _T("Filter") : _T("Pitch")); }
						}
						break;
					case INSLIST_FILENAME:
						if (pIns)
						{
							s = mpt::ToCString(sndFile.GetCharsetInternal(), pIns->filename);
						}
						break;
					case INSLIST_PLUGIN:
						if (pIns != nullptr && pIns->nMixPlug > 0 && sndFile.m_MixPlugins[pIns->nMixPlug - 1].IsValidPlugin())
						{
							s.Format(_T("FX%02u: "), pIns->nMixPlug);
							s += mpt::ToCString(sndFile.m_MixPlugins[pIns->nMixPlug - 1].GetLibraryName());
						}
						break;
					}
					lvi.mask = LVIF_TEXT;
					lvi.iItem = iIns;
					lvi.iSubItem = nCol;
					lvi.pszText = const_cast<TCHAR *>(s.GetString());
					if ((iCol) || (iIns < nCount))
					{
						bool update = true;
						if (iIns < nCount)
						{
							TCHAR stmp[512];
							lvi2 = lvi;
							lvi2.pszText = stmp;
							lvi2.cchTextMax = mpt::saturate_cast<int>(std::size(stmp));
							stmp[0] = 0;
							m_ItemList.GetItem(&lvi2);
							if (s == stmp) update = false;
						}
						if (update) m_ItemList.SetItem(&lvi);
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
}


void CViewComments::RecalcLayout()
{
	CRect rect;

	if (!m_hWnd) return;
	GetClientRect(&rect);
	m_ToolBar.SetWindowPos(NULL, 0, 0, rect.Width(), DETAILS_TOOLBAR_CY, SWP_NOZORDER|SWP_NOACTIVATE);
	m_ItemList.SetWindowPos(NULL, -1, DETAILS_TOOLBAR_CY, rect.Width()+2, rect.Height() - DETAILS_TOOLBAR_CY + 1, SWP_NOZORDER|SWP_NOACTIVATE);
}


void CViewComments::UpdateButtonState()
{
	const CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		m_ToolBar.SetState(IDC_LIST_SAMPLES, ((m_nListId == IDC_LIST_SAMPLES) ? TBSTATE_CHECKED : 0)|TBSTATE_ENABLED);
		m_ToolBar.SetState(IDC_LIST_INSTRUMENTS, ((m_nListId == IDC_LIST_INSTRUMENTS) ? TBSTATE_CHECKED : 0)|TBSTATE_ENABLED);
		m_ToolBar.SetState(IDC_LIST_PATTERNS, ((m_nListId == IDC_LIST_PATTERNS) ? TBSTATE_CHECKED : 0)|TBSTATE_ENABLED);
		m_ToolBar.EnableButton(IDC_LIST_INSTRUMENTS, (pModDoc->GetNumInstruments()) ? TRUE : FALSE);
	}
}


void CViewComments::OnBeginLabelEdit(LPNMHDR, LRESULT *)
{
	CEdit *editCtrl = m_ItemList.GetEditControl();
	if(editCtrl)
	{
		m_editLabel = true;
		const CModSpecifications &specs = GetDocument()->GetSoundFile().GetModSpecifications();
		const auto maxStrLen = (m_nListId == IDC_LIST_SAMPLES) ? specs.sampleNameLengthMax : specs.instrNameLengthMax;
		editCtrl->LimitText(maxStrLen);
		CMainFrame::GetInputHandler()->Bypass(true);
	}
}


void CViewComments::OnEndLabelEdit(LPNMHDR pnmhdr, LRESULT *result)
{
	CMainFrame::GetInputHandler()->Bypass(false);
	if(!m_editLabel)
	{
		*result = FALSE;
		return;
	}
	m_editLabel = false;

	LV_DISPINFO &lvDispInfo = *reinterpret_cast<LV_DISPINFO *>(pnmhdr);
	LV_ITEM &lvItem = lvDispInfo.item;
	CModDoc *pModDoc = GetDocument();

	if(lvItem.pszText != nullptr && !lvItem.iSubItem && pModDoc)
	{
		UINT iItem = lvItem.iItem;
		CSoundFile &sndFile = pModDoc->GetSoundFile();

		if(m_nListId == IDC_LIST_SAMPLES)
		{
			if(iItem < sndFile.GetNumSamples())
			{
				sndFile.m_szNames[iItem + 1] = mpt::ToCharset(sndFile.GetCharsetInternal(), CString(lvItem.pszText));
				pModDoc->UpdateAllViews(this, SampleHint(static_cast<SAMPLEINDEX>(iItem + 1)).Info().Names(), this);
				pModDoc->SetModified();
			}
		} else if(m_nListId == IDC_LIST_INSTRUMENTS)
		{
			if((iItem < sndFile.GetNumInstruments()) && (sndFile.Instruments[iItem + 1]))
			{
				ModInstrument *pIns = sndFile.Instruments[iItem + 1];
				pIns->name = mpt::ToCharset(sndFile.GetCharsetInternal(), CString(lvItem.pszText));
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


void CViewComments::OnSize(UINT nType, int cx, int cy)
{
	CModScrollView::OnSize(nType, cx, cy);
	if (((nType == SIZE_RESTORED) || (nType == SIZE_MAXIMIZED)) && (cx > 0) && (cy > 0) && (m_hWnd))
	{
		RecalcLayout();
	}
}


void CViewComments::OnShowSamples() { SwitchToList(IDC_LIST_SAMPLES); }
void CViewComments::OnShowInstruments() { SwitchToList(IDC_LIST_INSTRUMENTS); }
void CViewComments::OnShowPatterns() { SwitchToList(IDC_LIST_PATTERNS); }


bool CViewComments::SwitchToList(int list)
{
	if(list == m_nListId)
		return false;

	if(list == IDC_LIST_SAMPLES)
	{
		m_nListId = IDC_LIST_SAMPLES;
		UpdateButtonState();
		UpdateView(UpdateHint().ModType());
	} else if(list == IDC_LIST_INSTRUMENTS)
	{
		const CModDoc *modDoc = GetDocument();
		if(!modDoc || !modDoc->GetNumInstruments())
			return false;

		m_nListId = IDC_LIST_INSTRUMENTS;
		UpdateButtonState();
		UpdateView(UpdateHint().ModType());
	/*} else if(list == IDC_LIST_PATTERNS)
	{
		m_nListId = IDC_LIST_PATTERNS;
		UpdateButtonState();
		UpdateView(UpdateHint().ModType());*/
	} else
	{
		return false;
	}
	return true;
}


void CViewComments::OnDblClickListItem(NMHDR *, LRESULT *)
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


void CViewComments::OnRClickListItem(NMHDR *, LRESULT *)
{
	const auto ih = CMainFrame::GetMainFrame()->GetInputHandler();
	HMENU menu = ::CreatePopupMenu();
	::AppendMenu(menu, MF_STRING, ID_EDIT_COPY, ih->GetKeyTextFromCommand(kcEditCopy, _T("&Copy Names")));
	::AppendMenu(menu, MF_STRING | (IsClipboardFormatAvailable(CF_UNICODETEXT) ? 0 : MF_DISABLED), ID_EDIT_PASTE, ih->GetKeyTextFromCommand(kcEditPaste, _T("&Paste Names")));
	CPoint pt;
	::GetCursorPos(&pt);
	::TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hWnd, NULL);
	::DestroyMenu(menu);

}


void CViewComments::OnCustomDrawList(NMHDR *pNMHDR, LRESULT *pResult)
{
	const auto &lvcd = *reinterpret_cast<NMLVCUSTOMDRAW *>(pNMHDR);
	*pResult = CDRF_DODEFAULT;

	const bool useFont = lvcd.iSubItem == 0 || (m_nListId == IDC_LIST_SAMPLES && lvcd.iSubItem == SMPLIST_FILENAME) || (m_nListId == IDC_LIST_INSTRUMENTS && lvcd.iSubItem == INSLIST_FILENAME);
	switch(lvcd.nmcd.dwDrawStage)
	{
		case CDDS_PREPAINT:
			*pResult = CDRF_NOTIFYITEMDRAW;
			break;
		case CDDS_ITEMPREPAINT:
			*pResult = CDRF_NOTIFYSUBITEMDRAW;
			break;
		case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
			if(useFont)
			{
				m_oldFont = SelectFont(lvcd.nmcd.hdc, m_fixedFont);
				*pResult = CDRF_NEWFONT | CDRF_NOTIFYPOSTPAINT;
			}
			break;
		case CDDS_ITEMPOSTPAINT | CDDS_SUBITEM:
			if(useFont)
			{
				SelectFont(lvcd.nmcd.hdc, m_oldFont);
				*pResult = CDRF_NEWFONT;
			}
			break;
	}
}


void CViewComments::OnCopyNames()
{
	std::wstring names;
	const CSoundFile &sndFile = GetDocument()->GetSoundFile();
	if(m_nListId == IDC_LIST_SAMPLES)
	{
		for(SAMPLEINDEX i = 1; i <= sndFile.GetNumSamples(); i++)
			names += mpt::ToWide(sndFile.GetCharsetInternal(), sndFile.GetSampleName(i)) + L"\r\n";
	} else if(m_nListId == IDC_LIST_INSTRUMENTS)
	{
		for(INSTRUMENTINDEX i = 1; i <= sndFile.GetNumInstruments(); i++)
			names += mpt::ToWide(sndFile.GetCharsetInternal(), sndFile.GetInstrumentName(i)) + L"\r\n";
	}
	const size_t sizeBytes = (names.length() + 1) * sizeof(wchar_t);
	Clipboard clipboard(CF_UNICODETEXT, sizeBytes);
	if(auto dst = clipboard.Get(); dst.data())
	{
		std::memcpy(dst.data(), names.c_str(), sizeBytes);
	}
}


void CViewComments::OnPasteNames()
{
	Clipboard clipboard(CF_UNICODETEXT);
	if(!clipboard.IsValid())
		return;

	if(Reporting::Confirm(MPT_UFORMAT("Replace all {} names?")(m_nListId == IDC_LIST_INSTRUMENTS ? U_("instrument") : U_("sample"))) != cnfYes)
		return;

	auto whitespace = mpt::default_whitespace<std::wstring>();
	whitespace.push_back(L'\0');
	const auto names = mpt::split(mpt::trim_right(std::wstring{clipboard.GetWideString()}, whitespace), std::wstring{L"\n"});

	CSoundFile &sndFile = GetDocument()->GetSoundFile();
	const auto FormatName = [&](size_t index, size_t maxLength)
	{
		if(index >= names.size())
			return std::string{};
		return mpt::replace(mpt::ToCharset(sndFile.GetCharsetInternal(), names[index]), "\t", " ").substr(0, maxLength);
	};

	CriticalSection cs;
	if(m_nListId == IDC_LIST_SAMPLES)
	{
		if(sndFile.GetNumSamples() < names.size())
			sndFile.m_nSamples = std::min(sndFile.GetModSpecifications().samplesMax, mpt::saturate_cast<SAMPLEINDEX>(names.size()));

		for(SAMPLEINDEX i = 1; i <= sndFile.GetNumSamples(); i++)
		{
			sndFile.m_szNames[i] = FormatName(i - 1, sndFile.GetModSpecifications().sampleNameLengthMax);
		}
		cs.Leave();
		GetDocument()->UpdateAllViews(SampleHint().Names());
	} else if(m_nListId == IDC_LIST_INSTRUMENTS)
	{
		if(sndFile.GetNumInstruments() < names.size())
			sndFile.m_nInstruments = std::min(sndFile.GetModSpecifications().instrumentsMax, mpt::saturate_cast<INSTRUMENTINDEX>(names.size()));

		for(INSTRUMENTINDEX i = 1; i <= sndFile.GetNumInstruments(); i++)
		{
			if(sndFile.Instruments[i] || sndFile.AllocateInstrument(i))
				sndFile.Instruments[i]->name = FormatName(i - 1, sndFile.GetModSpecifications().instrNameLengthMax);
		}
		cs.Leave();
		GetDocument()->UpdateAllViews(InstrumentHint().Names());
	}

	GetDocument()->SetModified();
}


OPENMPT_NAMESPACE_END
