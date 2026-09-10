param(
	[string]$WorkspaceRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path,
	[string]$ManifestPath = (Join-Path $PSScriptRoot 'Manifest.json')
)

$ErrorActionPreference = 'Stop'
$manifest = Get-Content -LiteralPath $ManifestPath -Raw | ConvertFrom-Json
$rows = @(
	foreach ($entry in @($manifest.files | Where-Object { $_.workspaceRelativePath })) {
		$baselinePath = Join-Path $PSScriptRoot ([string]$entry.path)
		$targetPath = Join-Path $WorkspaceRoot ([string]$entry.workspaceRelativePath)
		$baselineHash = ([string]$entry.sha256).ToUpperInvariant()
		if (-not (Test-Path -LiteralPath $targetPath -PathType Leaf)) {
			[ordered]@{ status = 'MISSING_TARGET'; workspacePath = $entry.workspaceRelativePath; baselinePath = $entry.path; baselineSha256 = $baselineHash }
			continue
		}
		$targetHash = (Get-FileHash -LiteralPath $targetPath -Algorithm SHA256).Hash.ToUpperInvariant()
		[ordered]@{ status = $(if ($targetHash -eq $baselineHash) { 'MATCH' } else { 'DRIFT' }); workspacePath = $entry.workspaceRelativePath; baselinePath = $entry.path; baselineSha256 = $baselineHash; workspaceSha256 = $targetHash }
	}
)

$rows | ConvertTo-Json -Depth 10
