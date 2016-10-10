
 project "stb_vorbis"
  uuid "E0D81662-85EF-4172-B0D8-F8DCFF712607"
  language "C"
  location ( "../../build/" .. mpt_projectpathname .. "/ext" )
  mpt_projectname = "stb_vorbis"
  dofile "../../build/premake/premake-defaults-LIB.lua"
  dofile "../../build/premake/premake-defaults.lua"
  dofile "../../build/premake/premake-defaults-winver.lua"
  targetname "openmpt-stb_vorbis"
  includedirs { }
	filter {}
	filter { "action:vs*" }
		characterset "Unicode"
	filter {}
  defines { "STB_VORBIS_NO_PULLDATA_API", "STB_VORBIS_NO_STDIO" }
  files {
   "../../include/stb_vorbis/stb_vorbis.c",
  }
  filter { "action:vs*" }
    buildoptions { "/wd4005", "/wd4100", "/wd4244", "/wd4245", "/wd4701" }
  filter {}
