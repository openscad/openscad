# Builds the Emscripten sysroot for PythonSCAD WASM (Eigen, Boost, CGAL, …).
# Based on https://github.com/openscad/openscad-wasm Dockerfile.base, pinned to
# emscripten/emsdk:6.0.0 so CPython 3.14 and PythonSCAD share one toolchain.
#
# Build:
#   docker build -f docker/wasm/sysroot.dockerfile --target wasm-sysroot \
#     -t pythonscad-wasm-sysroot:local .

ARG EMSCRIPTEN_SDK_TAG=emscripten/emsdk:6.0.0

FROM ${EMSCRIPTEN_SDK_TAG} AS libs-fetch
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates git make patch wget xz-utils \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /ow
RUN git clone --depth 1 https://github.com/openscad/openscad-wasm.git .
# Tarball targets from openscad-wasm Makefile; use mirrors (gmplib.org is often unreachable).
RUN mkdir -p libs \
    && wget -q https://ftp.gnu.org/gnu/gmp/gmp-6.3.0.tar.xz \
    && tar xf gmp-6.3.0.tar.xz -C libs && mv libs/gmp-6.3.0 libs/gmp && rm gmp-6.3.0.tar.xz \
    && wget -q https://www.mpfr.org/mpfr-4.2.1/mpfr-4.2.1.tar.xz \
    && tar xf mpfr-4.2.1.tar.xz -C libs && mv libs/mpfr-4.2.1 libs/mpfr && rm mpfr-4.2.1.tar.xz \
    && wget -q https://github.com/boostorg/boost/releases/download/boost-1.87.0/boost-1.87.0-b2-nodocs.tar.xz \
    && tar xf boost-1.87.0-b2-nodocs.tar.xz -C libs && mv libs/boost-1.87.0 libs/boost \
    && rm boost-1.87.0-b2-nodocs.tar.xz \
    && sed -i -E 's/-fwasm-exceptions/-fexceptions/g' libs/boost/tools/build/src/tools/emscripten.jam
RUN make libs \
    && rm -rf libs/fontconfig \
    && git clone --depth 1 --branch 2.14.2 https://gitlab.freedesktop.org/fontconfig/fontconfig.git libs/fontconfig \
    && awk '/^SUBDIRS=fontconfig fc-case fc-lang src/ { print "SUBDIRS=fontconfig fc-case fc-lang src"; skip=1; next } skip && /^[[:space:]]*its po po-conf test$/ { skip=0; next } skip { next } { print }' libs/fontconfig/Makefile.am > libs/fontconfig/Makefile.am.new \
    && mv libs/fontconfig/Makefile.am.new libs/fontconfig/Makefile.am \
    && sed -i 's/  RUN_FC_CACHE_TEST=test -z "$(DESTDIR)"/  RUN_FC_CACHE_TEST=false/' libs/fontconfig/Makefile.am

