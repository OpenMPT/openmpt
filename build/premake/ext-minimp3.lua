
 project "minimp3"
  uuid "e88c4285-efb1-4226-bcac-e904ba792a48"
  language "C"
  location ( "%{wks.location}" .. "/ext" )
  mpt_kind "default"
  targetname "openmpt-minimp3"
  includedirs { }
	filter {}
  defines { }
  files {
   "../../include/minimp3/minimp3.c",
   "../../include/minimp3/minimp3.h",
  }
  filter { "action:vs*", "kind:SharedLib" }
    files { "../../build/premake/def/ext-minimp3.def" }
  filter {}

function mpt_use_minimp3 ()
	filter {}
	filter { "action:vs*" }
		includedirs {
			"../../include",
		}
	filter { "not action:vs*" }
		externalincludedirs {
			"../../include",
		}
	filter {}
	links {
		"minimp3",
	}
	filter {}
end
