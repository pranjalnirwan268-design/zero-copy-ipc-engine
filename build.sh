#!/bin/bash
set -e

mkdir -p bin

echo "[Build] Compiling producer for Linux..."
g++ -O3 -std=c++20 -I include src/producer.cpp -o bin/producer -lrt -lpthread

echo "[Build] Compiling consumer for Linux..."
g++ -O3 -std=c++20 -I include src/consumer.cpp -o bin/consumer -lrt -lpthread

echo -e "\n[Build] Success! Linux binaries ready in ./bin/"