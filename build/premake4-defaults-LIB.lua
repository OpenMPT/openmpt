
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
