#pragma once


#include <cstdint>

#include "mmio.hpp"

// STM32F429 SPI/I2S register definitions derived from the STM32F429 CMSIS
// device header and cross-checked against the reference manual. The SPI and I2S
// peripherals share the same hardware block, so this example models the full
// SPI_TypeDef layout and then adds the I2S-specific configuration registers.
namespace stm32f429::spi {

struct CR1 : mmio::Register<CR1> {
  struct CPHA : mmio::BitField<CR1, 0, 1> {
    static constexpr auto FIRST_EDGE = value(0);
    static constexpr auto SECOND_EDGE = value(1);
  };

  struct CPOL : mmio::BitField<CR1, 1, 1> {
    static constexpr auto LOW = value(0);
    static constexpr auto HIGH = value(1);
  };

  struct MSTR : mmio::BitField<CR1, 2, 1> {
    static constexpr auto SLAVE = value(0);
    static constexpr auto MASTER = value(1);
  };

  struct BR : mmio::ValueField<CR1, 3, 3> {
    static constexpr auto DIV_2 = value(0);
    static constexpr auto DIV_4 = value(1);
    static constexpr auto DIV_8 = value(2);
    static constexpr auto DIV_16 = value(3);
    static constexpr auto DIV_32 = value(4);
    static constexpr auto DIV_64 = value(5);
    static constexpr auto DIV_128 = value(6);
    static constexpr auto DIV_256 = value(7);
  };

  struct SPE : mmio::BitField<CR1, 6, 1> {
    static constexpr auto DISABLED = value(0);
    static constexpr auto ENABLED = value(1);
  };

  struct LSBFIRST : mmio::BitField<CR1, 7, 1> {
    static constexpr auto MSB_FIRST = value(0);
    static constexpr auto LSB_FIRST = value(1);
  };

  struct SSI : mmio::BitField<CR1, 8, 1> {
    static constexpr auto LOW = value(0);
    static constexpr auto HIGH = value(1);
  };

  struct SSM : mmio::BitField<CR1, 9, 1> {
    static constexpr auto HARDWARE = value(0);
    static constexpr auto SOFTWARE = value(1);
  };

  struct RXONLY : mmio::BitField<CR1, 10, 1> {
    static constexpr auto FULL_DUPLEX = value(0);
    static constexpr auto RECEIVE_ONLY = value(1);
  };

  struct DFF : mmio::BitField<CR1, 11, 1> {
    static constexpr auto BITS_8 = value(0);
    static constexpr auto BITS_16 = value(1);
  };

  struct CRCNEXT : mmio::BitField<CR1, 12, 1> {
    static constexpr auto DATA_PHASE = value(0);
    static constexpr auto CRC_PHASE = value(1);
  };

  struct CRCEN : mmio::BitField<CR1, 13, 1> {
    static constexpr auto DISABLED = value(0);
    static constexpr auto ENABLED = value(1);
  };

  struct BIDIOE : mmio::BitField<CR1, 14, 1> {
    static constexpr auto RECEIVE = value(0);
    static constexpr auto TRANSMIT = value(1);
  };

  struct BIDIMODE : mmio::BitField<CR1, 15, 1> {
    static constexpr auto TWO_LINE = value(0);
    static constexpr auto ONE_LINE = value(1);
  };
};

struct CR2 : mmio::Register<CR2> {
  struct RXDMAEN : mmio::BitField<CR2, 0, 1> {
    static constexpr auto DISABLED = value(0);
    static constexpr auto ENABLED = value(1);
  };

  struct TXDMAEN : mmio::BitField<CR2, 1, 1> {
    static constexpr auto DISABLED = value(0);
    static constexpr auto ENABLED = value(1);
  };

  struct SSOE : mmio::BitField<CR2, 2, 1> {
    static constexpr auto DISABLED = value(0);
    static constexpr auto ENABLED = value(1);
  };

  struct FRF : mmio::BitField<CR2, 4, 1> {
    static constexpr auto MOTOROLA = value(0);
    static constexpr auto TI = value(1);
  };

  struct ERRIE : mmio::BitField<CR2, 5, 1> {
    static constexpr auto DISABLED = value(0);
    static constexpr auto ENABLED = value(1);
  };

  struct RXNEIE : mmio::BitField<CR2, 6, 1> {
    static constexpr auto DISABLED = value(0);
    static constexpr auto ENABLED = value(1);
  };

