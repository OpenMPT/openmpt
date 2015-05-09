 
 project "miniz"
  uuid "B5E0C06B-8121-426A-8FFB-4293ECAAE29C"
  language "C"
  location ( "../../build/" .. _ACTION .. "-ext" )
  objdir "../../build/obj/miniz"
  files {
   "../../include/miniz/miniz.c",
  }
  dofile "../../build/premake4-win/premake4-defaults-LIB.lua"
  dofile "../../build/premake4-win/premake4-defaults.lua"
  flags { "StaticRuntime" }
