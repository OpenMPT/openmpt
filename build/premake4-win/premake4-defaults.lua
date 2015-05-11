
  filter {}

  filter { "kind:StaticLib", "configurations:Debug", "architecture:x86" }
   targetdir "../../build/lib/x86/Debug"
  filter { "kind:StaticLib", "configurations:Release", "architecture:x86" }
   targetdir "../../build/lib/x86/Release"
  filter { "kind:StaticLib", "configurations:ReleaseNoLTCG", "architecture:x86" }
   targetdir "../../build/lib/x86/ReleaseNoLTCG"
  filter { "kind:StaticLib", "configurations:Debug", "architecture:x86_64" }
   targetdir "../../build/lib/x86_64/Debug"
  filter { "kind:StaticLib", "configurations:Release", "architecture:x86_64" }
   targetdir "../../build/lib/x86_64/Release"
  filter { "kind:StaticLib", "configurations:ReleaseNoLTCG", "architecture:x86_64" }
   targetdir "../../build/lib/x86_64/ReleaseNoLTCG"
  	
  filter { "kind:not StaticLib", "configurations:Debug", "architecture:x86" }
   targetdir "../../bin/Win32-Debug"
  filter { "kind:not StaticLib", "configurations:Release", "architecture:x86" }
   targetdir "../../bin/Win32"
  filter { "kind:not StaticLib", "configurations:ReleaseNoLTCG", "architecture:x86" }
   targetdir "../../bin/Win32"
  filter { "kind:not StaticLib", "configurations:Debug", "architecture:x86_64" }
   targetdir "../../bin/x64-Debug"
  filter { "kind:not StaticLib", "configurations:Release", "architecture:x86_64" }
   targetdir "../../bin/x64"
  filter { "kind:not StaticLib", "configurations:ReleaseNoLTCG", "architecture:x86_64" }
   targetdir "../../bin/x64"

  filter { "action:vs2008" }
   includedirs { "../../include/msinttypes/stdint" }

  filter { "configurations:Debug" }
	 configuration "Debug"
   defines { "DEBUG" }
   flags { "Symbols" }
   optimize "Debug"

  filter { "configurations:Release" }
  configuration "Release"
   defines { "NDEBUG" }
   flags { "Symbols", "LinkTimeOptimization", "MultiProcessorCompile" }
   optimize "Full"
   floatingpoint "Fast"

  filter { "configurations:ReleaseNoLTCG" }
   defines { "NDEBUG" }
   flags { "MultiProcessorCompile" }
   optimize "Full"
   floatingpoint "Fast"

  filter {}
   defines { "WIN32", "_CRT_SECURE_NO_WARNINGS", "_CRT_NONSTDC_NO_DEPRECATE", "_CRT_SECURE_NO_DEPRECATE", "_CRT_NONSTDC_NO_WARNINGS" }

  filter {}
