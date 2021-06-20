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
#include "../misc/mptWine.h"


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
	m_bitmapTarget = std::make_unique<RawGDIDIB>(m_bitmapSrc->Width(), m_bitmapSrc->Height());
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
	if(mpt::OS::Windows::IsWine())
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
	app += MPT_UFORMAT("OpenMPT{} ({} ({} bit))")(
			BuildVariants().GetBuildVariantDescription(BuildVariants().GetBuildVariant()),
			mpt::OS::Windows::Name(mpt::OS::Windows::GetProcessArchitecture()),
			mpt::arch_bits)
		+ U_("\n");
	app += U_("Version ") + Build::GetVersionStringSimple() + U_("\n\n");
	app += Build::GetURL(Build::Url::Website) + U_("\n");
	SetDlgItemText(IDC_EDIT3, mpt::ToCString(mpt::String::Replace(app, U_("\n"), U_("\r\n"))));

	m_bmp.SubclassDlgItem(IDC_BITMAP1, this);

	m_Tab.InsertItem(TCIF_TEXT, 0, _T("OpenMPT"), 0, 0, 0, 0);
	m_Tab.InsertItem(TCIF_TEXT, 1, _T("Components"), 0, 0, 0, 0);
	m_Tab.InsertItem(TCIF_TEXT, 2, _T("Credits"), 0, 0, 0, 0);
	m_Tab.InsertItem(TCIF_TEXT, 3, _T("License"), 0, 0, 0, 0);
	m_Tab.InsertItem(TCIF_TEXT, 4, _T("Resources"), 0, 0, 0, 0);
	if(mpt::OS::Windows::IsWine()) m_Tab.InsertItem(TCIF_TEXT, 5, _T("Wine"), 0, 0, 0, 0);
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
	m_TabEdit.SetWindowText(mpt::ToCString(mpt::String::Replace(GetTabText(m_Tab.GetCurSel()), U_("\n"), U_("\r\n"))));
}


#ifdef ENABLE_ASM
static mpt::ustring ProcSupportToString(uint32 procSupport)
{
	std::vector<mpt::ustring> features;
#if MPT_COMPILER_MSVC && defined(ENABLE_ASM)
#if defined(ENABLE_X86)
	features.push_back(U_("x86"));
#endif
#if defined(ENABLE_AMD64)
	features.push_back(U_("amd64"));
#endif
	struct ProcFlag
	{
		decltype(procSupport) flag;
		const char *name;
	};
	static constexpr ProcFlag flags[] =
	{
		{ 0, "" },
#if defined(ENABLE_MMX)
		{ CPU::feature::mmx, "mmx" },
#endif
#if defined(ENABLE_SSE)
		{ CPU::feature::sse, "sse" },
#endif
#if defined(ENABLE_SSE2)
		{ CPU::feature::sse2, "sse2" },
#endif
#if defined(ENABLE_SSE3)
		{ CPU::feature::sse3, "sse3" },
		{ CPU::feature::ssse3, "ssse3" },
#endif
#if defined(ENABLE_SSE4)
		{ CPU::feature::sse4_1, "sse4.1" },
		{ CPU::feature::sse4_2, "sse4.2" },
#endif
#if defined(ENABLE_AVX)
		{ CPU::feature::avx, "avx" },
#endif
#if defined(ENABLE_AVX2)
		{ CPU::feature::avx2, "avx2" },
#endif
	};
	for(const auto &f : flags)
	{
		if(procSupport & f.flag) features.push_back(mpt::ToUnicode(mpt::Charset::ASCII, f.name));
	}
#else
	MPT_UNUSED_VARIABLE(procSupport);
#endif
	return mpt::String::Combine(features, U_(" "));
}
#endif


