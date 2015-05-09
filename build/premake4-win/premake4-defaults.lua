
if MPT_PREMAKE_VERSION == "5.0" then

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
   floatingpoint "Default"

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

else

  configuration "*"

  configuration "vs2008"
   includedirs { "../../include/msinttypes/stdint" }

  configuration "Debug"
   defines { "DEBUG" }
if MPT_PREMAKE_VERSION == "4.3" then
   flags { "Symbols" }
elseif MPT_PREMAKE_VERSION == "4.4" then
   flags { "Symbols" }
end

  configuration "Release"
   defines { "NDEBUG" }
if MPT_PREMAKE_VERSION == "4.3" then
   flags { "Symbols", "Optimize", "FloatFast" }
   buildoptions { "/MP" }
elseif MPT_PREMAKE_VERSION == "4.4" then
   flags { "Symbols", "Optimize", "FloatFast" }
   buildoptions { "/GL /MP" }
   linkoptions { "/LTCG" }
end

  configuration "ReleaseNoLTCG"
   defines { "NDEBUG" }
if MPT_PREMAKE_VERSION == "4.3" then
   flags { "Optimize", "FloatFast" }
   buildoptions { "/GL- /MP" }
elseif MPT_PREMAKE_VERSION == "4.4" then
   flags { "Optimize", "FloatFast" }
   buildoptions { "/MP" }
end

  configuration "*"
   defines { "WIN32", "_CRT_SECURE_NO_WARNINGS", "_CRT_NONSTDC_NO_DEPRECATE", "_CRT_SECURE_NO_DEPRECATE", "_CRT_NONSTDC_NO_WARNINGS" }

  configuration "*"

end
