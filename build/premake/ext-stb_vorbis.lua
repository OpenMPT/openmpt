
 project "stb_vorbis"
  uuid "E0D81662-85EF-4172-B0D8-F8DCFF712607"
  language "C"
  location ( "%{wks.location}" .. "/ext" )
  mpt_kind "default"
  targetname "openmpt-stb_vorbis"
  includedirs { }
	filter {}
  defines { "STB_VORBIS_NO_PULLDATA_API", "STB_VORBIS_NO_STDIO" }
  files {
   "../../include/stb_vorbis/stb_vorbis.c",
  }
  filter { "action:vs*" }
    buildoptions { "/wd4005", "/wd4100", "/wd4244", "/wd4245", "/wd4701" }
  filter {}
  filter { "kind:SharedLib" }
   files { "../../build/premake/def/ext-stb_vorbis.def" }
  filter {}

function mpt_use_stbvorbis ()
	filter {}
	dependencyincludedirs {
		"../../include",
	}
	filter {}
	defines {
		"STB_VORBIS_HEADER_ONLY",
		"STB_VORBIS_NO_PULLDATA_API",
		"STB_VORBIS_NO_STDIO",
	}
	links {
		"stb_vorbis",
	}
	filter {}
end