FROM ${EMSCRIPTEN_SDK_TAG} AS builder
ARG CMAKE_BUILD_TYPE=Release
ARG MESON_BUILD_TYPE=release
ARG EMSCRIPTEN_FLAGS="-fexceptions -fPIC"
RUN apt-get update && apt-get install -y --no-install-recommends \
    autoconf automake autopoint autotools-dev bison build-essential cmake flex gettext git \
    gperf libltdl-dev libtool ninja-build pkg-config python3 python3-pip python-is-python3 \
    texinfo unzip wget \
    && rm -rf /var/lib/apt/lists/* \
    && pip3 install --break-system-packages meson packaging 'cmake>=3.29'
ENV CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
ENV MESON_BUILD_TYPE=${MESON_BUILD_TYPE}
ENV CFLAGS="${EMSCRIPTEN_FLAGS}"
ENV CXXFLAGS="${EMSCRIPTEN_FLAGS}"
ENV LDFLAGS="${EMSCRIPTEN_FLAGS}"

FROM builder AS boost
COPY --from=libs-fetch /ow/libs/boost .
RUN ./bootstrap.sh && \
    ./b2 \
      --disable-icu \
      --prefix=/emsdk/upstream/emscripten/cache/sysroot \
      --with-filesystem \
      --with-program_options \
      --with-regex \
      --with-system \
      address-model=32 \
      cxxflags="-std=c++17 -stdlib=libc++ -fexceptions -fPIC" \
      install \
      link=static \
      linkflags="-stdlib=libc++ -fexceptions -fPIC" \
      release \
      runtime-link=static \
      toolset=emscripten

FROM builder AS libexpat
COPY --from=libs-fetch /ow/libs/libexpat .
RUN cd expat && \
    ./buildconf.sh && \
    emconfigure ./configure \
        --without-docbook \
        --host wasm32-unknown-linux \
        --prefix=/emsdk/upstream/emscripten/cache/sysroot \
        --enable-shared=no \
        --disable-dependency-tracking && \
    emmake make && \
    emmake make install

FROM builder AS zlib
COPY --from=libs-fetch /ow/libs/zlib .
RUN emcmake cmake \
        -DINSTALL_PKGCONFIG_DIR="/emsdk/upstream/emscripten/cache/sysroot/lib/pkgconfig/" \
        -B ../build && \
    cmake --build ../build && \
    cmake --install ../build

FROM builder AS libzip
COPY --from=zlib /emsdk/upstream/emscripten/cache/sysroot /emsdk/upstream/emscripten/cache/sysroot
COPY --from=libs-fetch /ow/libs/libzip .
RUN emcmake cmake \
        -B ../build \
        -G Ninja \
        -DBUILD_SHARED_LIBS=OFF \
        -DENABLE_SHARED=OFF && \
    cmake --build ../build --parallel && \
    cmake --install ../build

FROM builder AS libffi
COPY --from=libs-fetch /ow/libs/libffi .
RUN ./autogen.sh && \
    emconfigure ./configure \
      --host wasm32-unknown-linux \
      --prefix=/emsdk/upstream/emscripten/cache/sysroot \
      --enable-static \
      --disable-shared \
      --disable-dependency-tracking \
      --disable-builddir \
      --disable-multi-os-directory \
      --disable-raw-api \
      --disable-docs && \
    emmake make && \
    emmake make install SUBDIRS='include'

FROM builder AS glib
COPY --from=zlib /emsdk/upstream/emscripten/cache/sysroot /emsdk/upstream/emscripten/cache/sysroot
COPY --from=libffi /emsdk/upstream/emscripten/cache/sysroot /emsdk/upstream/emscripten/cache/sysroot
COPY --from=libs-fetch /ow/libs/glib .
COPY docker/wasm/emscripten-crossfile.meson emscripten-crossfile.meson
RUN meson \
      setup \
      ../build \
      --prefix=/emsdk/upstream/emscripten/cache/sysroot \
      --cross-file=emscripten-crossfile.meson \
      --default-library=static \
      --buildtype=${MESON_BUILD_TYPE} \
      --force-fallback-for=pcre2,gvdb \
      -Dselinux=disabled \
      -Dxattr=false \
      -Dlibmount=disabled \
      -Dnls=disabled \
      -Dtests=false \
      -Dglib_assert=false \
      -Dglib_checks=false && \
    meson install -C ../build

FROM builder AS freetype
COPY --from=zlib /emsdk/upstream/emscripten/cache/sysroot /emsdk/upstream/emscripten/cache/sysroot
COPY --from=libs-fetch /ow/libs/freetype .
RUN emcmake cmake \
        -B ../build \
        -DFT_REQUIRE_ZLIB=TRUE && \
    cmake --build ../build && \
    cmake --install ../build

FROM builder AS libxml2
COPY --from=libs-fetch /ow/libs/libxml2 .
RUN emcmake cmake \
        -B ../build \
        -G Ninja \
        -DLIBXML2_WITH_PYTHON=OFF \
        -DLIBXML2_WITH_LZMA=OFF \
        -DLIBXML2_WITH_ZLIB=OFF \
        -DLIBXML2_WITH_PROGRAMS=OFF \
        -DLIBXML2_WITH_TESTS=OFF \
        -DBUILD_SHARED_LIBS=OFF && \
    cmake --build ../build --parallel && \
    cmake --install ../build

FROM builder AS fontconfig
COPY --from=zlib /emsdk/upstream/emscripten/cache/sysroot /emsdk/upstream/emscripten/cache/sysroot
COPY --from=freetype /emsdk/upstream/emscripten/cache/sysroot /emsdk/upstream/emscripten/cache/sysroot
COPY --from=libxml2 /emsdk/upstream/emscripten/cache/sysroot /emsdk/upstream/emscripten/cache/sysroot
COPY --from=libs-fetch /ow/libs/fontconfig .
ENV FREETYPE_CFLAGS="-I/emsdk/upstream/emscripten/cache/sysroot/include/freetype2"
ENV FREETYPE_LIBS="-lfreetype -lz"
RUN emconfigure ./autogen.sh \
        --host none \
        --disable-docs \
        --disable-shared \
        --enable-static \
        --sysconfdir=/ \
        --localstatedir=/ \
        --with-default-fonts=/fonts \
        --enable-libxml2 \
        --prefix=/emsdk/upstream/emscripten/cache/sysroot && \
    emmake make && \
    emmake make install

FROM builder AS harfbuzz
COPY --from=freetype /emsdk/upstream/emscripten/cache/sysroot /emsdk/upstream/emscripten/cache/sysroot
COPY --from=libs-fetch /ow/libs/harfbuzz .
RUN emcmake cmake \
        -B ../build \
        -G Ninja \
        -DHB_HAVE_FREETYPE=ON && \
    cmake --build ../build --parallel && \
    cmake --install ../build

FROM builder AS eigen
COPY --from=libs-fetch /ow/libs/eigen .
RUN emcmake cmake \
        -B ../build \
        -G Ninja \
        -DCMAKE_INSTALL_PREFIX=/emsdk/upstream/emscripten/cache/sysroot && \
    cmake --build ../build --parallel && \
    cmake --install ../build && \
    ln -s /emsdk/upstream/emscripten/cache/sysroot/include/eigen3/Eigen \
          /emsdk/upstream/emscripten/cache/sysroot/include/Eigen

FROM builder AS cgal
COPY --from=libs-fetch /ow/libs/cgal .
RUN emcmake cmake \
        -B ../build \
        -G Ninja && \
    cmake --build ../build --parallel && \
    cmake --install ../build

FROM builder AS gmp
COPY --from=libs-fetch /ow/libs/gmp .
RUN emconfigure ./configure \
        --disable-assembly \
        --host none \
        --enable-cxx --prefix=/emsdk/upstream/emscripten/cache/sysroot \
        HOST_CC=gcc && \
    make && \
    make install

FROM builder AS mpfr
COPY --from=gmp /emsdk/upstream/emscripten/cache/sysroot /emsdk/upstream/emscripten/cache/sysroot
COPY --from=libs-fetch /ow/libs/mpfr .
RUN emconfigure ./configure \
        --host none \
        --with-gmp=/emsdk/upstream/emscripten/cache/sysroot \
        --prefix=/emsdk/upstream/emscripten/cache/sysroot && \
    make && \
    make install

FROM builder AS doubleconversion
COPY --from=libs-fetch /ow/libs/doubleconversion .
RUN emcmake cmake \
        -B ../build \
        -G Ninja && \
    cmake --build ../build --parallel && \
    cmake --install ../build

FROM builder AS lib3mf
COPY --from=zlib /emsdk/upstream/emscripten/cache/sysroot /emsdk/upstream/emscripten/cache/sysroot
COPY --from=libzip /emsdk/upstream/emscripten/cache/sysroot /emsdk/upstream/emscripten/cache/sysroot
COPY --from=libs-fetch /ow/libs/lib3mf .
RUN sed -i 's/add_library(${PROJECT_NAME} SHARED/add_library(${PROJECT_NAME} STATIC/' CMakeLists.txt
RUN emcmake cmake \
        -B ../build \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
        -DLIB3MF_TESTS=OFF \
        -DUSE_INCLUDED_ZLIB=OFF \
        -DUSE_INCLUDED_LIBZIP=OFF \
        -DCMAKE_PREFIX_PATH=/emsdk/upstream/emscripten/cache/sysroot \
        -DCMAKE_LIBRARY_PATH=/emsdk/upstream/emscripten/cache/sysroot/lib \
        -G Ninja && \
    cmake --build ../build --parallel && \
    cmake --install ../build

FROM ${EMSCRIPTEN_SDK_TAG} AS wasm-sysroot
RUN embuilder build MINIMAL_PIC --pic
COPY --from=boost /emsdk/upstream/emscripten/cache/sysroot /emsdk/upstream/emscripten/cache/sysroot
COPY --from=gmp /emsdk/upstream/emscripten/cache/sysroot /emsdk/upstream/emscripten/cache/sysroot
COPY --from=mpfr /emsdk/upstream/emscripten/cache/sysroot /emsdk/upstream/emscripten/cache/sysroot
COPY --from=cgal /emsdk/upstream/emscripten/cache/sysroot /emsdk/upstream/emscripten/cache/sysroot
COPY --from=eigen /emsdk/upstream/emscripten/cache/sysroot /emsdk/upstream/emscripten/cache/sysroot
COPY --from=harfbuzz /emsdk/upstream/emscripten/cache/sysroot /emsdk/upstream/emscripten/cache/sysroot
COPY --from=fontconfig /emsdk/upstream/emscripten/cache/sysroot /emsdk/upstream/emscripten/cache/sysroot
COPY --from=glib /emsdk/upstream/emscripten/cache/sysroot /emsdk/upstream/emscripten/cache/sysroot
COPY --from=libzip /emsdk/upstream/emscripten/cache/sysroot /emsdk/upstream/emscripten/cache/sysroot
COPY --from=libffi /emsdk/upstream/emscripten/cache/sysroot /emsdk/upstream/emscripten/cache/sysroot
COPY --from=doubleconversion /emsdk/upstream/emscripten/cache/sysroot /emsdk/upstream/emscripten/cache/sysroot
COPY --from=lib3mf /emsdk/upstream/emscripten/cache/sysroot /emsdk/upstream/emscripten/cache/sysroot
COPY --from=freetype /emsdk/upstream/emscripten/cache/sysroot /emsdk/upstream/emscripten/cache/sysroot
COPY --from=libxml2 /emsdk/upstream/emscripten/cache/sysroot /emsdk/upstream/emscripten/cache/sysroot
RUN apt-get update && apt-get install -y --no-install-recommends \
    bison cmake flex gettext ninja-build pkg-config \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /
