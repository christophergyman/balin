#!/bin/bash
set -e

cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug
# Debug and Release both write output/balin. Remove the stale binary so the
# debug link always runs and ./output/balin is the build with the overlay.
rm -f output/balin
cmake --build build/debug -j
./output/balin
