#!/bin/bash
set -e

mkdir -p bin

CXX_FLAGS="-O3 -std=c++20 -march=native -flto -Wall -Wextra -I include"
LIBS="-lrt -lpthread"

echo "[Build] Compiling producer for Linux..."
g++ $CXX_FLAGS src/producer.cpp -o bin/producer $LIBS

echo "[Build] Compiling consumer for Linux..."
g++ $CXX_FLAGS src/consumer.cpp -o bin/consumer $LIBS

echo -e "\n[Build] Success! Linux binaries ready in ./bin/"