#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
${CC:-clang} -DTEST_HOST -O2 source/htmlplay.c -o tests/htmlplay_host
./tests/htmlplay_host games/01_esquivar.html tests/esquivar.ppm
./tests/htmlplay_host games/02_pintar.html tests/pintar.ppm
./source/build.sh
file htmlplay.dge | grep 'ELF 32-bit LSB executable, MIPS'
readelf -h htmlplay.dge | grep -q 'MIPS'
echo 'Pruebas completas: dos juegos HTML en PC + ELF MIPS32 válido.'
