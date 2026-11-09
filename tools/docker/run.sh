#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Mike Maven and Rumiboard contributors
# Run a build or test command inside the Rumiboard build image (tools/docker/Dockerfile), with the repository
# mounted at /w. Builds the image the first time (a few minutes: it downloads the toolchain, the SDK and Emscripten).
#   tools/docker/run.sh ./build.sh
#   tools/docker/run.sh sh tests/run_tests.sh
#   tools/docker/run.sh sh web/emu/build.sh
# REBUILD=1 rebuilds the image.
set -e
cd "$(dirname "$0")/../.."
IMG=rumiboard-build
TC=tools/docker/cache/jieli-linux-toolchain.tar.xz          # (the toolchain, fetched here once: see the Dockerfile)
if [ -n "$REBUILD" ] || ! docker image inspect "$IMG" >/dev/null 2>&1; then
    if [ ! -s "$TC" ]; then
        mkdir -p tools/docker/cache
        curl -fL --retry 5 https://pkgman.jieliapp.com/s/linux-toolchain -o "$TC"
    fi
    docker build --platform linux/amd64 -t "$IMG" -f tools/docker/Dockerfile .
fi
[ $# -gt 0 ] || set -- sh
exec docker run --rm -it --platform linux/amd64 -v "$PWD":/w -w /w "$IMG" "$@"
