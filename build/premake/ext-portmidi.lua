
 project "portmidi"
  uuid "2512E2CA-578A-4F10-9363-4BFC9A5EF960"
  language "C"
  location ( "../../build/" .. mpt_projectpathname .. "/ext" )
  mpt_projectname = "portmidi"
  dofile "../../build/premake/premake-defaults-LIBorDLL.lua"
  dofile "../../build/premake/premake-defaults.lua"
  dofile "../../build/premake/premake-defaults-winver.lua"
  targetname "openmpt-portmidi"
  includedirs { "../../include/portmidi/porttime", "../../include/portmidi/pm_common", "../../include/portmidi/pm_win" }
	filter {}
	filter { "action:vs*" }
--		characterset "Unicode"
--		flags { "Unicode" }
		characterset "MBCS"
	filter {}
  files {
   "../../include/portmidi/porttime/porttime.c",
   "../../include/portmidi/porttime/ptwinmm.c",
   "../../include/portmidi/pm_common/pmutil.c",
   "../../include/portmidi/pm_common/portmidi.c",
   "../../include/portmidi/pm_win/pmwin.c",
   "../../include/portmidi/pm_win/pmwinmm.c",
  }
  files {
   "../../include/portmidi/porttime/porttime.h",
   "../../include/portmidi/pm_common/pminternal.h",
   "../../include/portmidi/pm_common/pmutil.h",
   "../../include/portmidi/pm_common/portmidi.h",
   "../../include/portmidi/pm_win/pmwinmm.h",
  }
  links {
   "winmm"
  }
  filter {}
  filter { "kind:StaticLib" }
  filter { "kind:SharedLib" }
   defines { "_WINDLL" }
  filter {}
