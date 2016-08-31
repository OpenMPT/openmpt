
  filter {}

	if _OPTIONS["xp"] then
		filter { "architecture:x86" }
			defines { "_WIN32_WINNT=0x0501" }
		filter { "architecture:x86_64" }
			defines { "_WIN32_WINNT=0x0502" }
	else
		filter {}
			defines { "_WIN32_WINNT=0x0601" }
	end

  filter {}
