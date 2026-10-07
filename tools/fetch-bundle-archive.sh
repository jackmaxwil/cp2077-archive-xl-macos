#!/bin/bash
# Put ArchiveXL's packed resources, ArchiveXL.archive, into bundle/packed/archive/pc/mod/ (where RED4ext's
# tools/cp-dev and scripts/create_release.sh take it from).
#
# The archive is built with WolvenKit's app (Windows) from base-game files, so it is taken from upstream's release
# for the same version instead. Archives are platform-independent. The download is checked against pinned checksums.
set -euo pipefail

VERSION=1.26.1
ZIP_SHA256=7cb3e7eeec9a8561fe0e8543a2d65c53cc4ec9921ee2b144b2968b3d2db9ddab
ARCHIVE_SHA256=46390178f9a85c6c2348e0635d47d0565a5d9581e351150847dea5f3439484eb

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="$ROOT/bundle/packed/archive/pc/mod/ArchiveXL.archive"
if [[ -f "$OUT" ]] && [[ "$(shasum -a 256 "$OUT" | cut -d' ' -f1)" == "$ARCHIVE_SHA256" ]]; then
    echo "ArchiveXL.archive $VERSION already in place"
    exit 0
fi

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
curl -fsSL -o "$TMP/axl.zip" "https://github.com/psiberx/cp2077-archive-xl/releases/download/v$VERSION/ArchiveXL-$VERSION.zip"
[[ "$(shasum -a 256 "$TMP/axl.zip" | cut -d' ' -f1)" == "$ZIP_SHA256" ]] || { echo "checksum mismatch for ArchiveXL-$VERSION.zip" >&2; exit 1; }
unzip -q -o "$TMP/axl.zip" red4ext/plugins/ArchiveXL/Bundle/ArchiveXL.archive -d "$TMP"
[[ "$(shasum -a 256 "$TMP/red4ext/plugins/ArchiveXL/Bundle/ArchiveXL.archive" | cut -d' ' -f1)" == "$ARCHIVE_SHA256" ]] || { echo "checksum mismatch for ArchiveXL.archive" >&2; exit 1; }
mkdir -p "$(dirname "$OUT")"
mv "$TMP/red4ext/plugins/ArchiveXL/Bundle/ArchiveXL.archive" "$OUT"
echo "ArchiveXL.archive $VERSION -> $OUT"
