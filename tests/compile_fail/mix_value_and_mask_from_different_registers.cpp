#include "mmio.hpp"
#include "spi_example_registers.hpp"

// Negative test: masks and encoded values are different semantic categories and must not compose.
// This case is also cross-register, which would additionally mix unrelated layouts.

int main() {
  auto invalid = SPI_MR::DLY::value(3) | SPI_CR::SPIEN::MASK;
  (void)invalid;
  return 0;
}
