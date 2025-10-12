
 project "lhasa"
  uuid "6B11F6A8-B131-4D2B-80EF-5731A9016436"
  language "C"
  location ( "%{wks.location}" .. "/ext" )
  mpt_kind "default"
  targetname "openmpt-lhasa"
  files {
   "../../include/lhasa/lib/crc16.c",
   "../../include/lhasa/lib/ext_header.c",
   "../../include/lhasa/lib/lh1_decoder.c",
   "../../include/lhasa/lib/lh5_decoder.c",
   "../../include/lhasa/lib/lh6_decoder.c",
   "../../include/lhasa/lib/lh7_decoder.c",
   "../../include/lhasa/lib/lha_arch_unix.c",
   "../../include/lhasa/lib/lha_arch_win32.c",
   "../../include/lhasa/lib/lha_basic_reader.c",
   "../../include/lhasa/lib/lha_decoder.c",
   "../../include/lhasa/lib/lha_endian.c",
   "../../include/lhasa/lib/lha_file_header.c",
   "../../include/lhasa/lib/lha_input_stream.c",
   "../../include/lhasa/lib/lha_reader.c",
   "../../include/lhasa/lib/lhx_decoder.c",
   "../../include/lhasa/lib/lk7_decoder.c",
   "../../include/lhasa/lib/lz5_decoder.c",
   "../../include/lhasa/lib/lzs_decoder.c",
   "../../include/lhasa/lib/macbinary.c",
   "../../include/lhasa/lib/null_decoder.c",
   "../../include/lhasa/lib/pm1_decoder.c",
   "../../include/lhasa/lib/pm2_decoder.c",
  }
  files {
   "../../include/lhasa/lib/crc16.h",
   "../../include/lhasa/lib/ext_header.h",
   "../../include/lhasa/lib/lha_arch.h",
   "../../include/lhasa/lib/lha_basic_reader.h",
   "../../include/lhasa/lib/lha_decoder.h",
   "../../include/lhasa/lib/lha_endian.h",
   "../../include/lhasa/lib/lha_file_header.h",
   "../../include/lhasa/lib/lha_input_stream.h",
   "../../include/lhasa/lib/macbinary.h",
   "../../include/lhasa/lib/public/lha_decoder.h",
   "../../include/lhasa/lib/public/lha_file_header.h",
   "../../include/lhasa/lib/public/lha_input_stream.h",
   "../../include/lhasa/lib/public/lha_reader.h",
   "../../include/lhasa/lib/public/lhasa.h",
  }
  filter { "action:vs*" }
    buildoptions { "/wd4244", "/wd4267" }
  filter {}
  filter { "kind:SharedLib" }
   files { "../../build/premake/def/ext-lhasa.def" }
  filter {}

function mpt_use_lhasa ()
	filter {}
	dependencyincludedirs {
		"../../include/lhasa/lib/public",
	}
	filter {}
	links {
		"lhasa",
	}
	filter {}
end
