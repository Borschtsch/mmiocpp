// Local-register variant: the same misuse must fail for local register values too.
#include "access_example_registers.hpp"

// Negative test: set<Field>(raw) is only valid for value fields whose access
// policy allows masked replacement. A read-only count field cannot be updated.

int main() {
  ACCESS_STATUS accessStatus;
  accessStatus.set<ACCESS_STATUS::COUNT>(3u);
  return 0;
}