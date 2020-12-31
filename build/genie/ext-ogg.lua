 
 project "ogg"
  uuid "d8d5e11c-f959-49ef-b741-b3f6de52ded8"
  language "C"
  location ( "../../build/" .. mpt_projectpathname .. "/ext" )
  mpt_projectname = "ogg"
  dofile "../../build/genie/genie-defaults-LIBorDLL.lua"
  dofile "../../build/genie/genie-defaults.lua"
  targetname "openmpt-ogg"
  includedirs { "../../include/ogg/include" }
  flags { "Unicode" }
  files {
   "../../include/ogg/include/ogg/ogg.h",
   "../../include/ogg/include/ogg/os_types.h",
   "../../include/ogg/src/bitwise.c",
   "../../include/ogg/src/framing.c",
  }
  configuration "DebugShared or ReleaseShared"
   files { "../../include/ogg/win32/ogg.def" }
  configuration {}
    buildoptions { "/wd6001", "/wd6011" } -- /analyze
