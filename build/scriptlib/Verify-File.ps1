param([String]$filename="", [Int32]$filesize=0, [String]$hashvalue="", [String]$hashname="SHA512")
Function Get-FileHashWin7([String] $FileName, $HashName) {
	$FileStream = New-Object System.IO.FileStream($FileName,[System.IO.FileMode]::Open)
	$StringBuilder = New-Object System.Text.StringBuilder
	[System.Security.Cryptography.HashAlgorithm]::Create($HashName).ComputeHash($FileStream)|%{[Void]$StringBuilder.Append($_.ToString("x2"))}
	$FileStream.Close()
	$StringBuilder.ToString()
}
Write-Output "Verify " $filename
if ((Get-Item $filename).length -ne $filesize) {
	Write-Output "Failed " $filename
	exit 1
}
if ((Get-FileHashWin7 $filename $hashname) -ne $hashvalue) {
	Write-Output "Failed " $filename
	exit 1
}
exit 0
