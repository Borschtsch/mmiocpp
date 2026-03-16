#include "access_example_registers.hpp"

// Negative test: write-only fields may describe commands or payloads, but they
// cannot be read back through the typed API.

int main() {
  ACCESS_COMMAND::Instance<0x1000u> accessCommand;
  (void)accessCommand.get<ACCESS_COMMAND::COUNT>();
  return 0;
}