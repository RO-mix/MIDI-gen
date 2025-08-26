# Simple headless test runner for Creative MIDI Generator

param(
    [string]$JucePath = "D:\PROG\JUCE",
    [string]$ProjectPath = "$PSScriptRoot\..\juce_project",
    [string]$BuildPath = "$PSScriptRoot\..\build"
)

Write-Host "Creative MIDI Generator - Headless Test Runner" -ForegroundColor Green
Write-Host "==============================================" -ForegroundColor Green

# Check CMake
try {
    $cmakeVersion = & cmake --version 2>&1
    if ($LASTEXITCODE -eq 0) {
        Write-Host "CMake found" -ForegroundColor Green
    }
} catch {
    Write-Error "CMake not found. Please install from https://cmake.org/"
    exit 1
}

# Check JUCE
if (-not (Test-Path $JucePath)) {
    Write-Error "JUCE not found at: $JucePath"
    exit 1
} else {
    Write-Host "JUCE found: $JucePath" -ForegroundColor Green
}

# Create or clean build directory
if (Test-Path $BuildPath) {
    Remove-Item -Recurse -Force $BuildPath | Out-Null
    Write-Host "Cleaned existing build directory: $BuildPath" -ForegroundColor Cyan
}
New-Item -ItemType Directory -Path $BuildPath | Out-Null
Write-Host "Created fresh build directory: $BuildPath" -ForegroundColor Cyan

# Configure CMake
Write-Host "Configuring CMake..." -ForegroundColor Yellow
Push-Location $ProjectPath
try {
    $cmakeArgs = @(
        "-S", $ProjectPath,
        "-B", $BuildPath,
        "-G", "Unix Makefiles",
        "-DCMAKE_C_COMPILER=gcc",
        "-DCMAKE_CXX_COMPILER=g++",
        "-DJUCE_PATH=$JucePath",
        "-DCMAKE_BUILD_TYPE=Release"
    )

    $result = & cmake @cmakeArgs 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Error "CMake configuration failed"
        Write-Host $result
        exit 1
    }
    Write-Host "CMake configured successfully" -ForegroundColor Green
} finally {
    Pop-Location
}

# Build standalone app
Write-Host "Building standalone application..." -ForegroundColor Yellow
try {
    $result = & cmake --build $BuildPath --target CreativeMIDIGenerator_Standalone --config Release 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Build failed"
        Write-Host $result
        exit 1
    }
    Write-Host "Build completed successfully" -ForegroundColor Green
} catch {
    Write-Error "Build error: $($_.Exception.Message)"
    exit 1
}

# Find executable
$exePath = Get-ChildItem -Path $BuildPath -Recurse -Filter "*Standalone*.exe" | Select-Object -First 1

if (-not $exePath) {
    Write-Error "Standalone executable not found"
    exit 1
}

Write-Host "Found executable: $($exePath.FullName)" -ForegroundColor Cyan

# Run tests
Write-Host "Running headless tests..." -ForegroundColor Yellow
try {
    $testResult = & $exePath.FullName 2>&1
    $exitCode = $LASTEXITCODE

    Write-Host "Test Results:" -ForegroundColor Cyan
    Write-Host $testResult

    if ($exitCode -eq 0) {
        Write-Host "All tests passed!" -ForegroundColor Green
    } else {
        Write-Host "Some tests failed" -ForegroundColor Yellow
    }

    exit $exitCode
} catch {
    Write-Error "Test execution failed: $($_.Exception.Message)"
    exit 1
}