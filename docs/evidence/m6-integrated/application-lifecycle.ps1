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
 [DllImport("user32.dll",CharSet=CharSet.Unicode)] public static extern int GetClassName(IntPtr hwnd,System.Text.StringBuilder name,int count);
 [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hwnd,out uint pid);
 [DllImport("user32.dll",CharSet=CharSet.Unicode)] public static extern IntPtr SendMessage(IntPtr hwnd,uint msg,IntPtr w,IntPtr l);
}
"@
$profile=Join-Path (Get-Location) 'build/audit-m6/build/gui/m6-session'
New-Item -ItemType Directory -Force $profile | Out-Null
$ini=Join-Path $profile 'settings.ini'
@"
[Appearance]
Theme=original
VisualizerVisible=0
PlaylistVisible=0
EqualizerVisible=0
Spectrum=1
ScalePercent=100
[Equalizer]
Enabled=1
Preamp=-2
Preset=Custom
Band0=2.5
Band1=3
Band2=1
Band3=-2
Band4=6
Band5=2
Band6=3
Band7=2
Band8=1
Band9=1
"@ | Set-Content -Encoding ascii -LiteralPath $ini
$results=@()
for($run=1;$run -le 2;$run++){
 $startInfo=New-Object Diagnostics.ProcessStartInfo
$startInfo.FileName=Join-Path $dir 'LiteSic.exe'
$startInfo.Arguments='--validation'
$startInfo.WorkingDirectory=(Get-Location).Path
$startInfo.UseShellExecute=$false
$startInfo.CreateNoWindow=$true
$p=[Diagnostics.Process]::Start($startInfo)
 Start-Sleep -Seconds 2
 $p.Refresh();if($p.HasExited){throw 'Unexpected startup exit'}
 $ownedPid=$p.Id
 [AuditNativeCounters]::EnumWindows({param($h,$unused) [uint32]$pidValue=0;[void][AuditNativeCounters]::GetWindowThreadProcessId($h,[ref]$pidValue);if($pidValue -eq $ownedPid){$cls=New-Object System.Text.StringBuilder 80;[void][AuditNativeCounters]::GetClassName($h,$cls,80);Write-Output ("own window="+$cls.ToString());if($cls.ToString() -ne 'LiteSic.MainWnd'){return $true};[void][AuditNativeCounters]::SendMessage($h,0x0010,[IntPtr]::Zero,[IntPtr]::Zero)};return $true},[IntPtr]::Zero) | Out-Null
 if(-not $p.WaitForExit(10000)){throw 'Isolated application did not close normally'}
 Write-Output ("process exit="+$p.ExitCode); $content=Get-Content -Raw -LiteralPath $ini
 foreach($expected in @('Enabled=1','Preamp=-2.000000','Preset=Custom','Band0=2.500000','Band4=6.000000','Band9=1.000000','Theme=original','EqualizerVisible=0','VisualizerVisible=0','ScalePercent=100')){
  if(-not $content.Contains($expected)){throw "Lost persisted value: $expected"}
 }
 $results+=[pscustomobject]@{run=$run;exit_code=$p.ExitCode;profile=$profile;eq_hidden=$true;custom_gains_preserved=$true}
}
$results | ConvertTo-Json | Set-Content -Encoding utf8 build/audit-m6/application-lifecycle.json



