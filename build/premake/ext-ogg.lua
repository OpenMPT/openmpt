 
 project "ogg"
  uuid "d8d5e11c-f959-49ef-b741-b3f6de52ded8"
  language "C"
  location ( "../../build/" .. mpt_projectpathname .. "/ext" )
  objdir "../../build/obj/ogg"
  dofile "../../build/premake/premake-defaults-LIBorDLL.lua"
  dofile "../../build/premake/premake-defaults.lua"
  dofile "../../build/premake/premake-defaults-winver.lua"
  targetname "openmpt-ogg"
  includedirs { "../../include/ogg/include" }
  characterset "MBCS"
  files {
   "../../include/ogg/include/ogg/ogg.h",
   "../../include/ogg/include/ogg/os_types.h",
   "../../include/ogg/src/bitwise.c",
   "../../include/ogg/src/framing.c",
  }
  filter { "kind:SharedLib" }
   files { "../../include/ogg/win32/ogg.def" }
  filter {}
