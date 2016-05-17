; OpenMPT Install script - File associations
; Written by Johannes Schultz
; http://openmpt.org/
; http://sagamusix.de/

[Setup]
ChangesAssociations=yes

[Tasks]
; common file extensions
Name: "associate_common";      Description: "Associate OpenMPT with common module files"; GroupDescription: "File associations:";
Name: "associate_common\mod";  Description: "ProTracker / Noise Tracker / etc. (MOD)";
Name: "associate_common\s3m";  Description: "Scream Tracker 3 (S3M)";
Name: "associate_common\xm";   Description: "Fasttracker 2 (XM)";
Name: "associate_common\it";   Description: "Impulse Tracker (IT)";
Name: "associate_common\mptm"; Description: "OpenMPT (MPTM)";
; same, but compressed
Name: "associate_common\compressed"; Description: "Above when compressed (MDR, MDZ, S3Z, XMZ, ITZ, MPTMZ)";
; less common
Name: "associate_exotic";     Description: "Associate OpenMPT with less common module files"; GroupDescription: "File associations:";
Name: "associate_exotic\669"; Description: "Composer 669 / UNIS 669 (669)";
Name: "associate_exotic\amf"; Description: "ASYLUM Music Format / Advanced Music Format (AMF)";
Name: "associate_exotic\ams"; Description: "Extreme's Tracker / Velvet Studio (AMS)";
Name: "associate_exotic\dbm"; Description: "Digi Booster Pro (DBM)";
Name: "associate_exotic\digi"; Description: "Digi Booster (DIGI)";
Name: "associate_exotic\dmf"; Description: "X-Tracker (DMF)";
Name: "associate_exotic\dsm"; Description: "DSIK (DSM)";
Name: "associate_exotic\far"; Description: "Farandole Composer (FAR)";
Name: "associate_exotic\gdm"; Description: "General Digital Music (GDM)";
Name: "associate_exotic\imf"; Description: "Imago Orpheus (IMF)";
Name: "associate_exotic\ice"; Description: "Ice Tracker (ICE)";
Name: "associate_exotic\itp"; Description: "Impulse Tracker Project (ITP)";
Name: "associate_exotic\j2b"; Description: "Jazz Jackrabbit 2 Music (J2B)";
Name: "associate_exotic\m15"; Description: "Ultimate Soundtracker (M15)";
Name: "associate_exotic\mdl"; Description: "DigiTrakker (MDL)";
Name: "associate_exotic\med"; Description: "OctaMED (MED)";
Name: "associate_exotic\mms"; Description: "MultiMedia Sound (MMS)";
Name: "associate_exotic\mo3"; Description: "MO3 compressed modules (MO3)";
Name: "associate_exotic\mt2"; Description: "MadTracker 2 (MT2)";
Name: "associate_exotic\mtm"; Description: "MultiTracker Modules (MTM)";
Name: "associate_exotic\okt"; Description: "Oktalyzer (OKT)";
Name: "associate_exotic\plm"; Description: "Disorder Tracker 2 (PLM)";
Name: "associate_exotic\psm"; Description: "Epic Megagames MASI (PSM)";
Name: "associate_exotic\pt36"; Description: "ProTracker 3.6 (PT36)";
Name: "associate_exotic\ptm"; Description: "PolyTracker (PTM)";
Name: "associate_exotic\sfx"; Description: "SoundFX (SFX)";
Name: "associate_exotic\sfx2"; Description: "SoundFX 2 (SFX2)";
Name: "associate_exotic\st26"; Description: "SoundTracker 2.6 (ST26)";
Name: "associate_exotic\stm"; Description: "Scream Tracker 2 (STM)";
Name: "associate_exotic\ult"; Description: "UltraTracker (ULT)";
Name: "associate_exotic\umx"; Description: "Unreal Music (UMX)";
Name: "associate_exotic\wow"; Description: "Grave Composer (WOW)";

