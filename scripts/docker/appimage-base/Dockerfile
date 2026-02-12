# -----------------------------------------------------------------------------
# Base image
# -----------------------------------------------------------------------------
FROM ubuntu:22.04 AS base
ENV DEBIAN_FRONTEND=noninteractive
ENV CMAKE_GENERATOR=Ninja

# Install only the absolute bare minimum build tools used by almost everyone
# (wget/curl/ca-certificates for downloading, build-essential for compiling)
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential \
        ca-certificates \
        cmake \
        curl \
        git \
        gnupg \
        ninja-build \
        wget \
        file \
        unzip \
        && rm -rf /var/lib/apt/lists/*

# -----------------------------------------------------------------------------
# Lib3MF
# -----------------------------------------------------------------------------
FROM base AS lib3mf-builder

# Install Lib3MF specific dependencies
RUN apt-get update && \
    apt-get install -y --no-install-recommends pkg-config libzip-dev && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

ARG LIB3MF_VERSION=2.4.1
ARG LIB3MF_URL=https://github.com/3MFConsortium/lib3mf/archive/refs/tags

WORKDIR /tmp/build

RUN wget -q "${LIB3MF_URL}/v${LIB3MF_VERSION}.tar.gz" -O lib3mf.tar.gz && \
    mkdir lib3mf && \
    tar -xzf lib3mf.tar.gz -C lib3mf --strip-components=1 && \
    rm lib3mf.tar.gz

WORKDIR /tmp/build/lib3mf/build

RUN cmake .. \
    -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DLIB3MF_TESTS=OFF \
    -DUSE_INCLUDED_ZLIB=OFF \
    -DUSE_INCLUDED_LIBZIP=OFF \
    -DUSE_INCLUDED_SSL=OFF \
    -DCMAKE_INSTALL_PREFIX=/inst \
    -DCMAKE_INSTALL_LIBDIR=lib && \
    cmake --build . --config Release && \
    cmake --install .

# -----------------------------------------------------------------------------
# CGAL
# -----------------------------------------------------------------------------
FROM base AS cgal-builder

# Install CGAL specific dependencies
# CGAL often needs GMP/MPFR (though sometimes header-only, full builds need them)
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        libgmp-dev \
        libboost-dev \
        libmpfr-dev && \
    rm -rf /var/lib/apt/lists/*

ARG CGAL_URL=https://github.com/CGAL/cgal/releases/download
ARG CGAL_VERSION=6.1

WORKDIR /tmp/build

RUN wget -q ${CGAL_URL}/v${CGAL_VERSION}/CGAL-${CGAL_VERSION}-library.tar.xz -O cgal.tar.xz && \
    mkdir cgal && \
    tar -xf cgal.tar.xz -C cgal --strip-components=1 && \
    rm cgal.tar.xz

WORKDIR /tmp/build/cgal/build

RUN cmake .. \
    -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCGAL_HEADER_ONLY=OFF \
    -DBUILD_TESTING=OFF \
    -DBUILD_DOC=OFF \
    -DCMAKE_INSTALL_PREFIX=/inst \
    -DCMAKE_INSTALL_LIBDIR=lib && \
    cmake --build . --config Release && \
    cmake --install .

# -----------------------------------------------------------------------------
# QScintilla
# -----------------------------------------------------------------------------
FROM base AS qscintilla-builder

# Install Qt5 dependencies specifically for QScintilla
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        qmake6 \
        qt6-base-dev \
        libqt6svg6-dev \
        && rm -rf /var/lib/apt/lists/*

WORKDIR /build
RUN wget -q https://www.riverbankcomputing.com/static/Downloads/QScintilla/2.14.1/QScintilla_src-2.14.1.tar.gz && \
    tar xvf QScintilla_src-2.14.1.tar.gz

WORKDIR /build/QScintilla_src-2.14.1/src
RUN qmake6 && \
    make -j$(nproc) && \
    make install INSTALL_ROOT=/inst

# -----------------------------------------------------------------------------
# AppImage Tooling (LinuxDeploy)
# -----------------------------------------------------------------------------
FROM base AS deploy-tools

# Dependencies for extracting AppImages and running linuxdeploy scripts
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        file \
        unzip \
        python3-dev \
        && rm -rf /var/lib/apt/lists/*

WORKDIR /appimage

RUN ld_arch="${TARGETARCH:-$(uname -m)}" && \
    wget -q "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-${ld_arch}.AppImage" && \
    wget -q "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-${ld_arch}.AppImage" && \
    wget -q https://github.com/t-paul/linuxdeploy-plugin-python/archive/refs/heads/master.zip -O linuxdeploy-plugin-python.zip && \
    wget -q https://raw.githubusercontent.com/linuxdeploy/linuxdeploy-plugin-gtk/master/linuxdeploy-plugin-gtk.sh

RUN set -e; \
    ld_arch="${TARGETARCH:-$(uname -m)}"; \
    chmod +x linuxdeploy*.AppImage *.sh && \
    "./linuxdeploy-${ld_arch}.AppImage" --appimage-extract && \
    "./linuxdeploy-plugin-qt-${ld_arch}.AppImage" --appimage-extract && \
    mkdir -p usr/bin/share && \
    mv -iv squashfs-root/usr/bin/* usr/bin && \
    mv -iv squashfs-root/plugins . && \
    rm -rf squashfs-root && \
    unzip -p linuxdeploy-plugin-python.zip linuxdeploy-plugin-python-master/linuxdeploy-plugin-python.sh > usr/bin/linuxdeploy-plugin-python.sh && \
    unzip -p linuxdeploy-plugin-python.zip linuxdeploy-plugin-python-master/share/sitecustomize.py > usr/bin/sitecustomize.py && \
    chmod 755 /appimage/usr/bin/linuxdeploy-plugin-python.sh && \
    rm -rf *.AppImage *.zip

# -----------------------------------------------------------------------------
# Appimage builder setup
# -----------------------------------------------------------------------------
FROM base AS appimage-base

# Install Runtime/Link-time dependencies required for the final OpenSCAD aggregation
# These are things that OpenSCAD itself needs to compile or run the dependency scripts.
RUN apt-get update && apt-get -y full-upgrade && \
    apt-get install -y --no-install-recommends \
        appstream \
        apt-transport-https \
        apt-utils \
        libbz2-dev \
        liblzma-dev \
        libsqlite3-dev \
        libssl-dev \
        libgmp-dev \
        lsb-release \
        ncat \
        nettle-dev \
        openssh-client \
        python3-dev \
	qmake6 \
        software-properties-common && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /openscad

RUN --mount=type=bind,source=scripts/get-dependencies.py,target=./get-dependencies.py \
    --mount=type=bind,source=scripts/deps,target=./deps \
    ./get-dependencies.py --yes --profile pythonscad-qt6 && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

COPY --from=lib3mf-builder /inst /usr/
COPY --from=cgal-builder /inst /usr/
COPY --from=qscintilla-builder /inst/usr /usr/
RUN ldconfig

COPY --from=deploy-tools /appimage /appimage

RUN --mount=type=bind,source=scripts/check-dependencies.sh,target=./check-dependencies.sh \
    ./check-dependencies.sh
