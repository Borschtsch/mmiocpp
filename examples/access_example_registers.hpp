#pragma once

#include "mmio.hpp"

// Example register definitions that exercise access semantics beyond plain
// read/write fields. These are intentionally synthetic so the field-access
// rules stay obvious in the tests.

struct ACCESS_STATUS : mmio::Register<ACCESS_STATUS, std::uint32_t, mmio::Ro> {
  struct READY : mmio::BitField<ACCESS_STATUS, 0, 1> {
    static constexpr auto IDLE = value(0);
    static constexpr auto ASSERTED = value(1);
  };

  struct COUNT : mmio::ValueField<ACCESS_STATUS, 4, 4> {};

  struct OVERRUN : mmio::BitField<ACCESS_STATUS, 8, 1, mmio::W1c> {
    static constexpr auto OK = state(0);
    static constexpr auto DETECTED = state(1);
    static constexpr auto CLEAR = action(1);
  };

  struct FRAME : mmio::BitField<ACCESS_STATUS, 9, 1, mmio::W1c> {
    static constexpr auto OK = state(0);
    static constexpr auto DETECTED = state(1);
    static constexpr auto CLEAR = action(1);
  };
};

struct ACCESS_COMMAND : mmio::Register<ACCESS_COMMAND, std::uint32_t, mmio::Wo> {
  struct START : mmio::BitField<ACCESS_COMMAND, 0, 1> {
    static constexpr auto TRIGGER = value(1);
  };

  struct STOP : mmio::BitField<ACCESS_COMMAND, 1, 1> {
    static constexpr auto TRIGGER = value(1);
  };

  struct COUNT : mmio::ValueField<ACCESS_COMMAND, 8, 4> {};
};

struct ACCESS_LATCH : mmio::Register<ACCESS_LATCH, std::uint32_t, mmio::Ro> {
  struct ENABLED : mmio::BitField<ACCESS_LATCH, 0, 1, mmio::W1s> {
    static constexpr auto OFF = state(0);
    static constexpr auto ON = state(1);
    static constexpr auto SET = action(1);
  };

  struct CHANNEL : mmio::BitField<ACCESS_LATCH, 1, 1, mmio::W1s> {
    static constexpr auto OFF = state(0);
    static constexpr auto ON = state(1);
    static constexpr auto SET = action(1);
  };
};

struct ACCESS_ZERO : mmio::Register<ACCESS_ZERO, std::uint32_t, mmio::Ro> {
  struct STICKY : mmio::BitField<ACCESS_ZERO, 0, 1, mmio::W0c> {
    static constexpr auto CLEAR = state(0);
    static constexpr auto SET = state(1);
    static constexpr auto CLEAR_LATCH = action(0);
  };

  struct ARMED : mmio::BitField<ACCESS_ZERO, 1, 1, mmio::W0s> {
    static constexpr auto DISARMED = state(0);
    static constexpr auto SET = state(1);
    static constexpr auto FORCE_SET = action(0);
  };
};