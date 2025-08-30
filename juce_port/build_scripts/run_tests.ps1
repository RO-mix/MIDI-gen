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
    Write-Host "Attempting to clean existing build directory: $BuildPath" -ForegroundColor Yellow
    try {
        Remove-Item -Recurse -Force $BuildPath -ErrorAction Stop
        Write-Host "Cleaned existing build directory: $BuildPath" -ForegroundColor Green
    } catch {
        Write-Host "Warning: Could not fully clean build directory. Some files may be in use." -ForegroundColor Yellow
        Write-Host "Error: $($_.Exception.Message)" -ForegroundColor Gray
    }
}

if (-not (Test-Path $BuildPath)) {
    try {
        New-Item -ItemType Directory -Path $BuildPath -ErrorAction Stop | Out-Null
        Write-Host "Created fresh build directory: $BuildPath" -ForegroundColor Green
    } catch {
        Write-Error "Failed to create build directory: $($_.Exception.Message)"
        exit 1
    }
} else {
    Write-Host "Using existing build directory: $BuildPath" -ForegroundColor Cyan
}

# Configure CMake
Write-Host "Configuring CMake..." -ForegroundColor Yellow
Push-Location $ProjectPath
try {
    $cmakeArgs = @(
        "-S", "`"$ProjectPath`"",
        "-B", "`"$BuildPath`"",
        "-G", "Visual Studio 17 2022",
        "-A", "x64",
        "-DJUCE_PATH=`"$JucePath`""
    )

    Write-Host "Running CMake with args: $($cmakeArgs -join ' ')" -ForegroundColor Yellow
    $result = & cmake @cmakeArgs 2>&1
    Write-Host "CMake exit code: $LASTEXITCODE" -ForegroundColor Yellow
    if ($LASTEXITCODE -ne 0) {
        Write-Error "CMake configuration failed"
        Write-Host $result
        exit 1
    }
    Write-Host "CMake configured successfully" -ForegroundColor Green
    Write-Host "CMake output: $result" -ForegroundColor Gray
} finally {
    Pop-Location
}

# Build HeadlessTestRunner
Write-Host "Building headless test runner..." -ForegroundColor Yellow
try {
    $result = & cmake --build $BuildPath --target HeadlessTestRunner --config Release 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Build failed for HeadlessTestRunner"
        Write-Host $result
        exit 1
    }
    Write-Host "HeadlessTestRunner built successfully" -ForegroundColor Green
} catch {
    Write-Error "Build error: $($_.Exception.Message)"
    exit 1
}

# Find and Run HeadlessTestRunner
$testExePath = Get-ChildItem -Path $BuildPath -Recurse -Filter "HeadlessTestRunner.exe" | Select-Object -First 1
if (-not $testExePath) {
    Write-Error "HeadlessTestRunner.exe not found"
    exit 1
}

Write-Host "Found test runner: $($testExePath.FullName)" -ForegroundColor Cyan
Write-Host "--- RUNNING HEADLESS TESTS ---" -ForegroundColor Yellow
try {
    # Execute and capture output. We don't use 2>&1 here to separate stdout and stderr.
    $testResult = & $testExePath.FullName
    $exitCode = $LASTEXITCODE

    Write-Host $testResult
    Write-Host "--- TESTS FINISHED (Exit Code: $exitCode) ---" -ForegroundColor Yellow

    if ($exitCode -ne 0) {
        Write-Error "Some tests failed. Halting script."
        exit $exitCode
    }
} catch {
    Write-Error "Test execution failed: $($_.Exception.Message)"
    exit 1
}


# Build standalone app
Write-Host "Building standalone application..." -ForegroundColor Yellow
try {
    $result = & cmake --build $BuildPath --target CreativeMIDIGenerator_Standalone --config Release 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Build failed for Standalone App"
        Write-Host $result
        exit 1
    }
    Write-Host "Build completed successfully" -ForegroundColor Green
} catch {
    Write-Error "Build error: $($_.Exception.Message)"
    exit 1
}

# Find executable
$exePath = Get-ChildItem -Path $BuildPath -Recurse -Filter "CreativeMIDIGenerator.exe" | Select-Object -First 1

if (-not $exePath) {
    Write-Error "Standalone executable not found"
    exit 1
}

Write-Host "Found executable: $($exePath.FullName)" -ForegroundColor Cyan

# Run standalone app
Write-Host "--- LAUNCHING APPLICATION ---" -ForegroundColor Green
try {
    & $exePath.FullName
} catch {
    Write-Error "Application execution failed: $($_.Exception.Message)"
    exit 1
}