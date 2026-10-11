$ErrorActionPreference='Stop'
$dir=Join-Path (Get-Location) 'build/audit-m6/app/64/release'
New-Item -ItemType Directory -Force $dir | Out-Null
Copy-Item -LiteralPath build/64/release/LiteSic.exe -Destination (Join-Path $dir 'LiteSic.exe')
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class AuditNativeCounters {
 [DllImport("user32.dll")] public static extern uint GetGuiResources(IntPtr process,uint flags);
 [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc callback,IntPtr param);
 public delegate bool EnumProc(IntPtr hwnd,IntPtr param);
 [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hwnd,out uint pid);
 [DllImport("user32.dll",CharSet=CharSet.Unicode)] public static extern IntPtr SendMessage(IntPtr hwnd,uint msg,IntPtr w,IntPtr l);
}
"@
$startInfo=New-Object Diagnostics.ProcessStartInfo
$startInfo.FileName=Join-Path $dir 'LiteSic.exe'
$startInfo.Arguments='--validation'
$startInfo.WorkingDirectory=(Get-Location).Path
$startInfo.UseShellExecute=$false
$startInfo.CreateNoWindow=$true
$p=[Diagnostics.Process]::Start($startInfo)
try {
 $rows=@()
 for($i=0;$i -le 30;$i++){
  $p.Refresh();if($p.HasExited){throw 'Application exited during idle sampling'}
  $rows+=[pscustomobject]@{seconds=$i;private_bytes=$p.PrivateMemorySize64;working_set=$p.WorkingSet64;cpu_ms=$p.TotalProcessorTime.TotalMilliseconds;threads=$p.Threads.Count;gdi=[AuditNativeCounters]::GetGuiResources($p.Handle,0);user=[AuditNativeCounters]::GetGuiResources($p.Handle,1)}
  if($i -lt 30){Start-Sleep -Seconds 1}
 }
 $rows | ConvertTo-Json | Set-Content -Encoding utf8 build/audit-m6/application-idle.json
} finally {
 $ownedPid=$p.Id
 [AuditNativeCounters]::EnumWindows({param($h,$unused) [uint32]$pidValue=0;[void][AuditNativeCounters]::GetWindowThreadProcessId($h,[ref]$pidValue);if($pidValue -eq $ownedPid){[void][AuditNativeCounters]::SendMessage($h,0x0010,[IntPtr]::Zero,[IntPtr]::Zero)};return $true},[IntPtr]::Zero) | Out-Null
 if(-not $p.WaitForExit(10000)){throw 'Isolated application did not close normally'}
 if($p.ExitCode -ne 0){throw 'Isolated application returned an error'}
}

