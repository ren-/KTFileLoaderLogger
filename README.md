# KTFileLoaderLogger

Logs every resource the KTGL engine mounts, with its hash and type, for Koei
Tecmo / Team Ninja games. One `ktfl.asi` per game, loaded through an ASI loader.

Fork of [MrIkso/Nioh3FileLoaderLogger](https://github.com/MrIkso/Nioh3FileLoaderLogger)
(MIT), generalised to per-game signature profiles.

## Supported games

| Game | Exe | Verified on |
|---|---|---|
| Nioh 3 | `Nioh3.exe` | 2026-09-17 |
| Wo Long: Fallen Dynasty | `WoLong.exe` | 2026-09-22 |
| Wo Long 2 | `WoLong2.exe` | Wings of Ember alpha demo |
| Rise of the Ronin | `Ronin.exe` | 2026-08-09 |

A game update can move the hooked functions. If the log says the signatures
were not found, the profile needs refreshing; see "Adding a game".

## Install

1. Install an ASI loader such as [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader).
   It runs plugins after SteamStub has decrypted the exe, which the signature scan needs.
   If the game folder already has a `dinput8.dll`, name the loader after another
   DLL the game imports instead, for example `winmm.dll` or `xinput1_4.dll`.
2. Download the zip for your game from Releases.
3. Unzip `ktfl.asi` into the game folder (or the loader's `plugins` folder).

The plugin opens a console and writes `ktfl_log.txt` next to itself. It stays
passive if the process name does not match the game it was built for.

## Output

```
[VFS] Hash: 0x4885EA48 | Type: ModelData (.g1m)
```

Same format as the original tool. "Hash" is the 32-bit value the engine's own
error log prints as `Hash[0x%08x]` for the resource.

## Building

```
build.bat            builds every profile to dist\<game>\ktfl.asi
build.bat nioh3      builds one
release.ps1          zips dist\* into release\
```

Needs `cl` or `clang-cl` and Python 3. Nothing is read at runtime; each
profile is turned into a header by `tools/gen_profile.py` and compiled in.

## Profiles

One `profiles/<game>.ini` per game:

```ini
[settings]
console = true          ; open a console and mirror the log there
dedupe = false          ; log each (hash, type) pair once
log = ktfl_log.txt

[nioh3]
exe = Nioh3.exe
game = Nioh 3
build = 2026-09-17
sig_mount = 40 55 56 57 ...   ; function called once per resource (the hook)
sig_index = 40 53 48 83 ...   ; helper that maps an entry to its hash
mgr_arg = 3             ; 1-based argument holding the resource manager
entry_arg = 7           ; 1-based argument holding the resource entry
pool_off = 0x218        ; manager + pool_off is passed to the helper
type_off = 8            ; entry + type_off = u32 type ktid
index_args = pool,entry ; helper argument order: pool,entry or entry,pool
dump = 0                ; log the first N entries as hex
```

## Adding a game

1. Get an unpacked exe (SteamStub removed) and find the per-resource function
   and the hash helper. The string `can't create resource Name[%s] Hash[0x%08x]`
   is referenced from the hook function in most builds.
2. Write a profile with signatures that match exactly once:

   ```
   python tools\check_profiles.py profiles\game.ini game.unpacked.exe
   ```

3. Build, run, and check the log. If types show as Unknown, set `dump` to a
   few entries and look for a known type ktid in the hex to find `type_off`.

## Layout

- `src/` plugin source
- `profiles/` per-game ini files
- `tools/` profile checker and header generator
- `third_party/minhook/` vendored MinHook (BSD-2)

## License

MIT. See `LICENSE`.
