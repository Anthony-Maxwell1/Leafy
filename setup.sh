#!/usr/bin/env bash
# RUN THIS ONCE AFTER CLONING REPO
set -e

# =========================
# Args
# =========================
FIRMWARE="$1"
PREBUILT="${2:-true}"  # default true

if [ -z "$FIRMWARE" ]; then
  echo "Usage: ./setup.sh {firmware} {prebuilt=true|false}"
  exit 1
fi

ROOT_DIR="$(pwd)"
HOME_DIR="$HOME"
XTOOLS_DIR="$HOME_DIR/x-tools"

echo "Firmware: $FIRMWARE"
echo "Use prebuilt toolchain: $PREBUILT"

# =========================
# 0. Install dependencies
# =========================
install_deps() {
  if command -v apt-get >/dev/null 2>&1; then
    sudo apt-get update
    sudo apt-get install -y \
      build-essential autoconf automake bison flex gawk \
      libtool libtool-bin libncurses-dev curl file git gperf \
      help2man texinfo unzip wget meson
  elif command -v pacman >/dev/null 2>&1; then
    sudo pacman -Sy --needed --noconfirm \
      base-devel autoconf automake bison flex gawk \
      libtool ncurses curl file git gperf help2man \
      texinfo unzip wget meson
  else
    echo "Unsupported package manager. Install dependencies manually."
    exit 1
  fi
}

install_deps

# =========================
# 1-3. Toolchain (prebuilt)
# =========================
if [ "$PREBUILT" = "true" ]; then
  if [ -d "$XTOOLS_DIR" ]; then
    echo "x-tools already exists, skipping download."
  else
    echo "Downloading prebuilt toolchain..."
    wget "https://github.com/koreader/koxtoolchain/releases/download/2025.05/${FIRMWARE}.tar.gz"

    echo "Extracting..."
    tar -xzf "${FIRMWARE}.tar.gz"

    # Move x-tools to HOME if needed
    if [ -d "x-tools" ]; then
      mv x-tools "$XTOOLS_DIR"
    fi
  fi
fi

# =========================
# 4-6. Build toolchain manually
# =========================
if [ "$PREBUILT" = "false" ]; then
  echo "Building toolchain from source..."

  rm -rf "$XTOOLS_DIR"

  git clone https://github.com/koreader/koxtoolchain
  cd koxtoolchain
  ./gen-tc.sh "$FIRMWARE"
  cd "$ROOT_DIR"
fi

# =========================
# 3.5 Ensure PATH
# =========================
export PATH="$XTOOLS_DIR/bin:$PATH"

if ! echo "$PATH" | grep -q "x-tools"; then
  echo "Failed to add x-tools to PATH"
  exit 1
fi

# =========================
# 8. Clone Kindle SDK
# =========================
if [ ! -d "kindle-sdk" ]; then
  git clone --recursive --depth=1 https://github.com/KindleModding/kindle-sdk.git
fi

cd kindle-sdk

# =========================
# 9. Generate SDK
# =========================
SDK_OUTPUT=$(./gen-sdk.sh "$FIRMWARE")

echo "$SDK_OUTPUT"

# =========================
# 10. Extract cross-file path
# =========================
# grab second last commented line
CROSS_FILE=$(echo "$SDK_OUTPUT" | grep '^#' | tail -n 2 | head -n 1 | sed 's/^# //')

if [ ! -f "$CROSS_FILE" ]; then
  echo "Failed to detect cross file path"
  exit 1
fi

echo "Detected cross file: $CROSS_FILE"

cd "$ROOT_DIR"

# =========================
# 11. Meson setup
# =========================
meson setup --cross-file "$CROSS_FILE" "builddir_${FIRMWARE}" || true

# =========================
# 12. Replace Lua Makefile
# =========================
cp lua_mod/Makefile src/external/lua/Makefile

# =========================
# 13. Build Lua
# =========================
cd src/external/lua

make \
  CC=arm-$FIRMWARE-linux-gnueabihf-gcc \
  AR=arm-$FIRMWARE-linux-gnueabihf-ar \
  RANLIB=arm-$FIRMWARE-linux-gnueabihf-ranlib

make install

cd "$ROOT_DIR/src/external/freetype"
./autogen.sh
./configure   --host=arm-$FIRMWARE-linux-gnueabihf   --prefix=$PWD/build   --without-harfbuzz   --without-bzip2   --without-png   --without-zlib

# =========================
# Final build
# =========================
meson compile -C "builddir_${FIRMWARE}"

# =========================
# Done
# =========================
echo "First build done!"
