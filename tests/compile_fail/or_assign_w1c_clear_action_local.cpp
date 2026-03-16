// Local-register variant: the same misuse must fail for local register values too.
#include "access_example_registers.hpp"

// Negative test: W1C actions are whole-write semantics, not OR-safe updates.
// operator|= must reject them to avoid implying a read-modify-write clear.

int main() {
  ACCESS_STATUS accessStatus;
  accessStatus |= ACCESS_STATUS::OVERRUN::CLEAR;
  return 0;
}