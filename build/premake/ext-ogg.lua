 
 project "ogg"
  uuid "d8d5e11c-f959-49ef-b741-b3f6de52ded8"
  language "C"
  location ( "../../build/" .. _ACTION .. "-ext" )
  objdir "../../build/obj/ogg"
  dofile "../../build/premake/premake-defaults-LIB.lua"
  dofile "../../build/premake/premake-defaults.lua"
  includedirs { "../../include/ogg/include" }
  characterset "MBCS"
  files {
   "../../include/ogg/include/ogg/ogg.h",
   "../../include/ogg/include/ogg/os_types.h",
   "../../include/ogg/src/bitwise.c",
   "../../include/ogg/src/framing.c",
  }
