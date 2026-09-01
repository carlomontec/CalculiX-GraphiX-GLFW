#define MyAppName "CalculiX GraphiX GLFW"
#define MyAppVersion "2.23"
#define MyAppPublisher "CalculiX Open Source Community"
#define MyAppURL "https://github.com/carlomontec/CalculiX-GraphiX-GLFW"
#define MyAppExeName "cgx_glfw.exe"

[Setup]
AppId={{9F784F34-8A87-4C3F-982B-5E77114A1D9A}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\CalculiX\{#MyAppName}
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
OutputDir=..\..\dist
OutputBaseFilename=CalculiX-GraphiX-GLFW-Setup-windows-x86_64
SetupIconFile=cgx.ico
Compression=lzma
SolidCompression=yes
WizardStyle=modern
ChangesAssociations=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "..\..\bin\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "cgx.ico"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\cgx.ico"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\cgx.ico"; Tasks: desktopicon

[Registry]
; .frd file association
Root: HKA; Subkey: "Software\Classes\.frd"; ValueType: string; ValueName: ""; ValueData: "CalculiX.FRD"; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\.frd\OpenWithProgids"; ValueType: string; ValueName: "CalculiX.FRD"; ValueData: ""; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\CalculiX.FRD"; ValueType: string; ValueName: ""; ValueData: "CalculiX Results File (.frd)"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\CalculiX.FRD\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\cgx.ico,0"
Root: HKA; Subkey: "Software\Classes\CalculiX.FRD\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""

; .fbl batch file association
Root: HKA; Subkey: "Software\Classes\.fbl"; ValueType: string; ValueName: ""; ValueData: "CalculiX.FBL"; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\CalculiX.FBL"; ValueType: string; ValueName: ""; ValueData: "CalculiX Batch File (.fbl)"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\CalculiX.FBL\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\cgx.ico,0"
Root: HKA; Subkey: "Software\Classes\CalculiX.FBL\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""-b"" ""%1"""

; .inp file association
Root: HKA; Subkey: "Software\Classes\.inp"; ValueType: string; ValueName: ""; ValueData: "CalculiX.INP"; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\CalculiX.INP"; ValueType: string; ValueName: ""; ValueData: "CalculiX Input File (.inp)"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\CalculiX.INP\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\cgx.ico,0"
Root: HKA; Subkey: "Software\Classes\CalculiX.INP\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""-c"" ""%1"""

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
