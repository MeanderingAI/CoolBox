FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

ARG CMAKE_VERSION=4.3.1

RUN apt-get update && apt-get install -y \
    bash \
    bison \
    ca-certificates \
    cargo \
    curl \
    doxygen \
    g++ \
    git \
    golang-go \
    gfortran \
    graphviz \
    libeigen3-dev \
    libgsl-dev \
    libgtest-dev \
    libsqlite3-dev \
    libcurl4-openssl-dev \
    libssl-dev \
    libxml2-dev \
    make \
    maven \
    openjdk-17-jdk \
    pandoc \
    pkg-config \
    python3 \
    python3-pip \
    python3-venv \
    r-base \
    r-base-dev \
    rustc \
    unzip \
    zip \
    && rm -rf /var/lib/apt/lists/*

RUN curl -fsSL -o /tmp/cmake.tar.gz "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz" \
    && tar -xzf /tmp/cmake.tar.gz -C /opt \
    && ln -sf "/opt/cmake-${CMAKE_VERSION}-linux-x86_64/bin/cmake" /usr/local/bin/cmake \
    && ln -sf "/opt/cmake-${CMAKE_VERSION}-linux-x86_64/bin/ctest" /usr/local/bin/ctest \
    && ln -sf "/opt/cmake-${CMAKE_VERSION}-linux-x86_64/bin/cpack" /usr/local/bin/cpack \
    && rm -f /tmp/cmake.tar.gz

WORKDIR /workspace
