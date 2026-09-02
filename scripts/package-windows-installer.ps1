param(
    [string]$Version = '0.1.0-alpha.0'
)

$ErrorActionPreference = 'Stop'

$sourceDir = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$artifactsDir = Join-Path $sourceDir 'artifacts'
$portableDir = Join-Path $artifactsDir "ScanTailor-FanMode-$Version-win-x64"
$installerDefinition = Join-Path $sourceDir 'packaging\windows\ScanTailor-FanMode.iss'
$installerPath = Join-Path $artifactsDir "ScanTailor-FanMode-$Version-Setup-x64.exe"
$requiredRuntimeFiles = @(
    'scantailor-fanmode.exe',
    'libjpeg-8.dll',
    'libjbig-0.dll',
    'libpng16-16.dll',
    'libtiff-6.dll'
)

$compilerCandidates = @(
    (Join-Path $env:LOCALAPPDATA 'Programs\Inno Setup 6\ISCC.exe'),
    'C:\Program Files (x86)\Inno Setup 6\ISCC.exe',
    'C:\Program Files\Inno Setup 6\ISCC.exe'
)
$compiler = $compilerCandidates |
    Where-Object { Test-Path -LiteralPath $_ } |
    Select-Object -First 1

if (-not $compiler) {
    throw 'Inno Setup 6 was not found. Install JRSoftware.InnoSetup with winget first.'
}
if (-not (Test-Path -LiteralPath $installerDefinition)) {
    throw "Installer definition was not found: $installerDefinition"
}
if (-not (Test-Path -LiteralPath $portableDir)) {
    throw "Build the portable package before the installer. Missing directory: $portableDir"
}

foreach ($runtimeFile in $requiredRuntimeFiles) {
    $runtimePath = Join-Path $portableDir $runtimeFile
    if (-not (Test-Path -LiteralPath $runtimePath)) {
        throw "The portable package is incomplete. Missing runtime file: $runtimePath"
    }
}

& $compiler "/DMyAppVersion=$Version" $installerDefinition
if ($LASTEXITCODE -ne 0) {
    throw "Inno Setup failed with exit code $LASTEXITCODE."
}
if (-not (Test-Path -LiteralPath $installerPath)) {
    throw "The installer was not created: $installerPath"
}

Write-Host "Windows installer: $installerPath"
