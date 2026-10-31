#!/bin/sh
set -eu
cd "$(dirname "$0")"
CC=${CC:-clang}
"$CC" --target=mipsel-linux-gnu -march=mips32 -mabi=32 -EL -O2 -G0 \
  -fno-builtin -fno-stack-protector -fno-pie -mno-abicalls -nostdlib \
  -fuse-ld=lld -Wl,-e,_start -static -Wl,--no-undefined \
  -o ../htmlplay.dge htmlplay.c
file ../htmlplay.dge
