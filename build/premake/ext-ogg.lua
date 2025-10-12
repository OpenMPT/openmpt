 
 project "ogg"
  uuid "d8d5e11c-f959-49ef-b741-b3f6de52ded8"
  language "C"
  location ( "%{wks.location}" .. "/ext" )
  mpt_kind "default"
  targetname "openmpt-ogg"
  includedirs { "../../include/ogg/include" }
	filter {}
  files {
   "../../include/ogg/include/ogg/ogg.h",
   "../../include/ogg/include/ogg/os_types.h",
   "../../include/ogg/src/bitwise.c",
   "../../include/ogg/src/framing.c",
   "../../include/ogg/src/crctable.h",
  }
  filter { "kind:SharedLib" }
   files { "../../include/ogg/win32/ogg.def" }
	filter {}
	if MPT_COMPILER_MSVC or MPT_COMPILER_CLANGCL then
		buildoptions { "/wd6001", "/wd6011" } -- /analyze
	end
	filter {}

function mpt_use_ogg ()
	filter {}
	dependencyincludedirs {
		"../../include/ogg/include",
	}
	filter {}
	links {
		"ogg",
	}
	filter {}
end
