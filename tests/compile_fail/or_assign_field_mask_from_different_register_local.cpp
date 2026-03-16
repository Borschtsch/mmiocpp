// Local-register variant: the same misuse must fail for local register values too.
#include "mmio.hpp"
#include "spi_example_registers.hpp"

// Negative test: register-modifying operators must reject operands from another register definition.
// Reusing foreign encodings or masks would couple unrelated register layouts.

int main() {
  SPI_CR spiCr;
  spiCr |= SPI_MR::MSTR::MASK;
  return 0;
}
