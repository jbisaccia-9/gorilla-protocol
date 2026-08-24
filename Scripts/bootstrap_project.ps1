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
$Editor = Join-Path $UnrealRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$CreateMap = Join-Path $Root "Scripts\create_boot_map.py"
$BootMap = Join-Path $Root "Content\GorillaProtocol\Maps\L_Boot.umap"

if (-not (Test-Path $Editor -PathType Leaf)) {
    throw "UnrealEditor-Cmd.exe not found under UnrealRoot."
}

& $Editor $Project -unattended -nop4 -nosplash "-ExecutePythonScript=$CreateMap"

if ($LASTEXITCODE -ne 0) {
    throw "Boot map generation failed with exit code $LASTEXITCODE."
}

if (-not (Test-Path $BootMap -PathType Leaf)) {
    throw "Unreal completed without creating L_Boot.umap."
}

Write-Host "Boot map created. Commit L_Boot.umap through Git LFS."
