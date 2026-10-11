[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('setup','doctor','fixture','m5-fixtures','launch','inspect','capture','click','key','open-fixture','close','test','drag','resize')]
    [string]$Action,
    [ValidateSet('32','64')][string]$Architecture = '64',
    [ValidateSet('debug','release')][string]$Configuration = 'debug',
    [string]$Label,
    [int]$X = -1,
    [int]$Y = -1,
    [ValidateSet('left','right')][string]$Button = 'left',
    [ValidateSet('escape','enter','tab','space','left','right','home','end','up','down','delete','ctrl-a')][string]$Key,
    [ValidateSet('main','playlist','eq','menu')][string]$Target = 'main',
    [ValidateSet('','ctrl','shift')][string]$Modifier = '',
    [switch]$Double,
    [string]$Fixture = 'silence',
    [int]$Width,
    [int]$Height,
    [int]$EndX,
    [int]$EndY,
    [int]$MenuIndex = -1,
    [switch]$Dialog
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$venvRoot = Join-Path $projectRoot '.tools\gui-venv'
$pythonExe = Join-Path $venvRoot 'Scripts\python.exe'
if ($Action -eq 'setup') {
    # Do not install into the global interpreter or relax HTTPS validation.
    & py -3.11 -c "import struct,sys; assert sys.version_info[:2] == (3,11) and struct.calcsize('P') == 8, 'Requer Python 3.11 x64'"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    if (-not (Test-Path -LiteralPath $pythonExe)) {
        & py -3.11 -m venv $venvRoot
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
    & $pythonExe -m pip --use-feature=truststore --disable-pip-version-check install --only-binary=:all: --require-hashes -r (Join-Path $PSScriptRoot 'gui\requirements-win-py311.lock')
    exit $LASTEXITCODE
}
if (-not (Test-Path -LiteralPath $pythonExe)) { throw 'Execute scripts/gui.ps1 -Action setup primeiro.' }
if ($Action -eq 'test') {
    & $pythonExe (Join-Path $PSScriptRoot 'gui\test_driver.py')
    exit $LASTEXITCODE
}
$driverArgs = @((Join-Path $PSScriptRoot 'gui\litesic_gui.py'), $Action,
    '--arch', $Architecture, '--config', $Configuration, '--target', $Target, '--fixture', $Fixture)
if ($Label) { $driverArgs += @('--label', $Label) }
if ($Dialog) { $driverArgs += '--dialog' }
if ($MenuIndex -ge 0) { $driverArgs += @('--menu-index',$MenuIndex) }
if ($Action -in @('click','drag')) { $driverArgs += @('--x', $X, '--y', $Y, '--button', $Button) }
if ($Action -eq 'click' -and $Modifier) { $driverArgs += @('--modifier', $Modifier) }
if ($Double) { $driverArgs += '--double' }
if ($Action -eq 'drag') { $driverArgs += @('--end-x', $EndX, '--end-y', $EndY) }
if ($Action -eq 'resize') { $driverArgs += @('--width', $Width, '--height', $Height) }
if ($Action -eq 'key') {
    if (-not $Key) { throw '-Action key requer -Key.' }
    $driverArgs += @('--key', $Key)
}
& $pythonExe @driverArgs
exit $LASTEXITCODE
