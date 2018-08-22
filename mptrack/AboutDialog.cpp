/*
 * AboutDialog.cpp
 * ---------------
 * Purpose: About dialog with credits, system information and a fancy demo effect.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "resource.h"
#include "AboutDialog.h"
#include "Image.h"
#include "Mptrack.h"
#include "TrackerSettings.h"
#include "BuildVariants.h"
#include "../common/version.h"
#include "../common/mptWine.h"


OPENMPT_NAMESPACE_BEGIN


CAboutDlg *CAboutDlg::instance = nullptr;

BEGIN_MESSAGE_MAP(CRippleBitmap, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEHOVER()
	ON_WM_MOUSELEAVE()
END_MESSAGE_MAP()


CRippleBitmap::CRippleBitmap()
{
	m_bitmapSrc = LoadPixelImage(GetResource(MAKEINTRESOURCE(IDB_MPTRACK), _T("PNG")));
	m_bitmapTarget = mpt::make_unique<RawGDIDIB>(m_bitmapSrc->Width(), m_bitmapSrc->Height());
	m_offset1.assign(m_bitmapSrc->Pixels().size(), 0);
	m_offset2.assign(m_bitmapSrc->Pixels().size(), 0);
	m_frontBuf = m_offset2.data();
	m_backBuf = m_offset1.data();

	// Pre-fill first and last row of output bitmap, since those won't be touched.
	const RawGDIDIB::Pixel *in1 = m_bitmapSrc->Pixels().data(), *in2 = m_bitmapSrc->Pixels().data() + (m_bitmapSrc->Height() - 1) * m_bitmapSrc->Width();
	RawGDIDIB::Pixel *out1 = m_bitmapTarget->Pixels().data(), *out2 = m_bitmapTarget->Pixels().data() + (m_bitmapSrc->Height() - 1) * m_bitmapSrc->Width();
	for(uint32 i = 0; i < m_bitmapSrc->Width(); i++)
	{
		*(out1++) = *(in1++);
		*(out2++) = *(in2++);
	}

	MemsetZero(m_bi);
	m_bi.biSize = sizeof(BITMAPINFOHEADER);
	m_bi.biWidth = m_bitmapSrc->Width();
	m_bi.biHeight = -(int32)m_bitmapSrc->Height();
	m_bi.biPlanes = 1;
	m_bi.biBitCount = 32;
	m_bi.biCompression = BI_RGB;
	m_bi.biSizeImage = m_bitmapSrc->Width() * m_bitmapSrc->Height() * 4;
}


CRippleBitmap::~CRippleBitmap()
{
	if(!m_showMouse)
	{
		ShowCursor(TRUE);
	}
}


void CRippleBitmap::OnMouseMove(UINT nFlags, CPoint point)
{

	// Rate limit in order to avoid too may ripples.
	DWORD now = timeGetTime();
	if(now - m_lastRipple < UPDATE_INTERVAL)
		return;
	m_lastRipple = now;

	// Initiate ripples at cursor location
	point.x = Util::ScalePixelsInv(point.x, m_hWnd);
	point.y = Util::ScalePixelsInv(point.y, m_hWnd);
	Limit(point.x, 1, int(m_bitmapSrc->Width()) - 2);
	Limit(point.y, 2, int(m_bitmapSrc->Height()) - 3);
	int32 *p = m_backBuf + point.x + point.y * m_bitmapSrc->Width();
	p[0] += (nFlags & MK_LBUTTON) ? 50 : 150;
	p[0] += (nFlags & MK_MBUTTON) ? 150 : 0;

	int32 w = m_bitmapSrc->Width();
	// Make the initial point of this ripple a bit "fatter".
	p[-1]     += p[0] / 2; p[1]      += p[0] / 2;
	p[-w]     += p[0] / 2; p[w]      += p[0] / 2;
	p[-w - 1] += p[0] / 4; p[-w + 1] += p[0] / 4;
	p[w - 1]  += p[0] / 4; p[w + 1]  += p[0] / 4;

	m_damp = !(nFlags & MK_RBUTTON);
	m_activity = true;

	// Wine will only ever generate MouseLeave message when the message
	// queue is completely empty and the hover timeout has expired.
	// This results in a hidden mouse cursor long after it had already left the
	// control.
	// Avoid hiding the mouse cursor on Wine. Interferring with the users input
	// methods is an absolute no-go.
	if(mpt::Windows::IsWine())
	{
		return;
	}

	TRACKMOUSEEVENT me;
	me.cbSize = sizeof(TRACKMOUSEEVENT);
	me.hwndTrack = m_hWnd;
	me.dwFlags = TME_LEAVE | TME_HOVER;
	me.dwHoverTime = 1500;

	if(TrackMouseEvent(&me) && m_showMouse)
	{
		ShowCursor(FALSE);
		m_showMouse = false;
	}
}


void CRippleBitmap::OnMouseLeave()
{
	if(!m_showMouse)
	{
		ShowCursor(TRUE);
		m_showMouse = true;
	}
}


void CRippleBitmap::OnPaint()
{
	CPaintDC dc(this);

	CRect rect;
	GetClientRect(rect);
	StretchDIBits(dc.m_hDC,
		0, 0, rect.Width(), rect.Height(),
		0, 0, m_bitmapTarget->Width(), m_bitmapTarget->Height(),
		m_bitmapTarget->Pixels().data(),
		reinterpret_cast<BITMAPINFO *>(&m_bi), DIB_RGB_COLORS, SRCCOPY);
}


bool CRippleBitmap::Animate()
{
	// Were there any pixels being moved in the last frame?
	if(!m_activity)
		return false;

	DWORD now = timeGetTime();
	if(now - m_lastFrame < UPDATE_INTERVAL)
		return true;
	m_lastFrame = now;
	m_activity = false;

	m_frontBuf = (m_frame ? m_offset2 : m_offset1).data();
	m_backBuf = (m_frame ? m_offset1 : m_offset2).data();

	// Spread the ripples...
	const int32 w = m_bitmapSrc->Width(), h = m_bitmapSrc->Height();
	const int32 numPixels = w * (h - 2);
	const int32 *back = m_backBuf + w;
	int32 *front = m_frontBuf + w;
	for(int32 i = numPixels; i != 0; i--, back++, front++)
	{
		(*front) = (back[-1] + back[1] + back[w] + back[-w]) / 2 - (*front);
		if(m_damp) (*front) -= (*front) >> 5;
	}

	// ...and compute the final picture.
	const int32 *offset = m_frontBuf + w;
	const RawGDIDIB::Pixel *pixelIn = m_bitmapSrc->Pixels().data() + w;
	RawGDIDIB::Pixel *pixelOut = m_bitmapTarget->Pixels().data() + w;
	RawGDIDIB::Pixel *limitMin = m_bitmapSrc->Pixels().data(), *limitMax = m_bitmapSrc->Pixels().data() + m_bitmapSrc->Pixels().size() - 1;
	for(int32 i = numPixels; i != 0; i--, pixelIn++, pixelOut++, offset++)
	{
		// Compute pixel displacement
		const int32 xOff = offset[-1] - offset[1];
		const int32 yOff = offset[-w] - offset[w];

		if(xOff | yOff)
		{
			const RawGDIDIB::Pixel *p = pixelIn + xOff + yOff * w;
			Limit(p, limitMin, limitMax);
			// Add a bit of shading depending on how far we're displacing the pixel...
			pixelOut->r = mpt::saturate_cast<uint8>(p->r + (p->r * xOff) / 32);
			pixelOut->g = mpt::saturate_cast<uint8>(p->g + (p->g * xOff) / 32);
			pixelOut->b = mpt::saturate_cast<uint8>(p->b + (p->b * xOff) / 32);
			// ...and mix it with original picture
			pixelOut->r = (pixelOut->r + pixelIn->r) / 2u;
			pixelOut->g = (pixelOut->g + pixelIn->g) / 2u;
			pixelOut->b = (pixelOut->b + pixelIn->b) / 2u;
			// And now some cheap image smoothing...
			pixelOut[-1].r = (pixelOut->r + pixelOut[-1].r) / 2u;
			pixelOut[-1].g = (pixelOut->g + pixelOut[-1].g) / 2u;
			pixelOut[-1].b = (pixelOut->b + pixelOut[-1].b) / 2u;
			pixelOut[-w].r = (pixelOut->r + pixelOut[-w].r) / 2u;
			pixelOut[-w].g = (pixelOut->g + pixelOut[-w].g) / 2u;
			pixelOut[-w].b = (pixelOut->b + pixelOut[-w].b) / 2u;
			m_activity = true;	// Also use this to update activity status...
		} else
		{
			*pixelOut = *pixelIn;
		}
	}

	m_frame = !m_frame;

	InvalidateRect(NULL, FALSE);

	return true;
}


CAboutDlg::~CAboutDlg()
{
	instance = nullptr;
}


void CAboutDlg::OnOK()
{
	instance = nullptr;
	if(m_TimerID != 0)
	{
		KillTimer(m_TimerID);
		m_TimerID = 0;
	}
	DestroyWindow();
	delete this;
}


void CAboutDlg::OnCancel()
{
	OnOK();
}


BOOL CAboutDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	mpt::ustring app;
	app += mpt::format(MPT_USTRING("OpenMPT %1 (%2 bit)"))(mpt::Windows::Name(mpt::Windows::GetProcessArchitecture()), mpt::arch_bits)
		+ (!BuildVariants().CurrentBuildIsModern() ? MPT_USTRING(" for older Windows") : MPT_USTRING(""))
		+ MPT_USTRING("\n");
	app += MPT_USTRING("Version ") + Build::GetVersionStringSimple() + MPT_USTRING("\n\n");
	app += Build::GetURL(Build::Url::Website) + MPT_USTRING("\n");
	SetDlgItemText(IDC_EDIT3, mpt::ToCString(mpt::String::Replace(app, MPT_USTRING("\n"), MPT_USTRING("\r\n"))));

	m_bmp.SubclassDlgItem(IDC_BITMAP1, this);

	m_Tab.InsertItem(TCIF_TEXT, 0, _T("OpenMPT"), 0, 0, 0, 0);
	m_Tab.InsertItem(TCIF_TEXT, 1, _T("Components"), 0, 0, 0, 0);
	m_Tab.InsertItem(TCIF_TEXT, 2, _T("Credits"), 0, 0, 0, 0);
	m_Tab.InsertItem(TCIF_TEXT, 3, _T("License"), 0, 0, 0, 0);
	m_Tab.InsertItem(TCIF_TEXT, 4, _T("Resources"), 0, 0, 0, 0);
	if(mpt::Windows::IsWine()) m_Tab.InsertItem(TCIF_TEXT, 5, _T("Wine"), 0, 0, 0, 0);
	m_Tab.SetCurSel(0);

	OnTabChange(nullptr, nullptr);

	if(m_TimerID != 0)
	{
		KillTimer(m_TimerID);
		m_TimerID = 0;
	}
	m_TimerID = SetTimer(TIMERID_ABOUT_DEFAULT, CRippleBitmap::UPDATE_INTERVAL, nullptr);

	return TRUE;
}


void CAboutDlg::OnTimer(UINT_PTR nIDEvent)
{
	if(nIDEvent == m_TimerID)
	{
		m_bmp.Animate();
	}
}


void CAboutDlg::OnTabChange(NMHDR * /*pNMHDR*/ , LRESULT * /*pResult*/ )
{
	m_TabEdit.SetWindowText(mpt::ToCString(mpt::String::Replace(GetTabText(m_Tab.GetCurSel()), MPT_USTRING("\n"), MPT_USTRING("\r\n"))));
}


