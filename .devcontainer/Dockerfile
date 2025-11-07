FROM debian:bookworm

# Install base build tools and mingw-w64 toolchain
RUN apt-get update && apt-get install -y \
    build-essential \
    git \
    wget \
    ca-certificates \
    pkg-config \
    make \
    mingw-w64 \
    mingw-w64-tools \
    mingw-w64-x86-64-dev \
    zstd \
    && rm -rf /var/lib/apt/lists/*

# Set up environment for cross-compilation
ENV MINGW_PREFIX=/usr/x86_64-w64-mingw32
ENV PKG_CONFIG_PATH=${MINGW_PREFIX}/lib/pkgconfig
ENV PATH=${MINGW_PREFIX}/bin:$PATH

# Create workspace directory
WORKDIR /workspace

# Copy and run MSYS2 dependencies installer
COPY install-msys2-deps.sh /tmp/
RUN chmod +x /tmp/install-msys2-deps.sh && \
    /tmp/install-msys2-deps.sh && \
    rm /tmp/install-msys2-deps.sh

# Set default shell
CMD ["/bin/bash"]
