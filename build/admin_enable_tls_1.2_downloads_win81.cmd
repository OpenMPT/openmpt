@echo off

echo "Please only run this script if downloading files from GitHub or other sites fails."
echo ""
echo "This script needs to be run with admin rights."
echo "It may additionally require KB3154520 to be installed."
echo ""
echo "For more information, see"
echo "'https://developercommunity.visualstudio.com/content/problem/201610/unable-to-download-some-components-due-to-tls-12-o.html'"
echo "and"
echo "'https://support.microsoft.com/en-us/help/3154518/support-for-tls-system-default-versions-included-in-the-net-framework'"
echo "."
pause

echo "Enabling system default TLS versions for .NET 2.0 32bit"
reg add HKLM\SOFTWARE\Microsoft\.NETFramework\v2.0.50727 /v SystemDefaultTlsVersions /t REG_DWORD /d 1 /f /reg:32

echo "Enabling system default TLS versions for .NET 4.0 64bit"
reg add HKLM\SOFTWARE\Microsoft\.NETFramework\v4.0.30319 /v SystemDefaultTlsVersions /t REG_DWORD /d 1 /f /reg:32

echo "Enabling system default TLS versions for .NET 2.0 32bit"
reg add HKLM\SOFTWARE\Microsoft\.NETFramework\v2.0.50727 /v SystemDefaultTlsVersions /t REG_DWORD /d 1 /f /reg:64

echo "Enabling system default TLS versions for .NET 4.0 64bit"
reg add HKLM\SOFTWARE\Microsoft\.NETFramework\v4.0.30319 /v SystemDefaultTlsVersions /t REG_DWORD /d 1 /f /reg:64

pause
