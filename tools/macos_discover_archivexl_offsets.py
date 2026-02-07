#!/usr/bin/env python3
from __future__ import annotations

import argparse
import bisect
import json
import re
import struct
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable
from collections import defaultdict


IMAGE_BASE = 0x100000000


@dataclass(frozen=True)
class AddressId:
    name: str
    hash: int  # uint32 stored as python int


@dataclass(frozen=True)
class BinString:
    vaddr: int
    text: str


@dataclass(frozen=True)
class Resolved:
    name: str
    hash: int
    offset: int
    evidence: dict


def _repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


def _default_library_path() -> Path:
    return _repo_root() / "src" / "Red" / "Addresses" / "Library.hpp"


def parse_library_hpp(path: Path) -> list[AddressId]:
    """
    Parse `src/Red/Addresses/Library.hpp` and return ordered (name, hash) pairs.

    Expected format:
        constexpr uint32_t Some_Name = 123456789;
    """
    text = path.read_text(encoding="utf-8", errors="replace")
    rx = re.compile(
        r"^\s*constexpr\s+uint32_t\s+(?P<name>[A-Za-z0-9_]+)\s*=\s*(?P<val>\d+)\s*;\s*(?://.*)?$",
        re.MULTILINE,
    )

    items: list[AddressId] = []
    for m in rx.finditer(text):
        items.append(AddressId(name=m.group("name"), hash=int(m.group("val"), 10)))

    if not items:
        raise RuntimeError(f"No 'constexpr uint32_t' entries found in {path}")

    seen: set[int] = set()
    dup: list[int] = []
    for it in items:
        if it.hash in seen:
            dup.append(it.hash)
        seen.add(it.hash)
        if not (0 <= it.hash <= 0xFFFFFFFF):
            raise RuntimeError(f"Hash out of uint32 range for {it.name}: {it.hash}")

    if dup:
        raise RuntimeError(f"Duplicate hashes found in {path}: {dup[:10]} (total={len(dup)})")

    return items


def _run_json(cmd: list[str]) -> dict:
    out = subprocess.check_output(cmd, text=True)
    return json.loads(out)


def load_strings(binary: Path) -> list[BinString]:
    obj = _run_json(["rabin2", "-z", "-j", str(binary)])
    strings = obj.get("strings", [])
    out: list[BinString] = []
    for s in strings:
        text = s.get("string")
        vaddr = s.get("vaddr")
        if isinstance(text, str) and isinstance(vaddr, int):
            out.append(BinString(vaddr=vaddr, text=text))
    return out


def load_sections(binary: Path) -> list[dict]:
    obj = _run_json(["rabin2", "-S", "-j", str(binary)])
    # Older/newer versions use either "sections" or return list directly.
    if isinstance(obj, dict) and "sections" in obj:
        return obj["sections"]
    if isinstance(obj, list):
        return obj
    raise RuntimeError("Unexpected rabin2 -S -j output")


def _find_section(sections: list[dict], suffix: str) -> dict:
    for s in sections:
        name = s.get("name")
        if isinstance(name, str) and name.endswith(suffix):
            return s
    raise RuntimeError(f"Section not found: *{suffix}")


def _decode_uleb128(data: bytes, start: int) -> tuple[int, int]:
    """Return (value, next_index)."""
    value = 0
    shift = 0
    i = start
    while i < len(data):
        b = data[i]
        i += 1
        value |= (b & 0x7F) << shift
        if (b & 0x80) == 0:
            return value, i
        shift += 7
        if shift > 63:
            raise RuntimeError("ULEB128 overflow")
    raise RuntimeError("Truncated ULEB128")


def load_function_starts(binary: Path) -> list[int]:
    """
    Decode Mach-O LC_FUNCTION_STARTS as a sorted list of function start *offsets* (relative to IMAGE_BASE).
    """
    return load_function_starts_from_data(binary.read_bytes())


def load_function_starts_from_data(data: bytes) -> list[int]:
    """
    Decode Mach-O LC_FUNCTION_STARTS from an in-memory Mach-O image.
    Returns a sorted list of function start *offsets* (relative to IMAGE_BASE).
    """
    # mach_header_64: 8 uint32
    if len(data) < 32:
        raise RuntimeError("Binary too small for mach_header_64")
    magic = struct.unpack_from("<I", data, 0)[0]
    if magic != 0xFEEDFACF:
        raise RuntimeError(f"Unsupported Mach-O magic: 0x{magic:08X}")

    _, _, _, _, ncmds, sizeofcmds, _, _ = struct.unpack_from("<8I", data, 0)
    off = 32

    LC_FUNCTION_STARTS = 0x26
    dataoff = None
    datasize = None

    for _ in range(ncmds):
        if off + 8 > len(data):
            raise RuntimeError("Truncated load commands")
        cmd, cmdsize = struct.unpack_from("<II", data, off)
        if cmd == LC_FUNCTION_STARTS:
            # linkedit_data_command: cmd, cmdsize, dataoff, datasize
            if off + 16 > len(data):
                raise RuntimeError("Truncated LC_FUNCTION_STARTS")
            _, _, dataoff, datasize = struct.unpack_from("<4I", data, off)
            break
        off += cmdsize

    if dataoff is None or datasize is None:
        raise RuntimeError("LC_FUNCTION_STARTS not found")
    if dataoff + datasize > len(data):
        raise RuntimeError("LC_FUNCTION_STARTS points outside file")

    blob = data[dataoff : dataoff + datasize]
    starts: list[int] = []
    cur = 0
    i = 0
    while i < len(blob):
        delta, i = _decode_uleb128(blob, i)
        if delta == 0:
            break
        cur += delta
        starts.append(cur)
    if not starts:
        raise RuntimeError("Decoded 0 function starts from LC_FUNCTION_STARTS")
    return starts


