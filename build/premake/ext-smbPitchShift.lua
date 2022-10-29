
 project "smbPitchShift"
  uuid "89AF16DD-32CC-4A7E-B219-5F117D761F9F"
  language "C++"
  location ( "%{wks.location}" .. "/ext" )
  mpt_kind "default"
  targetname "openmpt-smbpitchshift"
  includedirs { }
	filter {}
  files {
   "../../include/smbPitchShift/smbPitchShift.cpp",
  }
  files {
   "../../include/smbPitchShift/smbPitchShift.h",
  }
  filter { "action:vs*" }
    buildoptions { "/wd4244" }
  filter {}
  filter { "kind:SharedLib" }
   defines { "SMBPITCHSHIFT_BUILD_DLL" }
  filter {}

function mpt_use_smbpitchshift ()
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
	filter { "configurations:*Shared" }
		defines { "SMBPITCHSHIFT_USE_DLL" }
	filter { "not configurations:*Shared" }
	filter {}
	links {
		"smbPitchShift",
	}
	filter {}
end
