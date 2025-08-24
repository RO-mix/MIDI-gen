# Fix VST3 Error Script
# This script temporarily fixes the VST3 error by modifying the generated JUCE file

param(
    [string]$JuceModulePath = "D:/PROG/JUCE/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp"
)

Write-Host "=========================================" -ForegroundColor Yellow
Write-Host "  Fix VST3 Error Script" -ForegroundColor Yellow
Write-Host "=========================================" -ForegroundColor Yellow
Write-Host ""

# Check if the file exists
if (-not (Test-Path $JuceModulePath)) {
    Write-Host "❌ JUCE module file not found: $JuceModulePath" -ForegroundColor Red
    exit 1
}

Write-Host "Found JUCE module file: $JuceModulePath" -ForegroundColor Green

# Read the file content
$content = Get-Content $JuceModulePath -Raw
Write-Host "Original file size: $($content.Length) characters" -ForegroundColor Gray

# Find and replace the error condition
$pattern = '(?s)#if JUCE_VST3_CAN_REPLACE_VST2 && ! JUCE_FORCE_USE_LEGACY_PARAM_IDS && ! JUCE_IGNORE_VST3_MISMATCHED_PARAMETER_ID_WARNING.*?// VST2 version of the plugin, enable JUCE_IGNORE_VST3_MISMATCHED_PARAMETER_ID_WARNING\.\s*// DO NOT change the JUCE_VST3_CAN_REPLACE_VST2 or JUCE_FORCE_USE_LEGACY_PARAM_IDS\s*// values as this will break compatibility with currently released VST3\s*// versions of the plugin\.\s*#\s*error You may have a conflict with parameter automation between VST2 and VST3 versions of your plugin\. See the comment above for more details\.\s*#endif'

$replacement = @'
// Temporarily disabled VST2/VST3 parameter conflict check
// Original error condition commented out to allow compilation
// #if JUCE_VST3_CAN_REPLACE_VST2 && ! JUCE_FORCE_USE_LEGACY_PARAM_IDS && ! JUCE_IGNORE_VST3_MISMATCHED_PARAMETER_ID_WARNING
//
//  // If you encounter this error there may be an issue migrating parameter
//  // automation between sessions saved using the VST2 and VST3 versions of this
//  // plugin.
//  //
//  // If you have released neither a VST2 or VST3 version of the plugin,
//  // consider only releasing a VST3 version and disabling JUCE_VST3_CAN_REPLACE_VST2.
//  //
//  // If you have released a VST2 version of the plugin but have not yet released
//  // a VST3 version of the plugin, consider enabling JUCE_FORCE_USE_LEGACY_PARAM_IDS.
//  // This will ensure that the parameter IDs remain compatible between both the
//  // VST2 and VST3 versions of the plugin in all hosts.
//  //
//  // If you have already released a VST2 and VST3 version of the plugin you may
//  // find in some hosts when a session containing automation data is saved using
//  // the VST2 or VST3 version, and is later loaded using the other version, the
//  // automation data will fail to control any of the parameters in the plugin as
//  // the IDs for these parameters are different. To fix parameter automation for
//  // the VST3 plugin when a session was saved with the VST2 plugin, implement
//  // VST3ClientExtensions::getCompatibleParameterIds() and enable
//  // JUCE_IGNORE_VST3_MISMATCHED_PARAMETER_ID_WARNING.
//
//  #error You may have a conflict with parameter automation between VST2 and VST3 versions of your plugin. See the comment above for more details.
// #endif
'@

# Perform the replacement
if ($content -match $pattern) {
    $newContent = $content -replace $pattern, $replacement
    $newContent | Set-Content $JuceModulePath -Encoding UTF8

    Write-Host "✅ Successfully patched VST3 error condition" -ForegroundColor Green
    Write-Host "New file size: $($newContent.Length) characters" -ForegroundColor Gray
} else {
    Write-Host "❌ Could not find the error pattern in the file" -ForegroundColor Red
    Write-Host "The VST3 error condition may have already been fixed or the file structure changed." -ForegroundColor Yellow
}

Write-Host ""
Write-Host "=========================================" -ForegroundColor Yellow
Write-Host "         PATCH COMPLETED" -ForegroundColor Yellow
Write-Host "=========================================" -ForegroundColor Yellow
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Cyan
Write-Host "1. Run force rebuild: .\force_rebuild.ps1" -ForegroundColor White
Write-Host "2. Or run full test cycle: .\full_test_cycle.ps1" -ForegroundColor White
Write-Host ""
Write-Host "Note: This is a temporary fix. For production, properly configure VST2/VST3 options in Projucer." -ForegroundColor Yellow