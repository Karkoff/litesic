[CmdletBinding()]
param(
    [ValidatePattern('^\d+\.\d+\.\d+$')][string]$Version = '1.0.0',
    [ValidateRange(1,16)][int]$Jobs = 4,
    [ValidatePattern('^\d{8}-\d{6}-\d{3}$')][string]$RunId = (Get-Date -Format 'yyyyMMdd-HHmmss-fff'),
    [string]$Msys2Root = $env:LITESIC_MSYS2_ROOT
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = Split-Path -Parent $PSScriptRoot
if (-not $Msys2Root) { $Msys2Root = Join-Path $repo '.tools/msys64' }
$sourceVersion = [regex]::Match((Get-Content (Join-Path $repo 'res/app.rc') -Raw), 'FILEVERSION\s+(\d+),(\d+),(\d+),(\d+)')
if (-not $sourceVersion.Success -or (($sourceVersion.Groups | Select-Object -Skip 1 -First 3).Value -join '.') -ne $Version) {
    throw 'Release version must match res/app.rc.'
}
$relative = 'build/desktop/v' + $Version + '-' + $RunId
$run = Join-Path $repo $relative
$assets = Join-Path $run 'assets'
New-Item -ItemType Directory -Path $assets -Force | Out-Null
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
$utf8 = New-Object Text.UTF8Encoding($false)
$records = @()
$saved = @{}
foreach ($name in @('MSYSTEM','CHERE_INVOKING','TMP','TEMP','TMPDIR','LITESIC_NO_CAPTURE')) {
    $saved[$name] = [Environment]::GetEnvironmentVariable($name,'Process')
}
Push-Location $repo
try {
    $tempPath = Join-Path $repo 'build/tmp'
    New-Item -ItemType Directory -Force -Path $tempPath | Out-Null
    $env:CHERE_INVOKING = '1'
    $env:TMP=$tempPath; $env:TEMP=$tempPath; $env:TMPDIR=$tempPath
    $env:LITESIC_NO_CAPTURE='1'
    foreach ($architecture in @('64','32')) {
        $prefix = if ($architecture -eq '64') { 'mingw64' } else { 'mingw32' }
        $arch = if ($architecture -eq '64') { 'x64' } else { 'x86' }
        $env:MSYSTEM = $prefix.ToUpperInvariant()
        # Fresh isolated output never replaces a running user's player.
        $build = "$relative/native/$architecture/release"
        $internal = "$relative/internal/$architecture/release"
        foreach ($target in @('all','test')) {
            & (Join-Path $Msys2Root 'usr/bin/bash.exe') -lc "set -o pipefail; make -j$Jobs ARCH=$architecture DEBUG=0 BUILD_DIR=$build INTERNAL_DIR=$internal $target 2>&1 | tee $relative/$arch-$target.log"
            if ($LASTEXITCODE -ne 0) { throw "$arch $target failed. See $run" }
        }
        $exe = Join-Path $repo "$build/LiteSic.exe"
        $bytes = [IO.File]::ReadAllBytes($exe)
        $machine = [BitConverter]::ToUInt16($bytes,([BitConverter]::ToInt32($bytes,0x3c)+4))
        $expected = if ($architecture -eq '64') { 0x8664 } else { 0x14c }
        if ($machine -ne $expected) { throw "Wrong architecture: $arch" }
        if ([Diagnostics.FileVersionInfo]::GetVersionInfo($exe).FileVersion -ne "$Version.0") { throw 'Wrong executable version' }
        $dump = @(& (Join-Path $Msys2Root "$prefix/bin/objdump.exe") -p $exe)
        if ($LASTEXITCODE -ne 0) { throw 'Dependency audit failed' }
        $imports = @($dump | Select-String 'DLL Name:\s*(.+)' | ForEach-Object { $_.Matches[0].Groups[1].Value.Trim() })
        $systemDlls = @('comctl32.dll','comdlg32.dll','d2d1.dll','dwrite.dll','gdi32.dll','kernel32.dll','msvcrt.dll','ole32.dll','oleacc.dll','oleaut32.dll','shell32.dll','user32.dll','winmm.dll','advapi32.dll','uuid.dll')
        if (-not $imports.Count) { throw 'No PE imports found' }
        foreach ($dll in $imports) { if ($dll.ToLowerInvariant() -notin $systemDlls) { throw "Unexpected dependency: $dll" } }
        foreach ($portable in @($false,$true)) {
            $suffix = if ($portable) { "portable-$arch" } else { $arch }
            $layout = Join-Path $run "layout-$suffix"
            New-Item -ItemType Directory -Path $layout | Out-Null
            Copy-Item -LiteralPath $exe -Destination (Join-Path $layout 'LiteSic.exe')
            $catalogs = @(Get-ChildItem -Path (Join-Path $repo 'locales/*/messages.tsv') -File)
            if ($catalogs.Count -ne 18) { throw 'Expected 18 language catalogs' }
            foreach ($catalog in $catalogs) {
                $destination = Join-Path $layout ('locales/' + $catalog.Directory.Name)
                New-Item -ItemType Directory -Path $destination -Force | Out-Null
                Copy-Item -LiteralPath $catalog.FullName -Destination $destination
            }
            $resources = Join-Path $layout 'resources'
            $licenses = Join-Path $layout 'licenses'
            New-Item -ItemType Directory -Path $resources,$licenses | Out-Null
            Copy-Item -Path (Join-Path $repo 'packaging/desktop/*.txt') -Destination $resources
            Copy-Item -Path (Join-Path $repo 'packaging/licenses/*.txt') -Destination $licenses
            foreach ($entry in @(
                @('libgcc/COPYING3','GCC-GPL-3.0.txt'),
                @('libgcc/COPYING.RUNTIME','GCC-Runtime-Exception.txt'),
                @('crt/COPYING','MinGW-COPYING.txt'),
                @('crt/COPYING.MinGW-w64.txt','MinGW-w64.txt'),
                @('crt/COPYING.MinGW-w64-runtime.txt','MinGW-w64-runtime.txt')
            )) {
                Copy-Item -LiteralPath (Join-Path $Msys2Root "$prefix/share/licenses/$($entry[0])") -Destination (Join-Path $licenses $entry[1])
            }
            if ($portable) { [IO.File]::WriteAllText((Join-Path $resources 'portable.flag'), "LiteSic portable profile: ../data`n", $utf8) }
            $files = @(Get-ChildItem -LiteralPath $layout -Recurse -File | Sort-Object FullName)
            if (@(Get-ChildItem -LiteralPath $layout -File).Name -ne 'LiteSic.exe') { throw 'Unexpected root file' }
            $zipPath = Join-Path $assets "LiteSic-v$Version-$suffix.zip"
            $zip = [IO.Compression.ZipFile]::Open($zipPath,[IO.Compression.ZipArchiveMode]::Create)
            try {
                foreach ($file in $files) {
                    $name = $file.FullName.Substring($layout.Length+1).Replace('\','/')
                    if ($name -notmatch '^(LiteSic\.exe|locales/[^/]+/messages\.tsv|resources/(README\.(pt-BR|en)\.txt|portable\.flag)|licenses/[^/]+\.txt)$') { throw "Unexpected payload: $name" }
                    $entry = $zip.CreateEntry($name,[IO.Compression.CompressionLevel]::Optimal)
                    $entry.LastWriteTime = [DateTimeOffset]::new(2000,1,1,0,0,0,[TimeSpan]::Zero)
                    $inputStream = [IO.File]::OpenRead($file.FullName)
                    $outputStream = $entry.Open()
                    try { $inputStream.CopyTo($outputStream) } finally { $inputStream.Dispose(); $outputStream.Dispose() }
                }
            } finally { $zip.Dispose() }
            $unpacked = Join-Path $run "unpacked-$suffix"
            [IO.Compression.ZipFile]::ExtractToDirectory($zipPath,$unpacked)
            foreach ($file in $files) {
                $name = $file.FullName.Substring($layout.Length+1)
                if ((Get-FileHash -LiteralPath $file.FullName).Hash -ne (Get-FileHash -LiteralPath (Join-Path $unpacked $name)).Hash) { throw "ZIP integrity failed: $name" }
            }
            $records += [ordered]@{
                Asset=[IO.Path]::GetFileName($zipPath); Architecture=$arch; Portable=$portable
                Bytes=(Get-Item -LiteralPath $zipPath).Length; SHA256=(Get-FileHash -LiteralPath $zipPath).Hash
                ExecutableSHA256=(Get-FileHash -LiteralPath $exe).Hash; Imports=$imports
                Files=@($files | ForEach-Object { $_.FullName.Substring($layout.Length+1).Replace('\','/') })
            }
            Write-Host "PACKAGE PASS: $suffix"
        }
    }
    $checksums = ($records | ForEach-Object { $_.SHA256.ToLowerInvariant() + '  ' + $_.Asset }) -join "`n"
    [IO.File]::WriteAllText((Join-Path $assets 'SHA256SUMS.txt'),$checksums+"`n",$utf8)
    [ordered]@{Version=$Version;Commit=(& git rev-parse HEAD);WorkingTreeStatus=@(& git status --short);Packages=$records} |
        ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $run 'manifest.json') -Encoding UTF8
    Write-Host "DESKTOP_RELEASE_DIRECTORY=$run"
} finally {
    Pop-Location
    foreach ($name in $saved.Keys) { [Environment]::SetEnvironmentVariable($name,$saved[$name],'Process') }
}
