#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
${CC:-clang} -DTEST_HOST -O2 source/htmlplay.c -o tests/htmlplay_host

count=0
for game in games/*.html; do
    [ -f "$game" ] || continue
    name=$(basename "$game" .html)
    ./tests/htmlplay_host "$game" "tests/$name.ppm"
    count=$((count + 1))
done
[ "$count" -eq 8 ] || { echo "Se esperaban 8 juegos HTML, hay $count" >&2; exit 1; }

./source/build.sh
file htmlplay.dge | grep 'ELF 32-bit LSB executable, MIPS'
readelf -h htmlplay.dge | grep -q 'MIPS'
echo "Pruebas completas: $count juegos HTML en PC + ELF MIPS32 valido."
