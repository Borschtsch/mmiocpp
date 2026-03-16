#pragma once


#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "spi_registers.hpp"

namespace stm32f429::drivers {

enum class I2sMode { slaveTx, slaveRx, masterTx, masterRx };
enum class I2sStandard { philips, msbJustified, lsbJustified, pcmShort, pcmLong };
enum class I2sDataFormat { bits16, bits16Extended, bits24, bits32 };
enum class I2sClockPolarity { low, high };
enum class I2sMasterClock { disabled, enabled };

enum class I2sResult { ok, busy, timeout, invalidArgument, error };
enum class I2sState { reset, ready, busyTx, busyRx, busyTransfer, error };

enum class I2sError : std::uint32_t {
  none = 0,
  overrun = 1u << 0,
  underrun = 1u << 1,
  frameFormat = 1u << 2,
  prescaler = 1u << 3,
};

constexpr I2sError operator|(I2sError lhs, I2sError rhs) noexcept {
  return static_cast<I2sError>(static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
}

constexpr I2sError& operator|=(I2sError& lhs, I2sError rhs) noexcept {
  lhs = lhs | rhs;
  return lhs;
}

constexpr bool any(I2sError value) noexcept {
  return value != I2sError::none;
}

struct I2sPrescaler {
  std::uint16_t linearDivider = 2;
  bool odd = false;
};

struct I2sConfig {
  I2sMode mode = I2sMode::masterTx;
  I2sStandard standard = I2sStandard::philips;
  I2sDataFormat dataFormat = I2sDataFormat::bits16;
  I2sClockPolarity clockPolarity = I2sClockPolarity::low;
  I2sMasterClock masterClock = I2sMasterClock::disabled;
  I2sPrescaler prescaler{};
  bool enableAfterConfigure = true;
};

struct I2sCallbacks {
  void* context = nullptr;
  void (*txComplete)(void*) = nullptr;
  void (*rxComplete)(void*) = nullptr;
  void (*transferComplete)(void*) = nullptr;
  void (*error)(void*, I2sError) = nullptr;
};

// I2sDriver models the STM32 I2S personality of the shared SPI/I2S block.
// It intentionally transfers 16-bit register words because the STM32 data
// register is always programmed in halfwords even when an audio frame spans
// multiple writes. Clock tree setup and sample-rate math stay above this layer.
template <typename MainBlock, typename ExtensionBlock = void>
class I2sDriver {
 public:
  using MainBlockType = MainBlock;
  using ExtensionBlockType = ExtensionBlock;
  using WordType = std::uint16_t;
  static constexpr bool kHasExtension = !std::is_void_v<ExtensionBlock>;
  static constexpr std::size_t kDefaultPollLimit = 1'000'000u;

  I2sResult configure(const I2sConfig& config) {
    if (this->isBusy()) {
      return I2sResult::busy;
    }

    if ((config.prescaler.linearDivider < 2u) || (config.prescaler.linearDivider > 0xFFu)) {
      this->lastError = I2sError::prescaler;
      this->currentState = I2sState::error;
      return I2sResult::invalidArgument;
    }

    this->disableInterrupts();
    this->disable();

    this->config = config;
    this->lastError = I2sError::none;
    this->writeMainConfiguration();
    if constexpr (kHasExtension) {
      this->writeExtensionConfiguration();
    }
    this->currentState = I2sState::ready;

    if (this->config.enableAfterConfigure) {
      this->enable();
    }

    return I2sResult::ok;
  }

  void setCallbacks(I2sCallbacks callbacks) noexcept { this->callbackSet = callbacks; }

  I2sState state() const noexcept { return this->currentState; }
  I2sError error() const noexcept { return this->lastError; }
  bool ready() const noexcept { return this->currentState == I2sState::ready; }

  void clearError() noexcept {
    this->lastError = I2sError::none;
    if (this->currentState == I2sState::error) {
      this->currentState = I2sState::ready;
    }
  }

  void enable() const { this->ensureMainEnabled(); }

  void disable() const {
    typename MainBlock::I2SCFGR cfgr;
    cfgr &= ~i2s::I2SCFGR::I2SE::MASK;

    if constexpr (kHasExtension) {
      typename ExtensionBlock::I2SCFGR extensionCfgr;
      extensionCfgr &= ~i2s::I2SCFGR::I2SE::MASK;
    }
  }

  bool enabled() const {
    typename MainBlock::I2SCFGR cfgr;
    return cfgr & i2s::I2SCFGR::I2SE::ENABLED;
  }

  bool busyFlag() const {
    typename MainBlock::SR sr;
    return sr & i2s::SR::BSY::BUSY;
  }

  bool txReady() const {
    typename MainBlock::SR sr;
    return sr & i2s::SR::TXE::EMPTY;
  }

  bool rxReady() const {
    typename MainBlock::SR sr;
    return sr & i2s::SR::RXNE::NOT_EMPTY;
  }

  // STM32 clears OVR by reading DR and then SR.
  void clearOverrun() const {
    typename MainBlock::DR dr;
    typename MainBlock::SR sr;
    (void)dr.template get<i2s::DR::DATA>();
    (void)sr.template get<i2s::SR::OVR>();
  }

  // STM32 clears UDR by reading SR.
  void clearUnderrun() const {
    typename MainBlock::SR sr;
    (void)sr.template get<i2s::SR::UDR>();
  }

  void flushReceive() const {
    typename MainBlock::DR dr;
    while (this->rxReady()) {
      (void)dr.template get<i2s::DR::DATA>();
    }
  }

  I2sResult transmitPolling(const WordType* data,
                            std::size_t count,
                            std::size_t pollLimit = kDefaultPollLimit) {
    if ((count == 0u) || (data == nullptr)) {
      return I2sResult::invalidArgument;
    }
    if (this->isBusy()) {
      return I2sResult::busy;
    }

    this->currentState = I2sState::busyTx;
    this->lastError = I2sError::none;
    this->ensureMainEnabled();

    for (std::size_t index = 0; index < count; ++index) {
      if (!this->waitUntil([this]() { return this->txReady(); }, pollLimit)) {
        return this->failTimeout();
      }

      this->writeMainData(data[index]);

      if (this->captureSimpleErrors()) {
        return I2sResult::error;
      }
    }

    if (!this->waitUntil([this]() { return this->txReady(); }, pollLimit)) {
      return this->failTimeout();
    }
    if (!this->waitUntil([this]() { return !this->busyFlag(); }, pollLimit)) {
      return this->failTimeout();
    }

    this->currentState = I2sState::ready;
    return I2sResult::ok;
  }

  I2sResult receivePolling(WordType* data,
                           std::size_t count,
                           std::size_t pollLimit = kDefaultPollLimit) {
    if ((count == 0u) || (data == nullptr)) {
      return I2sResult::invalidArgument;
    }
    if (this->isBusy()) {
      return I2sResult::busy;
    }

    this->currentState = I2sState::busyRx;
    this->lastError = I2sError::none;
    this->ensureMainEnabled();

    for (std::size_t index = 0; index < count; ++index) {
      if (!this->waitUntil([this]() { return this->rxReady(); }, pollLimit)) {
        return this->failTimeout();
      }

      data[index] = this->readMainData();

      if (this->captureSimpleErrors()) {
        return I2sResult::error;
      }
    }

    this->currentState = I2sState::ready;
    return I2sResult::ok;
  }

  template <typename Ext = ExtensionBlock, std::enable_if_t<!std::is_void_v<Ext>, int> = 0>
  I2sResult transmitReceivePolling(const WordType* txData,
                                   WordType* rxData,
                                   std::size_t count,
                                   std::size_t pollLimit = kDefaultPollLimit) {
    if ((count == 0u) || (txData == nullptr) || (rxData == nullptr)) {
      return I2sResult::invalidArgument;
    }
    if (this->isBusy()) {
      return I2sResult::busy;
    }

    this->currentState = I2sState::busyTransfer;
    this->lastError = I2sError::none;

    const bool txMode = this->isTransmitMode(this->config.mode);
    std::size_t txIndex = 0u;
    std::size_t rxIndex = 0u;

    this->enableExtensionBlock();
    this->ensureMainEnabled();

    if (txMode) {
      this->writeMainData(txData[txIndex++]);
    } else {
      this->writeExtensionData(txData[txIndex++]);
    }

    while ((txIndex < count) || (rxIndex < count)) {
      if (txMode) {
        if (txIndex < count) {
          if (!this->waitUntil([this]() { return this->txReady(); }, pollLimit)) {
            return this->failTimeout();
          }
          this->writeMainData(txData[txIndex++]);
        }

        if (rxIndex < count) {
          if (!this->waitUntil([this]() { return this->extensionRxReady(); }, pollLimit)) {
            return this->failTimeout();
          }
          rxData[rxIndex++] = this->readExtensionData();
        }
      } else {
        if (txIndex < count) {
          if (!this->waitUntil([this]() { return this->extensionTxReady(); }, pollLimit)) {
            return this->failTimeout();
          }
          this->writeExtensionData(txData[txIndex++]);
        }

        if (rxIndex < count) {
          if (!this->waitUntil([this]() { return this->rxReady(); }, pollLimit)) {
            return this->failTimeout();
          }
          rxData[rxIndex++] = this->readMainData();
        }
      }

      if (this->captureFullDuplexErrors()) {
        return I2sResult::error;
      }
    }

    if (txMode) {
      if (!this->waitUntil([this]() { return this->txReady(); }, pollLimit)) {
        return this->failTimeout();
      }
    } else {
      if (!this->waitUntil([this]() { return this->extensionTxReady(); }, pollLimit)) {
        return this->failTimeout();
      }
    }
    if (!this->waitUntil([this]() { return !this->busyFlag(); }, pollLimit)) {
      return this->failTimeout();
    }

    this->currentState = I2sState::ready;
    return I2sResult::ok;
  }

  I2sResult startTransmitInterrupt(const WordType* data, std::size_t count) {
    if ((count == 0u) || (data == nullptr)) {
      return I2sResult::invalidArgument;
    }
    if (this->isBusy()) {
      return I2sResult::busy;
    }

    this->beginAsync(I2sState::busyTx);
    this->txBuffer = data;
    this->rxBuffer = nullptr;
    this->txRemaining = count;
    this->rxRemaining = 0u;
    this->currentOperation = OperationKind::tx;

    this->enableMainErrorInterrupt();
    this->enableMainTxeInterrupt();
    return I2sResult::ok;
  }

  I2sResult startReceiveInterrupt(WordType* data, std::size_t count) {
    if ((count == 0u) || (data == nullptr)) {
      return I2sResult::invalidArgument;
    }
    if (this->isBusy()) {
      return I2sResult::busy;
    }

    this->beginAsync(I2sState::busyRx);
    this->txBuffer = nullptr;
    this->rxBuffer = data;
    this->txRemaining = 0u;
    this->rxRemaining = count;
    this->currentOperation = OperationKind::rx;

    this->enableMainErrorInterrupt();
    this->enableMainRxInterrupt();
    return I2sResult::ok;
  }

  template <typename Ext = ExtensionBlock, std::enable_if_t<!std::is_void_v<Ext>, int> = 0>
  I2sResult startTransmitReceiveInterrupt(const WordType* txData,
                                          WordType* rxData,
                                          std::size_t count) {
    if ((count == 0u) || (txData == nullptr) || (rxData == nullptr)) {
      return I2sResult::invalidArgument;
    }
    if (this->isBusy()) {
      return I2sResult::busy;
    }

    this->beginAsync(I2sState::busyTransfer);
    this->txBuffer = txData;
    this->rxBuffer = rxData;
    this->txRemaining = count;
    this->rxRemaining = count;
    this->currentOperation = OperationKind::transfer;

    const bool txMode = this->isTransmitMode(this->config.mode);
    this->enableExtensionBlock();
    this->ensureMainEnabled();

    if (txMode) {
      this->enableExtensionErrorInterrupt();
      this->enableExtensionRxInterrupt();
      this->enableMainErrorInterrupt();
      this->enableMainTxeInterrupt();

      this->writeMainData(*this->txBuffer++);
      --this->txRemaining;
      if (this->txRemaining == 0u) {
        this->disableMainTxeInterrupt();
      }
    } else {
      this->enableExtensionErrorInterrupt();
      this->enableExtensionTxeInterrupt();
      this->enableMainErrorInterrupt();
      this->enableMainRxInterrupt();

      this->writeExtensionData(*this->txBuffer++);
      --this->txRemaining;
      if (this->txRemaining == 0u) {
        this->disableExtensionTxeInterrupt();
      }
    }

    return I2sResult::ok;
  }

  void cancelInterrupt() {
    this->disableInterrupts();
    this->txBuffer = nullptr;
    this->rxBuffer = nullptr;
    this->txRemaining = 0u;
    this->rxRemaining = 0u;
    this->currentOperation = OperationKind::none;
    this->currentState = I2sState::ready;
  }

  // Call this from the real IRQ handlers. In full-duplex mode the same method
  // can be reached from both the main SPIx IRQ and the I2Sxext companion IRQ.
  void onInterrupt() {
    if (!this->isBusy()) {
      return;
    }

    if (this->currentOperation == OperationKind::transfer) {
      if (this->captureFullDuplexErrors()) {
        this->disableInterrupts();
        this->invokeError();
        return;
      }

      if (this->isTransmitMode(this->config.mode)) {
        if ((this->txRemaining > 0u) && this->txReady()) {
          this->writeMainData(*this->txBuffer++);
          --this->txRemaining;
          if (this->txRemaining == 0u) {
            this->disableMainTxeInterrupt();
          }
        }

        if ((this->rxRemaining > 0u) && this->extensionRxReady()) {
          *this->rxBuffer++ = this->readExtensionData();
          --this->rxRemaining;
          if (this->rxRemaining == 0u) {
            this->disableExtensionRxInterrupt();
          }
        }
      } else {
        if ((this->txRemaining > 0u) && this->extensionTxReady()) {
          this->writeExtensionData(*this->txBuffer++);
          --this->txRemaining;
          if (this->txRemaining == 0u) {
            this->disableExtensionTxeInterrupt();
          }
        }

        if ((this->rxRemaining > 0u) && this->rxReady()) {
          *this->rxBuffer++ = this->readMainData();
          --this->rxRemaining;
          if (this->rxRemaining == 0u) {
            this->disableMainRxInterrupt();
          }
        }
      }
    } else {
      if (this->captureSimpleErrors()) {
        this->disableInterrupts();
        this->invokeError();
        return;
      }

      if ((this->txRemaining > 0u) && this->txReady()) {
        this->writeMainData(*this->txBuffer++);
        --this->txRemaining;
        if (this->txRemaining == 0u) {
          this->disableMainTxeInterrupt();
        }
      }

      if ((this->rxRemaining > 0u) && this->rxReady()) {
        *this->rxBuffer++ = this->readMainData();
        --this->rxRemaining;
        if (this->rxRemaining == 0u) {
          this->disableMainRxInterrupt();
        }
      }
    }

    if ((this->txRemaining == 0u) && (this->rxRemaining == 0u) &&
        (this->currentOperation != OperationKind::none)) {
      this->disableInterrupts();
      const auto completed = this->currentOperation;
      this->currentOperation = OperationKind::none;
      this->currentState = I2sState::ready;
      this->invokeCompletion(completed);
    }
  }

 private:
  enum class OperationKind { none, tx, rx, transfer };

  I2sConfig config{};
  I2sCallbacks callbackSet{};
  I2sState currentState = I2sState::reset;
  I2sError lastError = I2sError::none;
  const WordType* txBuffer = nullptr;
  WordType* rxBuffer = nullptr;
  std::size_t txRemaining = 0u;
  std::size_t rxRemaining = 0u;
  OperationKind currentOperation = OperationKind::none;

  bool isBusy() const noexcept {
    return (this->currentState == I2sState::busyTx) ||
           (this->currentState == I2sState::busyRx) ||
           (this->currentState == I2sState::busyTransfer);
  }

  template <typename Predicate>
  static bool waitUntil(Predicate predicate, std::size_t limit) {
    for (std::size_t spin = 0; spin < limit; ++spin) {
      if (predicate()) {
        return true;
      }
    }
    return false;
  }

  static constexpr bool isTransmitMode(I2sMode mode) noexcept {
    return (mode == I2sMode::masterTx) || (mode == I2sMode::slaveTx);
  }

  static constexpr auto modeBits(I2sMode mode) {
    switch (mode) {
      case I2sMode::slaveTx: return i2s::I2SCFGR::MODE_SLAVE_TX;
      case I2sMode::slaveRx: return i2s::I2SCFGR::MODE_SLAVE_RX;
      case I2sMode::masterTx: return i2s::I2SCFGR::MODE_MASTER_TX;
      case I2sMode::masterRx: return i2s::I2SCFGR::MODE_MASTER_RX;
    }
    return i2s::I2SCFGR::MODE_MASTER_TX;
  }

  static constexpr auto extensionModeBits(I2sMode mode) {
    return isTransmitMode(mode) ? i2s::I2SCFGR::MODE_SLAVE_RX : i2s::I2SCFGR::MODE_SLAVE_TX;
  }

  static constexpr auto standardBits(I2sStandard standard) {
    switch (standard) {
      case I2sStandard::philips: return i2s::I2SCFGR::STANDARD_PHILIPS;
      case I2sStandard::msbJustified: return i2s::I2SCFGR::STANDARD_MSB;
      case I2sStandard::lsbJustified: return i2s::I2SCFGR::STANDARD_LSB;
      case I2sStandard::pcmShort: return i2s::I2SCFGR::STANDARD_PCM_SHORT;
      case I2sStandard::pcmLong: return i2s::I2SCFGR::STANDARD_PCM_LONG;
    }
    return i2s::I2SCFGR::STANDARD_PHILIPS;
  }

  static constexpr auto dataFormatBits(I2sDataFormat format) {
    switch (format) {
      case I2sDataFormat::bits16: return i2s::I2SCFGR::DATAFORMAT_16B;
      case I2sDataFormat::bits16Extended: return i2s::I2SCFGR::DATAFORMAT_16B_EXTENDED;
      case I2sDataFormat::bits24: return i2s::I2SCFGR::DATAFORMAT_24B;
      case I2sDataFormat::bits32: return i2s::I2SCFGR::DATAFORMAT_32B;
    }
    return i2s::I2SCFGR::DATAFORMAT_16B;
  }

  static constexpr auto polarityBits(I2sClockPolarity polarity) {
    return (polarity == I2sClockPolarity::low) ? i2s::I2SCFGR::CPOL_LOW
                                               : i2s::I2SCFGR::CPOL_HIGH;
  }

  static constexpr auto masterClockBits(I2sMasterClock output) {
    return (output == I2sMasterClock::enabled) ? i2s::I2SPR::MCLKOUTPUT_ENABLE
                                               : i2s::I2SPR::MCLKOUTPUT_DISABLE;
  }

  static constexpr auto oddBits(bool odd) {
    return odd ? i2s::I2SPR::ODD::ODD_FACTOR : i2s::I2SPR::ODD::EVEN;
  }

  void ensureMainEnabled() const {
    if (!this->enabled()) {
      typename MainBlock::I2SCFGR cfgr;
      cfgr |= i2s::I2SCFGR::I2SE::ENABLED;
    }
  }

  void enableExtensionBlock() const {
    if constexpr (kHasExtension) {
      typename ExtensionBlock::I2SCFGR cfgr;
      cfgr |= i2s::I2SCFGR::I2SE::ENABLED;
    }
  }

  I2sResult failTimeout() {
    this->currentState = I2sState::ready;
    return I2sResult::timeout;
  }

  void beginAsync(I2sState newState) {
    this->lastError = I2sError::none;
    this->currentState = newState;
    this->ensureMainEnabled();
  }

  void writeMainConfiguration() const {
    typename MainBlock::CR2 cr2;
    typename MainBlock::I2SCFGR cfgr;
    typename MainBlock::I2SPR prescaler;

    cfgr = i2s::I2SCFGR::I2SMOD::I2S_MODE |
           this->modeBits(this->config.mode) |
           this->standardBits(this->config.standard) |
           this->dataFormatBits(this->config.dataFormat) |
           this->polarityBits(this->config.clockPolarity) |
           (this->config.enableAfterConfigure ? i2s::I2SCFGR::I2SE::ENABLED
                                              : i2s::I2SCFGR::I2SE::DISABLED);
    prescaler = this->masterClockBits(this->config.masterClock) |
                this->oddBits(this->config.prescaler.odd) |
                i2s::I2SPR::I2SDIV::value(this->config.prescaler.linearDivider);
    cr2 = i2s::CR2::RXDMAEN::DISABLED | i2s::CR2::TXDMAEN::DISABLED |
          i2s::CR2::ERRIE::DISABLED | i2s::CR2::RXNEIE::DISABLED |
          i2s::CR2::TXEIE::DISABLED;
  }

  void writeExtensionConfiguration() const {
    if constexpr (kHasExtension) {
      typename ExtensionBlock::CR2 cr2;
      typename ExtensionBlock::I2SCFGR cfgr;
      typename ExtensionBlock::I2SPR prescaler;

      cfgr = i2s::I2SCFGR::I2SMOD::I2S_MODE |
             this->extensionModeBits(this->config.mode) |
             this->standardBits(this->config.standard) |
             this->dataFormatBits(this->config.dataFormat) |
             this->polarityBits(this->config.clockPolarity) |
             i2s::I2SCFGR::I2SE::DISABLED;
      prescaler = i2s::I2SPR::MCLKOUTPUT_DISABLE |
                  i2s::I2SPR::ODD::EVEN |
                  i2s::I2SPR::I2SDIV::value(2);
      cr2 = i2s::CR2::RXDMAEN::DISABLED | i2s::CR2::TXDMAEN::DISABLED |
            i2s::CR2::ERRIE::DISABLED | i2s::CR2::RXNEIE::DISABLED |
            i2s::CR2::TXEIE::DISABLED;
    }
  }

  void disableInterrupts() const {
    this->disableMainTxeInterrupt();
    this->disableMainRxInterrupt();
    this->disableMainErrorInterrupt();

    if constexpr (kHasExtension) {
      this->disableExtensionTxeInterrupt();
      this->disableExtensionRxInterrupt();
      this->disableExtensionErrorInterrupt();
    }
  }

  void enableMainTxeInterrupt() const {
    typename MainBlock::CR2 cr2;
    cr2 |= i2s::CR2::TXEIE::ENABLED;
  }

  void enableMainRxInterrupt() const {
    typename MainBlock::CR2 cr2;
    cr2 |= i2s::CR2::RXNEIE::ENABLED;
  }

  void enableMainErrorInterrupt() const {
    typename MainBlock::CR2 cr2;
    cr2 |= i2s::CR2::ERRIE::ENABLED;
  }

  void disableMainTxeInterrupt() const {
    typename MainBlock::CR2 cr2;
    cr2 &= ~i2s::CR2::TXEIE::MASK;
  }

  void disableMainRxInterrupt() const {
    typename MainBlock::CR2 cr2;
    cr2 &= ~i2s::CR2::RXNEIE::MASK;
  }

  void disableMainErrorInterrupt() const {
    typename MainBlock::CR2 cr2;
    cr2 &= ~i2s::CR2::ERRIE::MASK;
  }

  void enableExtensionTxeInterrupt() const {
    if constexpr (kHasExtension) {
      typename ExtensionBlock::CR2 cr2;
      cr2 |= i2s::CR2::TXEIE::ENABLED;
    }
  }

  void enableExtensionRxInterrupt() const {
    if constexpr (kHasExtension) {
      typename ExtensionBlock::CR2 cr2;
      cr2 |= i2s::CR2::RXNEIE::ENABLED;
    }
  }

  void enableExtensionErrorInterrupt() const {
    if constexpr (kHasExtension) {
      typename ExtensionBlock::CR2 cr2;
      cr2 |= i2s::CR2::ERRIE::ENABLED;
    }
  }

  void disableExtensionTxeInterrupt() const {
    if constexpr (kHasExtension) {
      typename ExtensionBlock::CR2 cr2;
      cr2 &= ~i2s::CR2::TXEIE::MASK;
    }
  }

  void disableExtensionRxInterrupt() const {
    if constexpr (kHasExtension) {
      typename ExtensionBlock::CR2 cr2;
      cr2 &= ~i2s::CR2::RXNEIE::MASK;
    }
  }

  void disableExtensionErrorInterrupt() const {
    if constexpr (kHasExtension) {
      typename ExtensionBlock::CR2 cr2;
      cr2 &= ~i2s::CR2::ERRIE::MASK;
    }
  }

  void writeMainData(WordType value) const {
    typename MainBlock::DR dr;
    dr.template set<i2s::DR::DATA>(value);
  }

  WordType readMainData() const {
    typename MainBlock::DR dr;
    return dr.template get<i2s::DR::DATA>();
  }

  bool extensionTxReady() const {
    if constexpr (kHasExtension) {
      typename ExtensionBlock::SR sr;
      return sr & i2s::SR::TXE::EMPTY;
    }
    return false;
  }

  bool extensionRxReady() const {
    if constexpr (kHasExtension) {
      typename ExtensionBlock::SR sr;
      return sr & i2s::SR::RXNE::NOT_EMPTY;
    }
    return false;
  }

  void writeExtensionData(WordType value) const {
    if constexpr (kHasExtension) {
      typename ExtensionBlock::DR dr;
      dr.template set<i2s::DR::DATA>(value);
    }
  }

  WordType readExtensionData() const {
    if constexpr (kHasExtension) {
      typename ExtensionBlock::DR dr;
      return dr.template get<i2s::DR::DATA>();
    }
    return 0u;
  }

  void clearExtensionOverrun() const {
    if constexpr (kHasExtension) {
      typename ExtensionBlock::DR dr;
      typename ExtensionBlock::SR sr;
      (void)dr.template get<i2s::DR::DATA>();
      (void)sr.template get<i2s::SR::OVR>();
    }
  }

  void clearExtensionUnderrun() const {
    if constexpr (kHasExtension) {
      typename ExtensionBlock::SR sr;
      (void)sr.template get<i2s::SR::UDR>();
    }
  }

  bool captureSimpleErrors() {
    typename MainBlock::SR sr;
    I2sError errors = I2sError::none;

    if (sr & i2s::SR::OVR::OVERRUN) {
      this->clearOverrun();
      errors |= I2sError::overrun;
    }
    if (sr & i2s::SR::UDR::UNDERRUN) {
      this->clearUnderrun();
      errors |= I2sError::underrun;
    }
    if (sr & i2s::SR::FRE::ERROR) {
      errors |= I2sError::frameFormat;
    }

    if (any(errors)) {
      this->lastError |= errors;
      this->currentState = I2sState::error;
      return true;
    }

    return false;
  }

  bool captureFullDuplexErrors() {
    if constexpr (!kHasExtension) {
      return false;
    } else {
      typename MainBlock::SR mainSr;
      typename ExtensionBlock::SR extensionSr;
      I2sError errors = I2sError::none;

      if (this->isTransmitMode(this->config.mode)) {
        if (extensionSr & i2s::SR::OVR::OVERRUN) {
          this->clearExtensionOverrun();
          errors |= I2sError::overrun;
        }
        if (mainSr & i2s::SR::UDR::UNDERRUN) {
          this->clearUnderrun();
          errors |= I2sError::underrun;
        }
      } else {
        if (mainSr & i2s::SR::OVR::OVERRUN) {
          this->clearOverrun();
          errors |= I2sError::overrun;
        }
        if (extensionSr & i2s::SR::UDR::UNDERRUN) {
          this->clearExtensionUnderrun();
          errors |= I2sError::underrun;
        }
      }

      if ((mainSr & i2s::SR::FRE::ERROR) || (extensionSr & i2s::SR::FRE::ERROR)) {
        errors |= I2sError::frameFormat;
      }

      if (any(errors)) {
        this->lastError |= errors;
        this->currentState = I2sState::error;
        return true;
      }

      return false;
    }
  }

  void invokeCompletion(OperationKind completed) const {
    switch (completed) {
      case OperationKind::tx:
        if (this->callbackSet.txComplete != nullptr) {
          this->callbackSet.txComplete(this->callbackSet.context);
        }
        break;
      case OperationKind::rx:
        if (this->callbackSet.rxComplete != nullptr) {
          this->callbackSet.rxComplete(this->callbackSet.context);
        }
        break;
      case OperationKind::transfer:
        if (this->callbackSet.transferComplete != nullptr) {
          this->callbackSet.transferComplete(this->callbackSet.context);
        }
        break;
      case OperationKind::none:
        break;
    }
  }

  void invokeError() const {
    if (this->callbackSet.error != nullptr) {
      this->callbackSet.error(this->callbackSet.context, this->lastError);
    }
  }
};

}  // namespace stm32f429::drivers

