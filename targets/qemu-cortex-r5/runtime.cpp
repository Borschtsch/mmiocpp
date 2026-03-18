#include "semihosting.hpp"

#if defined(MMIOCPP_TARGET_COVERAGE)
#include <cstddef>
#include <cstdint>
extern "C" {
#include <gcov.h>
}
#endif

#if defined(MMIOCPP_TARGET_COVERAGE)
extern "C" const gcov_info* const __gcov_info_start[];
extern "C" const gcov_info* const __gcov_info_end[];
extern "C" [[noreturn]] void mmiocpp_target_exit(int status);
#endif
namespace {

#if defined(MMIOCPP_TARGET_COVERAGE)
constexpr std::size_t kCoverageScratchCapacity = 16u * 1024u;

alignas(std::uintptr_t) unsigned char g_coverageScratch[kCoverageScratchCapacity];
std::size_t g_coverageScratchUsed = 0u;
bool g_coverageWriteFailed = false;

struct CoverageFileContext {
  std::uintptr_t handle = mmiocpp::qemu::semihosting::kInvalidHandle;
};

void report_coverage_error(const char* message) {
  mmiocpp::qemu::semihosting::write0("Coverage export failed: ");
  mmiocpp::qemu::semihosting::write0(message);
  mmiocpp::qemu::semihosting::write0("\n");
  g_coverageWriteFailed = true;
}

[[noreturn]] void fail_coverage_runtime(const char* message) {
  report_coverage_error(message);
  mmiocpp_target_exit(3);
}
void* coverage_allocate(unsigned length, void*) {
  const auto alignment = static_cast<std::size_t>(alignof(std::uintptr_t));
  const auto alignedOffset = (g_coverageScratchUsed + alignment - 1u) & ~(alignment - 1u);
  const auto requestedLength = static_cast<std::size_t>(length);
  if ((alignedOffset + requestedLength) > kCoverageScratchCapacity) {
    report_coverage_error("scratch buffer exhausted");
    return nullptr;
  }

  void* memory = &g_coverageScratch[alignedOffset];
  g_coverageScratchUsed = alignedOffset + requestedLength;
  return memory;
}

void coverage_open_file(const char* filename, void* arg) {
  auto* context = static_cast<CoverageFileContext*>(arg);
  if (filename == nullptr) {
    report_coverage_error("gcov did not provide a file name");
    return;
  }

  context->handle = mmiocpp::qemu::semihosting::open_write_binary(filename);
  if (context->handle == mmiocpp::qemu::semihosting::kInvalidHandle) {
    report_coverage_error(filename);
  }
}

void coverage_write_chunk(const void* data, unsigned length, void* arg) {
  auto* context = static_cast<CoverageFileContext*>(arg);
  if (context->handle == mmiocpp::qemu::semihosting::kInvalidHandle) {
    return;
  }

  if (!mmiocpp::qemu::semihosting::write(context->handle, data, static_cast<std::uintptr_t>(length))) {
    report_coverage_error("could not write gcda data");
    context->handle = mmiocpp::qemu::semihosting::kInvalidHandle;
  }
}

void dump_coverage() {
  auto info = __gcov_info_start;
  if (info == __gcov_info_end) {
    report_coverage_error("no gcov info section was linked");
    return;
  }

  while (info != __gcov_info_end) {
    CoverageFileContext context{};
    g_coverageScratchUsed = 0u;
    __gcov_info_to_gcda(*info, coverage_open_file, coverage_write_chunk, coverage_allocate, &context);

    if ((context.handle != mmiocpp::qemu::semihosting::kInvalidHandle) &&
        !mmiocpp::qemu::semihosting::close(context.handle)) {
      report_coverage_error("could not close gcda file");
    }

    ++info;
  }
}
#endif

}  // namespace

#if defined(MMIOCPP_TARGET_COVERAGE)
extern "C" void abort(void) {
  fail_coverage_runtime("unexpected libgcov abort");
}

extern "C" std::size_t fread(void*, std::size_t, std::size_t, void*) {
  fail_coverage_runtime("unexpected libgcov file read");
}
#endif

extern "C" [[noreturn]] void mmiocpp_target_exit(int status) {
#if defined(MMIOCPP_TARGET_COVERAGE)
  dump_coverage();
  if ((status == 0) && g_coverageWriteFailed) {
    status = 2;
  }
#endif
  mmiocpp::qemu::semihosting::exit(status);
}
