#pragma once
#include "afxwin.h"
class CViewPattern;
//class CModScrollView;

#define FXVSTATUS_LDRAGGING		0x01
#define FXVSTATUS_RDRAGGING		0x02
//#define FXVSTATUS_NCLBTNDOWN	0x02
//#define INSSTATUS_SPLITCURSOR	0x04

// EffectVis dialog
class CEffectVis : public CModControlDlg
{
	DECLARE_DYNAMIC(CEffectVis)

public:
	CEffectVis(CViewPattern *pViewPattern, UINT startRow, UINT endRow, UINT nchn, CModDoc* pModDoc, UINT pat);
	virtual ~CEffectVis();
	//{{AFX_VIRTUAL(CEffectVis)
	//virtual void OnDraw(CDC *);
	//}}AFX_VIRTUAL

// Dialog Data
	enum { IDD = IDD_EFFECTVISUALIZER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//CFastBitmap m_Dib;

	CBitmap m_bGrid, m_bNodes, m_bPlayPos;
	CBitmap *m_pbOldGrid, *m_pbOldNodes, *m_pbOldPlayPos;
	CDC m_dcGrid, m_dcNodes, m_dcPlayPos;
	//LPMODPLUGDIB nodeBMP;
	void DrawNodes();
	void DrawGrid();

	void ShowVis(CDC * pDC, CRect rectBorder);
	void ShowVisImage(CDC *pDC);
	BOOL m_boolForceRedraw, m_boolUseBitmaps;
	RECT invalidated;
	
	int m_nLastDrawnRow; // for interpolation
	int m_nRowToErase;
	int m_nParamToErase;

	UINT m_nOldPlayPos;


//	int m_nRectWidth, m_nRectHeight;
//	CRect m_rectDraw;
//	CRect m_rectOwner;
	CBrush m_brushBlack;

public:
	UINT m_startRow;
    UINT m_endRow;
	UINT m_nRows;
	UINT m_nChan;
	UINT m_nPattern;
	long m_nFillEffect;

    int m_nDragItem;
	UINT m_nBtnMouseOver;
	DWORD m_dwStatus;

	void InvalidateRow(int row);
	float m_pixelsPerRow, m_pixelsPerParam;
	void UpdateSelection(UINT startRow, UINT endRow, UINT nchn, CModDoc* m_pModDoc, UINT pats);
	void Update();
	int RowToScreenX(UINT row);
	int ParamToScreenY(BYTE param);
	BYTE GetParam(UINT row);
	BYTE GetCommand(UINT row);
	void SetParam(UINT row, BYTE param);
	void SetCommand(UINT row, BYTE cmd);
	BYTE ScreenYToParam(int y);
	UINT ScreenXToRow(int x);
	void SetPlayCursor(UINT nPat, UINT nRow);

	CSoundFile* m_pSndFile;
	CModDoc* m_pModDoc;
	CRect m_rcDraw;
	CRect m_rcFullWin;

	CComboBox m_cmbEffectList;
	CButton m_btnFillCheck;
	bool m_bFillCheck;
	CEdit m_edVisStatus;

	virtual VOID OnOK();
	virtual VOID OnCancel();
	BOOL OpenEditor(CWnd *parent);
	VOID DoClose();
	afx_msg void OnClose();
	LONG* GetSplitPosRef() {return NULL;} 	//rewbs.varWindowSize

	CViewPattern *m_pViewPattern;
	

	DECLARE_MESSAGE_MAP()
	BOOL OnInitDialog();
	afx_msg void OnPaint();
	
public: //HACK for first window repos
	afx_msg void OnSize(UINT nType, int cx, int cy);
protected:
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnFillBlanksCheck();
	afx_msg void OnEffectChanged();
//	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
//	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
//	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
//	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	//{{AFX_MSG(CEffectVis)
	afx_msg void OnEditUndo();
	//}}AFX_MSG

private:

	void MakeChange(int currentRow, int newParam);
};
