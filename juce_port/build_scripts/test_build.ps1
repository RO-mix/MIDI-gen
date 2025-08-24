# Test Build Script for Creative MIDI Generator
# This script checks if the build was successful and runs basic tests

Write-Host "=== Creative MIDI Generator Build Test ===" -ForegroundColor Green
Write-Host "Checking build output and running tests..." -ForegroundColor Cyan
Write-Host ""

# Check if Builds directory exists
$buildsPath = "..\juce_project\Builds"
if (Test-Path $buildsPath) {
    Write-Host "✅ Builds directory found" -ForegroundColor Green

    # Check for Visual Studio 2022 builds
    $vs2022Path = "$buildsPath\VisualStudio2022"
    if (Test-Path $vs2022Path) {
        Write-Host "✅ Visual Studio 2022 project found" -ForegroundColor Green

        # Check for Release build
        $releasePath = "$vs2022Path\x64\Release"
        if (Test-Path $releasePath) {
            Write-Host "✅ Release build directory found" -ForegroundColor Green

            # Look for .dll files
            $dllFiles = Get-ChildItem -Path $releasePath -Filter "*.dll" -Recurse
            if ($dllFiles.Count -gt 0) {
                Write-Host "✅ Found $($dllFiles.Count) .dll file(s):" -ForegroundColor Green
                foreach ($dll in $dllFiles) {
                    Write-Host "   - $($dll.Name)" -ForegroundColor White
                }
            } else {
                Write-Host "❌ No .dll files found in Release directory" -ForegroundColor Red
            }

            # Look for .exe files (for standalone)
            $exeFiles = Get-ChildItem -Path $releasePath -Filter "*.exe" -Recurse
            if ($exeFiles.Count -gt 0) {
                Write-Host "✅ Found $($exeFiles.Count) .exe file(s):" -ForegroundColor Green
                foreach ($exe in $exeFiles) {
                    Write-Host "   - $($exe.Name)" -ForegroundColor White
                }
            }

            # Look for VST3 directory
            $vst3Path = "$releasePath\VST3"
            if (Test-Path $vst3Path) {
                Write-Host "✅ VST3 directory found" -ForegroundColor Green
                $vst3Files = Get-ChildItem -Path $vst3Path -Filter "*.vst3" -Recurse
                if ($vst3Files.Count -gt 0) {
                    Write-Host "✅ Found VST3 plugin(s):" -ForegroundColor Green
                    foreach ($vst3 in $vst3Files) {
                        Write-Host "   - $($vst3.Name)" -ForegroundColor White
                    }
                }
            } else {
                Write-Host "❌ VST3 directory not found" -ForegroundColor Yellow
            }
        } else {
            Write-Host "❌ Release build directory not found" -ForegroundColor Red
        }

        # Check for Debug build
        $debugPath = "$vs2022Path\x64\Debug"
        if (Test-Path $debugPath) {
            Write-Host "✅ Debug build directory found" -ForegroundColor Green
        } else {
            Write-Host "❌ Debug build directory not found" -ForegroundColor Yellow
        }
    } else {
        Write-Host "❌ Visual Studio 2022 project not found" -ForegroundColor Red
    }
} else {
    Write-Host "❌ Builds directory not found - project needs to be exported from Projucer first" -ForegroundColor Red
}

Write-Host ""
Write-Host "=== Build Test Summary ===" -ForegroundColor Green
Write-Host "If you see ❌ errors above, check:" -ForegroundColor Cyan
Write-Host "1. Did you open the .jucer file in Projucer?" -ForegroundColor White
Write-Host "2. Did you select Visual Studio 2022 exporter?" -ForegroundColor White
Write-Host "3. Did you click 'File → Save and Open in IDE'?" -ForegroundColor White
Write-Host "4. Did the Visual Studio build complete successfully?" -ForegroundColor White
Write-Host ""
Write-Host "=== Next Steps ===" -ForegroundColor Green
Write-Host "1. If build succeeded, copy the .vst3 file to your DAW plugins folder" -ForegroundColor White
Write-Host "2. Restart your DAW and look for 'CreativeMIDIGenerator'" -ForegroundColor White
Write-Host "3. Test the plugin with different parameters" -ForegroundColor White
Write-Host ""
Write-Host "Good luck with testing! 🚀" -ForegroundColor Green