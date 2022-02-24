 
 project "miniz"
  uuid "B5E0C06B-8121-426A-8FFB-4293ECAAE29C"
  language "C"
  location ( "../../build/" .. mpt_projectpathname .. "/ext" )
  dofile "../../build/premake/premake-defaults-LIBorDLL.lua"
  targetname "openmpt-miniz"
	filter {}
	filter { "action:vs*" }
		characterset "Unicode"
	filter {}
  files {
   "../../include/miniz/miniz.c",
   "../../include/miniz/miniz.h",
  }
  filter { "action:vs*" }
    buildoptions { "/wd4244" }
	filter {}

	filter { "kind:SharedLib" }
		defines { "MINIZ_EXPORT=__declspec(dllexport)" }
	filter {}
