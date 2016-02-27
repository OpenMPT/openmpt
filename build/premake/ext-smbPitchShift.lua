
 project "smbPitchShift"
  uuid "89AF16DD-32CC-4A7E-B219-5F117D761F9F"
  language "C++"
  location ( "../../build/" .. _ACTION .. "-ext" )
  objdir "../../build/obj/smbPitchShift"
  includedirs { }
  characterset "MBCS"
  files {
   "../../include/smbPitchShift/smbPitchShift.cpp",
  }
  files {
   "../../include/smbPitchShift/smbPitchShift.h",
  }
  dofile "../../build/premake/premake-defaults-LIB.lua"
  dofile "../../build/premake/premake-defaults.lua"