def load_lc_main_entryoff(binary: Path) -> int:
    """
    Return Mach-O LC_MAIN entryoff (offset relative to IMAGE_BASE).
    """
    return load_lc_main_entryoff_from_data(binary.read_bytes())


def load_lc_main_entryoff_from_data(data: bytes) -> int:
    """
    Return Mach-O LC_MAIN entryoff from an in-memory Mach-O image (offset relative to IMAGE_BASE).
    """
    if len(data) < 32:
        raise RuntimeError("Binary too small for mach_header_64")
    magic = struct.unpack_from("<I", data, 0)[0]
    if magic != 0xFEEDFACF:
        raise RuntimeError(f"Unsupported Mach-O magic: 0x{magic:08X}")

    _, _, _, _, ncmds, _, _, _ = struct.unpack_from("<8I", data, 0)
    off = 32

    # LC_MAIN is LC_REQ_DYLD | 0x28
    LC_MAIN = 0x80000028

    for _ in range(ncmds):
        if off + 8 > len(data):
            raise RuntimeError("Truncated load commands")
        cmd, cmdsize = struct.unpack_from("<II", data, off)
        if cmd == LC_MAIN:
            # entry_point_command: cmd, cmdsize, entryoff(u64), stacksize(u64)
            if off + 24 > len(data):
                raise RuntimeError("Truncated LC_MAIN")
            _, _, entryoff, _ = struct.unpack_from("<IIQQ", data, off)
            return int(entryoff)
        off += cmdsize

    raise RuntimeError("LC_MAIN not found")


def vaddr_to_file_offset(sections: list[dict], vaddr: int) -> int | None:
    for s in sections:
        svaddr = s.get("vaddr")
        spaddr = s.get("paddr")
        ssize = s.get("vsize") or s.get("size")
        if not isinstance(svaddr, int) or not isinstance(spaddr, int) or not isinstance(ssize, int):
            continue
        if svaddr <= vaddr < (svaddr + ssize):
            return spaddr + (vaddr - svaddr)
    return None


def decode_chained_ptr(raw: int) -> int:
    """
    Best-effort decoding of pointers in __DATA_CONST with chained fixups.

    Some pointers in the file appear as tagged/encoded values; for this port we use the common
    decode heuristic used elsewhere in this workspace: (raw & 0xFFFFFFFF) + IMAGE_BASE.
    """
    if raw == 0:
        return 0
    if IMAGE_BASE <= raw < (IMAGE_BASE + 0x200000000):
        return raw
    return IMAGE_BASE + (raw & 0xFFFFFFFF)


def _split_constant(name: str) -> tuple[str, str]:
    if "_" not in name:
        return name, ""
    a, b = name.split("_", 1)
    return a, b


def _score_string(s: str, class_part_l: str, method_part_l: str) -> tuple[int, int, int, int, int]:
    """
    Lower score is better.
    Score tuple:
      - missing_class (0/1)
      - name_like (0/1)       (prefer non-CName/message-like strings)
      - structured (0/1)      (prefer path/scoped/message strings)
      - missing_method (0/1)  (prefer direct method substring when available)
      - length                (shorter is slightly preferred)
    """
    sl = s.lower()
    missing_class = 0 if class_part_l and class_part_l in sl else 1
    missing_method = 0 if method_part_l and method_part_l in sl else 1
    structured = 1 if ("/" in s or "::" in s or "_" in s or "." in s or " " in s or "\\" in s) else 0
    name_like = 1 if re.fullmatch(r"[A-Za-z][A-Za-z0-9]*", s) else 0
    # Prefer non-name-like, structured strings even if method substring isn't present.
    return (missing_class, name_like, -structured, missing_method, len(s))


def _camel_tokens(s: str) -> list[str]:
    """
    Split CamelCase / PascalCase / ALLCAPS / digits into lowercase tokens.
    Example: "AIWorkspotManager" -> ["ai","workspot","manager"]
    """
    rx = re.compile(r"[A-Z]+(?![a-z])|[A-Z]?[a-z]+|[0-9]+")
    return [m.group(0).lower() for m in rx.finditer(s) if m.group(0)]


GENERIC_TOKENS = {
    "get",
    "set",
    "load",
    "create",
    "init",
    "initialize",
    "uninitialize",
    "post",
    "update",
    "process",
    "add",
    "remove",
    "find",
    "check",
    "on",
    "start",
    "finish",
    "make",
    "register",
    "resolve",
    "serialize",
    "deserialize",
    "attach",
    "detach",
    "wait",
    "reserve",
    "state",
}

GENERIC_CLASS_TOKENS = {
    "data",
    "system",
    "manager",
    "component",
    "controller",
    "resource",
    "request",
    "state",
    "base",
    "info",
    "buffer",
    "array",
    "factory",
    "runtime",
    "helper",
    "service",
    "loader",
    "serializer",
    "character",
    "customization",
}


