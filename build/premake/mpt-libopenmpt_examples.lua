
if _ACTION ~= "vs2008" then

 project "libopenmpt_example_cxx"
  uuid "ce5b5a74-cdb1-4654-b928-f91725fb57c9"
  language "C++"
  location ( "../../build/" .. _ACTION )
  objdir "../../build/obj/libopenmpt_example_cxx"
  dofile "../../build/premake/premake-defaults-EXE.lua"
  dofile "../../build/premake/premake-defaults.lua"
  includedirs {
   "../..",
   "../../include/portaudio/bindings/cpp/include",
   "../../include/portaudio/include",
  }
  files {
   "../../examples/libopenmpt_example_cxx.cpp",
  }
  characterset "Unicode"
  flags { "Unicode" }
  links { "libopenmpt", "zlib", "vorbis", "ogg", "portaudio", "portaudiocpp", "ksuser", "winmm" }
  filter { "not configurations:*Shared", "not action:vs2008" }
  links { "delayimp" }
  linkoptions {
   "/DELAYLOAD:mf.dll",
   "/DELAYLOAD:mfplat.dll",
   "/DELAYLOAD:mfreadwrite.dll",
--   "/DELAYLOAD:mfuuid.dll", -- static library
   "/DELAYLOAD:propsys.dll",
  }
  filter { "configurations:*Shared" }
   defines { "LIBOPENMPT_USE_DLL" }
  filter {}

end

 project "libopenmpt_example_c"
  uuid "3f39804d-01c0-479c-ab8b-025683529c57"
  language "C"
  location ( "../../build/" .. _ACTION )
  objdir "../../build/obj/libopenmpt_example_c"
  dofile "../../build/premake/premake-defaults-EXE.lua"
  dofile "../../build/premake/premake-defaults.lua"
  includedirs {
   "../..",
   "../../include/portaudio/include",
  }
  files {
   "../../examples/libopenmpt_example_c.c",
  }
  characterset "Unicode"
  flags { "Unicode" }
  links { "libopenmpt", "zlib", "vorbis", "ogg", "portaudio", "ksuser", "winmm" }
  filter { "not configurations:*Shared", "not action:vs2008" }
  links { "delayimp" }
  linkoptions {
   "/DELAYLOAD:mf.dll",
   "/DELAYLOAD:mfplat.dll",
   "/DELAYLOAD:mfreadwrite.dll",
--   "/DELAYLOAD:mfuuid.dll", -- static library
   "/DELAYLOAD:propsys.dll",
  }
  filter { "configurations:*Shared" }
   defines { "LIBOPENMPT_USE_DLL" }
  filter {}

 project "libopenmpt_example_c_mem"
  uuid "4db3da91-fafd-43af-b3b7-35699b80aba1"
  language "C"
  location ( "../../build/" .. _ACTION )
  objdir "../../build/obj/libopenmpt_example_c_mem"
  dofile "../../build/premake/premake-defaults-EXE.lua"
  dofile "../../build/premake/premake-defaults.lua"
  includedirs {
   "../..",
   "../../include/portaudio/include",
  }
  files {
   "../../examples/libopenmpt_example_c_mem.c",
  }
  characterset "Unicode"
  flags { "Unicode" }
  links { "libopenmpt", "zlib", "vorbis", "ogg", "portaudio", "ksuser", "winmm" }
  filter { "not configurations:*Shared", "not action:vs2008" }
  links { "delayimp" }
  linkoptions {
   "/DELAYLOAD:mf.dll",
   "/DELAYLOAD:mfplat.dll",
   "/DELAYLOAD:mfreadwrite.dll",
--   "/DELAYLOAD:mfuuid.dll", -- static library
   "/DELAYLOAD:propsys.dll",
  }
  filter { "configurations:*Shared" }
   defines { "LIBOPENMPT_USE_DLL" }
  filter {}

 project "libopenmpt_example_c_unsafe"
  uuid "696a79ac-65eb-445f-981a-7639c54569f8"
  language "C"
  location ( "../../build/" .. _ACTION )
  objdir "../../build/obj/libopenmpt_example_c_unsafe"
  dofile "../../build/premake/premake-defaults-EXE.lua"
  dofile "../../build/premake/premake-defaults.lua"
  includedirs {
   "../..",
   "../../include/portaudio/include",
  }
  files {
   "../../examples/libopenmpt_example_c_unsafe.c",
  }
  characterset "Unicode"
  flags { "Unicode" }
  links { "libopenmpt", "zlib", "vorbis", "ogg", "portaudio", "ksuser", "winmm" }
  filter { "not configurations:*Shared", "not action:vs2008" }
  links { "delayimp" }
  linkoptions {
   "/DELAYLOAD:mf.dll",
   "/DELAYLOAD:mfplat.dll",
   "/DELAYLOAD:mfreadwrite.dll",
--   "/DELAYLOAD:mfuuid.dll", -- static library
   "/DELAYLOAD:propsys.dll",
  }
  filter { "configurations:*Shared" }
   defines { "LIBOPENMPT_USE_DLL" }
  filter {}

