
  filter {}

	if _OPTIONS["w2k"] then
		filter { "architecture:x86" }
			defines { "_WIN32_WINNT=0x0500" }
		filter { "architecture:x86_64" }
			defines { "_WIN32_WINNT=0x0502" }
	elseif _OPTIONS["xp"] then
		filter { "architecture:x86" }
			defines { "_WIN32_WINNT=0x0501" }
		filter { "architecture:x86_64" }
			defines { "_WIN32_WINNT=0x0502" }
	else
		filter { "action:vs2008" }
			defines { "_WIN32_WINNT=0x0600" }
		filter { "not action:vs2008" }
			defines { "_WIN32_WINNT=0x0601" }
	end

  filter {}
