
Set fso = CreateObject("Scripting.FileSystemObject")

' change to script directory
Set shell = CreateObject("Wscript.Shell")
shell.CurrentDirectory = fso.GetParentFolderName(WScript.ScriptFullName)

Function GetBinaryFile(filename)
	Set stream = CreateObject("ADODB.Stream")
	stream.Type = 1 ' binary
	stream.Open
	stream.LoadFromFile filename
	GetBinaryFile = stream.Read
End Function

Function SHA1(filename)
	Set oSHA1 = CreateObject("System.Security.Cryptography.SHA1CryptoServiceProvider")
	oSHA1.ComputeHash_2(GetBinaryFile(filename))
	hashstring = ""
	For i = 1 to LenB(oSHA1.Hash)
		hashstring = hashstring & LCase(Right("0" & Hex(AscB(MidB(oSHA1.Hash, i, 1))), 2))
	Next
	SHA1 = hashstring
End Function

Sub Download(url, expected_size, expected_sha1, filename)
	Set http = CreateObject("Microsoft.XMLHTTP")
	Set stream = CreateObject("ADODB.Stream")
	http.Open "GET", url, False
	http.Send
	With stream
		.Type = 1 ' binary
		.Open
		.Write http.responseBody
		.SaveToFile filename, 2 ' overwrite
	End With
	If fso.GetFile(filename).Size <> expected_size Then
		Err.Raise vbObjectError + 1, "libopenmpt", url & " size mismatch."
	End If
	If SHA1(filename) <> expected_sha1 Then
		Err.Raise vbObjectError + 1, "libopenmpt", url & " checksum mismatch."
	End If
End Sub

Sub DeleteFolder(pathname)
	If fso.FolderExists(pathname) Then
		fso.DeleteFolder pathname
	End If
End Sub

Sub UnZIP(zipfilename, destinationfolder)
	If Not fso.FolderExists(destinationfolder) Then
		fso.CreateFolder(destinationfolder)
	End If
	With CreateObject("Shell.Application")
		.NameSpace(destinationfolder).Copyhere .NameSpace(zipfilename).Items
	End With
End Sub

Sub CreateFolder(pathname)
	If Not fso.FolderExists(pathname) Then
		fso.CreateFolder pathname
	End If
End Sub

' we cannot use mpg123-1.24.0 yet because of https://sourceforge.net/p/mpg123/bugs/246/

CreateFolder "download.tmp"

Download "https://mpg123.de/download/win32/mpg123-1.23.8-x86.zip", 569733, "054aaf106712cb7f8308e9e9edd138e3e2789d9e", "download.tmp\mpg123-1.23.8-x86.zip"
DeleteFolder fso.BuildPath(fso.GetAbsolutePathName("."), "download.tmp\mpg123-1.23.8-x86")
UnZIP fso.BuildPath(fso.GetAbsolutePathName("."), "download.tmp\mpg123-1.23.8-x86.zip"), fso.BuildPath(fso.GetAbsolutePathName("."), "download.tmp")

Download "https://mpg123.de/download/win64/mpg123-1.23.8-x86-64.zip", 616269, "c29414fa4d93f8d963009073122f1101842f86a7", "download.tmp\mpg123-1.23.8-x86-64.zip"
DeleteFolder fso.BuildPath(fso.GetAbsolutePathName("."), "download.tmp\mpg123-1.23.8-x86-64")
UnZIP fso.BuildPath(fso.GetAbsolutePathName("."), "download.tmp\mpg123-1.23.8-x86-64.zip"), fso.BuildPath(fso.GetAbsolutePathName("."), "download.tmp")

CreateFolder "Licenses"

fso.CopyFile "download.tmp\mpg123-1.23.8-x86\COPYING.txt", "Licenses\License.mpg123.txt", True

If fso.FolderExists("bin") Then
	CreateFolder "bin\x86"
	CreateFolder "bin\x86_64"
	fso.CopyFile "download.tmp\mpg123-1.23.8-x86\libgcc_s_sjlj-1.dll",    "bin\x86\libgcc_s_sjlj-1.dll", True
	fso.CopyFile "download.tmp\mpg123-1.23.8-x86\libmpg123-0.dll",        "bin\x86\libmpg123-0.dll", True
	'fso.CopyFile "download.tmp\mpg123-1.23.8-x86-64\libgcc_s_sjlj-1.dll", "bin\x86_64\libgcc_s_sjlj-1.dll", True
	fso.CopyFile "download.tmp\mpg123-1.23.8-x86-64\libmpg123-0.dll",     "bin\x86_64\libmpg123-0.dll", True
End If

If fso.FolderExists("openmpt123") Then
	CreateFolder "openmpt123\x86"
	CreateFolder "openmpt123\x86_64"
	fso.CopyFile "download.tmp\mpg123-1.23.8-x86\libgcc_s_sjlj-1.dll",    "openmpt123\x86\libgcc_s_sjlj-1.dll", True
	fso.CopyFile "download.tmp\mpg123-1.23.8-x86\libmpg123-0.dll",        "openmpt123\x86\libmpg123-0.dll", True
	'fso.CopyFile "download.tmp\mpg123-1.23.8-x86-64\libgcc_s_sjlj-1.dll", "openmpt123\x86_64\libgcc_s_sjlj-1.dll", True
	fso.CopyFile "download.tmp\mpg123-1.23.8-x86-64\libmpg123-0.dll",     "openmpt123\x86_64\libmpg123-0.dll", True
End If

If fso.FolderExists("XMPlay") Then
	fso.CopyFile "download.tmp\mpg123-1.23.8-x86\libgcc_s_sjlj-1.dll",    "XMPlay\libgcc_s_sjlj-1.dll", True
	fso.CopyFile "download.tmp\mpg123-1.23.8-x86\libmpg123-0.dll",        "XMPlay\libmpg123-0.dll", True
End If

If fso.FolderExists("foobar2000") Then
	fso.CopyFile "download.tmp\mpg123-1.23.8-x86\libgcc_s_sjlj-1.dll",    "foobar2000\libgcc_s_sjlj-1.dll", True
	fso.CopyFile "download.tmp\mpg123-1.23.8-x86\libmpg123-0.dll",        "foobar2000\libmpg123-0.dll", True
End If

If fso.FolderExists("Winamp") Then
	fso.CopyFile "download.tmp\mpg123-1.23.8-x86\libgcc_s_sjlj-1.dll",    "Winamp\libgcc_s_sjlj-1.dll", True
	fso.CopyFile "download.tmp\mpg123-1.23.8-x86\libmpg123-0.dll",        "Winamp\libmpg123-0.dll", True
End If

DeleteFolder "download.tmp"

WScript.Echo "libmpg123 download successful"
