#include "BaseGenerator.h"
#include "../LiveNote.h"
#include <vector>

// This is the implementation of the Pimpl idiom.
// The state, which includes the potentially heavy <vector> header,
// is hidden away from the BaseGenerator.h file.
struct BaseGenerator::GeneratorState
{
    std::vector<LiveNote> recentNotes;
};

// --- Constructor ---
BaseGenerator::BaseGenerator()
    : pimpl(std::make_unique<GeneratorState>())
{
}

// --- Destructor ---
// This MUST be defined here in the .cpp file, where the definition of
// GeneratorState is known. Otherwise, the std::unique_ptr will not be able
// to call the destructor on an incomplete type.
BaseGenerator::~BaseGenerator() = default;


// --- Getter for recent notes ---
const std::vector<LiveNote>& BaseGenerator::getRecentNotes() const
{
    return pimpl->recentNotes;
}

void BaseGenerator::clearRecentNotes()
{
    pimpl->recentNotes.clear();
}

void BaseGenerator::addRecentNote(const LiveNote& note)
{
    pimpl->recentNotes.push_back(note);
}
