@echo off
REM -- First make map file from Microsoft Visual C++ generated resource.h
echo // MAKEHELP.BAT generated Help Map file.  Used by MPTRACK.HPJ. >"hlp\mptrack.hm"
echo. >>"hlp\mptrack.hm"
echo // Commands (ID_* and IDM_*) >>"hlp\mptrack.hm"
makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h >>"hlp\mptrack.hm"
makehm IDC_,HIDC_,0x10000 resource.h >>"hlp\mptrack.hm"
echo. >>"hlp\mptrack.hm"
echo // Prompts (IDP_*) >>"hlp\mptrack.hm"
makehm IDP_,HIDP_,0x30000 resource.h >>"hlp\mptrack.hm"
echo. >>"hlp\mptrack.hm"
echo // Resources (IDR_*) >>"hlp\mptrack.hm"
makehm IDR_,HIDR_,0x20000 resource.h >>"hlp\mptrack.hm"
echo. >>"hlp\mptrack.hm"
echo // Dialogs (IDD_*) >>"hlp\mptrack.hm"
makehm IDD_,HIDD_,0x20000 resource.h >>"hlp\mptrack.hm"
echo. >>"hlp\mptrack.hm"
echo // Frame Controls (IDW_*) >>"hlp\mptrack.hm"
makehm IDW_,HIDW_,0x50000 resource.h >>"hlp\mptrack.hm"
REM -- Make help for Project MPTRACK


echo Converting HTML to RTF
html2rtf -pc:\temp\tmp.$$$ -dhlp\sizes.def -rhlp\index.rtf html\index.htm
html2rtf -pc:\temp\tmp.$$$ -dhlp\sizes.def -rhlp\file.rtf html\file.htm
html2rtf -pc:\temp\tmp.$$$ -dhlp\sizes.def -rhlp\player.rtf html\player.htm
html2rtf -pc:\temp\tmp.$$$ -dhlp\sizes.def -rhlp\edit.rtf html\edit.htm
html2rtf -pc:\temp\tmp.$$$ -dhlp\sizes.def -rhlp\view.rtf html\view.htm
html2rtf -pc:\temp\tmp.$$$ -dhlp\sizes.def -rhlp\window.rtf html\window.htm
html2rtf -pc:\temp\tmp.$$$ -dhlp\sizes.def -rhlp\help.rtf html\help.htm
html2rtf -pc:\temp\tmp.$$$ -dhlp\sizes.def -rhlp\faq.rtf html\faq.htm
html2rtf -pc:\temp\tmp.$$$ -dhlp\sizes.def -rhlp\formats.rtf html\formats.htm
html2rtf -pc:\temp\tmp.$$$ -dhlp\sizes.def -rhlp\keyboard.rtf html\keyboard.htm
html2rtf -pc:\temp\tmp.$$$ -dhlp\sizes.def -rhlp\mainbar.rtf html\mainbar.htm
html2rtf -pc:\temp\tmp.$$$ -dhlp\sizes.def -rhlp\patterns.rtf html\patterns.htm
html2rtf -pc:\temp\tmp.$$$ -dhlp\sizes.def -rhlp\samples.rtf html\samples.htm
html2rtf -pc:\temp\tmp.$$$ -dhlp\sizes.def -rhlp\instrums.rtf html\instrums.htm
html2rtf -pc:\temp\tmp.$$$ -dhlp\sizes.def -rhlp\intro.rtf html\intro.htm
html2rtf -pc:\temp\tmp.$$$ -dhlp\sizes.def -rhlp\setup.rtf html\setup.htm
html2rtf -pc:\temp\tmp.$$$ -dhlp\sizes.def -rhlp\effects.rtf html\effects.htm
html2rtf -pc:\temp\tmp.$$$ -dhlp\sizes.def -rhlp\starting.rtf html\starting.htm
if exist c:\temp\tmp.$$$ del c:\temp\tmp.$$$

echo Building Win32 Help files
start /wait hcrtf -x "hlp\mptrack.hpj"
echo.
if exist bin\nul copy "hlp\mptrack.hlp" bin\mptrack.hlp
if exist bin\nul copy "hlp\mptrack.cnt" bin\mptrack.cnt
echo.

