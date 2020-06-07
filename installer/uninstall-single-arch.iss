


[Code]

function GetAppPath(AppId: String; IsWow64: Boolean): String;
var
	AppPath: String;
begin
	Result := '';
	AppPath := '';
	if IsWow64 then
	begin
		RegQueryStringValue(HKLM, 'SOFTWARE\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall\' + AppId + '_is1', 'Inno Setup: App Path', AppPath)
	end
	else
	begin
		RegQueryStringValue(HKLM, 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\' + AppId + '_is1', 'Inno Setup: App Path', AppPath)
	end;
	Result := AppPath;
end;

function GetUninstallCommand(AppId: String; IsWow64: Boolean): String;
var
	UninstallCommand: String;
begin
	Result := '';
	UninstallCommand := '';
	if IsWow64 then
	begin
		RegQueryStringValue(HKLM, 'SOFTWARE\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall\' + AppId + '_is1', 'UninstallString', UninstallCommand)
	end
	else
	begin
		RegQueryStringValue(HKLM, 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\' + AppId + '_is1', 'UninstallString', UninstallCommand)
	end;
	Result := UninstallCommand;
end;

function Uninstall(AppId: String; IsWow64: Boolean): Boolean;
var
	UninstallCommand: String;
	ResultCode: Integer;
begin
	Result := False;
	UninstallCommand := GetUninstallCommand(AppId, IsWow64);
	if UninstallCommand <> '' then
	begin
		ResultCode := 0;
		SuppressibleMsgBox(UninstallCommand, mbInformation, MB_OK, IDOK);
		if Exec(RemoveQuotes(UninstallCommand), '/SILENT /NORESTART /SUPPRESSMSGBOXES', '', SW_SHOW, ewWaitUntilTerminated, ResultCode) then
		begin
			SuppressibleMsgBox('Success', mbInformation, MB_OK, IDOK);
			Result := True;
		end
		else
		begin
			SuppressibleMsgBox('Failure', mbInformation, MB_OK, IDOK);
			Result := False;
		end;
	end
	else
	begin
		Result := True;
	end;
end;

function UninstallSingleArch: Boolean;
var
	AppId_x86: String;
	AppId_amd64: String;
	Success: Boolean;
begin
	Success := True;
	AppId_x86 := '{67903736-E9BB-4664-B148-F62BCAB4FA42}';
	AppId_amd64 := '{9814C59D-8CBE-4C38-8A5F-7BF9B4FFDA6D}';
	if IsWin64() then
	begin
		Success := Uninstall(AppId_amd64, False) and Success;
		Success := Uninstall(AppId_x86, True) and Success;
	end
	else
	begin
		Success := Uninstall(AppId_x86, False) and Success;
	end;
	Result := Success;
end;

function GetPreviousSingleArchInstallPath: String;
var
	AppId_x86: String;
	AppId_amd64: String;
	AppPath: String;
begin
	AppPath := '';
	AppId_x86 := '{67903736-E9BB-4664-B148-F62BCAB4FA42}';
	AppId_amd64 := '{9814C59D-8CBE-4C38-8A5F-7BF9B4FFDA6D}';
	if IsWin64() then
	begin
		if AppPath = '' then AppPath := GetAppPath(AppId_amd64, False);
		if AppPath = '' then AppPath := GetAppPath(AppId_x86, True);
	end
	else
	begin
		if AppPath = '' then AppPath := GetAppPath(AppId_x86, False);
	end;
	Result := AppPath;
end;

function HasPreviousSingleArchInstallPath: Boolean;
begin
	Result := GetPreviousSingleArchInstallPath() <> '';
end;




