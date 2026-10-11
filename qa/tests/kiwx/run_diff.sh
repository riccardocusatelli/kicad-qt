#!/bin/bash
# Differential test of kiwx: each kiwx_diff_*.cpp is built against wxWidgets (wx-config) and
# against kiwx, run in the same environment, and the outputs are compared.
#
#   qa/tests/kiwx/run_diff.sh [build dir] [test names...]
set -e
# KiCad runs in UTF-8 locales (the conversions of char strings depend on it in wxWidgets).
export LC_ALL=C.UTF-8
SRC=$(cd "$(dirname "$0")/../../.." && pwd)
OUT=${1:-/tmp/kiwx_diff}; shift || true
mkdir -p "$OUT"
KIWX_SRCS=$(ls "$SRC"/libs/kiwx/src/*.cpp "$SRC"/libs/kiwx/src/unix/*.cpp)
CXX=${CXX:-g++}
FLAGS="-std=c++20 -O1 -g -Wall -Wno-unused-variable"
KIWX_LIBS="-lexpat -lz -lpcre2-32 -ldl -lpthread"

# kiwx as a static library (rebuilt when a source changes)
mkdir -p "$OUT/kiwx_obj"
for s in $KIWX_SRCS; do
    o="$OUT/kiwx_obj/$(basename "$s" .cpp).o"
    if [ ! -f "$o" ] || [ "$s" -nt "$o" ] || [ -n "$(find "$SRC/libs/kiwx/include" "$SRC/libs/kiwx/src" -newer "$o" -name '*.h' | head -1)" ]; then
        $CXX $FLAGS -I"$SRC/libs/kiwx/include" -c "$s" -o "$o"
    fi
done
ar rcs "$OUT/libkiwx.a" "$OUT"/kiwx_obj/*.o

TESTS=${@:-$(cd "$SRC/qa/tests/kiwx" && ls kiwx_diff_*.cpp | sed 's/\.cpp$//')}
status=0
for t in $TESTS; do
    $CXX $FLAGS $(wx-config --cxxflags base,xml) -I"$SRC/qa/tests/kiwx" "$SRC/qa/tests/kiwx/$t.cpp" \
        -o "$OUT/$t.wx" $(wx-config --libs base,xml)
    $CXX $FLAGS -I"$SRC/libs/kiwx/include" -I"$SRC/qa/tests/kiwx" "$SRC/qa/tests/kiwx/$t.cpp" \
        -o "$OUT/$t.kiwx" "$OUT/libkiwx.a" $KIWX_LIBS
    ( cd "$OUT" && timeout 60 ./$t.wx > $t.wx.txt 2>&1 ) || true
    ( cd "$OUT" && timeout 60 ./$t.kiwx > $t.kiwx.txt 2>&1 ) || true
    # the source locations of the asserts differ
    sed -i -E 's/[^ "]*\.(cpp|h)\([0-9]+\): assert/SRC: assert/' "$OUT/$t.wx.txt" "$OUT/$t.kiwx.txt"
    if diff "$OUT/$t.wx.txt" "$OUT/$t.kiwx.txt" > "$OUT/$t.diff"; then
        echo "same  $t ($(wc -l < "$OUT/$t.wx.txt") lines)"
    else
        echo "DIFF  $t ($(grep -c '^<' "$OUT/$t.diff") lines differ, see $OUT/$t.diff)"
        status=1
    fi
done
exit $status
