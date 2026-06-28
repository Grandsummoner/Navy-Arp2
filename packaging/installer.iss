#define MyAppName "Navy Arp"
#define MyAppVersion "0.0.1"
#define MyAppPublisher "Navy Audio"
#define MyAppURL "https://www.navyaudio.com"
#define MyAppExeName "Navy Arp.exe"

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
AppId={{5D2AE2D6-8F8D-4F62-8356-EFA8A8B426BF}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={commonpf64}\{#MyAppPublisher}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
OutputDir=..\Artifacts
OutputBaseFilename=Navy_Arp_Installer
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "vst3"; Description: "Install VST3 Plugin (64-bit)"; GroupDescription: "Plugin Formats:"
Name: "standalone"; Description: "Install Standalone Application (64-bit)"; GroupDescription: "Plugin Formats:"

[Files]
; Standalone Executable (Reads from the root Artifacts directory)
Source: "..\Artifacts\Standalone\Navy Arp.exe"; DestDir: "{app}"; Flags: ignoreversion; Tasks: standalone
; VST3 Plugin Folder and its contents (Recursive copy from root Artifacts directory)
Source: "..\Artifacts\VST3\*"; DestDir: "{commoncf64}\VST3\Navy Arp.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs; Tasks: vst3

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: standalone
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon and standalone

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent; Tasks: standalone
