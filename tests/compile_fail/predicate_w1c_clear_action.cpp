#include "access_example_registers.hpp"

// Negative test: the CLEAR action of a W1C field is write-only. Predicate reads
// must use the readable DETECTED/OK states instead.

int main() {
  ACCESS_STATUS::Instance<0x1000u> accessStatus;
  const bool clears = accessStatus & ACCESS_STATUS::OVERRUN::CLEAR;
  return clears ? 0 : 1;
}