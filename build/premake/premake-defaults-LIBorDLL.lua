
  filter {}

  filter { "configurations:Debug" }
   kind "StaticLib"
  filter { "configurations:DebugShared" }
   kind "SharedLib"
  filter { "configurations:DebugMDd" }
   kind "StaticLib"
  filter { "configurations:Release" }
   kind "StaticLib"
  filter { "configurations:ReleaseShared" }
   kind "SharedLib"
  filter { "configurations:ReleaseLTCG" }
   kind "StaticLib"

  filter {}
