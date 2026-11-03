[CmdletBinding(DefaultParameterSetName='Store')]
param(
    [Parameter(ParameterSetName='Store')][string]$IdentityFile,
    [Parameter(Mandatory=$true,ParameterSetName='Validation')][switch]$ValidationOnly,
    [ValidatePattern('^\d+\.\d+\.\d+\.0$')][string]$Version = '1.0.0.0',
    [string]$SdkBin,
    [string]$AssetPython,
    [string]$Msys2Root = $env:LITESIC_MSYS2_ROOT
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = Split-Path -Parent $PSScriptRoot
if (-not $Msys2Root) { $Msys2Root = Join-Path $repo '.tools/msys64' }
$versionObject = [Version]$Version
if ($versionObject.Major -lt 1 -or $versionObject.Major -gt 65535 -or
    $versionObject.Minor -gt 65535 -or $versionObject.Build -gt 65535) { throw 'Invalid Store version' }
if ($ValidationOnly) {
    $identity = [pscustomobject]@{Name='LiteSic.LocalValidation';Publisher='CN=LiteSic Local Validation, OID.2.25.311729368913984317654407730594956997722=1';PublisherDisplayName='LiteSic local validation';StoreId=''}
} else {
    if (-not $IdentityFile) { $IdentityFile = Join-Path $repo 'packaging/store/identity.json' }
    $identity = Get-Content -LiteralPath $IdentityFile -Raw -Encoding UTF8 | ConvertFrom-Json
    if ($identity.Name -notmatch '^[A-Za-z0-9.-]{3,50}$' -or
        $identity.Publisher -notmatch '^CN=\S.+' -or
        [string]::IsNullOrWhiteSpace($identity.PublisherDisplayName) -or
        $identity.StoreId -notmatch '^[A-Z0-9]{12}$' -or
        $identity.Name -match '(?i)placeholder|example|localvalidation') {
        throw 'Supply the official public product identity from Partner Center.'
    }
}
if (-not $SdkBin) {
    $sdkRoot = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits/10/bin'
    $sdk = Get-ChildItem -LiteralPath $sdkRoot -Directory | Where-Object {
        $_.Name -match '^10\.0\.\d+\.0$' -and (Test-Path (Join-Path $_.FullName 'x64/makeappx.exe'))
    } | Sort-Object { [Version]$_.Name } -Descending | Select-Object -First 1
    if (-not $sdk) { throw 'Windows SDK MakeAppx.exe is required on the packaging machine.' }
    $SdkBin = Join-Path $sdk.FullName 'x64'
}
$makeappx = Join-Path $SdkBin 'makeappx.exe'
if (-not (Test-Path -LiteralPath $makeappx)) { throw "Missing $makeappx" }
$makepri = Join-Path $SdkBin 'makepri.exe'
if (-not (Test-Path -LiteralPath $makepri)) { throw "Missing $makepri" }
$hostVersion = [Environment]::OSVersion.Version
if ($hostVersion.Major -lt 10 -or $hostVersion.Build -lt 17763) { throw 'Package on Windows 10 1809 or later.' }
$maxTested = '10.0.' + $hostVersion.Build + '.0'
$mode = if ($ValidationOnly) { 'validation' } else { 'store' }
$run = Join-Path $repo ('build/store/' + $mode + '-' + $Version + '-' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
# A unique, never-cleaned output keeps previous artifacts and working builds intact.
New-Item -ItemType Directory -Path $run -Force | Out-Null
$bundleInput = Join-Path $run 'bundle-input'
New-Item -ItemType Directory -Path $bundleInput | Out-Null
$log = Join-Path $run 'packaging.txt'
function Invoke-MakeAppx([string[]]$Arguments) {
    $output = @(& $makeappx @Arguments 2>&1)
    $output | Out-File -FilePath $log -Append -Encoding UTF8
    if ($LASTEXITCODE -ne 0) { $output | Select-Object -Last 20 | Out-Host; throw "MakeAppx failed with exit code $LASTEXITCODE; see $log" }
    Write-Host "MakeAppx $($Arguments[0]): PASS"
}
$records = @()
foreach ($architecture in @('64','32')) {
    # Isolate outputs: a running traditional player can lock its Release exe.
    # Use the same Makefile, compiler, flags and resources without replacing it.
    $saved = @{}
    foreach ($name in @('MSYSTEM','CHERE_INVOKING','TMP','TEMP','TMPDIR')) { $saved[$name]=[Environment]::GetEnvironmentVariable($name,'Process') }
    Push-Location $repo
    try {
        $env:MSYSTEM = if ($architecture -eq '64') { 'MINGW64' } else { 'MINGW32' }
        $env:CHERE_INVOKING = '1'
        $tempPath = Join-Path $repo 'build/tmp'
        New-Item -ItemType Directory -Force -Path $tempPath | Out-Null
        $env:TMP=$tempPath; $env:TEMP=$tempPath; $env:TMPDIR=$tempPath
        & (Join-Path $Msys2Root 'usr/bin/bash.exe') -lc "set -o pipefail; make -j4 ARCH=$architecture DEBUG=0 BUILD_DIR=build/store-native/$architecture/release INTERNAL_DIR=build/internal/store-native/$architecture/release all 2>&1 | tee build/store/native-build-$architecture.txt"
        if ($LASTEXITCODE -ne 0) { throw "Release build failed: $architecture" }
    } finally {
        Pop-Location
        foreach ($name in $saved.Keys) { [Environment]::SetEnvironmentVariable($name,$saved[$name],'Process') }
    }
    $arch = if ($architecture -eq '64') { 'x64' } else { 'x86' }
    $prefix = if ($architecture -eq '64') { 'mingw64' } else { 'mingw32' }
    $layout = Join-Path $run "layout-$arch"
    New-Item -ItemType Directory -Path $layout | Out-Null
    $exe = Join-Path $repo "build/store-native/$architecture/release/LiteSic.exe"
    $pe = [IO.File]::ReadAllBytes($exe)
    $machine = [BitConverter]::ToUInt16($pe, ([BitConverter]::ToInt32($pe,0x3c)+4))
    $expectedMachine = if ($arch -eq 'x64') { 0x8664 } else { 0x14c }
    if ($machine -ne $expectedMachine) { throw "PE architecture mismatch: $arch" }
    Copy-Item -LiteralPath $exe -Destination (Join-Path $layout 'LiteSic.exe')
    $catalogs = @(Get-ChildItem -LiteralPath (Join-Path $repo 'locales') -Directory | Sort-Object Name)
    if ($catalogs.Count -ne 18) { throw 'Review the supported-language contract before packaging.' }
    foreach ($catalog in $catalogs) {
        $destination = Join-Path $layout ('locales/' + $catalog.Name)
        New-Item -ItemType Directory -Path $destination -Force | Out-Null
        Copy-Item -LiteralPath (Join-Path $catalog.FullName 'messages.tsv') -Destination $destination
    }
    $licenseDir = Join-Path $layout 'licenses'
    New-Item -ItemType Directory -Path $licenseDir | Out-Null
    Copy-Item -Path (Join-Path $repo 'packaging/licenses/*.txt') -Destination $licenseDir
    # Toolchain notices are build inputs, not development tools in the package.
    foreach ($entry in @(
        @('libgcc/COPYING3','GCC-GPL-3.0.txt'),
        @('libgcc/COPYING.RUNTIME','GCC-Runtime-Exception.txt'),
        @('crt/COPYING','MinGW-COPYING.txt'),
        @('crt/COPYING.MinGW-w64.txt','MinGW-w64.txt'),
        @('crt/COPYING.MinGW-w64-runtime.txt','MinGW-w64-runtime.txt')
    )) {
        Copy-Item -LiteralPath (Join-Path $Msys2Root "$prefix/share/licenses/$($entry[0])") -Destination (Join-Path $licenseDir $entry[1])
    }
    & (Join-Path $PSScriptRoot 'store-assets.ps1') -OutputDirectory (Join-Path $layout 'Assets') -PythonPath $AssetPython
    # The listing-only 300 px logo does not belong in the runtime payload.
    $listingDir = Join-Path $run 'listing'
    New-Item -ItemType Directory -Force -Path $listingDir | Out-Null
    Move-Item -LiteralPath (Join-Path $layout 'Assets/StoreListingLogo.png') -Destination (Join-Path $listingDir "StoreListingLogo-$arch.png")
    $template = Get-Content -LiteralPath (Join-Path $repo 'packaging/store/AppxManifest.xml.in') -Raw -Encoding UTF8
    $languages = ($catalogs | ForEach-Object { '    <Resource Language="' + $_.Name + '" />' }) -join "`r`n"
    $substitutions = [ordered]@{
        '@NAME@'=$identity.Name; '@PUBLISHER@'=$identity.Publisher
        '@PUBLISHER_DISPLAY_NAME@'=$identity.PublisherDisplayName
        '@VERSION@'=$Version; '@ARCH@'=$arch; '@MAX_VERSION_TESTED@'=$maxTested
    }
    foreach ($key in $substitutions.Keys) { $template = $template.Replace($key,[Security.SecurityElement]::Escape([string]$substitutions[$key])) }
    $template = $template.Replace('@LANGUAGES@',$languages)
    [IO.File]::WriteAllText((Join-Path $layout 'AppxManifest.xml'),$template,(New-Object Text.UTF8Encoding($false)))
    $priConfig = Join-Path $run "priconfig-$arch.xml"
    & $makepri createconfig /cf $priConfig /dq en-US 2>&1 | Out-File -FilePath $log -Append -Encoding UTF8
    if ($LASTEXITCODE -ne 0) { throw 'MakePri configuration failed' }
    # Keep every language and scale in the main package; no optional resource packs.
    [xml]$priXml = Get-Content -LiteralPath $priConfig -Raw
    $packaging = $priXml.SelectSingleNode('//packaging')
    if ($packaging) { $packaging.ParentNode.RemoveChild($packaging) | Out-Null; $priXml.Save($priConfig) }
    & $makepri new /pr $layout /cf $priConfig /mn (Join-Path $layout 'AppxManifest.xml') /of (Join-Path $layout 'resources.pri') 2>&1 | Out-File -FilePath $log -Append -Encoding UTF8
    if ($LASTEXITCODE -ne 0) { throw 'MakePri indexing failed' }
    $objdump = Join-Path $Msys2Root "$prefix/bin/objdump.exe"
    $dump = @(& $objdump -p $exe)
    if ($LASTEXITCODE -ne 0) { throw 'PE dependency audit failed' }
    $imports = @($dump | Select-String 'DLL Name:\s*(.+)' | ForEach-Object { $_.Matches[0].Groups[1].Value.Trim() })
    $systemDlls = @('comctl32.dll','comdlg32.dll','d2d1.dll','dwrite.dll','gdi32.dll','kernel32.dll','msvcrt.dll','ole32.dll','oleacc.dll','oleaut32.dll','shell32.dll','user32.dll','winmm.dll','advapi32.dll','uuid.dll')
    foreach ($dll in $imports) { if ($dll.ToLowerInvariant() -notin $systemDlls) { throw "Review unexpected dependency: $dll" } }
    $package = Join-Path $bundleInput "LiteSic-$Version-$arch.msix"
    # Do not use /nv: schema and semantic validation must run.
    Invoke-MakeAppx -Arguments @('pack','/d',$layout,'/p',$package,'/h','SHA256')
    $unpacked = Join-Path $run "unpacked-$arch"
    Invoke-MakeAppx -Arguments @('unpack','/p',$package,'/d',$unpacked)
    $files = @(Get-ChildItem -LiteralPath $layout -Recurse -File | ForEach-Object {
        $relative = $_.FullName.Substring($layout.Length+1)
        $hash = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
        if ((Get-FileHash -LiteralPath (Join-Path $unpacked $relative) -Algorithm SHA256).Hash -ne $hash) { throw "Unpack integrity failed: $relative" }
        [ordered]@{Path=$relative;Bytes=$_.Length;SHA256=$hash}
    })
    $sourceVersion = [regex]::Match((Get-Content (Join-Path $repo 'res/app.rc') -Raw),'FILEVERSION\s+(\d+),(\d+),(\d+),(\d+)')
    if (-not $sourceVersion.Success) { throw 'Missing native source version' }
    $nativeVersion = ($sourceVersion.Groups | Select-Object -Skip 1 | ForEach-Object { $_.Value }) -join '.'
    $records += [ordered]@{Architecture=$arch;NativeSourceVersion=$nativeVersion;WindowsFileVersion=[Diagnostics.FileVersionInfo]::GetVersionInfo($exe).FileVersion;ExecutableSHA256=(Get-FileHash $exe).Hash;Imports=$imports;PackageSHA256=(Get-FileHash $package).Hash;Files=$files}
}
$bundle = Join-Path $run "LiteSic-$Version-$mode.msixbundle"
Invoke-MakeAppx -Arguments @('bundle','/d',$bundleInput,'/p',$bundle,'/bv',$Version)
$unbundle = Join-Path $run 'unbundled'
Invoke-MakeAppx -Arguments @('unbundle','/p',$bundle,'/d',$unbundle)
foreach ($package in Get-ChildItem -LiteralPath $bundleInput -File) {
    if ((Get-FileHash $package.FullName).Hash -ne (Get-FileHash (Join-Path $unbundle $package.Name)).Hash) { throw 'Bundle integrity failed' }
}
$metadata = [ordered]@{
    Mode=$mode;ReadyForSubmission=$false;SubmissionStatus='NOT_SUBMITTED'
    Created=[DateTimeOffset]::Now.ToString('o');SourceCommit=(& git -C $repo rev-parse HEAD)
    SourceChanges=@(& git -C $repo status --short);Identity=$identity;Version=$Version
    MinimumWindows='10.0.17763.0';MaxVersionTested=$maxTested
    Bundle=[IO.Path]::GetFileName($bundle);BundleSHA256=(Get-FileHash $bundle).Hash
    Signature='Unsigned; Microsoft Store signs the submitted package'
    Certification='NOT_RUN';RuntimeValidation='PENDING_PACKAGED_INSTALLATION'
    Architectures=$records
}
$metadata | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath (Join-Path $run 'package-evidence.json') -Encoding UTF8
if ($ValidationOnly) { 'LOCAL VALIDATION ONLY - DO NOT SUBMIT' | Set-Content -LiteralPath (Join-Path $run 'VALIDATION-ONLY.txt') }
Write-Output "Bundle: $bundle"
Write-Output "Evidence: $(Join-Path $run 'package-evidence.json')"
Write-Output 'Packaging passed. Installation, runtime, WACK and Partner Center review are separate gates.'
Copy-Item -Path (Join-Path $repo 'packaging/store/listing.*.json') -Destination $listingDir
Copy-Item -LiteralPath (Join-Path $repo 'packaging/store/submission.json'),(Join-Path $repo 'docs/PRIVACY.md') -Destination $listingDir
$screenshots = Join-Path $repo 'packaging/store/screenshots'
if(Test-Path -LiteralPath $screenshots) { Copy-Item -LiteralPath $screenshots -Destination $listingDir -Recurse }
