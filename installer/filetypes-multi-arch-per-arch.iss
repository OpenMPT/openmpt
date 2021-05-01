


[Registry]



Root: HKA; Subkey: "SOFTWARE\Classes\{#OpenMPTFile}"; Flags: deletekey uninsdeletekey; Components: {#Component}

Root: HKA; Subkey: "SOFTWARE\Classes\{#OpenMPTFile}"; ValueType: string; ValueName: ""; ValueData: "{#OpenMPTarchFriendly} Module"; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Classes\{#OpenMPTFile}"; ValueType: string; ValueName: "PerceivedType"; ValueData: "audio"; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Classes\{#OpenMPTFile}\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\OpenMPT File Icon.ico,0"; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Classes\{#OpenMPTFile}\shell\Open"; ValueType: string; ValueName: "MultiSelectModel"; ValueData: "Player"; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Classes\{#OpenMPTFile}\shell\Open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\bin\{#OpenMPTarchPath}\OpenMPT.exe"" /shared ""%1"""; Components: {#Component}



Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}"; Flags: deletekey uninsdeletekey; Components: {#Component}

Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities"; ValueType: string; ValueName: "ApplicationDescription"; ValueData: "{#OpenMPTarchFriendly} is an application for editing and playing tracked music in various formats."; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities"; ValueType: string; ValueName: "ApplicationName"; ValueData: "{#OpenMPTarchFriendly}"; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities"; ValueType: string; ValueName: "ApplicationIcon"; ValueData: "{app}\OpenMPT App Icon.ico,0"; Components: {#Component}

Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\MIMEAssociations"; ValueType: string; ValueName: "audio/it"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\MIMEAssociations"; ValueType: string; ValueName: "audio/xm"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\MIMEAssociations"; ValueType: string; ValueName: "audio/s3m"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\MIMEAssociations"; ValueType: string; ValueName: "audio/x-s3m"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\MIMEAssociations"; ValueType: string; ValueName: "audio/x-zipped-mod"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\MIMEAssociations"; ValueType: string; ValueName: "audio/x-zipped-it"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\MIMEAssociations"; ValueType: string; ValueName: "audio/x-tracker-module"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\MIMEAssociations"; ValueType: string; ValueName: "audio/x-screamtracker-module"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\MIMEAssociations"; ValueType: string; ValueName: "audio/x-protracker-module"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\MIMEAssociations"; ValueType: string; ValueName: "audio/x-startracker-module"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\MIMEAssociations"; ValueType: string; ValueName: "audio/x-fasttracker-module"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\MIMEAssociations"; ValueType: string; ValueName: "audio/x-oktalyzer-tracker-module"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\MIMEAssociations"; ValueType: string; ValueName: "audio/x-taketracker-module"; ValueData: {#OpenMPTFile}; Components: {#Component}

Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".mod"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".s3m"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".xm"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".it"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".mptm"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".mdr"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".mdz"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".s3z"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".xmz"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".itz"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".mptmz"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".669"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".amf"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".ams"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".c67"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".dbm"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".digi"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".dmf"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".dsm"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".dsym"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".dtm"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".far"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".fmt"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".gdm"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".imf"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".ice"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".itp"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".j2b"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".m15"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".mdl"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".med"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".mms"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".mo3"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".mt2"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".mtm"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".mus"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".okt"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".oxm"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".plm"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".psm"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".pt36"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".ptm"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".sfx"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".sfx2"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".st26"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".stm"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".stp"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".symmod"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".ult"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".umx"; ValueData: {#OpenMPTFile}; Components: {#Component}
Root: HKA; Subkey: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".wow"; ValueData: {#OpenMPTFile}; Components: {#Component}

Root: HKA; Subkey: "SOFTWARE\RegisteredApplications"; ValueType: string; ValueName: {#OpenMPTarchFriendly}; ValueData: "SOFTWARE\Clients\Media\{#OpenMPTarch}\Capabilities"; Flags: uninsdeletevalue; Components: {#Component}


