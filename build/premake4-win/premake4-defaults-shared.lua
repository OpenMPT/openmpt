
  configuration "Debug"
   defines { "DEBUG" }
   defines { "WIN32", "_CRT_SECURE_NO_WARNINGS", "_CRT_NONSTDC_NO_DEPRECATE", "_CRT_SECURE_NO_DEPRECATE", "_CRT_NONSTDC_NO_WARNINGS" }
if MPT_PREMAKE_VERSION == "4.3" then
   flags { "Symbols" }
elseif MPT_PREMAKE_VERSION == "4.4" then
   flags { "Symbols" }
elseif MPT_PREMAKE_VERSION == "5.0" then
   flags { "Symbols" }
   optimize "Debug"
   floatingpoint "Default"
end

  configuration "Release"
   defines { "NDEBUG" }
   defines { "WIN32", "_CRT_SECURE_NO_WARNINGS", "_CRT_NONSTDC_NO_DEPRECATE", "_CRT_SECURE_NO_DEPRECATE", "_CRT_NONSTDC_NO_WARNINGS" }
if MPT_PREMAKE_VERSION == "4.3" then
   flags { "Symbols", "Optimize", "FloatFast" }
   buildoptions { "/MP" }
elseif MPT_PREMAKE_VERSION == "4.4" then
   flags { "Symbols", "Optimize", "FloatFast" }
   buildoptions { "/GL /MP" }
   linkoptions { "/LTCG" }
elseif MPT_PREMAKE_VERSION == "5.0" then
   flags { "Symbols", "LinkTimeOptimization", "MultiProcessorCompile" }
   optimize "Full"
   floatingpoint "Fast"
end

  configuration "ReleaseNoLTCG"
   defines { "NDEBUG" }
   defines { "WIN32", "_CRT_SECURE_NO_WARNINGS", "_CRT_NONSTDC_NO_DEPRECATE", "_CRT_SECURE_NO_DEPRECATE", "_CRT_NONSTDC_NO_WARNINGS" }
if MPT_PREMAKE_VERSION == "4.3" then
   flags { "Optimize", "FloatFast" }
   buildoptions { "/GL- /MP" }
elseif MPT_PREMAKE_VERSION == "4.4" then
   flags { "Optimize", "FloatFast" }
   buildoptions { "/MP" }
elseif MPT_PREMAKE_VERSION == "5.0" then
   flags { "MultiProcessorCompile" }
   optimize "Full"
   floatingpoint "Fast"
end
