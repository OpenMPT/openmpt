@ECHO OFF
html2rtf -ptmp.hpj ..\html\index.htm
html2rtf -ptmp.hpj ..\html\file.htm
if exist tmp.hpj del tmp.hpj
start /wait hcrtf -x "mptrack.hpj"
