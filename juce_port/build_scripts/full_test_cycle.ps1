# Full Test Cycle for Creative MIDI Generator
# Runs complete test suite: environment check -> build validation -> headless tests -> standalone launch

param(
    [switch]$SkipBuildCheck = $false,
    [switch]$SkipHeadlessTests = $false,
    [switch]$LaunchStandalone = $true
)

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "  Creative MIDI Generator - Full Test Cycle" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host ""

# Set working directory to script location
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptDir

# Track test results
$testResults = @()

function Write-Step {
    param([string]$Message)
    Write-Host "[$((Get-Date).ToString('HH:mm:ss'))] $Message" -ForegroundColor Yellow
}

function Write-Success {
    param([string]$Message)
    Write-Host "[$((Get-Date).ToString('HH:mm:ss'))] [OK] $Message" -ForegroundColor Green
    $script:testResults += @{Step = $Message; Result = "PASSED"; Timestamp = Get-Date}
}

function Write-Error {
    param([string]$Message)
    Write-Host "[$((Get-Date).ToString('HH:mm:ss'))] [FAIL] $Message" -ForegroundColor Red
    $script:testResults += @{Step = $Message; Result = "FAILED"; Timestamp = Get-Date}
}

function Write-Info {
    param([string]$Message)
    Write-Host "[$((Get-Date).ToString('HH:mm:ss'))] [INFO] $Message" -ForegroundColor Gray
}

# Step 1: Environment Check
Write-Step "Step 1: Environment Check"
try {
    & ".\manual_test.ps1"
    if ($LASTEXITCODE -eq 0 -or $LASTEXITCODE -eq $null) {
        Write-Success "Environment check passed"
    } else {
        Write-Error "Environment check failed"
        exit 1
    }
} catch {
    Write-Error "Environment check script not found or failed to execute"
    exit 1
}

# Step 2: Build/Compilation Check
Write-Step "Step 2: Build/Compilation Check"

# Check if MSBuild is available for automated compilation
$possiblePaths = @(
    "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe",
    "C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe",
    "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe",
    "C:\Program Files (x86)\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe",
    "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe",
    "C:\Program Files (x86)\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe"
)

$msbuildPath = $null
foreach ($path in $possiblePaths) {
    if (Test-Path $path) {
        $msbuildPath = $path
        break
    }
}

if ($msbuildPath -and (Test-Path $msbuildPath)) {
    Write-Info "MSBuild found, attempting automated compilation..."

    # Look for Visual Studio solution file
    $solutionPath = "..\juce_project\Builds\VisualStudio2022\CreativeMIDIGenerator.sln"
    if (Test-Path $solutionPath) {
        Write-Step "Compiling project with MSBuild..."
        try {
            & $msbuildPath $solutionPath "/p:Configuration=Release" "/p:Platform=x64" "/t:Build" "/verbosity:minimal"
            if ($LASTEXITCODE -eq 0) {
                Write-Success "Project compiled successfully"
            } else {
                Write-Error "Compilation failed with MSBuild"
                Write-Info "Please check Visual Studio for compilation errors"
                exit 1
            }
        } catch {
            Write-Error "MSBuild execution failed: $($_.Exception.Message)"
            Write-Info "Falling back to manual compilation instructions"
        }
    } else {
        Write-Info "Solution file not found, skipping automated compilation"
    }
} else {
    Write-Info "MSBuild not found, will check existing build files"
}

# Step 3: Build Validation
if (-not $SkipBuildCheck) {
    Write-Step "Step 4: Build Validation"
    try {
        & ".\test_build.ps1"
        if ($LASTEXITCODE -eq 0 -or $LASTEXITCODE -eq $null) {
            Write-Success "Build validation passed"
        } else {
            Write-Error "Build validation failed - please rebuild the project first"
            Write-Info "Manual compilation steps:"
            Write-Info "1. Open CreativeMIDIGenerator.jucer in Projucer"
            Write-Info "2. Select Visual Studio 2022 exporter"
            Write-Info "3. Click File -> Save and Open in IDE"
            Write-Info "4. In Visual Studio: Build -> Build Solution"
            exit 1
        }
    } catch {
        Write-Error "Build validation script not found or failed to execute"
        exit 1
    }
} else {
    Write-Info "Skipping build validation (SkipBuildCheck = true)"
}

