#!/usr/bin/env bash
set -euo pipefail
JUCE_PATH="${1:-/Applications/JUCE}"
CONFIG="${2:-Release}"

cmake -B build -DJUCE_PATH="$JUCE_PATH"
cmake --build build --config "$CONFIG"

echo "Build finished. Look for the VST3 in:"
echo "build/ArkShimmerReverb_artefacts/$CONFIG/VST3/"
