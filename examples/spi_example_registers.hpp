#pragma once


#include "mmio.hpp"

// Example register definitions used by the demo and tests. These stay outside
// the public framework header so mmio.hpp remains generic and reusable.

struct SPI_CR : mmio::Register<SPI_CR> {

  struct SPIEN : mmio::BitField<SPI_CR, 0, 1> {
    static constexpr auto DISABLE = value(0);
    static constexpr auto ENABLE = value(1);
  };

  struct SWRST : mmio::BitField<SPI_CR, 7, 1> {
    static constexpr auto IDLE = value(0);
    static constexpr auto RESET = value(1);
  };

  struct CMD : mmio::ValueField<SPI_CR, 8, 2> {};
};

struct SPI_MR : mmio::Register<SPI_MR> {

  struct DelayCycles {
    std::uint8_t cycles{};

    constexpr DelayCycles() = default;
    constexpr DelayCycles(std::uint32_t raw) : cycles(static_cast<std::uint8_t>(raw)) {}
    constexpr operator std::uint32_t() const { return cycles; }
  };

  struct MSTR : mmio::BitField<SPI_MR, 0, 1> {
    static constexpr auto SLAVE = value(0);
    static constexpr auto MASTER = value(1);
  };

  struct CSAAT : mmio::BitField<SPI_MR, 3, 1> {
    static constexpr auto RELEASE = value(0);
    static constexpr auto KEEP_ASSERTED = value(1);
  };

  struct PCS : mmio::ValueField<SPI_MR, 4, 2> {};

  struct DLY : mmio::ValueField<SPI_MR, 8, 4, DelayCycles> {};
};

// Status registers are typically driven by hardware. The target runtime tests
// update this register through the bound address directly and then assert that
// the typed API decodes the resulting bit states correctly.
struct SPI_SR : mmio::Register<SPI_SR> {

  struct RDRF : mmio::BitField<SPI_SR, 0, 1> {
    static constexpr auto EMPTY = value(0);
    static constexpr auto READY = value(1);
  };

  struct TDRE : mmio::BitField<SPI_SR, 1, 1> {
    static constexpr auto BUSY = value(0);
    static constexpr auto READY = value(1);
  };

  struct OVRES : mmio::BitField<SPI_SR, 3, 1> {
    static constexpr auto OK = value(0);
    static constexpr auto OVERRUN = value(1);
  };
};