  struct TXEIE : mmio::BitField<CR2, 7, 1> {
    static constexpr auto DISABLED = value(0);
    static constexpr auto ENABLED = value(1);
  };
};

struct SR : mmio::Register<SR> {
  struct RXNE : mmio::BitField<SR, 0, 1> {
    static constexpr auto EMPTY = value(0);
    static constexpr auto NOT_EMPTY = value(1);
  };

  struct TXE : mmio::BitField<SR, 1, 1> {
    static constexpr auto NOT_EMPTY = value(0);
    static constexpr auto EMPTY = value(1);
  };

  struct CHSIDE : mmio::BitField<SR, 2, 1> {
    static constexpr auto LEFT = value(0);
    static constexpr auto RIGHT = value(1);
  };

  struct UDR : mmio::BitField<SR, 3, 1> {
    static constexpr auto OK = value(0);
    static constexpr auto UNDERRUN = value(1);
  };

  struct CRCERR : mmio::BitField<SR, 4, 1> {
    static constexpr auto OK = value(0);
    static constexpr auto ERROR = value(1);
  };

  struct MODF : mmio::BitField<SR, 5, 1> {
    static constexpr auto OK = value(0);
    static constexpr auto FAULT = value(1);
  };

  struct OVR : mmio::BitField<SR, 6, 1> {
    static constexpr auto OK = value(0);
    static constexpr auto OVERRUN = value(1);
  };

  struct BSY : mmio::BitField<SR, 7, 1> {
    static constexpr auto IDLE = value(0);
    static constexpr auto BUSY = value(1);
  };

  struct FRE : mmio::BitField<SR, 8, 1> {
    static constexpr auto OK = value(0);
    static constexpr auto ERROR = value(1);
  };
};

struct DR : mmio::Register<DR> {
  struct DATA : mmio::ValueField<DR, 0, 16, std::uint16_t> {};
};

struct CRCPR : mmio::Register<CRCPR> {
  struct POLYNOMIAL : mmio::ValueField<CRCPR, 0, 16, std::uint16_t> {};
};

struct RXCRCR : mmio::Register<RXCRCR> {
  struct VALUE : mmio::ValueField<RXCRCR, 0, 16, std::uint16_t> {};
};

struct TXCRCR : mmio::Register<TXCRCR> {
  struct VALUE : mmio::ValueField<TXCRCR, 0, 16, std::uint16_t> {};
};

}  // namespace stm32f429::spi

namespace stm32f429::i2s {

using CR2 = spi::CR2;
using SR = spi::SR;
using DR = spi::DR;

struct I2SCFGR : mmio::Register<I2SCFGR> {
  struct CHLEN : mmio::BitField<I2SCFGR, 0, 1> {
    static constexpr auto BITS_16 = value(0);
    static constexpr auto BITS_32 = value(1);
  };

  struct DATLEN : mmio::ValueField<I2SCFGR, 1, 2> {
    static constexpr auto BITS_16 = value(0);
    static constexpr auto BITS_24 = value(1);
    static constexpr auto BITS_32 = value(2);
  };

  struct CKPOL : mmio::BitField<I2SCFGR, 3, 1> {
    static constexpr auto LOW = value(0);
    static constexpr auto HIGH = value(1);
  };

  struct I2SSTD : mmio::ValueField<I2SCFGR, 4, 2> {
    static constexpr auto PHILIPS = value(0);
    static constexpr auto MSB_JUSTIFIED = value(1);
    static constexpr auto LSB_JUSTIFIED = value(2);
    static constexpr auto PCM = value(3);
  };

  struct PCMSYNC : mmio::BitField<I2SCFGR, 7, 1> {
    static constexpr auto SHORT = value(0);
    static constexpr auto LONG = value(1);
  };

  struct I2SCFG : mmio::ValueField<I2SCFGR, 8, 2> {
    static constexpr auto SLAVE_TX = value(0);
    static constexpr auto SLAVE_RX = value(1);
    static constexpr auto MASTER_TX = value(2);
    static constexpr auto MASTER_RX = value(3);
  };

  struct I2SE : mmio::BitField<I2SCFGR, 10, 1> {
    static constexpr auto DISABLED = value(0);
    static constexpr auto ENABLED = value(1);
  };

  struct I2SMOD : mmio::BitField<I2SCFGR, 11, 1> {
    static constexpr auto SPI_MODE = value(0);
    static constexpr auto I2S_MODE = value(1);
  };

