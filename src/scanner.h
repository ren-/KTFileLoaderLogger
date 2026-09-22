// AOB scan over a module's .text. SteamStub decrypts .text late and commits
// pages as it goes, so only committed readable pages are read; callers poll.
#pragma once
#include <cstddef>
#include <cstdint>

namespace sig {

// One signature: space-separated hex bytes, "?" or "??" for a wildcard.
// Returns the single match, or null if the pattern is absent OR ambiguous.
// Two matches means the build moved and guessing is worse than doing nothing.
// *count receives the number of matches seen (0, 1, or 2 = "more than one").
const uint8_t* ScanUnique(const uint8_t* text, size_t size, const char* pattern, int* count);

// Locate the .text section of a loaded PE image.
bool GetTextSection(uintptr_t base, const uint8_t** out_ptr, size_t* out_size);

} // namespace sig
