
  configuration "x64"
   targetsuffix "64"
  
  configuration "DebugLib"
   targetdir "bin/DebugLib"
  
  configuration "DebugDLL"
   targetdir "../mptrack/Debug"
  
  configuration "NormalLib"
   targetdir "bin/NormalLib"
  
  configuration "NormalDLL"
   targetdir "../mptrack/bin"
  
  configuration "ReleaseLib"
   targetdir "bin/ReleaseLib"
  
  configuration "ReleaseDLL"
   targetdir "../mptrack/bin"
  
  configuration "*Lib"
   kind "StaticLib"
   
  configuration "*DLL"
   kind "SharedLib"

  configuration "Debug*"
   defines { "DEBUG" }
   defines { "WIN32", "_CRT_SECURE_NO_WARNINGS", "_CRT_NONSTDC_NO_DEPRECATE", "_CRT_SECURE_NO_DEPRECATE", "_CRT_NONSTDC_NO_WARNINGS" }
   flags { "Symbols" }
  
  configuration "Normal*"
   defines { "NDEBUG" }
   defines { "WIN32", "_CRT_SECURE_NO_WARNINGS", "_CRT_NONSTDC_NO_DEPRECATE", "_CRT_SECURE_NO_DEPRECATE", "_CRT_NONSTDC_NO_WARNINGS" }
   flags { "Symbols", "Optimize", "FloatFast", "StaticRuntime" }
   buildoptions { "/GL- /MP" }

  configuration "Release*"
   defines { "NDEBUG" }
   defines { "WIN32", "_CRT_SECURE_NO_WARNINGS", "_CRT_NONSTDC_NO_DEPRECATE", "_CRT_SECURE_NO_DEPRECATE", "_CRT_NONSTDC_NO_WARNINGS" }
   flags { "Symbols", "Optimize", "FloatFast", "StaticRuntime" }
   buildoptions { "/MP" }
