#!/bin/bash
#
#  This program source code file is part of KICAD, a free EDA CAD application.
#
#  Copyright The KiCad Developers, see AUTHORS.txt for contributors.
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 3
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

# The 3D models of a package: kicad-cli exports the board qa/data/pcbnew/qt_3d_models to VRML,
# which reads its models with the 3D plugins of the package (the ones the 3D viewer uses, found
# where the package has them): the STEP model with the plugin of OpenCASCADE, converted to VRML,
# and the VRML model with the plugin of VRML, in the board.
#
# usage: check_3d_models.sh <output directory> <kicad-cli command...>

set -e

out=$1
shift
board=$(cd "$(dirname "$0")/../../../qa/data/pcbnew/qt_3d_models" && pwd)/qt_3d_models.kicad_pcb

rm -rf "$out"
mkdir -p "$out/models"

# The models in a directory: the STEP model is converted by its plugin (the VRML one is copied)
"$@" pcb export vrml --models-dir "$out/models" --output "$out/models.wrl" "$board"
ls -l "$out/models"

if [ ! -s "$out/models/TO-252-2.wrl" ]; then
    echo "FAIL: the STEP model is not read (the 3D plugin of OpenCASCADE)"
    exit 1
fi

# The models in the board: the VRML model read by its plugin (its colour in the board)
"$@" pcb export vrml --output "$out/board.wrl" "$board"

if ! grep -Eq "diffuseColor 0\.8(0*1)? 0\.2 0\.1" "$out/board.wrl"; then
    echo "FAIL: the VRML model is not read (the 3D plugin of VRML)"
    exit 1
fi

echo "ok: the 3D plugins read the STEP and VRML models"
