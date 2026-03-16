// Local-register variant: the same misuse must fail for local register values too.
#include "access_example_registers.hpp"

// Negative test: the SET action of a W1S field is not a readable state.
// Predicate checks must use OFF/ON instead.

int main() {
  ACCESS_LATCH accessLatch;
  const bool setAction = accessLatch & ACCESS_LATCH::ENABLED::SET;
  return setAction ? 0 : 1;
}