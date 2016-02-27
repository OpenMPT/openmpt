
 project "portmidi"
  uuid "2512E2CA-578A-4F10-9363-4BFC9A5EF960"
  language "C"
  location ( "../../build/" .. _ACTION .. "-ext" )
  objdir "../../build/obj/portmidi"
  includedirs { "../../include/portmidi/porttime", "../../include/portmidi/pm_common", "../../include/portmidi/pm_win" }
  characterset "MBCS"
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
  dofile "../../build/premake/premake-defaults-LIB.lua"
  dofile "../../build/premake/premake-defaults.lua"
