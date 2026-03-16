#include "access_example_registers.hpp"

// Negative test: set<Field>(raw) is a masked replacement API, so it requires a
// readable backing register image. Write-only registers must use whole-register
// encoded writes instead.

int main() {
  ACCESS_COMMAND::Instance<0x1000u> accessCommand;
  accessCommand.set<ACCESS_COMMAND::COUNT>(7u);
  return 0;
}