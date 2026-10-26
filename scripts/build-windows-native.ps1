param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',
    [switch]$SkipTests,
    [string]$MsysRoot = (Join-Path $env:LOCALAPPDATA 'Programs\MSYS2')
)

$ErrorActionPreference = 'Stop'

$sourceDir = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$buildDir = Join-Path $sourceDir 'build-fanmode'
$msysRoot = [System.IO.Path]::GetFullPath($MsysRoot)
$ucrtRoot = Join-Path $msysRoot 'ucrt64'
$cmake = Join-Path $ucrtRoot 'bin\cmake.exe'
$ninja = Join-Path $ucrtRoot 'bin\ninja.exe'
$ctest = Join-Path $ucrtRoot 'bin\ctest.exe'

foreach ($requiredFile in @($cmake, $ninja, $ctest)) {
    if (-not (Test-Path -LiteralPath $requiredFile)) {
        throw "Required FanMode build tool was not found: $requiredFile"
    }
}

$previousPath = $env:Path
try {
    $env:Path = (Join-Path $ucrtRoot 'bin') + ';' + (Join-Path $msysRoot 'usr\bin') + ';' + $env:Path

    & $cmake -S $sourceDir -B $buildDir -G Ninja `
        "-DCMAKE_BUILD_TYPE=$Configuration" `
        "-DCMAKE_POLICY_VERSION_MINIMUM=3.5" `
        "-DCMAKE_C_COMPILER=$ucrtRoot/bin/gcc.exe" `
        "-DCMAKE_CXX_COMPILER=$ucrtRoot/bin/g++.exe" `
        "-DBUILD_TESTS=$(-not $SkipTests)" `
        "-DCMAKE_PREFIX_PATH=$ucrtRoot" `
        "-DCMAKE_MAKE_PROGRAM=$ninja"
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configuration failed with exit code $LASTEXITCODE."
    }

    & $cmake --build $buildDir --parallel
    if ($LASTEXITCODE -ne 0) {
        throw "FanMode build failed with exit code $LASTEXITCODE."
    }

    if (-not $SkipTests) {
        $env:Path = $buildDir + ';' + $env:Path
        & $ctest --test-dir $buildDir --output-on-failure --no-tests=error -C $Configuration
        if ($LASTEXITCODE -ne 0) {
            throw "FanMode tests failed with exit code $LASTEXITCODE."
        }
    }
} finally {
    $env:Path = $previousPath
}

Write-Host "FanMode executable: $(Join-Path $buildDir 'scantailor-fanmode.exe')"
