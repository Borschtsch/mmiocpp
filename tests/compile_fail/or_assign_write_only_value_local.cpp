// Local-register variant: the same misuse must fail for local register values too.
#include "access_example_registers.hpp"

// Negative test: operator|= is only valid for OR-safe readable bit states.
// A write-only command cannot participate in a read-modify-write update.

int main() {
  ACCESS_COMMAND accessCommand;
  accessCommand |= ACCESS_COMMAND::START::TRIGGER;
  return 0;
}