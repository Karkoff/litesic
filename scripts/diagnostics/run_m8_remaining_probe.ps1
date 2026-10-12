param([ValidateSet('32','64')][string]$Architecture='64')
$ErrorActionPreference='Stop'
$repoRoot=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$bashPath=Join-Path $repoRoot '.tools/msys64/usr/bin/bash.exe'
$prefix=if($Architecture -eq '32'){'mingw32'}else{'mingw64'}
$savedMsystem=$env:MSYSTEM
$savedChere=$env:CHERE_INVOKING
$env:MSYSTEM=$prefix.ToUpperInvariant()
$env:CHERE_INVOKING='1'
Push-Location $repoRoot
try {
    New-Item -ItemType Directory -Force 'build/m8-consolidation' | Out-Null
    $objects='playlist playlist_view m3u tags playback playlist_worker audio-test util miniaudio_impl ui_theme eq_dsp eq_state i18n language_selection skin plwnd mainwnd visualizer ui_draw ui_player eqwnd theme_window ui_tooltip'.Split(' ') | ForEach-Object { "build/$Architecture/release/$_.o" }
    $command="/$prefix/bin/g++.exe -std=c++17 -Wall -Wextra -D_WIN32_WINNT=0x0601 -DUNICODE -D_UNICODE -DLITESIC_TEST -Isrc -Itests -Ithird_party -static -static-libgcc -static-libstdc++ -Wl,--no-insert-timestamp -o build/m8-consolidation/audit-$Architecture.exe scripts/diagnostics/m8_remaining_probe.cpp $($objects -join ' ') -lcomdlg32 -lcomctl32 -luser32 -lkernel32 -lgdi32 -lole32 -lshell32 -lwinmm -ld2d1 -ldwrite -lm"
    & $bashPath -lc $command
    if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
    & "./build/m8-consolidation/audit-$Architecture.exe"
    exit $LASTEXITCODE
} finally { Pop-Location; $env:MSYSTEM=$savedMsystem; $env:CHERE_INVOKING=$savedChere }
