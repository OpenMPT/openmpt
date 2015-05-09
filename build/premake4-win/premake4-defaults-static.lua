
  configuration "Debug"
   defines { "DEBUG" }
   defines { "WIN32", "_CRT_SECURE_NO_WARNINGS", "_CRT_NONSTDC_NO_DEPRECATE", "_CRT_SECURE_NO_DEPRECATE", "_CRT_NONSTDC_NO_WARNINGS" }
if MPT_PREMAKE_VERSION == "4.3" then
   flags { "Symbols", "StaticRuntime" }
elseif MPT_PREMAKE_VERSION == "4.4" then
   flags { "Symbols", "StaticRuntime" }
elseif MPT_PREMAKE_VERSION == "5.0" then
   flags { "Symbols", "StaticRuntime" }
   optimize "Debug"
   floatingpoint "Default"
end

  configuration "Release"
   defines { "NDEBUG" }
   defines { "WIN32", "_CRT_SECURE_NO_WARNINGS", "_CRT_NONSTDC_NO_DEPRECATE", "_CRT_SECURE_NO_DEPRECATE", "_CRT_NONSTDC_NO_WARNINGS" }
if MPT_PREMAKE_VERSION == "4.3" then
   flags { "Symbols", "Optimize", "FloatFast", "StaticRuntime" }
   buildoptions { "/MP" }
elseif MPT_PREMAKE_VERSION == "4.4" then
   flags { "Symbols", "Optimize", "FloatFast", "StaticRuntime" }
   buildoptions { "/GL /MP" }
   linkoptions { "/LTCG" }
elseif MPT_PREMAKE_VERSION == "5.0" then
   flags { "Symbols", "LinkTimeOptimization", "MultiProcessorCompile", "StaticRuntime" }
   optimize "Full"
   floatingpoint "Fast"
end

  configuration "ReleaseNoLTCG"
   defines { "NDEBUG" }
   defines { "WIN32", "_CRT_SECURE_NO_WARNINGS", "_CRT_NONSTDC_NO_DEPRECATE", "_CRT_SECURE_NO_DEPRECATE", "_CRT_NONSTDC_NO_WARNINGS" }
if MPT_PREMAKE_VERSION == "4.3" then
   flags { "Optimize", "FloatFast", "StaticRuntime" }
   buildoptions { "/GL- /MP" }
elseif MPT_PREMAKE_VERSION == "4.4" then
   flags { "Optimize", "FloatFast", "StaticRuntime" }
   buildoptions { "/MP" }
elseif MPT_PREMAKE_VERSION == "5.0" then
   flags { "MultiProcessorCompile", "StaticRuntime" }
   optimize "Full"
   floatingpoint "Fast"
end
