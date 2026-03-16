#pragma once


#include <cstddef>
#include <cstdint>

#include "spi_registers.hpp"

namespace stm32f429::drivers {

enum class SpiRole { master, slave };
enum class SpiClockPhase { firstEdge, secondEdge };
enum class SpiClockPolarity { low, high };
enum class SpiBitOrder { msbFirst, lsbFirst };
enum class SpiFrameSize { bits8, bits16 };
enum class SpiFrameFormat { motorola, ti };
enum class SpiBusMode { fullDuplex, receiveOnly, halfDuplexTx, halfDuplexRx };
enum class SpiBaudDivider { div2, div4, div8, div16, div32, div64, div128, div256 };

enum class SpiResult { ok, busy, timeout, invalidArgument, error };
enum class SpiState { reset, ready, busyTx, busyRx, busyTransfer, error };

enum class SpiError : std::uint32_t {
  none = 0,
  overrun = 1u << 0,
  modeFault = 1u << 1,
  crc = 1u << 2,
  frameFormat = 1u << 3,
};

constexpr SpiError operator|(SpiError lhs, SpiError rhs) noexcept {
  return static_cast<SpiError>(static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
}

constexpr SpiError& operator|=(SpiError& lhs, SpiError rhs) noexcept {
  lhs = lhs | rhs;
  return lhs;
}

constexpr bool any(SpiError value) noexcept {
  return value != SpiError::none;
}

struct SpiConfig {
  SpiRole role = SpiRole::master;
  SpiClockPhase phase = SpiClockPhase::firstEdge;
  SpiClockPolarity polarity = SpiClockPolarity::low;
  SpiBitOrder bitOrder = SpiBitOrder::msbFirst;
  SpiFrameSize frameSize = SpiFrameSize::bits8;
  SpiFrameFormat frameFormat = SpiFrameFormat::motorola;
  SpiBusMode busMode = SpiBusMode::fullDuplex;
  SpiBaudDivider baudDivider = SpiBaudDivider::div2;
  bool softwareSlaveManagement = false;
  bool internalSlaveSelectHigh = true;
  bool nssOutputEnable = false;
  bool crcEnable = false;
  std::uint16_t crcPolynomial = 7;
  bool enableAfterConfigure = true;
};

struct SpiCallbacks {
  void* context = nullptr;
  void (*txComplete)(void*) = nullptr;
  void (*rxComplete)(void*) = nullptr;
  void (*transferComplete)(void*) = nullptr;
  void (*error)(void*, SpiError) = nullptr;
};

// SpiDriver is a thin typed wrapper around one STM32 SPI peripheral instance.
// It keeps register composition at compile time and only stores transfer state
// needed for blocking and interrupt-driven operations. DMA, chip-select timing,
// and higher-level protocol framing deliberately stay outside this layer.
template <typename Block>
class SpiDriver {
 public:
  using BlockType = Block;
  using WordType = std::uint16_t;
  static constexpr std::size_t kDefaultPollLimit = 1'000'000u;

  SpiResult configure(const SpiConfig& config) {
    if (this->isBusy()) {
      return SpiResult::busy;
    }

    this->disableInterrupts();
    this->disable();

    this->config = config;
    this->lastError = SpiError::none;
    this->writeConfiguration();
    this->currentState = SpiState::ready;

    if (this->config.enableAfterConfigure) {
      this->enable();
    }

    return SpiResult::ok;
  }

  void setCallbacks(SpiCallbacks callbacks) noexcept { this->callbackSet = callbacks; }

  SpiState state() const noexcept { return this->currentState; }
  SpiError error() const noexcept { return this->lastError; }
  bool ready() const noexcept { return this->currentState == SpiState::ready; }

  void clearError() noexcept {
    this->lastError = SpiError::none;
    if (this->currentState == SpiState::error) {
      this->currentState = SpiState::ready;
    }
  }

  void enable() const {
    typename Block::CR1 cr1;
    cr1 |= spi::CR1::SPE::ENABLED;
  }

  void disable() const {
    typename Block::CR1 cr1;
    cr1 &= ~spi::CR1::SPE::MASK;
  }

  bool enabled() const {
    typename Block::CR1 cr1;
    return cr1 & spi::CR1::SPE::ENABLED;
  }

  bool busyFlag() const {
    typename Block::SR sr;
    return sr & spi::SR::BSY::BUSY;
  }

  bool txReady() const {
    typename Block::SR sr;
    return sr & spi::SR::TXE::EMPTY;
  }

  bool rxReady() const {
    typename Block::SR sr;
    return sr & spi::SR::RXNE::NOT_EMPTY;
  }

  // STM32 clears OVR by reading DR and then SR.
  void clearOverrun() const {
    typename Block::DR dr;
    typename Block::SR sr;
    (void)dr.template get<spi::DR::DATA>();
    (void)sr.template get<spi::SR::OVR>();
  }

  void flushReceive() const {
    typename Block::DR dr;
    while (this->rxReady()) {
      (void)dr.template get<spi::DR::DATA>();
    }
  }

  // Polling helpers use the data register width directly. In 8-bit mode the
  // low 8 bits of each word are transferred and the upper bits are ignored.
  SpiResult transmitPolling(const WordType* data,
                            std::size_t count,
                            std::size_t pollLimit = kDefaultPollLimit) {
    if ((count == 0u) || (data == nullptr) || !this->supportsTransmit()) {
      return SpiResult::invalidArgument;
    }
    if (this->isBusy()) {
      return SpiResult::busy;
    }

    this->currentState = SpiState::busyTx;
    this->lastError = SpiError::none;
    this->ensureEnabled();

    for (std::size_t index = 0; index < count; ++index) {
      if (!this->waitUntil([this]() { return this->txReady(); }, pollLimit)) {
        return this->failTimeout();
      }

      this->writeData(data[index]);

      if (this->config.busMode == SpiBusMode::fullDuplex) {
        if (!this->waitUntil([this]() { return this->rxReady(); }, pollLimit)) {
          return this->failTimeout();
        }
        (void)this->readData();
      }

      if (this->captureErrors()) {
        return SpiResult::error;
      }
    }

    if (!this->waitUntil([this]() { return this->txReady(); }, pollLimit)) {
      return this->failTimeout();
    }

    this->currentState = SpiState::ready;
    return SpiResult::ok;
  }

  SpiResult receivePolling(WordType* data,
                           std::size_t count,
                           std::size_t pollLimit = kDefaultPollLimit,
                           WordType fillWord = 0xFFFFu) {
    if ((count == 0u) || (data == nullptr) || !this->supportsReceive()) {
      return SpiResult::invalidArgument;
    }
    if (this->isBusy()) {
      return SpiResult::busy;
    }

    this->currentState = SpiState::busyRx;
    this->lastError = SpiError::none;
    this->ensureEnabled();

    for (std::size_t index = 0; index < count; ++index) {
      if (this->config.busMode == SpiBusMode::fullDuplex) {
        if (!this->waitUntil([this]() { return this->txReady(); }, pollLimit)) {
          return this->failTimeout();
        }
        this->writeData(fillWord);
      }

      if (!this->waitUntil([this]() { return this->rxReady(); }, pollLimit)) {
        return this->failTimeout();
      }

      data[index] = this->readData();

      if (this->captureErrors()) {
        return SpiResult::error;
      }
    }

    this->currentState = SpiState::ready;
    return SpiResult::ok;
  }

  SpiResult transferPolling(const WordType* txData,
                            WordType* rxData,
                            std::size_t count,
                            std::size_t pollLimit = kDefaultPollLimit) {
    if ((count == 0u) || (txData == nullptr) || (rxData == nullptr) || !this->supportsTransfer()) {
      return SpiResult::invalidArgument;
    }
    if (this->isBusy()) {
      return SpiResult::busy;
    }

    this->currentState = SpiState::busyTransfer;
    this->lastError = SpiError::none;
    this->ensureEnabled();

    for (std::size_t index = 0; index < count; ++index) {
      if (!this->waitUntil([this]() { return this->txReady(); }, pollLimit)) {
        return this->failTimeout();
      }

      this->writeData(txData[index]);

      if (!this->waitUntil([this]() { return this->rxReady(); }, pollLimit)) {
        return this->failTimeout();
      }

      rxData[index] = this->readData();

      if (this->captureErrors()) {
        return SpiResult::error;
      }
    }

    this->currentState = SpiState::ready;
    return SpiResult::ok;
  }

  SpiResult startTransmitInterrupt(const WordType* data, std::size_t count) {
    if ((count == 0u) || (data == nullptr) || !this->supportsTransmit()) {
      return SpiResult::invalidArgument;
    }
    if (this->isBusy()) {
      return SpiResult::busy;
    }

    this->beginAsync(SpiState::busyTx);
    this->txBuffer = data;
    this->txFillWord = 0xFFFFu;
    this->txRemaining = count;
    this->rxBuffer = nullptr;
    this->rxRemaining = (this->config.busMode == SpiBusMode::fullDuplex) ? count : 0u;
    this->currentOperation = OperationKind::tx;

    this->enableAsyncInterrupts();
    return SpiResult::ok;
  }

  SpiResult startReceiveInterrupt(WordType* data,
                                  std::size_t count,
                                  WordType fillWord = 0xFFFFu) {
    if ((count == 0u) || (data == nullptr) || !this->supportsReceive()) {
      return SpiResult::invalidArgument;
    }
    if (this->isBusy()) {
      return SpiResult::busy;
    }

    this->beginAsync(SpiState::busyRx);
    this->txBuffer = nullptr;
    this->txFillWord = fillWord;
    this->txRemaining = (this->config.busMode == SpiBusMode::fullDuplex) ? count : 0u;
    this->rxBuffer = data;
    this->rxRemaining = count;
    this->currentOperation = OperationKind::rx;

    this->enableAsyncInterrupts();
    return SpiResult::ok;
  }

  SpiResult startTransferInterrupt(const WordType* txData,
                                   WordType* rxData,
                                   std::size_t count) {
    if ((count == 0u) || (txData == nullptr) || (rxData == nullptr) || !this->supportsTransfer()) {
      return SpiResult::invalidArgument;
    }
    if (this->isBusy()) {
      return SpiResult::busy;
    }

    this->beginAsync(SpiState::busyTransfer);
    this->txBuffer = txData;
    this->txFillWord = 0xFFFFu;
    this->txRemaining = count;
    this->rxBuffer = rxData;
    this->rxRemaining = count;
    this->currentOperation = OperationKind::transfer;

    this->enableAsyncInterrupts();
    return SpiResult::ok;
  }

  void cancelInterrupt() {
    this->disableInterrupts();
    this->txBuffer = nullptr;
    this->rxBuffer = nullptr;
    this->txRemaining = 0u;
    this->rxRemaining = 0u;
    this->currentOperation = OperationKind::none;
    this->currentState = SpiState::ready;
  }

  // Call this directly from the real SPI interrupt handler. The driver keeps
  // the register sequencing local and only reports completion or error.
  void onInterrupt() {
    if (!this->isBusy()) {
      return;
    }

    if (this->captureErrors()) {
      this->disableInterrupts();
      this->invokeError();
      return;
    }

    if ((this->txRemaining > 0u) && this->txReady()) {
      this->writeNextAsyncWord();
      if (this->txRemaining == 0u) {
        this->disableTxeInterrupt();
      }
    }

    if ((this->rxRemaining > 0u) && this->rxReady()) {
      this->readNextAsyncWord();
      if (this->rxRemaining == 0u) {
        this->disableRxneInterrupt();
      }
    }

    if ((this->txRemaining == 0u) && (this->rxRemaining == 0u) &&
        (this->currentOperation != OperationKind::none)) {
      this->disableInterrupts();
      const auto completed = this->currentOperation;
      this->currentOperation = OperationKind::none;
      this->currentState = SpiState::ready;
      this->invokeCompletion(completed);
    }
  }

 private:
  enum class OperationKind { none, tx, rx, transfer };

  SpiConfig config{};
  SpiCallbacks callbackSet{};
  SpiState currentState = SpiState::reset;
  SpiError lastError = SpiError::none;
  const WordType* txBuffer = nullptr;
  WordType* rxBuffer = nullptr;
  WordType txFillWord = 0xFFFFu;
  std::size_t txRemaining = 0u;
  std::size_t rxRemaining = 0u;
  OperationKind currentOperation = OperationKind::none;

  bool isBusy() const noexcept {
    return (this->currentState == SpiState::busyTx) ||
           (this->currentState == SpiState::busyRx) ||
           (this->currentState == SpiState::busyTransfer);
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

  bool supportsTransmit() const noexcept {
    return (this->config.busMode == SpiBusMode::fullDuplex) ||
           (this->config.busMode == SpiBusMode::halfDuplexTx);
  }

  bool supportsReceive() const noexcept {
    return (this->config.busMode == SpiBusMode::fullDuplex) ||
           (this->config.busMode == SpiBusMode::receiveOnly) ||
           (this->config.busMode == SpiBusMode::halfDuplexRx);
  }

  bool supportsTransfer() const noexcept {
    return this->config.busMode == SpiBusMode::fullDuplex;
  }

  void ensureEnabled() const {
    if (!this->enabled()) {
      this->enable();
    }
  }

  SpiResult failTimeout() {
    this->currentState = SpiState::ready;
    return SpiResult::timeout;
  }

  void beginAsync(SpiState newState) {
    this->lastError = SpiError::none;
    this->currentState = newState;
    this->ensureEnabled();
  }

  void writeConfiguration() const {
    typename Block::CR1 cr1;
    typename Block::CR2 cr2;
    typename Block::CRCPR crcpr;

    const auto cr1Value = this->phaseBits(this->config.phase) |
                          this->polarityBits(this->config.polarity) |
                          this->roleBits(this->config.role) |
                          this->baudBits(this->config.baudDivider) |
                          this->bitOrderBits(this->config.bitOrder) |
                          this->frameSizeBits(this->config.frameSize) |
                          this->softwareNssBits(this->config.softwareSlaveManagement) |
                          this->internalSsBits(this->config.internalSlaveSelectHigh) |
                          this->crcEnableBits(this->config.crcEnable) |
                          this->busModeBits(this->config.busMode) |
                          spi::CR1::CRCNEXT::DATA_PHASE |
                          (this->config.enableAfterConfigure ? spi::CR1::SPE::ENABLED
                                                             : spi::CR1::SPE::DISABLED);

    cr1 = cr1Value;
    cr2 = this->frameFormatBits(this->config.frameFormat) |
          this->nssOutputBits(this->config.nssOutputEnable) |
          spi::CR2::RXDMAEN::DISABLED | spi::CR2::TXDMAEN::DISABLED |
          spi::CR2::ERRIE::DISABLED | spi::CR2::RXNEIE::DISABLED |
          spi::CR2::TXEIE::DISABLED;
    crcpr.template set<spi::CRCPR::POLYNOMIAL>(this->config.crcPolynomial);
  }

  static constexpr auto phaseBits(SpiClockPhase phase) {
    return (phase == SpiClockPhase::firstEdge) ? spi::CR1::CPHA::FIRST_EDGE
                                               : spi::CR1::CPHA::SECOND_EDGE;
  }

  static constexpr auto polarityBits(SpiClockPolarity polarity) {
    return (polarity == SpiClockPolarity::low) ? spi::CR1::CPOL::LOW : spi::CR1::CPOL::HIGH;
  }

  static constexpr auto roleBits(SpiRole role) {
    return (role == SpiRole::master) ? spi::CR1::MSTR::MASTER : spi::CR1::MSTR::SLAVE;
  }

  static constexpr auto baudBits(SpiBaudDivider divider) {
    switch (divider) {
      case SpiBaudDivider::div2: return spi::CR1::BR::DIV_2;
      case SpiBaudDivider::div4: return spi::CR1::BR::DIV_4;
      case SpiBaudDivider::div8: return spi::CR1::BR::DIV_8;
      case SpiBaudDivider::div16: return spi::CR1::BR::DIV_16;
      case SpiBaudDivider::div32: return spi::CR1::BR::DIV_32;
      case SpiBaudDivider::div64: return spi::CR1::BR::DIV_64;
      case SpiBaudDivider::div128: return spi::CR1::BR::DIV_128;
      case SpiBaudDivider::div256: return spi::CR1::BR::DIV_256;
    }
    return spi::CR1::BR::DIV_2;
  }

  static constexpr auto bitOrderBits(SpiBitOrder order) {
    return (order == SpiBitOrder::msbFirst) ? spi::CR1::LSBFIRST::MSB_FIRST
                                            : spi::CR1::LSBFIRST::LSB_FIRST;
  }

  static constexpr auto frameSizeBits(SpiFrameSize size) {
    return (size == SpiFrameSize::bits8) ? spi::CR1::DFF::BITS_8 : spi::CR1::DFF::BITS_16;
  }

  static constexpr auto softwareNssBits(bool software) {
    return software ? spi::CR1::SSM::SOFTWARE : spi::CR1::SSM::HARDWARE;
  }

  static constexpr auto internalSsBits(bool high) {
    return high ? spi::CR1::SSI::HIGH : spi::CR1::SSI::LOW;
  }

  static constexpr auto crcEnableBits(bool enabled) {
    return enabled ? spi::CR1::CRCEN::ENABLED : spi::CR1::CRCEN::DISABLED;
  }

  static constexpr auto busModeBits(SpiBusMode mode) {
    switch (mode) {
      case SpiBusMode::fullDuplex:
        return spi::CR1::BIDIMODE::TWO_LINE | spi::CR1::RXONLY::FULL_DUPLEX;
      case SpiBusMode::receiveOnly:
        return spi::CR1::BIDIMODE::TWO_LINE | spi::CR1::RXONLY::RECEIVE_ONLY;
      case SpiBusMode::halfDuplexTx:
        return spi::CR1::BIDIMODE::ONE_LINE | spi::CR1::BIDIOE::TRANSMIT;
      case SpiBusMode::halfDuplexRx:
        return spi::CR1::BIDIMODE::ONE_LINE | spi::CR1::BIDIOE::RECEIVE;
    }
    return spi::CR1::BIDIMODE::TWO_LINE | spi::CR1::RXONLY::FULL_DUPLEX;
  }

  static constexpr auto frameFormatBits(SpiFrameFormat format) {
    return (format == SpiFrameFormat::motorola) ? spi::CR2::FRF::MOTOROLA
                                                : spi::CR2::FRF::TI;
  }

  static constexpr auto nssOutputBits(bool enabled) {
    return enabled ? spi::CR2::SSOE::ENABLED : spi::CR2::SSOE::DISABLED;
  }

  void enableAsyncInterrupts() const {
    typename Block::CR2 cr2;
    cr2 |= spi::CR2::ERRIE::ENABLED;
    if (this->txRemaining > 0u) {
      cr2 |= spi::CR2::TXEIE::ENABLED;
    }
    if (this->rxRemaining > 0u) {
      cr2 |= spi::CR2::RXNEIE::ENABLED;
    }
  }

  void disableInterrupts() const {
    typename Block::CR2 cr2;
    cr2 &= ~(spi::CR2::TXEIE::MASK | spi::CR2::RXNEIE::MASK | spi::CR2::ERRIE::MASK);
  }

  void disableTxeInterrupt() const {
    typename Block::CR2 cr2;
    cr2 &= ~spi::CR2::TXEIE::MASK;
  }

  void disableRxneInterrupt() const {
    typename Block::CR2 cr2;
    cr2 &= ~spi::CR2::RXNEIE::MASK;
  }

  void writeData(WordType value) const {
    typename Block::DR dr;
    dr.template set<spi::DR::DATA>(value);
  }

  WordType readData() const {
    typename Block::DR dr;
    return dr.template get<spi::DR::DATA>();
  }

  void writeNextAsyncWord() {
    this->writeData(this->txBuffer != nullptr ? *this->txBuffer++ : this->txFillWord);
    --this->txRemaining;
  }

  void readNextAsyncWord() {
    const auto value = this->readData();
    if (this->rxBuffer != nullptr) {
      *this->rxBuffer++ = value;
    }
    --this->rxRemaining;
  }

  bool captureErrors() {
    typename Block::SR sr;
    SpiError errors = SpiError::none;

    if (sr & spi::SR::OVR::OVERRUN) {
      errors |= SpiError::overrun;
    }
    if (sr & spi::SR::MODF::FAULT) {
      errors |= SpiError::modeFault;
    }
    if (sr & spi::SR::CRCERR::ERROR) {
      errors |= SpiError::crc;
    }
    if (sr & spi::SR::FRE::ERROR) {
      errors |= SpiError::frameFormat;
    }

    if (any(errors)) {
      this->lastError |= errors;
      this->currentState = SpiState::error;
      return true;
    }

    return false;
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

