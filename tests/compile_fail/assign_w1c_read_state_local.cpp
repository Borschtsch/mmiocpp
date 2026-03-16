// Local-register variant: the same misuse must fail for local register values too.
#include "access_example_registers.hpp"

// Negative test: the read-side state of a W1C field reports hardware status.
// It is not a write action, so whole-register assignment must reject it.

int main() {
  ACCESS_STATUS accessStatus;
  accessStatus = ACCESS_STATUS::OVERRUN::DETECTED;
  return 0;
}