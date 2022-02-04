
 project "xmp-openmpt"
  uuid "AEA14F53-ADB0-45E5-9823-81F4F36886C2"
  language "C++"
  location ( "../../build/" .. mpt_projectpathname )
  vpaths { ["*"] = "../../libopenmpt/" }
  mpt_projectname = "xmp-openmpt"
  dofile "../../build/premake/premake-defaults-DLL.lua"
  dofile "../../build/premake/premake-defaults.lua"
  warnings "Extra"
  local extincludedirs = {
   "../..",
   "../../include",
   "../../include/pugixml/src",
  }
  filter { "action:vs*" }
    includedirs ( extincludedirs )
  filter { "not action:vs*" }
    sysincludedirs ( extincludedirs )
  filter {}
  includedirs {
   "../..",
   "$(IntDir)/svn_version",
   "../../build/svn_version",
  }
  files {
   "../../libopenmpt/xmp-openmpt.cpp",
   "../../libopenmpt/libopenmpt_plugin_settings.hpp",
   "../../libopenmpt/libopenmpt_plugin_gui.hpp",
   "../../libopenmpt/libopenmpt_plugin_gui.cpp",
   "../../libopenmpt/libopenmpt_plugin_gui.rc",
   "../../libopenmpt/resource.h",
  }

	filter { "action:vs*", "kind:SharedLib or ConsoleApp or WindowedApp" }
		resdefines {
			"MPT_BUILD_VER_FILENAME=\"" .. mpt_projectname .. ".dll\"",
			"MPT_BUILD_VER_FILEDESC=\"" .. mpt_projectname .. "\"",
		}
	filter { "action:vs*", "kind:SharedLib or ConsoleApp or WindowedApp" }
		resincludedirs {
			"$(IntDir)/svn_version",
			"../../build/svn_version",
			"$(ProjDir)/../../build/svn_version",
		}
		files {
			"../../libopenmpt/libopenmpt_version.rc",
		}
	filter { "action:vs*", "kind:SharedLib" }
		resdefines { "MPT_BUILD_VER_DLL" }
	filter { "action:vs*", "kind:ConsoleApp or WindowedApp" }
		resdefines { "MPT_BUILD_VER_EXE" }
	filter {}

  characterset "Unicode"
  flags { "MFC" }
	-- work-around https://developercommunity.visualstudio.com/t/link-errors-when-building-mfc-application-with-cla/1617786
	if _OPTIONS["clang"] then
		filter {}
		filter { "configurations:Debug" }
			if charset == "Unicode" then
				ignoredefaultlibraries { "uafxcwd.lib", "libcmtd.lib" }
				links { "uafxcwd.lib", "libcmtd.lib" }
			else
				ignoredefaultlibraries { "nafxcwd.lib", "libcmtd.lib" }
				links { "nafxcwd.lib", "libcmtd.lib" }
			end
		filter { "configurations:DebugShared" }
			if charset == "Unicode" then
				ignoredefaultlibraries { "mfc140ud.lib", "msvcrtd.lib" }
				links { "mfc140ud.lib", "msvcrtd.lib" }
			else
				ignoredefaultlibraries { "mfc140d.lib", "msvcrtd.lib" }
				links { "mfc140d.lib", "msvcrtd.lib" }
			end
		filter { "configurations:Checked" }
			if charset == "Unicode" then
				ignoredefaultlibraries { "uafxcw.lib", "libcmt.lib" }
				links { "uafxcw.lib", "libcmt.lib" }
			else
				ignoredefaultlibraries { "nafxcw.lib", "libcmt.lib" }
				links { "nafxcw.lib", "libcmt.lib" }
			end
		filter { "configurations:CheckedShared" }
			if charset == "Unicode" then
				ignoredefaultlibraries { "mfc140u.lib", "msvcrt.lib" }
				links { "mfc140u.lib", "msvcrt.lib" }
			else
				ignoredefaultlibraries { "mfc140.lib", "msvcrt.lib" }
				links { "mfc140.lib", "msvcrt.lib" }
			end
		filter { "configurations:Release" }
			if charset == "Unicode" then
				ignoredefaultlibraries { "uafxcw.lib", "libcmt.lib" }
				links { "uafxcw.lib", "libcmt.lib" }
			else
				ignoredefaultlibraries { "nafxcw.lib", "libcmt.lib" }
				links { "nafxcw.lib", "libcmt.lib" }
			end
		filter { "configurations:ReleaseShared" }
			if charset == "Unicode" then
				ignoredefaultlibraries { "mfc140u.lib", "msvcrt.lib" }
				links { "mfc140u.lib", "msvcrt.lib" }
			else
				ignoredefaultlibraries { "mfc140.lib", "msvcrt.lib" }
				links { "mfc140.lib", "msvcrt.lib" }
			end
		filter {}
	end
  links { "libopenmpt", "zlib", "vorbis", "ogg", "mpg123", "pugixml" }
  filter {}
  prebuildcommands { "..\\..\\build\\svn_version\\update_svn_version_vs_premake.cmd $(IntDir)" }
