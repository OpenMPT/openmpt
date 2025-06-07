
function mpt_libopenmpt_rc (name)
	filter {}
	filter { "action:vs*", "kind:SharedLib or ConsoleApp or WindowedApp" }
		resdefines {
			"MPT_BUILD_VER_FILENAME=\"" .. name .. ".dll\"",
			"MPT_BUILD_VER_FILEDESC=\"" .. name .. "\"",
		}
	filter {}
	filter { "action:vs*", "kind:SharedLib or ConsoleApp or WindowedApp" }
		resincludedirs {
			"$(IntDir)/svn_version",
			"../../build/svn_version",
			"$(ProjDir)/../../build/svn_version",
		}
		files {
			"../../libopenmpt/libopenmpt_version.rc",
		}
	filter {}
	filter { "action:vs*", "kind:SharedLib" }
		resdefines { "MPT_BUILD_VER_DLL" }
	filter {}
	filter { "action:vs*", "kind:ConsoleApp or WindowedApp" }
		resdefines { "MPT_BUILD_VER_EXE" }
	filter {}
end

 project "libopenmpt_example_cxx"
  uuid "ce5b5a74-cdb1-4654-b928-f91725fb57c9"
  language "C++"
  vpaths { ["*"] = "../../examples/" }
  mpt_kind "Console"
  warnings "Extra"
	mpt_use_libopenmpt()
	mpt_use_portaudio()
	mpt_use_portaudiocpp()
  files {
   "../../examples/libopenmpt_example_cxx.cpp",
  }
  mpt_libopenmpt_rc("libopenmpt_example_cxx");
  links { "ksuser", "winmm" }

 project "libopenmpt_example_c"
  uuid "3f39804d-01c0-479c-ab8b-025683529c57"
  language "C"
  vpaths { ["*"] = "../../examples/" }
  mpt_kind "Console"
  warnings "Extra"
	mpt_use_libopenmpt()
	mpt_use_portaudio()
  files {
   "../../examples/libopenmpt_example_c.c",
  }
  mpt_libopenmpt_rc("libopenmpt_example_c");
  links { "ksuser", "winmm" }
  filter {}

 project "libopenmpt_example_c_mem"
  uuid "4db3da91-fafd-43af-b3b7-35699b80aba1"
  language "C"
  vpaths { ["*"] = "../../examples/" }
  mpt_kind "Console"
  warnings "Extra"
	mpt_use_libopenmpt()
	mpt_use_portaudio()
  files {
   "../../examples/libopenmpt_example_c_mem.c",
  }
  mpt_libopenmpt_rc("libopenmpt_example_c_mem");
  links { "ksuser", "winmm" }
  filter {}

 project "libopenmpt_example_c_unsafe"
  uuid "696a79ac-65eb-445f-981a-7639c54569f8"
  language "C"
  vpaths { ["*"] = "../../examples/" }
  mpt_kind "Console"
  warnings "Extra"
	mpt_use_libopenmpt()
	mpt_use_portaudio()
  files {
   "../../examples/libopenmpt_example_c_unsafe.c",
  }
  mpt_libopenmpt_rc("libopenmpt_example_c_unsafe");
  links { "ksuser", "winmm" }
  filter {}

 project "libopenmpt_example_c_probe"
  uuid "3fbc000d-2574-4a02-96ba-db82d7e7d7bb"
  language "C"
  vpaths { ["*"] = "../../examples/" }
  mpt_kind "Console"
  warnings "Extra"
	mpt_use_libopenmpt()
  files {
   "../../examples/libopenmpt_example_c_probe.c",
  }
  mpt_libopenmpt_rc("libopenmpt_example_c_probe");
  filter {}

