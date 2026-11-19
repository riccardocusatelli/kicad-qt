#!/bin/bash
#
# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright The KiCad Developers, see AUTHORS.txt for contributors.
#
# This program is free software; you can redistribute it and/or modify it under the terms of
# the GNU General Public License as published by the Free Software Foundation; either version 3
# of the License, or (at your option) any later version.
#
# Runs kicad-cli of a build tree as on a server: no display, no Wayland, no Qt platform, a new
# empty home (no configuration, no library tables).  DRC, ERC and the exports of the demos must
# succeed, write their files and print no assert.
#
# Usage: headless_cli_test.sh <build>/kicad/kicad-cli <output directory>

set -euo pipefail

CLI=$(realpath "$1")
OUT=$(mkdir -p "$2" && realpath "$2")
SRC=$(realpath "$(dirname "$0")/../../..")
BOARD=$SRC/demos/video/video.kicad_pcb
SCH=$SRC/demos/pic_programmer/pic_programmer.kicad_sch
HOME_DIR=$(mktemp -d)
FAILED=0

run()
{
    local name=$1
    shift
    echo "::group::$name"

    if env -u DISPLAY -u WAYLAND_DISPLAY -u QT_QPA_PLATFORM -u XDG_RUNTIME_DIR \
           HOME="$HOME_DIR" XDG_CONFIG_HOME="$HOME_DIR/.config" KICAD_RUN_FROM_BUILD_DIR=1 \
           timeout 900 "$CLI" "$@" > "$OUT/$name.log" 2>&1; then
        status=0
    else
        status=$?
    fi

    cat "$OUT/$name.log"
    echo "::endgroup::"

    if [ $status -ne 0 ]; then
        echo "::error::$name: exit code $status"
        FAILED=1
    elif grep -qi "assert" "$OUT/$name.log"; then
        echo "::error::$name: assert in the output"
        FAILED=1
    fi
}

# A file (or a directory with files) must have been written
expect()
{
    if [ -d "$1" ] && [ -n "$(ls -A "$1")" ] || [ -s "$1" ]; then
        echo "ok: $1"
    else
        echo "::error::not written: $1"
        FAILED=1
    fi
}

run version version
run pcb-drc pcb drc --format json -o "$OUT/drc.json" "$BOARD"
expect "$OUT/drc.json"
run sch-erc sch erc --format json -o "$OUT/erc.json" "$SCH"
expect "$OUT/erc.json"
run pcb-gerbers pcb export gerbers -o "$OUT/gerbers/" "$BOARD"
expect "$OUT/gerbers"
run pcb-drill pcb export drill -o "$OUT/drill/" "$BOARD"
expect "$OUT/drill"
run pcb-pos pcb export pos -o "$OUT/video.pos" "$BOARD"
expect "$OUT/video.pos"
run pcb-svg pcb export svg --layers F.Cu,Edge.Cuts -o "$OUT/video.svg" "$BOARD"
expect "$OUT/video.svg"
# The board only: the demo refers to 3D models of the libraries, not installed here (a missing
# model makes the exit code 2)
run pcb-step pcb export step --board-only --include-tracks -o "$OUT/video.step" "$BOARD"
expect "$OUT/video.step"
# The VRML export uses the GLU tessellator (thirdparty/libtess without OpenGL)
run pcb-vrml pcb export vrml -o "$OUT/video.wrl" "$BOARD"
expect "$OUT/video.wrl"
run pcb-render pcb render --quality basic -o "$OUT/video.png" "$BOARD"
expect "$OUT/video.png"
run sch-pdf sch export pdf -o "$OUT/pic_programmer.pdf" "$SCH"
expect "$OUT/pic_programmer.pdf"
run sch-netlist sch export netlist -o "$OUT/pic_programmer.net" "$SCH"
expect "$OUT/pic_programmer.net"
run sch-bom sch export bom -o "$OUT/pic_programmer.csv" "$SCH"
expect "$OUT/pic_programmer.csv"

# The reports are valid and find what the demos have
python3 - "$OUT" <<'PY'
import json, sys
out = sys.argv[1]
drc = json.load( open( f"{out}/drc.json" ) )
erc = json.load( open( f"{out}/erc.json" ) )
n_drc = len( drc["violations"] )
n_erc = sum( len( s["violations"] ) for s in erc["sheets"] )
print( f"DRC: {n_drc} violations, {len( drc['unconnected_items'] )} unconnected; ERC: {n_erc}" )
# The video demo is clean in KiCad 11 (0 DRC violations): the DRC report must be complete,
# the ERC of pic_programmer still finds its violations
if "unconnected_items" not in drc or n_erc == 0:
    sys.exit( "::error::incomplete DRC report or empty ERC report" )
PY

rm -rf "$HOME_DIR"
exit $FAILED
