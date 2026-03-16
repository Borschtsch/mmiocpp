// Local-register variant: the same misuse must fail for local register values too.
#include "mmio.hpp"
#include "spi_example_registers.hpp"

// Negative test: raw register writes are intentionally hidden from the public API.
// Callers should write typed encoded values instead of bypassing register-field safety.

int main() {
  SPI_CR spiCr;
  spiCr.write(0);
  return 0;
}
