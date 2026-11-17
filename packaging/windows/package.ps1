# PLAN.md P2.8 / §12's Windows row: windeployqt --qmldir ... --release,
# then zip (a portable artefact). Inno Setup/WiX installer is deferred -
# not part of this chunk (PLAN.md's P2.8 note) - a portable zip is
# simpler to verify in CI and already satisfies "Artefacts launch on
# clean VMs". Unsigned (P2.8's own acceptance); Authenticode signing is
# P7.1's job, later.
param(
    [Parameter(Mandatory = $true)][string]$BuildDir,
    [Parameter(Mandatory = $true)][string]$OutDir
)
$ErrorActionPreference = "Stop"

$Version = "0.1.0" # mirrors main.cpp's kVersion (P2.1) - no single source of truth yet
$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")

$StageDir = Join-Path $BuildDir "LumaTune-staging"
if (Test-Path $StageDir) { Remove-Item -Recurse -Force $StageDir }
New-Item -ItemType Directory -Force -Path $StageDir | Out-Null
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

Copy-Item (Join-Path $BuildDir "src\app\lumatune.exe") $StageDir

windeployqt --qmldir (Join-Path $RepoRoot "src\ui\qml") --release (Join-Path $StageDir "lumatune.exe")
if ($LASTEXITCODE -ne 0) { throw "windeployqt failed (exit $LASTEXITCODE)" }

$ZipPath = Join-Path $OutDir "LumaTune-$Version-win64.zip"
if (Test-Path $ZipPath) { Remove-Item $ZipPath }
Compress-Archive -Path (Join-Path $StageDir '*') -DestinationPath $ZipPath
