param(
	[string]$ManifestPath = (Join-Path $PSScriptRoot 'Manifest.json')
)

$ErrorActionPreference = 'Stop'
$manifest = Get-Content -LiteralPath $ManifestPath -Raw | ConvertFrom-Json
$failures = [System.Collections.Generic.List[string]]::new()
$checked = 0

foreach ($entry in @($manifest.files)) {
	$fullPath = Join-Path $PSScriptRoot ([string]$entry.path)
	if (-not (Test-Path -LiteralPath $fullPath -PathType Leaf)) {
		$failures.Add("MISSING $($entry.path)")
		continue
	}
	if ([string]$entry.integrityMode -eq 'detached') {
		continue
	}
	$checked++
	$actual = (Get-FileHash -LiteralPath $fullPath -Algorithm SHA256).Hash.ToUpperInvariant()
	if ($actual -ne ([string]$entry.sha256).ToUpperInvariant()) {
		$failures.Add("HASH $($entry.path) expected=$($entry.sha256) actual=$actual")
	}
}

$detachedPath = Join-Path $PSScriptRoot 'Manifest.sha256'
if (-not (Test-Path -LiteralPath $detachedPath -PathType Leaf)) {
	$failures.Add('MISSING Manifest.sha256')
} else {
	$expectedManifestHash = ((Get-Content -LiteralPath $detachedPath -Raw).Trim() -split '\s+')[0].ToUpperInvariant()
	$actualManifestHash = (Get-FileHash -LiteralPath $ManifestPath -Algorithm SHA256).Hash.ToUpperInvariant()
	if ($expectedManifestHash -ne $actualManifestHash) {
		$failures.Add("HASH Manifest.json expected=$expectedManifestHash actual=$actualManifestHash")
	}
}

if ($failures.Count -gt 0) {
	$failures | ForEach-Object { Write-Output $_ }
	Write-Output "FAIL checked=$checked failures=$($failures.Count)"
	exit 1
}

Write-Output "PASS checked=$checked manifestSelfHash=detached"
exit 0
