# ArchiveXL macOS Port — Status

> **Last updated:** 2026-02-01  
> **Target game build:** Cyberpunk 2077 macOS **v2.3.1**  
> **Target arch:** Apple Silicon (arm64)  

## Current headline

- **Primary blocker:** macOS address mapping for ArchiveXL’s **130 custom hash IDs**.
- **Automated discovery status (latest run):** **113 / 130 resolved**, **17 unresolved**  
  (see `tools/archivexl_offsets_report_tmp.json`)

## Source-of-truth files

- **Hash list (must match 1:1)**: `src/Red/Addresses/Library.hpp` (130 `constexpr uint32_t` IDs)
- **macOS resolver table (target output)**: `lib/Support/macOS/ArchiveXLAddressResolver.cpp`
- **Discovery tool**: `tools/macos_discover_archivexl_offsets.py`
- **Reports**
  - `tools/archivexl_offsets_report_tmp.json` (latest heuristic run)
  - `tools/archivexl_offsets_report.json` (older baseline run)

## How to reproduce the current report

```bash
python3 tools/macos_discover_archivexl_offsets.py \
  --binary "$HOME/Library/Application Support/Steam/steamapps/common/Cyberpunk 2077/Cyberpunk2077.app/Contents/MacOS/Cyberpunk2077" \
  --max-candidates 48 \
  --out tools/archivexl_offsets_report_tmp.json
```

## Deep-dive docs

- `docs/MACOS_ADDRESS_DISCOVERY.md` (pipeline + learnings + remaining gaps)
- `docs/MACOS_PORTING_AUDIT.md` (original feasibility/port audit)

