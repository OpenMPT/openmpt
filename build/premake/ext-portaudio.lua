  
 project "portaudio"
  uuid "189B867F-FF4B-45A1-B741-A97492EE69AF"
  language "C"
  location ( "%{wks.location}" .. "/ext" )
  mpt_kind "default"
  targetname "openmpt-portaudio"
  includedirs { "../../include/portaudio/include", "../../include/portaudio/src/common", "../../include/portaudio/src/os/win" }
	filter {}
		if MPT_WIN_BEFORE(MPT_WIN["7"]) then
			defines {
				"PA_USE_ASIO=0",
				"PA_USE_DS=1",
				"PA_USE_WMME=1",
				"PA_USE_WASAPI=1",
				"PA_USE_WDMKS=0",
			}
		elseif MPT_OS_WINDOWS_WINRT then
			defines {
				"PA_USE_ASIO=0",
				"PA_USE_DS=0",
				"PA_USE_WMME=0",
				"PA_USE_WASAPI=0",
				"PA_USE_WDMKS=0",
			}
		else
			defines {
				"PAWIN_USE_WDMKS_DEVICE_INFO",
				"PA_WDMKS_NO_KSGUID_LIB",
				"PA_USE_ASIO=0",
				"PA_USE_DS=0",
				"PA_USE_WMME=1",
				"PA_USE_WASAPI=1",
				"PA_USE_WDMKS=1",
			}
		end
	filter {}
  files {
   "../../include/portaudio/src/common/pa_allocation.c",
   "../../include/portaudio/src/common/pa_allocation.h",
   "../../include/portaudio/src/common/pa_converters.c",
   "../../include/portaudio/src/common/pa_converters.h",
   "../../include/portaudio/src/common/pa_cpuload.c",
   "../../include/portaudio/src/common/pa_cpuload.h",
   "../../include/portaudio/src/common/pa_debugprint.c",
   "../../include/portaudio/src/common/pa_debugprint.h",
   "../../include/portaudio/src/common/pa_dither.c",
   "../../include/portaudio/src/common/pa_dither.h",
   "../../include/portaudio/src/common/pa_endianness.h",
   "../../include/portaudio/src/common/pa_front.c",
   "../../include/portaudio/src/common/pa_gitrevision.h",
   "../../include/portaudio/src/common/pa_hostapi.h",
   "../../include/portaudio/src/common/pa_memorybarrier.h",
   "../../include/portaudio/src/common/pa_process.c",
   "../../include/portaudio/src/common/pa_process.h",
   "../../include/portaudio/src/common/pa_ringbuffer.c",
   "../../include/portaudio/src/common/pa_ringbuffer.h",
   "../../include/portaudio/src/common/pa_stream.c",
   "../../include/portaudio/src/common/pa_stream.h",
   "../../include/portaudio/src/common/pa_trace.c",
   "../../include/portaudio/src/common/pa_trace.h",
   "../../include/portaudio/src/common/pa_types.h",
   "../../include/portaudio/src/common/pa_util.h",
   "../../include/portaudio/src/hostapi/skeleton/pa_hostapi_skeleton.c",
   "../../include/portaudio/src/os/win/pa_win_coinitialize.c",
   "../../include/portaudio/src/os/win/pa_win_coinitialize.h",
   "../../include/portaudio/src/os/win/pa_win_hostapis.c",
   "../../include/portaudio/src/os/win/pa_win_util.c",
   "../../include/portaudio/src/os/win/pa_win_waveformat.c",
   "../../include/portaudio/src/os/win/pa_x86_plain_converters.c",
   "../../include/portaudio/src/os/win/pa_x86_plain_converters.h",
  }
	filter {}
		if MPT_WIN_BEFORE(MPT_WIN["7"]) then
			files {
				"../../include/portaudio/src/hostapi/wmme/pa_win_wmme.c",
				"../../include/portaudio/src/hostapi/dsound/pa_win_ds.c",
				"../../include/portaudio/src/hostapi/dsound/pa_win_ds_dynlink.c",
				"../../include/portaudio/src/hostapi/dsound/pa_win_ds_dynlink.h",
				"../../include/portaudio/src/hostapi/wasapi/pa_win_wasapi.c",
			}
		elseif MPT_OS_WINDOWS_WINRT then
		else
			files {
				"../../include/portaudio/src/hostapi/wmme/pa_win_wmme.c",
				"../../include/portaudio/src/hostapi/wasapi/pa_win_wasapi.c",
				"../../include/portaudio/src/hostapi/wdmks/pa_win_wdmks.c",
				"../../include/portaudio/src/os/win/pa_win_wdmks_utils.c",
				"../../include/portaudio/src/os/win/pa_win_wdmks_utils.h",
			}
		end
	filter {}
  files {
   "../../include/portaudio/include/pa_asio.h",
   "../../include/portaudio/include/pa_jack.h",
   "../../include/portaudio/include/pa_linux_alsa.h",
   "../../include/portaudio/include/pa_mac_core.h",
   "../../include/portaudio/include/pa_win_ds.h",
   "../../include/portaudio/include/pa_win_wasapi.h",
   "../../include/portaudio/include/pa_win_waveformat.h",
   "../../include/portaudio/include/pa_win_wdmks.h",
   "../../include/portaudio/include/pa_win_wmme.h",
   "../../include/portaudio/include/portaudio.h",
  }
	filter {}
	if MPT_COMPILER_MSVC or MPT_COMPILER_CLANGCL then
		buildoptions { "/wd4018", "/wd4091", "/wd4267", "/wd4312" }
		buildoptions { "/wd6001", "/wd6011", "/wd6053", "/wd6216", "/wd6217", "/wd6255", "/wd6258", "/wd6385", "/wd6386", "/wd6387", "/wd28159" } -- /analyze
	end
	filter {}
	if MPT_COMPILER_CLANGCL or MPT_COMPILER_CLANG then
		buildoptions {
			"-Wno-implicit-const-int-float-conversion",
			"-Wno-missing-braces",
			"-Wno-sometimes-uninitialized",
			"-Wno-switch",
			"-Wno-unused-but-set-variable",
			"-Wno-unused-function",
			"-Wno-unused-variable",
		}
	end
	filter {}
	if MPT_BUILD_MSBUILD then
		files { "../../build/premake/lnk/ext-portaudio.c" }
	end
	filter {}
  filter { "configurations:Debug" }
   defines { "PA_ENABLE_DEBUG_OUTPUT" }
  filter { "configurations:DebugShared" }
   defines { "PA_ENABLE_DEBUG_OUTPUT" }
  filter { "configurations:DebugMDd" }
   defines { "PA_ENABLE_DEBUG_OUTPUT" }
	filter {}
	if MPT_OS_WINDOWS then
		filter {}
		filter { "kind:SharedLib" }
			if MPT_WIN_BEFORE(MPT_WIN["7"]) then
				files { "../../build/premake/def/ext-portaudio-retro.def" }
			elseif MPT_OS_WINDOWS_WINRT then
				files { "../../build/premake/def/ext-portaudio-uwp.def" }
			else
				files { "../../build/premake/def/ext-portaudio.def" }
			end
		filter {}
	end
	filter {}

function mpt_use_portaudio ()
	filter {}
	dependencyincludedirs {
		"../../include/portaudio/include",
	}
	filter {}
	links {
		"portaudio",
	}
	filter {}
end
