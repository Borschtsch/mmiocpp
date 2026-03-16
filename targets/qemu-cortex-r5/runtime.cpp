#include "semihosting.hpp"

extern "C" [[noreturn]] void mmiopp_target_exit(int status) {
  mmiopp::qemu::semihosting::exit(status);
}