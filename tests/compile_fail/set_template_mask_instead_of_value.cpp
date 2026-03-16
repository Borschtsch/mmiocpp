#include "mmio.hpp"
#include "spi_example_registers.hpp"

// Negative test: set(...) writes an encoded value, not a mask.
// A mask answers 'which bits', while set(...) must answer 'which state should be written'.

int main() {
  SPI_CR::Instance<0x1000u> spiCr;
  spiCr.set<SPI_CR::SPIEN::MASK>();
  return 0;
}
