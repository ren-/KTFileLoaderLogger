#include "scanner.h"

#include <windows.h>
#include <cstring>

namespace sig {
namespace {

constexpr size_t kMaxPattern = 96;

struct Pattern {
    uint8_t bytes[kMaxPattern];
    bool    wild[kMaxPattern];
    size_t  len;
};

int HexVal(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

bool Parse(const char* p, Pattern* out) {
    out->len = 0;
    while (*p) {
        while (*p == ' ' || *p == '\t') ++p;
        if (!*p) break;
        if (out->len >= kMaxPattern) return false;
        if (p[0] == '?') {
            out->wild[out->len] = true;
            out->bytes[out->len] = 0;
            ++out->len;
            ++p;
            if (*p == '?') ++p;
            continue;
        }
        int hi = HexVal(p[0]);
        int lo = HexVal(p[1]);
        if (hi < 0 || lo < 0) return false;
        out->wild[out->len] = false;
        out->bytes[out->len] = (uint8_t)((hi << 4) | lo);
        ++out->len;
        p += 2;
    }
    return out->len > 0;
}

bool Readable(const MEMORY_BASIC_INFORMATION& mbi) {
    return mbi.State == MEM_COMMIT &&
        (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY |
                        PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE |
                        PAGE_EXECUTE_WRITECOPY)) != 0 &&
        (mbi.Protect & PAGE_GUARD) == 0;
}

// Scan one committed run. Keeps the first match and counts a second one.
void ScanRun(const uint8_t* text, size_t size, const Pattern& pat, size_t anchor,
             const uint8_t** found, int* count) {
    if (size < pat.len) return;
    const uint8_t want = pat.bytes[anchor];
    const size_t last = size - pat.len;
    for (size_t i = 0; i <= last; ++i) {
        if (text[i + anchor] != want) continue;
        size_t k = 0;
        for (; k < pat.len; ++k) {
            if (pat.wild[k]) continue;
            if (text[i + k] != pat.bytes[k]) break;
        }
        if (k != pat.len) continue;
        if (*found) { *count = 2; return; }
        *found = text + i;
        *count = 1;
    }
}

} // namespace

const uint8_t* ScanUnique(const uint8_t* text, size_t size, const char* pattern, int* count) {
    int local = 0;
    if (!count) count = &local;
    *count = 0;
    Pattern pat;
    if (!Parse(pattern, &pat) || size < pat.len) return nullptr;
    size_t anchor = 0;
    while (anchor < pat.len && pat.wild[anchor]) ++anchor;
    if (anchor == pat.len) return nullptr;

    const uint8_t* found = nullptr;
    const uint8_t* p   = text;
    const uint8_t* end = text + size;
    while (p < end && *count < 2) {
        MEMORY_BASIC_INFORMATION mbi;
        if (!VirtualQuery(p, &mbi, sizeof mbi)) break;
        const uint8_t* rgnEnd = (const uint8_t*)mbi.BaseAddress + mbi.RegionSize;
        if (rgnEnd > end) rgnEnd = end;
        if (rgnEnd <= p) break;
        if (Readable(mbi)) {
            const size_t kChunk = 4u << 20;
            const uint8_t* q = p;
            while (q < rgnEnd && *count < 2) {
                size_t take = (size_t)(rgnEnd - q);
                if (take > kChunk) take = kChunk;
                ScanRun(q, take, pat, anchor, &found, count);
                if (take <= pat.len) break;
                q += take - (pat.len - 1);
            }
        }
        p = rgnEnd;
    }
    return *count == 1 ? found : nullptr;
}

bool GetTextSection(uintptr_t base, const uint8_t** out_ptr, size_t* out_size) {
    auto* dos = (IMAGE_DOS_HEADER*)base;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return false;
    auto* nt = (IMAGE_NT_HEADERS64*)(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return false;
    auto* sec = IMAGE_FIRST_SECTION(nt);
    for (int i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
        if (memcmp(sec[i].Name, ".text", 6) == 0) {
            *out_ptr  = (const uint8_t*)(base + sec[i].VirtualAddress);
            *out_size = sec[i].Misc.VirtualSize;
            return true;
        }
    }
    return false;
}

} // namespace sig
