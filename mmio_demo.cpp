#include "spi_example_registers.hpp"

namespace {

// The demo stays as a compile-checked usage example. Host-side builds do not
// execute direct MMIO writes; real runtime behavior is covered by the QEMU
// target tests.
void demoSequence() {
  SPI_CR::Instance<0xFFFE0000u> spiCr;
  SPI_MR::Instance<0xFFFE0004u> spiMr;

  spiCr &= ~SPI_CR::MASK;
  spiMr &= ~SPI_MR::MASK;

  spiCr = SPI_CR::SPIEN::ENABLE | SPI_CR::SWRST::RESET;
  spiCr |= SPI_CR::SPIEN::ENABLE;
  spiCr &= ~SPI_CR::SWRST::MASK;
  spiCr ^= SPI_CR::SPIEN::MASK;

  spiMr = SPI_MR::MSTR::MASTER | SPI_MR::DLY::value(7);
  spiMr.set<SPI_MR::MSTR::MASTER>();
  spiMr.set(SPI_MR::MSTR::MASTER | SPI_MR::DLY::value(7));
  spiMr.set<SPI_MR::DLY>(7);
  spiMr |= SPI_MR::MSTR::MASTER;
  spiMr &= ~SPI_MR::MSTR::MASK;

  const bool isMaster = (spiMr & SPI_MR::MSTR::MASTER);
  const bool isSlave = (spiMr & SPI_MR::MSTR::SLAVE);
  (void)isMaster;
  (void)isSlave;
}

}  // namespace

int main() {
  (void)&demoSequence;
  return 0;
}
