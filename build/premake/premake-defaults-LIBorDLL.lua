
  filter {}

  filter { "configurations:Debug" }
   kind "StaticLib"
  filter { "configurations:DebugShared" }
   kind "SharedLib"
  filter { "configurations:Checked" }
   kind "StaticLib"
  filter { "configurations:CheckedShared" }
   kind "SharedLib"
  filter { "configurations:Release" }
   kind "StaticLib"
  filter { "configurations:ReleaseShared" }
   kind "SharedLib"

  filter {}
