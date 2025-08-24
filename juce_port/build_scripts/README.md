# Build Scripts for Creative MIDI Generator

This directory contains PowerShell scripts to help with building and testing the Creative MIDI Generator JUCE project.

## Available Scripts

### ⭐ `full_test_cycle.ps1` - **RECOMMENDED**
**Purpose**: Complete automated test suite that runs all tests, validates build, and launches standalone application.

**Usage**:
```powershell
cd juce_port\build_scripts
.\full_test_cycle.ps1
```

**Parameters**:
- `-SkipBuildCheck`: Skip build validation
- `-SkipHeadlessTests`: Skip headless tests
- `-LaunchStandalone`: Launch standalone app (default: true)

**What it does**:
1. ✅ Environment validation (JUCE, Projucer, source files)
2. ✅ Build validation (.vst3 files, project structure)
3. ✅ Headless tests (MIDI generation, scales, parameters)
4. ✅ Standalone application launch for manual testing
5. 📊 Complete test report with results

**Example usage**:
```powershell
# Full test cycle (recommended)
.\full_test_cycle.ps1

# Skip build check if already built
.\full_test_cycle.ps1 -SkipBuildCheck

# Only run tests without launching standalone
.\full_test_cycle.ps1 -LaunchStandalone:$false
```

### 1. `manual_test.ps1`
**Purpose**: Diagnostic script that checks your environment and provides step-by-step instructions.

**Usage**:
```powershell
cd juce_port\build_scripts
.\manual_test.ps1
```

**What it checks**:
- ✅ JUCE installation at D:\PROG\JUCE
- ✅ Projucer.exe availability
- ✅ Project file (.jucer) existence
- ✅ All required source files
- ❌ Build output directory

**Output**: Clear instructions for the next steps in your build process.

### 2. `test_build.ps1`
**Purpose**: Checks if the build was successful and validates the output files.

**Usage**:
```powershell
cd juce_port\build_scripts
.\test_build.ps1
```

**What it checks**:
- ✅ Builds directory structure
- ✅ Visual Studio 2022 project files
- ✅ Release and Debug build directories
- ✅ Generated .dll and .vst3 files
- ✅ VST3 plugin directory structure

**When to run**: After attempting to build the project in Visual Studio.

### 3. `run_headless_tests.ps1`
**Purpose**: Runs automated headless tests for core functionality.

**Usage**:
```powershell
cd juce_port\build_scripts
.\run_headless_tests.ps1
```

**Tests included**:
- MIDI generation algorithms
- Scale filtering
- Parameter validation
- Memory management

### 4. `setup_cmake.ps1`
**Purpose**: Alternative build setup using CMake (if needed).

**Usage**:
```powershell
cd juce_port\build_scripts
.\setup_cmake.ps1
```

## Quick Start Guide

### 🚀 Recommended: One-Command Testing
```powershell
cd juce_port\build_scripts
.\full_test_cycle.ps1
```

This single command will:
1. Check your environment
2. Validate the build
3. Run all tests
4. Launch standalone app for manual testing
5. Provide complete test report

### Manual Step-by-Step Testing

#### Step 1: Environment Check
```powershell
cd juce_port\build_scripts
.\manual_test.ps1
```

#### Step 2: Open in Projucer
Follow the instructions from `manual_test.ps1` to:
1. Open the .jucer file in Projucer
2. Select Visual Studio 2022 exporter
3. Click "File → Save and Open in IDE"

#### Step 3: Build in Visual Studio
1. Select "Release | x64" configuration
2. Build → Build Solution
3. Wait for compilation to complete

#### Step 4: Verify Build
```powershell
cd juce_port\build_scripts
.\test_build.ps1
```

#### Step 5: Test in DAW
1. Copy the .vst3 file to your DAW's plugins folder
2. Restart your DAW
3. Look for "CreativeMIDIGenerator" in the plugin list

## Testing Strategy

### Development Workflow
1. **Daily Development**: Use `.\full_test_cycle.ps1` for complete testing
2. **Quick Checks**: Use individual scripts for specific tests
3. **Pre-Release**: Always run full test cycle before releasing

### Test Coverage
- **Environment Tests**: JUCE installation, project structure
- **Build Tests**: Compilation success, VST3 generation
- **Unit Tests**: MIDI generation algorithms, scale filtering
- **Integration Tests**: Standalone application functionality
- **Manual Tests**: DAW integration, parameter automation

### Standalone vs. VST3 Testing
- **Standalone**: Quick testing, no DAW required, good for development
- **VST3**: Real-world testing in DAW environment
- **Both**: Recommended for comprehensive testing

### Troubleshooting Test Failures
1. **Environment Check Fails**: Verify JUCE installation at D:\PROG\JUCE
2. **Build Check Fails**: Rebuild project in Visual Studio
3. **Headless Tests Fail**: Check source code for compilation errors
4. **Standalone Won't Launch**: Ensure project built with Standalone Plugin format

## Common Issues and Solutions

### Issue: "Cannot open file Creative MIDI Generator.lib"
**Solution**: The project name was changed to "CreativeMIDIGenerator" to avoid library naming conflicts.

### Issue: "Compiler is out of heap space"
**Solution**: Compiler flags `/Zm500` are already configured in the project.

### Issue: "You may have a conflict with parameter automation"
**Solution**: The `buildStandalone` format was removed to prevent VST2/VST3 conflicts.

### Issue: Scripts not found
**Solution**: Make sure you're running from the correct directory:
```powershell
cd juce_port\build_scripts
```

## Troubleshooting

If you encounter issues:

1. **Run `manual_test.ps1`** to check your environment
2. **Verify JUCE installation** at D:\PROG\JUCE
3. **Check Visual Studio 2022** is properly installed
4. **Ensure all source files** are present in the correct directories
5. **Review build output** in Visual Studio for specific errors

## File Structure

```
juce_port/
├── juce_project/
│   ├── CreativeMIDIGenerator.jucer  # Main project file
│   ├── Source/                      # Source code
│   └── Builds/                      # Generated by Visual Studio
└── build_scripts/
    ├── manual_test.ps1             # Environment check
    ├── test_build.ps1              # Build validation
    ├── run_headless_tests.ps1      # Automated tests
    └── setup_cmake.ps1             # Alternative build setup
```

## Support

If you encounter issues not covered here:
1. Check the output of `manual_test.ps1`
2. Review the `VS2022_SETUP.md` in the parent directory
3. Consult the main `AGENTS.md` for detailed troubleshooting