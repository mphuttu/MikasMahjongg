#ifndef RedistRoot
  #define RedistRoot "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Redist\MSVC\14.51.36231\x64"
#endif

[Setup]
AppId={{1BA9BB1B-5C07-4075-9D91-820EC23E9A44}
AppName=MikasMahjongg
AppVersion=1.0
AppPublisher=Mika Huttunen
DefaultDirName={localappdata}\Programs\MikasMahjongg
DefaultGroupName=MikasMahjongg
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
OutputDir=dist
OutputBaseFilename=MikasMahjongg-Setup-x64
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayIcon={app}\MikasMahjongg.exe

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"
Name: "fi"; MessagesFile: "compiler:Languages\Finnish.isl"

[Files]
Source: "x64\Release\MikasMahjongg.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "Ohje-fi.chm"; DestDir: "{app}"; Flags: ignoreversion
Source: "Help-en.chm"; DestDir: "{app}"; Flags: ignoreversion
Source: "CREDITS.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "res\tiles\*.png"; DestDir: "{app}\res\tiles"; Flags: ignoreversion
Source: "{#RedistRoot}\Microsoft.VC145.CRT\msvcp140.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#RedistRoot}\Microsoft.VC145.CRT\vcruntime140.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#RedistRoot}\Microsoft.VC145.CRT\vcruntime140_1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#RedistRoot}\Microsoft.VC145.MFC\mfc140u.dll"; DestDir: "{app}"; Flags: ignoreversion

[INI]
Filename: "{app}\language.ini"; Section: "Language"; Key: "Code"; String: "{language}"

[Icons]
Name: "{group}\MikasMahjongg"; Filename: "{app}\MikasMahjongg.exe"

[Run]
Filename: "{app}\MikasMahjongg.exe"; Description: "{cm:LaunchProgram,MikasMahjongg}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: files; Name: "{app}\language.ini"
