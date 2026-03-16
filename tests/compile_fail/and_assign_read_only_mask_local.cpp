// Local-register variant: the same misuse must fail for local register values too.
#include "access_example_registers.hpp"

// Negative test: read-only masks still identify bit locations, but they cannot
// be used with operator&= because the register does not support clear-by-mask.

int main() {
  ACCESS_STATUS accessStatus;
  accessStatus &= ~ACCESS_STATUS::READY::MASK;
  return 0;
}