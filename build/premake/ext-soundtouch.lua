 
 project "soundtouch"
  uuid "F5F8F6DE-84CF-4E9D-91EA-D9B5E2AA36CD"
  language "C++"
  location ( "../../build/" .. mpt_projectpathname .. "/ext" )
  objdir "../../build/obj/soundtouch"
  dofile "../../build/premake/premake-defaults-DLL.lua"
  dofile "../../build/premake/premake-defaults.lua"
  dofile "../../build/premake/premake-defaults-winver.lua"
  filter { "configurations:*Shared" }
   targetname "openmpt-soundtouch"
  filter { "not configurations:*Shared" }
   targetname "OpenMPT_SoundTouch_f32"
  filter {}
  includedirs { "../../include/soundtouch/include" }
	filter {}
	filter { "action:vs*", "not action:vs2008" }
		characterset "Unicode"
		flags { "Unicode" }
	filter { "action:vs*", "action:vs2008" }
		characterset "MBCS"
	filter {}
  files {
   "../../include/soundtouch/include/BPMDetect.h",
   "../../include/soundtouch/include/FIFOSampleBuffer.h",
   "../../include/soundtouch/include/FIFOSamplePipe.h",
   "../../include/soundtouch/include/SoundTouch.h",
   "../../include/soundtouch/include/STTypes.h",
  }
  files {
   "../../include/soundtouch/source/SoundTouch/AAFilter.cpp",
   "../../include/soundtouch/source/SoundTouch/BPMDetect.cpp",
   "../../include/soundtouch/source/SoundTouch/cpu_detect_x86.cpp",
   "../../include/soundtouch/source/SoundTouch/FIFOSampleBuffer.cpp",
   "../../include/soundtouch/source/SoundTouch/FIRFilter.cpp",
   "../../include/soundtouch/source/SoundTouch/InterpolateCubic.cpp",
   "../../include/soundtouch/source/SoundTouch/InterpolateLinear.cpp",
   "../../include/soundtouch/source/SoundTouch/InterpolateShannon.cpp",
   "../../include/soundtouch/source/SoundTouch/mmx_optimized.cpp",
   "../../include/soundtouch/source/SoundTouch/PeakFinder.cpp",
   "../../include/soundtouch/source/SoundTouch/RateTransposer.cpp",
   "../../include/soundtouch/source/SoundTouch/SoundTouch.cpp",
   "../../include/soundtouch/source/SoundTouch/sse_optimized.cpp",
   "../../include/soundtouch/source/SoundTouch/TDStretch.cpp",
  }
  files {
   "../../include/soundtouch/source/SoundTouch/AAFilter.h",
   "../../include/soundtouch/source/SoundTouch/cpu_detect.h",
   "../../include/soundtouch/source/SoundTouch/FIRFilter.h",
   "../../include/soundtouch/source/SoundTouch/InterpolateCubic.h",
   "../../include/soundtouch/source/SoundTouch/InterpolateLinear.h",
   "../../include/soundtouch/source/SoundTouch/InterpolateShannon.h",
   "../../include/soundtouch/source/SoundTouch/PeakFinder.h",
   "../../include/soundtouch/source/SoundTouch/RateTransposer.h",
   "../../include/soundtouch/source/SoundTouch/TDStretch.h",
  }
  files {
   "../../include/soundtouch/source/SoundTouchDLL/SoundTouchDLL.cpp",
   "../../include/soundtouch/source/SoundTouchDLL/SoundTouchDLL.h",
  }
  defines { "DLL_EXPORTS" }
