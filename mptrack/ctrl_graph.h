#pragma once
class CCtrlGraph :
	public CModControlDlg
{
public:
	CCtrlGraph(void);
	~CCtrlGraph(void);

	LONG* GetSplitPosRef() {return &CMainFrame::GetSettings().glGraphWindowHeight;} 	//rewbs.varWindowSize
	virtual CRuntimeClass *GetAssociatedViewClass();
};