  static constexpr auto MODE_SLAVE_TX = I2SCFG::SLAVE_TX;
  static constexpr auto MODE_SLAVE_RX = I2SCFG::SLAVE_RX;
  static constexpr auto MODE_MASTER_TX = I2SCFG::MASTER_TX;
  static constexpr auto MODE_MASTER_RX = I2SCFG::MASTER_RX;

  static constexpr auto STANDARD_PHILIPS = I2SSTD::PHILIPS;
  static constexpr auto STANDARD_MSB = I2SSTD::MSB_JUSTIFIED;
  static constexpr auto STANDARD_LSB = I2SSTD::LSB_JUSTIFIED;
  static constexpr auto STANDARD_PCM_SHORT = I2SSTD::PCM | PCMSYNC::SHORT;
  static constexpr auto STANDARD_PCM_LONG = I2SSTD::PCM | PCMSYNC::LONG;

  static constexpr auto DATAFORMAT_16B = CHLEN::BITS_16 | DATLEN::BITS_16;
  static constexpr auto DATAFORMAT_16B_EXTENDED = CHLEN::BITS_32 | DATLEN::BITS_16;
  static constexpr auto DATAFORMAT_24B = CHLEN::BITS_32 | DATLEN::BITS_24;
  static constexpr auto DATAFORMAT_32B = CHLEN::BITS_32 | DATLEN::BITS_32;

  static constexpr auto CPOL_LOW = CKPOL::LOW;
  static constexpr auto CPOL_HIGH = CKPOL::HIGH;
};

struct I2SPR : mmio::Register<I2SPR> {
  struct I2SDIV : mmio::ValueField<I2SPR, 0, 8, std::uint16_t> {};

  struct ODD : mmio::BitField<I2SPR, 8, 1> {
    static constexpr auto EVEN = value(0);
    static constexpr auto ODD_FACTOR = value(1);
  };

  struct MCKOE : mmio::BitField<I2SPR, 9, 1> {
    static constexpr auto DISABLED = value(0);
    static constexpr auto ENABLED = value(1);
  };

  static constexpr auto MCLKOUTPUT_DISABLE = MCKOE::DISABLED;
  static constexpr auto MCLKOUTPUT_ENABLE = MCKOE::ENABLED;
};

}  // namespace stm32f429::i2s

namespace stm32f429 {

constexpr std::uintptr_t I2S2EXT_BASE = 0x40003400u;
constexpr std::uintptr_t SPI2_BASE = 0x40003800u;
constexpr std::uintptr_t SPI3_BASE = 0x40003C00u;
constexpr std::uintptr_t I2S3EXT_BASE = 0x40004000u;
constexpr std::uintptr_t SPI1_BASE = 0x40013000u;
constexpr std::uintptr_t SPI4_BASE = 0x40013400u;
constexpr std::uintptr_t SPI5_BASE = 0x40015000u;
constexpr std::uintptr_t SPI6_BASE = 0x40015400u;

// The extension blocks use the same register layout as SPI_TypeDef in the STM32
// device header, even though the reference manual notes that they are only used
// as I2S slave companions for full-duplex audio.
template <std::uintptr_t Base>
struct SpiI2sBlock {
  using CR1 = spi::CR1::Instance<Base + 0x00u>;
  using CR2 = spi::CR2::Instance<Base + 0x04u>;
  using SR = spi::SR::Instance<Base + 0x08u>;
  using DR = spi::DR::Instance<Base + 0x0Cu>;
  using CRCPR = spi::CRCPR::Instance<Base + 0x10u>;
  using RXCRCR = spi::RXCRCR::Instance<Base + 0x14u>;
  using TXCRCR = spi::TXCRCR::Instance<Base + 0x18u>;
  using I2SCFGR = i2s::I2SCFGR::Instance<Base + 0x1Cu>;
  using I2SPR = i2s::I2SPR::Instance<Base + 0x20u>;
};

using SPI1 = SpiI2sBlock<SPI1_BASE>;
using I2S2ext = SpiI2sBlock<I2S2EXT_BASE>;
using SPI2 = SpiI2sBlock<SPI2_BASE>;
using SPI3 = SpiI2sBlock<SPI3_BASE>;
using I2S3ext = SpiI2sBlock<I2S3EXT_BASE>;
using SPI4 = SpiI2sBlock<SPI4_BASE>;
using SPI5 = SpiI2sBlock<SPI5_BASE>;
using SPI6 = SpiI2sBlock<SPI6_BASE>;

}  // namespace stm32f429

