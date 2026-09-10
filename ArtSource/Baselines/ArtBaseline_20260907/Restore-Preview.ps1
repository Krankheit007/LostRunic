param(
	[string]$WorkspaceRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path,
	[string]$ManifestPath = (Join-Path $PSScriptRoot 'Manifest.json')
)

$ErrorActionPreference = 'Stop'
$manifest = Get-Content -LiteralPath $ManifestPath -Raw | ConvertFrom-Json
Write-Output 'PREVIEW ONLY: no files or Unreal scene state will be changed.'
Write-Output 'Close Unreal Editor before a human applies any selected copy operation.'
Write-Output ''

foreach ($entry in @($manifest.files | Where-Object { $_.workspaceRelativePath })) {
	$source = Join-Path $PSScriptRoot ([string]$entry.path)
	$destination = Join-Path $WorkspaceRoot ([string]$entry.workspaceRelativePath)
	Write-Output ("Copy-Item -LiteralPath '{0}' -Destination '{1}' -Force" -f $source, $destination)
}

Write-Output ''
Write-Output 'The preview is intentionally limited to manifest targets. It does not save assets, start PIE, or edit camera/light/material properties.'
