
 project "minimp3"
  uuid "e88c4285-efb1-4226-bcac-e904ba792a48"
  language "C"
  location ( "../../build/" .. mpt_projectpathname .. "/ext" )
  mpt_projectname = "minimp3"
  dofile "../../build/premake/premake-defaults-LIBorDLL.lua"
  dofile "../../build/premake/premake-defaults.lua"
  targetname "openmpt-minimp3"
  includedirs { }
	filter {}
	filter { "action:vs*" }
		characterset "Unicode"
	filter {}
  defines { }
  files {
   "../../include/minimp3/minimp3.c",
   "../../include/minimp3/minimp3.h",
  }
  filter { "action:vs*", "kind:SharedLib" }
    files { "../../build/premake/def/ext-minimp3.def" }
  filter {}
