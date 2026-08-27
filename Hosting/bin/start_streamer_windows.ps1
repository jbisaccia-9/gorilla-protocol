param(
    [Parameter(Mandatory = $true)]
    [string]$GameExecutable,
    [string]$StreamerUrl = "ws://127.0.0.1:8888",
    [int]$Width = 1280,
    [int]$Height = 720
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path $GameExecutable -PathType Leaf)) {
    throw "Packaged game executable not found: $GameExecutable"
}

& $GameExecutable `
    "-PixelStreamingURL=$StreamerUrl" `
    -PixelStreamingEncoderCodec=H264 `
    -PixelStreamingEncoderRateControl=CBR `
    -PixelStreamingWebRTCMaxFps=60 `
    -PixelStreamingWebRTCDegradationPreference=MAINTAIN_FRAMERATE `
    -RenderOffscreen -ForceRes "-ResX=$Width" "-ResY=$Height" `
    -AudioMixer -Unattended -StdOut -FullStdOutLogOutput

exit $LASTEXITCODE
