
 project "pugixml"
  uuid "07B89124-7C71-42cc-81AB-62B09BB61F9B"
  language "C++"
  location ( "../../build/" .. mpt_projectpathname .. "/ext" )
  dofile "../../build/premake/premake-defaults-LIB.lua"
  targetname "openmpt-pugixml"
  includedirs { }
	filter {}
	filter { "action:vs*" }
		characterset "Unicode"
	filter {}
  files {
   "../../include/pugixml/src/pugixml.cpp",
  }
  files {
   "../../include/pugixml/src/pugiconfig.hpp",
   "../../include/pugixml/src/pugixml.hpp",
  }
  filter { "action:vs*" }
    buildoptions { "/wd6054", "/wd28182" } -- /analyze
  filter {}
