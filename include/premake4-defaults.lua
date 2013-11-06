
  configuration { "Debug", "x32" }
   targetdir "../build/lib/x32/Debug"
  
  configuration { "Release", "x32" }
   targetdir "../build/lib/x32/Release"
  
  configuration { "ReleaseNoLTCG", "x32" }
   targetdir "../build/lib/x32/ReleaseNoLTCG"
  
  configuration { "Debug", "x64" }
   targetdir "../build/lib/x64/Debug"
  
  configuration { "Release", "x64" }
   targetdir "../build/lib/x64/Release"
  
  configuration { "ReleaseNoLTCG", "x64" }
   targetdir "../build/lib/x64/ReleaseNoLTCG"
  
  configuration "*"
   kind "StaticLib"
   
  configuration "Debug"
   defines { "DEBUG" }
   defines { "WIN32", "_CRT_SECURE_NO_WARNINGS", "_CRT_NONSTDC_NO_DEPRECATE", "_CRT_SECURE_NO_DEPRECATE", "_CRT_NONSTDC_NO_WARNINGS" }
   flags { "Symbols" }
  
  configuration "Release"
   defines { "NDEBUG" }
   defines { "WIN32", "_CRT_SECURE_NO_WARNINGS", "_CRT_NONSTDC_NO_DEPRECATE", "_CRT_SECURE_NO_DEPRECATE", "_CRT_NONSTDC_NO_WARNINGS" }
   flags { "Symbols", "Optimize", "FloatFast", "StaticRuntime" }
   buildoptions { "/MP" }

  configuration "ReleaseNoLTCG"
   defines { "NDEBUG" }
   defines { "WIN32", "_CRT_SECURE_NO_WARNINGS", "_CRT_NONSTDC_NO_DEPRECATE", "_CRT_SECURE_NO_DEPRECATE", "_CRT_NONSTDC_NO_WARNINGS" }
   flags { "Symbols", "Optimize", "FloatFast", "StaticRuntime" }
   buildoptions { "/GL- /MP" }
