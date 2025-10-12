
include_dependency "ext-ogg.lua"

 project "vorbis"
  -- NOTE: Unlike the official libvorbis, we built everything into a single library instead of the vorbis, vorbisenc, vorbisfile split.
  uuid "b544dcb7-16e5-41bc-b51b-7ead8cfdfa05"
  language "C"
  location ( "%{wks.location}" .. "/ext" )
  mpt_kind "default"
  targetname "openmpt-vorbis"
	
	mpt_use_ogg()
	
  includedirs {
   "../../include/vorbis/include",
   "../../include/vorbis/lib",
  }
	filter {}
  files {
   "../../include/vorbis/include/vorbis/codec.h",
   "../../include/vorbis/include/vorbis/vorbisenc.h",
   "../../include/vorbis/include/vorbis/vorbisfile.h",
  }
  files {
   "../../include/vorbis/lib/analysis.c",
   "../../include/vorbis/lib/backends.h",
   --"../../include/vorbis/lib/barkmel.c",
   "../../include/vorbis/lib/bitrate.c",
   "../../include/vorbis/lib/bitrate.h",
   "../../include/vorbis/lib/block.c",
   "../../include/vorbis/lib/codebook.c",
   "../../include/vorbis/lib/codebook.h",
   "../../include/vorbis/lib/codec_internal.h",
   "../../include/vorbis/lib/envelope.c",
   "../../include/vorbis/lib/envelope.h",
   "../../include/vorbis/lib/floor0.c",
   "../../include/vorbis/lib/floor1.c",
   "../../include/vorbis/lib/highlevel.h",
   "../../include/vorbis/lib/info.c",
   "../../include/vorbis/lib/lookup.c",
   "../../include/vorbis/lib/lookup.h",
   "../../include/vorbis/lib/lookup_data.h",
   "../../include/vorbis/lib/lpc.c",
   "../../include/vorbis/lib/lpc.h",
   "../../include/vorbis/lib/lsp.c",
   "../../include/vorbis/lib/lsp.h",
   "../../include/vorbis/lib/mapping0.c",
   "../../include/vorbis/lib/masking.h",
   "../../include/vorbis/lib/mdct.c",
   "../../include/vorbis/lib/mdct.h",
   "../../include/vorbis/lib/misc.h",
   "../../include/vorbis/lib/os.h",
   "../../include/vorbis/lib/psy.c",
   "../../include/vorbis/lib/psy.h",
   -- "../../include/vorbis/lib/psytune.c",
   "../../include/vorbis/lib/registry.c",
   "../../include/vorbis/lib/registry.h",
   "../../include/vorbis/lib/res0.c",
   "../../include/vorbis/lib/scales.h",
   "../../include/vorbis/lib/sharedbook.c",
   "../../include/vorbis/lib/smallft.c",
   "../../include/vorbis/lib/smallft.h",
   "../../include/vorbis/lib/synthesis.c",
   -- "../../include/vorbis/lib/tone.c",
   "../../include/vorbis/lib/vorbisenc.c",
   "../../include/vorbis/lib/vorbisfile.c",
   "../../include/vorbis/lib/window.c",
   "../../include/vorbis/lib/window.h",
   "../../include/vorbis/lib/books/coupled/res_books_51.h",
   "../../include/vorbis/lib/books/coupled/res_books_stereo.h",
   "../../include/vorbis/lib/books/floor/floor_books.h",
   "../../include/vorbis/lib/books/uncoupled/res_books_uncoupled.h",
   "../../include/vorbis/lib/modes/floor_all.h",
   "../../include/vorbis/lib/modes/psych_8.h",
   "../../include/vorbis/lib/modes/psych_11.h",
   "../../include/vorbis/lib/modes/psych_16.h",
   "../../include/vorbis/lib/modes/psych_44.h",
   "../../include/vorbis/lib/modes/residue_8.h",
   "../../include/vorbis/lib/modes/residue_16.h",
   "../../include/vorbis/lib/modes/residue_44.h",
   "../../include/vorbis/lib/modes/residue_44p51.h",
   "../../include/vorbis/lib/modes/residue_44u.h",
   "../../include/vorbis/lib/modes/setup_8.h",
   "../../include/vorbis/lib/modes/setup_11.h",
   "../../include/vorbis/lib/modes/setup_16.h",
   "../../include/vorbis/lib/modes/setup_22.h",
   "../../include/vorbis/lib/modes/setup_32.h",
   "../../include/vorbis/lib/modes/setup_44.h",
   "../../include/vorbis/lib/modes/setup_44p51.h",
   "../../include/vorbis/lib/modes/setup_44u.h",
   "../../include/vorbis/lib/modes/setup_X.h",
  }
  filter { "action:vs*" }
    buildoptions { "/wd4101", "/wd4244", "/wd4267", "/wd4305", "/wd4703" }
  filter {}
  filter { "action:vs*" }
    buildoptions { "/wd6001", "/wd6011", "/wd6255", "/wd6262", "/wd6263", "/wd6297", "/wd6308", "/wd6385", "/wd6386", "/wd6387", "/wd28182" } -- /analyze
	filter {}
		if _OPTIONS["clang"] then
			buildoptions {
				"-Wno-unused-but-set-variable",
				"-Wno-unused-variable",
			}
		end
	filter {}

  filter {}
  filter { "kind:SharedLib" }
   files { "../../build/premake/def/ext-vorbis.def" }
  filter {}

function mpt_use_vorbis ()
	filter {}
	dependencyincludedirs {
		"../../include/vorbis/include",
	}
	filter {}
	links {
		"vorbis",
	}
	filter {}
end
