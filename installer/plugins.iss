[Code]

// Helper code for installing the plugins that are shipped with OpenMPT.

// Register plugins in OpenMPT's plugin settings
procedure RegisterPlugin(plugName : String);
var
    totalNum : Integer;
    i : Integer;
    INIFile : String;
    plugPath: String;
begin

    INIFile := GetINIPath();
    totalNum := GetIniInt('VST Plugins', 'NumPlugins', 0, 0, 0, INIFile);

    // Check if the plugin has already been registered with OpenMPT.
    for i := 0 to totalNum - 1 do
    begin
        plugPath := GetIniString('VST Plugins', Format('Plugin%d', [i]), '', INIFile);
        if(CompareFilename(plugPath, plugName)) then
        begin
            // The plugin is already registered, so don't add it again.
            Exit;
        end;    
    end;

    // In portable mode, write a relative path into the config file.
    if(IsTaskSelected('portable')) then
    begin
        plugPath := '.';
    end else
    begin
        plugPath := ExpandConstant('{app}');
    end;
    SetIniString('VST Plugins', Format('Plugin%d', [totalNum]), plugPath + '\Plugins\' + plugName, INIFile);

    SetIniInt('VST Plugins', 'NumPlugins', totalNum + 1, INIFile);

end;
