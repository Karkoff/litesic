[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$OutputDirectory, [string]$PythonPath)
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
if (-not $PythonPath) { $PythonPath = Join-Path $repo '.tools/gui-venv/Scripts/python.exe' }
if (-not (Test-Path -LiteralPath $PythonPath)) { throw 'Use the existing GUI-tool venv (scripts/gui.ps1 -Action setup), or supply -PythonPath with Pillow installed.' }
& $PythonPath (Join-Path $PSScriptRoot 'store-assets.py') --output $OutputDirectory
if ($LASTEXITCODE -ne 0) { throw 'Store image generation failed' }
