#include "mmio.hpp"
#include "spi_example_registers.hpp"

// Negative test: register-modifying operators must reject operands from another register definition.
// Reusing foreign encodings or masks would couple unrelated register layouts.

int main() {
  SPI_CR::Instance<0x1000u> spiCr;
  spiCr |= SPI_MR::MSTR::MASK;
  return 0;
}
