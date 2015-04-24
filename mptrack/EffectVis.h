/*
 * EffectVis.h
 * -----------
 * Purpose: Implementation of parameter visualisation dialog.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "EffectInfo.h"

OPENMPT_NAMESPACE_BEGIN

class CViewPattern;

#define FXVSTATUS_LDRAGGING		0x01
#define FXVSTATUS_RDRAGGING		0x02

// EffectVis dialog
class CEffectVis : public CDialog
{
	DECLARE_DYNAMIC(CEffectVis)

public:
	enum
	{
		kAction_OverwriteFX,
		kAction_FillFX,
		kAction_OverwritePC,
		kAction_FillPC,
		kAction_Preserve
	};

	CEffectVis(CViewPattern *pViewPattern, ROWINDEX startRow, ROWINDEX endRow, CHANNELINDEX nchn, CModDoc *pModDoc, PATTERNINDEX pat);
	~CEffectVis();

// Dialog Data
	enum { IDD = IDD_EFFECTVISUALIZER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	EffectInfo effectInfo;

	CBitmap m_bGrid, m_bNodes, m_bPlayPos;
	CBitmap *m_pbOldGrid, *m_pbOldNodes, *m_pbOldPlayPos;
	CDC m_dcGrid, m_dcNodes, m_dcPlayPos;
	void DrawNodes();
	void DrawGrid();

	void ShowVis(CDC * pDC, CRect rectBorder);
	void ShowVisImage(CDC *pDC);
	RECT invalidated;

	ROWINDEX m_nLastDrawnRow; // for interpolation
	int m_nLastDrawnY; // for interpolation
	int m_nRowToErase;
	int m_nParamToErase;

	int m_nodeSizeHalf;	// Half width of a node;
	int m_marginBottom;
	int m_innerBorder;

	ROWINDEX m_nOldPlayPos;
	ModCommand m_templatePCNote;

protected:
	ROWINDEX m_startRow;
	ROWINDEX m_endRow;
	ROWINDEX m_nRows;
	CHANNELINDEX m_nChan;
	PATTERNINDEX m_nPattern;
	int m_nFillEffect, m_nAction;

	int m_nDragItem;
	UINT m_nBtnMouseOver;
	DWORD m_dwStatus;

	float m_pixelsPerRow, m_pixelsPerFXParam, m_pixelsPerPCParam;

	bool m_forceRedraw : 1;

	void InvalidateRow(int row);
	int RowToScreenX(ROWINDEX row) const;
	int RowToScreenY(ROWINDEX row) const;
	int PCParamToScreenY(uint16 param) const;
	int FXParamToScreenY(uint16 param) const;
	uint16 GetParam(ROWINDEX row) const;
	BYTE GetCommand(ROWINDEX row) const;
	void SetParamFromY(ROWINDEX row, int y);
	void SetCommand(ROWINDEX row, BYTE cmd);
	ModCommand::PARAM ScreenYToFXParam(int y) const;
	uint16 ScreenYToPCParam(int y) const;
	ROWINDEX ScreenXToRow(int x) const;
	bool IsPcNote(ROWINDEX row) const;
	void SetPcNote(ROWINDEX row);

	CSoundFile *m_pSndFile;
	CModDoc *m_pModDoc;
	CRect m_rcDraw;
	CRect m_rcFullWin;

	CComboBox m_cmbEffectList, m_cmbActionList;
	CEdit m_edVisStatus;

	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnClose();

	CViewPattern *m_pViewPattern;


	DECLARE_MESSAGE_MAP()
	BOOL OnInitDialog();
	afx_msg void OnPaint();

public:

	void UpdateSelection(ROWINDEX startRow, ROWINDEX endRow, CHANNELINDEX nchn, CModDoc *pModDoc, PATTERNINDEX pat);
	void Update();
	BOOL OpenEditor(CWnd *parent);
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
