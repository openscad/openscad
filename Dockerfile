# docker buildx build --platform linux/amd64,linux/arm64,linux/riscv64 -t gounthar/openscad:latest --output type=local,dest=./output .
# Use the slim version of Debian Bookworm as the base image
FROM debian:unstable-20250428-slim

# Install necessary packages and libraries
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    coreutils \
    curl \
    git \
    gnupg \
    imagemagick \
    python3 \
    python3-pip \
    python3-venv \
    rsync \
    unzip \
    xauth \
    xvfb \
    cmake \
    build-essential \
    && ln -s /usr/bin/python3 /usr/bin/python \
    && python3 -m venv /tmp/venv \
    && /tmp/venv/bin/pip install colorama codespell markdown \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

COPY . /openscad

# Update submodules,  install dependencies, and build OpenSCAD
RUN cd /openscad && git submodule update --init --recursive \
    && ./scripts/uni-get-dependencies.sh && ./scripts/check-dependencies.sh \
    && cmake -B build -DEXPERIMENTAL=1 \
    && cmake --build build -j$(nproc) \
    && cmake --install build --prefix /usr/local \
    && mkdir -p /output \
    && cp /usr/local/bin/openscad /output/

# Define the entrypoint to produce binaries
ENTRYPOINT ["/usr/local/bin/openscad"]
