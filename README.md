# ArchiveXL for macOS

A port of [ArchiveXL](https://github.com/psiberx/cp2077-archive-xl) by psiberx to the native macOS version of
Cyberpunk 2077 2.3.1 (Apple silicon, Steam). ArchiveXL is a RED4ext plugin that lets mods add and extend game
resources (appearances, garments, localization, factories, world streaming, and more) through `.xl` files, without
overwriting base game files. It runs in game on macOS; every game address it uses is verified in the RED4ext.SDK
address database.

## Install

ArchiveXL is part of the RED4ext macOS release (Cyberpunk 2077 2.3.1, Steam, Apple silicon). Quit the game, open Terminal
and run:

```bash
curl -fsSL https://raw.githubusercontent.com/jackmaxwil/RED4ext-macos/main/install.sh | bash
```

That installs RED4ext with TweakXL, ArchiveXL and ModMenu. Update, uninstall, `doctor` and `play` are in
[RED4ext's README](https://github.com/jackmaxwil/RED4ext-macos#install); the manual install is in
[INSTALL_MACOS.md](https://github.com/jackmaxwil/RED4ext-macos/blob/main/docs/INSTALL_MACOS.md). Start the game with
`launch_red4ext.sh` from the game folder; the Steam Play button starts it without mods.

It installs to `red4ext/plugins/ArchiveXL/` in the game folder:

```
red4ext/plugins/ArchiveXL/
  ArchiveXL.dylib
  Bundle/     ArchiveXL's own .xl files and ArchiveXL.archive
  Scripts/    ArchiveXL's REDscript sources
```

## Use it

Install ArchiveXL mods the same way as on Windows. ArchiveXL reads `.xl` files (YAML) from:

- the game's mod archive folder, `archive/pc/mod/`, including subfolders. A mod usually ships `MyMod.archive` and
  `MyMod.archive.xl` side by side.
- its own `red4ext/plugins/ArchiveXL/Bundle/` folder. Leave this one alone.

The `.xl` format is the same as on Windows; see the [upstream project](https://github.com/psiberx/cp2077-archive-xl).

## Known gaps on macOS

- Hot reload (`ArchiveXL.Reload()`) is disabled. It logs `ArchiveXL.Reload() is not supported on macOS.`
  Restart the game to pick up changed `.xl` files.
- At unload, static game handles are leaked on purpose (`LeakAtExit` in
  `src/App/Extensions/ExtensionBase.hpp`), because releasing them after the game has shut down crashed on exit.
- The macOS game itself does not load mod archives: it never reads `archive/pc/mod/`. ArchiveXL loads that folder
  instead (in name order, like the game does on Windows), so on macOS `.archive` mods need ArchiveXL even when they
  have no `.xl` file.
- Also off on macOS: the Transmog factory template override, native registration of the two PuppetState enums, and the
  WorldWidgetComponent limit patch, collision shape deletions in world streaming sectors (whole-actor deletions work),
  and the nails color fallback read from the character customization state (the color still comes from the nails
  component). Their addresses or layouts are not verified on macOS.

## Troubleshooting

- **Where is the log?** `red4ext/plugins/ArchiveXL/ArchiveXL-<date>-<time>.log`. RED4ext's own log is
  `red4ext/logs/red4ext-*.log`.
- **ArchiveXL does not load at all.** Read `red4ext/logs/red4ext-*.log`; it names a refused plugin and the reason.
  On any game build other than 2.3.1, RED4ext refuses to hook anything.
- **A mod's `.xl` file has no effect.** Look for `Loading "<file>.xl"...` in the ArchiveXL log. If it is missing, check
  that the file is under `archive/pc/mod/` and ends in `.xl`. If it is listed with an error, the YAML is wrong.
- **The game crashes.** Send the newest ArchiveXL log, the newest `red4ext/logs/red4ext-*.log` and the crash report from
  Console.app (Crash Reports, `Cyberpunk2077`).
- **A mod's `.archive` is not loaded.** The log lists every mod archive as `Archive "archive/pc/mod/<name>.archive"
  loaded.` Archives must sit directly in `archive/pc/mod/`, not in a subfolder.
- **Debug output.** `ARCHIVEXL_ADDR_TRACE=1` logs every address lookup, `ARCHIVEXL_HOOK_TRACE=1` every hook
  attach/detach and `ARCHIVEXL_DEPOT_TRACE=1` the game's archive groups, e.g. `ARCHIVEXL_HOOK_TRACE=1 ./launch_red4ext.sh`.

## Build from source

Needs Xcode Command Line Tools and Homebrew packages:

```bash
brew install cmake spdlog yaml-cpp
git submodule update --init --recursive
cmake -S . -B build-dev -DCMAKE_BUILD_TYPE=Release
cmake --build build-dev -j8
```

Output: `build-dev/ArchiveXL.dylib`. `tools/fetch-bundle-archive.sh` puts ArchiveXL's packed resources
(`ArchiveXL.archive`, built on Windows with WolvenKit) into `bundle/packed/`, taken from upstream's release for the same
version and checked against a pinned checksum; the release and install tools do this for you. If `../RED4ext.SDK` exists (the workspace layout used for the port), its headers
are used; otherwise the `vendor/RED4ext.SDK` submodule is. To build and install ArchiveXL together with RED4ext, use
RED4ext's `tools/cp-dev` or `scripts/create_release.sh`. The upstream `xmake.lua` is kept for Windows builds.

## macOS changes

- Hooks attach through RED4ext's native macOS hook engine (`lib/Support/macOS/MacOSHookingProvider.hpp`) instead of
  MinHook.
- Every address hash resolves through the RED4ext.SDK address database
  (`lib/Support/macOS/ArchiveXLAddressResolver.cpp`). Only verified entries resolve; anything else fails closed.
- arm64 ABI: struct results returned through x8 are declared as by-value returns, and virtual function offsets shift
  by 8 for the Itanium destructor pair.
- Functions the macOS game inlines are hooked at their callers or rebuilt in code (garment ChangeItem, TPP slot checks,
  GetSuffixValue, GetHairColor, mappin getters, and others).
- Writes to classes whose layout differs on macOS are checked against the game's RTTI offsets and skipped on a
  mismatch.
- The game root is found three levels above the executable (inside `Cyberpunk2077.app`).
- Built with CMake (`CMakeLists.txt`); macOS code is behind `#ifdef __APPLE__` so the fork stays rebaseable on upstream.

## Credits

ArchiveXL is by [psiberx](https://github.com/psiberx), MIT license (see `LICENSE` and `THIRD_PARTY_LICENSES`). The
macOS port keeps the upstream license.