# Step 4: Headless Tests
if (-not $SkipHeadlessTests) {
    Write-Step "Step 5: Running Headless Tests"
    try {
        & ".\run_headless_tests.ps1"
        if ($LASTEXITCODE -eq 0) {
            Write-Success "Headless tests completed"
        } else {
            Write-Error "Some headless tests failed"
            # Continue execution even if tests fail
        }
    } catch {
        Write-Error "Headless tests script not found or failed to execute"
        # Continue execution
    }
} else {
    Write-Info "Skipping headless tests (SkipHeadlessTests = true)"
}

# Step 5: Find and Launch Standalone Application
if ($LaunchStandalone) {
    Write-Step "Step 6: Launching Standalone Application"

    # Look for standalone executable in various locations
    $standalonePaths = @(
        "..\juce_project\Builds\VisualStudio2022\x64\Release\Standalone Plugin\Creative MIDI Generator.exe",
        "..\juce_project\Builds\VisualStudio2022\x64\Debug\Standalone Plugin\Creative MIDI Generator.exe",
        "..\build\Creative MIDI Generator.exe"
    )

    $standaloneFound = $false
    foreach ($path in $standalonePaths) {
        if (Test-Path $path) {
            Write-Info "Found standalone executable: $path"

            # Launch the standalone application
            try {
                Write-Step "Launching standalone application..."
                Start-Process $path

                Write-Success "Standalone application launched successfully"
                Write-Info "Please test the plugin manually in the standalone window"
                Write-Info "Close the standalone window when done testing"

                $standaloneFound = $true
                break
            } catch {
                Write-Error "Failed to launch standalone application: $($_.Exception.Message)"
            }
        }
    }

    if (-not $standaloneFound) {
        Write-Error "Standalone executable not found in any expected location"
        Write-Info "Expected locations:"
        foreach ($path in $standalonePaths) {
            Write-Info "  - $path"
        }
        Write-Info "To enable Standalone Plugin format:"
        Write-Info "1. Open CreativeMIDIGenerator.jucer in Projucer"
        Write-Info "2. Go to File -> Global Preferences -> Paths"
        Write-Info "3. Enable 'buildStandalone' format in exporters"
        Write-Info "4. Rebuild the project in Visual Studio"
        Write-Info "Note: For now, you can test the VST3 plugin in your DAW"
    }
} else {
    Write-Info "Skipping standalone launch (LaunchStandalone = false)"
}

# Step 6: Generate Test Report
Write-Step "Step 7: Generating Test Report"
Write-Host ""
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "         TEST CYCLE SUMMARY" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan

$passedCount = 0
$totalCount = $testResults.Count

foreach ($result in $testResults) {
    $status = if ($result.Result -eq "PASSED") { "[OK]" } else { "[FAIL]" }
    $color = if ($result.Result -eq "PASSED") { "Green" } else { "Red" }
    Write-Host "$status $($result.Step)" -ForegroundColor $color

    if ($result.Result -eq "PASSED") {
        $passedCount++
    }
}

Write-Host ""
Write-Host "Results: $passedCount/$totalCount tests passed" -ForegroundColor $(if ($passedCount -eq $totalCount) { "Green" } else { "Yellow" })

if ($passedCount -eq $totalCount) {
    Write-Host "SUCCESS: All tests passed! Plugin is ready for use." -ForegroundColor Green
} else {
    Write-Host "WARNING: Some tests failed. Check the output above for details." -ForegroundColor Yellow
}

Write-Host ""
Write-Host "=========================================" -ForegroundColor Cyan

# Instructions for next steps
Write-Host "Next Steps:" -ForegroundColor Cyan
Write-Host "1. Test the plugin in your favorite DAW" -ForegroundColor White
Write-Host "2. Copy .vst3 file to DAW plugins folder" -ForegroundColor White
Write-Host "3. Experiment with different parameters" -ForegroundColor White
Write-Host "4. Report any issues or feedback" -ForegroundColor White

Write-Host ""
Write-Host "Happy music making!" -ForegroundColor Magenta