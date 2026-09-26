# Run from an x64 Visual Studio Developer PowerShell after building raylib Release.
param([switch]$Profile)
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path $PSScriptRoot -Parent
$buildDir = Join-Path $repoRoot 'x64/SpatialHashTests'
New-Item -ItemType Directory -Force $buildDir | Out-Null
Push-Location $buildDir
try {
    $flags = @('/nologo', '/std:c++20', '/EHsc', '/O2', '/MD',
        "/I$repoRoot/ArchetypeECS", "/I$repoRoot/external/raylib/src")
    if ($Profile) { $flags += '/DPHYS_PROFILE' }
    & cl.exe @flags "$PSScriptRoot/SpatialHashTests.cpp" "$repoRoot/ArchetypeECS/SpatialGrid.cpp" "$repoRoot/ArchetypeECS/Colliders.cpp" /Fe:SpatialHashTests.exe /link "$repoRoot/x64/Release/raylib_gl43.lib" winmm.lib opengl32.lib user32.lib gdi32.lib shell32.lib
    if ($LASTEXITCODE -ne 0) { throw 'Test build failed' }
    & ./SpatialHashTests.exe
    if ($LASTEXITCODE -ne 0) { throw 'SpatialHash tests failed' }
} finally { Pop-Location }
