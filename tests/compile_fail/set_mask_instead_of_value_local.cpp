// Local-register variant: the same misuse must fail for local register values too.
#include "mmio.hpp"
#include "spi_example_registers.hpp"

// Negative test: set(...) writes an encoded value, not a mask.
// A mask answers 'which bits', while set(...) must answer 'which state should be written'.

int main() {
  SPI_CR spiCr;
  spiCr.set(SPI_CR::SPIEN::MASK);
  return 0;
}
