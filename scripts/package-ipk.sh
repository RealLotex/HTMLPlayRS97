#!/bin/sh
# Package a built MIPS32 executable as a RetroFW/GMenu2X .ipk.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BIN=${1:-"$ROOT/htmlplay.dge"}
VERSION=${VERSION:-0.1}
OUTPUT=${OUTPUT:-"$ROOT/dist/htmlplay_rs97_v${VERSION}.ipk"}
case "$VERSION" in *[!A-Za-z0-9.+-]*|'') echo "Versión inválida: $VERSION" >&2; exit 1;; esac
for tool in readelf tar ar mktemp; do
    command -v "$tool" >/dev/null 2>&1 || { echo "Falta: $tool" >&2; exit 1; }
done
[ -f "$BIN" ] || { echo "No existe el ejecutable: $BIN" >&2; exit 1; }
HEADER=$(readelf -h "$BIN" 2>/dev/null) || { echo "El archivo no es un ELF válido" >&2; exit 1; }
printf '%s\n' "$HEADER" | grep -q 'Class:.*ELF32' || { echo 'Se requiere ELF32' >&2; exit 1; }
printf '%s\n' "$HEADER" | grep -q 'Data:.*little endian' || { echo 'Se requiere little-endian' >&2; exit 1; }
printf '%s\n' "$HEADER" | grep -q 'Machine:.*MIPS' || { echo 'Se requiere CPU MIPS' >&2; exit 1; }
printf '%s\n' "$HEADER" | grep -q 'Flags:.*o32' || { echo 'Se requiere ABI MIPS o32' >&2; exit 1; }
if readelf -l "$BIN" | grep -q 'INTERP'; then
    echo 'Este empaquetador requiere un ejecutable estático sin intérprete dinámico' >&2
    exit 1
fi
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT HUP INT TERM
mkdir -p "$TMP/data/home/retrofw/apps/htmlplay/games" "$TMP/data/home/retrofw/apps/gmenu2x/sections/applications" "$TMP/control"
printf '2.0\n' > "$TMP/debian-binary"
cat > "$TMP/control/control" <<META
Package: htmlplay-rs97
Version: $VERSION
Description: Ejecutor local de HTML Canvas con JavaScript basico
Section: applications
Priority: optional
Maintainer: HTMLPlay project
Architecture: mipsel
META
cat > "$TMP/data/home/retrofw/apps/gmenu2x/sections/applications/htmlplay.lnk" <<'LNK'
title=HTMLPlay RS97
description=Juegos HTML con Canvas y JS basico
exec=/home/retrofw/apps/htmlplay/htmlplay.dge
icon=/home/retrofw/apps/htmlplay/htmlplay.png
clock=600
LNK
cp "$BIN" "$TMP/data/home/retrofw/apps/htmlplay/htmlplay.dge"
chmod 755 "$TMP/data/home/retrofw/apps/htmlplay/htmlplay.dge"
cp "$ROOT/htmlplay.png" "$ROOT/README.md" "$TMP/data/home/retrofw/apps/htmlplay/"
cp "$ROOT/games/"*.html "$TMP/data/home/retrofw/apps/htmlplay/games/"
(cd "$TMP/control" && tar -czf "$TMP/control.tar.gz" ./control)
(cd "$TMP/data" && tar -czf "$TMP/data.tar.gz" ./home)
mkdir -p "$(dirname -- "$OUTPUT")"
OUTPUT=$(cd "$(dirname -- "$OUTPUT")" && pwd)/$(basename -- "$OUTPUT")
rm -f "$OUTPUT"
(cd "$TMP" && ar q "$OUTPUT" debian-binary control.tar.gz data.tar.gz >/dev/null)
echo "Paquete generado: $OUTPUT"
