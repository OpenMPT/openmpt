/*
 * EffectVis.h
 * -----------
 * Purpose: Implementation of parameter visualisation dialog.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "EffectInfo.h"

OPENMPT_NAMESPACE_BEGIN

class CViewPattern;
class CModDoc;
class CSoundFile;

#define FXVSTATUS_LDRAGGING		0x01
#define FXVSTATUS_RDRAGGING		0x02

// EffectVis dialog
class CEffectVis : public CDialog
{
	DECLARE_DYNAMIC(CEffectVis)

public:
	enum EditAction
	{
		kAction_OverwriteFX,
		kAction_FillFX,
		kAction_OverwritePC,
		kAction_FillPC,
		kAction_Preserve
	};

	CEffectVis(CViewPattern *pViewPattern, ROWINDEX startRow, ROWINDEX endRow, CHANNELINDEX nchn, CModDoc &modDoc, PATTERNINDEX pat);

protected:
	void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	void PostNcDestroy() override;

	EffectInfo effectInfo;

	CBitmap m_bGrid, m_bNodes, m_bPlayPos;
	HBITMAP m_pbOldGrid = nullptr, m_pbOldNodes = nullptr, m_pbOldPlayPos = nullptr;
	CDC m_dcGrid, m_dcNodes, m_dcPlayPos;

	void DrawNodes();
	void DrawGrid();

	void ShowVis(CDC *pDC);
	void ShowVisImage(CDC *pDC);
	RECT invalidated;

	ROWINDEX m_nLastDrawnRow = ROWINDEX_INVALID; // for interpolation
	int m_nLastDrawnY = -1; // for interpolation
	int m_nRowToErase = -1;
	int m_nParamToErase = -1;

	int m_nodeSizeHalf;	// Half width of a node;
	int m_marginBottom;
	int m_innerBorder;

	ROWINDEX m_nOldPlayPos = ROWINDEX_INVALID;
	ModCommand m_templatePCNote;

protected:
	ROWINDEX m_startRow;
	ROWINDEX m_endRow;
	ROWINDEX m_nRows;
	CHANNELINDEX m_nChan;
	PATTERNINDEX m_nPattern;
	int m_nFillEffect;
	EditAction m_nAction = kAction_OverwriteFX;

	int m_nDragItem = -1;
	UINT m_nBtnMouseOver;
	DWORD m_dwStatus = 0;

	float m_pixelsPerRow = 1, m_pixelsPerFXParam = 1, m_pixelsPerPCParam = 1;

	bool m_forceRedraw  = true;

	void InvalidateRow(int row);
	int RowToScreenX(ROWINDEX row) const;
	int RowToScreenY(ROWINDEX row) const;
	int PCParamToScreenY(uint16 param) const;
	int FXParamToScreenY(uint16 param) const;
	uint16 GetParam(ROWINDEX row) const;
	EffectCommand GetCommand(ROWINDEX row) const;
	void SetParamFromY(ROWINDEX row, int y);
	void SetCommand(ROWINDEX row, EffectCommand cmd);
	ModCommand::PARAM ScreenYToFXParam(int y) const;
	uint16 ScreenYToPCParam(int y) const;
	ROWINDEX ScreenXToRow(int x) const;
	bool IsPcNote(ROWINDEX row) const;
	void SetPcNote(ROWINDEX row);

	CModDoc &m_ModDoc;
	CSoundFile &m_SndFile;
	CRect m_rcDraw;
	CRect m_rcFullWin;

	CComboBox m_cmbEffectList, m_cmbActionList;
	CEdit m_edVisStatus;

	void OnOK() override;
	void OnCancel() override;
	afx_msg void OnClose();

	CViewPattern *m_pViewPattern;


	DECLARE_MESSAGE_MAP()
	BOOL OnInitDialog() override;
	afx_msg void OnPaint();

public:

	void UpdateSelection(ROWINDEX startRow, ROWINDEX endRow, CHANNELINDEX nchn, PATTERNINDEX pat);
	void Update();
	void OpenEditor(CWnd *parent);
	void SetPlayCursor(PATTERNINDEX nPat, ROWINDEX nRow);
	void DoClose();

	afx_msg void OnSize(UINT nType, int cx, int cy);

protected:
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnEffectChanged();
	afx_msg void OnActionChanged();
	afx_msg BOOL OnEraseBkgnd(CDC *) { return TRUE; }

	void MakeChange(ROWINDEX currentRow, int newY);
};

OPENMPT_NAMESPACE_END
