
 project "stb_vorbis-shared"
  uuid "79948696-448c-4178-ac96-e27487654b68"
  language "C"
  location ( "../../build/" .. _ACTION .. "-ext" )
  objdir "../../build/obj/stb_vorbis-shared"
  includedirs { }
  characterset "MBCS"
  defines { "STB_VORBIS_NO_PULLDATA_API", "STB_VORBIS_NO_STDIO" }
  files {
   "../../include/stb_vorbis/stb_vorbis.c",
  }
  buildoptions { "/wd4005", "/wd4100", "/wd4244", "/wd4245", "/wd4701" }
  dofile "../../build/premake/premake-defaults-LIB.lua"
  dofile "../../build/premake/premake-defaults.lua"
  filter {}
   removeflags { "StaticRuntime" }
