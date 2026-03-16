// Local-register variant: the same misuse must fail for local register values too.
#include "mmio.hpp"
#include "spi_example_registers.hpp"

// Negative test: mask operators may only use masks from the same register definition.
// A foreign mask names unrelated bits and must never be accepted.

int main() {
  SPI_CR spiCr;
  spiCr &= ~SPI_MR::MSTR::MASK;
  return 0;
}
