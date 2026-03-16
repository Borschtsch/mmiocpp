#include "access_example_registers.hpp"

// Negative test: write-only command registers do not support mask-style RMW.
// The full register mask exists for layout purposes only and must not enable &=.

int main() {
  ACCESS_COMMAND::Instance<0x1000u> accessCommand;
  accessCommand &= ~ACCESS_COMMAND::MASK;
  return 0;
}