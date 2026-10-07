# ArchiveXL macOS port: agent notes

Fork of psiberx/cp2077-archive-xl (MIT), ported to Cyberpunk 2077 2.3.1 on macOS arm64. Branch `main`.
It runs in game as a RED4ext plugin. Read `README.md` first.

## Rules

- macOS first: everything must build and run on macOS arm64. Put macOS code behind `#ifdef __APPLE__` and keep the
  Windows path as upstream wrote it.
- Stay rebaseable on upstream: keep upstream files (`xmake.lua`, `support/`, `config/`, `tools/dist/`) and keep diffs
  small. Do not reformat or restructure upstream code.
- Verified addresses only. Every hash in `src/Red/Addresses/Library.hpp` resolves through
  `lib/Support/macOS/ArchiveXLAddressResolver.cpp` to the RED4ext.SDK address DB, and only entries marked verified
  resolve. Add or change addresses in RED4ext.SDK with evidence (its `docs/ADDRESS_AUDIT.md` and `docs/re/`), never
  here. If an address or layout cannot be verified, disable the feature on macOS (fail closed) and log it.
- Native hooks only. Hooks go through `MacOSHookingProvider` to RED4ext's native hook engine.
- Hook callbacks must not throw. Keep `.OrThrow()` on required hooks so missing addresses show up at startup.
- Do not launch the game or Steam from tooling.

## macOS ABI notes

- Struct results returned through x8 are declared as by-value returns of non-trivially-copyable types.
- `Core::RawVFunc` offsets stay MSVC offsets in the source; on macOS they shift by 8 (Itanium destructor pair).
- Some functions are inlined on macOS; hook their callers or rebuild them in code.
- Check writes to classes whose macOS layout differs against the game's RTTI offsets; skip on mismatch.
- A string reference is not proof of a function's identity: name-like strings often lead to CName registration code,
  and assert paths lead to trap stubs.

## Build and check

```bash
cmake -S . -B build-dev && cmake --build build-dev -j8
python3 ../RED4ext.SDK/scripts/plugin_requirements.py build-dev/ArchiveXL.dylib   # must report 0 unverified
```

RED4ext's `tools/cp-dev` (in the sibling RED4ext repo) builds, signs and installs ArchiveXL with its `scripts/` and
`bundle/source/resources` (plus `bundle/packed/archive/pc/mod` when present). Keep those paths.
