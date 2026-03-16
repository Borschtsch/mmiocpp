#include "mmio.hpp"
#include "spi_example_registers.hpp"

// Negative test: set<Field>(raw) is a masked update for a field on this register only.
// Using a field from another register would target the wrong bit slice.

int main() {
  SPI_CR::Instance<0x1000u> spiCr;
  spiCr.set<SPI_MR::DLY>(3);
  return 0;
}
