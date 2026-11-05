$ErrorActionPreference = 'Stop'
$projectDirectory = Split-Path -Parent $PSScriptRoot
$buildDirectory = Join-Path $projectDirectory '.pio/core-tests'
New-Item -ItemType Directory -Force -Path $buildDirectory | Out-Null

# Use the installed Microsoft C++ build tools without changing global PATH.
if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    if (-not (Test-Path -LiteralPath $vswhere)) {
        throw 'Install Visual Studio C++ Build Tools, or run the GCC command in README.md.'
    }
    $installation = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $installation) { throw 'No Visual Studio C++ build tools installation found.' }
    & (Join-Path $installation 'Common7/Tools/Launch-VsDevShell.ps1') -Arch amd64 -HostArch amd64 -SkipAutomaticLocation
}

Push-Location $buildDirectory
try {
    & cl.exe /nologo /std:c++17 /EHsc /W4 "/I$projectDirectory/lib/ControllerCore/src" "$PSScriptRoot/test_core.cpp" /Fe:core_tests.exe
    if ($LASTEXITCODE -ne 0) { throw 'Core tests did not compile.' }
    & ./core_tests.exe
    if ($LASTEXITCODE -ne 0) { throw 'Core tests failed.' }
} finally {
    Pop-Location
}
