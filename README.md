# mmiocpp
MMIO++ is an embedded memory‑mapped I/O access framework developed in modern C++.

The framework brings the familiar, concise access style of classic C register macros—the style embedded developers know well, while eliminating their pitfalls. With MMIO++, you get code that looks like clean, direct register manipulation, yet is backed by strict, modern C++ type safety and zero‑overhead abstractions.

Designed primarily for MCUs with integrated peripherals, MMIO++ also supports external devices that expose register maps over buses such as SPI or I²C, thanks to its flexible register description style.

## API Style
Register definitions look as simple as below
```cpp
/* Define register */
struct SPI_CR : mmio::Register<SPI_CR>
{
  /* Add necessary fields bit or value fields */
  struct SPIEN : mmio::BitField<SPI_CR, 0, 1> {
    static constexpr auto DISABLE = value(0); // Specify enumerations for bit-fields
    static constexpr auto ENABLE = value(1);
  };

  struct SWRST : mmio::BitField<SPI_CR, 7, 1> {
    static constexpr auto IDLE = value(0);
    static constexpr auto RESET = value(1);
  };

  struct DLY : mmio::ValueField<SPI_CR, 8, 2, uint8_t> {}; // Specify underlying data type for value
};
```

Each field exposes two distinct public concepts, and the API keeps them separate:

- `FIELD::VALUE_NAME`: an encoded register value for whole-register writes, predicates, and bit-field set operations.
- `FIELD::MASK`: the auto-derived bit mask for clear/toggle operations.

Numeric value fields use `FIELD::value(x)` for encoded values. Bit fields expose named encoded states only; their raw `value(...)` helper is reserved for field definitions.

Access policies fit into the same surface instead of introducing a second API:

- default fields are plain `Rw`, so normal register definitions do not need an explicit access tag
- when needed, access tags such as `Ro`, `Wo`, `W1c`, `W1s`, `W0c`, `W0s`, and `Rc` add stricter semantics without changing the operator surface
- symmetric fields define named constants with `value(...)`; asymmetric fields define readable states with `state(...)` and write actions with `action(...)`

Registers additionally accept encoded values directly through `set(...)`, while `set<FIELD>(raw)` remains the masked field-update form only when the field access semantics allow masked replacement.

Examples:

```cpp
/* Define concrete register */
SPI_CR::Instance<0xFFFE0000u> spiCr;
/* Or use its shadow variable */
SPI_CR spiCrShadow;

spiCrShadow = SPI_CR::SWRST::RESET;
cpiCr = spiCrShadow;

/* Use familiar C macro syntax to set field values or clear them */
spiCr |= SPI_CR::SPIEN::ENABLE | SPI_CR::DLY::value(7);
/* `FIELD::MASK` and `REGISTER::MASK` are derived automatically from the field layout. No raw register read/write API is exposed. */
spiCr &= ~SPI_CR::SWRST::MASK;
spiCr &= ~SPI_CR::MASK;

/* For those who like function calls more */
spiCr.set(SPI_CR::SPIEN::ENABLE | SPI_CR::DLY::value(7));
spiMr.set<SPI_MR::DLY>(7);

/* How about checking if value is set? */
const bool isMaster = spiMr & SPI_MR::MSTR::MASTER;
```

## Layout

