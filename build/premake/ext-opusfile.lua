
include_dependency "ext-ogg.lua"
include_dependency "ext-opus.lua"

 project "opusfile"
  uuid "f8517509-9317-4a46-b5ed-06ae5a399e6c"
  language "C"
  location ( "%{wks.location}" .. "/ext" )
  mpt_kind "default"
  targetname "openmpt-opusfile"
	
	mpt_use_ogg()
	mpt_use_opus()
	
  includedirs {
   "../../include/opusfile/include",
  }
	filter {}
  files {
   "../../include/opusfile/include/opusfile.h",
  }
  files {
   "../../include/opusfile/src/*.c",
   "../../include/opusfile/src/*.h",
  }
	filter {}
	if MPT_COMPILER_MSVC or MPT_COMPILER_CLANGCL then
		buildoptions { "/wd4267" }
	end
	filter {}
	if MPT_COMPILER_CLANGCL or MPT_COMPILER_CLANG then
		buildoptions {
			"-Wno-bitwise-op-parentheses",
			"-Wno-logical-op-parentheses",
			"-Wno-shift-op-parentheses",
			"-Wno-unused-but-set-variable",
			"-Wno-unused-const-variable",
		}
	end
	filter {}
  filter { "kind:SharedLib" }
   files { "../../build/premake/def/ext-opusfile.def" }
  filter {}

function mpt_use_opusfile ()
	filter {}
	dependencyincludedirs {
		"../../include/opusfile/include",
	}
	filter {}
	links {
		"opusfile",
	}
	filter {}
end
