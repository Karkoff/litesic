param([ValidateRange(1,5)][int]$Trials = 3,[switch]$Isolation)
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
Push-Location $repoRoot
$savedPath = $env:PATH
try {
    $outDir = 'build/m5-closeout/memory'
    New-Item -ItemType Directory -Force $outDir | Out-Null
    $compiler = Join-Path $repoRoot '.tools/msys64/mingw64/bin/g++.exe'
    $env:PATH = (Join-Path $repoRoot '.tools/msys64/mingw64/bin') + ';' + $env:PATH
    $cpp = [IO.File]::ReadAllText((Join-Path $repoRoot 'src/ui_draw.cpp'))
    $signature = 'bool Canvas::Begin(HDC dc, const RECT& pixels, float scale) {'
    if (-not $cpp.Contains($signature)) { throw 'Assinatura de Canvas::Begin mudou' }
    # Only the diagnostic copy changes. Production source and executable are untouched.
    $cpp = $cpp.Replace($signature,$signature + "`n    return false; // Diagnostic: force existing GDI fallback.")
    [IO.File]::WriteAllText((Join-Path $repoRoot "$outDir/ui_draw-gdi.cpp"),$cpp,(New-Object Text.UTF8Encoding $false))
    $flags = @('-std=c++17','-Wall','-Wextra','-g','-O0','-DDEBUG','-D_WIN32_WINNT=0x0601','-DUNICODE','-D_UNICODE','-DLITESIC_TEST','-DPSAPI_VERSION=1','-Isrc','-Ithird_party')
    & $compiler @flags -c "$outDir/ui_draw-gdi.cpp" -o "$outDir/ui_draw-gdi.o"
    if($LASTEXITCODE -ne 0) { throw 'Compilação GDI falhou' }
    $objects = 'playlist playlist_view m3u tags playback playlist_worker audio-test util miniaudio_impl plwnd mainwnd visualizer ui_player eqwnd ui_theme theme_window'.Split(' ') | ForEach-Object { "build/64/debug/$_.o" }
    $libs = @('-static','-static-libgcc','-static-libstdc++','-lcomdlg32','-lcomctl32','-luser32','-lkernel32','-lgdi32','-lole32','-lshell32','-lwinmm','-ld2d1','-ldwrite','-lpsapi','-lm')
    $backends=if($Isolation){@('fonts','software')}else{@('gdi','d2d')}
    if($Isolation) {
        $original=[IO.File]::ReadAllText((Join-Path $repoRoot 'src/ui_draw.cpp'))
        $fonts=$original.Replace($signature,$signature+"`n    Initialize(); return false; // Diagnostic: fonts/factories, no DC targets.")
        $fonts=$fonts.Replace('    if (!target_) {',"    return true; // Diagnostic: skip target allocation.`n    if (!target_) {")
        [IO.File]::WriteAllText((Join-Path $repoRoot "$outDir/ui_draw-fonts.cpp"),$fonts,(New-Object Text.UTF8Encoding $false))
        $software=$original.Replace('D2D1_RENDER_TARGET_TYPE_DEFAULT','D2D1_RENDER_TARGET_TYPE_SOFTWARE')
        [IO.File]::WriteAllText((Join-Path $repoRoot "$outDir/ui_draw-software.cpp"),$software,(New-Object Text.UTF8Encoding $false))
        foreach($mode in $backends) {
            & $compiler @flags -c "$outDir/ui_draw-$mode.cpp" -o "$outDir/ui_draw-$mode.o"
            if($LASTEXITCODE -ne 0) { throw "Compilação $mode falhou" }
        }
    }
    foreach($backend in $backends) {
        $renderer = if($backend -eq 'd2d'){'build/64/debug/ui_draw.o'}else{"$outDir/ui_draw-$backend.o"}
        & $compiler @flags scripts/diagnostics/memory_probe.cpp @objects $renderer @libs -o "$outDir/probe-$backend.exe"
        if($LASTEXITCODE -ne 0) { throw "Link $backend falhou" }
    }
    foreach($trial in 1..$Trials) {
        foreach($backend in $backends) {
            & "$outDir/probe-$backend.exe" > "$outDir/$backend-$trial.jsonl"
            if($LASTEXITCODE -ne 0) { throw "Probe $backend $trial falhou" }
            Write-Output "PASS $backend trial $trial"
        }
    }
    Get-ChildItem "$outDir/probe-*.exe" | Get-FileHash | ConvertTo-Json | Set-Content "$outDir/binaries.json"
} finally { $env:PATH=$savedPath; Pop-Location }
