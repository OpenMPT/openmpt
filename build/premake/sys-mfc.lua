
function mpt_use_mfc ()
	filter {}
	mfc "On"
	defines {
		"_CSTRING_DISABLE_NARROW_WIDE_CONVERSION",
		"_AFX_NO_MFC_CONTROLS_IN_DIALOGS",
	}
	-- work-around https://developercommunity.visualstudio.com/t/link-errors-when-building-mfc-application-with-cla/1617786
	if _OPTIONS["clang"] then
		filter {}
		filter { "configurations:Debug" }
			if true then -- _AFX_NO_MFC_CONTROLS_IN_DIALOGS
				ignoredefaultlibraries { "afxnmcdd.lib" }
				links { "afxnmcdd.lib" }
			end
			if charset == "Unicode" then
				ignoredefaultlibraries { "uafxcwd.lib", "libcmtd.lib" }
				links { "uafxcwd.lib", "libcmtd.lib" }
			else
				ignoredefaultlibraries { "nafxcwd.lib", "libcmtd.lib" }
				links { "nafxcwd.lib", "libcmtd.lib" }
			end
		filter { "configurations:DebugShared" }
			if charset == "Unicode" then
				ignoredefaultlibraries { "mfc140ud.lib", "msvcrtd.lib" }
				links { "mfc140ud.lib", "msvcrtd.lib" }
			else
				ignoredefaultlibraries { "mfc140d.lib", "msvcrtd.lib" }
				links { "mfc140d.lib", "msvcrtd.lib" }
			end
		filter { "configurations:Checked" }
			if true then -- _AFX_NO_MFC_CONTROLS_IN_DIALOGS
				ignoredefaultlibraries { "afxnmcd.lib" }
				links { "afxnmcd.lib" }
			end
			if charset == "Unicode" then
				ignoredefaultlibraries { "uafxcw.lib", "libcmt.lib" }
				links { "uafxcw.lib", "libcmt.lib" }
			else
				ignoredefaultlibraries { "nafxcw.lib", "libcmt.lib" }
				links { "nafxcw.lib", "libcmt.lib" }
			end
		filter { "configurations:CheckedShared" }
			if charset == "Unicode" then
				ignoredefaultlibraries { "mfc140u.lib", "msvcrt.lib" }
				links { "mfc140u.lib", "msvcrt.lib" }
			else
				ignoredefaultlibraries { "mfc140.lib", "msvcrt.lib" }
				links { "mfc140.lib", "msvcrt.lib" }
			end
		filter { "configurations:Release" }
			if true then -- _AFX_NO_MFC_CONTROLS_IN_DIALOGS
				ignoredefaultlibraries { "afxnmcd.lib" }
				links { "afxnmcd.lib" }
			end
			if charset == "Unicode" then
				ignoredefaultlibraries { "uafxcw.lib", "libcmt.lib" }
				links { "uafxcw.lib", "libcmt.lib" }
			else
				ignoredefaultlibraries { "nafxcw.lib", "libcmt.lib" }
				links { "nafxcw.lib", "libcmt.lib" }
			end
		filter { "configurations:ReleaseShared" }
			if charset == "Unicode" then
				ignoredefaultlibraries { "mfc140u.lib", "msvcrt.lib" }
				links { "mfc140u.lib", "msvcrt.lib" }
			else
				ignoredefaultlibraries { "mfc140.lib", "msvcrt.lib" }
				links { "mfc140.lib", "msvcrt.lib" }
			end
		filter {}
	end	
	filter {}
end
