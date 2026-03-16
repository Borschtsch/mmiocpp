#include "mmio.hpp"

// Negative test: asymmetric access fields must define named state(...)
// and action(...) constants explicitly. value(...) would blur the meaning.

struct BAD_W1C : mmio::Register<BAD_W1C> {
  struct FLAG : mmio::BitField<BAD_W1C, 0, 1, mmio::W1c> {
    static constexpr auto BAD = value(1);
  };
};

int main() {
  return 0;
}