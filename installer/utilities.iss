[Code]

// Misc utility functions

// Get the right INI path.
function GetINIPath() : String;
begin
    if(IsTaskSelected('portable')) then
    begin
        Result := ExpandConstant('{app}\mptrack.ini');
    end else
    begin
        Result := ExpandConstant('{userappdata}\OpenMPT\mptrack.ini');
    end;
end;

// Checks whether the right part of a file path (f.e. 'C:\foo.bar') matches a (partial) file name (f.e. '.bar'). The comparison is case insensitive.
function CompareFilename(fullPath, fileName : String) : Boolean;
begin
    Result := (CompareText(fileName, Copy(fullPath, Length(fullPath) - Length(fileName) + 1, Length(fileName))) = 0);
end;