[Registry]
; common file extensions
Root: HKCR; Subkey: ".mod";  ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_common\mod
Root: HKCR; Subkey: ".s3m";  ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_common\s3m
Root: HKCR; Subkey: ".xm";   ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_common\xm
Root: HKCR; Subkey: ".it";   ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_common\it
Root: HKCR; Subkey: ".mptm"; ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_common\mptm
; same, but compressed
Root: HKCR; Subkey: ".mdr";   ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_common\compressed
Root: HKCR; Subkey: ".mdz";   ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_common\compressed
Root: HKCR; Subkey: ".s3z";   ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_common\compressed
Root: HKCR; Subkey: ".xmz";   ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_common\compressed
Root: HKCR; Subkey: ".itz";   ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_common\compressed
Root: HKCR; Subkey: ".mptmz"; ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_common\compressed
; less common
Root: HKCR; Subkey: ".669"; ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_exotic\669
Root: HKCR; Subkey: ".amf"; ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_exotic\amf
Root: HKCR; Subkey: ".ams"; ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_exotic\ams
Root: HKCR; Subkey: ".dbm"; ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_exotic\dbm
Root: HKCR; Subkey: ".digi"; ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_exotic\digi
Root: HKCR; Subkey: ".dmf"; ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_exotic\dmf
Root: HKCR; Subkey: ".dsm"; ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_exotic\dsm
Root: HKCR; Subkey: ".far"; ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_exotic\far
Root: HKCR; Subkey: ".gdm"; ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_exotic\gdm
Root: HKCR; Subkey: ".imf"; ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_exotic\imf
Root: HKCR; Subkey: ".ice"; ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_exotic\ice
Root: HKCR; Subkey: ".itp"; ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_exotic\itp
Root: HKCR; Subkey: ".j2b"; ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_exotic\j2b
Root: HKCR; Subkey: ".m15"; ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_exotic\m15
Root: HKCR; Subkey: ".mdl"; ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_exotic\mdl
Root: HKCR; Subkey: ".med"; ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_exotic\med
Root: HKCR; Subkey: ".mms"; ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_exotic\mms
Root: HKCR; Subkey: ".mo3"; ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_exotic\mo3
Root: HKCR; Subkey: ".mt2"; ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_exotic\mt2
Root: HKCR; Subkey: ".mtm"; ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_exotic\mtm
Root: HKCR; Subkey: ".okt"; ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_exotic\okt
Root: HKCR; Subkey: ".plm"; ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_exotic\plm
Root: HKCR; Subkey: ".psm"; ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_exotic\psm
Root: HKCR; Subkey: ".pt36"; ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_exotic\pt36
Root: HKCR; Subkey: ".ptm"; ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_exotic\ptm
Root: HKCR; Subkey: ".sfx"; ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_exotic\sfx
Root: HKCR; Subkey: ".sfx2"; ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_exotic\sfx2
Root: HKCR; Subkey: ".st26"; ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_exotic\st26
Root: HKCR; Subkey: ".stm"; ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_exotic\stm
Root: HKCR; Subkey: ".ult"; ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_exotic\ult
Root: HKCR; Subkey: ".umx"; ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_exotic\umx
Root: HKCR; Subkey: ".wow"; ValueType: string; ValueName: ""; ValueData: "OpenMPTFile"; Flags: uninsdeletevalue; Tasks: associate_exotic\wow

; important (setup)
Root: HKCR; Subkey: "OpenMPTFile"; ValueType: string; ValueName: ""; ValueData: "OpenMPT Module"; Flags: uninsdeletekey; Tasks: associate_common or associate_exotic
Root: HKCR; SubKey: "OpenMPTFile"; ValueType: string; ValueName: "PerceivedType"; ValueData: "audio"; Tasks: associate_common or associate_exotic                                                                                                           
Root: HKCR; Subkey: "OpenMPTFile\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\mpt.ico,0"; Tasks: associate_common or associate_exotic                                                                                                           
Root: HKCR; Subkey: "OpenMPTFile\shell\Open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\mptrack.exe"" ""%1"""; Tasks: associate_common or associate_exotic
Root: HKCR; Subkey: "OpenMPTFile\shell\Open\ddeexec"; ValueType: string; ValueName: ""; ValueData: "[Edit(""%1"")]"; Tasks: associate_common or associate_exotic

[Files]
; icon file (should be moved into EXE)
Source: "..\packageTemplate\mpt.ico";            DestDir: "{app}"; Flags: ignoreversion

