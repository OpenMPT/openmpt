  
 project "portaudiocpp"
  uuid "627cf18a-c8ca-451e-afd0-8679cadfda6b"
  language "C++"
  location ( "%{wks.location}" .. "/ext" )
  mpt_kind "static"
  targetname "openmpt-portaudiocpp"
	
	mpt_use_portaudio()
	
  includedirs { "../../include/portaudio/bindings/cpp/include" }
	filter {}
  defines {
   "PAWIN_USE_WDMKS_DEVICE_INFO",
   "PA_WDMKS_NO_KSGUID_LIB",
   "PA_USE_ASIO=0",
   "PA_USE_DS=1",
   "PA_USE_WMME=1",
   "PA_USE_WASAPI=1",
   "PA_USE_WDMKS=1",
  }
  files {
   "../../include/portaudio/bindings/cpp/include/portaudiocpp/*.hxx",
  }
  files {
   "../../include/portaudio/bindings/cpp/source/portaudiocpp/*.cxx",
  }
  filter { "configurations:Debug" }
   defines { "PA_ENABLE_DEBUG_OUTPUT" }
  filter { "configurations:DebugShared" }
   defines { "PA_ENABLE_DEBUG_OUTPUT" }
  filter { "configurations:DebugMDd" }
   defines { "PA_ENABLE_DEBUG_OUTPUT" }
  filter {}

function mpt_use_portaudiocpp ()
	filter {}
	filter { "action:vs*" }
		includedirs {
			"../../include/portaudio/bindings/cpp/include",
		}
	filter { "not action:vs*" }
		externalincludedirs {
			"../../include/portaudio/bindings/cpp/include",
		}
	filter {}
	links {
		"portaudiocpp",
	}
	filter {}
end
