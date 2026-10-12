# Migrate the legacy mixed output layout. Never recurse or remove unknown files.
[CmdletBinding(SupportsShouldProcess)]
param()
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$buildRoot = [IO.Path]::GetFullPath((Join-Path $repoRoot 'build'))
$objectNames = @(Get-ChildItem -LiteralPath (Join-Path $repoRoot 'src') -File |
    Where-Object { $_.Extension -in @('.cpp','.c') } | ForEach-Object { $_.BaseName }) + @('app.res','audio-test')
$testNames = @(Get-ChildItem -LiteralPath (Join-Path $repoRoot 'tests') -File |
    Where-Object { $_.Extension -in @('.cpp','.c') } | ForEach-Object { 'test-' + $_.BaseName })
$probeNames = @('eq-probe','eq-before-m6','perf-probe','perf-probe-sharedfactory','perf-probe-software')
$selected = @()
foreach ($arch in @('64','32')) {
    foreach ($config in @('release','debug')) {
        $directory = [IO.Path]::GetFullPath((Join-Path $buildRoot "$arch/$config"))
        if (-not $directory.StartsWith($buildRoot + [IO.Path]::DirectorySeparatorChar)) { throw 'Unsafe path' }
        if (-not (Test-Path -LiteralPath $directory)) { continue }
        foreach ($file in Get-ChildItem -LiteralPath $directory -File) {
            if ($file.Name -eq 'LiteSic.exe') { continue }
            $known = ($file.Extension -in @('.o','.d') -and $file.BaseName -in ($objectNames + $testNames)) -or
                ($file.Extension -eq '.exe' -and $file.BaseName -in ($testNames + $probeNames)) -or
                $file.Name -eq 'build.log'
            if (-not $known) { throw "Arquivo desconhecido preservado; revise antes da limpeza: $($file.FullName)" }
            if ($file.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Link preservado: $($file.FullName)" }
            $selected += [PSCustomObject]@{Path=$file.FullName;Bytes=$file.Length;SHA256=(Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash}
        }
    }
}
$reportDir = Join-Path $buildRoot 'validation/clean-runtime'
New-Item -ItemType Directory -Force $reportDir | Out-Null
$manifest = Join-Path $reportDir ('removed-' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff') + '.csv')
$selected | Export-Csv -NoTypeInformation -Encoding UTF8 -LiteralPath $manifest
foreach ($file in $selected) {
    if ($PSCmdlet.ShouldProcess($file.Path,'Remove regenerable legacy build artifact')) {
        Remove-Item -LiteralPath $file.Path
    }
}
Write-Output "Legacy artifacts: $($selected.Count); inventory: $manifest"