static mpt::ustring ProcSupportToString(uint32 procSupport)
{
	std::vector<mpt::ustring> features;
#if MPT_COMPILER_MSVC && defined(ENABLE_ASM)
#if defined(ENABLE_X86)
	features.push_back(MPT_USTRING("x86"));
#endif
#if defined(ENABLE_X64)
	features.push_back(MPT_USTRING("x86-64"));
#endif
	struct ProcFlag
	{
		decltype(procSupport) flag;
		const char *name;
	};
	static constexpr ProcFlag flags[] =
	{
		{ 0, "" },
#if defined(ENABLE_X86)
		{ PROCSUPPORT_CMOV, "cmov" },
#endif
#if defined(ENABLE_MMX)
		{ PROCSUPPORT_MMX, "mmx" },
#endif
#if defined(ENABLE_SSE)
		{ PROCSUPPORT_SSE, "sse" },
#endif
#if defined(ENABLE_SSE2)
		{ PROCSUPPORT_SSE2, "sse2" },
#endif
#if defined(ENABLE_SSE3)
		{ PROCSUPPORT_SSE3, "sse3" },
		{ PROCSUPPORT_SSSE3, "ssse3" },
#endif
#if defined(ENABLE_SSE4)
		{ PROCSUPPORT_SSE4_1, "sse4.1" },
		{ PROCSUPPORT_SSE4_2, "sse4.2" },
#endif
#if defined(ENABLE_X86_AMD)
		{ PROCSUPPORT_AMD_MMXEXT, "mmxext" },
		{ PROCSUPPORT_AMD_3DNOW, "3dnow" },
		{ PROCSUPPORT_AMD_3DNOWEXT, "3dnowext" },
#endif
	};
	for(const auto &f : flags)
	{
		if(procSupport & f.flag) features.push_back(mpt::ToUnicode(mpt::CharsetASCII, f.name));
	}
#endif
	return mpt::String::Combine(features, MPT_USTRING(" "));
}


