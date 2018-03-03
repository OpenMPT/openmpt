#pragma once
#include <GdiPlus.h>

#include "win32_op.h"

class GdiplusErrorHandler {
public:
	void operator<<(Gdiplus::Status p_code) {
		if (p_code != Gdiplus::Ok) {
			throw pfc::exception(pfc::string_formatter() << "Gdiplus error (" << (unsigned) p_code << ")");
		}
	}
};

class GdiplusScope {
public:
	GdiplusScope() {
		Gdiplus::GdiplusStartupInput input;
		Gdiplus::GdiplusStartupOutput output;
		GdiplusErrorHandler() << Gdiplus::GdiplusStartup(&m_token,&input,&output);
	}
	~GdiplusScope() {
		Gdiplus::GdiplusShutdown(m_token);
	}

	PFC_CLASS_NOT_COPYABLE_EX(GdiplusScope);
private:
	ULONG_PTR m_token;
};

static HBITMAP GdiplusLoadBitmap(UINT id, const TCHAR * resType, CSize size) {
	using namespace Gdiplus;
	try {

	
		pfc::ptrholder_t<uResource> resource = LoadResourceEx(core_api::get_my_instance(),MAKEINTRESOURCE(id),resType);
		if (resource.is_empty()) throw pfc::exception_bug_check();

		pfc::com_ptr_t<IStream> stream; stream.attach( SHCreateMemStream((const BYTE*)resource->GetPointer(), resource->GetSize()) );
		if ( stream.is_empty() ) throw std::bad_alloc();
				
		GdiplusErrorHandler EH;
		pfc::ptrholder_t<Image> source = new Image(stream.get_ptr());
		
		EH << source->GetLastStatus();
		pfc::ptrholder_t<Bitmap> resized = new Bitmap(size.cx, size.cy, PixelFormat32bppARGB);
		EH << resized->GetLastStatus();
		
		{
			pfc::ptrholder_t<Graphics> target = new Graphics(resized.get_ptr());
			EH << target->GetLastStatus();
			EH << target->SetInterpolationMode(InterpolationModeHighQuality);
			EH << target->Clear(Color(0,0,0,0));
			EH << target->DrawImage(source.get_ptr(), Rect(0,0,size.cx,size.cy));
		}

		HBITMAP bmp = NULL;
		EH << resized->GetHBITMAP(Gdiplus::Color::White, & bmp );
		return bmp;
	} catch(...) {
		PFC_ASSERT( !"Should not get here");
		return NULL;
	}
}

static HICON GdiplusLoadIcon(UINT id, const TCHAR * resType, CSize size) {
	using namespace Gdiplus;
	try {

	
		pfc::ptrholder_t<uResource> resource = LoadResourceEx(core_api::get_my_instance(),MAKEINTRESOURCE(id),resType);
		if (resource.is_empty()) throw pfc::exception_bug_check();

		pfc::com_ptr_t<IStream> stream; stream.attach( SHCreateMemStream((const BYTE*)resource->GetPointer(), resource->GetSize()) );
		if (stream.is_empty()) throw std::bad_alloc();

		GdiplusErrorHandler EH;
		pfc::ptrholder_t<Image> source = new Image(stream.get_ptr());
		
		EH << source->GetLastStatus();
		pfc::ptrholder_t<Bitmap> resized = new Bitmap(size.cx, size.cy, PixelFormat32bppARGB);
		EH << resized->GetLastStatus();
		
		{
			pfc::ptrholder_t<Graphics> target = new Graphics(resized.get_ptr());
			EH << target->GetLastStatus();
			EH << target->SetInterpolationMode(InterpolationModeHighQuality);
			EH << target->Clear(Color(0,0,0,0));
			EH << target->DrawImage(source.get_ptr(), Rect(0,0,size.cx,size.cy));
		}

		HICON icon = NULL;
		EH << resized->GetHICON(&icon);
		return icon;
	} catch(...) {
		PFC_ASSERT( !"Should not get here");
		return NULL;
	}
}
static HICON GdiplusLoadPNGIcon(UINT id, CSize size) {return GdiplusLoadIcon(id, _T("PNG"), size);}

static HICON LoadPNGIcon(UINT id, CSize size) {
	GdiplusScope scope;
	return GdiplusLoadPNGIcon(id, size);
}

static void GdiplusLoadButtonPNG(CIcon & icon, HWND btn_, UINT image) {
	CButton btn(btn_);
	if (icon == NULL) {
		CRect client; WIN32_OP_D( btn.GetClientRect(client) );
		CSize size = client.Size();
		int v = MulDiv(pfc::min_t<int>(size.cx, size.cy), 3, 4);
		if (v < 8) v = 8;
		icon = GdiplusLoadPNGIcon(image, CSize(v,v));
	}
	btn.SetIcon(icon);
}

#pragma comment(lib, "gdiplus.lib")
