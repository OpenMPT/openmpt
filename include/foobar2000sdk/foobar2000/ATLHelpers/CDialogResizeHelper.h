class CDialogResizeHelper {
public:
	typedef dialog_resize_helper::param ParamOld;

	struct Param {
		t_uint32 id;
		float snapLeft, snapTop, snapRight, snapBottom;
	};
private:
	void AddSizeGrip();
public:
	inline void set_min_size(unsigned x,unsigned y) {min_x = x; min_y = y;}
	inline void set_max_size(unsigned x,unsigned y) {max_x = x; max_y = y;}
	
	
	BEGIN_MSG_MAP_EX(CDialogResizeHelper)
		if (uMsg == WM_INITDIALOG) OnInitDialog(hWnd);
		MSG_WM_SIZE(OnSize)
		MSG_WM_DESTROY(OnDestroy)
		MSG_WM_GETMINMAXINFO(OnGetMinMaxInfo)
	END_MSG_MAP()

	template<typename TParam, t_size paramCount> CDialogResizeHelper(const TParam (& src)[paramCount],CRect const& minMaxRange = CRect(0,0,0,0)) {
		InitTable(src, paramCount);
		InitMinMax(minMaxRange);
	}

	void InitTable(const Param* table, t_size tableSize);
	void InitTable(const ParamOld * table, t_size tableSize);
	void InitMinMax(const CRect & range);
	
	bool EvalRect(UINT id, CRect & out) const;

private:
	bool _EvalRect(t_size index, CSize wndSize, CRect & out) const;
	void OnGetMinMaxInfo(LPMINMAXINFO lpMMI) const;
	void OnSize(UINT nType, CSize size);
	void OnInitDialog(CWindow thisWnd);
	void OnDestroy();

	pfc::array_t<Param> m_table;
	pfc::map_t<UINT, CRect> m_origRects;
	CRect m_rcOrigClient;
	CWindow m_thisWnd, m_sizeGrip;
	unsigned min_x,min_y,max_x,max_y;
};

template<typename TTracker> class CDialogResizeHelperTracking : public CDialogResizeHelper {
public:
	template<typename TParam, t_size paramCount, typename TCfgVar> CDialogResizeHelperTracking(const TParam (& src)[paramCount],CRect const& minMaxRange, TCfgVar & cfgVar) : CDialogResizeHelper(src, minMaxRange), m_tracker(cfgVar) {}
	
	BEGIN_MSG_MAP_EX(CDialogResizeHelperST)
		CHAIN_MSG_MAP(CDialogResizeHelper)
		CHAIN_MSG_MAP_MEMBER(m_tracker)
	END_MSG_MAP()

private:
	TTracker m_tracker;
};

typedef CDialogResizeHelperTracking<cfgDialogSizeTracker> CDialogResizeHelperST;
typedef CDialogResizeHelperTracking<cfgDialogPositionTracker> CDialogResizeHelperPT;

#define REDRAW_DIALOG_ON_RESIZE() if (uMsg == WM_SIZE) RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
