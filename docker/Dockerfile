FROM ubuntu:26.04

# Setting TZ and Japanese Locale
ENV DEBIAN_FRONTEND=noninteractive \
    LANG=ja_JP.UTF-8 \
    LANGUAGE=ja_JP:ja \
    LC_ALL=ja_JP.UTF-8

# locales のインストールと日本語ロケール生成
RUN apt update && \
    apt-get upgrade -y && \
    apt install -y locales && \
    locale-gen ja_JP.UTF-8 && \
    update-locale LANG=ja_JP.UTF-8

# install
RUN apt update &&\
    apt-get upgrade -y &&\
    apt install -y \
    curl \
    wget \
    tar \
    gettext \
    tree \
    make \
    cmake \
    git \
    gcc \
    g++ \
    gcc-mingw-w64 \
    g++-mingw-w64

# Setup wxWidgets (mingw)
WORKDIR /opt
RUN git clone https://github.com/wxWidgets/wxWidgets.git &&\
    cd wxWidgets &&\
    git checkout v3.2.10 &&\
    git submodule update --init --recursive

# 1. Debug 版のビルド
RUN mkdir -p /opt/wxWidgets/build-debug
WORKDIR /opt/wxWidgets/build-debug
RUN ../configure \
    --prefix=/usr/x86_64-w64-mingw32/debug \
    --host=x86_64-w64-mingw32 \
    --with-msw \
    --disable-shared \
    --enable-unicode \
    --enable-debug \
    && make -j$(nproc) && make install

# 2. Release 版のビルド
RUN mkdir -p /opt/wxWidgets/build-release
WORKDIR /opt/wxWidgets/build-release
RUN ../configure \
    --prefix=/usr/x86_64-w64-mingw32/release \
    --host=x86_64-w64-mingw32 \
    --with-msw \
    --disable-shared \
    --enable-unicode \
    --disable-debug \
    --enable-optimise \
    && make -j$(nproc) && make install

# Setup OpenCV
WORKDIR /opt

RUN git clone https://github.com/opencv/opencv.git -b 4.12.0 --depth 1

RUN git clone https://github.com/opencv/opencv_contrib.git -b 4.12.0 --depth 1

# Build
RUN mkdir -p /opt/opencv/build-mingw
WORKDIR /opt/opencv/build-mingw
RUN cmake .. \
    -DCMAKE_SYSTEM_NAME=Windows \
    -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
    -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
    -DCMAKE_INSTALL_PREFIX=/usr/x86_64-w64-mingw32 \
    -DCMAKE_FIND_ROOT_PATH=/usr/x86_64-w64-mingw32 \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
    -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
    -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_PERF_TESTS=OFF  \
    -DBUILD_TESTS=OFF \
    -DWITH_ADE=OFF \
    -DWITH_OPENJPEG=OFF \
    -DWITH_WEBP=OFF \
    -DWITH_TIFF=OFF \
    -DWITH_QUIRC=OFF \
    -DWITH_OPENEXR=OFF \
    -DWITH_JPEG=OFF \
    -DWITH_LAPACK=OFF \
    -DWITH_JASPER=OFF \
    -DWITH_PNG=OFF \
    -DWITH_GSTREAMER=OFF \
    -DWITH_FFMPEG=OFF \
    -DWITH_EIGEN=OFF \
    -DWITH_DSHOW=OFF \
    -DWITH_DIRECTX=OFF \
    -DWITH_ARITH_DEC=OFF \
    -DWITH_ARITH_ENC=OFF \
    -DWITH_1394=OFF \
    -DWITH_IMGCODEC_HDR=OFF \
    -DWITH_IMGCODEC_PFM=OFF \
    -DWITH_IMGCODEC_PXM=OFF \
    -DWITH_IMGCODEC_SUNRASTER=OFF \
    -DBUILD_OPENJPEG=OFF \
    -DBUILD_JASPER=OFF \
    -DBUILD_JAVA=OFF \
    -DBUILD_JPEG=OFF \
    -DBUILD_OPENEXR=OFF \
    -DBUILD_PNG=OFF \
    -DBUILD_TBB=OFF \
    -DBUILD_opencv_python3=OFF \
    -DOPENCV_EXTRA_MODULES_PATH=/opt/opencv_contrib/modules \
    && make -j$(nproc) \
    && make install