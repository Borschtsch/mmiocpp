#include "mmio.hpp"
#include "spi_example_registers.hpp"

// Negative test: predicates compare against encoded field states, not masks.
// A mask names a bit range but does not mean 'the register currently equals this state'.

int main() {
  SPI_CR::Instance<0x1000u> spiCr;
  const bool invalid = (spiCr & SPI_CR::SPIEN::MASK);
  return invalid ? 1 : 0;
}
