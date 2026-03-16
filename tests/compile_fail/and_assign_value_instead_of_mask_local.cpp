// Local-register variant: the same misuse must fail for local register values too.
#include "mmio.hpp"
#include "spi_example_registers.hpp"

// Negative test: operator&= is a mask operation, not a value write.
// Encoded values represent logical field states, not bit-selection masks.

int main() {
  SPI_MR spiMr;
  spiMr &= SPI_MR::DLY::value(3);
  return 0;
}
