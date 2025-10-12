
 project "in_openmpt_wa2"
  uuid "9165eeec-0102-4f02-a8f5-3a771bf62ef6"
  language "C++"
  vpaths { ["*"] = "../../libopenmpt/" }
  mpt_kind "shared"
  warnings "Extra"
	
	mpt_use_libopenmpt()
	mpt_use_winamp()
	
  includedirs {
   "../..",
   "$(IntDir)/svn_version",
  }
  files {
   "../../libopenmpt/in_openmpt/in_openmpt.cpp",
   "../../libopenmpt/plugin-common/libopenmpt_plugin_settings.hpp",
   "../../libopenmpt/plugin-common/libopenmpt_plugin_gui.hpp",
   "../../libopenmpt/plugin-common/libopenmpt_plugin_gui.cpp",
   "../../libopenmpt/plugin-common/libopenmpt_plugin_gui.rc",
   "../../libopenmpt/plugin-common/resource.h",
  }

	filter {}
	filter { "action:vs*", "kind:SharedLib or ConsoleApp or WindowedApp" }
		resdefines {
			"MPT_BUILD_VER_FILENAME=\"" .. "in_openmpt_wa2" .. ".dll\"",
			"MPT_BUILD_VER_FILEDESC=\"" .. "in_openmpt_wa2" .. "\"",
		}
	filter {}
	filter { "action:vs*", "kind:SharedLib or ConsoleApp or WindowedApp" }
		resincludedirs {
			"$(IntDir)/svn_version",
			"../../build/svn_version",
			"$(ProjDir)/../../build/svn_version",
		}
		files {
			"../../libopenmpt/libopenmpt_version.rc",
		}
	filter {}
	filter { "action:vs*", "kind:SharedLib" }
		resdefines { "MPT_BUILD_VER_DLL" }
	filter {}
	filter { "action:vs*", "kind:ConsoleApp or WindowedApp" }
		resdefines { "MPT_BUILD_VER_EXE" }
	filter {}

	defines { "MPT_BUILD_IN_OPENMPT_WINAMP2" }
	mpt_use_mfc(_OPTIONS["windows-charset"])
	defines { "MPT_WITH_MFC" }
	if _OPTIONS["windows-charset"] ~= "Unicode" then
		defines { "NO_WARN_MBCS_MFC_DEPRECATION" }
	end

  filter {}
  prebuildcommands { "..\\..\\build\\svn_version\\update_svn_version_vs_premake.cmd $(IntDir)" }
