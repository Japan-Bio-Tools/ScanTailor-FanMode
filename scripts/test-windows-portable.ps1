param([string]$Version = '0.1.0-alpha.0')
$ErrorActionPreference = 'Stop'
$sourceDir = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$name = "ScanTailor-FanMode-$Version-win-x64"
$artifacts = Join-Path $sourceDir 'artifacts'
# A fresh directory ensures that only ZIP contents are tested.
$testDir = Join-Path ([IO.Path]::GetTempPath()) ([Guid]::NewGuid().ToString())
Expand-Archive -LiteralPath (Join-Path $artifacts "$name.zip") -DestinationPath $testDir
$package = Join-Path $testDir $name
$previousPath = $env:Path
$previousQt = $env:QT_PLUGIN_PATH
$previousPlatform = $env:QT_QPA_PLATFORM
$previousTessdata = $env:TESSDATA_PREFIX
try {
    $required = @('scantailor-fanmode.exe', 'pdftoppm.exe', 'tesseract.exe', 'qpdf.exe',
        'platforms\qwindows.dll', 'imageformats\qjpeg.dll', 'iconengines\qsvgicon.dll',
        'tessdata\eng.traineddata', 'tessdata\ara.traineddata', 'tessdata\configs\pdf',
        'LICENSE.txt', 'BUILD-INFO.txt', 'share\poppler', 'THIRD-PARTY-LICENSES')
    foreach ($item in $required) {
        if (-not (Test-Path -LiteralPath (Join-Path $package $item))) { throw "Missing ZIP entry: $item" }
    }
    foreach ($dependency in @('*Qt5Core*.dll', '*qpdf*.dll', '*lept*.dll', '*tiff*.dll')) {
        if (-not (Get-ChildItem -Path (Join-Path $package $dependency) -File)) {
            throw "Missing runtime DLL: $dependency"
        }
    }
    foreach ($license in @('qt5-base', 'poppler', 'poppler-data', 'tesseract-ocr', 'leptonica', 'qpdf')) {
        if (-not (Get-ChildItem -LiteralPath (Join-Path $package "THIRD-PARTY-LICENSES\$license") -File -Recurse)) {
            throw "Missing dependency license: $license"
        }
    }
    $env:Path = "$package;$env:SystemRoot\System32;$env:SystemRoot"
    $env:QT_PLUGIN_PATH = $package
    # Exercise the real Windows platform plugin, not offscreen.
    $env:QT_QPA_PLATFORM = 'windows'
    $env:TESSDATA_PREFIX = Join-Path $package 'tessdata'
    $env:OMP_THREAD_LIMIT = '2'
    Push-Location $testDir
    try {
        Add-Type -AssemblyName System.Drawing
        $bitmap = [Drawing.Bitmap]::new(1600, 400)
        $graphics = [Drawing.Graphics]::FromImage($bitmap)
        $font = [Drawing.Font]::new('Arial', 48)
        try {
            $graphics.Clear([Drawing.Color]::White)
            $graphics.DrawString('ScanTailor portable test', $font, [Drawing.Brushes]::Black, 40, 120)
            $bitmap.Save((Join-Path $testDir 'input.png'), [Drawing.Imaging.ImageFormat]::Png)
        } finally { $font.Dispose(); $graphics.Dispose(); $bitmap.Dispose() }
        & "$package\tesseract.exe" input.png output -l eng+ara --psm 6 txt pdf
        if ($LASTEXITCODE -ne 0) { throw 'Packaged OCR failed.' }
        if ((Get-Content output.txt -Raw) -notmatch '(?i)scantailor portable test') { throw 'OCR text mismatch.' }
        & "$package\qpdf.exe" --check output.pdf
        if ($LASTEXITCODE -ne 0) { throw 'Packaged QPDF failed.' }
        & "$package\pdftoppm.exe" -r 100 -tiff -singlefile output.pdf roundtrip
        if ($LASTEXITCODE -ne 0 -or -not (Test-Path roundtrip.tif)) { throw 'Packaged PDF import failed.' }
        $app = Start-Process -FilePath "$package\scantailor-fanmode.exe" -PassThru
        try {
            $deadline = [DateTime]::UtcNow.AddSeconds(30)
            do {
                Start-Sleep -Milliseconds 500
                $app.Refresh()
                if ($app.HasExited) { throw "Portable GUI exited early: $($app.ExitCode)" }
            } while ($app.MainWindowHandle -eq 0 -and [DateTime]::UtcNow -lt $deadline)
            if ($app.MainWindowHandle -eq 0 -or $app.MainWindowTitle -notmatch 'ScanTailor FanMode') {
                throw 'Portable GUI main window was not displayed.'
            }
        } finally {
            if (-not $app.HasExited) {
                $null = $app.CloseMainWindow()
                if (-not $app.WaitForExit(5000)) { $app.Kill() }
            }
        }
    } finally { Pop-Location }
    @('PASS: ZIP contents and dependency licenses', 'PASS: OCR with eng+ara enabled',
      'PASS: QPDF validation and PDF-to-TIFF', 'PASS: Windows GUI main window',
      'All runtime checks used extracted ZIP and a PATH without MSYS2.',
      'User PC startup and Japanese vertical-text correction remain unverified.') |
        Set-Content -LiteralPath (Join-Path $artifacts 'portable-validation.txt') -Encoding utf8
} finally {
    $env:Path = $previousPath
    $env:QT_PLUGIN_PATH = $previousQt
    $env:QT_QPA_PLATFORM = $previousPlatform
    $env:TESSDATA_PREFIX = $previousTessdata
    Remove-Item -LiteralPath $testDir -Recurse -Force
}
