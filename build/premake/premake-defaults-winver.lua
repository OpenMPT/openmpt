
	filter {}

	if _OPTIONS["win10"] then
		defines { "_WIN32_WINNT=0x0A00" }
	elseif _OPTIONS["win81"] then
		defines { "_WIN32_WINNT=0x0603" }
	elseif _OPTIONS["win7"] then
		defines { "_WIN32_WINNT=0x0601" }
	end

  filter {}
