  
 project "opus"
  uuid "9a2d9099-e1a2-4287-b845-e3598ad24d70"
  language "C"
  location ( "../../build/" .. _ACTION .. "-ext" )
  objdir "../../build/obj/opus"
  dofile "../../build/premake/premake-defaults-LIBorDLL.lua"
  dofile "../../build/premake/premake-defaults.lua"
  targetname "openmpt-opus"
  includedirs {
   "../../include/ogg/include",
   "../../include/opus/include",
   "../../include/opus/celt",
   "../../include/opus/silk",
   "../../include/opus/silk/float",
   "../../include/opus/src",
   "../../include/opus/win32",
   "../../include/opus",
  }
  characterset "MBCS"
  files {
   "../../include/opus/include/opus.h",
   "../../include/opus/include/opus_custom.h",
   "../../include/opus/include/opus_defines.h",
   "../../include/opus/include/opus_multistream.h",
   "../../include/opus/include/opus_types.h",
  }
  files {
   "../../include/opus/celt/*.c",
   "../../include/opus/celt/*.h",
   "../../include/opus/celt/x86/*.c",
   "../../include/opus/celt/x86/*.h",
   "../../include/opus/silk/*.c",
   "../../include/opus/silk/*.h",
   "../../include/opus/silk/float/*.c",
   "../../include/opus/silk/float/*.h",
   "../../include/opus/silk/x86/*.c",
   "../../include/opus/silk/x86/*.h",
   "../../include/opus/src/*.c",
   "../../include/opus/src/*.h",
  }
  excludes {
   "../../include/opus/celt/opus_custom_demo.c",
   "../../include/opus/src/opus_compare.c",
   "../../include/opus/src/opus_demo.c",
   "../../include/opus/src/repacketizer_demo.c",
  }
  defines { "HAVE_CONFIG_H" }
  links { }
  buildoptions { "/wd4244" }
  filter { "kind:SharedLib" }
   defines { "DLL_EXPORT" }
  filter {}
