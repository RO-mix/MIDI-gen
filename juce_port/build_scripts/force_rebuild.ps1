# Force Rebuild Script for Creative MIDI Generator
# This script forces a complete rebuild of the project

param(
    [string]$JucePath = "D:\PROG\JUCE",
    [string]$ProjectPath = "$PSScriptRoot\..\juce_project",
    [string]$BuildPath = "$PSScriptRoot\..\build"
)

Write-Host "=========================================" -ForegroundColor Magenta
Write-Host "  Creative MIDI Generator - Force Rebuild" -ForegroundColor Magenta
Write-Host "=========================================" -ForegroundColor Magenta
Write-Host ""

# Function to write colored output
function Write-Step {
    param([string]$Message)
    Write-Host "[$((Get-Date).ToString('HH:mm:ss'))] $Message" -ForegroundColor Yellow
}

function Write-Success {
    param([string]$Message)
    Write-Host "[$((Get-Date).ToString('HH:mm:ss'))] [OK] $Message" -ForegroundColor Green
}

function Write-Error {
    param([string]$Message)
    Write-Host "[$((Get-Date).ToString('HH:mm:ss'))] [FAIL] $Message" -ForegroundColor Red
}

function Write-Info {
    param([string]$Message)
    Write-Host "[$((Get-Date).ToString('HH:mm:ss'))] [INFO] $Message" -ForegroundColor Gray
}

# Step 1: Clean existing build files
Write-Step "Step 1: Cleaning existing build files"

# Clean x64 directories
$cleanPaths = @(
    "$ProjectPath\Builds\VisualStudio2022\x64\Release",
    "$ProjectPath\Builds\VisualStudio2022\x64\Debug",
    "$BuildPath"
)

foreach ($path in $cleanPaths) {
    if (Test-Path $path) {
        Write-Info "Removing: $path"
        Remove-Item -Path $path -Recurse -Force -ErrorAction SilentlyContinue
    } else {
        Write-Info "Path not found (already clean): $path"
    }
}

# Step 2: Check MSBuild availability
Write-Step "Step 2: Finding MSBuild"

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
        Write-Success "MSBuild found: $path"
        break
    }
}

if (-not $msbuildPath) {
    Write-Error "MSBuild not found in any expected location"
    Write-Host ""
    Write-Host "Please install Visual Studio 2022 with MSBuild or run manual compilation:" -ForegroundColor Yellow
    Write-Host "1. Open CreativeMIDIGenerator.jucer in Projucer" -ForegroundColor White
    Write-Host "2. Select Visual Studio 2022 exporter" -ForegroundColor White
    Write-Host "3. Click File → Save and Open in IDE" -ForegroundColor White
    Write-Host "4. In Visual Studio: Build → Build Solution" -ForegroundColor White
    Write-Host ""
    exit 1
}

# Step 2.5: Resave project with Projucer to apply changes
Write-Step "Step 2.5: Resaving project with Projucer"
$projucerPath = Join-Path $PSScriptRoot "..\..\JUCE\Projucer.exe"
$jucerFilePath = Join-Path $PSScriptRoot "..\juce_project\CreativeMIDIGenerator.jucer"

if (Test-Path $projucerPath) {
    if (Test-Path $jucerFilePath) {
        Write-Info "Found Projucer. Resaving Jucer project..."
        # Using Start-Process -Wait to ensure the script waits for the GUI app to close.
        $processInfo = Start-Process -FilePath $projucerPath -ArgumentList "--resave `"$jucerFilePath`"" -Wait -NoNewWindow -PassThru
        if ($processInfo.ExitCode -eq 0) {
            Write-Success "Project resaved successfully."
        } else {
            Write-Error "Projucer failed to resave the project. Exit code: $($processInfo.ExitCode)"
            Write-Info "If Projucer seems to work but this step fails, it might be returning a non-zero exit code on success."
            Write-Info "In that case, we may need to ignore this error."
        }
    } else {
        Write-Error "Jucer file not found at $jucerFilePath"
    }
} else {
    Write-Info "Projucer.exe not found at $projucerPath. Skipping automatic resave."
}


# Step 3: Check solution file
Write-Step "Step 3: Checking solution file"
$solutionPath = "$ProjectPath\Builds\VisualStudio2022\Creative MIDI Generator.sln"

if (-not (Test-Path $solutionPath)) {
    Write-Error "Solution file not found: $solutionPath"
    Write-Host ""
    Write-Host "Please regenerate the Visual Studio project:" -ForegroundColor Yellow
    Write-Host "1. Open CreativeMIDIGenerator.jucer in Projucer" -ForegroundColor White
    Write-Host "2. Select Visual Studio 2022 exporter" -ForegroundColor White
    Write-Host "3. Click File → Save Project" -ForegroundColor White
    Write-Host ""
    exit 1
} else {
    Write-Success "Solution file found"
}

# Step 4: Force rebuild
Write-Step "Step 4: Starting force rebuild"

try {
    Write-Info "Running: Clean solution..."
    & $msbuildPath $solutionPath "/t:Clean" "/p:Configuration=Release" "/p:Platform=x64" "/verbosity:minimal"

    Write-Info "Running: Rebuild solution..."
    $rebuildResult = & $msbuildPath $solutionPath "/t:Rebuild" "/p:Configuration=Release" "/p:Platform=x64" "/verbosity:minimal" 2>&1

    if ($LASTEXITCODE -eq 0) {
        Write-Success "Project rebuilt successfully"
    } else {
        Write-Error "Rebuild failed"
        Write-Host "Build output:" -ForegroundColor Red
        Write-Host $rebuildResult -ForegroundColor Red
        exit 1
    }
} catch {
    Write-Error "MSBuild execution failed: $($_.Exception.Message)"
    exit 1
}

# Step 5: Verify build output
Write-Step "Step 5: Verifying build output"

$releasePath = "$ProjectPath\Builds\VisualStudio2022\x64\Release"
if (Test-Path $releasePath) {
    # Check for VST3
    $vst3Path = "$releasePath\VST3"
    if (Test-Path $vst3Path) {
        $vst3Files = Get-ChildItem -Path $vst3Path -Filter "*.vst3" -Recurse
        if ($vst3Files.Count -gt 0) {
            Write-Success "VST3 plugin built: $($vst3Files[0].FullName)"
        }
    }

    # Check for Standalone
    $standalonePath = "$releasePath\Standalone Plugin"
    if (Test-Path $standalonePath) {
        $exeFiles = Get-ChildItem -Path $standalonePath -Filter "*.exe" -Recurse
        if ($exeFiles.Count -gt 0) {
            Write-Success "Standalone app built: $($exeFiles[0].FullName)"
        }
    }

    Write-Info "Build completed successfully! Files are ready for testing."
} else {
    Write-Error "Build output directory not found"
    exit 1
}

Write-Host ""
Write-Host "=========================================" -ForegroundColor Magenta
Write-Host "         FORCE REBUILD COMPLETED" -ForegroundColor Magenta
Write-Host "=========================================" -ForegroundColor Magenta
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Cyan
Write-Host "1. Run full test cycle: .\full_test_cycle.ps1" -ForegroundColor White
Write-Host "2. Or run tests only: .\run_tests.ps1" -ForegroundColor White
Write-Host "3. Copy .vst3 to your DAW plugins folder" -ForegroundColor White
Write-Host ""
Write-Host "Happy testing! 🚀" -ForegroundColor Green