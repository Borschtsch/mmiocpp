#include "access_example_registers.hpp"

// Negative test: W1S writes are edge-triggering actions, not OR-safe state
// updates. operator|= must reject them.

int main() {
  ACCESS_LATCH::Instance<0x1000u> accessLatch;
  accessLatch |= ACCESS_LATCH::ENABLED::SET;
  return 0;
}