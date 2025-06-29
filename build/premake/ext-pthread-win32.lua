 
 project "pthread-win32"
  uuid "2f5b53aa-ab47-4b76-9913-87210353d8e4"
  language "C"
  location ( "%{wks.location}" .. "/ext" )
  mpt_kind "shared"
  targetname "openmpt-pthread-win32"
  includedirs {
		"../../include/pthread-win32",
	}
	filter {}
	files {
		"../../include/pthread-win32/*.c",
		"../../include/pthread-win32/*.h",
	}
	excludes {
		"../../include/pthread-win32/pthread.c",
		"../../include/pthread-win32/pthread-EH.cpp",
		"../../include/pthread-win32/pthread-JMP.c",
	}
	filter {}
		defines { "HAVE_CONFIG_H" }
		defines { "PTW32_CLEANUP_SEH" }
	filter {}
	if true then
		filter {}
			defines { "_WINDLL" }
		filter {}
	else
		filter {}
		filter { "configurations:*Shared" }
			defines { "_WINDLL" }
		filter {}
		filter { "not configurations:*Shared" }
			defines { "PTW32_STATIC_LIB" }
		filter {}
	end

function mpt_use_pthread_win32 ()
	filter {}
	filter { "action:vs*" }
		includedirs {
			"../../include/pthread-win32",
		}
	filter { "not action:vs*" }
		externalincludedirs {
			"../../include/pthread-win32",
		}
	filter {}
		defines { "PTW32_CLEANUP_SEH" }
	filter {}
	if true then
		filter {}
	else
		filter {}
		filter { "configurations:*Shared" }
		filter {}
		filter { "not configurations:*Shared" }
			defines { "PTW32_STATIC_LIB" }
		filter {}
	end
	links {
		"pthread-win32",
	}
	filter {}
end
