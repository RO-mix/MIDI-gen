# Ручное тестирование сборки Creative MIDI Generator

Write-Host "=== Manual Build Test for Creative MIDI Generator ===" -ForegroundColor Green
Write-Host "This script will help you test the build process step by step" -ForegroundColor Cyan
Write-Host ""

# Step 1: Check if JUCE exists
$jucePath = "D:\PROG\JUCE"
if (Test-Path $jucePath) {
    Write-Host "✅ JUCE found at: $jucePath" -ForegroundColor Green

    $projucerPath = Join-Path $jucePath "Projucer.exe"
    if (Test-Path $projucerPath) {
        Write-Host "✅ Projucer found: $projucerPath" -ForegroundColor Green
    } else {
        Write-Host "❌ Projucer not found" -ForegroundColor Red
    }
} else {
    Write-Host "❌ JUCE not found at: $jucePath" -ForegroundColor Red
    Write-Host "Please install JUCE to D:\PROG\JUCE" -ForegroundColor Yellow
}

Write-Host ""

# Step 2: Check project files
$projectPath = "..\juce_project\CreativeMIDIGenerator.jucer"
if (Test-Path $projectPath) {
    Write-Host "✅ Project file found: $projectPath" -ForegroundColor Green
} else {
    Write-Host "❌ Project file not found: $projectPath" -ForegroundColor Red
}

Write-Host ""

# Step 3: Check source files
$sourceFiles = @(
    "..\juce_project\Source\PluginProcessor.h",
    "..\juce_project\Source\PluginProcessor.cpp",
    "..\juce_project\Source\Generators\BaseGenerator.h",
    "..\juce_project\Source\Generators\RandomGenerator.h",
    "..\juce_project\Source\Generators\RandomGenerator.cpp",
    "..\juce_project\Source\Theory\Scales.h",
    "..\juce_project\Source\Theory\Scales.cpp"
)

Write-Host "Source files check:" -ForegroundColor Cyan
foreach ($file in $sourceFiles) {
    if (Test-Path $file) {
        Write-Host "  ✅ $file" -ForegroundColor Green
    } else {
        Write-Host "  ❌ $file" -ForegroundColor Red
    }
}

Write-Host ""

# Step 4: Instructions for user
Write-Host "=== Next Steps ===" -ForegroundColor Green
Write-Host ""
Write-Host "1. Open Projucer:" -ForegroundColor Cyan
Write-Host "   & '$projucerPath' '$projectPath'" -ForegroundColor White
Write-Host ""
Write-Host "2. In Projucer:" -ForegroundColor Cyan
Write-Host "   - Select Visual Studio 2022 exporter" -ForegroundColor White
Write-Host "   - Click: File → Save and Open in IDE" -ForegroundColor White
Write-Host ""
Write-Host "3. In Visual Studio:" -ForegroundColor Cyan
Write-Host "   - Select: Release | x64" -ForegroundColor White
Write-Host "   - Build: Build → Build Solution" -ForegroundColor White
Write-Host ""
Write-Host "4. Check results:" -ForegroundColor Cyan
Write-Host "   - Look for .dll file in Builds\VisualStudio2022\x64\Release\" -ForegroundColor White
Write-Host ""
Write-Host "5. Test in DAW:" -ForegroundColor Cyan
Write-Host "   - Copy .dll to DAW plugins folder" -ForegroundColor White
Write-Host "   - Restart DAW and look for 'CreativeMIDIGenerator'" -ForegroundColor White

Write-Host ""
Write-Host "=== Common Issues & Solutions ===" -ForegroundColor Yellow
Write-Host ""
Write-Host "Issue: 'Cannot open file Creative MIDI Generator.lib'" -ForegroundColor Red
Write-Host "Solution: The project name was changed to 'CreativeMIDIGenerator'" -ForegroundColor White
Write-Host ""
Write-Host "Issue: 'compiler is out of heap space'" -ForegroundColor Red
Write-Host "Solution: Compiler flags /Zm500 are already set" -ForegroundColor White
Write-Host ""
Write-Host "Issue: 'You may have a conflict with parameter automation'" -ForegroundColor Red
Write-Host "Solution: Removed buildStandalone format" -ForegroundColor White

Write-Host ""
Write-Host "Good luck with the build! 🚀" -ForegroundColor Green