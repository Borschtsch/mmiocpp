#include "access_example_registers.hpp"

// Negative test: a read-only state can be observed but not written back.
// ACCESS_STATUS::READY::ASSERTED is predicate-only, so whole-register writes must reject it.

int main() {
  ACCESS_STATUS::Instance<0x1000u> accessStatus;
  accessStatus = ACCESS_STATUS::READY::ASSERTED;
  return 0;
}