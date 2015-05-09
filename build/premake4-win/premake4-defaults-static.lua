
  configuration "Debug"
   defines { "DEBUG" }
   defines { "WIN32", "_CRT_SECURE_NO_WARNINGS", "_CRT_NONSTDC_NO_DEPRECATE", "_CRT_SECURE_NO_DEPRECATE", "_CRT_NONSTDC_NO_WARNINGS" }
   flags { "Symbols", "StaticRuntime" }
  
  configuration "Release"
   defines { "NDEBUG" }
   defines { "WIN32", "_CRT_SECURE_NO_WARNINGS", "_CRT_NONSTDC_NO_DEPRECATE", "_CRT_SECURE_NO_DEPRECATE", "_CRT_NONSTDC_NO_WARNINGS" }
   flags { "Symbols", "Optimize", "FloatFast", "StaticRuntime" }
if MPT_PREMAKE_VERSION == "4.3" then
   buildoptions { "/MP" }
elseif MPT_PREMAKE_VERSION == "4.4" then
   buildoptions { "/GL /MP" }
   linkoptions { "/LTCG" }
end

  configuration "ReleaseNoLTCG"
   defines { "NDEBUG" }
   defines { "WIN32", "_CRT_SECURE_NO_WARNINGS", "_CRT_NONSTDC_NO_DEPRECATE", "_CRT_SECURE_NO_DEPRECATE", "_CRT_NONSTDC_NO_WARNINGS" }
   flags { "Optimize", "FloatFast", "StaticRuntime" }
if MPT_PREMAKE_VERSION == "4.3" then
   buildoptions { "/GL- /MP" }
elseif MPT_PREMAKE_VERSION == "4.4" then
   buildoptions { "/MP" }
end
