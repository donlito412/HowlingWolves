$ErrorActionPreference = "Stop"
$PluginName = "Howling Wolves"
$BuildRoot = "build\HowlingWolves_artefacts"
$StagingDir = "Installer\staging_win"
$OutputDir = "."

New-Item -ItemType Directory -Force -Path $StagingDir
New-Item -ItemType Directory -Force -Path "$StagingDir\VST3"

if (Test-Path "$BuildRoot\Release\VST3") {
    Write-Host "Found Release VST3"
    Copy-Item -Path "$BuildRoot\Release\VST3\*" -Destination "$StagingDir\VST3" -Recurse -Force
} elseif (Test-Path "$BuildRoot\VST3") {
    Write-Host "Found VST3"
    Copy-Item -Path "$BuildRoot\VST3\*" -Destination "$StagingDir\VST3" -Recurse -Force
} else {
    Write-Error "ERROR: COULD NOT FIND BUILT VST3 DIRECTORIES!"
    exit 1
}

$IssContent = @"
[Setup]
AppName=Howling Wolves
AppVersion=1.0.19
DefaultDirName={cf}\VST3
DefaultGroupName=Howling Wolves
OutputBaseFilename=HowlingWolves_Win_Installer
OutputDir=$OutputDir
Compression=lzma
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64
DisableDirPage=yes

[Files]
Source: "Installer\staging_win\VST3\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
"@

Set-Content -Path "installer.iss" -Value $IssContent
Write-Host "Compiling Inno Setup Script..."
& "C:\Program Files (x86)\Inno Setup 6\iscc.exe" installer.iss
if ($LASTEXITCODE -ne 0) {
    Write-Error "Inno Setup Compiler failed with exit code $LASTEXITCODE"
    exit $LASTEXITCODE
}
Write-Host "Win Installer Compilation Complete."
