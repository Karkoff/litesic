param([ValidateSet('32','64')][string]$Architecture='64', [ValidateRange(1,5)][int]$Trials=3)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
Push-Location $root
$savedPath=$env:PATH
try {
    & powershell -NoProfile -ExecutionPolicy Bypass -File scripts/dev.ps1 -Action eq-probe -Architecture $Architecture
    if($LASTEXITCODE) {throw 'EQ probe build/run failed'}
    $variant="build/$Architecture/release"
    $prefix=if($Architecture -eq '32'){'mingw32'}else{'mingw64'}
    $env:PATH=(Join-Path $root ".tools/msys64/$prefix/bin")+';'+$savedPath
    # Compile the starting commit's audio unit; shims only satisfy new control symbols.
    $source=& git show b27da1e:src/audio.cpp
    if($LASTEXITCODE) {throw 'Baseline source unavailable'}
    $source | Set-Content -Encoding UTF8 build/audio-before-m6.cpp
    @'
void Audio_UpdateEq(const Eq::Parameters&) {}
void Audio_TestSkipEq(BOOL) {}
'@ | Add-Content -Encoding UTF8 build/audio-before-m6.cpp
    $compiler=Join-Path $root ".tools/msys64/$prefix/bin/g++.exe"
    $flags=@('-std=c++17','-O2','-D_WIN32_WINNT=0x0601','-DUNICODE','-D_UNICODE','-DLITESIC_TEST','-Isrc','-Ithird_party')
    & $compiler @flags '-c' 'build/audio-before-m6.cpp' '-o' "$variant/audio-before-m6.o"
    if($LASTEXITCODE) {throw 'Baseline audio compilation failed'}
    $objects=@('playlist','playlist_view','m3u','tags','playback','playlist_worker','audio-before-m6','util','miniaudio_impl','ui_theme','eq_dsp','eq_state','plwnd','mainwnd','visualizer','ui_draw','ui_player','eqwnd','theme_window') | ForEach-Object {"$variant/$_.o"}
    $libraries=@('-lcomdlg32','-lcomctl32','-luser32','-lkernel32','-lgdi32','-lole32','-lshell32','-lwinmm','-ld2d1','-ldwrite','-lm','-lpsapi')
    & $compiler @flags '-static' '-static-libgcc' '-static-libstdc++' '-o' "$variant/eq-before-m6.exe" 'scripts/diagnostics/eq_probe.cpp' @objects @libraries
    if($LASTEXITCODE) {throw 'Baseline link failed'}
    New-Item -ItemType Directory -Force build/m6 | Out-Null
    $results=@()
    for($trial=1;$trial -le $Trials;$trial++) {
        $before=& "$variant/eq-before-m6.exe" 6
        if($LASTEXITCODE) {throw 'Baseline run failed'}
        $results+=@{trial=$trial;baseline=$true;sample=($before|ConvertFrom-Json)}
        foreach($mode in @(6,0,1,2,3,4,5)) {
            $after=& "$variant/eq-probe.exe" $mode
            if($LASTEXITCODE) {throw "Probe mode $mode failed"}
            $results+=@{trial=$trial;baseline=$false;sample=($after|ConvertFrom-Json)}
        }
    }
    $results | ConvertTo-Json -Depth 6 | Set-Content -Encoding UTF8 "build/m6/performance-$Architecture-release.json"
    $results | ForEach-Object { $_.sample } | Format-Table mode,mean_us,p99_us,max_us,process_cpu_ms,wall_ms,working_set,private_bytes
} finally {$env:PATH=$savedPath;Pop-Location}
