
  configuration { "Debug", "x32" }
   targetdir "../bin/Win32-Debug"
  
  configuration { "Release", "x32" }
   targetdir "../bin/Win32"
  
  configuration { "ReleaseNoLTCG", "x32" }
   targetdir "../bin/Win32"
  
  configuration { "Debug", "x64" }
   targetdir "../bin/x64-Debug"
  
  configuration { "Release", "x64" }
   targetdir "../bin/x64"
  
  configuration { "ReleaseNoLTCG", "x64" }
   targetdir "../bin/x64"
  
  configuration "*"
   kind "SharedLib"

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
   flags { "Optimize", "FloatFast", "StaticRuntime" }
   buildoptions { "/GL- /MP" }
