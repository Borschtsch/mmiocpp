#include "mmio.hpp"
#include "spi_example_registers.hpp"

// Negative test: get<Field>() may only decode fields that belong to the target register.
// Reading a foreign field would reinterpret unrelated bits as the wrong logical value.

int main() {
  SPI_CR::Instance<0x1000u> spiCr;
  auto delay = spiCr.get<SPI_MR::DLY>();
  (void)delay;
  return 0;
}