mpt::ustring CAboutDlg::GetTabText(int tab)
{
	const mpt::ustring lf = U_("\n");
	const mpt::ustring yes = U_("yes");
	const mpt::ustring no = U_("no");
#ifdef ENABLE_ASM
	const CPU::Info CPUInfo = CPU::Info::Get();
#endif
	mpt::ustring text;
	switch(tab)
	{
		case 0:
			text = U_("OpenMPT - Open ModPlug Tracker\n\n")
				+ MPT_UFORMAT("Version: {}\n")(Build::GetVersionStringExtended())
				+ MPT_UFORMAT("Source Code: {}\n")(SourceInfo::Current().GetUrlWithRevision() + UL_(" ") + SourceInfo::Current().GetStateString())
				+ MPT_UFORMAT("Build Date: {}\n")(Build::GetBuildDateString())
				+ MPT_UFORMAT("Compiler: {}\n")(Build::GetBuildCompilerString())
				+ MPT_UFORMAT("Architecture: {}\n")(mpt::OS::Windows::Name(mpt::OS::Windows::GetProcessArchitecture()))
				+ MPT_UFORMAT("Required Windows Kernel Level: {}\n")(mpt::OS::Windows::Version::VersionToString(mpt::OS::Windows::Version::GetMinimumKernelLevel()))
				+ MPT_UFORMAT("Required Windows API Level: {}\n")(mpt::OS::Windows::Version::VersionToString(mpt::OS::Windows::Version::GetMinimumAPILevel()));
			{
				text += U_("Required CPU features: ");
				std::vector<mpt::ustring> features;
				#if MPT_COMPILER_MSVC
					#if defined(_M_X64)
						features.push_back(U_("x86-64"));
						if(CPU::GetMinimumFeatures() & CPU::feature::avx) features.push_back(U_("avx"));
						if(CPU::GetMinimumFeatures() & CPU::feature::avx2) features.push_back(U_("avx2"));
					#elif defined(_M_IX86)
						if(CPU::GetMinimumFeatures() & CPU::feature::sse) features.push_back(U_("sse"));
						if(CPU::GetMinimumFeatures() & CPU::feature::sse2) features.push_back(U_("sse2"));
						if(CPU::GetMinimumFeatures() & CPU::feature::avx) features.push_back(U_("avx"));
						if(CPU::GetMinimumFeatures() & CPU::feature::avx2) features.push_back(U_("avx2"));
					#else
						if(CPU::GetMinimumFeatures() & CPU::feature::sse) features.push_back(U_("sse"));
						if(CPU::GetMinimumFeatures() & CPU::feature::sse2) features.push_back(U_("sse2"));
						if(CPU::GetMinimumFeatures() & CPU::feature::avx) features.push_back(U_("avx"));
						if(CPU::GetMinimumFeatures() & CPU::feature::avx2) features.push_back(U_("avx2"));
					#endif
				#endif
				text += mpt::String::Combine(features, U_(" "));
				text += lf;
			}
#ifdef ENABLE_ASM
			text += MPT_UFORMAT("Optional CPU features used: {}\n")(ProcSupportToString(CPU::GetEnabledFeatures()));
#endif // ENABLE_ASM
			text += lf;
			text += MPT_UFORMAT("System Architecture: {}\n")(mpt::OS::Windows::Name(mpt::OS::Windows::GetHostArchitecture()));
#ifdef ENABLE_ASM
			text += MPT_UFORMAT("CPU: {}, Family {}, Model {}, Stepping {}\n")
				( mpt::ToUnicode(mpt::Charset::ASCII, (std::strlen(CPUInfo.VendorID) > 0) ? std::string(CPUInfo.VendorID) : std::string("Generic"))
				, CPUInfo.Family
				, CPUInfo.Model
				, CPUInfo.Stepping
				);
			text += MPT_UFORMAT("CPU Name: {}\n")(mpt::ToUnicode(mpt::Charset::ASCII, (std::strlen(CPUInfo.BrandID) > 0) ? std::string(CPUInfo.BrandID) : std::string("")));
			text += MPT_UFORMAT("Available CPU features: {}\n")(ProcSupportToString(CPUInfo.AvailableFeatures));
#endif // ENABLE_ASM
			text += MPT_UFORMAT("Operating System: {}\n\n")(mpt::OS::Windows::Version::Current().GetName());
			text += MPT_UFORMAT("OpenMPT Install Path{1}: {0}\n")(theApp.GetInstallPath(), theApp.IsPortableMode() ? U_(" (portable)") : U_(""));
			text += MPT_UFORMAT("OpenMPT Executable Path{1}: {0}\n")(theApp.GetInstallBinArchPath(), theApp.IsPortableMode() ? U_(" (portable)") : U_(""));
			text += MPT_UFORMAT("Settings{1}: {0}\n")(theApp.GetConfigFileName(), theApp.IsPortableMode() ? U_(" (portable)") : U_(""));
			break;
		case 1:
			{
			std::vector<std::string> components = ComponentManager::Instance()->GetRegisteredComponents();
			if(!TrackerSettings::Instance().ComponentsKeepLoaded)
				{
					text += U_("Components are loaded and unloaded as needed.\n\n");
					for(const auto &component : components)
					{
						ComponentInfo info = ComponentManager::Instance()->GetComponentInfo(component);
						mpt::ustring name = mpt::ToUnicode(mpt::Charset::ASCII, (info.name.substr(0, 9) == "Component") ? info.name.substr(9) : info.name);
						if(!info.settingsKey.empty())
						{
							name = mpt::ToUnicode(mpt::Charset::ASCII, info.settingsKey);
						}
						text += name + lf;
					}
				} else
				{
					for(int available = 1; available >= 0; --available)
					{
						if(available)
						{
							text += U_("Loaded Components:\n");
						} else
						{
							text += U_("\nUnloaded Components:\n");
						}
						for(const auto &component : components)
						{
							ComponentInfo info = ComponentManager::Instance()->GetComponentInfo(component);
							if(available  && info.state != ComponentStateAvailable) continue;
							if(!available && info.state == ComponentStateAvailable) continue;
							mpt::ustring name = mpt::ToUnicode(mpt::Charset::ASCII, (info.name.substr(0, 9) == "Component") ? info.name.substr(9) : info.name);
							if(!info.settingsKey.empty())
							{
								name = mpt::ToUnicode(mpt::Charset::ASCII, info.settingsKey);
							}
							text += MPT_UFORMAT("{}: {}")
								( name
								, info.state == ComponentStateAvailable ? U_("ok") :
									info.state == ComponentStateUnavailable? U_("missing") :
									info.state == ComponentStateUnintialized ? U_("not loaded") :
									info.state == ComponentStateBlocked ? U_("blocked") :
									info.state == ComponentStateUnregistered ? U_("unregistered") :
									U_("unknown")
								);
							if(info.type != ComponentTypeUnknown)
							{
								text += MPT_UFORMAT(" ({})")
									( info.type == ComponentTypeBuiltin ? U_("builtin") :
										info.type == ComponentTypeSystem ? U_("system") :
										info.type == ComponentTypeSystemInstallable ? U_("system, optional") :
										info.type == ComponentTypeBundled ? U_("bundled") :
										info.type == ComponentTypeForeign ? U_("foreign") :
										U_("unknown")
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
			text += U_("Website:\n") + Build::GetURL(Build::Url::Website);
			text += U_("\n\nForum:\n") + Build::GetURL(Build::Url::Forum);
			text += U_("\n\nBug Tracker:\n") + Build::GetURL(Build::Url::Bugtracker);
			text += U_("\n\nUpdates:\n") + Build::GetURL(Build::Url::Updates);
			break;
		case 5:
			try
			{
				if(!theApp.GetWine())
				{
					text += U_("Wine integration not available.\n");
				} else
				{

					mpt::Wine::Context & wine = *theApp.GetWine();
					
					text += MPT_UFORMAT("Windows: {}\n")
						( mpt::OS::Windows::Version::Current().IsWindows() ? yes : no
						);
					text += MPT_UFORMAT("Windows version: {}\n")
						( 
						mpt::OS::Windows::Version::Current().IsAtLeast(mpt::OS::Windows::Version::Win81) ? U_("Windows 8.1") :
						mpt::OS::Windows::Version::Current().IsAtLeast(mpt::OS::Windows::Version::Win8) ? U_("Windows 8") :
						mpt::OS::Windows::Version::Current().IsAtLeast(mpt::OS::Windows::Version::Win7) ? U_("Windows 7") :
						mpt::OS::Windows::Version::Current().IsAtLeast(mpt::OS::Windows::Version::WinVista) ? U_("Windows Vista") :
						mpt::OS::Windows::Version::Current().IsAtLeast(mpt::OS::Windows::Version::WinXP) ? U_("Windows XP") :
						mpt::OS::Windows::Version::Current().IsAtLeast(mpt::OS::Windows::Version::Win2000) ? U_("Windows 2000") :
						mpt::OS::Windows::Version::Current().IsAtLeast(mpt::OS::Windows::Version::WinNT4) ? U_("Windows NT4") :
						U_("unknown")
						);
					text += MPT_UFORMAT("Windows original: {}\n")
						( mpt::OS::Windows::IsOriginal() ? yes : no
						);

					text += U_("\n");

					text += MPT_UFORMAT("Wine: {}\n")
						( mpt::OS::Windows::IsWine() ? yes : no
						);
					text += MPT_UFORMAT("Wine Version: {}\n")
						( mpt::ToUnicode(mpt::Charset::UTF8, wine.VersionContext().RawVersion())
						);
					text += MPT_UFORMAT("Wine Build ID: {}\n")
						( mpt::ToUnicode(mpt::Charset::UTF8, wine.VersionContext().RawBuildID())
						);
					text += MPT_UFORMAT("Wine Host Sys Name: {}\n")
						( mpt::ToUnicode(mpt::Charset::UTF8, wine.VersionContext().RawHostSysName())
						);
					text += MPT_UFORMAT("Wine Host Release: {}\n")
						( mpt::ToUnicode(mpt::Charset::UTF8, wine.VersionContext().RawHostRelease())
						);

					text += U_("\n");

					text += MPT_UFORMAT("uname -m: {}\n")
						( mpt::ToUnicode(mpt::Charset::UTF8, wine.Uname_m())
						);
					text += MPT_UFORMAT("HOME: {}\n")
						( mpt::ToUnicode(mpt::Charset::UTF8, wine.HOME())
						);
					text += MPT_UFORMAT("XDG_DATA_HOME: {}\n")
						( mpt::ToUnicode(mpt::Charset::UTF8, wine.XDG_DATA_HOME())
						);
					text += MPT_UFORMAT("XDG_CACHE_HOME: {}\n")
						( mpt::ToUnicode(mpt::Charset::UTF8, wine.XDG_CACHE_HOME())
						);
					text += MPT_UFORMAT("XDG_CONFIG_HOME: {}\n")
						( mpt::ToUnicode(mpt::Charset::UTF8, wine.XDG_CONFIG_HOME())
						);

					text += U_("\n");

					text += MPT_UFORMAT("OpenMPT folder: {}\n")
						( theApp.GetInstallPath().ToUnicode()
						);
					text += MPT_UFORMAT("OpenMPT folder (host): {}\n")
						( mpt::ToUnicode(mpt::Charset::UTF8, wine.PathToPosix(theApp.GetInstallPath()))
						);
					text += MPT_UFORMAT("OpenMPT config folder: {}\n")
						( theApp.GetConfigPath().ToUnicode()
						);
					text += MPT_UFORMAT("OpenMPT config folder (host): {}\n")
						( mpt::ToUnicode(mpt::Charset::UTF8, wine.PathToPosix(theApp.GetConfigPath()))
						);
					text += MPT_UFORMAT("Host root: {}\n")
						( wine.PathToWindows("/").ToUnicode()
						);

				}
			} catch(const mpt::Wine::Exception & e)
			{
				text += U_("Exception: ") + mpt::get_exception_text<mpt::ustring>(e) + U_("\n");
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