def pick_candidate_strings(
    strings: list[BinString],
    target: AddressId,
    max_candidates: int,
) -> list[str]:
    """
    Return a ranked list of candidate marker strings that exist in the binary.
    """
    class_part, method_part = _split_constant(target.name)
    class_part_l = class_part.lower()
    method_part_l = method_part.lower()

    class_tokens = [t for t in _camel_tokens(class_part) if len(t) >= 3]
    method_tokens = [t for t in _camel_tokens(method_part) if len(t) >= 3]

    distinct_class_tokens = [t for t in class_tokens if t not in GENERIC_CLASS_TOKENS]
    distinct_method_tokens = [t for t in method_tokens if t not in GENERIC_TOKENS]

    method_is_generic = len(distinct_method_tokens) == 0

    def _collect(*, require_method: bool, require_class: bool) -> list[str]:
        out: list[str] = []
        for bs in strings:
            sl = bs.text.lower()

            direct = bool(method_part_l and method_part_l in sl)
            method_hits = sum(1 for t in method_tokens if t in sl) if method_tokens else int(direct)
            distinct_method_hits = sum(1 for t in distinct_method_tokens if t in sl) if distinct_method_tokens else 0
            class_hits = sum(1 for t in distinct_class_tokens if t in sl) if distinct_class_tokens else 0

            if require_method and method_part_l:
                # Require some method signal (direct match or any token hit).
                if not (direct or method_hits > 0):
                    continue

                # If the method name only matches via tokens, prefer at least one *distinct* method token hit.
                # This avoids picking random strings that only contain generic verbs like "load"/"create".
                if not direct and distinct_method_tokens and distinct_method_hits <= 0:
                    continue

            # For fully generic method names, always require some class signal (even in relaxed modes).
            if method_part_l and method_is_generic:
                if distinct_class_tokens:
                    if class_hits <= 0:
                        continue
                elif class_tokens:
                    if not any(t in sl for t in class_tokens):
                        continue
                elif class_part_l and class_part_l not in sl:
                    continue
            elif require_class:
                # For non-generic methods, require class signal when we have distinct class tokens.
                if distinct_class_tokens and class_hits <= 0:
                    if class_part_l and class_part_l not in sl:
                        continue

            # Avoid extremely short / generic markers.
            if len(bs.text) < 6:
                continue

            out.append(bs.text)
        return out

    # Pass 1: strict (require method + class).
    candidates = _collect(require_method=True, require_class=True)

    # Pass 2: if empty, relax the class requirement for non-generic methods.
    if not candidates:
        candidates = _collect(require_method=True, require_class=False)

    # Pass 3: if still empty, fall back to class-only anchors (some functions have no useful method strings).
    if not candidates and class_part_l:
        candidates = _collect(require_method=False, require_class=True)

    # Deduplicate while preserving order.
    seen: set[str] = set()
    uniq: list[str] = []
    for s in candidates:
        if s not in seen:
            seen.add(s)
            uniq.append(s)

    def _is_name_like(t: str) -> bool:
        return re.fullmatch(r"[A-Za-z][A-Za-z0-9]*", t) is not None

    def _is_profiler_like(t: str) -> bool:
        # Examples: "Entity/Spawn/LoadAppearancePart", "Mesh/FinishAppearancePreload"
        # These are often registered once in ConstNameBuilder and not referenced directly by code.
        if "/" not in t:
            return False
        if any(ch in t for ch in [" ", "%", "\n", "\r", "\\"]):
            return False
        if "." in t:  # file paths / extensions are useful anchors
            return False
        return True

    def _is_message_or_path_like(t: str) -> bool:
        tl = t.lower()
        if any(ch in t for ch in [" ", "%", "\n", "\r", "\\"]):
            return True
        if any(ext in tl for ext in [".cpp", ".hpp", ".h", ".mesh", ".ink", ".json", ".xml", ".yaml", ".yml"]):
            return True
        return False

    # Prefer message/file-path anchors over CName-like tokens.
    if any(not _is_name_like(s) for s in uniq):
        uniq = [s for s in uniq if not _is_name_like(s)]

    # Prefer message/path anchors over profiler-like tokens when available.
    if any(_is_message_or_path_like(s) for s in uniq):
        uniq = [s for s in uniq if _is_message_or_path_like(s) and not _is_profiler_like(s)]

    uniq.sort(key=lambda s: _score_string(s, class_part_l, method_part_l))
    return uniq[:max_candidates]


def scan_adrp_add_xrefs(
    data: bytes,
    text_section: dict,
    target_addrs: set[int],
) -> dict[int, list[int]]:
    """
    Scan __TEXT.__text for ADRP+ADD that materializes one of target_addrs.
    Returns: {target_addr: [ref_vaddr_of_adrp, ...]}
    """
    paddr = int(text_section["paddr"])
    vaddr = int(text_section["vaddr"])
    size = int(text_section.get("vsize") or text_section["size"])

    text = memoryview(data)[paddr : paddr + size]

    target_pages: dict[int, set[int]] = {}
    for a in target_addrs:
        page = a & ~0xFFF
        target_pages.setdefault(page, set()).add(a)

    results: dict[int, list[int]] = {a: [] for a in target_addrs}

    # ARM64 decode loop over __text bytes.
    for off in range(0, len(text) - 8, 4):
        instr = struct.unpack_from("<I", text, off)[0]

        # ADRP (1xx1 0000 ....)
        if (instr & 0x9F000000) != 0x90000000:
            continue

        rd = instr & 0x1F
        immhi = (instr >> 5) & 0x7FFFF
        immlo = (instr >> 29) & 0x3
        imm = (immhi << 2) | immlo
        if imm & 0x100000:  # sign extend 21-bit
            imm |= ~0x1FFFFF

        pc = vaddr + off
        page_addr = (pc & ~0xFFF) + (imm << 12)

        wanted = target_pages.get(page_addr)
        if not wanted:
            continue

        next_instr = struct.unpack_from("<I", text, off + 4)[0]
        # ADD Xd, Xn, #imm (1001 0001 00.. ....)
        if (next_instr & 0xFFC00000) != 0x91000000:
            continue
        add_rn = (next_instr >> 5) & 0x1F
        add_imm = (next_instr >> 10) & 0xFFF
        if add_rn != rd:
            continue

        target = page_addr + add_imm
        if target in wanted:
            results[target].append(pc)

    # Drop empties to keep output small.
    return {k: v for k, v in results.items() if v}


