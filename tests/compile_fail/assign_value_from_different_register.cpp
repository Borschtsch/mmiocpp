#include "mmio.hpp"
#include "spi_example_registers.hpp"

// Negative test: typed value-field encodings belong to one specific register layout.
// Assigning them to another register would reinterpret the payload at unrelated offsets.

int main() {
  SPI_CR::Instance<0x1000u> spiCr;
  spiCr = SPI_MR::DLY::value(3);
  return 0;
}
