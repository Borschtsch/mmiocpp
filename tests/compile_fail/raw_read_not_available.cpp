#include "mmio.hpp"
#include "spi_example_registers.hpp"

// Negative test: raw register reads are intentionally hidden from the public API.
// Callers should use typed predicates or get<Field>() instead of bypassing field safety.

int main() {
  SPI_CR::Instance<0x1000u> spiCr;
  const auto raw = spiCr.read();
  (void)raw;
  return 0;
}
