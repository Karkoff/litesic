<#
Capture all five existing LiteSic windows while switching the production
language commands in an already launched, isolated validation session.
No Windows preference or personal LiteSic configuration is changed.
#>
[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$python = Join-Path $root '.tools/gui-venv/Scripts/python.exe'
$driver = Join-Path $PSScriptRoot 'litesic_gui.py'
$codes = @('pt-BR','en-US','es-ES','fr-FR','de-DE','it-IT','pt-PT','nl-NL','pl-PL','ru-RU','ja-JP','zh-CN','zh-TW','ko-KR','tr-TR','ar-SA','hi-IN','sv-SE')
foreach ($code in $codes) {
    & $python $driver capture --target main --label "t7-$code-before"
    if ($LASTEXITCODE) { throw "Capture failed: $code" }
    & $python $driver language --target main --query $code --label "t7-$code-main"
    if ($LASTEXITCODE) { throw "Language change failed: $code" }
    foreach ($target in @('playlist','eq','themes','info')) {
        & $python $driver capture --target $target --label "t7-$code-$target"
        if ($LASTEXITCODE) { throw "Capture failed: $code/$target" }
    }
}