def scan_adr_xrefs(
    data: bytes,
    text_section: dict,
    target_addrs: set[int],
) -> dict[int, list[int]]:
    """
    Scan __TEXT.__text for ADR that materializes one of target_addrs.
    Returns: {target_addr: [ref_vaddr_of_adr, ...]}
    """
    paddr = int(text_section["paddr"])
    vaddr = int(text_section["vaddr"])
    size = int(text_section.get("vsize") or text_section["size"])

    text = memoryview(data)[paddr : paddr + size]
    results: dict[int, list[int]] = {a: [] for a in target_addrs}

    for off in range(0, len(text) - 4, 4):
        instr = struct.unpack_from("<I", text, off)[0]
        # ADR (same encoding family as ADRP, but op=0)
        if (instr & 0x9F000000) != 0x10000000:
            continue

        immhi = (instr >> 5) & 0x7FFFF
        immlo = (instr >> 29) & 0x3
        imm = (immhi << 2) | immlo
        if imm & 0x100000:  # sign extend 21-bit
            imm |= ~0x1FFFFF

        pc = vaddr + off
        target = pc + imm
        if target in results:
            results[target].append(pc)

    return {k: v for k, v in results.items() if v}


def scan_adrp_ldr_xrefs(
    data: bytes,
    sections: list[dict],
    text_section: dict,
    target_addrs: set[int],
) -> dict[int, list[int]]:
    """
    Scan __TEXT.__text for ADRP+LDR (unsigned immediate) that loads a pointer to one of target_addrs.
    Returns: {target_addr: [ref_vaddr_of_adrp, ...]}
    """
    paddr = int(text_section["paddr"])
    vaddr = int(text_section["vaddr"])
    size = int(text_section.get("vsize") or text_section["size"])

    text = memoryview(data)[paddr : paddr + size]
    results: dict[int, list[int]] = {a: [] for a in target_addrs}

    for off in range(0, len(text) - 8, 4):
        instr = struct.unpack_from("<I", text, off)[0]
        # ADRP
        if (instr & 0x9F000000) != 0x90000000:
            continue

        rd = instr & 0x1F
        immhi = (instr >> 5) & 0x7FFFF
        immlo = (instr >> 29) & 0x3
        imm = (immhi << 2) | immlo
        if imm & 0x100000:  # sign extend 21-bit
            imm |= ~0x1FFFFF

        pc = vaddr + off
        page_addr = (pc & ~0xFFF) + (imm << 12)

        next_instr = struct.unpack_from("<I", text, off + 4)[0]
        # LDR Xt, [Xn, #imm12*8] (unsigned immediate, 64-bit)
        if (next_instr & 0xFFC00000) != 0xF9400000:
            continue
        rn = (next_instr >> 5) & 0x1F
        if rn != rd:
            continue
        imm12 = (next_instr >> 10) & 0xFFF
        addr = page_addr + (imm12 << 3)

        fileoff = vaddr_to_file_offset(sections, addr)
        if fileoff is None or fileoff + 8 > len(data):
            continue

        raw_ptr = struct.unpack_from("<Q", data, fileoff)[0]
        ptr = decode_chained_ptr(raw_ptr)
        if ptr in results:
            results[ptr].append(pc)

    return {k: v for k, v in results.items() if v}


def scan_xrefs_to_addrs(
    data: bytes,
    sections: list[dict],
    text_section: dict,
    target_addrs: set[int],
) -> dict[int, list[int]]:
    """
    Combined xref scan to target addresses using multiple AArch64 patterns.
    """
    merged: dict[int, list[int]] = {a: [] for a in target_addrs}

    for part in (
        scan_adrp_add_xrefs(data, text_section, target_addrs),
        scan_adr_xrefs(data, text_section, target_addrs),
        scan_adrp_ldr_xrefs(data, sections, text_section, target_addrs),
    ):
        for tgt, refs in part.items():
            merged[tgt].extend(refs)

    # Deduplicate while preserving order for each target.
    out: dict[int, list[int]] = {}
    for tgt, refs in merged.items():
        if not refs:
            continue
        seen: set[int] = set()
        uniq: list[int] = []
        for r in refs:
            if r not in seen:
                seen.add(r)
                uniq.append(r)
        out[tgt] = uniq

    return out


def refs_to_function_starts(
    refs: Iterable[int],
    function_starts: list[int],
) -> set[int]:
    """
    Map absolute vaddr refs to function start offsets (relative to IMAGE_BASE) using LC_FUNCTION_STARTS.
    """
    out: set[int] = set()
    for ref in refs:
        off = ref - IMAGE_BASE
        idx = bisect.bisect_right(function_starts, off) - 1
        if idx >= 0:
            out.add(function_starts[idx])
    return out


