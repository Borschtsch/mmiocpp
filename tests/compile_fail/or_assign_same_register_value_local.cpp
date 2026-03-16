// Local-register variant: the same misuse must fail for local register values too.
#include "mmio.hpp"
#include "spi_example_registers.hpp"

// Negative test: operator|= cannot update value fields, even on the same register.
// OR can only force bits high; it cannot replace a field value that may require zero bits.

int main() {
  SPI_CR spiCr;
  spiCr |= SPI_CR::CMD::value(1);
  return 0;
}
