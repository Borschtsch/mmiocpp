// Local-register variant: the same misuse must fail for local register values too.
#include "access_example_registers.hpp"

// Negative test: write-only command registers do not support mask-style RMW.
// The full register mask exists for layout purposes only and must not enable &=.

int main() {
  ACCESS_COMMAND accessCommand;
  accessCommand &= ~ACCESS_COMMAND::MASK;
  return 0;
}