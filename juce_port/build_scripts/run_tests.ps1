# Simple headless test runner for Creative MIDI Generator

param(
    [string]$JucePath = "D:\PROG\JUCE",
    [string]$ProjectPath = "$PSScriptRoot\..\juce_project",
    [string]$BuildPath = "$PSScriptRoot\..\build",
    [switch]$NoClean = $false
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
if ($NoClean) {
    Write-Host "Running with -NoClean option. Build directory will not be cleared." -ForegroundColor Cyan
}
elseif (Test-Path $BuildPath) {
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
Write-Host "Now running headless tests..." -ForegroundColor Yellow
try {
    # Execute and capture output. We don't use 2>&1 here to separate stdout and stderr.
    $testResult = & $testExePath.FullName
    $exitCode = $LASTEXITCODE

    Write-Host $testResult
    Write-Host "Tests finished with exit code: $exitCode" -ForegroundColor Yellow

    if ($exitCode -ne 0) {
        Write-Error "Some tests failed. Halting script."
        exit $exitCode
    }
} catch {
    Write-Error "Test execution failed: $($_.Exception.Message)"
    exit 1
}


# Build all plugin formats
Write-Host "Building all plugin formats (VST3, Standalone)..." -ForegroundColor Yellow
try {
    $result = & cmake --build $BuildPath --target CreativeMIDIGenerator --config Release 2>&1
    Write-Host $result # Always print the build output
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Build failed for plugin formats"
        exit 1
    }
    Write-Host "Plugin formats built successfully" -ForegroundColor Green
} catch {
    Write-Error "Build error: $($_.Exception.Message)"
    exit 1
}

# --- DEBUG: List all files in the build directory ---
Write-Host "--- DEBUG: Listing contents of build directory ---" -ForegroundColor Magenta
Get-ChildItem -Path $BuildPath -Recurse | Select-Object FullName
Write-Host "--- DEBUG: End of file list ---" -ForegroundColor Magenta

# Verify build artifacts
$exePath = Get-ChildItem -Path $BuildPath -Recurse -File -Filter "CreativeMIDIGenerator.exe" | Select-Object -First 1
if (-not $exePath) {
    Write-Error "Standalone executable not found"
} else {
    Write-Host "Found Standalone executable: $($exePath.FullName)" -ForegroundColor Cyan
}

$vst3Path = Get-ChildItem -Path $BuildPath -Recurse -Directory -Filter "CreativeMIDIGenerator.vst3" | Select-Object -First 1
if (-not $vst3Path) {
    Write-Host "Warning: VST3 plugin not found" -ForegroundColor Yellow
} else {
    Write-Host "Found VST3 plugin: $($vst3Path.FullName)" -ForegroundColor Cyan
}

if (-not $exePath) {
    # Fail the script if the main executable is missing
    exit 1
}

# Run standalone app (optional, for local testing)
Write-Host "--- SCRIPT FINISHED ---" -ForegroundColor Green
Write-Host "Note: To run the standalone app, execute $($exePath.FullName)"