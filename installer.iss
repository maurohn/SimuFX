; =============================================================================
;  SimuFX Installer — Inno Setup Script
;  Post-processing engine + SimuConfig for SimuV3 (rFactor 1)
; =============================================================================

#define MyAppName "SimuFX"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "SimuV3 Team"
#define MyAppURL "https://github.com/maurohn/SimuFX"

[Setup]
AppId={{B3F7A2E1-4C8D-4F9A-A1B2-D3E4F5A6B7C8}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
DefaultDirName={code:GetSimuV3Path}
DefaultGroupName={#MyAppName}
DisableDirPage=no
DisableProgramGroupPage=yes
OutputDir=installer_output
OutputBaseFilename=SimuFX_Setup_{#MyAppVersion}
SetupIconFile=SimuV3.ico
UninstallDisplayIcon={app}\SimuV3.ico
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x86compatible
PrivilegesRequired=lowest

[Languages]
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Messages]
spanish.WelcomeLabel2=Este asistente instalará SimuFX — motor de post-procesado profesional para SimuV3.%n%nSe hará un backup automático de d3d9.dll antes de reemplazarlo.
english.WelcomeLabel2=This wizard will install SimuFX — professional post-processing engine for SimuV3.%n%nA backup of d3d9.dll will be created automatically before replacing it.

[Types]
Name: "full"; Description: "Instalación completa (SimuFX + SimuConfig)"
Name: "minimal"; Description: "Solo SimuFX (sin configurador)"
Name: "custom"; Description: "Personalizada"; Flags: iscustom

[Components]
Name: "core"; Description: "SimuFX Engine (d3d9.dll proxy + shaders)"; Types: full minimal custom; Flags: fixed
Name: "config"; Description: "SimuConfig — Configurador visual"; Types: full custom
Name: "presets"; Description: "Presets de post-procesado"; Types: full custom

[Files]
; === Core engine ===
Source: "dist\d3d9.dll"; DestDir: "{app}"; Components: core; Flags: ignoreversion
Source: "dist\D3DCOMPILER_47.dll"; DestDir: "{app}"; Components: core; Flags: ignoreversion
Source: "dist\msvcp140.dll"; DestDir: "{app}"; Components: core; Flags: ignoreversion
Source: "dist\msvcp140_1.dll"; DestDir: "{app}"; Components: core; Flags: ignoreversion
Source: "dist\vcruntime140.dll"; DestDir: "{app}"; Components: core; Flags: ignoreversion

; === Shaders ===
Source: "dist\shaders\*.hlsl"; DestDir: "{app}\shaders"; Components: core; Flags: ignoreversion

; === SimuFX config ===
Source: "dist\SimuFX\global.ini"; DestDir: "{app}\SimuFX"; Components: core; Flags: onlyifdoesntexist
Source: "dist\SimuFX\presets\*.ini"; DestDir: "{app}\SimuFX\presets"; Components: presets; Flags: ignoreversion

; === SimuConfig ===
Source: "dist\SimuConfig.exe"; DestDir: "{app}"; Components: config; Flags: ignoreversion
Source: "dist\SimuV3.ico"; DestDir: "{app}"; Components: core; Flags: ignoreversion

[Icons]
Name: "{group}\SimuConfig"; Filename: "{app}\SimuConfig.exe"; IconFilename: "{app}\SimuV3.ico"; Components: config
Name: "{group}\Desinstalar SimuFX"; Filename: "{uninstallexe}"
Name: "{commondesktop}\SimuConfig"; Filename: "{app}\SimuConfig.exe"; IconFilename: "{app}\SimuV3.ico"; Components: config; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Crear acceso directo en el Escritorio"; Components: config

[Run]
Filename: "{app}\SimuConfig.exe"; Description: "Abrir SimuConfig ahora"; Flags: nowait postinstall skipifsilent; Components: config

[UninstallRun]
; Restore backup on uninstall
Filename: "cmd.exe"; Parameters: "/c if exist ""{app}\d3d9.dll.backup"" (copy /Y ""{app}\d3d9.dll.backup"" ""{app}\d3d9.dll"" && del ""{app}\d3d9.dll.backup"")"; Flags: runhidden

[UninstallDelete]
Type: filesandordirs; Name: "{app}\shaders"
Type: files; Name: "{app}\SimuV3.ico"

[Code]
// ─── Auto-detect SimuV3 installation path ───────────────────────────────

function GetSteamPath(): String;
var
  Path: String;
begin
  Result := '';
  // Try registry (Steam default)
  if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Valve\Steam', 'InstallPath', Path) then
    Result := Path
  else if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\WOW6432Node\Valve\Steam', 'InstallPath', Path) then
    Result := Path
  else if RegQueryStringValue(HKEY_CURRENT_USER, 'SOFTWARE\Valve\Steam', 'SteamPath', Path) then
  begin
    StringChange(Path, '/', '\');
    Result := Path;
  end;
end;

function FindSimuV3InLibraryFolders(SteamPath: String): String;
var
  VdfFile, Line, LibPath, Candidate: String;
  Lines: TArrayOfString;
  I: Integer;
begin
  Result := '';

  // Check default Steam library
  Candidate := SteamPath + '\steamapps\common\SimuV3';
  if FileExists(Candidate + '\Config.ini') then
  begin
    Result := Candidate;
    Exit;
  end;

  // Parse libraryfolders.vdf for additional library paths
  VdfFile := SteamPath + '\steamapps\libraryfolders.vdf';
  if not FileExists(VdfFile) then Exit;

  if LoadStringsFromFile(VdfFile, Lines) then
  begin
    for I := 0 to GetArrayLength(Lines) - 1 do
    begin
      Line := Trim(Lines[I]);
      if Pos('"path"', Line) > 0 then
      begin
        // Extract path from: "path"    "D:\SteamLibrary"
        LibPath := Copy(Line, Pos('"path"', Line) + 7, Length(Line));
        LibPath := Trim(LibPath);
        // Remove quotes
        StringChange(LibPath, '"', '');
        LibPath := Trim(LibPath);
        // Unescape double backslashes
        StringChange(LibPath, '\\', '\');

        Candidate := LibPath + '\steamapps\common\SimuV3';
        if FileExists(Candidate + '\Config.ini') then
        begin
          Result := Candidate;
          Exit;
        end;
      end;
    end;
  end;
end;

function GetSimuV3Path(Param: String): String;
var
  SteamPath: String;
begin
  SteamPath := GetSteamPath();
  if SteamPath <> '' then
    Result := FindSimuV3InLibraryFolders(SteamPath);

  // Hardcoded fallbacks
  if Result = '' then
  begin
    if FileExists('D:\SteamLibrary\steamapps\common\SimuV3\Config.ini') then
      Result := 'D:\SteamLibrary\steamapps\common\SimuV3'
    else if FileExists('C:\SteamLibrary\steamapps\common\SimuV3\Config.ini') then
      Result := 'C:\SteamLibrary\steamapps\common\SimuV3'
    else if FileExists('C:\Program Files (x86)\Steam\steamapps\common\SimuV3\Config.ini') then
      Result := 'C:\Program Files (x86)\Steam\steamapps\common\SimuV3'
    else
      Result := 'C:\SimuV3';
  end;
end;

// ─── Backup d3d9.dll before install ─────────────────────────────────────

function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  GamePath, OrigDll, BackupDll: String;
begin
  Result := '';
  GamePath := WizardDirValue();
  OrigDll := GamePath + '\d3d9.dll';
  BackupDll := GamePath + '\d3d9.dll.backup';

  if FileExists(OrigDll) and (not FileExists(BackupDll)) then
  begin
    Log('Backing up original d3d9.dll → d3d9.dll.backup');
    if not FileCopy(OrigDll, BackupDll, False) then
    begin
      Result := 'No se pudo hacer backup de d3d9.dll. Cerrá el juego e intentá de nuevo.';
      Exit;
    end;
    Log('Backup created successfully.');
  end
  else if FileExists(BackupDll) then
    Log('Backup already exists, skipping.')
  else
    Log('No original d3d9.dll found (fresh install).');
end;

// ─── Validate selected directory ────────────────────────────────────────

function NextButtonClick(CurPageID: Integer): Boolean;
begin
  Result := True;
  if CurPageID = wpSelectDir then
  begin
    if not FileExists(WizardDirValue() + '\Config.ini') then
    begin
      if MsgBox('No se encontró Config.ini en esta carpeta.' + #13#10 +
                '¿Estás seguro de que es la carpeta correcta de SimuV3?',
                mbConfirmation, MB_YESNO) = IDNO then
        Result := False;
    end;
  end;
end;
