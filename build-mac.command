#!/bin/zsh
set -e
cd "$(dirname "$0")/native"
if ! command -v cmake >/dev/null; then
  echo "CMake missing. Install: brew install cmake"
  exit 1
fi
MODE=OFF
if [[ "${1:-}" == "--nam" ]]; then MODE=ON; fi
echo "DAZAMP v0.3 build — NAM support: $MODE"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DDAZAMP_WITH_NAM="$MODE"
cmake --build build --config Release -j 4
printf '\nCheck native/build/DAZAMP_artefacts/Release/ for AU, VST3, standalone.\n'
