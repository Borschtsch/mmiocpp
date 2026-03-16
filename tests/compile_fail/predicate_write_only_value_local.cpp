// Local-register variant: the same misuse must fail for local register values too.
#include "access_example_registers.hpp"

// Negative test: a write-only command encodes what should be placed on the bus.
// It is not a readable state, so predicate checks must reject it.

int main() {
  ACCESS_COMMAND accessCommand;
  const bool triggered = accessCommand & ACCESS_COMMAND::START::TRIGGER;
  return triggered ? 0 : 1;
}