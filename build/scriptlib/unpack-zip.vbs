
On Error Resume Next

Set fso = CreateObject("Scripting.FileSystemObject")
Set shell = CreateObject("Wscript.Shell")

Sub UnZIP(zipfilename, destinationfolder)
	With CreateObject("Shell.Application")
		.NameSpace(destinationfolder).Copyhere .NameSpace(zipfilename).Items
	End With
End Sub

zipfile = WScript.Arguments(0)
If Err.Number <> 0 Then
	WSCript.Quit 1
End If

outdir  = WScript.Arguments(1)
If Err.Number <> 0 Then
	WSCript.Quit 1
End If

WScript.Echo zipfile
WScript.Echo outdir

If fso.FolderExists(fso.GetAbsolutePathName(outdir)) Then
	fso.DeleteFolder fso.GetAbsolutePathName(outdir)
End If

fso.CreateFolder fso.GetAbsolutePathName(outdir)
If Err.Number <> 0 Then
	WSCript.Quit 1
End If

UnZIP fso.GetAbsolutePathName(zipfile), fso.GetAbsolutePathName(outdir)
If Err.Number <> 0 Then
	WSCript.Quit 1
End If
