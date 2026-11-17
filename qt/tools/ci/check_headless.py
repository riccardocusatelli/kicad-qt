#!/usr/bin/env python3
#
# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright The KiCad Developers, see AUTHORS.txt for contributors.
#
# This program is free software; you can redistribute it and/or modify it under the terms of
# the GNU General Public License as published by the Free Software Foundation; either version 3
# of the License, or (at your option) any later version.

"""Checks a KICAD_HEADLESS_ONLY build tree (Linux): kicad-cli and its KIFACEs are there, no
program or library of the build links a GUI, OpenGL or desktop library, and no object of the
build includes their headers.

Usage: check_headless.py <build directory>
"""

import os
import re
import subprocess
import sys

# Libraries that a headless kicad-cli must not link directly
FORBIDDEN_LIBS = re.compile(
    r"^lib(GL|GLU|GLX|EGL|OpenGL|GLEW|X11|X[a-z]+|xcb.*|wayland-.*|gtk-.*|gdk-.*|secret-1|"
    r"poppler.*|spnav|Qt6.*|wx_.*)\.so")

# Headers of the same libraries
FORBIDDEN_HEADERS = re.compile(
    r"^/usr/include/(GL|EGL|X11|xcb|gtk-3\.0|gdk-pixbuf-2\.0|libsecret-1|poppler|"
    r"x86_64-linux-gnu/qt6|qt6|wx-3\.\d)/|/usr/include/(wayland-[^/]+|spnav[^/]*)\.h$")

REQUIRED = [ "kicad/kicad-cli", "pcbnew/_pcbnew_cli.kiface", "eeschema/_eeschema_cli.kiface" ]


def is_elf( path ):
    try:
        with open( path, "rb" ) as f:
            return f.read( 4 ) == b"\x7fELF"
    except OSError:
        return False


def needed( path ):
    out = subprocess.run( [ "readelf", "-d", path ], capture_output=True, text=True ).stdout
    return re.findall( r"\(NEEDED\)\s+Shared library: \[([^\]]+)\]", out )


def main():
    build = sys.argv[1]
    errors = []

    for rel in REQUIRED:
        if not os.path.isfile( os.path.join( build, rel ) ):
            errors.append( f"missing: {rel}" )

    count = 0

    for root, dirs, files in os.walk( build ):
        dirs[:] = [ d for d in dirs if d not in ( "CMakeFiles", ".ccache" ) ]

        for name in files:
            path = os.path.join( root, name )

            if name.endswith( ( ".o", ".a" ) ) or os.path.islink( path ) or not is_elf( path ):
                continue

            count += 1

            for lib in needed( path ):
                if FORBIDDEN_LIBS.match( lib ):
                    errors.append( f"{os.path.relpath( path, build )} links {lib}" )

    print( f"{count} programs and libraries checked" )

    deps = subprocess.run( [ "ninja", "-C", build, "-t", "deps" ], capture_output=True,
                           text=True, check=True ).stdout
    obj = None
    headers = set()

    for line in deps.splitlines():
        if line and not line.startswith( " " ):
            obj = line.split( ":" )[0]
        elif line.strip() and FORBIDDEN_HEADERS.search( line.strip() ):
            headers.add( ( obj, line.strip() ) )

    for obj, header in sorted( headers ):
        errors.append( f"{obj} includes {header}" )

    for e in errors:
        print( "ERROR:", e )

    if errors:
        sys.exit( 1 )

    print( "OK: no GUI, OpenGL or desktop library or header" )


if __name__ == "__main__":
    main()
