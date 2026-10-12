[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$driver = Join-Path (Split-Path -Parent $PSScriptRoot) 'gui.ps1'
# Launch/capture/close only. No coordinates or keyboard input and no personal profile.
foreach ($catalogCode in @('pt-BR','en-US','es-ES','fr-FR','de-DE','it-IT','pt-PT','nl-NL','pl-PL','ru-RU','ja-JP','zh-CN','zh-TW','ko-KR','tr-TR','ar-SA','hi-IN','sv-SE')) {
    & $driver -Action launch -Architecture 64 -Configuration release -Validation -Language $catalogCode -Label "i18n-$catalogCode-main"
    if ($LASTEXITCODE -ne 0) { throw "Launch failed: $catalogCode" }
    try {
        & $driver -Action capture -Target playlist -Label "i18n-$catalogCode-playlist"
        if ($LASTEXITCODE -ne 0) { throw "Playlist capture failed: $catalogCode" }
        # The isolated profile must have EQ visible (set via the observed existing control).
        & $driver -Action capture -Target eq -Label "i18n-$catalogCode-eq"
        if ($LASTEXITCODE -ne 0) { throw "EQ capture failed: $catalogCode" }
    }
    finally { & $driver -Action close }
}
