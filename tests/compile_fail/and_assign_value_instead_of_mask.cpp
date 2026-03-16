#include "mmio.hpp"
#include "spi_example_registers.hpp"

// Negative test: operator&= is a mask operation, not a value write.
// Encoded values represent logical field states, not bit-selection masks.

int main() {
  SPI_MR::Instance<0x1000u> spiMr;
  spiMr &= SPI_MR::DLY::value(3);
  return 0;
}