- `include/mmio.hpp`: public header with the core MMIO abstractions only. Registers always dereference their bound MMIO address directly.
- `examples/spi_example_registers.hpp`: example SPI register map used by the demo and tests.
- `examples/access_example_registers.hpp`: example access-semantics register map covering RO, WO, W1C, W1S, and zero-write semantics.
- `examples/stm32f429/spi_registers.hpp`: STM32F429 full SPI/I2S block register definitions and peripheral aliases derived from the SDK register map.
- `examples/stm32f429/spi_driver.hpp`: header-only STM32F429 SPI polling/interrupt driver abstraction built on the typed register map.
- `examples/stm32f429/i2s_driver.hpp`: header-only STM32F429 I2S polling/interrupt driver abstraction with optional I2Sxext full-duplex support.
- `mmio_demo.cpp`: compile-checked usage example. Host builds do not execute the MMIO sequence.
- `tests/mmio_tests.cpp`: host-side positive compile checks for the public API shape.
- `tests/compile_fail/*.cpp`: negative compile tests that lock down cross-register misuse and mask/value confusion.
- `targets/qemu-cortex-m/`: freestanding Cortex-M3 target harness, startup code, linker script, and runtime MMIO tests for QEMU.
- `targets/qemu-cortex-r5/`: freestanding Cortex-R5 target harness, startup code, linker script, and runtime MMIO tests for upstream QEMU.
- `cmake/toolchains/arm-none-eabi-gcc.cmake`: Arm GNU bare-metal toolchain file for the Cortex-M/QEMU build.
- `cmake/run_compile_fail_test.cmake`: CTest helper that compiles each negative test and expects failure.
- `CMakePresets.json`: direct CMake entry points for host build, host test, Cortex-M3 QEMU build/run/test, and Cortex-R5 QEMU build/run/test on Windows.
- `scripts/bootstrap.ps1`: Windows tool bootstrap. It installs native tools and the repo-local WinLibs fallback that the direct presets use.
- `scripts/bootstrap.sh`: Linux/macOS native tool bootstrap.
- `Brewfile`: macOS tool manifest for the Homebrew bootstrap path.

## Bootstrap

Windows:

```powershell
pwsh ./scripts/bootstrap.ps1
```

Windows portable fallback only:

```powershell
pwsh ./scripts/bootstrap.ps1 -PortableFallbackOnly
```

Linux or macOS:

```sh
bash ./scripts/bootstrap.sh
```

## Build

Recommended direct CMake commands on Windows:

```powershell
cmake --workflow --preset host
cmake --workflow --preset host-test
cmake --workflow --preset qemu-build
cmake --workflow --preset qemu-run
cmake --workflow --preset qemu-test
cmake --workflow --preset qemu-r5-build
cmake --workflow --preset qemu-r5-run
cmake --workflow --preset qemu-r5-test
```

Those presets use the known-good Windows tool layout for this repo:

- the repo-local WinLibs toolchain in `.local/winlibs`
- the Arm GNU Toolchain in its standard `Program Files (x86)` location
- QEMU in `C:\Program Files\qemu`

Direct host CMake commands on Linux or macOS:

```sh
cmake -S . -B build/host -G Ninja
cmake --build build/host
```

Direct Arm Cortex-M3/QEMU CMake commands on Linux or macOS:

```sh
cmake -S . -B build/qemu -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi-gcc.cmake \
  -DMMIOPP_BUILD_DEMO=OFF \
  -DMMIOPP_BUILD_TESTS=OFF \
  -DMMIOPP_BUILD_QEMU_TARGET=ON
cmake --build build/qemu
```

## Test

Host compile checks and compile-fail coverage through direct CMake on Windows:

```powershell
cmake --workflow --preset host-test
```

QEMU Cortex-M3 runtime tests through direct CMake on Windows:

```powershell
cmake --workflow --preset qemu-test
cmake --workflow --preset qemu-run
```

QEMU Cortex-R5 runtime tests through direct CMake on Windows:

```powershell
cmake --workflow --preset qemu-r5-build
cmake --workflow --preset qemu-r5-run
cmake --workflow --preset qemu-r5-test
```

The Cortex-R5 path uses upstream `qemu-system-arm` with the `none` machine, a real `cortex-r5` CPU model, semihosting for pass/fail reporting, and a reserved RAM window for the test register addresses.

Host compile checks and compile-fail coverage on Linux or macOS:

```sh
ctest --test-dir build/host --output-on-failure
```

QEMU Cortex-M3 runtime tests on Linux or macOS:

```sh
ctest --test-dir build/qemu --output-on-failure
```

The host checks validate API shape, misuse rejection, and compile-smoke coverage for the STM32F429 register-map and driver examples. Runtime register behavior is validated on both Cortex-M3 and Cortex-R5 QEMU targets so the library executes against real target CPU address spaces instead of a desktop MMIO shim.
