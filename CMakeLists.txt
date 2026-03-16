cmake_minimum_required(VERSION 3.16)

project(mmiocpp VERSION 0.1.0 LANGUAGES CXX)

option(MMIOPP_BUILD_DEMO "Build the host demo executable" ON)
option(MMIOPP_BUILD_TESTS "Build the host compile-check executable" ON)
option(MMIOPP_BUILD_QEMU_TARGET "Build the Cortex-M3 QEMU target runtime tests" OFF)
option(MMIOPP_BUILD_QEMU_R5_TARGET "Build the Cortex-R5 QEMU target runtime tests" OFF)

set(MMIOPP_QEMU_SYSTEM_ARM "" CACHE FILEPATH "Path to qemu-system-arm.")
set(MMIOPP_QEMU_MACHINE "mps2-an385" CACHE STRING "QEMU machine for the Cortex-M target.")
set(MMIOPP_QEMU_CPU "cortex-m3" CACHE STRING "QEMU CPU for the Cortex-M target.")
set(MMIOPP_QEMU_R5_MACHINE "none" CACHE STRING "QEMU machine for the Cortex-R5 target.")
set(MMIOPP_QEMU_R5_CPU "cortex-r5" CACHE STRING "QEMU CPU for the Cortex-R5 target.")
set(MMIOPP_QEMU_R5_MEMORY "32M" CACHE STRING "RAM size for the Cortex-R5 target.")

add_library(mmiopp INTERFACE)
add_library(mmiopp::mmiopp ALIAS mmiopp)

