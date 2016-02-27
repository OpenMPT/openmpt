
 project "xmp-openmpt"
  uuid "AEA14F53-ADB0-45E5-9823-81F4F36886C2"
  language "C++"
  location ( "../../build/" .. _ACTION )
  objdir "../../build/obj/xmp-openmpt"
  includedirs {
   "../..",
   "../../include",
   "../../include/pugixml/src",
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
  characterset "Unicode"
  flags { "MFC", "Unicode" }
  links { "libopenmpt", "miniz", "pugixml" }
  prebuildcommands { "..\\..\\build\\svn_version\\update_svn_version_vs_premake.cmd $(IntDir)" }
  dofile "../../build/premake/premake-defaults-DLL.lua"
  dofile "../../build/premake/premake-defaults.lua"
