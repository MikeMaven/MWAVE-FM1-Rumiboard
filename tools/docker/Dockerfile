# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Mike Maven and Rumiboard contributors
#
# Everything Rumiboard builds with, in one Linux x86-64 image: the JieLi toolchain (Linux x86-64 only), the AC79
# SDK files the package needs, Python with Pillow (+ fribidi/raqm for the font tool), fontTools and mido, Node for
# the web tests, and Emscripten for the browser emulator. For Macs and other machines where these do not install
# cleanly. Build and use it with tools/docker/run.sh:
#
#   tools/docker/run.sh ./build.sh                  the firmware package (build/melodee.fwsc)
#   tools/docker/run.sh sh tests/run_tests.sh       the host tests
#   tools/docker/run.sh sh web/emu/build.sh         the browser emulator (build/emu)
#
# The repository is mounted at /w; build/ lands in the checkout as usual.
FROM --platform=linux/amd64 python:3.12-slim
# (bookworm with Python 3.12: tools/build.py uses f-string syntax that 3.11 rejects)

RUN apt-get update -qq && apt-get install -y -qq --no-install-recommends \
        ca-certificates curl git xz-utils \
        libfribidi0 libraqm0 build-essential clang \
    && rm -rf /var/lib/apt/lists/*

# Node 22 (the web tests' jsdom needs a current Node)
RUN curl -fsSL https://deb.nodesource.com/setup_22.x | bash - && apt-get install -y -qq nodejs && rm -rf /var/lib/apt/lists/*

# Python tools (the image's own venv, so the host's Python is not involved)
RUN python3 -m venv /opt/py && /opt/py/bin/pip install -q Pillow fonttools mido
ENV PATH=/opt/py/bin:$PATH PYTHON=/opt/py/bin/python

# The JieLi toolchain, downloaded by run.sh into tools/docker/cache/ (JieLi's server refuses the download from
# inside an image build now and then; the host fetches it once), unpacked where tools/get_toolchain.sh would
COPY tools/docker/cache/jieli-linux-toolchain.tar.xz /tmp/toolchain.tar.xz
RUN mkdir -p /root/.jieli/toolchain && tar -xJ --strip-components=1 -C /root/.jieli/toolchain -f /tmp/toolchain.tar.xz \
    && test -d /root/.jieli/toolchain/pi32v2 && rm /tmp/toolchain.tar.xz
ENV JIELI_TOOLCHAIN=/root/.jieli/toolchain

# The AC79 SDK: three files of it go into the package (BUILDING.md)
RUN git clone -q --depth 1 --branch AC79NN_SDK_V1.2.1_2023-12-13 https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK.git /root/fw-AC79_AIoT_SDK
ENV AC79_SDK=/root/fw-AC79_AIoT_SDK

# Emscripten, for web/emu
RUN git clone -q --depth 1 https://github.com/emscripten-core/emsdk.git /opt/emsdk \
    && cd /opt/emsdk && ./emsdk install latest >/dev/null && ./emsdk activate latest >/dev/null
ENV PATH=/opt/emsdk/upstream/emscripten:/opt/emsdk/upstream/bin:$PATH EM_CONFIG=/opt/emsdk/.emscripten

WORKDIR /w
