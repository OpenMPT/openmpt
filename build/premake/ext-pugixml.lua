
 project "pugixml"
  uuid "07B89124-7C71-42cc-81AB-62B09BB61F9B"
  language "C++"
  location ( "../../build/" .. _ACTION .. "-ext" )
  objdir "../../build/obj/pugixml"
  dofile "../../build/premake/premake-defaults-LIB.lua"
  dofile "../../build/premake/premake-defaults.lua"
  includedirs { }
  characterset "MBCS"
  files {
   "../../include/pugixml/src/pugixml.cpp",
  }
  files {
   "../../include/pugixml/src/pugiconfig.hpp",
   "../../include/pugixml/src/pugixml.hpp",
  }
