# syntax=docker/dockerfile:1

ARG REGISTRY=192.168.1.246:5000
ARG REPOSITORY=toolchains/gcc
ARG GCC_TAG=linux-native-nightly   # better: pin a commit tag
ARG CABIN_VERSION=0.17.0

# Stage 1: prebuilt native GCC trunk from the local registry
FROM ${REGISTRY}/${REPOSITORY}:${GCC_TAG} AS gcc

# Stage 2: Cabin
FROM rust:slim AS cabin
ARG CABIN_VERSION
RUN cargo install cabinpkg --version ${CABIN_VERSION} --locked --root /out

# Stage 3: dev image
FROM ubuntu:24.04 AS dev
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates git make ninja-build pkg-config ccache gdb \
        clang-format clangd \
        binutils libc6-dev \
        libgmp10 libmpfr6 libmpc3 libisl23 libzstd1 zlib1g \
    && rm -rf /var/lib/apt/lists/*

COPY --from=gcc /opt/gcc-trunk /opt/gcc-trunk
COPY --from=cabin /out/bin/ /usr/local/bin/

ENV PATH=/opt/gcc-trunk/bin:${PATH} \
    LD_LIBRARY_PATH=/opt/gcc-trunk/lib64 \
    CC=/opt/gcc-trunk/bin/gcc \
    CXX=/opt/gcc-trunk/bin/g++ \
    CCACHE_DIR=/ccache

# Smoke test: fail the build if the toolchain can't compile, link and run iostream
RUN printf '#include <iostream>\nint main(){std::cout<<"ok\\n";}\n' > /tmp/t.cc \
    && g++ -std=c++26 /tmp/t.cc -o /tmp/t && /tmp/t \
    && rm /tmp/t /tmp/t.cc

ARG UID=1000
ARG GID=1000
RUN userdel -r ubuntu 2>/dev/null || true \
    && groupadd -g ${GID} dev \
    && useradd -m -u ${UID} -g ${GID} -s /bin/bash dev \
    && mkdir -m 0777 /ccache \
    && install -d -o dev -g dev /home/dev/.cache \
    && chown dev:dev /ccache
USER dev
WORKDIR /mnt
