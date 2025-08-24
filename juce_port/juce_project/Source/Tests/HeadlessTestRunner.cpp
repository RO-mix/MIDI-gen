#include "HeadlessTester.h"
#include <JuceHeader.h>
#include <iostream>

int main(int argc, char* argv[])
{
    // No JUCE initialiser - attempt a truly headless build without a message manager.

    std::cout << "🎵 Creative MIDI Generator - Headless Test Runner" << std::endl;
    std::cout << "==================================================" << std::endl;

    HeadlessTester tester;
    juce::StringArray results = tester.runAllTests();
    tester.printTestResults(results);

    int passedCount = 0;
    for (const auto& result : results)
    {
        if (result.contains("PASSED"))
            passedCount++;
    }

    return (passedCount == results.size()) ? 0 : 1;
}
