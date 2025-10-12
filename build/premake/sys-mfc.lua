
function mpt_use_mfc (mfc_charset)
	filter {}
	mfc "On"
	defines {
		"_CSTRING_DISABLE_NARROW_WIDE_CONVERSION",
		"_AFX_NO_MFC_CONTROLS_IN_DIALOGS",
	}
	if mfc_charset ~= "Unicode" then
		defines { "NO_WARN_MBCS_MFC_DEPRECATION" }
	end
	-- work-around https://developercommunity.visualstudio.com/t/link-errors-when-building-mfc-application-with-cla/1617786
	if MPT_COMPILER_CLANGCL then
		filter {}
		filter { "configurations:Debug" }
			if true then -- _AFX_NO_MFC_CONTROLS_IN_DIALOGS
				ignoredefaultlibraries { "afxnmcdd.lib" }
				links { "afxnmcdd.lib" }
			end
			if mfc_charset == "Unicode" then
				ignoredefaultlibraries { "uafxcwd.lib", "libcmtd.lib" }
				links { "uafxcwd.lib", "libcmtd.lib" }
			else
				ignoredefaultlibraries { "nafxcwd.lib", "libcmtd.lib" }
				links { "nafxcwd.lib", "libcmtd.lib" }
			end
		filter { "configurations:DebugShared" }
			if mfc_charset == "Unicode" then
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
			if mfc_charset == "Unicode" then
				ignoredefaultlibraries { "uafxcw.lib", "libcmt.lib" }
				links { "uafxcw.lib", "libcmt.lib" }
			else
				ignoredefaultlibraries { "nafxcw.lib", "libcmt.lib" }
				links { "nafxcw.lib", "libcmt.lib" }
			end
		filter { "configurations:CheckedShared" }
			if mfc_charset == "Unicode" then
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
			if mfc_charset == "Unicode" then
				ignoredefaultlibraries { "uafxcw.lib", "libcmt.lib" }
				links { "uafxcw.lib", "libcmt.lib" }
			else
				ignoredefaultlibraries { "nafxcw.lib", "libcmt.lib" }
				links { "nafxcw.lib", "libcmt.lib" }
			end
		filter { "configurations:ReleaseShared" }
			if mfc_charset == "Unicode" then
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
