#!/usr/bin/env bash
# The AppImage of the Qt build on Linux, from a staged install (DESTDIR, prefix /usr).
#
#   appimage.sh <AppDir> <output directory> <version> [<directory of the libraries>]
#
# The libraries (symbols, footprints, template with the tables) are optional: their install
# (cmake --install of kicad-symbols, kicad-footprints, kicad-templates: their share/kicad) goes
# to share/kicad.
# Needs linuxdeploy and linuxdeploy-plugin-qt (LINUXDEPLOY, the plugin next to it or on the PATH)
# and QMAKE (the qmake of the Qt the build used).
set -euo pipefail
trap 'echo "appimage.sh: failed at line $LINENO" >&2' ERR

appdir=$(realpath "$1")
out=$(realpath -m "$2")
version=$3
libs=${4:-}
linuxdeploy=${LINUXDEPLOY:-linuxdeploy}

test -x "$appdir/usr/bin/kicad-qt"
mkdir -p "$out"

# The libraries, with the data
if [ -n "$libs" ]; then
    for dir in symbols footprints template; do
        [ -d "$libs/$dir" ] || { echo "No $dir in $libs" >&2; exit 1; }
        mkdir -p "$appdir/usr/share/kicad/$dir"
        cp -R "$libs/$dir/." "$appdir/usr/share/kicad/$dir/"
    done
fi

# KiCad looks for the libraries in $APPDIR/share/kicad (PATHS::GetStockEDALibraryPath)
ln -sfn usr/share "$appdir/share"

# One launcher: the shell of the Qt build (the editors are KIFACEs it loads, next to it)
desktop=$appdir/usr/share/applications/org.kicad.kicad.desktop
test -f "$desktop"
find "$appdir/usr/share/applications" -name '*.desktop' ! -name org.kicad.kicad.desktop -delete
sed -i -e 's/^Exec=kicad /Exec=kicad-qt /' -e 's/^Exec=kicad$/Exec=kicad-qt/' "$desktop"
grep -q '^Exec=kicad-qt' "$desktop"

# The data of KiCad in the image, whatever the prefix of the build
cat > "$out/AppRun" <<'APPRUN'
#!/bin/sh
HERE=$(dirname "$(readlink -f "$0")")
export APPDIR=${APPDIR:-$HERE}
export KICAD_STOCK_DATA_HOME=${KICAD_STOCK_DATA_HOME:-$APPDIR/usr/share/kicad}
case "$(basename "${ARGV0:-$0}")" in
    kicad-cli*) exec "$APPDIR/usr/bin/kicad-cli" "$@" ;;
esac
if [ "$1" = "kicad-cli" ]; then
    shift
    exec "$APPDIR/usr/bin/kicad-cli" "$@"
fi
exec "$APPDIR/usr/bin/kicad-qt" "$@"
APPRUN
chmod +x "$out/AppRun"

# The libraries the programs, the KIFACEs and Qt (with its plugins) need
deps=()
for kiface in "$appdir"/usr/bin/*.kiface; do
    deps+=( --deploy-deps-only "$kiface" )
done

# The KiCad libraries and the 3D plugins ($APPDIR/usr/lib/x86_64-linux-gnu/kicad/plugins/3d, see
# PATHS::GetStockPluginsPath)
for lib in "$appdir"/usr/lib*/libki*.so* "$appdir"/usr/lib*/*/libki*.so* \
           "$appdir"/usr/lib/*/kicad/plugins/3d/*.so*; do
    [ -e "$lib" ] && deps+=( --deploy-deps-only "$lib" )
done

# The shared libraries of KiCad (lib/<multiarch> of the prefix): found by linuxdeploy
libdirs=$(find "$appdir/usr" -name 'lib*.so*' ! -name '*.kiface' -printf '%h\n' | sort -u | paste -sd: -)
export LD_LIBRARY_PATH=$libdirs${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}

export LDAI_OUTPUT="$out/kicad-qt-$version-x86_64.AppImage"
export LINUXDEPLOY_OUTPUT_VERSION=$version
export NO_STRIP=1   # the libraries of the distribution: their strip can be too old for them

"$linuxdeploy" --appdir "$appdir" \
    --executable "$appdir/usr/bin/kicad-qt" --executable "$appdir/usr/bin/kicad-cli" \
    "${deps[@]}" \
    --desktop-file "$desktop" \
    --custom-apprun "$out/AppRun" \
    --plugin qt --output appimage

rm -f "$out/AppRun"
ls -l "$LDAI_OUTPUT"
