# ArchiveXL (macOS Port)

> **⚠️ macOS-only fork/port.** For Windows, use the original: [psiberx/cp2077-archive-xl](https://github.com/psiberx/cp2077-archive-xl)

ArchiveXL is a modding tool that allows you to load custom resources without touching original game files,
thus allowing multiple mods to expand same resources without conflicts.

With the mod you can:

- Load custom entity factories (necessary for item additions)
- Add localization texts that can be used in scripts, resources and TweakDB
- Edit existing localization texts without overwriting original resources
- Override submeshes visibility of entity parts
- Add visual tags to a clothing item
- Spawn widgets from any library without registering dependencies

## Getting Started

### Compatibility

- Cyberpunk 2077 **macOS v2.3.1**
- Apple Silicon (arm64)
- [RED4ext macOS port](https://github.com/memaxo/RED4ext) installed and working
- [redscript](https://github.com/jac3km4/redscript) 0.5.31+

### Installation

1. Install requirements:
   - [RED4ext](https://docs.red4ext.com/getting-started/installing-red4ext) 1.29.0+
2. Extract the release archive `ArchiveXL-x.x.x.zip` into the Cyberpunk 2077 directory.

## Documentation

- **macOS port docs**
  - `docs/STATUS.md` (current status)
  - `docs/MACOS_ADDRESS_DISCOVERY.md` (address discovery pipeline + current gaps)
  - `docs/MACOS_PORTING_AUDIT.md` (original porting feasibility/audit)

- [Dynamic appearances](https://github.com/psiberx/cp2077-archive-xl/wiki#dynamic-appearances)
- [Body types](https://github.com/psiberx/cp2077-archive-xl/wiki#body-types)
- [Appearance suffixes](https://github.com/psiberx/cp2077-archive-xl/wiki#appearance-suffixes)
- [Components overrides](https://github.com/psiberx/cp2077-archive-xl/wiki#components-overrides)
- [Visual tags](https://github.com/psiberx/cp2077-archive-xl/wiki#visual-tags)
- [Extending resources](https://github.com/psiberx/cp2077-archive-xl/wiki#extending-resources)
