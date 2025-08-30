#!/bin/bash

# Simple headless test runner for Creative MIDI Generator (Bash version)

# --- Configuration ---
# Assuming JUCE is in the root directory alongside the project port
JUCE_PATH_DEFAULT="$(pwd)/JUCE"
# Use environment variable if set, otherwise use the default
JUCE_PATH="${JUCE_PATH:-$JUCE_PATH_DEFAULT}"

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_PATH="$SCRIPT_DIR/../juce_project"
BUILD_PATH="$SCRIPT_DIR/../build"

# --- Colors for output ---
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
RED='\033[0;31m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

echo -e "${GREEN}Creative MIDI Generator - Headless Test Runner (Linux)${NC}"
echo -e "${GREEN}====================================================${NC}"

# --- Check dependencies ---
if ! command -v cmake &> /dev/null
then
    echo -e "${RED}CMake not found. Please install CMake.${NC}"
    exit 1
fi
echo -e "${GREEN}CMake found${NC}"

if [ ! -d "$JUCE_PATH" ]; then
    echo -e "${RED}JUCE not found at: $JUCE_PATH${NC}"
    exit 1
fi
echo -e "${GREEN}JUCE found: $JUCE_PATH${NC}"

# --- Create or clean build directory ---
if [ -d "$BUILD_PATH" ]; then
    echo -e "${YELLOW}Attempting to clean existing build directory: $BUILD_PATH${NC}"
    rm -rf "$BUILD_PATH"
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}Cleaned existing build directory.${NC}"
    else
        echo -e "${RED}Warning: Could not fully clean build directory.${NC}"
    fi
fi

mkdir -p "$BUILD_PATH"
if [ $? -ne 0 ]; then
    echo -e "${RED}Failed to create build directory: $BUILD_PATH${NC}"
    exit 1
fi
echo -e "${GREEN}Created fresh build directory: $BUILD_PATH${NC}"

# --- Configure CMake ---
echo -e "${YELLOW}Configuring CMake...${NC}"
cmake -S "$PROJECT_PATH" -B "$BUILD_PATH" -G "Unix Makefiles" -DJUCE_PATH="$JUCE_PATH"
if [ $? -ne 0 ]; then
    echo -e "${RED}CMake configuration failed${NC}"
    exit 1
fi
echo -e "${GREEN}CMake configured successfully${NC}"

# --- Build HeadlessTestRunner ---
echo -e "${YELLOW}Building headless test runner...${NC}"
cmake --build "$BUILD_PATH" --target HeadlessTestRunner --config Release
if [ $? -ne 0 ]; then
    echo -e "${RED}Build failed for HeadlessTestRunner${NC}"
    exit 1
fi
echo -e "${GREEN}HeadlessTestRunner built successfully${NC}"

# --- Find and Run HeadlessTestRunner ---
TEST_EXE_PATH=$(find "$BUILD_PATH" -name "HeadlessTestRunner" -type f -executable)
if [ -z "$TEST_EXE_PATH" ]; then
    echo -e "${RED}HeadlessTestRunner executable not found${NC}"
    exit 1
fi

echo -e "${CYAN}Found test runner: $TEST_EXE_PATH${NC}"
echo -e "${YELLOW}Now running headless tests...${NC}"
"$TEST_EXE_PATH"
EXIT_CODE=$?

echo -e "${YELLOW}Tests finished with exit code: $EXIT_CODE${NC}"
if [ $EXIT_CODE -ne 0 ]; then
    echo -e "${RED}Some tests failed. Halting script.${NC}"
    exit $EXIT_CODE
fi

echo -e "${GREEN}All tests passed successfully!${NC}"

# --- Build all plugin formats (VST3, Standalone, etc.) ---
echo -e "${YELLOW}Building all plugin formats...${NC}"
cmake --build "$BUILD_PATH" --target CreativeMIDIGenerator --config Release
if [ $? -ne 0 ]; then
    echo -e "${RED}Build failed for plugin formats${NC}"
    exit 1
fi
echo -e "${GREEN}Plugin formats built successfully${NC}"

# --- Verify Standalone executable exists ---
# The find command is still useful to confirm the build produced something.
APP_EXE_PATH=$(find "$BUILD_PATH" -name "CreativeMIDIGenerator" -type f -executable)
if [ -z "$APP_EXE_PATH" ]; then
    echo -e "${RED}Standalone executable not found after build. This might indicate a problem.${NC}"
    exit 1
fi

echo -e "${CYAN}Found Standalone executable: $APP_EXE_PATH${NC}"
# We can also check for the VST3
VST3_PATH=$(find "$BUILD_PATH" -name "CreativeMIDIGenerator.vst3" -type d)
if [ -z "$VST3_PATH" ]; then
    echo -e "${YELLOW}Warning: VST3 plugin not found after build.${NC}"
else
    echo -e "${CYAN}Found VST3 plugin: $VST3_PATH${NC}"
fi
echo -e "${GREEN}--- SCRIPT FINISHED ---${NC}"
# Note: The original script tried to launch the GUI app.
# This is not practical in a headless environment, so we'll just confirm it builds.
