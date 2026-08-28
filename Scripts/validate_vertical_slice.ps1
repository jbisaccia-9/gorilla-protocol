$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$Contract = Join-Path $Root "Build\VerticalSliceAssets.txt"
$Manifest = Join-Path $Root "Licenses\AssetManifest.csv"

if (-not (Test-Path $Contract -PathType Leaf)) {
    throw "Missing vertical-slice asset contract."
}
if (-not (Test-Path $Manifest -PathType Leaf)) {
    throw "Missing asset license manifest."
}

$ManifestText = Get-Content $Manifest -Raw
$Missing = $false
foreach ($Asset in Get-Content $Contract) {
    $Asset = $Asset.Trim()
    if ([string]::IsNullOrWhiteSpace($Asset) -or $Asset.StartsWith("#")) {
        continue
    }

    if (-not (Test-Path (Join-Path $Root $Asset) -PathType Leaf)) {
        Write-Error "Missing production asset: $Asset" -ErrorAction Continue
        $Missing = $true
        continue
    }
    if (-not $ManifestText.Contains("$Asset,")) {
        Write-Error "Production asset is not licensed in AssetManifest.csv: $Asset" -ErrorAction Continue
        $Missing = $true
    }
}

if ($Missing) {
    throw "Vertical slice is not packageable. Finish authored content; do not add proxy fallbacks."
}

Write-Host "Gorilla Protocol vertical-slice asset gate passed."
