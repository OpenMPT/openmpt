
 project "miniz-shared"
  uuid "224c35cd-12dc-4ebc-8183-b38a7199ad4b"
  language "C"
  location ( "../../build/" .. _ACTION .. "-ext" )
  objdir "../../build/obj/miniz-shared"
  dofile "../../build/premake/premake-defaults-LIB.lua"
  dofile "../../build/premake/premake-defaults.lua"
  characterset "MBCS"
  files {
   "../../include/miniz/miniz.c",
  }
  filter {}
   removeflags { "StaticRuntime" }
