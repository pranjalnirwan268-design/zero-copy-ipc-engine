if (-not (Test-Path -Path "bin")) {
    New-Item -ItemType Directory -Path "bin" | Out-Null
}

Write-Host "[Build] Compiling producer.exe..." -ForegroundColor Cyan
g++ -std=c++20 -I include src/producer.cpp -o bin/producer.exe

Write-Host "[Build] Compiling consumer.exe..." -ForegroundColor Cyan
g++ -std=c++20 -I include src/consumer.cpp -o bin/consumer.exe

Write-Host "`n[Build] Success! Executables ready in ./bin/" -ForegroundColor Green