// KTFileLoaderLogger: logs every resource the KTGL engine mounts (hash
// + type ktid). One ktfl.asi per game; the profile is baked in at build
// time (profile_gen.h). Load via an ASI loader, which runs plugins after
// SteamStub has decrypted .text.
#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <share.h>
#include <unordered_set>
#include "MinHook.h"
#include "scanner.h"
#include "types.h"
#include "profile_gen.h"

#define KTFL_VERSION "1.1.0"
#define WIDE_(s) L##s
#define WIDE(s) WIDE_(s)

// ------------------------------------------------------------------ state ----
static WCHAR   g_dir[MAX_PATH];
static FILE*   g_log;
static CRITICAL_SECTION g_lock;
static std::unordered_set<uint64_t> g_seen;

static void Log(const char* fmt, ...) {
    char line[1024];
    va_list ap; va_start(ap, fmt); vsnprintf(line, sizeof line, fmt, ap); va_end(ap);
    EnterCriticalSection(&g_lock);
    if (g_log) { fputs(line, g_log); fputc('\n', g_log); fflush(g_log); }
    if (P_CONSOLE) { fputs(line, stdout); fputc('\n', stdout); fflush(stdout); }
    LeaveCriticalSection(&g_lock);
}

// ------------------------------------------------------------------- hook ----
using MountFn = uint64_t(__fastcall*)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                                      uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                                      uint64_t, uint64_t, uint64_t, uint64_t);
using IndexFn = int(__fastcall*)(uint64_t, uint64_t);

static MountFn g_origMount;
static IndexFn g_index;

// SEH only, no C++ objects in here.
static bool Probe(uint64_t mgr, uint64_t entry, uint32_t* idx, uint32_t* type) {
    __try {
        uint64_t pool = mgr + P_POOL_OFF;
        *idx  = (uint32_t)(P_INDEX_SWAP ? g_index(entry, pool) : g_index(pool, entry));
        *type = *(const uint32_t*)(entry + P_TYPE_OFF);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// Copies the first 64 bytes of an entry; SEH only.
static bool ReadEntry(uint64_t entry, uint8_t* out) {
    __try { memcpy(out, (const void*)entry, 64); return true; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}

static void DumpEntry(uint64_t entry) {
    static volatile LONG n;
    if (InterlockedIncrement(&n) > P_DUMP) return;
    uint8_t b[64];
    if (!ReadEntry(entry, b)) return;
    char hex[64 * 3 + 1]; char* p = hex;
    for (int i = 0; i < 64; ++i) p += sprintf(p, "%02X%s", b[i], (i % 16 == 15) ? " | " : " ");
    Log("entry %p: %s", (void*)entry, hex);
}

static void LogMount(uint32_t idx, uint32_t type) {
    if (P_DEDUPE) {
        uint64_t key = ((uint64_t)idx << 32) | type;
        EnterCriticalSection(&g_lock);
        bool fresh = g_seen.insert(key).second;
        LeaveCriticalSection(&g_lock);
        if (!fresh) return;
    }
    const TypeInfo* t = LookupType(type);
    if (t) Log("[VFS] Hash: 0x%08X | Type: %s (%s)", idx, t->name, t->ext);
    else   Log("[VFS] Hash: 0x%08X | Type: Unknown (0x%08X)", idx, type);
}

static uint64_t __fastcall HookMount(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4,
                                     uint64_t a5, uint64_t a6, uint64_t a7, uint64_t a8,
                                     uint64_t a9, uint64_t a10, uint64_t a11, uint64_t a12,
                                     uint64_t a13, uint64_t a14) {
    const uint64_t args[14] = { a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14 };
    uint64_t mgr = args[P_MGR_ARG - 1], entry = args[P_ENTRY_ARG - 1];
    uint32_t idx = 0, type = 0;
    if (mgr && entry && Probe(mgr, entry, &idx, &type) && idx != 0xFFFFFFFFu) {
        if (P_DUMP) DumpEntry(entry);
        LogMount(idx, type);
    }
    return g_origMount(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14);
}

// ----------------------------------------------------------------- worker ----
static void OpenOutputs() {
    if (P_CONSOLE) {
        AllocConsole();
        FILE* f;
        freopen_s(&f, "CONOUT$", "w", stdout);
        SetConsoleTitleA("KTFileLoaderLogger " KTFL_VERSION " " P_GAME);
    }
    WCHAR path[MAX_PATH];
    wcscpy_s(path, g_dir); wcscat_s(path, L"\\" WIDE(P_LOG));
    g_log = _wfsopen(path, L"w", _SH_DENYNO);   // readable while the game runs
}

struct Found { const uint8_t* mount; const uint8_t* index; int cm, ci; };

static Found ScanOnce(const uint8_t* text, size_t size) {
    Found f{};
    f.mount = sig::ScanUnique(text, size, P_SIG_MOUNT, &f.cm);
    f.index = sig::ScanUnique(text, size, P_SIG_INDEX, &f.ci);
    return f;
}

static DWORD WINAPI Worker(LPVOID) {
    OpenOutputs();
    Log("KTFileLoaderLogger " KTFL_VERSION " [" P_NAME "] " P_GAME " (" P_BUILD ")");

    WCHAR exe[MAX_PATH];
    GetModuleFileNameW(nullptr, exe, MAX_PATH);
    const WCHAR* base = wcsrchr(exe, L'\\'); base = base ? base + 1 : exe;
    if (_wcsicmp(base, WIDE(P_EXE)) != 0) { Log("built for " P_EXE ", staying passive"); return 0; }

    const uint8_t* text; size_t size;
    if (!sig::GetTextSection((uintptr_t)GetModuleHandleW(nullptr), &text, &size)) {
        Log("no .text section"); return 0;
    }

    // Under an ASI loader .text is already decrypted; retry briefly just in case.
    const DWORD deadline = GetTickCount() + 30000;
    Found f{};
    for (;;) {
        f = ScanOnce(text, size);
        if (f.cm >= 2 || f.ci >= 2) {
            Log("signature ambiguous (mount=%d index=%d), staying passive", f.cm, f.ci);
            return 0;
        }
        if (f.mount && f.index) break;
        if ((LONG)(GetTickCount() - deadline) >= 0) {
            Log("signatures not found (mount=%d index=%d), staying passive", f.cm, f.ci);
            return 0;
        }
        Sleep(250);
    }
    Log("mount %p index %p", f.mount, f.index);
    g_index = (IndexFn)f.index;

    MH_STATUS st = MH_Initialize();
    if (st != MH_OK) { Log("MH_Initialize: %s", MH_StatusToString(st)); return 0; }
    st = MH_CreateHook((LPVOID)f.mount, (LPVOID)HookMount, (LPVOID*)&g_origMount);
    if (st != MH_OK) { Log("MH_CreateHook: %s", MH_StatusToString(st)); return 0; }
    st = MH_EnableHook((LPVOID)f.mount);
    if (st != MH_OK) { Log("MH_EnableHook: %s", MH_StatusToString(st)); return 0; }
    Log("hooked");
    return 0;
}

// ---------------------------------------------------------------- DllMain ----
BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(inst);
        InitializeCriticalSection(&g_lock);
        GetModuleFileNameW(inst, g_dir, MAX_PATH);
        WCHAR* slash = wcsrchr(g_dir, L'\\');
        if (slash) *slash = 0;
        HANDLE t = CreateThread(nullptr, 0, Worker, nullptr, 0, nullptr);
        if (t) CloseHandle(t);
    }
    // Never unhook at unload: the game may still be inside the trampoline.
    return TRUE;
}
