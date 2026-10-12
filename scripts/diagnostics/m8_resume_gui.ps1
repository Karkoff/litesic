$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$state=Get-Content (Join-Path $repo 'build/m8-completion/gui-pid.json') | ConvertFrom-Json
$process=Get-Process -Id $state.pid -ErrorAction SilentlyContinue
if($process){throw 'Close the explicitly selected GUI fixture before resuming.'}
$exe=Join-Path $repo 'build/64/release/LiteSic.exe'
$opened=Start-Process -FilePath $exe -ArgumentList @("--profile=$($state.profile)") -WindowStyle Hidden -PassThru
$state.pid=$opened.Id
$state | ConvertTo-Json | Set-Content -Encoding utf8 (Join-Path $repo 'build/m8-completion/gui-pid.json')
Write-Output "Resumed isolated GUI fixture PID $($opened.Id)"
