FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    clang \
    clang-format \
    clang-tidy \
    cmake \
    gdb \
    git \
    libclang-rt-18-dev \
    lcov \
    libusb-1.0-0-dev \
    lldb \
    make \
    mingw-w64 \
    ninja-build \
    pkg-config \
    valgrind \
    && rm -rf /var/lib/apt/lists/*
