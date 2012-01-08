[CustomMessages]
english.ProgressTitle=Searching
english.ProgressCaption=Searching for VST plugins
english.ProgressText=Searching for plugin files...

; http://www.vincenzo.net/isxkb/index.php?title=Search_for_a_file

[Code]
var
    ProgressPage:   TOutputProgressWizardPage;
    ProgressValue:  Integer;
    ArrayLen:       LongInt;
    bExitSetup:     Boolean;
    VSTPluginNumber: Integer;
    OldVSTPluginNumber: Integer;

procedure ProcessDirectory (RootDir: String; Progress: Boolean; INIFile: String);
var
    NewRoot:        String;
    FilePath:       String;
    FindRec:        TFindRec;
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
                        ProcessDirectory (FilePath, Progress, INIFile)
                    else if CompareFilename(FindRec.Name, '.dll') then
                    begin
                        // Start action -->
                        // .
                        // Add your custom code here.
                        // FilePath contains the file name
                        // including its full path name.
                        // Try not to call a function for every file
                        // as this could take a very long time.
                        // .
                        SetIniString('VST Plugins', 'Plugin' + IntToStr(VSTPluginNumber), FilePath, INIFile);
                        VSTPluginNumber := VSTPluginNumber + 1;
                        // <-- End action.
            
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

procedure OnVSTScan(INIFile: String);
var
    Dir:        String;
begin
    VSTPluginNumber := GetIniInt('VST Plugins', 'NumPlugins', 0, 0, 0, INIFile);
    OldVSTPluginNumber := VSTPluginNumber;

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
    ProcessDirectory (Dir, TRUE, INIFile);
    // Hide the progress page.
    try
    finally
        ProgressPage.Hide;
    end;

    // Update INI key

    if(VSTPluginNumber <> OldVSTPluginNumber) then
    begin
        SetIniInt('VST Plugins', 'NumPlugins', VSTPluginNumber, INIFile);
    end;
end;


