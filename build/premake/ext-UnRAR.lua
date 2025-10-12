 
 project "UnRAR"
  uuid "95CC809B-03FC-4EDB-BB20-FD07A698C05F"
  language "C++"
  location ( "%{wks.location}" .. "/ext" )
  mpt_kind "default"
  targetname "openmpt-unrar"
  includedirs { "../../include/unrar" }
	filter {}
  defines {
   "NOMINMAX",
   "NOVOLUME",
   "UNRAR",
   "RAR_NOCRYPT",
   "RARDLL",
   "SILENT",
  }
  files {
   "../../include/unrar/archive.cpp",
   "../../include/unrar/arcread.cpp",
   "../../include/unrar/blake2s.cpp",
   "../../include/unrar/cmddata.cpp",
   "../../include/unrar/consio.cpp",
   "../../include/unrar/crc.cpp",
   "../../include/unrar/crypt.cpp",
   "../../include/unrar/dll.cpp",
   "../../include/unrar/encname.cpp",
   "../../include/unrar/errhnd.cpp",
   "../../include/unrar/extinfo.cpp",
   "../../include/unrar/extract.cpp",
   "../../include/unrar/filcreat.cpp",
   "../../include/unrar/file.cpp",
   "../../include/unrar/filefn.cpp",
   "../../include/unrar/filestr.cpp",
   "../../include/unrar/find.cpp",
   "../../include/unrar/getbits.cpp",
   "../../include/unrar/global.cpp",
   "../../include/unrar/hash.cpp",
   "../../include/unrar/headers.cpp",
   "../../include/unrar/isnt.cpp",
   "../../include/unrar/largepage.cpp",
   "../../include/unrar/list.cpp",
   "../../include/unrar/match.cpp",
   "../../include/unrar/options.cpp",
   "../../include/unrar/pathfn.cpp",
   "../../include/unrar/qopen.cpp",
   "../../include/unrar/rarvm.cpp",
   "../../include/unrar/rawread.cpp",
   "../../include/unrar/rdwrfn.cpp",
   "../../include/unrar/recvol.cpp",
   "../../include/unrar/rijndael.cpp",
   "../../include/unrar/rs.cpp",
   "../../include/unrar/rs16.cpp",
   "../../include/unrar/scantree.cpp",
   "../../include/unrar/secpassword.cpp",
   "../../include/unrar/sha1.cpp",
   "../../include/unrar/sha256.cpp",
   "../../include/unrar/smallfn.cpp",
   "../../include/unrar/strfn.cpp",
   "../../include/unrar/strlist.cpp",
   "../../include/unrar/system.cpp",
   "../../include/unrar/threadpool.cpp",
   "../../include/unrar/timefn.cpp",
   "../../include/unrar/ui.cpp",
   "../../include/unrar/unicode.cpp",
   "../../include/unrar/unpack.cpp",
   "../../include/unrar/volume.cpp",
  }
  files {
   "../../include/unrar/archive.hpp",
   "../../include/unrar/array.hpp",
   "../../include/unrar/blake2s.hpp",
   "../../include/unrar/cmddata.hpp",
   "../../include/unrar/coder.hpp",
   "../../include/unrar/compress.hpp",
   "../../include/unrar/consio.hpp",
   "../../include/unrar/crc.hpp",
   "../../include/unrar/crypt.hpp",
   "../../include/unrar/dll.hpp",
   "../../include/unrar/encname.hpp",
   "../../include/unrar/errhnd.hpp",
   "../../include/unrar/extinfo.hpp",
   "../../include/unrar/extract.hpp",
   "../../include/unrar/filcreat.hpp",
   "../../include/unrar/file.hpp",
   "../../include/unrar/filefn.hpp",
   "../../include/unrar/filestr.hpp",
   "../../include/unrar/find.hpp",
   "../../include/unrar/getbits.hpp",
   "../../include/unrar/global.hpp",
   "../../include/unrar/hash.hpp",
   "../../include/unrar/headers.hpp",
   "../../include/unrar/headers5.hpp",
   "../../include/unrar/isnt.hpp",
   "../../include/unrar/largepage.hpp",
   "../../include/unrar/list.hpp",
   "../../include/unrar/loclang.hpp",
   "../../include/unrar/log.hpp",
   "../../include/unrar/match.hpp",
   "../../include/unrar/model.hpp",
   "../../include/unrar/options.hpp",
   "../../include/unrar/os.hpp",
   "../../include/unrar/pathfn.hpp",
   "../../include/unrar/qopen.hpp",
   "../../include/unrar/rar.hpp",
   "../../include/unrar/rardefs.hpp",
   "../../include/unrar/rarlang.hpp",
   "../../include/unrar/raros.hpp",
   "../../include/unrar/rartypes.hpp",
   "../../include/unrar/rarvm.hpp",
   "../../include/unrar/rawint.hpp",
   "../../include/unrar/rawread.hpp",
   "../../include/unrar/rdwrfn.hpp",
   "../../include/unrar/recvol.hpp",
   "../../include/unrar/resource.hpp",
   "../../include/unrar/rijndael.hpp",
   "../../include/unrar/rs.hpp",
   "../../include/unrar/rs16.hpp",
   "../../include/unrar/scantree.hpp",
   "../../include/unrar/secpassword.hpp",
   "../../include/unrar/sha1.hpp",
   "../../include/unrar/sha256.hpp",
   "../../include/unrar/smallfn.hpp",
   "../../include/unrar/strfn.hpp",
   "../../include/unrar/strlist.hpp",
   "../../include/unrar/suballoc.hpp",
   "../../include/unrar/system.hpp",
   "../../include/unrar/threadpool.hpp",
   "../../include/unrar/timefn.hpp",
   "../../include/unrar/ui.hpp",
   "../../include/unrar/unicode.hpp",
   "../../include/unrar/unpack.hpp",
   "../../include/unrar/version.hpp",
   "../../include/unrar/volume.hpp",
  }
	filter {}
	filter { "action:vs*" }
		buildoptions { "/wd4996" }
	filter {}
	filter { "action:vs*" }
		buildoptions {
			"/wd6031",
			"/wd6262",
			"/wd28159",
		} -- analyze
	filter {}
		if _OPTIONS["clang"] then
			buildoptions {
				"-Wno-dangling-else",
				"-Wno-logical-not-parentheses",
				"-Wno-logical-op-parentheses",
				"-Wno-missing-braces",
				"-Wno-switch",
				"-Wno-unused-but-set-variable",
				"-Wno-unused-function",
				"-Wno-unused-variable",
			}
		end
	filter {}
  filter { "kind:SharedLib" }
   files { "../../include/unrar/dll_nocrypt.def" }
  filter {}

function mpt_use_unrar ()
	filter {}
	dependencyincludedirs {
		"../../include",
	}
	filter {}
	links {
		"UnRAR",
	}
	filter {}
end
