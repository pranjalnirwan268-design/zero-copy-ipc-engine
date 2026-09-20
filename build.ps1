$ErrorActionPreference = "Stop"

if (-not (Test-Path -Path "bin")) {
    New-Item -ItemType Directory -Path "bin" | Out-Null
}

$CXX_FLAGS = "-O3", "-std=c++20", "-march=native", "-flto", "-Wall", "-Wextra", "-I", "include"

Write-Host "[Build] Compiling producer.exe..." -ForegroundColor Cyan
g++ @CXX_FLAGS src/producer.cpp -o bin/producer.exe
if ($LASTEXITCODE -ne 0) {
    Write-Host "[Build] Failed to compile producer.exe!" -ForegroundColor Red
    exit $LASTEXITCODE
}

Write-Host "[Build] Compiling consumer.exe..." -ForegroundColor Cyan
g++ @CXX_FLAGS src/consumer.cpp -o bin/consumer.exe
if ($LASTEXITCODE -ne 0) {
    Write-Host "[Build] Failed to compile consumer.exe!" -ForegroundColor Red
    exit $LASTEXITCODE
}

Write-Host "`n[Build] Success! Executables ready in ./bin/" -ForegroundColor Green