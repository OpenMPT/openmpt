
configuration {}

	if _ACTION ~= "postprocess" then

	if _OPTIONS["target"] == "windesktop81" then
		defines { "_WIN32_WINNT=0x0603" }
		
	elseif _OPTIONS["target"] == "winphone8" then
		defines { "_WIN32_WINNT=0x0602" }
		premake.vstudio.toolset = "v110_wp80"

	elseif _OPTIONS["target"] == "winphone81" then
		defines { "_WIN32_WINNT=0x0603" }
		premake.vstudio.toolset = "v120_wp81"
		premake.vstudio.storeapp = "8.1"

	elseif _OPTIONS["target"] == "winstore81" then
		defines { "_WIN32_WINNT=0x0603" }
		premake.vstudio.toolset = "v120"
		premake.vstudio.storeapp = "8.1"

	elseif _OPTIONS["target"] == "winstore82" then
		defines { "_WIN32_WINNT=0x0603" }
		premake.vstudio.storeapp = "8.2"
		local action = premake.action.current()
		action.vstudio.windowsTargetPlatformVersion = "10.0.10240.0"
		action.vstudio.windowsTargetPlatformMinVersion = "10.0.10240.0"

	elseif _OPTIONS["target"] == "winstore10" then
		defines { "_WIN32_WINNT=0x0A00" }
		premake.vstudio.storeapp = "10.0"

	end

	end
