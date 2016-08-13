
  filter {}

  filter { "action:vs2008" }
   defines { "_WIN32_WINNT=0x0500" }

	if _OPTIONS["xp"] then
		filter { "not action:vs2008", "architecture:x86" }
			defines { "_WIN32_WINNT=0x0501" }
		filter { "not action:vs2008", "architecture:x86_64" }
			defines { "_WIN32_WINNT=0x0502" }
	else
		filter { "not action:vs2008" }
			defines { "_WIN32_WINNT=0x0601" }
	end

  filter {}
