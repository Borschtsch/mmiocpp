#include "mmio.hpp"
#include "spi_example_registers.hpp"

// Negative test: set(...) and set<V>() accept encoded values only from the same register definition.
// Cross-register values must fail so unrelated layouts cannot be composed by accident.

int main() {
  SPI_CR::Instance<0x1000u> spiCr;
  spiCr.set(SPI_MR::DLY::value(3));
  return 0;
}