def refs_to_function_start_counts(
    refs: Iterable[int],
    function_starts: list[int],
) -> dict[int, int]:
    """
    Map absolute vaddr refs to {function_start_offset: ref_count} (offsets relative to IMAGE_BASE).
    """
    out: dict[int, int] = {}
    for ref in refs:
        off = ref - IMAGE_BASE
        idx = bisect.bisect_right(function_starts, off) - 1
        if idx < 0:
            continue
        fs = function_starts[idx]
        out[fs] = out.get(fs, 0) + 1
    return out


def _is_brk(instr: int) -> bool:
    # BRK #imm: mask out immediate bits [20:5]
    return (instr & 0xFFE0001F) == 0xD4200000


def _is_bl(instr: int) -> bool:
    return (instr & 0xFC000000) == 0x94000000


def _is_adrp(instr: int) -> bool:
    return (instr & 0x9F000000) == 0x90000000


def _is_adr(instr: int) -> bool:
    return (instr & 0x9F000000) == 0x10000000


def _is_add_imm(instr: int) -> bool:
    return (instr & 0xFFC00000) == 0x91000000


def _is_br(instr: int) -> bool:
    return (instr & 0xFFFFFC1F) == 0xD61F0000


def _is_blr(instr: int) -> bool:
    return (instr & 0xFFFFFC1F) == 0xD63F0000


def _branch_target_imm26(instr: int, pc: int) -> int:
    imm26 = instr & 0x03FFFFFF
    if imm26 & 0x02000000:
        imm26 |= ~0x03FFFFFF
    return (pc + (imm26 << 2)) & 0xFFFFFFFFFFFFFFFF


def _branch_target_imm19(instr: int, pc: int) -> int:
    imm19 = (instr >> 5) & 0x7FFFF
    if imm19 & 0x40000:
        imm19 |= ~0x7FFFF
    return (pc + (imm19 << 2)) & 0xFFFFFFFFFFFFFFFF


def _branch_target_imm14(instr: int, pc: int) -> int:
    imm14 = (instr >> 5) & 0x3FFF
    if imm14 & 0x2000:
        imm14 |= ~0x3FFF
    return (pc + (imm14 << 2)) & 0xFFFFFFFFFFFFFFFF


def is_assert_trap_stub(
    fs: int,
    function_starts: list[int],
    data: bytes,
    text_section: dict,
    *,
    max_size: int = 0x140,
) -> bool:
    """
    Heuristic: detect a tiny "assert/trap" leaf function that ends in BRK and calls into a shared assert routine.
    """
    # Find the next function start to estimate size.
    idx = bisect.bisect_left(function_starts, fs)
    if idx < 0 or idx >= len(function_starts):
        return False
    if function_starts[idx] != fs:
        return False
    if idx + 1 >= len(function_starts):
        return False

    next_fs = function_starts[idx + 1]
    size = next_fs - fs
    if size <= 0 or size > max_size:
        return False

    start_vaddr = IMAGE_BASE + fs
    text_vaddr = int(text_section["vaddr"])
    text_paddr = int(text_section["paddr"])
    text_size = int(text_section.get("vsize") or text_section["size"])
    if not (text_vaddr <= start_vaddr < text_vaddr + text_size):
        return False

    start_off = text_paddr + (start_vaddr - text_vaddr)
    end_off = start_off + size
    if end_off > len(data) or start_off < 0:
        return False

    last_instr = struct.unpack_from("<I", data, end_off - 4)[0]
    if not _is_brk(last_instr):
        return False

    # Must contain at least one BL before the trap (common assert/log routine).
    for off in range(start_off, end_off - 4, 4):
        instr = struct.unpack_from("<I", data, off)[0]
        if _is_bl(instr):
            return True

    return False


