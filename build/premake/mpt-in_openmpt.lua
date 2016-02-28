
 project "in_openmpt"
  uuid "D75AEB78-5537-49BD-9085-F92DEEFA84E8"
  language "C++"
  location ( "../../build/" .. _ACTION )
  objdir "../../build/obj/in_openmpt"
  includedirs {
   "../..",
   "../../include",
   "$(IntDir)/svn_version",
   "../../build/svn_version",
  }
  files {
   "../../libopenmpt/in_openmpt.cpp",
   "../../libopenmpt/libopenmpt_plugin_settings.hpp",
   "../../libopenmpt/libopenmpt_plugin_gui.hpp",
   "../../libopenmpt/libopenmpt_plugin_gui.cpp",
   "../../libopenmpt/libopenmpt_plugin_gui.rc",
   "../../libopenmpt/resource.h",
  }
  characterset "Unicode"
  flags { "MFC", "Unicode" }
  links { "libopenmpt" }
  filter { "not action:vs2008" }
  linkoptions {
   "/DELAYLOAD:mf.dll",
   "/DELAYLOAD:mfplat.dll",
   "/DELAYLOAD:mfreadwrite.dll",
--   "/DELAYLOAD:mfuuid.dll", -- static library
   "/DELAYLOAD:propsys.dll",
  }
  filter {}
  prebuildcommands { "..\\..\\build\\svn_version\\update_svn_version_vs_premake.cmd $(IntDir)" }
  dofile "../../build/premake/premake-defaults-DLL.lua"
  dofile "../../build/premake/premake-defaults.lua"
