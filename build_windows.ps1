param(
    [string]$JucePath = "C:/JUCE",
    [string]$Config = "Release"
)

if (-not (Test-Path $JucePath)) {
    Write-Host "JUCE path not found: $JucePath"
    Write-Host "Download JUCE or pass -JucePath 'C:/path/to/JUCE'"
    exit 1
}

cmake -B build -DJUCE_PATH="$JucePath"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

cmake --build build --config $Config
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "Build finished. Look for the VST3 in:"
Write-Host "build/ArkShimmerReverb_artefacts/$Config/VST3/"
