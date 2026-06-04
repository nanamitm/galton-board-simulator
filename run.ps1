$env:PATH = "C:\Qt\6.11.1\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;" + $env:PATH

if (Test-Path ".\build\GaltonBoardSimulator.exe") {
    Write-Host "Launching Galton Board Simulator..." -ForegroundColor Cyan
    Start-Process ".\build\GaltonBoardSimulator.exe"
} else {
    Write-Host "Error: Executable not found. Please build first." -ForegroundColor Red
}