def scan_controlflow_xrefs_to_targets(
    data: bytes,
    sections: list[dict],
    text_section: dict,
    target_addrs: set[int],
) -> dict[int, list[int]]:
    """
    Scan __TEXT.__text for control-flow transfers to any of target_addrs.

    Covers:
      - B / BL
      - B.cond
      - CBZ/CBNZ
      - TBZ/TBNZ
      - ADRP+ADD+(BR|BLR)
      - ADRP+LDR+(BR|BLR)   (pointer load; chained-fixup decode)
      - ADR+(BR|BLR)
      - LDR (literal)+(BR|BLR)  (pointer load; chained-fixup decode)

    Returns: {target_addr: [ref_vaddr_of_branch/callsite, ...]}
    """
    paddr = int(text_section["paddr"])
    vaddr = int(text_section["vaddr"])
    size = int(text_section.get("vsize") or text_section["size"])

    text = memoryview(data)[paddr : paddr + size]
    results: dict[int, list[int]] = {a: [] for a in target_addrs}
    tgt_set = set(target_addrs)

    for off in range(0, len(text) - 12, 4):
        instr = struct.unpack_from("<I", text, off)[0]
        pc = vaddr + off

        # B / BL
        op = instr & 0xFC000000
        if op in (0x14000000, 0x94000000):
            tgt = _branch_target_imm26(instr, pc)
            if tgt in tgt_set:
                results[tgt].append(pc)
            continue

        # B.cond
        if (instr & 0xFF000010) == 0x54000000:
            tgt = _branch_target_imm19(instr, pc)
            if tgt in tgt_set:
                results[tgt].append(pc)
            continue

        # CBZ/CBNZ (32/64) — mask out sf bit
        if (instr & 0x7F000000) in (0x34000000, 0x35000000):
            tgt = _branch_target_imm19(instr, pc)
            if tgt in tgt_set:
                results[tgt].append(pc)
            continue

        # TBZ/TBNZ (32/64) — mask out sf bit
        if (instr & 0x7F000000) in (0x36000000, 0x37000000):
            tgt = _branch_target_imm14(instr, pc)
            if tgt in tgt_set:
                results[tgt].append(pc)
            continue

        # ADRP+ADD+(BR|BLR)
        if _is_adrp(instr) and (off + 8) < len(text):
            rd = instr & 0x1F
            immhi = (instr >> 5) & 0x7FFFF
            immlo = (instr >> 29) & 0x3
            imm = (immhi << 2) | immlo
            if imm & 0x100000:
                imm |= ~0x1FFFFF
            page_addr = (pc & ~0xFFF) + (imm << 12)

            add_instr = struct.unpack_from("<I", text, off + 4)[0]
            if not _is_add_imm(add_instr):
                continue
            add_rd = add_instr & 0x1F
            add_rn = (add_instr >> 5) & 0x1F
            if add_rd != rd or add_rn != rd:
                continue
            add_imm = (add_instr >> 10) & 0xFFF
            tgt = page_addr + add_imm
            if tgt not in tgt_set:
                continue

            br_instr = struct.unpack_from("<I", text, off + 8)[0]
            if not (_is_br(br_instr) or _is_blr(br_instr)):
                continue
            rn = (br_instr >> 5) & 0x1F
            if rn != rd:
                continue
            results[tgt].append(pc + 8)
            continue

        # ADRP+LDR+(BR|BLR)
        if _is_adrp(instr) and (off + 8) < len(text):
            rd = instr & 0x1F
            immhi = (instr >> 5) & 0x7FFFF
            immlo = (instr >> 29) & 0x3
            imm = (immhi << 2) | immlo
            if imm & 0x100000:
                imm |= ~0x1FFFFF
            page_addr = (pc & ~0xFFF) + (imm << 12)

            ldr_instr = struct.unpack_from("<I", text, off + 4)[0]
            if (ldr_instr & 0xFFC00000) != 0xF9400000:
                continue
            rn = (ldr_instr >> 5) & 0x1F
            if rn != rd:
                continue
            rt = ldr_instr & 0x1F
            imm12 = (ldr_instr >> 10) & 0xFFF
            load_addr = page_addr + (imm12 << 3)

            fileoff = vaddr_to_file_offset(sections, load_addr)
            if fileoff is None or fileoff + 8 > len(data):
                continue
            raw_ptr = struct.unpack_from("<Q", data, fileoff)[0]
            tgt = decode_chained_ptr(raw_ptr)
            if tgt not in tgt_set:
                continue

            br_instr = struct.unpack_from("<I", text, off + 8)[0]
            if not (_is_br(br_instr) or _is_blr(br_instr)):
                continue
            br_rn = (br_instr >> 5) & 0x1F
            if br_rn != rt:
                continue
            results[tgt].append(pc + 8)
            continue

        # ADR+(BR|BLR)
        if _is_adr(instr) and (off + 4) < len(text):
            rd = instr & 0x1F
            immhi = (instr >> 5) & 0x7FFFF
            immlo = (instr >> 29) & 0x3
            imm = (immhi << 2) | immlo
            if imm & 0x100000:
                imm |= ~0x1FFFFF
            tgt = pc + imm
            if tgt not in tgt_set:
                continue

            br_instr = struct.unpack_from("<I", text, off + 4)[0]
            if not (_is_br(br_instr) or _is_blr(br_instr)):
                continue
            rn = (br_instr >> 5) & 0x1F
            if rn != rd:
                continue
            results[tgt].append(pc + 4)
            continue

        # LDR (literal)+(BR|BLR)
        #   LDR Xt, #imm19
        #   BLR/BR Xt
        if (instr & 0xFF000000) == 0x58000000 and (off + 4) < len(text):
            rt = instr & 0x1F
            lit_addr = _branch_target_imm19(instr, pc)
            fileoff = vaddr_to_file_offset(sections, lit_addr)
            if fileoff is None or fileoff + 8 > len(data):
                continue
            raw_ptr = struct.unpack_from("<Q", data, fileoff)[0]
            tgt = decode_chained_ptr(raw_ptr)
            if tgt not in tgt_set:
                continue
            br_instr = struct.unpack_from("<I", text, off + 4)[0]
            if not (_is_br(br_instr) or _is_blr(br_instr)):
                continue
            br_rn = (br_instr >> 5) & 0x1F
            if br_rn != rt:
                continue
            results[tgt].append(pc + 4)
            continue

    return {k: v for k, v in results.items() if v}


def _read_qword_vaddr(data: bytes, sections: list[dict], vaddr: int) -> int:
    fileoff = vaddr_to_file_offset(sections, vaddr)
    if fileoff is None or fileoff + 8 > len(data):
        raise RuntimeError(f"Could not map vaddr to file offset: 0x{vaddr:X}")
    return struct.unpack_from("<Q", data, fileoff)[0]


