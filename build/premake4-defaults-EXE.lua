
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
   kind "ConsoleApp"
