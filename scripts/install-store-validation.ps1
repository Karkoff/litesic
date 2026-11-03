[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$PackagePath,
    [Parameter(Mandatory=$true)][string]$ResultPath,
    [switch]$ReplaceInstalledValidation
)
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent $PSScriptRoot
$allowed=[IO.Path]::GetFullPath((Join-Path $repo 'build/store'))+[IO.Path]::DirectorySeparatorChar
$ResultPath=[IO.Path]::GetFullPath($ResultPath)
if(-not $ResultPath.StartsWith($allowed,[StringComparison]::OrdinalIgnoreCase)) { throw 'Result must be inside build/store.' }
$PackagePath=(Resolve-Path -LiteralPath $PackagePath).Path
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip=[IO.Compression.ZipFile]::OpenRead($PackagePath)
try {
    $reader=New-Object IO.StreamReader($zip.GetEntry('AppxManifest.xml').Open())
    try { [xml]$manifest=$reader.ReadToEnd() } finally { $reader.Dispose() }
} finally { $zip.Dispose() }
$identity=$manifest.Package.Identity
if($identity.Name -ne 'LiteSic.LocalValidation' -or $identity.Publisher -notmatch 'OID\.2\.25\.311729368913984317654407730594956997722=1') {
    throw 'This helper installs only the separate unsigned local-validation identity.'
}
$result=[ordered]@{Package=[IO.Path]::GetFileName($PackagePath);PackageSHA256=(Get-FileHash $PackagePath).Hash;Status='FAILED';Message='';PackageFullName='';PreviousPackage='';PreviousProfileExisted=$false;PreviousProfileRemoved=$null}
try {
    $principal=New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
    if(-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) { throw 'An administrator must approve the normal Windows elevation prompt.' }
    $previous=Get-AppxPackage -Name 'LiteSic.LocalValidation'
    if($previous) {
        if(-not $ReplaceInstalledValidation) { throw 'A validation package is already installed. Explicitly select -ReplaceInstalledValidation for the uninstall/reinstall test.' }
        $result.PreviousPackage=$previous.PackageFullName
        $previousProfile=Join-Path $env:LOCALAPPDATA ('Packages/'+$previous.PackageFamilyName+'/LocalState/LiteSic')
        $result.PreviousProfileExisted=Test-Path -LiteralPath $previousProfile
        Remove-AppxPackage -Package $previous.PackageFullName
        if(Get-AppxPackage -Name 'LiteSic.LocalValidation') { throw 'Previous test package is still registered.' }
        $result.PreviousProfileRemoved=-not (Test-Path -LiteralPath $previousProfile)
    }
    Add-AppxPackage -Path $PackagePath -AllowUnsigned
    $installed=Get-AppxPackage -Name 'LiteSic.LocalValidation'
    if(-not $installed) { throw 'No registered validation package found after installation.' }
    $result.PackageFullName=$installed.PackageFullName
    $result.Status='INSTALLED_RUNTIME_TESTS_PENDING'
} catch {
    $result.Message=$_.Exception.Message
} finally {
    $result | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $ResultPath -Encoding UTF8
}
if($result.Status -eq 'FAILED') { throw $result.Message }
