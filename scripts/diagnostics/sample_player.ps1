param([ValidateRange(1,60)][int]$Seconds=30,[ValidatePattern('^[a-zA-Z0-9_-]+$')][string]$Label='live-paused')
$ErrorActionPreference='Stop'
$repoRoot=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$state=Get-Content (Join-Path $repoRoot 'build/gui/session.json') -Raw | ConvertFrom-Json
$process=Get-Process -Id $state.pid
if($process.Path -ne $state.exe) { throw 'Identidade do executável mudou' }
if((Get-FileHash -LiteralPath $process.Path).Hash.ToLowerInvariant() -ne $state.sha256) { throw 'Hash do executável mudou' }
# pywin32 serializes the stored FILETIME at millisecond precision.
if([Math]::Abs(($process.StartTime.ToUniversalTime()-[DateTime]::Parse($state.created).ToUniversalTime()).TotalMilliseconds) -gt 1) { throw 'Identidade temporal mudou' }
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class LiteSicResourceCounters {
    [DllImport("user32.dll", SetLastError=true)]
    public static extern uint GetGuiResources(IntPtr process, uint flags);
}
'@
$samples=@()
$timer=[Diagnostics.Stopwatch]::StartNew()
for($i=0;$i -le $Seconds;$i++) {
    $process.Refresh()
    if($process.HasExited) { throw 'O processo terminou durante a amostra' }
    $samples += [PSCustomObject]@{
        seconds=$timer.Elapsed.TotalSeconds
        working_set=$process.WorkingSet64
        private_bytes=$process.PrivateMemorySize64
        handles=$process.HandleCount
        gdi=[LiteSicResourceCounters]::GetGuiResources($process.Handle,0)
        user=[LiteSicResourceCounters]::GetGuiResources($process.Handle,1)
        cpu_ms=$process.TotalProcessorTime.TotalMilliseconds
        responding=$process.Responding
    }
    if($i -lt $Seconds) { Start-Sleep -Seconds 1 }
}
$outDir=Join-Path $repoRoot 'build/m5-closeout/memory'
New-Item -ItemType Directory -Force $outDir | Out-Null
[PSCustomObject]@{process=$state;samples=$samples} | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $outDir "$Label.json")
$samples | Select-Object -First 1 -Last 1 | ConvertTo-Json
