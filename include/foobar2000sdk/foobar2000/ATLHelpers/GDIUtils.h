static HBITMAP CreateDIB24(CSize size) {
	struct {
		BITMAPINFOHEADER bmi;
	} bi = {};
	bi.bmi.biSize = sizeof(bi.bmi);
	bi.bmi.biWidth = size.cx;
	bi.bmi.biHeight = size.cy;
	bi.bmi.biPlanes = 1;
	bi.bmi.biBitCount = 24;
	bi.bmi.biCompression = BI_RGB;
	void * bitsPtr;
	return CreateDIBSection(NULL, reinterpret_cast<const BITMAPINFO*>(&bi), DIB_RGB_COLORS,&bitsPtr,0,0);
}

static HBITMAP CreateDIB16(CSize size) {
	struct {
		BITMAPINFOHEADER bmi;
	} bi = {};
	bi.bmi.biSize = sizeof(bi.bmi);
	bi.bmi.biWidth = size.cx;
	bi.bmi.biHeight = size.cy;
	bi.bmi.biPlanes = 1;
	bi.bmi.biBitCount = 16;
	bi.bmi.biCompression = BI_RGB;
	void * bitsPtr;
	return CreateDIBSection(NULL, reinterpret_cast<const BITMAPINFO*>(&bi), DIB_RGB_COLORS,&bitsPtr,0,0);
}

static HBITMAP CreateDIB8(CSize size, const COLORREF palette[256]) {
	struct {
		BITMAPINFOHEADER bmi;
		COLORREF colors[256];
	} bi = { };
	for(int c = 0; c < 256; ++c ) bi.colors[c] = palette[c];
	bi.bmi.biSize = sizeof(bi.bmi);
	bi.bmi.biWidth = size.cx;
	bi.bmi.biHeight = size.cy;
	bi.bmi.biPlanes = 1;
	bi.bmi.biBitCount = 8;
	bi.bmi.biCompression = BI_RGB;
	bi.bmi.biClrUsed = 256;
	void * bitsPtr;
	return CreateDIBSection(NULL, reinterpret_cast<const BITMAPINFO*>(&bi), DIB_RGB_COLORS,&bitsPtr,0,0);
}

static void CreateScaledFont(CFont & out, CFontHandle in, double scale) {
	LOGFONT lf;
	WIN32_OP_D( in.GetLogFont(lf) );
	int temp = pfc::rint32(scale * lf.lfHeight);
	if (temp == 0) temp = pfc::sgn_t(lf.lfHeight);
	lf.lfHeight = temp;
	WIN32_OP_D( out.CreateFontIndirect(&lf) != NULL );
}

static void CreateScaledFontEx(CFont & out, CFontHandle in, double scale, int weight) {
	LOGFONT lf;
	WIN32_OP_D( in.GetLogFont(lf) );
	int temp = pfc::rint32(scale * lf.lfHeight);
	if (temp == 0) temp = pfc::sgn_t(lf.lfHeight);
	lf.lfHeight = temp;
	lf.lfWeight = weight;
	WIN32_OP_D( out.CreateFontIndirect(&lf) != NULL );
}

static void CreatePreferencesHeaderFont(CFont & out, CWindow source) {
	CreateScaledFontEx(out, source.GetFont(), 1.3, FW_BOLD);
}

static void CreatePreferencesHeaderFont2(CFont & out, CWindow source) {
	CreateScaledFontEx(out, source.GetFont(), 1.1, FW_BOLD);
}

template<typename TCtrl>
class CAltFontCtrl : public TCtrl {
public:
	void Initialize(CWindow wnd, double scale, int weight) {
		CreateScaledFontEx(m_font, wnd.GetFont(), scale, weight);
		_initWnd(wnd);
	}
	void MakeHeader(CWindow wnd) {
		CreatePreferencesHeaderFont(m_font, wnd);
		_initWnd(wnd);
	}
	void MakeHeader2(CWindow wnd) {
		CreatePreferencesHeaderFont2(m_font, wnd);
		_initWnd(wnd);
	}
private:
	void _initWnd(CWindow wnd) {
		SubclassWindow(wnd); SetFont(m_font);
	}
	CFont m_font;
};

class CFontScaled : public CFont {
public:
	CFontScaled(HFONT _in, double scale) {
		CreateScaledFont(*this, _in, scale);
	}
};

class DCClipRgnScope {
public:
	DCClipRgnScope(HDC dc) : m_dc(dc) {
		m_dc.GetClipRgn(m_rgn);
	}
	~DCClipRgnScope() {
		m_dc.SelectClipRgn(m_rgn);
	}

