/*
 * MPTrackWine.cpp
 * ---------------
 * Purpose: OpenMPT Wine support functions.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#include "MPTrackWine.h"

#include "mpt/uuid/uuid.hpp"

#include "Reporting.h"

#include "AboutDialog.h"
#include "Mainfrm.h"
#include "Mptrack.h"
#include "MPTrackUtilWine.h"
#include "TrackerSettings.h"
#include "../common/ComponentManager.h"
#include "../common/FileReader.h"
#include "../common/mptFileIO.h"
#include "../misc/mptOS.h"
#include "../misc/mptWine.h"
#include "mpt/crc/crc.hpp"
#include "mpt/fs/fs.hpp"
#include "mpt/io_file/fstream.hpp"
#include "mpt/io_file/inputfile.hpp"
#include "mpt/io_file_read/inputfile_filecursor.hpp"
#include "mpt/string/utility.hpp"
#include "openmpt/sounddevice/SoundDevice.hpp"
#include "openmpt/sounddevice/SoundDeviceManager.hpp"
#include "wine/NativeSoundDevice.h"
#include "wine/NativeSoundDeviceMarshalling.h"

#include <ios>

#include <contrib/minizip/unzip.h>
#include <contrib/minizip/iowin32.h>


OPENMPT_NAMESPACE_BEGIN


static mpt::ustring WineGetWindowTitle()
{
	return U_("OpenMPT Wine integration");
}


static std::string WineGetWindowTitleUTF8()
{
	return mpt::ToCharset(mpt::Charset::UTF8, WineGetWindowTitle());
}


static mpt::PathString WineGetSupportZipFilename()
{
	return P_("openmpt-wine-support.zip");
}


static char SanitizeBuildIdChar(char c)
{
	char result = c;
	if (c == '\0') result = '_';
	else if (c >= 'a' && c <= 'z') result = c;
	else if (c >= 'A' && c <= 'Z') result = c;
	else if (c >= '0' && c <= '9') result = c;
	else if (c == '!') result = c;
	else if (c == '+') result = c;
	else if (c == '-') result = c;
	else if (c == '.') result = c;
	else if (c == '~') result = c;
	else if (c == '_') result = c;
	else result = '_';
	return result;
}


static std::string SanitizeBuildID(std::string id)
{
	for(auto & c : id)
	{
		c = SanitizeBuildIdChar(c);
	}
	return id;
}


namespace WineIntegration {


static mpt::crc64_jones WineHashVersion(mpt::crc64_jones crc)
{
	std::string s;
	s += mpt::ToCharset(mpt::Charset::UTF8, Build::GetVersionStringExtended());
	s += " ";
	s += mpt::ToCharset(mpt::Charset::UTF8, mpt::OS::Windows::Name(mpt::OS::Windows::GetProcessArchitecture()));
	s += " ";
	s += mpt::ToCharset(mpt::Charset::UTF8, SourceInfo::Current().GetUrlWithRevision());
	s += " ";
	s += mpt::ToCharset(mpt::Charset::UTF8, SourceInfo::Current().GetStateString());
	crc(s.begin(), s.end());
	return crc;
}


static mpt::crc64_jones WineHashFile(mpt::crc64_jones crc, mpt::PathString filename)
{
	mpt::IO::InputFile file(filename, TrackerSettings::Instance().MiscCacheCompleteFileBeforeLoading);
	if(!file.IsValid())
	{
		return crc;
	}
	FileReader f = GetFileReader(file);
	FileReader::PinnedView view = f.ReadPinnedView();
	crc(view.begin(), view.end());
	return crc;
}


static mpt::crc64_jones WineHashSettings(mpt::crc64_jones crc)
{
	std::string result =
		"-c"
		"-" + mpt::afmt::dec(TrackerSettings::Instance().WineSupportEnablePulseAudio.Get()) +
		"-" + mpt::afmt::dec(TrackerSettings::Instance().WineSupportEnablePortAudio.Get());
	crc(result.begin(), result.end());
	return crc;
}


mpt::ustring WineGetSystemInfoString(mpt::OS::Wine::VersionContext & wineVersion)
{
	mpt::ustring msg;

	msg += CAboutDlg::GetTabText(5);

	msg += U_("\n");

	msg += MPT_UFORMAT("OpenMPT detected Wine {} running on {}.\n")
		( wineVersion.Version().AsString()
		, wineVersion.HostClass() == mpt::osinfo::osclass::Linux ? U_("Linux") : U_("unknown system")
		);

	return msg;

}


bool WineSetupIsSupported(mpt::OS::Wine::VersionContext & wineVersion)
{
	bool supported = true;
	if(wineVersion.RawBuildID().empty()) supported = false;
	if(!TrackerSettings::Instance().WineSupportAllowUnknownHost)
	{
		if((wineVersion.HostClass() == mpt::osinfo::osclass::Linux) || ((wineVersion.HostClass() == mpt::osinfo::osclass::BSD_) && wineVersion.RawHostSysName() == "FreeBSD"))
		{
			// ok
		} else
		{
			supported = false;
		}
	}
	if(!wineVersion.Version().IsValid()) supported = false;
	if(wineVersion.Version().IsAtLeast(mpt::osinfo::windows::wine::version{10, 11, 0}) && wineVersion.Version().IsBefore(mpt::osinfo::windows::wine::version{10, 12, 0}))
	{
		// Blacklisted due to <https://bugs.openmpt.org/view.php?id=1904>.
		// Probably caused by <https://gitlab.winehq.org/wine/wine/-/commit/96cd811903e3d3f227c39e12235725baf793f4b9>,
		// and maybe fixed by <https://gitlab.winehq.org/wine/wine/-/commit/aae0f624d48d1e4f79e6947798e61b4d1006bf76>.
		supported = false;
	}
	return supported;
}

bool WineSetupIsSupported(mpt::Wine::Context & wine)
{
	bool supported = true;
	if(theApp.GetInstallPath().empty()) supported = false;
	if(wine.PathToPosix(theApp.GetInstallPath()).empty()) supported = false;
	if(wine.PathToPosix(theApp.GetConfigPath()).empty()) supported = false;
	if(wine.PathToWindows("/").empty()) supported = false;
	if(supported)
	{
		if(wine.HOME().empty()) supported = false;
	}
	if(supported)
	{
		if(wine.Uname_m() == "x86_64" && mpt::pointer_size != 8) supported = false;
	}
	return supported;
}


static std::map<std::string, std::vector<char>> UnzipToMap(mpt::PathString filename)
{
	std::map<std::string, std::vector<char>> filetree;
	{
		zlib_filefunc64_def zipfilefuncs;
		MemsetZero(zipfilefuncs);
		fill_win32_filefunc64W(&zipfilefuncs);
		unzFile zipfile = unzOpen2_64(filename.ToWide().c_str(), &zipfilefuncs);
		if(!zipfile)
		{
			throw mpt::Wine::Exception("Archive is not a zip file");
		}
		for(int status = unzGoToFirstFile(zipfile); status == UNZ_OK; status = unzGoToNextFile(zipfile))
		{
			int openstatus = UNZ_OK;
			openstatus = unzOpenCurrentFile(zipfile);
			if(openstatus != UNZ_OK)
			{
				unzClose(zipfile);
				throw mpt::Wine::Exception("Archive is corrupted.");
			}
			unz_file_info info;
			MemsetZero(info);
			char name[1024];
			MemsetZero(name);
			openstatus = unzGetCurrentFileInfo(zipfile, &info, name, sizeof(name) - 1, nullptr, 0, nullptr, 0);
			if(openstatus != UNZ_OK)
			{
				unzCloseCurrentFile(zipfile);
				unzClose(zipfile);
				throw mpt::Wine::Exception("Archive is corrupted.");
			}
			std::vector<char> data(info.uncompressed_size);
			unzReadCurrentFile(zipfile, &data[0], info.uncompressed_size);
			unzCloseCurrentFile(zipfile);

			data = mpt::buffer_cast<std::vector<char>>(mpt::replace(mpt::buffer_cast<std::string>(data), std::string("\r\n"), std::string("\n")));

			filetree[mpt::replace(mpt::ToCharset(mpt::Charset::UTF8, mpt::Charset::CP437, name), std::string("\\"), std::string("/"))] = data;
		}
		unzClose(zipfile);
	}
	return filetree;
}


bool IsSupported()
{
	return theApp.GetWine() ? true : false;
}


bool IsCompiled()
{
	return !theApp.GetWineWrapperDllFilename().empty();
}


void Initialize()
{

	if(!mpt::OS::Windows::IsWine())
	{
		return;
	}

	if(!TrackerSettings::Instance().WineSupportEnabled)
	{
		return;
	}

	mpt::OS::Wine::VersionContext wineVersion = *theApp.GetWineVersion();
	if(!WineSetupIsSupported(wineVersion))
	{
		Reporting::Notification(U_("OpenMPT does not support Wine integration on your current Wine setup."), WineGetWindowTitle());
		return;
	}

	try
	{
		mpt::Wine::Context wine = mpt::Wine::Context(wineVersion);
		if(!WineSetupIsSupported(wine))
		{
			Reporting::Notification(U_("OpenMPT does not support Wine integration on your current Wine setup."), WineGetWindowTitle());
			return;
		}
		theApp.SetWine(std::make_shared<mpt::Wine::Context>(wine));
	} catch(const std::exception & e)
	{
		mpt::ustring msg =
			U_("OpenMPT was not able to determine Wine configuration details on your current Wine setup:" "\n") +
			mpt::get_exception_text<mpt::ustring>(e) +
			U_("\n" "OpenMPT native Wine Integration will not be available.");
		Reporting::Error(msg, WineGetWindowTitle());
		return;
	}
	mpt::Wine::Context wine = *theApp.GetWine();

	try
	{

		struct Paths
		{
			mpt::PathString AppData;
			mpt::PathString AppData_Wine;
			mpt::PathString AppData_Wine_WineVersion;
			mpt::PathString AppData_Wine_WineVersion_OpenMPTVersion;
			std::string Host_AppData;
			std::string Host_AppData_Wine;
			std::string Host_AppData_Wine_WineVersion;
			std::string Host_AppData_Wine_WineVersion_OpenMPTVersion;
			std::string Host_Native_OpenMPT_Wine_WineVersion_OpenMPTVersion;
			static void CreatePath(mpt::PathString path)
			{
				if(mpt::native_fs{}.is_directory(path))
				{
					return;
				}
				if(CreateDirectory(path.AsNative().c_str(), NULL) == 0)
				{
					throw mpt::Wine::Exception("Failed to create directory: " + path.ToUTF8());
				}
			}
			std::string GetOpenMPTVersion() const
			{
				mpt::crc64_jones crc;
				crc = WineHashVersion(crc);
				crc = WineHashFile(crc, theApp.GetInstallPath() + WineGetSupportZipFilename());
				crc = WineHashSettings(crc);
				std::string ver = mpt::ToCharset(mpt::Charset::UTF8, Build::GetVersionStringPure() + U_("_") + mpt::OS::Windows::Name(mpt::OS::Windows::GetProcessArchitecture()))
					+ std::string("-") + mpt::afmt::hex0<16>(crc.result());
				return ver;
			}
			Paths(mpt::Wine::Context & wine)
			{
				AppData = theApp.GetConfigPath().WithoutTrailingSlash();
				AppData_Wine = AppData.WithTrailingSlash() + P_("Wine");
				AppData_Wine_WineVersion = AppData_Wine.WithTrailingSlash() + mpt::PathString::FromUTF8(SanitizeBuildID(wine.VersionContext().RawBuildID()));
				AppData_Wine_WineVersion_OpenMPTVersion = AppData_Wine_WineVersion.WithTrailingSlash() + mpt::PathString::FromUTF8(GetOpenMPTVersion());
				CreatePath(AppData);
				CreatePath(AppData_Wine);
				CreatePath(AppData_Wine_WineVersion);
				CreatePath(AppData_Wine_WineVersion_OpenMPTVersion);
				Host_AppData = wine.PathToPosixCanonical(AppData);
				Host_AppData_Wine = wine.PathToPosixCanonical(AppData_Wine);
				Host_AppData_Wine_WineVersion = wine.PathToPosixCanonical(AppData_Wine_WineVersion);
				Host_AppData_Wine_WineVersion_OpenMPTVersion = wine.PathToPosixCanonical(AppData_Wine_WineVersion_OpenMPTVersion);
				Host_Native_OpenMPT_Wine_WineVersion_OpenMPTVersion = wine.XDG_DATA_HOME() + "/OpenMPT/Wine/" + SanitizeBuildID(wine.VersionContext().RawBuildID()) + "/" + GetOpenMPTVersion();
			}
		};

		const Paths paths(wine);

		const std::string nativeSearchPath = paths.Host_Native_OpenMPT_Wine_WineVersion_OpenMPTVersion;

		if(!TrackerSettings::Instance().WineSupportAlwaysRecompile)
		{
			if(mpt::native_fs{}.is_file(paths.AppData_Wine_WineVersion_OpenMPTVersion.WithTrailingSlash() + P_("success.txt")))
			{
				theApp.SetWineWrapperDllFilename(paths.AppData_Wine_WineVersion_OpenMPTVersion.WithTrailingSlash() + P_("openmpt_wine_wrapper.dll"));
				return;
			}
		}

		if(TrackerSettings::Instance().WineSupportAskCompile)
		{
			if(Reporting::Confirm(U_("OpenMPT Wine integration requires recompilation and will not work otherwise." "\n"
				"Recompile now?"), WineGetWindowTitle(), false, false) != cnfYes)
			{
				return;
			}
		}

		std::map<std::string, std::vector<char>> filetree;
		filetree = UnzipToMap(theApp.GetInstallPath() + WineGetSupportZipFilename());

		Util::Wine::Dialog dialog(WineGetWindowTitleUTF8(), TrackerSettings::Instance().WineSupportCompileVerbosity < 6);

		std::string script =
			"#!/usr/bin/env sh" "\n"
			"\n"
			"touch message.txt" "\n"
			"\n";

		script += dialog.Detect();

		if(TrackerSettings::Instance().WineSupportCompileVerbosity >= 6)
		{
			script +=
				"echo Working directory:" "\n"
				"pwd" "\n";
		}

		script += "\n";

		if(TrackerSettings::Instance().WineSupportCompileVerbosity >= 3)
		{
			script += "echo " + WineGetWindowTitleUTF8() + "\n";
		} else
		{
			if(TrackerSettings::Instance().WineSupportCompileVerbosity == 2)
			{
				script +=
					"{" "\n"
					" echo 0" "\n"
					" echo 100" "\n"
					"} | " + dialog.Progress("[>>] Prepare OpenMPT Wine Integration\\n[  ] Compile native support\\n[  ] Compile Wine wrapper\\n\\n[1/3] Preparing OpenMPT Wine Integration ...") + "\n";
			} else
			{
				script += dialog.Status("Preparing OpenMPT Wine Integration.") + "\n";
			}
		}

		script +=
			"\n"
			"printf \"#pragma once\\n\" >> common/svn_version.h" "\n"
			"printf \"#define OPENMPT_VERSION_URL \\\"" + mpt::ToCharset(mpt::Charset::ASCII, SourceInfo::Current().Url()) + "\\\"\\n\" >> common/svn_version.h" "\n"
			"printf \"#define OPENMPT_VERSION_DATE \\\"" + mpt::ToCharset(mpt::Charset::ASCII, SourceInfo::Current().Date()) + "\\\"\\n\" >> common/svn_version.h" "\n"
			"printf \"#define OPENMPT_VERSION_REVISION " + mpt::afmt::dec(SourceInfo::Current().Revision()) + "\\n\" >> common/svn_version.h" "\n"
			"printf \"#define OPENMPT_VERSION_DIRTY " + mpt::afmt::dec(SourceInfo::Current().IsDirty()) + "\\n\" >> common/svn_version.h" "\n"
			"printf \"#define OPENMPT_VERSION_MIXEDREVISIONS " + mpt::afmt::dec(SourceInfo::Current().HasMixedRevisions()) + "\\n\" >> common/svn_version.h" "\n"
			"printf \"#define OPENMPT_VERSION_IS_PACKAGE " + mpt::afmt::dec(SourceInfo::Current().IsPackage()) + "\\n\" >> common/svn_version.h" "\n"
			"\n"
			"missing=" "\n"
			"\n";

		const std::string make = ((wineVersion.HostClass() == mpt::osinfo::osclass::BSD_) ? "gmake" : "make");

		const std::string commands[] =
		{
			make,
			"pkg-config",
			"cpp",
			"cc",
			"c++",
			"ld",
			"ccache",
		};
		for(const auto &command : commands)
		{
			script +=
				"command -v " + command + " 2>/dev/null 1>/dev/null" "\n"
				"if [ \"$?\" -ne \"0\" ] ; then" "\n"
				" missing=\"$missing " + command + "\"" "\n"
				"fi" "\n";
		}

		script +=
			"if [ \"x$missing\" = \"x\" ] ; then" "\n"
			" printf \"\"" "\n"
			"else" "\n";
#if 0
		if(!TrackerSettings::Instance().WineSupportSilentCompile >= 1)
		{
			script += " " + dialog.YesNo("The following commands are missing:\\n\\n$missing\\n\\nDo you want OpenMPT to try installing those now?") + "\n";
		}
		script +=
			" if [ \"$?\" -ne \"0\" ] ; then" "\n"
			"  exit 1" "\n"
			" fi" "\n";
#else
		if(TrackerSettings::Instance().WineSupportCompileVerbosity >= 1)
		{
			script += " " + dialog.MessageBox("The following commands are missing:\\n\\n$missing\\n\\nPlease install them with your system package installer.") + "\n";
		}
		script += " exit 1" "\n";
#endif
		script +=
			"fi" "\n"
			"\n"
			"mkdir -p " + wine.EscapePosixShell(wine.XDG_DATA_HOME()) + "/OpenMPT/Wine" "\n"
			"mkdir -p " + wine.EscapePosixShell(wine.XDG_CACHE_HOME()) + "/OpenMPT/Wine" "\n"
			"mkdir -p " + wine.EscapePosixShell(wine.XDG_CONFIG_HOME()) + "/OpenMPT/Wine" "\n"
			"mkdir -p " + wine.EscapePosixShell(paths.Host_Native_OpenMPT_Wine_WineVersion_OpenMPTVersion) + "\n"
			"\n"
			"CCACHE_DIR=" + wine.EscapePosixShell(wine.XDG_CACHE_HOME()) + "/OpenMPT/Wine/ccache" "\n"
			"CCACHE_COMPRESS=1" " \n"
			"export CCACHE_DIR" " \n"
			"export CCACHE_COMPRESS" " \n"
			"\n";

		std::vector<std::string> winegcc;
		if constexpr(mpt::arch_bits == 32)
		{ // 32bit winegcc probably cannot compile to 64bit
			winegcc.push_back("winegcc32-development");
		}
		MPT_MAYBE_CONSTANT_IF(TrackerSettings::Instance().WineSupportForeignOpenMPT || (mpt::arch_bits == 64))
		{
			winegcc.push_back("winegcc64-development");
		}
		winegcc.push_back("winegcc-development");
		if(wineVersion.HostClass() != mpt::osinfo::osclass::BSD_)
		{ // avoid C++ compiler on *BSD because libc++ Win32 support tends to be missing there.
			if constexpr(mpt::arch_bits == 32)
			{ // 32bit winegcc probably cannot compile to 64bit
				winegcc.push_back("wineg++32-development");
			}
			MPT_MAYBE_CONSTANT_IF(TrackerSettings::Instance().WineSupportForeignOpenMPT || (mpt::arch_bits == 64))
			{
				winegcc.push_back("wineg++64-development");
			}
			winegcc.push_back("wineg++-development");
		}
		if constexpr(mpt::arch_bits == 32)
		{ // 32bit winegcc probably cannot compile to 64bit
			winegcc.push_back("winegcc32");
		}
		MPT_MAYBE_CONSTANT_IF(TrackerSettings::Instance().WineSupportForeignOpenMPT || (mpt::arch_bits == 64))
		{
			winegcc.push_back("winegcc64");
		}
		winegcc.push_back("winegcc");
		if(wineVersion.HostClass() != mpt::osinfo::osclass::BSD_)
		{ // avoid C++ compiler on *BSD because libc++ Win32 support tends to be missing there.
			if constexpr(mpt::arch_bits == 32)
			{ // 32bit winegcc probably cannot compile to 64bit
				winegcc.push_back("wineg++32");
			}
			MPT_MAYBE_CONSTANT_IF(TrackerSettings::Instance().WineSupportForeignOpenMPT || (mpt::arch_bits == 64))
			{
				winegcc.push_back("wineg++64");
			}
			winegcc.push_back("wineg++");
		}
		for(const auto &c : winegcc)
		{
			script +=
				"if command -v " + c + " 2>/dev/null 1>/dev/null ; then" "\n"
				" MPT_WINEGXX=" + c + "\n"
				"fi" "\n";
		}
		script += "if [ -z $MPT_WINEGXX ] ; then" "\n";
		if(TrackerSettings::Instance().WineSupportCompileVerbosity >= 1)
		{
			script += " " + dialog.MessageBox("WineGCC not found.\\nPlease install it with your system package installer.") + "\n";
		}
		script +=
			" exit 1" "\n"
			"fi" "\n";

		// Work-around for Debian 8, Wine 1.6.2
		MPT_MAYBE_CONSTANT_IF(TrackerSettings::Instance().WineSupportForeignOpenMPT || (mpt::arch_bits == 64))
		{
			script +=
				"if [ `$MPT_WINEGXX > /dev/null 2>&1 ; echo $?` -eq 127 ] ; then" "\n"
				" if command -v /usr/lib/x86_64-linux-gnu/wine/bin/winegcc 2>/dev/null 1>/dev/null ; then" "\n"
				"  MPT_WINEGXX=/usr/lib/x86_64-linux-gnu/wine/bin/winegcc" "\n"
				"  PATH=/usr/lib/x86_64-linux-gnu/wine/bin:\"${PATH}\"" "\n"
				"  export PATH" "\n"
				" fi" "\n"
				"fi" "\n";
		}
		if constexpr(mpt::arch_bits == 32)
		{
			script +=
				"if [ `$MPT_WINEGXX > /dev/null 2>&1 ; echo $?` -eq 127 ] ; then" "\n"
				" if command -v /usr/lib/i386-linux-gnu/wine/bin/winegcc 2>/dev/null 1>/dev/null ; then" "\n"
				"  MPT_WINEGXX=/usr/lib/i386-linux-gnu/wine/bin/winegcc" "\n"
				"  PATH=/usr/lib/i386-linux-gnu/wine/bin:\"${PATH}\"" "\n"
				"  export PATH" "\n"
				" fi" "\n"
				"fi" "\n";
		}

		std::string features;
		if(TrackerSettings::Instance().WineSupportForeignOpenMPT)
		{
			features += " MPT_ARCH_BITS=" + mpt::afmt::dec(mpt::arch_bits);
			if constexpr(mpt::arch_bits == 64)
			{
				features += " MPT_TARGET=" "x86_64-linux-gnu-";
			} else
			{
				features += " MPT_TARGET=" "i686-linux-gnu-";
			}
		}
		features += " MPT_TRY_PORTAUDIO=" + mpt::afmt::dec(TrackerSettings::Instance().WineSupportEnablePortAudio.Get());
		features += " MPT_TRY_PULSEAUDIO=" + mpt::afmt::dec(TrackerSettings::Instance().WineSupportEnablePulseAudio.Get());
		features += " MPT_TRY_RTAUDIO=" + mpt::afmt::dec(TrackerSettings::Instance().WineSupportEnableRtAudio.Get());
		
		const int makeverbosity = Clamp(TrackerSettings::Instance().WineSupportCompileVerbosity.Get(), 0, 6);

		if(TrackerSettings::Instance().WineSupportCompileVerbosity == 2)
		{

			script +=
				"{" "\n"
				" echo 0" "\n"
				" " + make + " -j " + mpt::afmt::dec(std::max(std::thread::hardware_concurrency(), static_cast<unsigned int>(1))) + " -f build/wine/native_support.mk" + " V=" + mpt::afmt::dec(makeverbosity) + " " + features + " all MPT_PROGRESS_FILE=\"&4\" 4>&1 1>stdout.txt 2>stderr.txt" "\n"
				" echo -n $? > stdexit.txt" "\n"
				" echo 100" "\n"
				"} | " + dialog.Progress("[OK] Prepare OpenMPT Wine Integration\\n[>>] Compile native support\\n[  ] Compile Wine wrapper\\n\\n[2/3] Compiling native support ...") + "\n"
				"MPT_EXITCODE=`cat stdexit.txt`" "\n"
				"if [ \"$MPT_EXITCODE\" -ne \"0\" ] ; then" "\n";
			if(TrackerSettings::Instance().WineSupportCompileVerbosity >= 1)
			{
				script +=
					" " + dialog.MessageBox("OpenMPT Wine integration failed to compile.") + "\n"
					" " + dialog.TextBox("stderr.txt") + "\n";
			}
			script +=
				" exit 1" "\n"
				"fi" "\n"
				"if [ -s stderr.txt ] ; then" "\n"
				" " + dialog.TextBox("stderr.txt") + "\n"
				"fi" "\n"

				"{" "\n"
				" echo 0" "\n"
				" " + make + " -j " + mpt::afmt::dec(std::max(std::thread::hardware_concurrency(), static_cast<unsigned int>(1))) + " -f build/wine/wine_wrapper.mk" + " V=" + mpt::afmt::dec(makeverbosity) + " WINEGXX=$MPT_WINEGXX " + "MPT_WINEGCC_LANG=" + ((wineVersion.HostClass() == mpt::osinfo::osclass::BSD_) ? "C" : "CPLUSPLUS") + " MPT_WINE_SEARCHPATH=" + wine.EscapePosixShell(nativeSearchPath) + " all MPT_PROGRESS_FILE=\"&4\" 4>&1 1>stdout.txt 2>stderr.txt" "\n"
				" echo -n $? > stdexit.txt" "\n"
				" echo 100" "\n"
				"} | " + dialog.Progress("[OK] Prepare OpenMPT Wine Integration\\n[OK] Compile native support\\n[>>] Compile Wine wrapper\\n\\n[3/3] Compiling Wine wrapper ...") + "\n"
				"MPT_EXITCODE=`cat stdexit.txt`" "\n"
				"if [ \"$MPT_EXITCODE\" -ne \"0\" ] ; then" "\n";
			if(TrackerSettings::Instance().WineSupportCompileVerbosity >= 1)
			{
				script +=
					" " + dialog.MessageBox("OpenMPT Wine integration failed to compile.") + "\n"
					" " + dialog.TextBox("stderr.txt") + "\n";
			}
			script +=
				" exit 1" "\n"
				"fi" "\n"
				"if [ -s stderr.txt ] ; then" "\n"
				" " + dialog.TextBox("stderr.txt") + "\n"
				"fi" "\n";

		} else
		{

			script +=
				make + " -j " + mpt::afmt::dec(std::max(std::thread::hardware_concurrency(), static_cast<unsigned int>(1))) + " -f build/wine/native_support.mk" + " V=" + mpt::afmt::dec(makeverbosity) + " " + features + " all" "\n"
				"if [ \"$?\" -ne \"0\" ] ; then" "\n";
			if(TrackerSettings::Instance().WineSupportCompileVerbosity >= 1)
			{
				script +=
					" " + dialog.MessageBox("OpenMPT Wine integration failed to compile.") + "\n"
					" " + dialog.TextBox("stderr.txt") + "\n";
			}
			script +=
				" exit 1" "\n"
				"fi" "\n"
				"if [ -s stderr.txt ] ; then" "\n"
				" " + dialog.TextBox("stderr.txt") + "\n"
				"fi" "\n";

			script +=
				make + " -j " + mpt::afmt::dec(std::max(std::thread::hardware_concurrency(), static_cast<unsigned int>(1))) + " -f build/wine/wine_wrapper.mk" + " V=" + mpt::afmt::dec(makeverbosity) + " WINEGXX=$MPT_WINEGXX " + "MPT_WINEGCC_LANG=" + ((wineVersion.HostClass() == mpt::osinfo::osclass::BSD_) ? "C" : "CPLUSPLUS") + " MPT_WINE_SEARCHPATH=" + wine.EscapePosixShell(nativeSearchPath) + " all" "\n"
				"if [ \"$?\" -ne \"0\" ] ; then" "\n";
			if(TrackerSettings::Instance().WineSupportCompileVerbosity >= 1)
			{
				script +=
					" " + dialog.MessageBox("OpenMPT Wine integration failed to compile.") + "\n"
					" " + dialog.TextBox("stderr.txt") + "\n";
			}
			script +=
				" exit 1" "\n"
				"fi" "\n"
				"if [ -s stderr.txt ] ; then" "\n"
				" " + dialog.TextBox("stderr.txt") + "\n"
				"fi" "\n";

		}
		
		script += "\n";

		if(TrackerSettings::Instance().WineSupportCompileVerbosity >= 6)
		{
			script += dialog.MessageBox("OpenMPT Wine integration compiled successfully.") + "\n";
		}

		script +=
			"\n"
			"exit 0" "\n";

		CMainFrame::GetMainFrame()->EnableWindow(FALSE);
		mpt::Wine::ExecResult result;
		try
		{
			FlagSet<mpt::Wine::ExecFlags> flags = mpt::Wine::ExecFlagNone;
			if(TrackerSettings::Instance().WineSupportCompileVerbosity >= 1)
			{
				flags = (mpt::Wine::ExecFlagProgressWindow | mpt::Wine::ExecFlagInteractive);
			} else if(TrackerSettings::Instance().WineSupportCompileVerbosity == 0)
			{
				flags = (mpt::Wine::ExecFlagProgressWindow | mpt::Wine::ExecFlagSilent);
			} else
			{
				flags = (mpt::Wine::ExecFlagSilent);
			}
			result = Util::Wine::ExecutePosixShellScript
				( wine
				, script
				, flags
				, filetree
				, WineGetWindowTitleUTF8()
				, "Compiling Wine support ..."
				);
		} catch(const mpt::Wine::Exception & /* e */ )
		{
			CMainFrame::GetMainFrame()->EnableWindow(TRUE);
			throw;
		}
		CMainFrame::GetMainFrame()->EnableWindow(TRUE);
		if(result.exitcode != 0)
		{
			if(result.filetree["message.txt"].size() > 0)
			{
				throw mpt::Wine::Exception(std::string(result.filetree["message.txt"].begin(), result.filetree["message.txt"].end()));
			} else
			{
				throw mpt::Wine::Exception("Executing Wine integration build script failed.");
			}
		}

		{
			std::string fn = "libopenmpt_native_support.so";
			mpt::IO::ofstream f(wine.PathToWindows(nativeSearchPath) + P_("\\") + mpt::PathString::FromUTF8(fn), std::ios::binary);
			f.write(&result.filetree[fn][0], result.filetree[fn].size());
			f.flush();
			if(!f)
			{
				throw mpt::Wine::Exception("Writing libopenmpt_native_support.so failed.");
			}
		}
		{
			std::string fn = "openmpt_wine_wrapper.dll";
			mpt::IO::ofstream f(paths.AppData_Wine_WineVersion_OpenMPTVersion + P_("\\") + mpt::PathString::FromUTF8(fn), std::ios::binary);
			f.write(&result.filetree[fn][0], result.filetree[fn].size());
			f.flush();
			if(!f)
			{
				throw mpt::Wine::Exception("Writing openmpt_wine_wrapper.dll failed.");
			}
		}
		{
			std::string fn = "success.txt";
			mpt::IO::ofstream f(paths.AppData_Wine_WineVersion_OpenMPTVersion + P_("\\") + mpt::PathString::FromUTF8(fn), std::ios::binary);
			f.imbue(std::locale::classic());
			f << std::string("1");
			f.flush();
			if(!f)
			{
				throw mpt::Wine::Exception("Writing success.txt failed.");
			}
		}

		theApp.SetWineWrapperDllFilename(paths.AppData_Wine_WineVersion_OpenMPTVersion + P_("\\") + P_("openmpt_wine_wrapper.dll"));

	} catch(const mpt::Wine::Exception &e)
	{
		Reporting::Error(U_("Setting up OpenMPT Wine integration failed: ") + mpt::get_exception_text<mpt::ustring>(e), WineGetWindowTitle());
	}

}


} // namespace WineIntegration


