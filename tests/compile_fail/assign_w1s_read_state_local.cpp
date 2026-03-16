// Local-register variant: the same misuse must fail for local register values too.
#include "access_example_registers.hpp"

// Negative test: a W1S field exposes separate readable states and write actions.
// The ON state is readable, but only SET is writable.

int main() {
  ACCESS_LATCH accessLatch;
  accessLatch = ACCESS_LATCH::ENABLED::ON;
  return 0;
}