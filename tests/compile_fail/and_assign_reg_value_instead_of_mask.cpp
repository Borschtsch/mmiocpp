#include "mmio.hpp"
#include "spi_example_registers.hpp"

// Negative test: operator&= is a mask operation, not a value write.
// Encoded values represent logical field states, not bit-selection masks.

int main() {
  SPI_CR::Instance<0x1000u> spiCr;
  spiCr &= SPI_CR::SPIEN::ENABLE;
  return 0;
}
