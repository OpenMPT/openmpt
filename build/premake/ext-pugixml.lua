
 project "pugixml"
  uuid "07B89124-7C71-42cc-81AB-62B09BB61F9B"
  language "C++"
  location ( "%{wks.location}" .. "/ext" )
  mpt_kind "default"
  targetname "openmpt-pugixml"
  includedirs { }
	filter {}
	filter { "configurations:*Shared" }
		defines { "PUGIXML_API=__declspec(dllexport)" }
	filter { "not configurations:*Shared" }
	filter {}
  files {
   "../../include/pugixml/src/pugixml.cpp",
  }
  files {
   "../../include/pugixml/src/pugiconfig.hpp",
   "../../include/pugixml/src/pugixml.hpp",
  }
  filter { "action:vs*" }
    buildoptions { "/wd6054", "/wd28182" } -- /analyze
  filter {}

function mpt_use_pugixml ()
	filter {}
	filter { "action:vs*" }
		includedirs {
			"../../include/pugixml/src",
		}
	filter { "not action:vs*" }
		externalincludedirs {
			"../../include/pugixml/src",
		}
	filter {}
	filter { "configurations:*Shared" }
		defines { "PUGIXML_API=__declspec(dllimport)" }
	filter { "not configurations:*Shared" }
	filter {}
	links {
		"pugixml",
	}
	filter {}
end
