// Local-register variant: the same misuse must fail for local register values too.
#include "access_example_registers.hpp"

// Negative test: set<Field>(raw) is a masked replacement API, so it requires a
// readable backing register image. Write-only registers must use whole-register
// encoded writes instead.

int main() {
  ACCESS_COMMAND accessCommand;
  accessCommand.set<ACCESS_COMMAND::COUNT>(7u);
  return 0;
}