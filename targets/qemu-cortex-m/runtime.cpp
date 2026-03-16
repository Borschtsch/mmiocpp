#include "semihosting.hpp"

extern "C" [[noreturn]] void mmiopp_target_exit(int status) {
  mmiopp::qemu::semihosting::exit(status);
}

extern "C" [[noreturn]] void Default_Handler() {
  mmiopp::qemu::semihosting::write0("Unhandled exception in QEMU target test\n");
  mmiopp::qemu::semihosting::exit(1);
}