// Local-register variant: the same misuse must fail for local register values too.
#include "mmio.hpp"
#include "spi_example_registers.hpp"

// Negative test: operator^= toggles bits selected by a mask only.
// Encoded values are data to be written, not masks to be toggled.

int main() {
  SPI_MR spiMr;
  spiMr ^= SPI_MR::DLY::value(3);
  return 0;
}