target_include_directories(mmiopp
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_features(mmiopp INTERFACE cxx_std_17)

function(mmiopp_copy_gnu_runtime target_name)
  if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    get_filename_component(_compiler_bin "${CMAKE_CXX_COMPILER}" DIRECTORY)
    set(_runtime_dlls
      libstdc++-6.dll
      libgcc_s_seh-1.dll
      libwinpthread-1.dll
    )

    foreach(_dll IN LISTS _runtime_dlls)
      find_file(_dll_path "${_dll}" PATHS "${_compiler_bin}" NO_DEFAULT_PATH)
      if(_dll_path)
        add_custom_command(TARGET "${target_name}" POST_BUILD
          COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                  "${_dll_path}"
                  "$<TARGET_FILE_DIR:${target_name}>"
          VERBATIM
        )
      endif()
      unset(_dll_path CACHE)
    endforeach()
  endif()
endfunction()

if(MMIOPP_BUILD_DEMO)
  add_executable(mmio_demo mmio_demo.cpp)
  target_link_libraries(mmio_demo PRIVATE mmiopp::mmiopp)
  target_include_directories(mmio_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/examples)
  mmiopp_copy_gnu_runtime(mmio_demo)
endif()

if(MMIOPP_BUILD_TESTS)
  enable_testing()

  add_executable(mmiopp_tests tests/mmio_tests.cpp)
  target_link_libraries(mmiopp_tests PRIVATE mmiopp::mmiopp)
  target_include_directories(mmiopp_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/examples)
  mmiopp_copy_gnu_runtime(mmiopp_tests)

  add_test(NAME mmiopp_host_compile_checks COMMAND "$<TARGET_FILE_NAME:mmiopp_tests>")
  set_tests_properties(mmiopp_host_compile_checks PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  )

  set(_mmiopp_compile_fail_script "${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_compile_fail_test.cmake")
  file(GLOB _mmiopp_compile_fail_cases LIST_DIRECTORIES false CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/compile_fail/*.cpp"
  )

  foreach(_mmiopp_compile_fail_case IN LISTS _mmiopp_compile_fail_cases)
    get_filename_component(_mmiopp_compile_fail_name "${_mmiopp_compile_fail_case}" NAME_WE)
    add_test(
      NAME "compile_fail_${_mmiopp_compile_fail_name}"
      COMMAND "${CMAKE_COMMAND}"
        "-DMMIOPP_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
        "-DMMIOPP_SOURCE_FILE=${_mmiopp_compile_fail_case}"
        "-DMMIOPP_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
        "-DMMIOPP_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}"
        -P "${_mmiopp_compile_fail_script}"
    )
  endforeach()
endif()

if(MMIOPP_BUILD_QEMU_TARGET)
  enable_testing()
  enable_language(ASM)

  if(NOT MMIOPP_QEMU_SYSTEM_ARM AND DEFINED ENV{MMIOPP_QEMU_SYSTEM_ARM} AND NOT "$ENV{MMIOPP_QEMU_SYSTEM_ARM}" STREQUAL "")
    set(MMIOPP_QEMU_SYSTEM_ARM "$ENV{MMIOPP_QEMU_SYSTEM_ARM}")
  endif()

  if(NOT MMIOPP_QEMU_SYSTEM_ARM)
    find_program(MMIOPP_QEMU_SYSTEM_ARM
      NAMES qemu-system-arm qemu-system-arm.exe
      HINTS
        "C:/Program Files/qemu"
    )
  endif()

  set(_mmiopp_qemu_map_file "${CMAKE_CURRENT_BINARY_DIR}/mmiopp_qemu_tests.map")

  add_executable(mmiopp_qemu_tests
    targets/qemu-cortex-m/startup.S
    targets/qemu-cortex-m/runtime.cpp
    targets/qemu-cortex-m/mmio_target_tests.cpp
  )
  target_link_libraries(mmiopp_qemu_tests PRIVATE mmiopp::mmiopp)
  target_include_directories(mmiopp_qemu_tests
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/examples
      ${CMAKE_CURRENT_SOURCE_DIR}/targets/qemu-cortex-m
  )
  target_compile_options(mmiopp_qemu_tests
    PRIVATE
      -mcpu=cortex-m3
      -mthumb
      -ffreestanding
      -fno-exceptions
      -fno-rtti
      -fno-threadsafe-statics
      -fno-use-cxa-atexit
      -ffunction-sections
      -fdata-sections
  )
  target_link_options(mmiopp_qemu_tests
    PRIVATE
      -mcpu=cortex-m3
      -mthumb
      -nostdlib
      -Wl,--gc-sections
      "-Wl,-T,${CMAKE_CURRENT_SOURCE_DIR}/targets/qemu-cortex-m/linker.ld"
      "-Wl,-Map,${_mmiopp_qemu_map_file}"
  )
  set_target_properties(mmiopp_qemu_tests PROPERTIES SUFFIX ".elf")

  if(MMIOPP_QEMU_SYSTEM_ARM)
    add_custom_target(run_mmiopp_qemu_tests
      COMMAND "${MMIOPP_QEMU_SYSTEM_ARM}"
              -machine "${MMIOPP_QEMU_MACHINE}"
              -cpu "${MMIOPP_QEMU_CPU}"
              -nographic
              -monitor none
              -serial none
              -semihosting-config enable=on,target=native
              -kernel "$<TARGET_FILE_NAME:mmiopp_qemu_tests>"
      DEPENDS mmiopp_qemu_tests
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
      USES_TERMINAL
      VERBATIM
    )

    add_test(NAME mmiopp_qemu_runtime
      COMMAND "${MMIOPP_QEMU_SYSTEM_ARM}"
              -machine "${MMIOPP_QEMU_MACHINE}"
              -cpu "${MMIOPP_QEMU_CPU}"
              -nographic
              -monitor none
              -serial none
              -semihosting-config enable=on,target=native
              -kernel "$<TARGET_FILE_NAME:mmiopp_qemu_tests>"
    )
    set_tests_properties(mmiopp_qemu_runtime PROPERTIES
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    )
  endif()
endif()

if(MMIOPP_BUILD_QEMU_R5_TARGET)
  enable_testing()
  enable_language(ASM)

  if(NOT MMIOPP_QEMU_SYSTEM_ARM AND DEFINED ENV{MMIOPP_QEMU_SYSTEM_ARM} AND NOT "$ENV{MMIOPP_QEMU_SYSTEM_ARM}" STREQUAL "")
    set(MMIOPP_QEMU_SYSTEM_ARM "$ENV{MMIOPP_QEMU_SYSTEM_ARM}")
  endif()

  if(NOT MMIOPP_QEMU_SYSTEM_ARM)
    find_program(MMIOPP_QEMU_SYSTEM_ARM
      NAMES qemu-system-arm qemu-system-arm.exe
      HINTS
        "C:/Program Files/qemu"
    )
  endif()

  set(_mmiopp_qemu_r5_map_file "${CMAKE_CURRENT_BINARY_DIR}/mmiopp_qemu_r5_tests.map")

  add_executable(mmiopp_qemu_r5_tests
    targets/qemu-cortex-r5/startup.S
    targets/qemu-cortex-r5/runtime.cpp
    targets/qemu-cortex-r5/mmio_target_tests.cpp
  )
  target_link_libraries(mmiopp_qemu_r5_tests PRIVATE mmiopp::mmiopp)
  target_include_directories(mmiopp_qemu_r5_tests
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/examples
      ${CMAKE_CURRENT_SOURCE_DIR}/targets/qemu-cortex-r5
  )
  target_compile_options(mmiopp_qemu_r5_tests
    PRIVATE
      -mcpu=cortex-r5
      -marm
      -ffreestanding
      -fno-exceptions
      -fno-rtti
      -fno-threadsafe-statics
      -fno-use-cxa-atexit
      -ffunction-sections
      -fdata-sections
  )
  target_link_options(mmiopp_qemu_r5_tests
    PRIVATE
      -mcpu=cortex-r5
      -marm
      -nostdlib
      -Wl,--gc-sections
      "-Wl,-T,${CMAKE_CURRENT_SOURCE_DIR}/targets/qemu-cortex-r5/linker.ld"
      "-Wl,-Map,${_mmiopp_qemu_r5_map_file}"
  )
  set_target_properties(mmiopp_qemu_r5_tests PROPERTIES SUFFIX ".elf")

  if(MMIOPP_QEMU_SYSTEM_ARM)
    add_custom_target(run_mmiopp_qemu_r5_tests
      COMMAND "${MMIOPP_QEMU_SYSTEM_ARM}"
              -machine "${MMIOPP_QEMU_R5_MACHINE}"
              -cpu "${MMIOPP_QEMU_R5_CPU}"
              -m "${MMIOPP_QEMU_R5_MEMORY}"
              -display none
              -monitor none
              -serial none
              -semihosting-config enable=on,target=native
              -device "loader,file=$<TARGET_FILE_NAME:mmiopp_qemu_r5_tests>,cpu-num=0"
      DEPENDS mmiopp_qemu_r5_tests
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
      USES_TERMINAL
      VERBATIM
    )

    add_test(NAME mmiopp_qemu_r5_runtime
      COMMAND "${MMIOPP_QEMU_SYSTEM_ARM}"
              -machine "${MMIOPP_QEMU_R5_MACHINE}"
              -cpu "${MMIOPP_QEMU_R5_CPU}"
              -m "${MMIOPP_QEMU_R5_MEMORY}"
              -display none
              -monitor none
              -serial none
              -semihosting-config enable=on,target=native
              -device "loader,file=$<TARGET_FILE_NAME:mmiopp_qemu_r5_tests>,cpu-num=0"
    )
    set_tests_properties(mmiopp_qemu_r5_runtime PROPERTIES
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    )
  endif()
endif()
