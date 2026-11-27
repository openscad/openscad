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
# Qt
# -----------------------------------------------------------------------------
FROM base AS qt

ARG QT_VERSION=6.10.3
ARG AQTINSTALL_VERSION=3.3.0
ARG QSCINTILLA_VERSION=2.14.1

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        libb2-1 \
        libdbus-1-3 \
        libegl1 \
        libfontconfig1 \
        libfreetype6 \
        libgl1 \
        libgl1-mesa-dev \
        libglib2.0-0 \
        libglu1-mesa-dev \
        libharfbuzz0b \
        libmd4c0 \
        libpcre2-16-0 \
        libpng16-16 \
        libpulse0 \
        libssl3 \
        libwayland-cursor0 \
        libwayland-egl1 \
        libx11-6 \
        libx11-xcb1 \
        libxcb-icccm4 \
        libxcb-image0 \
        libxcb-keysyms1 \
        libxcb-randr0 \
        libxcb-render-util0 \
        libxcb-shape0 \
        libxcb-sync1 \
        libxcb-xfixes0 \
        libxcb-xinerama0 \
        libxcb-xkb1 \
        libxcb-cursor0 \
        libxkbcommon-x11-0 \
        libxrandr2 \
        libzstd1 \
        python3-pip \
        && rm -rf /var/lib/apt/lists/*

ENV QT_ROOT=/opt/Qt/${QT_VERSION}/gcc_64
ENV PATH=${QT_ROOT}/bin:${PATH}
ENV CMAKE_PREFIX_PATH=${QT_ROOT}

RUN pip3 install --no-cache-dir aqtinstall==${AQTINSTALL_VERSION} && \
    aqt install-qt linux desktop ${QT_VERSION} linux_gcc_64 \
        -m qt5compat qtmultimedia -O /opt/Qt

RUN echo "${QT_ROOT}/lib" > /etc/ld.so.conf.d/qt.conf && ldconfig

WORKDIR /tmp/qscintilla
RUN wget -q "https://www.riverbankcomputing.com/static/Downloads/QScintilla/${QSCINTILLA_VERSION}/QScintilla_src-${QSCINTILLA_VERSION}.tar.gz" && \
    tar xf "QScintilla_src-${QSCINTILLA_VERSION}.tar.gz" && \
    cd "QScintilla_src-${QSCINTILLA_VERSION}/src" && \
    qmake && \
    make -j$(nproc) && \
    make install

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
FROM qt AS appimage-base

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
        software-properties-common && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /openscad

RUN --mount=type=bind,source=scripts/uni-get-dependencies.sh,target=./uni-get-dependencies.sh \
    ./uni-get-dependencies.sh no-qt && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

RUN ln -sf ${QT_ROOT}/bin/qmake /usr/bin/qmake6

COPY --from=lib3mf-builder /inst /usr/
COPY --from=cgal-builder /inst /usr/
RUN ldconfig

COPY --from=deploy-tools /appimage /appimage

RUN --mount=type=bind,source=scripts/check-dependencies.sh,target=./check-dependencies.sh \
    ./check-dependencies.sh
