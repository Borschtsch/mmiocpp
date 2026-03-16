#include "mmio.hpp"
#include "spi_example_registers.hpp"

// Negative test: encoded values may only compose when they belong to the same register definition.
// Cross-register composition would silently combine unrelated bit layouts.

int main() {
  auto invalid = SPI_CR::SPIEN::ENABLE | SPI_MR::PCS::value(2);
  (void)invalid;
  return 0;
}
