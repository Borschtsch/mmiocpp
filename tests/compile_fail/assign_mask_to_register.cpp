#include "mmio.hpp"
#include "spi_example_registers.hpp"

// Negative test: whole-register assignment requires an encoded register value.
// A mask only selects bits for clear/toggle operations and cannot express a field state.

int main() {
  SPI_CR::Instance<0x1000u> spiCr;
  spiCr = SPI_CR::SPIEN::MASK;
  return 0;
}
