
  filter {}

  filter { "kind:StaticLib", "configurations:Debug", "architecture:x86" }
   targetdir "../../build/lib/x86/Debug"
  filter { "kind:StaticLib", "configurations:DebugShared", "architecture:x86" }
   targetdir "../../build/lib/x86/DebugShared"
  filter { "kind:StaticLib", "configurations:DebugMDd", "architecture:x86" }
   targetdir "../../build/lib/x86/DebugMDd"
  filter { "kind:StaticLib", "configurations:Release", "architecture:x86" }
   targetdir "../../build/lib/x86/Release"
  filter { "kind:StaticLib", "configurations:ReleaseShared", "architecture:x86" }
   targetdir "../../build/lib/x86/ReleaseShared"
  filter { "kind:StaticLib", "configurations:ReleaseLTCG", "architecture:x86" }
   targetdir "../../build/lib/x86/ReleaseLTCG"
  filter { "kind:StaticLib", "configurations:Debug", "architecture:x86_64" }
   targetdir "../../build/lib/x86_64/Debug"
  filter { "kind:StaticLib", "configurations:DebugShared", "architecture:x86_64" }
   targetdir "../../build/lib/x86_64/DebugShared"
  filter { "kind:StaticLib", "configurations:DebugMDd", "architecture:x86_64" }
   targetdir "../../build/lib/x86_64/DebugMDd"
  filter { "kind:StaticLib", "configurations:Release", "architecture:x86_64" }
   targetdir "../../build/lib/x86_64/Release"
  filter { "kind:StaticLib", "configurations:ReleaseShared", "architecture:x86_64" }
   targetdir "../../build/lib/x86_64/ReleaseShared"
  filter { "kind:StaticLib", "configurations:ReleaseLTCG", "architecture:x86_64" }
   targetdir "../../build/lib/x86_64/ReleaseLTCG"
  	
  filter { "kind:not StaticLib", "configurations:Debug", "architecture:x86" }
   targetdir "../../bin/Win32-Debug"
  filter { "kind:not StaticLib", "configurations:DebugShared", "architecture:x86" }
   targetdir "../../bin/Win32-Shared-Debug"
  filter { "kind:not StaticLib", "configurations:DebugMDd", "architecture:x86" }
   targetdir "../../bin/Win32-DebugMDd"
  filter { "kind:not StaticLib", "configurations:Release", "architecture:x86" }
   targetdir "../../bin/Win32"
  filter { "kind:not StaticLib", "configurations:ReleaseShared", "architecture:x86" }
   targetdir "../../bin/Win32-Shared"
  filter { "kind:not StaticLib", "configurations:ReleaseLTCG", "architecture:x86" }
   targetdir "../../bin/Win32"
  filter { "kind:not StaticLib", "configurations:Debug", "architecture:x86_64" }
   targetdir "../../bin/x64-Debug"
  filter { "kind:not StaticLib", "configurations:DebugShared", "architecture:x86_64" }
   targetdir "../../bin/x64-Shared-Debug"
  filter { "kind:not StaticLib", "configurations:DebugMDd", "architecture:x86_64" }
   targetdir "../../bin/x64-DebugMDd"
  filter { "kind:not StaticLib", "configurations:Release", "architecture:x86_64" }
   targetdir "../../bin/x64"
  filter { "kind:not StaticLib", "configurations:ReleaseShared", "architecture:x86_64" }
   targetdir "../../bin/x64-Shared"
  filter { "kind:not StaticLib", "configurations:ReleaseLTCG", "architecture:x86_64" }
   targetdir "../../bin/x64"

  filter { "action:vs2008" }
   includedirs { "../../include/msinttypes/stdint" }

  filter { "configurations:Debug" }
   defines { "DEBUG" }
   defines { "MPT_BUILD_MSVC_STATIC" }
   flags { "Symbols" }
   flags { "StaticRuntime" }
   optimize "Debug"

  filter { "configurations:DebugShared" }
   defines { "DEBUG" }
   defines { "MPT_BUILD_MSVC_SHARED" }
   flags { "Symbols" }
   optimize "Debug"

  filter { "configurations:DebugMDd" }
   defines { "DEBUG" }
   defines { "MPT_BUILD_MSVC_STATIC" }
   flags { "Symbols" }
   optimize "Debug"

  filter { "configurations:Release" }
   defines { "NDEBUG" }
   defines { "MPT_BUILD_MSVC_STATIC" }
   flags { "Symbols", "MultiProcessorCompile" }
   flags { "StaticRuntime" }
   optimize "Speed"
   floatingpoint "Fast"

  filter { "configurations:ReleaseShared" }
   defines { "NDEBUG" }
   defines { "MPT_BUILD_MSVC_SHARED" }
   flags { "Symbols", "MultiProcessorCompile" }
   optimize "Speed"
   floatingpoint "Fast"

  filter { "configurations:ReleaseLTCG" }
   defines { "NDEBUG" }
   defines { "MPT_BUILD_MSVC_STATIC" }
   flags { "Symbols", "MultiProcessorCompile", "LinkTimeOptimization" }
   flags { "StaticRuntime" }
   optimize "Full"
   floatingpoint "Fast"

  filter { "configurations:Release", "not action:vs2008" }
   vectorextensions "SSE2"

  filter { "configurations:ReleaseShared", "not action:vs2008" }
   vectorextensions "SSE2"

  filter { "configurations:ReleaseLTCG", "not action:vs2008" }
   vectorextensions "SSE2"

  filter {}
   defines { "WIN32", "_CRT_SECURE_NO_WARNINGS", "_CRT_NONSTDC_NO_DEPRECATE", "_CRT_SECURE_NO_DEPRECATE", "_CRT_NONSTDC_NO_WARNINGS" }

  filter {}
