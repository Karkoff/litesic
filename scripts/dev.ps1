param(
    [ValidateSet('build', 'test', 'run', 'clean', 'eq-probe', 'perf-probe')][string]$Action = 'build',
    [ValidateSet('32', '64')][string]$Architecture = '64',
    [switch]$DebugBuild,
    [string]$Msys2Root = $env:LITESIC_MSYS2_ROOT
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
if (-not $Msys2Root) { $Msys2Root = Join-Path $repoRoot '.tools/msys64' }
$bashPath = Join-Path $Msys2Root 'usr/bin/bash.exe'
$prefix = if ($Architecture -eq '32') { 'mingw32' } else { 'mingw64' }
foreach ($tool in @('usr/bin/bash.exe', 'usr/bin/make.exe', "$prefix/bin/gcc.exe", "$prefix/bin/g++.exe", "$prefix/bin/windres.exe")) {
    if (-not (Test-Path -LiteralPath (Join-Path $Msys2Root $tool))) {
        throw "Ferramenta ausente: $tool em $Msys2Root. Consulte docs/DEVELOPMENT.md."
    }
}
$saved = @{}
foreach ($name in @('MSYSTEM', 'CHERE_INVOKING', 'TMP', 'TEMP', 'TMPDIR')) {
    $saved[$name] = [Environment]::GetEnvironmentVariable($name, 'Process')
}
Push-Location $repoRoot
try {
    $tempPath = Join-Path $repoRoot 'build/tmp'
    New-Item -ItemType Directory -Force $tempPath | Out-Null
    $env:MSYSTEM = $prefix.ToUpperInvariant()
    $env:CHERE_INVOKING = '1'
    $env:TMP = $tempPath
    $env:TEMP = $tempPath
    $env:TMPDIR = $tempPath
    $debugValue = if ($DebugBuild) { '1' } else { '0' }
    $target = if ($Action -eq 'build') { 'all' } else { $Action }
    & $bashPath -lc "exec make ARCH=$Architecture DEBUG=$debugValue $target"
    $result = $LASTEXITCODE
}
finally {
    Pop-Location
    foreach ($name in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($name, $saved[$name], 'Process')
    }
}
exit $result
