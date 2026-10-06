#ifndef PublishDir
  #error PublishDir must point to the complete self-contained publish folder.
#endif
#ifndef OutputDir
  #define OutputDir "..\dist"
#endif

[Setup]
AppId={{37EEAF25-D97D-4F39-9D18-FD4702F47A2C}
AppName=VideoPlay
AppVersion=2.0.0-preview
AppVerName=VideoPlay 2.0 Preview
AppPublisher=ArcueidShiki
AppPublisherURL=https://github.com/ArcueidShiki/VideoPlay
DefaultDirName={localappdata}\Programs\VideoPlay
DefaultGroupName=VideoPlay
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
MinVersion=10.0.19041
WizardStyle=modern
SetupIconFile=..\WinUIPlayer\Assets\AppIcon.ico
UninstallDisplayIcon={app}\VideoPlay.exe
OutputDir={#OutputDir}
OutputBaseFilename=VideoPlay-2.0.0-preview-win-x64-setup
Compression=lzma2
SolidCompression=yes
CloseApplications=yes
RestartApplications=no

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Shortcuts:"; Flags: unchecked

[Files]
Source: "{#PublishDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "install-id.txt"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\VideoPlay"; Filename: "{app}\VideoPlay.exe"; WorkingDir: "{app}"; IconFilename: "{app}\VideoPlay.exe"
Name: "{autodesktop}\VideoPlay"; Filename: "{app}\VideoPlay.exe"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{app}\VideoPlay.exe"; Description: "Open VideoPlay"; Flags: nowait postinstall skipifsilent

[Code]
function DirectoryHasFiles(Path: String): Boolean;
var Item: TFindRec;
begin
  Result := False;
  if FindFirst(AddBackslash(Path) + '*', Item) then
  begin
    try
      repeat
        if (Item.Name <> '.') and (Item.Name <> '..') then
          Result := True;
      until Result or not FindNext(Item);
    finally
      FindClose(Item);
    end;
  end;
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
var InstallId: AnsiString;
begin
  Result := '';
  if DirectoryHasFiles(ExpandConstant('{app}')) then
  begin
    if not LoadStringFromFile(ExpandConstant('{app}\install-id.txt'), InstallId) or
       (Trim(InstallId) <> '37EEAF25-D97D-4F39-9D18-FD4702F47A2C') then
      Result := 'Choose a new, empty folder for VideoPlay. Existing files will be preserved.';
  end;
end;

{ Inno tracks installed files. No recursive uninstall deletion: user-added media stays. }
