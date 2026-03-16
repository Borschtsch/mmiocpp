// Local-register variant: the same misuse must fail for local register values too.
#include "access_example_registers.hpp"

// Negative test: W1S writes are edge-triggering actions, not OR-safe state
// updates. operator|= must reject them.

int main() {
  ACCESS_LATCH accessLatch;
  accessLatch |= ACCESS_LATCH::ENABLED::SET;
  return 0;
}