#include "access_example_registers.hpp"

// Negative test: operator|= is only valid for OR-safe readable bit states.
// A write-only command cannot participate in a read-modify-write update.

int main() {
  ACCESS_COMMAND::Instance<0x1000u> accessCommand;
  accessCommand |= ACCESS_COMMAND::START::TRIGGER;
  return 0;
}