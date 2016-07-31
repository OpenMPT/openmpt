
  filter {}

  filter { "action:vs2008" }
   defines { "_WIN32_WINNT=0x0500" }

  filter { "not action:vs2008" }
		if _OPTIONS["xp"] then
			defines { "_WIN32_WINNT=0x0501" }
		else
			defines { "_WIN32_WINNT=0x0601" }
		end

  filter {}
