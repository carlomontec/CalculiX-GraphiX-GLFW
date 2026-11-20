#define MyAppName "CalculiX GraphiX GLFW"
#ifndef MyAppVersion
  #define MyAppVersion "2.23"
#endif
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
PrivilegesRequiredOverridesAllowed=dialog
AllowNoIcons=yes
LicenseFile=..\..\cgx_2.23\COPYING
OutputDir=..\..\dist
OutputBaseFilename=CalculiX-GraphiX-GLFW-Setup-windows-x86_64
SetupIconFile=cgx.ico
Compression=lzma
SolidCompression=yes
WizardStyle=modern
ChangesAssociations=yes
ChangesEnvironment=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "addtopath"; Description: "Add CalculiX GraphiX (cgx / cgx_glfw) to PATH"; GroupDescription: "Command Line:"

[Files]
Source: "..\..\bin\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\bin\{#MyAppExeName}"; DestDir: "{app}"; DestName: "cgx.exe"; Flags: ignoreversion
Source: "cgx.ico"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\cgx_2.23\COPYING"; DestDir: "{app}\licenses"; DestName: "COPYING.txt"; Flags: ignoreversion
Source: "..\..\THIRD_PARTY_LICENSES.md"; DestDir: "{app}\licenses"; Flags: ignoreversion

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
Root: HKA; Subkey: "Software\Classes\.fbl\OpenWithProgids"; ValueType: string; ValueName: "CalculiX.FBL"; ValueData: ""; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\CalculiX.FBL"; ValueType: string; ValueName: ""; ValueData: "CalculiX Batch File (.fbl)"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\CalculiX.FBL\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\cgx.ico,0"
Root: HKA; Subkey: "Software\Classes\CalculiX.FBL\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""-b"" ""%1"""

; .inp file association
Root: HKA; Subkey: "Software\Classes\.inp"; ValueType: string; ValueName: ""; ValueData: "CalculiX.INP"; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\.inp\OpenWithProgids"; ValueType: string; ValueName: "CalculiX.INP"; ValueData: ""; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\CalculiX.INP"; ValueType: string; ValueName: ""; ValueData: "CalculiX Input File (.inp)"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\CalculiX.INP\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\cgx.ico,0"
Root: HKA; Subkey: "Software\Classes\CalculiX.INP\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""-c"" ""%1"""

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[Code]
procedure EnvTarget(var RootKey: Integer; var SubKey: String);
begin
  if IsAdminInstallMode then
  begin
    RootKey := HKEY_LOCAL_MACHINE;
    SubKey := 'SYSTEM\CurrentControlSet\Control\Session Manager\Environment';
  end
  else
  begin
    RootKey := HKEY_CURRENT_USER;
    SubKey := 'Environment';
  end;
end;

procedure EnvAddPath(Dir: String);
var
  RootKey: Integer;
  SubKey, Paths: String;
begin
  EnvTarget(RootKey, SubKey);
  if not RegQueryStringValue(RootKey, SubKey, 'Path', Paths) then
    Paths := '';

  if Pos(';' + Uppercase(Dir) + ';', ';' + Uppercase(Paths) + ';') > 0 then
    exit;

  if (Paths <> '') and (Paths[Length(Paths)] <> ';') then
    Paths := Paths + ';';
  Paths := Paths + Dir;

  RegWriteExpandStringValue(RootKey, SubKey, 'Path', Paths);
end;

procedure EnvRemovePath(Dir: String);
var
  RootKey, P: Integer;
  SubKey, Paths: String;
begin
  EnvTarget(RootKey, SubKey);
  if not RegQueryStringValue(RootKey, SubKey, 'Path', Paths) then
    exit;

  P := Pos(';' + Uppercase(Dir) + ';', ';' + Uppercase(Paths) + ';');
  if P = 0 then
    exit;

  Paths := ';' + Paths + ';';
  StringChangeEx(Paths, ';' + Dir + ';', ';', True);
  if (Length(Paths) > 0) and (Paths[1] = ';') then
    Delete(Paths, 1, 1);
  if (Length(Paths) > 0) and (Paths[Length(Paths)] = ';') then
    Delete(Paths, Length(Paths), 1);

  RegWriteExpandStringValue(RootKey, SubKey, 'Path', Paths);
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if (CurStep = ssPostInstall) and WizardIsTaskSelected('addtopath') then
    EnvAddPath(ExpandConstant('{app}'));
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usPostUninstall then
    EnvRemovePath(ExpandConstant('{app}'));
end;
