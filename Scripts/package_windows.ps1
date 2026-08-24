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
$BootMap = Join-Path $Root "Content\GorillaProtocol\Maps\L_Boot.umap"
$Archive = Join-Path $Root "Artifacts\Windows-Shipping"

if (-not (Test-Path $Uat -PathType Leaf)) {
    throw "RunUAT.bat not found under UnrealRoot."
}

if (-not (Test-Path $BootMap -PathType Leaf)) {
    throw "Missing boot map. Run Scripts/bootstrap_project.ps1 after installing UE5.8."
}

& $Uat BuildCookRun `
    "-project=$Project" -noP4 -platform=Win64 -clientconfig=Shipping `
    -build -cook -stage -pak -iostore -compressed -prereqs -archive `
    "-archivedirectory=$Archive" -utf8output

if ($LASTEXITCODE -ne 0) {
    throw "Unreal packaging failed with exit code $LASTEXITCODE."
}
