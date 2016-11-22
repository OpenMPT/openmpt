; OpenMPT Install script for InnoSetup
; Written by Johannes Schultz
; http://openmpt.org/
; http://sagamusix.de/

; This file is provided for creating an install package without the proprietary unmo3.dll (for example for the SourceForge package).
; Instead of including the file in the setup package, the user instead has the possibility to automatically download unmo3.dll from
; our servers.
; The download code requires the Inno Download Plugin from https://code.google.com/p/inno-download-plugin/

; Uncomment one of following lines, if you haven't checked "Add IDP include path to ISPPBuiltins.iss" option during IDP installation:
;#pragma include __INCLUDE__ + ";" + ReadReg(HKLM, "Software\Mitrich Software\Inno Download Plugin", "InstallDir")
;#pragma include __INCLUDE__ + ";" + "C:\Program Files (x86)\Inno Download Plugin"

#define DOWNLOAD_MO3
#define BaseNameAddition "_sf"
#include <idp.iss>
#include "win32.iss"

[Code]

// Verify checksum of downloaded file, and if it is OK, copy it to the app directory.
procedure VerifyUNMO3Checksum();
begin
    if(IsTaskSelected('downloadmo3') And FileExists(ExpandConstant('{tmp}\openmpt-unmo3.dll.tmp'))) then
    begin
	    if(GetSHA1OfFile(ExpandConstant('{tmp}\openmpt-unmo3.dll.tmp')) <> '4c285992c11edbef5612c0cbaf8708499bcaee01') then
    	begin
    	   MsgBox('Warning: unmo3.dll has been downloaded, but its checksum is wrong! This means that the downloaded file might corrupted or manipulated. The file has thus not been installed. Please re-download the OpenMPT installer from https://openmpt.org/ instead.', mbCriticalError, MB_OK)
    	end else
    	begin
    	   FileCopy(ExpandConstant('{tmp}\openmpt-unmo3.dll.tmp'), ExpandConstant('{app}\unmo3.dll'), true);
    	end;
    	DeleteFile(ExpandConstant('{tmp}\openmpt-unmo3.dll.tmp'));
    end;
end;

[Code]
procedure InitializeWizard();
begin
    idpDownloadAfter(wpReady);
end;

procedure CurPageChanged(CurPageID: Integer);
begin
    if CurPageID = wpReady then
    begin
        // User can navigate to 'Ready to install' page several times, so we 
        // need to clear file list to ensure that only needed files are added.
        idpClearFiles;
        if(IsTaskSelected('downloadmo3')) then
        begin
            idpAddFile('http://download.openmpt.org/archive/unmo3/2.4.1.2/win-x86/unmo3.dll', ExpandConstant('{tmp}\openmpt-unmo3.dll.tmp'));
            idpAddMirror('http://download.openmpt.org/archive/unmo3/2.4.1.2/win-x86/unmo3.dll', 'ftp://ftp.untergrund.net/users/sagamusix/openmpt/archive/unmo3/2.4.1.2/win-x86/unmo3.dll');
        end;
    end;
end;