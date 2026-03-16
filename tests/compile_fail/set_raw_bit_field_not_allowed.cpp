#include "mmio.hpp"
#include "spi_example_registers.hpp"

// Negative test: set<Field>(raw) is reserved for value fields only.
// Bit fields must be updated through their named encoded values.

int main() {
  SPI_MR::Instance<0x1000u> spiMr;
  spiMr.set<SPI_MR::MSTR>(1);
  return 0;
}
