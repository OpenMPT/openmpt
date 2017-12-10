
	filter {}

	if _OPTIONS["win10"] then
		filter {}
			defines { "_WIN32_WINNT=0x0A00" }
		filter {}
	elseif _OPTIONS["xp"] then
		filter { "architecture:x86" }
			defines { "_WIN32_WINNT=0x0501" }
		filter { "architecture:x86_64" }
			defines { "_WIN32_WINNT=0x0502" }
	else
		filter { "architecture:x86 or architecture:x86_64" }
			defines { "_WIN32_WINNT=0x0601" }
		filter { "architecture:ARM or architecture:ARM64" }
			defines { "_WIN32_WINNT=0x0A00" }
	end

  filter {}
