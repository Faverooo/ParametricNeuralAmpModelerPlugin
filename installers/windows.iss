#ifndef PluginVersion
  #error Define PluginVersion when compiling this installer.
#endif
#ifndef BundleDir
  #error Define BundleDir when compiling this installer.
#endif

[Setup]
AppId=Faverooo.ParametricNAM.VST3
AppName=ParametricNAM VST3
AppVersion={#PluginVersion}
AppPublisher=Faverooo
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
DisableProgramGroupPage=yes
PrivilegesRequired=admin
OutputBaseFilename=ParametricNAM-VST3-{#PluginVersion}-Windows-X64-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName=ParametricNAM VST3
UninstallFilesDir={commonappdata}\Faverooo\ParametricNAM\Uninstall

[Files]
Source: "{#BundleDir}\*"; DestDir: "{app}\ParametricNAM.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
