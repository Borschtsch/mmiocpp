#include "mmio.hpp"
#include "spi_example_registers.hpp"

// Negative test: encoded bit values carry their owning register definition.
// Assigning one to another register must fail instead of silently reusing unrelated bit positions.

int main() {
  SPI_CR::Instance<0x1000u> spiCr;
  spiCr = SPI_MR::MSTR::MASTER;
  return 0;
}
