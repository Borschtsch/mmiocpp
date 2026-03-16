#include "mmio.hpp"
#include "spi_example_registers.hpp"

// Negative test: masks and encoded values are different semantic categories and must not compose.
// A mask answers 'which bits', while an encoded value answers 'which logical state'.

int main() {
  auto invalid = SPI_CR::SPIEN::ENABLE | SPI_CR::SWRST::MASK;
  (void)invalid;
  return 0;
}
