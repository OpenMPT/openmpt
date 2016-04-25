  
 project "opusfile"
  uuid "f8517509-9317-4a46-b5ed-06ae5a399e6c"
  language "C"
  location ( "../../build/" .. _ACTION .. "-ext" )
  objdir "../../build/obj/opusfile"
  dofile "../../build/premake/premake-defaults-LIBorDLL.lua"
  dofile "../../build/premake/premake-defaults.lua"
  targetname "openmpt-opusfile"
  includedirs {
   "../../include/ogg/include",
   "../../include/opus/include",
   "../../include/opusfile/include",
  }
  characterset "MBCS"
  files {
   "../../include/opusfile/include/opusfile.h",
  }
  files {
   "../../include/opusfile/src/*.c",
   "../../include/opusfile/src/*.h",
  }
  links { "ogg", "opus" }
  filter {}
  filter { "kind:SharedLib" }
   files { "../../build/premake/def/ext-opusfile.def" }
  filter {}
