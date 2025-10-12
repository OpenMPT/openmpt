 
 project "miniz"
  uuid "B5E0C06B-8121-426A-8FFB-4293ECAAE29C"
  language "C"
  location ( "%{wks.location}" .. "/ext" )
  mpt_kind "default"
  targetname "openmpt-miniz"
	filter {}
  files {
   "../../include/miniz/miniz.c",
   "../../include/miniz/miniz.h",
  }

	filter {}
	if MPT_COMPILER_MSVC or MPT_COMPILER_CLANGCL then
		buildoptions { "/wd4244" }
	end
	filter {}
	if MPT_OS_WINDOWS then
		filter {}
		filter { "kind:SharedLib" }
			defines { "MINIZ_EXPORT=__declspec(dllexport)" }
		filter {}
	end
	filter {}
	defines {
		"MINIZ_NO_STDIO",
	}
	filter {}

function mpt_use_miniz ()
	filter {}
	dependencyincludedirs {
		"../../include",
	}
	filter {}
	defines {
		"MINIZ_NO_STDIO",
	}
	filter {}
	links {
		"miniz",
	}
	filter {}
end
