[CustomMessages]
english.ProgressTitle=Searching
english.ProgressCaption=Searching for VST plugins
english.ProgressText=Searching for plugin files...

; http://www.vincenzo.net/isxkb/index.php?title=Search_for_a_file

[Code]
var
	ProgressPage:	TOutputProgressWizardPage;
	ProgressValue:	Integer;
	ArrayLen:		LongInt;
	bExitSetup:		Boolean;
  INIFile: String;
  VSTPluginNumber: Integer;

procedure ProcessDirectory (RootDir: String; Progress: Boolean);
var
	NewRoot:		String;
	FilePath:		String;
	FindRec:		TFindRec;
begin
	if bExitSetup then
		Exit;
	NewRoot := AddBackSlash (RootDir);
	if FindFirst (NewRoot + '*', FindRec) then
	begin
		try
			repeat
				if (FindRec.Name <> '.') AND (FindRec.Name <> '..') then
				begin
					FilePath := NewRoot + FindRec.Name;
					if FindRec.Attributes AND FILE_ATTRIBUTE_DIRECTORY > 0 then
						ProcessDirectory (FilePath, Progress)
					else
					begin
						// Start action -->
						// .
						// Add your custom code here.
						// FilePath contains the file name
						// including its full path name.
						// Try not to call a function for every file
						// as this could take a very long time.
						// .
						// <-- End action.
            SetIniString('VST Plugins', 'Plugin' + IntToStr(VSTPluginNumber), FilePath, INIFile);
            VSTPluginNumber := VSTPluginNumber +1;

						ArrayLen := ArrayLen + 1;
						if (Progress) then
						begin
							if (ArrayLen mod 1000) = (ArrayLen / 1000) then
							begin
								ProgressValue := ProgressValue + 1;
								if ProgressValue = 100 then
									ProgressValue := 0;
								ProgressPage.SetProgress (ProgressValue, 100);
							end;
						end;
					end;
				end;
				if (bExitSetup) then
					Exit;
			until NOT FindNext (FindRec);
		finally
			FindClose(FindRec);
		end;
	end;
end;

function MessageBox (hWnd: Integer; lpText, lpCaption: String; uType: Cardinal): Integer;
 external 'MessageBoxA@user32.dll stdcall';

procedure CancelButtonClick (CurPageID: Integer; var Cancel, Confirm: Boolean);
begin
	Confirm := FALSE;
	if (MessageBox (0, SetupMessage (msgExitSetupMessage),
		SetupMessage (msgExitSetupTitle), 4 + 32) = 6) then
	begin
		bExitSetup := TRUE;
	end;
end;

procedure CurStepChanged (CurStep: TSetupStep);
var
	Dir:		String;
begin
	if ((CurStep = ssInstall) And (IsTaskSelected('vst_scan'))) then
	begin

    // Get the right INI path.
    if(IsTaskSelected('portable')) then
    begin
        INIFile := ExpandConstant('{app}\mptrack.ini');
    end else
    begin
        INIFile := ExpandConstant('{userappdata}\OpenMPT\mptrack.ini');
    end;
    VSTPluginNumber := GetIniInt('VST Plugins', 'NumPlugin', 0, 0, 0, INIFile);

		// The folder to scan.
    Dir := ExpandConstant('{pf}\Steinberg\VstPlugins');
    RegQueryStringValue(HKEY_LOCAL_MACHINE, 'Software\VST', 'VSTPluginsPath', Dir); // won't touch Dir if registry path does not exist
		// The progress page.
		ProgressPage := CreateOutputProgressPage (CustomMessage ('ProgressTitle'),
			CustomMessage ('ProgressCaption'));
		ProgressPage.SetText (CustomMessage ('ProgressText'), Dir);
		ProgressPage.SetProgress(0, 0);
		ProgressPage.Show;
		// Make the Cancel button visible during the operation.
		;WizardForm.CancelButton.Visible := TRUE;
		// Scan the folder.
		ProcessDirectory (Dir, TRUE);
		// Hide the progress page.
		try
		finally
			ProgressPage.Hide;
		end;

    // Update INI key
    SetIniInt('VST Plugins', 'NumPlugin', VSTPluginNumber, INIFile);
    end;
end;

