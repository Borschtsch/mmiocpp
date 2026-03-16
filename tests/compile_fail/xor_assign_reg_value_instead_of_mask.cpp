#include "mmio.hpp"
#include "spi_example_registers.hpp"

// Negative test: operator^= toggles bits selected by a mask only.
// Encoded values are data to be written, not masks to be toggled.

int main() {
  SPI_CR::Instance<0x1000u> spiCr;
  spiCr ^= SPI_CR::SPIEN::ENABLE;
  return 0;
}
