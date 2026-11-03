[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$PackagePath,
    [string]$ReportDirectory,
    [string]$WackPath = (Join-Path ${env:ProgramFiles(x86)} 'Windows Kits/10/App Certification Kit/appcert.exe')
)
$ErrorActionPreference='Stop'
if (-not (Test-Path -LiteralPath $WackPath)) { throw 'Windows App Certification Kit not installed.' }
$PackagePath = (Resolve-Path -LiteralPath $PackagePath).Path
if (-not $ReportDirectory) {
    $repo = Split-Path -Parent $PSScriptRoot
    $ReportDirectory = Join-Path $repo ('build/store/wack-' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
}
New-Item -ItemType Directory -Force -Path $ReportDirectory | Out-Null
$ReportDirectory=(Resolve-Path -LiteralPath $ReportDirectory).Path
$reportPath=Join-Path $ReportDirectory 'report.xml'
if(Test-Path -LiteralPath $reportPath) { throw 'Use a new report directory; previous results are preserved.' }
$reset=Start-Process -FilePath $WackPath -ArgumentList 'reset' -WindowStyle Hidden -PassThru -Wait
if($reset.ExitCode -ne 0) { throw "WACK reset failed: $($reset.ExitCode)" }
$test=Start-Process -FilePath $WackPath -ArgumentList @('test','-appxpackagepath',('"'+$PackagePath+'"'),'-reportoutputpath',('"'+$reportPath+'"')) -WindowStyle Hidden -PassThru -Wait
if(-not (Test-Path -LiteralPath $reportPath)) { throw "WACK produced no report (exit $($test.ExitCode))." }
[xml]$report=Get-Content -LiteralPath $reportPath -Raw -Encoding UTF8
$tests=@($report.SelectNodes('//TEST'))
$failed=@($tests | Where-Object { $_.SelectSingleNode('RESULT').InnerText -ne 'PASS' })
$summary=[ordered]@{
    Package=[IO.Path]::GetFileName($PackagePath);PackageSHA256=(Get-FileHash $PackagePath).Hash
    ProcessExitCode=$test.ExitCode;ReportedOverall=$report.REPORT.GetAttribute('OVERALL_RESULT')
    PartialRun=$report.REPORT.GetAttribute('PARTIAL_RUN');Tests=$tests.Count;Failed=$failed.Count
    ReportSHA256=(Get-FileHash $reportPath).Hash
    TestResults=@($tests | ForEach-Object { [ordered]@{Name=$_.GetAttribute('NAME');Result=$_.SelectSingleNode('RESULT').InnerText;Messages=$_.SelectSingleNode('MESSAGES').InnerXml} })
}
$summary | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $ReportDirectory 'summary.json') -Encoding UTF8
# The kit can report overall PASS while optional tests fail. Reject that too.
if($test.ExitCode -ne 0 -or $summary.ReportedOverall -ne 'PASS' -or $summary.PartialRun -ne 'FALSE' -or $tests.Count -eq 0 -or $failed.Count -ne 0) {
    throw "WACK did not fully pass. Exit=$($test.ExitCode), failed=$($failed.Count). See $reportPath"
}
Write-Output "WACK PASS: $($tests.Count) tests; report: $reportPath"
