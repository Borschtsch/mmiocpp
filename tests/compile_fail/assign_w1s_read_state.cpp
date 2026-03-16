#include "access_example_registers.hpp"

// Negative test: a W1S field exposes separate readable states and write actions.
// The ON state is readable, but only SET is writable.

int main() {
  ACCESS_LATCH::Instance<0x1000u> accessLatch;
  accessLatch = ACCESS_LATCH::ENABLED::ON;
  return 0;
}