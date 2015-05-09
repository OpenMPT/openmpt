
 project "miniz-shared"
  uuid "224c35cd-12dc-4ebc-8183-b38a7199ad4b"
  language "C"
  location ( "../../build/" .. _ACTION .. "-ext" )
  objdir "../../build/obj/miniz-shared"
  files {
   "../../include/miniz/miniz.c",
  }
  dofile "../../build/premake4-win/premake4-defaults-LIB.lua"
  dofile "../../build/premake4-win/premake4-defaults.lua"