def discover_resource_depot_vtable_overrides(
    data: bytes,
    sections: list[dict],
) -> dict[str, tuple[int, dict]]:
    """
    Escape hatch: derive ResourceDepot method addresses from a known vtable in __DATA_CONST for v2.3.1.
    """
    # v2.3.1: identified via res::ResourceGameDepot construction and vtable inspection
    VTABLE_VADDR = 0x10728A630
    REQUEST_INDEX = 3
    EXISTS_INDEX = 4

    req_raw = _read_qword_vaddr(data, sections, VTABLE_VADDR + REQUEST_INDEX * 8)
    ex_raw = _read_qword_vaddr(data, sections, VTABLE_VADDR + EXISTS_INDEX * 8)

    req_ptr = decode_chained_ptr(req_raw)
    ex_ptr = decode_chained_ptr(ex_raw)

    if req_ptr == 0 or ex_ptr == 0:
        raise RuntimeError("ResourceDepot vtable entries decoded to 0")
    if req_ptr < IMAGE_BASE or ex_ptr < IMAGE_BASE:
        raise RuntimeError("ResourceDepot vtable entries decoded outside image")

    req_off = req_ptr - IMAGE_BASE
    ex_off = ex_ptr - IMAGE_BASE

    return {
        "ResourceDepot_RequestResource": (
            req_off,
            {
                "source": "vtable",
                "vtable_vaddr": f"0x{VTABLE_VADDR:X}",
                "index": REQUEST_INDEX,
                "raw_ptr": f"0x{req_raw:X}",
                "decoded_ptr": f"0x{req_ptr:X}",
            },
        ),
        "ResourceDepot_CheckResource": (
            ex_off,
            {
                "source": "vtable",
                "vtable_vaddr": f"0x{VTABLE_VADDR:X}",
                "index": EXISTS_INDEX,
                "raw_ptr": f"0x{ex_raw:X}",
                "decoded_ptr": f"0x{ex_ptr:X}",
            },
        ),
    }


