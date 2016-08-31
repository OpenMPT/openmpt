
 project "libopenmpt_example_cxx"
  uuid "ce5b5a74-cdb1-4654-b928-f91725fb57c9"
  language "C++"
  location ( "../../build/" .. mpt_projectpathname )
  vpaths { ["*"] = "../../examples/" }
  mpt_projectname = "libopenmpt_example_cxx"
  dofile "../../build/premake/premake-defaults-EXE.lua"
  dofile "../../build/premake/premake-defaults.lua"
  local extincludedirs = {
   "../..",
   "../../include/portaudio/bindings/cpp/include",
   "../../include/portaudio/include",
  }
	filter { "action:vs*" }
		includedirs ( extincludedirs )
	filter { "not action:vs*" }
		sysincludedirs ( extincludedirs )
	filter {}
  includedirs {
   "../..",
  }
  files {
   "../../examples/libopenmpt_example_cxx.cpp",
  }
  characterset "Unicode"
  flags { "Unicode" }
  links { "libopenmpt", "zlib", "vorbis", "ogg", "portaudio", "portaudiocpp", "ksuser", "winmm" }
  filter { "not configurations:*Shared" }
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

 project "libopenmpt_example_c"
  uuid "3f39804d-01c0-479c-ab8b-025683529c57"
  language "C"
  location ( "../../build/" .. mpt_projectpathname )
  vpaths { ["*"] = "../../examples/" }
  mpt_projectname = "libopenmpt_example_c"
  dofile "../../build/premake/premake-defaults-EXE.lua"
  dofile "../../build/premake/premake-defaults.lua"
  local extincludedirs = {
   "../..",
   "../../include/portaudio/include",
  }
	filter { "action:vs*" }
		includedirs ( extincludedirs )
	filter { "not action:vs*" }
		sysincludedirs ( extincludedirs )
	filter {}
  includedirs {
   "../..",
  }
  files {
   "../../examples/libopenmpt_example_c.c",
  }
  characterset "Unicode"
  flags { "Unicode" }
  links { "libopenmpt", "zlib", "vorbis", "ogg", "portaudio", "ksuser", "winmm" }
  filter { "not configurations:*Shared" }
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
  location ( "../../build/" .. mpt_projectpathname )
  vpaths { ["*"] = "../../examples/" }
  mpt_projectname = "libopenmpt_example_c_mem"
  dofile "../../build/premake/premake-defaults-EXE.lua"
  dofile "../../build/premake/premake-defaults.lua"
  local extincludedirs = {
   "../..",
   "../../include/portaudio/include",
  }
	filter { "action:vs*" }
		includedirs ( extincludedirs )
	filter { "not action:vs*" }
		sysincludedirs ( extincludedirs )
	filter {}
  includedirs {
   "../..",
  }
  files {
   "../../examples/libopenmpt_example_c_mem.c",
  }
  characterset "Unicode"
  flags { "Unicode" }
  links { "libopenmpt", "zlib", "vorbis", "ogg", "portaudio", "ksuser", "winmm" }
  filter { "not configurations:*Shared" }
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
  location ( "../../build/" .. mpt_projectpathname )
  vpaths { ["*"] = "../../examples/" }
  mpt_projectname = "libopenmpt_example_c_unsafe"
  dofile "../../build/premake/premake-defaults-EXE.lua"
  dofile "../../build/premake/premake-defaults.lua"
  local extincludedirs = {
   "../..",
   "../../include/portaudio/include",
  }
	filter { "action:vs*" }
		includedirs ( extincludedirs )
	filter { "not action:vs*" }
		sysincludedirs ( extincludedirs )
	filter {}
  includedirs {
   "../..",
  }
  files {
   "../../examples/libopenmpt_example_c_unsafe.c",
  }
  characterset "Unicode"
  flags { "Unicode" }
  links { "libopenmpt", "zlib", "vorbis", "ogg", "portaudio", "ksuser", "winmm" }
  filter { "not configurations:*Shared" }
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

 project "libopenmpt_example_c_probe"
  uuid "3fbc000d-2574-4a02-96ba-db82d7e7d7bb"
  language "C"
  location ( "../../build/" .. mpt_projectpathname )
  vpaths { ["*"] = "../../examples/" }
  mpt_projectname = "libopenmpt_example_c_probe"
  dofile "../../build/premake/premake-defaults-EXE.lua"
  dofile "../../build/premake/premake-defaults.lua"
  local extincludedirs = {
   "../..",
   "../../include/portaudio/include",
  }
	filter { "action:vs*" }
		includedirs ( extincludedirs )
	filter { "not action:vs*" }
		sysincludedirs ( extincludedirs )
	filter {}
  includedirs {
   "../..",
  }
  files {
   "../../examples/libopenmpt_example_c_probe.c",
  }
  characterset "Unicode"
  flags { "Unicode" }
  links { "libopenmpt", "zlib", "vorbis", "ogg" }
  filter { "not configurations:*Shared" }
  links { "delayimp" }
  linkoptions {
  }
  filter { "configurations:*Shared" }
   defines { "LIBOPENMPT_USE_DLL" }
  filter {}

