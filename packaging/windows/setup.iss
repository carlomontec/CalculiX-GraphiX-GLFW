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
UninstallDisplayIcon={app}\cgx.ico
UninstallDisplayName={#MyAppName}
CloseApplications=yes
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
const
  UninstallRegKey = 'Software\Microsoft\Windows\CurrentVersion\Uninstall\{9F784F34-8A87-4C3F-982B-5E77114A1D9A}_is1';

var
  MaintenancePage: TInputOptionWizardPage;
  ExistingUninsExe: String;
  ExistingInstallDir: String;
  ExistingVersion: String;
  UninstalledViaMaintenance: Boolean;

function TryReadUninstallKey(RootKey: Integer; var UninsExe, InstallDir, DisplayVer: String): Boolean;
var
  RawUnins, RawLoc, RawVer: String;
begin
  Result := False;
  UninsExe := '';
  InstallDir := '';
  DisplayVer := '';

  if not RegQueryStringValue(RootKey, UninstallRegKey, 'UninstallString', RawUnins) then
    exit;

  UninsExe := RemoveQuotes(Trim(RawUnins));
  if (UninsExe = '') or (not FileExists(UninsExe)) then
  begin
    UninsExe := '';
    exit;
  end;

  if RegQueryStringValue(RootKey, UninstallRegKey, 'InstallLocation', RawLoc) then
    InstallDir := RemoveBackslashUnlessRoot(Trim(RawLoc));

  if RegQueryStringValue(RootKey, UninstallRegKey, 'DisplayVersion', RawVer) then
    DisplayVer := Trim(RawVer);

  Result := True;
end;

function FindExistingInstall(var UninsExe, InstallDir, DisplayVer: String): Boolean;
begin
  Result := False;
  if IsAdminInstallMode then
  begin
    if TryReadUninstallKey(HKLM32, UninsExe, InstallDir, DisplayVer) then
    begin
      Result := True;
      exit;
    end;
    if IsWin64 and TryReadUninstallKey(HKLM64, UninsExe, InstallDir, DisplayVer) then
    begin
      Result := True;
      exit;
    end;
    if TryReadUninstallKey(HKCU, UninsExe, InstallDir, DisplayVer) then
    begin
      Result := True;
      exit;
    end;
  end
  else
  begin
    if TryReadUninstallKey(HKCU, UninsExe, InstallDir, DisplayVer) then
    begin
      Result := True;
      exit;
    end;
    if TryReadUninstallKey(HKLM32, UninsExe, InstallDir, DisplayVer) then
    begin
      Result := True;
      exit;
    end;
    if IsWin64 and TryReadUninstallKey(HKLM64, UninsExe, InstallDir, DisplayVer) then
    begin
      Result := True;
      exit;
    end;
  end;
end;

procedure InitializeWizard();
var
  SubCaption, LocationInfo: String;
begin
  UninstalledViaMaintenance := False;
  MaintenancePage := nil;

  if FindExistingInstall(ExistingUninsExe, ExistingInstallDir, ExistingVersion) and (not WizardSilent) then
  begin
    LocationInfo := ExistingInstallDir;
    if LocationInfo = '' then
      LocationInfo := ExtractFileDir(ExistingUninsExe);

    SubCaption := 'Detected installation: ' + LocationInfo;
    if ExistingVersion <> '' then
      SubCaption := SubCaption + ' (version ' + ExistingVersion + ')';
    SubCaption := SubCaption + '.' + #13#10 + #13#10 + 'Choose whether to reinstall/upgrade or uninstall:';

    MaintenancePage := CreateInputOptionPage(
      wpWelcome,
      'Existing Installation Detected',
      'CalculiX GraphiX GLFW is already installed on this computer.',
      SubCaption,
      True,
      False
    );
    MaintenancePage.Add('Reinstall or upgrade CalculiX GraphiX GLFW');
    MaintenancePage.Add('Uninstall CalculiX GraphiX GLFW');
    MaintenancePage.SelectedValueIndex := 0;
  end;
end;

function NextButtonClick(CurPageID: Integer): Boolean;
var
  ResultCode: Integer;
  DummyUnins, DummyDir, DummyVer: String;
begin
  Result := True;

  if (MaintenancePage <> nil) and (CurPageID = MaintenancePage.ID) and (MaintenancePage.SelectedValueIndex = 1) then
  begin
    if not Exec(ExistingUninsExe, '', '', SW_SHOWNORMAL, ewWaitUntilTerminated, ResultCode) then
    begin
      ShellExec('runas', ExistingUninsExe, '', '', SW_SHOWNORMAL, ewWaitUntilTerminated, ResultCode);
    end;

    if not FindExistingInstall(DummyUnins, DummyDir, DummyVer) then
    begin
      UninstalledViaMaintenance := True;
      WizardForm.Close;
    end;
    Result := False;
  end;
end;

procedure CancelButtonClick(CurPageID: Integer; var Cancel, Confirm: Boolean);
begin
  if UninstalledViaMaintenance then
  begin
    Cancel := True;
    Confirm := False;
  end;
end;

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
  SubKey, Paths, CleanDir, UpperPaths, UpperClean: String;
begin
  CleanDir := RemoveBackslashUnlessRoot(Trim(Dir));
  if CleanDir = '' then
    exit;

  EnvTarget(RootKey, SubKey);
  if not RegQueryStringValue(RootKey, SubKey, 'Path', Paths) then
    Paths := '';

  UpperPaths := ';' + Uppercase(Paths) + ';';
  UpperClean := Uppercase(CleanDir);
  if (Pos(';' + UpperClean + ';', UpperPaths) > 0) or (Pos(';' + UpperClean + '\;', UpperPaths) > 0) then
    exit;

  if (Paths <> '') and (Paths[Length(Paths)] <> ';') then
    Paths := Paths + ';';
  Paths := Paths + CleanDir;

  RegWriteExpandStringValue(RootKey, SubKey, 'Path', Paths);
end;

procedure EnvRemovePath(Dir: String);
var
  RootKey: Integer;
  SubKey, Paths, CleanDir: String;
begin
  CleanDir := RemoveBackslashUnlessRoot(Trim(Dir));
  if CleanDir = '' then
    exit;

  EnvTarget(RootKey, SubKey);
  if not RegQueryStringValue(RootKey, SubKey, 'Path', Paths) then
    exit;

  Paths := ';' + Paths + ';';
  StringChangeEx(Paths, ';' + CleanDir + '\;', ';', True);
  StringChangeEx(Paths, ';' + CleanDir + ';', ';', True);
  while Pos(';;', Paths) > 0 do
    StringChangeEx(Paths, ';;', ';', False);

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
