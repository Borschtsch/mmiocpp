// Local-register variant: the same misuse must fail for local register values too.
#include "access_example_registers.hpp"

// Negative test: write-only fields may describe commands or payloads, but they
// cannot be read back through the typed API.

int main() {
  ACCESS_COMMAND accessCommand;
  (void)accessCommand.get<ACCESS_COMMAND::COUNT>();
  return 0;
}