mpt::ustring CAboutDlg::GetTabText(int tab)
{
	const mpt::ustring lf = MPT_USTRING("\n");
	const mpt::ustring yes = MPT_USTRING("yes");
	const mpt::ustring no = MPT_USTRING("no");
	mpt::ustring text;
	switch(tab)
	{
		case 0:
			text = MPT_USTRING("OpenMPT - Open ModPlug Tracker\n\n")
				+ mpt::format(MPT_USTRING("Version: %1\n"))(Build::GetVersionStringExtended())
				+ mpt::format(MPT_USTRING("Source Code: %1\n"))(SourceInfo::Current().GetUrlWithRevision() + MPT_ULITERAL(" ") + SourceInfo::Current().GetStateString())
				+ mpt::format(MPT_USTRING("Build Date: %1\n"))(Build::GetBuildDateString())
				+ mpt::format(MPT_USTRING("Compiler: %1\n"))(Build::GetBuildCompilerString())
				+ mpt::format(MPT_USTRING("Architecture: %1\n"))(mpt::Windows::Name(mpt::Windows::GetProcessArchitecture()))
				+ mpt::format(MPT_USTRING("Required Windows Kernel Level: %1\n"))(mpt::Windows::Version::VersionToString(mpt::Windows::Version::GetMinimumKernelLevel()))
				+ mpt::format(MPT_USTRING("Required Windows API Level: %1\n"))(mpt::Windows::Version::VersionToString(mpt::Windows::Version::GetMinimumAPILevel()));
			{
				text += MPT_USTRING("Required CPU features: ");
				std::vector<mpt::ustring> features;
				#if MPT_COMPILER_MSVC
					#if defined(_M_X64)
						features.push_back(MPT_USTRING("x86-64"));
						if(GetMinimumAVXVersion() >= 1) features.push_back(MPT_USTRING("avx"));
						if(GetMinimumAVXVersion() >= 2) features.push_back(MPT_USTRING("avx2"));
					#elif defined(_M_IX86)
						if(GetMinimumSSEVersion() <= 0 && GetMinimumAVXVersion() <= 0 ) features.push_back(MPT_USTRING("fpu"));
						if(GetMinimumSSEVersion() >= 1) features.push_back(MPT_USTRING("cmov"));
						if(GetMinimumSSEVersion() >= 1) features.push_back(MPT_USTRING("sse"));
						if(GetMinimumSSEVersion() >= 2) features.push_back(MPT_USTRING("sse2"));
						if(GetMinimumAVXVersion() >= 1) features.push_back(MPT_USTRING("avx"));
						if(GetMinimumAVXVersion() >= 2) features.push_back(MPT_USTRING("avx2"));
					#else
						if(GetMinimumSSEVersion() <= 0 && GetMinimumAVXVersion() <= 0 ) features.push_back(MPT_USTRING("fpu"));
						if(GetMinimumSSEVersion() >= 1) features.push_back(MPT_USTRING("cmov"));
						if(GetMinimumSSEVersion() >= 1) features.push_back(MPT_USTRING("sse"));
						if(GetMinimumSSEVersion() >= 2) features.push_back(MPT_USTRING("sse2"));
						if(GetMinimumAVXVersion() >= 1) features.push_back(MPT_USTRING("avx"));
						if(GetMinimumAVXVersion() >= 2) features.push_back(MPT_USTRING("avx2"));
					#endif
				#endif
				text += mpt::String::Combine(features, MPT_USTRING(" "));
				text += lf;
			}
#ifdef ENABLE_ASM
			text += mpt::format(MPT_USTRING("Optional CPU features used: %1\n"))(ProcSupportToString(GetProcSupport()));
#endif // ENABLE_ASM
			text += lf;
			text += mpt::format(MPT_USTRING("System Architecture: %1\n"))(mpt::Windows::Name(mpt::Windows::GetHostArchitecture()));
#ifdef ENABLE_ASM
			text += mpt::format(MPT_USTRING("CPU: %1, Family %2, Model %3, Stepping %4\n"))
				( mpt::ToUnicode(mpt::CharsetASCII, (std::strlen(ProcVendorID) > 0) ? std::string(ProcVendorID) : std::string("Generic"))
				, ProcFamily
				, ProcModel
				, ProcStepping
				);
			text += mpt::format(MPT_USTRING("CPU Name: %1\n"))(mpt::ToUnicode(mpt::CharsetASCII, (std::strlen(ProcBrandID) > 0) ? std::string(ProcBrandID) : std::string("")));
			text += mpt::format(MPT_USTRING("Available CPU features: %1\n"))(ProcSupportToString(GetRealProcSupport()));
#endif // ENABLE_ASM
			text += mpt::format(MPT_USTRING("Operating System: %1\n\n"))(mpt::Windows::Version::Current().GetName());
			text += mpt::format(MPT_USTRING("OpenMPT Path%2: %1\n"))(theApp.GetAppDirPath(), theApp.IsPortableMode() ? MPT_USTRING(" (portable)") : MPT_USTRING(""));
			text += mpt::format(MPT_USTRING("Settings%2: %1\n"))(theApp.GetConfigFileName(), theApp.IsPortableMode() ? MPT_USTRING(" (portable)") : MPT_USTRING(""));
			break;
		case 1:
			{
			std::vector<std::string> components = ComponentManager::Instance()->GetRegisteredComponents();
			if(!TrackerSettings::Instance().ComponentsKeepLoaded)
				{
					text += MPT_USTRING("Components are loaded and unloaded as needed.\n\n");
					for(const auto &component : components)
					{
						ComponentInfo info = ComponentManager::Instance()->GetComponentInfo(component);
						mpt::ustring name = mpt::ToUnicode(mpt::CharsetASCII, (info.name.substr(0, 9) == "Component") ? info.name.substr(9) : info.name);
						if(!info.settingsKey.empty())
						{
							name = mpt::ToUnicode(mpt::CharsetASCII, info.settingsKey);
						}
						text += name + lf;
					}
				} else
				{
					for(int available = 1; available >= 0; --available)
					{
						if(available)
						{
							text += MPT_USTRING("Loaded Components:\n");
						} else
						{
							text += MPT_USTRING("\nUnloaded Components:\n");
						}
						for(const auto &component : components)
						{
							ComponentInfo info = ComponentManager::Instance()->GetComponentInfo(component);
							if(available  && info.state != ComponentStateAvailable) continue;
							if(!available && info.state == ComponentStateAvailable) continue;
							mpt::ustring name = mpt::ToUnicode(mpt::CharsetASCII, (info.name.substr(0, 9) == "Component") ? info.name.substr(9) : info.name);
							if(!info.settingsKey.empty())
							{
								name = mpt::ToUnicode(mpt::CharsetASCII, info.settingsKey);
							}
							text += mpt::format(MPT_USTRING("%1: %2"))
								( name
								, info.state == ComponentStateAvailable ? MPT_USTRING("ok") :
									info.state == ComponentStateUnavailable? MPT_USTRING("missing") :
									info.state == ComponentStateUnintialized ? MPT_USTRING("not loaded") :
									info.state == ComponentStateBlocked ? MPT_USTRING("blocked") :
									info.state == ComponentStateUnregistered ? MPT_USTRING("unregistered") :
									MPT_USTRING("unknown")
								);
							if(info.type != ComponentTypeUnknown)
							{
								text += mpt::format(MPT_USTRING(" (%1)"))
									( info.type == ComponentTypeBuiltin ? MPT_USTRING("builtin") :
										info.type == ComponentTypeSystem ? MPT_USTRING("system") :
										info.type == ComponentTypeSystemInstallable ? MPT_USTRING("system, optional") :
										info.type == ComponentTypeBundled ? MPT_USTRING("bundled") :
										info.type == ComponentTypeForeign ? MPT_USTRING("foreign") :
										MPT_USTRING("unknown")
									);
							}
							text += lf;
						}
					}
				}
			}
			break;
		case 2:
			text += Build::GetFullCreditsString();
			break;
		case 3:
			text += Build::GetLicenseString();
			break;
		case 4:
			text += MPT_USTRING("Website:\n") + Build::GetURL(Build::Url::Website);
			text += MPT_USTRING("\n\nForum:\n") + Build::GetURL(Build::Url::Forum);
			text += MPT_USTRING("\n\nBug Tracker:\n") + Build::GetURL(Build::Url::Bugtracker);
			text += MPT_USTRING("\n\nUpdates:\n") + Build::GetURL(Build::Url::Updates);
			break;
		case 5:
			try
			{
				if(!theApp.GetWine())
				{
					text += MPT_USTRING("Wine integration not available.\n");
				} else
				{

					mpt::Wine::Context & wine = *theApp.GetWine();
					
					text += mpt::format(MPT_USTRING("Windows: %1\n"))
						( mpt::Windows::Version::Current().IsWindows() ? yes : no
						);
					text += mpt::format(MPT_USTRING("Windows version: %1\n"))
						( 
						mpt::Windows::Version::Current().IsAtLeast(mpt::Windows::Version::Win81) ? MPT_USTRING("Windows 8.1") :
						mpt::Windows::Version::Current().IsAtLeast(mpt::Windows::Version::Win8) ? MPT_USTRING("Windows 8") :
						mpt::Windows::Version::Current().IsAtLeast(mpt::Windows::Version::Win7) ? MPT_USTRING("Windows 7") :
						mpt::Windows::Version::Current().IsAtLeast(mpt::Windows::Version::WinVista) ? MPT_USTRING("Windows Vista") :
						mpt::Windows::Version::Current().IsAtLeast(mpt::Windows::Version::WinXP) ? MPT_USTRING("Windows XP") :
						mpt::Windows::Version::Current().IsAtLeast(mpt::Windows::Version::Win2000) ? MPT_USTRING("Windows 2000") :
						mpt::Windows::Version::Current().IsAtLeast(mpt::Windows::Version::WinNT4) ? MPT_USTRING("Windows NT4") :
						MPT_USTRING("unknown")
						);
					text += mpt::format(MPT_USTRING("Windows original: %1\n"))
						( mpt::Windows::IsOriginal() ? yes : no
						);

					text += MPT_USTRING("\n");

					text += mpt::format(MPT_USTRING("Wine: %1\n"))
						( mpt::Windows::IsWine() ? yes : no
						);
					text += mpt::format(MPT_USTRING("Wine Version: %1\n"))
						( mpt::ToUnicode(mpt::CharsetUTF8, wine.VersionContext().RawVersion())
						);
					text += mpt::format(MPT_USTRING("Wine Build ID: %1\n"))
						( mpt::ToUnicode(mpt::CharsetUTF8, wine.VersionContext().RawBuildID())
						);
					text += mpt::format(MPT_USTRING("Wine Host Sys Name: %1\n"))
						( mpt::ToUnicode(mpt::CharsetUTF8, wine.VersionContext().RawHostSysName())
						);
					text += mpt::format(MPT_USTRING("Wine Host Release: %1\n"))
						( mpt::ToUnicode(mpt::CharsetUTF8, wine.VersionContext().RawHostRelease())
						);

					text += MPT_USTRING("\n");

					text += mpt::format(MPT_USTRING("uname -m: %1\n"))
						( mpt::ToUnicode(mpt::CharsetUTF8, wine.Uname_m())
						);
					text += mpt::format(MPT_USTRING("HOME: %1\n"))
						( mpt::ToUnicode(mpt::CharsetUTF8, wine.HOME())
						);
					text += mpt::format(MPT_USTRING("XDG_DATA_HOME: %1\n"))
						( mpt::ToUnicode(mpt::CharsetUTF8, wine.XDG_DATA_HOME())
						);
					text += mpt::format(MPT_USTRING("XDG_CACHE_HOME: %1\n"))
						( mpt::ToUnicode(mpt::CharsetUTF8, wine.XDG_CACHE_HOME())
						);
					text += mpt::format(MPT_USTRING("XDG_CONFIG_HOME: %1\n"))
						( mpt::ToUnicode(mpt::CharsetUTF8, wine.XDG_CONFIG_HOME())
						);

					text += MPT_USTRING("\n");

					text += mpt::format(MPT_USTRING("OpenMPT folder: %1\n"))
						( theApp.GetAppDirPath().ToUnicode()
						);
					text += mpt::format(MPT_USTRING("OpenMPT folder (host): %1\n"))
						( mpt::ToUnicode(mpt::CharsetUTF8, wine.PathToPosix(theApp.GetAppDirPath()))
						);
					text += mpt::format(MPT_USTRING("OpenMPT config folder: %1\n"))
						( theApp.GetConfigPath().ToUnicode()
						);
					text += mpt::format(MPT_USTRING("OpenMPT config folder (host): %1\n"))
						( mpt::ToUnicode(mpt::CharsetUTF8, wine.PathToPosix(theApp.GetConfigPath()))
						);
					text += mpt::format(MPT_USTRING("Host root: %1\n"))
						( wine.PathToWindows("/").ToUnicode()
						);

				}
			} catch(const mpt::Wine::Exception & e)
			{
				text += MPT_USTRING("Exception: ") + mpt::get_exception_text<mpt::ustring>(e) + MPT_USTRING("\n");
			}
			break;
	}
	return text;
}


void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TABABOUT, m_Tab);
	DDX_Control(pDX, IDC_EDITABOUT, m_TabEdit);
}


BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	ON_WM_TIMER()
	ON_NOTIFY(TCN_SELCHANGE, IDC_TABABOUT, &CAboutDlg::OnTabChange)
END_MESSAGE_MAP()



OPENMPT_NAMESPACE_END
