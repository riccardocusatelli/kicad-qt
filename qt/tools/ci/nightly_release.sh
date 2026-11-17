#!/usr/bin/env bash
# The "nightly" pre-release of GitHub: the packages of the last commit of the main branch whose
# CI is green, shown on the home page of the repository (Releases).
#
#   nightly_release.sh <pattern of the assets of this platform> <file>...
#
# Each package workflow (Windows, macOS) calls it at the end of a green run on a push: the tag
# moves to the commit, the assets of the platform are replaced. Needs GH_TOKEN (contents: write),
# GH_REPO and GITHUB_SHA.
set -euo pipefail

tag=nightly
pattern=$1
shift

notes="Packages of the Qt build of KiCad (no wxWidgets), built by the CI from the last commit of
\`wx-free-core-upstream\` with green tests on its platform: \`${GITHUB_SHA}\`.

- Windows: \`*-portable.zip\` (extract it and run \`KiCad.cmd\`) or the installer \`*.exe\`.
- macOS (Apple Silicon): \`*.dmg\`. Not signed: the first time, right click on KiCad and Open."

# The release (the other platform may create it at the same time)
if ! gh release view "$tag" > /dev/null 2>&1; then
    gh release create "$tag" --prerelease --title "KiCad Qt nightly" --notes "$notes" \
        --target "$GITHUB_SHA" || gh release view "$tag" > /dev/null
fi

# The tag on this commit
git tag -f "$tag" "$GITHUB_SHA"
git push -f origin "refs/tags/$tag"

# The assets of this platform: the old ones (another version in the name) are removed
gh release view "$tag" --json assets --jq '.assets[].name' | { grep -E "$pattern" || true; } |
    while read -r asset; do
        gh release delete-asset "$tag" "$asset" --yes
    done

gh release upload "$tag" "$@" --clobber
gh release edit "$tag" --prerelease --notes "$notes"
