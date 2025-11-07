#!/bin/bash
set -e

# MSYS2 mingw64 repository
REPO_URL="https://repo.msys2.org/mingw/mingw64"
INSTALL_PREFIX="/usr/x86_64-w64-mingw32"

echo "Installing MSYS2 mingw64 packages to ${INSTALL_PREFIX}"

# Function to download and extract a package
install_package() {
    local package_name=$1
    echo "Processing package: ${package_name}"

    # Download package database to find latest version
    local pkg_pattern="${package_name}-[0-9]*.pkg.tar.zst"
    local pkg_url=$(wget -qO- "${REPO_URL}/" | grep -oP "${package_name}-[0-9][^\"]*\.pkg\.tar\.zst" | sort -V | tail -1)

    if [ -z "$pkg_url" ]; then
        echo "ERROR: Could not find package ${package_name}"
        return 1
    fi

    echo "  Found: ${pkg_url}"

    # Download package
    wget -q "${REPO_URL}/${pkg_url}" -O "/tmp/${pkg_url}"

    # Extract to install prefix
    tar -xf "/tmp/${pkg_url}" -C "${INSTALL_PREFIX}" --strip-components=1

    # Cleanup
    rm "/tmp/${pkg_url}"

    echo "  Installed: ${package_name}"
}

# Core packages to install (order matters - dependencies first)
PACKAGES=(
    # Base runtime libraries
    "mingw-w64-x86_64-gcc-libs"
    "mingw-w64-x86_64-gmp"
    "mingw-w64-x86_64-expat"
    "mingw-w64-x86_64-gettext-runtime"
    "mingw-w64-x86_64-libiconv"
    "mingw-w64-x86_64-zlib"
    "mingw-w64-x86_64-libffi"
    "mingw-w64-x86_64-pcre2"
    "mingw-w64-x86_64-bzip2"

    # Crypto and network dependencies (for curl)
    "mingw-w64-x86_64-openssl"
    "mingw-w64-x86_64-libssh2"
    "mingw-w64-x86_64-libidn2"
    "mingw-w64-x86_64-brotli"
    "mingw-w64-x86_64-zstd"
    "mingw-w64-x86_64-nghttp2"
    "mingw-w64-x86_64-nghttp3"
    "mingw-w64-x86_64-ngtcp2"
    "mingw-w64-x86_64-libpsl"

    # Image format libraries (for gdk-pixbuf)
    "mingw-w64-x86_64-libjpeg-turbo"
    "mingw-w64-x86_64-libtiff"

    # Font rendering dependencies
    "mingw-w64-x86_64-graphite2"

    # GLib and Cairo stack
    "mingw-w64-x86_64-glib2"
    "mingw-w64-x86_64-cairo"
    "mingw-w64-x86_64-fontconfig"
    "mingw-w64-x86_64-freetype"
    "mingw-w64-x86_64-fribidi"
    "mingw-w64-x86_64-harfbuzz"
    "mingw-w64-x86_64-libpng"
    "mingw-w64-x86_64-pango"
    "mingw-w64-x86_64-pixman"
    "mingw-w64-x86_64-gdk-pixbuf2"
    "mingw-w64-x86_64-atk"

    # GTK3 and application dependencies
    "mingw-w64-x86_64-gtk3"
    "mingw-w64-x86_64-fftw"
    "mingw-w64-x86_64-libusb"
    "mingw-w64-x86_64-curl"
    "mingw-w64-x86_64-portaudio"
)

# Install each package
for package in "${PACKAGES[@]}"; do
    install_package "$package"
done

# Create pkg-config wrapper for cross-compilation
cat > /usr/local/bin/x86_64-w64-mingw32-pkg-config << 'EOF'
#!/bin/bash
export PKG_CONFIG_PATH=/usr/x86_64-w64-mingw32/lib/pkgconfig
exec pkg-config "$@"
EOF

chmod +x /usr/local/bin/x86_64-w64-mingw32-pkg-config

echo "MSYS2 packages installed successfully"
echo "Installed to: ${INSTALL_PREFIX}"
