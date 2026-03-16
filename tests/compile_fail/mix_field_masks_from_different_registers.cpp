#include "mmio.hpp"
#include "spi_example_registers.hpp"

// Negative test: masks from different registers must never be combined.
// They describe different layouts, so the resulting bit set would have no valid meaning.

int main() {
  auto invalid = SPI_CR::SPIEN::MASK | SPI_MR::MSTR::MASK;
  (void)invalid;
  return 0;
}
