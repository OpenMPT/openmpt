	filter {}
		if _ACTION == "vs2017" then
			buildoptions { "/permissive-", "/Zc:twoPhase-" } -- '/Zc:twoPhase-' is required because combaseapi.h in Windwos 8.1 SDK relies on it
		end
	filter {}
