#include "mmio.hpp"
#include "spi_example_registers.hpp"

// Negative test: bit-field raw encoders are definition-time helpers only.
// Application code must use the named encoded states such as ENABLE/DISABLE.

int main() {
  const auto encoded = SPI_CR::SPIEN::value(1);
  (void)encoded;
  return 0;
}
