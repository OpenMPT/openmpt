  
 project "opusfile"
  uuid "f8517509-9317-4a46-b5ed-06ae5a399e6c"
  language "C"
  location ( "../../build/" .. mpt_projectpathname .. "/ext" )
  objdir "../../build/obj/opusfile"
  dofile "../../build/premake/premake-defaults-LIBorDLL.lua"
  dofile "../../build/premake/premake-defaults.lua"
  dofile "../../build/premake/premake-defaults-winver.lua"
  targetname "openmpt-opusfile"
  local extincludedirs = {
   "../../include/ogg/include",
   "../../include/opus/include",
	}
	filter { "action:vs*" }
		includedirs ( extincludedirs )
	filter { "not action:vs*" }
		sysincludedirs ( extincludedirs )
	filter {}
  includedirs {
   "../../include/opusfile/include",
  }
	filter {}
	filter { "action:vs*", "not action:vs2008" }
		characterset "Unicode"
		flags { "Unicode" }
	filter { "action:vs*", "action:vs2008" }
		characterset "MBCS"
	filter {}
  files {
   "../../include/opusfile/include/opusfile.h",
  }
  files {
   "../../include/opusfile/src/*.c",
   "../../include/opusfile/src/*.h",
  }
  links { "ogg", "opus" }
  filter { "action:vs*" }
    buildoptions { "/wd4267" }
  filter {}
  filter { "kind:SharedLib" }
   files { "../../build/premake/def/ext-opusfile.def" }
  filter {}
