#ifndef MMIOCPP_QEMU_CORTEX_R5_SEMIHOSTING_HPP
#define MMIOCPP_QEMU_CORTEX_R5_SEMIHOSTING_HPP

#include <cstdint>

namespace mmiocpp::qemu::semihosting {

constexpr std::uint32_t kSysOpen = 0x01u;
constexpr std::uint32_t kSysClose = 0x02u;
constexpr std::uint32_t kSysWrite0 = 0x04u;
constexpr std::uint32_t kSysWrite = 0x05u;
constexpr std::uint32_t kSysExitExtended = 0x20u;
constexpr std::uint32_t kApplicationExit = 0x20026u;
constexpr std::uintptr_t kInvalidHandle = static_cast<std::uintptr_t>(-1);
constexpr std::uint32_t kOpenWriteBinary = 5u;

struct OpenBlock {
  const char* path;
  std::uintptr_t mode;
  std::uintptr_t length;
};

struct WriteBlock {
  std::uintptr_t handle;
  const void* buffer;
  std::uintptr_t length;
};

inline std::uintptr_t call(std::uint32_t operation, const void* argument) {
  register std::uintptr_t r0 __asm("r0") = operation;
  register std::uintptr_t r1 __asm("r1") = reinterpret_cast<std::uintptr_t>(argument);
  __asm volatile("svc 0x123456" : "+r"(r0) : "r"(r1) : "memory");
  return r0;
}

inline std::uintptr_t string_length(const char* text) {
  std::uintptr_t length = 0u;
  while (text[length] != '\0') {
    ++length;
  }
  return length;
}

inline void write0(const char* text) {
  (void)call(kSysWrite0, text);
}

inline std::uintptr_t open_write_binary(const char* path) {
  if (path == nullptr) {
    return kInvalidHandle;
  }

  const OpenBlock block{path, kOpenWriteBinary, string_length(path)};
  return call(kSysOpen, &block);
}

inline bool write(std::uintptr_t handle, const void* buffer, std::uintptr_t length) {
  if (handle == kInvalidHandle) {
    return false;
  }

  auto remaining = length;
  auto cursor = static_cast<const std::uint8_t*>(buffer);
  while (remaining > 0u) {
    const WriteBlock block{handle, cursor, remaining};
    const auto unwritten = call(kSysWrite, &block);
    if (unwritten > remaining) {
      return false;
    }

    const auto written = remaining - unwritten;
    if (written == 0u) {
      return false;
    }

    cursor += written;
    remaining = unwritten;
  }

  return true;
}

inline bool close(std::uintptr_t handle) {
  if (handle == kInvalidHandle) {
    return false;
  }

  const std::uintptr_t block = handle;
  return static_cast<std::intptr_t>(call(kSysClose, &block)) == 0;
}

[[noreturn]] inline void exit(int status) {
  const std::uint32_t block[2] = {kApplicationExit, static_cast<std::uint32_t>(status)};
  (void)call(kSysExitExtended, block);
  while (true) {
  }
}

}  // namespace mmiocpp::qemu::semihosting

#endif  // MMIOCPP_QEMU_CORTEX_R5_SEMIHOSTING_HPP
