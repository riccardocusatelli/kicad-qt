#!/bin/bash
# Differential test of kiwx on Windows, from Linux: each kiwx_diff_*.cpp is cross-built with
# MinGW against wxWidgets for Windows (wxMSW, wxBase is enough) and against kiwx, both are run
# with Wine and the outputs are compared.
#
#   WX_MINGW_CONFIG=<wx-config of wxBase built for x86_64-w64-mingw32>
#   WX_MINGW_SRC=<its wxWidgets sources (the bundled zlib, expat and PCRE2, reused for kiwx)>
#   qa/tests/kiwx/run_diff_mingw.sh [build dir] [test names...]
#
# wxBase for Windows, e.g.:
#   ../configure --host=x86_64-w64-mingw32 --disable-gui --disable-shared --with-zlib=builtin \
#                --with-expat=builtin --with-regex=builtin --prefix=...
#
# Expected differences, from the test environment and not from kiwx:
# - the log messages and the asserts of wxWidgets go to OutputDebugString() when stderr is not a
#   console (redirected here): they are left out of the comparison;
# - "%a" in wxString::Format(): wxWidgets built with MinGW formats with msvcrt.dll, without it;
# - (const char*) wxString::c_str(): wxWidgets converts with the ANSI code page (1252 in Wine,
#   UTF-8 with the manifest of KiCad), kiwx always with UTF-8.
set -e
: "${WX_MINGW_CONFIG:?set WX_MINGW_CONFIG}" "${WX_MINGW_SRC:?set WX_MINGW_SRC}"
SRC=$(cd "$(dirname "$0")/../../.." && pwd)
OUT=${1:-/tmp/kiwx_diff_mingw}; shift || true
mkdir -p "$OUT"
KIWX_SRCS=$(ls "$SRC"/libs/kiwx/src/*.cpp "$SRC"/libs/kiwx/src/msw/*.cpp)
CXX=${CXX:-x86_64-w64-mingw32-g++-posix}
FLAGS="-std=c++20 -O1 -g -Wall -Wno-unused-variable -D__USE_MINGW_ANSI_STDIO=1 -DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN -DNOMINMAX"
# the libraries bundled with wxWidgets: zlib (with the wx_zlib_ prefix), expat, PCRE2 (16 bit)
WXLIBDIR=$("$WX_MINGW_CONFIG" --prefix)/lib
THIRDPARTY="-I$WX_MINGW_SRC/src/zlib -I$WX_MINGW_SRC/src/expat/expat/lib -I$WX_MINGW_SRC/3rdparty/pcre/src/wx -DPCRE2_STATIC -DXML_STATIC"
KIWX_LIBS="$(ls "$WXLIBDIR"/libwxregexu-*.a "$WXLIBDIR"/libwxexpat-*.a "$WXLIBDIR"/libwxzlib-*.a) -lshell32 -lshlwapi -lole32 -luuid -static"

mkdir -p "$OUT/kiwx_obj"
for s in $KIWX_SRCS; do
    o="$OUT/kiwx_obj/$(basename "$s" .cpp).o"
    if [ ! -f "$o" ] || [ "$s" -nt "$o" ] || [ -n "$(find "$SRC/libs/kiwx/include" "$SRC/libs/kiwx/src" -newer "$o" -name '*.h' | head -1)" ]; then
        $CXX $FLAGS -DKIWX_STATIC $THIRDPARTY -I"$SRC/libs/kiwx/include" -c "$s" -o "$o"
    fi
done
rm -f "$OUT/libkiwx.a"
x86_64-w64-mingw32-ar rcs "$OUT/libkiwx.a" "$OUT"/kiwx_obj/*.o

export WINEDEBUG=-all
TESTS=${@:-$(cd "$SRC/qa/tests/kiwx" && ls kiwx_diff_*.cpp | sed 's/\.cpp$//')}
status=0
for t in $TESTS; do
    $CXX $FLAGS $("$WX_MINGW_CONFIG" --cxxflags base,xml) -I"$SRC/qa/tests/kiwx" \
        "$SRC/qa/tests/kiwx/$t.cpp" -o "$OUT/$t.wx.exe" $("$WX_MINGW_CONFIG" --libs base,xml) -static
    $CXX $FLAGS -DKIWX_STATIC -I"$SRC/libs/kiwx/include" -I"$SRC/qa/tests/kiwx" "$SRC/qa/tests/kiwx/$t.cpp" \
        -o "$OUT/$t.kiwx.exe" "$OUT/libkiwx.a" $KIWX_LIBS
    ( cd "$OUT" && timeout 120 wine ./$t.wx.exe > $t.wx.txt 2>&1 ) || true
    ( cd "$OUT" && timeout 120 wine ./$t.kiwx.exe > $t.kiwx.txt 2>&1 ) || true
    # Wine writes CR LF; the log messages and asserts are only in the kiwx output (see above)
    sed -i -E 's/\r$//; /^[0-9]{2}:[0-9]{2}:[0-9]{2}: /d; /: assert /d' "$OUT/$t.wx.txt" "$OUT/$t.kiwx.txt"
    if diff "$OUT/$t.wx.txt" "$OUT/$t.kiwx.txt" > "$OUT/$t.diff"; then
        echo "same  $t ($(wc -l < "$OUT/$t.wx.txt") lines)"
    else
        echo "DIFF  $t ($(grep -c '^<' "$OUT/$t.diff") lines differ, see $OUT/$t.diff)"
        status=1
    fi
done
exit $status
