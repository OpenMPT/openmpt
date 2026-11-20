
 project "pitch-detection"
  uuid "2D7BC184-1145-4C3F-B864-D30F4D1AEB40"
  language "C++"
  location ( "%{wks.location}" .. "/ext" )
  mpt_kind "static"
  targetname "openmpt-pitch-detection"

  includedirs { "../../include/pitch-detection/include" }
  filter {}
  if MPT_COMPILER_MSVC or MPT_COMPILER_CLANGCL then
    buildoptions {
      "/wd4244",
    }
  end
  filter {}
  files {
    "../../include/pitch-detection/include/pitch_detection.h",
  }
  files {
    "../../include/pitch-detection/src/autocorrelation.cpp",
    "../../include/pitch-detection/src/mpm.cpp",
    "../../include/pitch-detection/src/parabolic_interpolation.cpp",
  }
  filter {}

function mpt_use_pitch_detection ()
  filter {}
  dependencyincludedirs {
    "../../include/pitch-detection/include",
  }
  filter {}
  links {
    "pitch-detection",
  }
  filter {}
end
