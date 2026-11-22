#!/bin/zsh
set -e
cd "$(dirname "$0")/native"
if ! command -v cmake >/dev/null; then
  echo "CMake is missing. Install CMake with: brew install cmake"
  exit 1
fi
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j 4
printf '\nDAZAMP build finished. Check native/build/DAZAMP_artefacts/Release/\n'
