@echo off

echo "Please only run this script if downloading files from GitHub or other sites fails."
echo ""
echo "This script needs to be run with admin rights."
echo "It may additionally require KB3154518 to be installed."
echo ""
echo "For more information, see"
echo "'https://developercommunity.visualstudio.com/content/problem/201610/unable-to-download-some-components-due-to-tls-12-o.html'"
echo "and"
echo "'https://support.microsoft.com/en-us/help/3154518/support-for-tls-system-default-versions-included-in-the-net-framework'"
echo "and"
echo "'https://docs.microsoft.com/en-us/previous-versions/windows/it-pro/windows-server-2012-R2-and-2012/dn786418(v=ws.11)'"
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

echo "Enabling TLS 1.1 32bit"
reg add "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\SecurityProviders\SCHANNEL\Protocols\TLS 1.1\Client" /v DisabledByDefault /t REG_DWORD /d 0 /f /reg:32

echo "Enabling TLS 1.1 64bit"
reg add "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\SecurityProviders\SCHANNEL\Protocols\TLS 1.1\Client" /v DisabledByDefault /t REG_DWORD /d 0 /f /reg:64

echo "Enabling TLS 1.2 32bit"
reg add "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\SecurityProviders\SCHANNEL\Protocols\TLS 1.2\Client" /v DisabledByDefault /t REG_DWORD /d 0 /f /reg:32

echo "Enabling TLS 1.2 64bit"
reg add "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\SecurityProviders\SCHANNEL\Protocols\TLS 1.2\Client" /v DisabledByDefault /t REG_DWORD /d 0 /f /reg:64

pause
