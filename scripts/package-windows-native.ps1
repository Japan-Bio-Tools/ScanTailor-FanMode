param(
    [string]$Version = '0.1.0-alpha.0'
)

$ErrorActionPreference = 'Stop'

$sourceDir = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$buildDir = Join-Path $sourceDir 'build-fanmode'
$artifactsDir = Join-Path $sourceDir 'artifacts'
$packageName = "ScanTailor-FanMode-$Version-win-x64"
$packageDir = Join-Path $artifactsDir $packageName
$zipPath = Join-Path $artifactsDir "$packageName.zip"
$executable = Join-Path $buildDir 'scantailor-fanmode.exe'
$msysRoot = Join-Path $env:LOCALAPPDATA 'Programs\MSYS2'
$ucrtRoot = Join-Path $msysRoot 'ucrt64'
$ucrtBin = Join-Path $ucrtRoot 'bin'
$objdump = Join-Path $ucrtBin 'objdump.exe'
$pdfConverter = Join-Path $buildDir 'pdftoppm.exe'
$ocrEngine = Join-Path $buildDir 'tesseract.exe'
$tessdata = Join-Path $buildDir 'tessdata'
$popplerData = Join-Path $ucrtRoot 'share\poppler'
$popplerDataLicenses = Join-Path $ucrtRoot 'share\licenses\poppler-data'
$tesseractLicenses = Join-Path $ucrtRoot 'share\licenses\tesseract-ocr'
$leptonicaLicenses = Join-Path $ucrtRoot 'share\licenses\leptonica'

if (-not (Test-Path -LiteralPath $executable)) {
    throw "Build FanMode before packaging. Missing executable: $executable"
}
if (-not (Test-Path -LiteralPath $objdump)) {
    throw "MSYS2 objdump was not found: $objdump"
}
if (-not (Test-Path -LiteralPath $pdfConverter)) {
    throw "Bundled PDF converter was not found: $pdfConverter"
}
if (-not (Test-Path -LiteralPath $ocrEngine)) {
    throw "Bundled OCR engine was not found: $ocrEngine"
}
if (-not (Test-Path -LiteralPath (Join-Path $tessdata 'ara.traineddata')) -or
    -not (Test-Path -LiteralPath (Join-Path $tessdata 'eng.traineddata'))) {
    throw "Bundled Arabic and English OCR data was not found: $tessdata"
}
if (-not (Test-Path -LiteralPath $popplerData)) {
    throw "Poppler encoding data was not found: $popplerData"
}

New-Item -ItemType Directory -Path $artifactsDir -Force | Out-Null

# Deleting only this exact, versioned package directory is safe and prevents stale DLLs.
$resolvedArtifacts = [System.IO.Path]::GetFullPath($artifactsDir).TrimEnd('\') + '\'
$resolvedPackage = [System.IO.Path]::GetFullPath($packageDir)
if (-not $resolvedPackage.StartsWith($resolvedArtifacts, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to replace a package outside the artifacts directory: $resolvedPackage"
}
if (Test-Path -LiteralPath $resolvedPackage) {
    Remove-Item -LiteralPath $resolvedPackage -Recurse -Force
}
New-Item -ItemType Directory -Path $resolvedPackage -Force | Out-Null

Copy-Item -LiteralPath $executable -Destination $resolvedPackage
Copy-Item -LiteralPath $pdfConverter -Destination $resolvedPackage
Copy-Item -LiteralPath $ocrEngine -Destination $resolvedPackage
Copy-Item -LiteralPath $tessdata -Destination $resolvedPackage -Recurse
Get-ChildItem -LiteralPath $buildDir -Filter '*.dll' -File | Copy-Item -Destination $resolvedPackage
Get-ChildItem -LiteralPath $buildDir -Filter 'scantailor-advanced_*.qm' -File |
    Copy-Item -Destination $resolvedPackage

foreach ($pluginDir in @('generic', 'iconengines', 'imageformats', 'networkinformation', 'platforms', 'styles', 'tls')) {
    $sourcePluginDir = Join-Path $buildDir $pluginDir
    if (Test-Path -LiteralPath $sourcePluginDir) {
        Copy-Item -LiteralPath $sourcePluginDir -Destination $resolvedPackage -Recurse
    }
}

$packageShareDir = Join-Path $resolvedPackage 'share'
New-Item -ItemType Directory -Path $packageShareDir -Force | Out-Null
Copy-Item -LiteralPath $popplerData -Destination $packageShareDir -Recurse
$thirdPartyLicenses = Join-Path $resolvedPackage 'THIRD-PARTY-LICENSES'
New-Item -ItemType Directory -Path $thirdPartyLicenses -Force | Out-Null
if (Test-Path -LiteralPath $popplerDataLicenses) {
    Copy-Item -LiteralPath $popplerDataLicenses -Destination $thirdPartyLicenses -Recurse
}
if (Test-Path -LiteralPath $tesseractLicenses) {
    Copy-Item -LiteralPath $tesseractLicenses -Destination $thirdPartyLicenses -Recurse
}
if (Test-Path -LiteralPath $leptonicaLicenses) {
    Copy-Item -LiteralPath $leptonicaLicenses -Destination $thirdPartyLicenses -Recurse
}

# Copy every non-system MinGW dependency, including dependencies of plugins.
$queue = [System.Collections.Generic.Queue[string]]::new()
Get-ChildItem -LiteralPath $resolvedPackage -Recurse -File |
    Where-Object { $_.Extension -in @('.exe', '.dll') } |
    ForEach-Object { $queue.Enqueue($_.FullName) }
$inspected = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)

while ($queue.Count -gt 0) {
    $binary = $queue.Dequeue()
    if (-not $inspected.Add($binary)) {
        continue
    }

    $imports = & $objdump -p $binary 2>$null |
        Select-String -Pattern '^\s*DLL Name:\s*(.+)$' |
        ForEach-Object { $_.Matches[0].Groups[1].Value.Trim() } |
        Sort-Object -Unique

    foreach ($import in $imports) {
        $packagedDependency = Join-Path $resolvedPackage $import
        if (Test-Path -LiteralPath $packagedDependency) {
            continue
        }

        $toolchainDependency = Join-Path $ucrtBin $import
        if (Test-Path -LiteralPath $toolchainDependency) {
            Copy-Item -LiteralPath $toolchainDependency -Destination $packagedDependency
            $queue.Enqueue($packagedDependency)
        }
    }
}

$buildInfo = @(
    'ScanTailor FanMode portable Windows build'
    "FanMode version: $Version"
    'Architecture: x64'
    'PDF import: Poppler pdftoppm with lossless LZW TIFF output'
    'Searchable PDF export: Tesseract OCR with bundled Arabic and English data'
    'License: GNU GPL version 3 (see LICENSE.txt)'
)
$buildInfo | Set-Content -LiteralPath (Join-Path $resolvedPackage 'BUILD-INFO.txt') -Encoding UTF8
Copy-Item -LiteralPath (Join-Path $sourceDir 'LICENSE') -Destination (Join-Path $resolvedPackage 'LICENSE.txt')

if (Test-Path -LiteralPath $zipPath) {
    Remove-Item -LiteralPath $zipPath -Force
}
Compress-Archive -LiteralPath $resolvedPackage -DestinationPath $zipPath -CompressionLevel Optimal

Write-Host "Portable package: $resolvedPackage"
Write-Host "Zip package: $zipPath"
