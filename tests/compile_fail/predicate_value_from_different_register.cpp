#include "mmio.hpp"
#include "spi_example_registers.hpp"

// Negative test: predicates must compare against values encoded for the same register.
// Foreign encodings would interpret unrelated bits as if they belonged to this register.

int main() {
  SPI_CR::Instance<0x1000u> spiCr;
  const bool invalid = (spiCr & SPI_MR::DLY::value(3));
  return invalid ? 1 : 0;
}
