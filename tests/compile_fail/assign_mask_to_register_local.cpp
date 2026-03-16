// Local-register variant: the same misuse must fail for local register values too.
#include "mmio.hpp"
#include "spi_example_registers.hpp"

// Negative test: whole-register assignment requires an encoded register value.
// A mask only selects bits for clear/toggle operations and cannot express a field state.

int main() {
  SPI_CR spiCr;
  spiCr = SPI_CR::SPIEN::MASK;
  return 0;
}