	HRGN OldVal() const throw() {return m_rgn;}

	PFC_CLASS_NOT_COPYABLE_EX(DCClipRgnScope)
private:
	CDCHandle m_dc;
	CRgn m_rgn;
};


static HBRUSH MakeTempBrush(HDC dc, COLORREF color) throw() {
	SetDCBrushColor(dc, color); return (HBRUSH) GetStockObject(DC_BRUSH);
}

class CDCBrush : public CBrushHandle {
public:
	CDCBrush(HDC dc, COLORREF color) throw() {
		m_dc = dc;
		m_oldCol = m_dc.SetDCBrushColor(color);
		m_hBrush = (HBRUSH) GetStockObject(DC_BRUSH);
	}
	~CDCBrush() throw() {
		m_dc.SetDCBrushColor(m_oldCol);
	}
	PFC_CLASS_NOT_COPYABLE_EX(CDCBrush)
private:
	CDCHandle m_dc;
	COLORREF m_oldCol;
};

class CDCPen : public CPenHandle {
public:
	CDCPen(HDC dc, COLORREF color) throw() {
		m_dc = dc;
		m_oldCol = m_dc.SetDCPenColor(color);
		m_hPen = (HPEN) GetStockObject(DC_PEN);
	}
	~CDCPen() throw() {
		m_dc.SetDCPenColor(m_oldCol);
	}
private:
	CDCHandle m_dc;
	COLORREF m_oldCol;
};


class CBackBuffer : public CDC {
public:
	CBackBuffer() : m_bitmapOld(NULL), m_curSize(0,0) {
		CreateCompatibleDC(NULL);
		ATLASSERT(m_hDC != NULL);
	}
	~CBackBuffer() {
		Dispose();
	}
	void Attach(HBITMAP bmp, CSize size) {
		Dispose();
		m_bitmap.Attach(bmp); m_curSize = size;
		m_bitmapOld = SelectBitmap(m_bitmap);
	}
	void Attach(HBITMAP bmp) {
		CSize size; 
		bool state = CBitmapHandle(bmp).GetSize(size);
		ATLASSERT(state);
		Attach(bmp, size);
	}
	BOOL Allocate(CSize size, HDC dcCompatible = NULL) {
		if (m_hDC == NULL) return FALSE;
		if (m_curSize == size) return TRUE;
		Dispose();
		HBITMAP temp;
		if (dcCompatible == NULL) {
			temp = CreateDIB24(size);
		} else {
			temp = CreateCompatibleBitmap(dcCompatible, size.cx, size.cy);
		}
		if (temp == NULL) return FALSE;
		Attach(temp);
		return TRUE;
	}

	void Dispose() {
		if (m_bitmap != NULL) {
			SelectBitmap(m_bitmapOld); m_bitmapOld = NULL;
			m_bitmap.DeleteObject();
		}
		m_curSize = CSize(0,0);
	}
	BOOL GetBitmapPtr(t_uint8 * & ptr, t_ssize & lineWidth) {
		if (m_bitmap == NULL) return FALSE;
		BITMAP bmp = {};
		if (!m_bitmap.GetBitmap(bmp)) return FALSE;
		lineWidth = bmp.bmWidthBytes;
		ptr = reinterpret_cast<t_uint8*>(bmp.bmBits);
		return TRUE;
	}
	CSize GetSize() const { return m_curSize; }

	PFC_CLASS_NOT_COPYABLE_EX(CBackBuffer)
private:
	CSize m_curSize;
	CBitmap m_bitmap;
	HBITMAP m_bitmapOld;
};

class CBackBufferScope : public CDCHandle {
public:
	CBackBufferScope(HDC hDC, HDC hDCBB, const CRect & rcPaint) : CDCHandle(hDCBB), m_dcOrig(hDC), m_rcPaint(rcPaint)
	{
		GetClipRgn(m_clipRgnOld); 
		CRgn temp;
		if (m_dcOrig.GetClipRgn(temp) == 1) {
			if (m_clipRgnOld != NULL) temp.CombineRgn(m_clipRgnOld,RGN_AND);
			SelectClipRgn(temp);
		}
		IntersectClipRect(rcPaint);
	}

	~CBackBufferScope()
	{
		m_dcOrig.BitBlt(m_rcPaint.left,m_rcPaint.top,m_rcPaint.Width(),m_rcPaint.Height(),m_hDC,m_rcPaint.left,m_rcPaint.top,SRCCOPY);
		SelectClipRgn(m_clipRgnOld);
	}
private:
	const CRect m_rcPaint;
	CDCHandle m_dcOrig;
	CRgn m_clipRgnOld;
};