def main() -> int:
    ap = argparse.ArgumentParser(description="Discover ArchiveXL address offsets from Cyberpunk2077 macOS binary")
    ap.add_argument(
        "--binary",
        type=Path,
        required=True,
        help="Path to Cyberpunk2077 Mach-O binary (arm64)",
    )
    ap.add_argument(
        "--library",
        type=Path,
        default=_default_library_path(),
        help="Path to ArchiveXL AddressLib header (default: <repo>/src/Red/Addresses/Library.hpp)",
    )
    ap.add_argument(
        "--max-candidates",
        type=int,
        default=8,
        help="Max candidate marker strings per address (default: 8)",
    )
    ap.add_argument(
        "--out",
        type=Path,
        default=None,
        help="Write JSON report to this path (default: stdout)",
    )
    args = ap.parse_args()

    ids = parse_library_hpp(args.library)
    strings = load_strings(args.binary)
    sections = load_sections(args.binary)
    text_sec = _find_section(sections, "__TEXT.__text")

    bin_data = args.binary.read_bytes()
    function_starts = load_function_starts_from_data(bin_data)
    entryoff = load_lc_main_entryoff_from_data(bin_data)

    # Hard overrides (best-effort "escape hatches" that are higher-confidence than string heuristics).
    hard_overrides: dict[str, tuple[int, dict]] = {}
    hard_overrides.update(discover_resource_depot_vtable_overrides(bin_data, sections))

    # Known-good marker overrides (tight anchors; prefer these over heuristics)
    marker_overrides: dict[str, str] = {
        "TweakDB_Load": "TweakDB/LoadOptimizedDLC",
        "JournalTree_ProcessJournalIndex": "JournalTree/ProcessJournalIndex",
        "JournalRootFolderEntry_Initialize": "JournalRootFolderEntry/Initialize/LoadRootResource",
        "MappinSystem_OnStreamingWorldLoaded": "OnStreamingWorldLoaded",
        "GameApplication_InitResourceDepot": "Core: Failed to initialise a resource depot!",
        # Resource depot archive init/load: prefer specific archive asserts (then caller-chase out of trap stubs).
        "ResourceDepot_InitializeArchives": "!baseSet.archives.Empty()",
        "ResourceDepot_LoadArchives": "LoadEntireArchiveIntoMemory failed: %hs",
    }

    # Build mapping from exact string -> all vaddrs
    str_to_addrs: dict[str, list[int]] = {}
    for s in strings:
        str_to_addrs.setdefault(s.text, []).append(s.vaddr)

    # Candidate markers per AddressId
    candidates_by_name: dict[str, list[str]] = {}
    target_string_addrs: set[int] = set()

    for it in ids:
        if it.name == "Main":
            candidates_by_name[it.name] = []
            continue
        if it.name in hard_overrides:
            candidates_by_name[it.name] = []
            continue

        if it.name in marker_overrides:
            cands = [marker_overrides[it.name]]
        else:
            cands = pick_candidate_strings(strings, it, args.max_candidates)
        candidates_by_name[it.name] = cands
        for c in cands:
            for a in str_to_addrs.get(c, []):
                target_string_addrs.add(a)

    # Bulk xref scan (multiple patterns -> __cstring)
    xrefs = scan_xrefs_to_addrs(bin_data, sections, text_sec, target_string_addrs)

    # Compute global "string-heavy" function penalty to avoid ConstNameBuilder-style initializers.
    func_to_strings: dict[int, set[int]] = defaultdict(set)
    for str_addr, refs in xrefs.items():
        for ref in refs:
            off = ref - IMAGE_BASE
            idx = bisect.bisect_right(function_starts, off) - 1
            if idx < 0:
                continue
            fs = function_starts[idx]
            func_to_strings[fs].add(str_addr)
    func_string_count: dict[int, int] = {fs: len(s) for fs, s in func_to_strings.items()}

    # Detect tiny assert/trap stubs and map them to their caller functions.
    stub_fs_set: set[int] = {
        fs for fs in func_string_count.keys() if is_assert_trap_stub(fs, function_starts, bin_data, text_sec)
    }
    stub_targets: set[int] = {IMAGE_BASE + fs for fs in stub_fs_set}
    stub_xrefs = (
        scan_controlflow_xrefs_to_targets(bin_data, sections, text_sec, stub_targets) if stub_targets else {}
    )

    stub_callers: dict[int, dict[int, int]] = {}
    for fs in stub_fs_set:
        refs = stub_xrefs.get(IMAGE_BASE + fs, [])
        counts = refs_to_function_start_counts(refs, function_starts)
        counts.pop(fs, None)
        stub_callers[fs] = counts

    resolved: list[Resolved] = []
    unresolved: list[dict] = []

    for it in ids:
        if it.name == "Main":
            resolved.append(
                Resolved(
                    name=it.name,
                    hash=it.hash,
                    offset=entryoff,
                    evidence={"source": "LC_MAIN", "entryoff": f"0x{entryoff:X}"},
                )
            )
            continue

        if it.name in hard_overrides:
            off, evidence = hard_overrides[it.name]
            resolved.append(
                Resolved(
                    name=it.name,
                    hash=it.hash,
                    offset=off,
                    evidence=evidence,
                )
            )
            continue

        # Aggregate votes for functions across all candidate markers.
        fs_markers: dict[int, set[str]] = {}
        fs_weighted_score: dict[int, float] = {}
        fs_total_refs: dict[int, float] = {}
        fs_marker_ref_counts: dict[int, dict[str, float]] = {}

        for cand in candidates_by_name.get(it.name, []):
            for addr in str_to_addrs.get(cand, []):
                refs = xrefs.get(addr, [])
                if not refs:
                    continue
                raw_counts = refs_to_function_start_counts(refs, function_starts)
                if not raw_counts:
                    continue

                # If a marker xref lands in a tiny assert/trap stub, attribute the vote to the caller(s) instead.
                counts: dict[int, float] = {}
                for fs, cnt in raw_counts.items():
                    callers = stub_callers.get(fs)
                    if callers:
                        total_calls = sum(callers.values())
                        if total_calls > 0:
                            for cfs, ccnt in callers.items():
                                counts[cfs] = counts.get(cfs, 0.0) + float(cnt) * (float(ccnt) / float(total_calls))
                            continue
                    counts[fs] = counts.get(fs, 0.0) + float(cnt)

                if not counts:
                    continue

                # Penalize markers referenced from many different functions.
                weight = 1.0 / float(len(counts))
                for fs, cnt in counts.items():
                    # Penalize functions that reference many different candidate strings (often init/registration code).
                    fcnt = func_string_count.get(fs, 0)
                    func_penalty = 1.0 / float(1 + fcnt)
                    fs_markers.setdefault(fs, set()).add(cand)
                    fs_weighted_score[fs] = fs_weighted_score.get(fs, 0.0) + float(cnt) * weight * func_penalty
                    fs_total_refs[fs] = fs_total_refs.get(fs, 0.0) + float(cnt)
                    fs_marker_ref_counts.setdefault(fs, {}).setdefault(cand, 0)
                    fs_marker_ref_counts[fs][cand] += cnt

        if not fs_weighted_score:
            unresolved.append(
                {
                    "name": it.name,
                    "hash": it.hash,
                    "hash_hex": f"0x{it.hash:08X}",
                    "candidates": candidates_by_name.get(it.name, []),
                }
            )
            continue

        def _rank(fs: int) -> tuple[int, float, int, int]:
            # Higher is better.
            return (
                len(fs_markers.get(fs, set())),
                fs_weighted_score.get(fs, 0.0),
                fs_total_refs.get(fs, 0),
                -fs,  # prefer lower offset when everything else ties
            )

        best_fs = max(fs_weighted_score.keys(), key=_rank)
        markers = sorted(fs_markers.get(best_fs, set()))
        marker_counts = fs_marker_ref_counts.get(best_fs, {})
        top_marker = max(marker_counts.items(), key=lambda kv: kv[1])[0] if marker_counts else ""
        top_vaddr = (str_to_addrs.get(top_marker) or [0])[0]

        resolved.append(
            Resolved(
                name=it.name,
                hash=it.hash,
                offset=best_fs,
                evidence={
                    "marker": top_marker,
                    "marker_vaddr": f"0x{top_vaddr:X}" if top_vaddr else None,
                    "markers_count": len(markers),
                    "markers": markers[:10],
                    "total_xrefs": fs_total_refs.get(best_fs, 0),
                    "weighted_score": fs_weighted_score.get(best_fs, 0.0),
                },
            )
        )

    report = {
        "binary": str(args.binary),
        "image_base": f"0x{IMAGE_BASE:X}",
        "resolved_count": len(resolved),
        "unresolved_count": len(unresolved),
        "resolved": [
            {
                "name": r.name,
                "hash": r.hash,
                "hash_hex": f"0x{r.hash:08X}",
                "offset": f"0x{r.offset:X}",
                "evidence": r.evidence,
            }
            for r in resolved
        ],
        "unresolved": unresolved,
    }

    out_text = json.dumps(report, indent=2) + "\n"
    if args.out:
        args.out.write_text(out_text, encoding="utf-8")
    else:
        sys.stdout.write(out_text)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())

