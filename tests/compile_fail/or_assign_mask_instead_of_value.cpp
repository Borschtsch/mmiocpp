#include "mmio.hpp"
#include "spi_example_registers.hpp"

// Negative test: operator|= is reserved for bit-style encoded values, not masks.
// Masks are only meaningful for clear/toggle style operations.

int main() {
  SPI_CR::Instance<0x1000u> spiCr;
  spiCr |= SPI_CR::SPIEN::MASK;
  return 0;
}
