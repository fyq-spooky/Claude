#!/bin/sh
# Baut und startet die Standalone-Tests des Simulationskerns (Linux/macOS, g++ oder clang++).
set -e
cd "$(dirname "$0")"
CXX="${CXX:-g++}"
FLAGS="-std=c++17 -O2 -Wall -Wextra -Wshadow -DSCOOTERSIM_STANDALONE"
"$CXX" $FLAGS SimTest.cpp ../../Source/ScooterSim/ScooterSimCore.cpp -o simtest
"$CXX" $FLAGS JumpTest.cpp ../../Source/ScooterSim/ScooterSimCore.cpp -o jumptest
./simtest
./jumptest
