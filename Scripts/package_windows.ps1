param(
    [Parameter(Mandatory = $false)]
    [string]$UnrealRoot = $env:UE_ROOT
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($UnrealRoot)) {
    throw "Set UE_ROOT or pass -UnrealRoot with the Unreal Engine installation directory."
}

$Root = Split-Path -Parent $PSScriptRoot
$Project = Join-Path $Root "GorillaProtocol.uproject"
$Uat = Join-Path $UnrealRoot "Engine\Build\BatchFiles\RunUAT.bat"
$Archive = Join-Path $Root "Artifacts\Windows-Shipping"

if (-not (Test-Path $Uat -PathType Leaf)) {
    throw "RunUAT.bat not found under UnrealRoot."
}

& (Join-Path $Root "Scripts\validate_vertical_slice.ps1")

& $Uat BuildCookRun `
    "-project=$Project" -noP4 -platform=Win64 -clientconfig=Shipping `
    -build -cook -stage -pak -iostore -compressed -prereqs -archive `
    "-archivedirectory=$Archive" -utf8output

if ($LASTEXITCODE -ne 0) {
    throw "Unreal packaging failed with exit code $LASTEXITCODE."
}
