#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

run_root() {
  if [ "$(id -u)" -eq 0 ]; then
    "$@"
  else
    sudo "$@"
  fi
}

install_macos() {
  if ! command -v brew >/dev/null 2>&1; then
    echo "Homebrew is required on macOS. Install it from https://brew.sh/ and rerun this script." >&2
    exit 1
  fi

  brew bundle --file "$repo_root/Brewfile"
}

install_apt() {
  run_root apt-get update
  run_root apt-get install -y build-essential cmake ninja-build clang lld qemu-system-arm gcc-arm-none-eabi libnewlib-arm-none-eabi
}

install_dnf() {
  run_root dnf install -y gcc-c++ cmake ninja-build clang lld qemu-system-arm arm-none-eabi-gcc-cs arm-none-eabi-gcc-cs-c++ arm-none-eabi-newlib
}

install_pacman() {
  run_root pacman -Sy --needed base-devel cmake ninja clang lld qemu-system-arm arm-none-eabi-gcc arm-none-eabi-newlib
}

install_zypper() {
  run_root zypper install -y gcc-c++ cmake ninja clang lld qemu
  echo "openSUSE does not expose an official Arm bare-metal toolchain package in the default repos; install cross-arm-none-eabi-gcc from the community package page if you want to run the QEMU target tests." >&2
}

case "$(uname -s)" in
  Darwin)
    install_macos
    ;;
  Linux)
    if command -v apt-get >/dev/null 2>&1; then
      install_apt
    elif command -v dnf >/dev/null 2>&1; then
      install_dnf
    elif command -v pacman >/dev/null 2>&1; then
      install_pacman
    elif command -v zypper >/dev/null 2>&1; then
      install_zypper
    else
      echo "Unsupported Linux distribution. Install a C++ compiler, CMake, Ninja, QEMU, and an Arm GNU bare-metal toolchain manually." >&2
      exit 1
    fi
    ;;
  *)
    echo "Unsupported platform: $(uname -s)" >&2
    exit 1
    ;;
 esac

 echo "Bootstrap completed."
 echo "Host build with: cmake -S . -B build/host -G Ninja && cmake --build build/host"
 echo "Host test with: ctest --test-dir build/host --output-on-failure"
 echo "QEMU build with: cmake -S . -B build/qemu -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi-gcc.cmake -DMMIOPP_BUILD_DEMO=OFF -DMMIOPP_BUILD_TESTS=OFF -DMMIOPP_BUILD_QEMU_TARGET=ON && cmake --build build/qemu"
 echo "QEMU test with: ctest --test-dir build/qemu --output-on-failure"
 echo "Cortex-R5 configure/build with: cmake -S . -B build/qemu-r5 -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi-gcc.cmake -DMMIOPP_BUILD_DEMO=OFF -DMMIOPP_BUILD_TESTS=OFF -DMMIOPP_BUILD_QEMU_R5_TARGET=ON && cmake --build build/qemu-r5"
 echo "Cortex-R5 test with: ctest --test-dir build/qemu-r5 --output-on-failure"