std::string ComponentWineWrapper::result_as_string(char * str) const
{
	std::string result = str;
	OpenMPT_Wine_Wrapper_String_Free(str);
	return result;
}

ComponentWineWrapper::ComponentWineWrapper()
	: ComponentLibrary(ComponentTypeBundled)
{
	return;
}

bool ComponentWineWrapper::DoInitialize()
{
	if(theApp.GetWineWrapperDllFilename().empty())
	{
		return false;
	}
	AddLibrary("WineWrapper", mpt::LibraryPath::FullPath(theApp.GetWineWrapperDllFilename()));
	
	MPT_COMPONENT_BIND("WineWrapper", OpenMPT_Wine_Wrapper_Init);
	MPT_COMPONENT_BIND("WineWrapper", OpenMPT_Wine_Wrapper_Fini);
	
	MPT_COMPONENT_BIND("WineWrapper", OpenMPT_Wine_Wrapper_String_Free);
	
	MPT_COMPONENT_BIND("WineWrapper", OpenMPT_Wine_Wrapper_SoundDevice_EnumerateDevices);
	
	MPT_COMPONENT_BIND("WineWrapper", OpenMPT_Wine_Wrapper_SoundDevice_Construct);
	MPT_COMPONENT_BIND("WineWrapper", OpenMPT_Wine_Wrapper_SoundDevice_Destruct);

	MPT_COMPONENT_BIND("WineWrapper", OpenMPT_Wine_Wrapper_SoundDevice_SetMessageReceiver);
	MPT_COMPONENT_BIND("WineWrapper", OpenMPT_Wine_Wrapper_SoundDevice_SetCallback);

	MPT_COMPONENT_BIND("WineWrapper", OpenMPT_Wine_Wrapper_SoundDevice_GetDeviceInfo);
	MPT_COMPONENT_BIND("WineWrapper", OpenMPT_Wine_Wrapper_SoundDevice_GetDeviceCaps);
	MPT_COMPONENT_BIND("WineWrapper", OpenMPT_Wine_Wrapper_SoundDevice_GetDeviceDynamicCaps);
	MPT_COMPONENT_BIND("WineWrapper", OpenMPT_Wine_Wrapper_SoundDevice_Init);
	MPT_COMPONENT_BIND("WineWrapper", OpenMPT_Wine_Wrapper_SoundDevice_Open);
	MPT_COMPONENT_BIND("WineWrapper", OpenMPT_Wine_Wrapper_SoundDevice_Close);
	MPT_COMPONENT_BIND("WineWrapper", OpenMPT_Wine_Wrapper_SoundDevice_Start);
	MPT_COMPONENT_BIND("WineWrapper", OpenMPT_Wine_Wrapper_SoundDevice_Stop);
	MPT_COMPONENT_BIND("WineWrapper", OpenMPT_Wine_Wrapper_SoundDevice_GetRequestFlags);
	MPT_COMPONENT_BIND("WineWrapper", OpenMPT_Wine_Wrapper_SoundDevice_IsInited);
	MPT_COMPONENT_BIND("WineWrapper", OpenMPT_Wine_Wrapper_SoundDevice_IsOpen);
	MPT_COMPONENT_BIND("WineWrapper", OpenMPT_Wine_Wrapper_SoundDevice_IsAvailable);
	MPT_COMPONENT_BIND("WineWrapper", OpenMPT_Wine_Wrapper_SoundDevice_IsPlaying);
	MPT_COMPONENT_BIND("WineWrapper", OpenMPT_Wine_Wrapper_SoundDevice_IsPlayingSilence);
	MPT_COMPONENT_BIND("WineWrapper", OpenMPT_Wine_Wrapper_SoundDevice_StopAndAvoidPlayingSilence);
	MPT_COMPONENT_BIND("WineWrapper", OpenMPT_Wine_Wrapper_SoundDevice_EndPlayingSilence);
	MPT_COMPONENT_BIND("WineWrapper", OpenMPT_Wine_Wrapper_SoundDevice_OnIdle);
	MPT_COMPONENT_BIND("WineWrapper", OpenMPT_Wine_Wrapper_SoundDevice_GetSettings);
	MPT_COMPONENT_BIND("WineWrapper", OpenMPT_Wine_Wrapper_SoundDevice_GetActualSampleFormat);
	MPT_COMPONENT_BIND("WineWrapper", OpenMPT_Wine_Wrapper_SoundDevice_GetEffectiveBufferAttributes);
	MPT_COMPONENT_BIND("WineWrapper", OpenMPT_Wine_Wrapper_SoundDevice_GetTimeInfo);
	MPT_COMPONENT_BIND("WineWrapper", OpenMPT_Wine_Wrapper_SoundDevice_GetStreamPosition);
	MPT_COMPONENT_BIND("WineWrapper", OpenMPT_Wine_Wrapper_SoundDevice_DebugIsFragileDevice);
	MPT_COMPONENT_BIND("WineWrapper", OpenMPT_Wine_Wrapper_SoundDevice_DebugInRealtimeCallback);
	MPT_COMPONENT_BIND("WineWrapper", OpenMPT_Wine_Wrapper_SoundDevice_GetStatistics);
	MPT_COMPONENT_BIND("WineWrapper", OpenMPT_Wine_Wrapper_SoundDevice_OpenDriverSettings);
	
	if(HasBindFailed())
	{
		Reporting::Error("OpenMPT Wine integration failed loading.", WineGetWindowTitle());
		return false;
	}
	if(OpenMPT_Wine_Wrapper_Init() != 0)
	{
		Reporting::Error("OpenMPT Wine integration initialization failed.", WineGetWindowTitle());
		return false;
	}
	if(TrackerSettings::Instance().WineSupportCompileVerbosity >= 6)
	{
		Reporting::Notification(MPT_AFORMAT("OpenMPT Wine integration loaded successfully.")(), WineGetWindowTitle());
	}
	return true;
}

ComponentWineWrapper::~ComponentWineWrapper()
{
	if(IsAvailable())
	{
		OpenMPT_Wine_Wrapper_Fini();
	}
}


namespace WineIntegration {


void Load()
{
	ReloadComponent<ComponentWineWrapper>();
}


} // namespace WineIntegration


OPENMPT_NAMESPACE_END
