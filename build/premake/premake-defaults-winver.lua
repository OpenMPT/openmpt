
	filter {}

	if _OPTIONS["win10"] then
		filter {}
			defines { "_WIN32_WINNT=0x0A00" }
		filter {}
	else
		filter { "architecture:x86 or architecture:x86_64" }
			defines { "_WIN32_WINNT=0x0601" }
		filter { "architecture:ARM or architecture:ARM64" }
			defines { "_WIN32_WINNT=0x0A00" }
	end

  filter {}
