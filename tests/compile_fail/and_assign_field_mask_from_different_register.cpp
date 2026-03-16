#include "mmio.hpp"
#include "spi_example_registers.hpp"

// Negative test: mask operators may only use masks from the same register definition.
// A foreign mask names unrelated bits and must never be accepted.

int main() {
  SPI_CR::Instance<0x1000u> spiCr;
  spiCr &= ~SPI_MR::MSTR::MASK;
  return 0;
}
