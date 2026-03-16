set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(_mmiopp_arm_gcc_hint "$ENV{MMIOPP_ARM_GCC}")
set(_mmiopp_arm_gxx_hint "$ENV{MMIOPP_ARM_GXX}")

if(WIN32)
  file(GLOB _mmiopp_arm_gcc_candidates
    "C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/*/bin/arm-none-eabi-gcc.exe"
  )
  file(GLOB _mmiopp_arm_gxx_candidates
    "C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/*/bin/arm-none-eabi-g++.exe"
  )

  if(_mmiopp_arm_gcc_candidates)
    list(SORT _mmiopp_arm_gcc_candidates COMPARE NATURAL ORDER DESCENDING)
    list(GET _mmiopp_arm_gcc_candidates 0 _mmiopp_arm_gcc_default)
  endif()

  if(_mmiopp_arm_gxx_candidates)
    list(SORT _mmiopp_arm_gxx_candidates COMPARE NATURAL ORDER DESCENDING)
    list(GET _mmiopp_arm_gxx_candidates 0 _mmiopp_arm_gxx_default)
  endif()
endif()

if(_mmiopp_arm_gcc_hint)
  set(MMIOPP_ARM_GCC "${_mmiopp_arm_gcc_hint}" CACHE FILEPATH "Path to arm-none-eabi-gcc." FORCE)
elseif(_mmiopp_arm_gcc_default)
  set(MMIOPP_ARM_GCC "${_mmiopp_arm_gcc_default}" CACHE FILEPATH "Path to arm-none-eabi-gcc." FORCE)
else()
  find_program(MMIOPP_ARM_GCC
    NAMES arm-none-eabi-gcc arm-none-eabi-gcc.exe
    HINTS
      "C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi"
    PATH_SUFFIXES bin
    DOC "Path to arm-none-eabi-gcc."
  )
endif()

if(_mmiopp_arm_gxx_hint)
  set(MMIOPP_ARM_GXX "${_mmiopp_arm_gxx_hint}" CACHE FILEPATH "Path to arm-none-eabi-g++.exe" FORCE)
elseif(_mmiopp_arm_gxx_default)
  set(MMIOPP_ARM_GXX "${_mmiopp_arm_gxx_default}" CACHE FILEPATH "Path to arm-none-eabi-g++.exe" FORCE)
else()
  find_program(MMIOPP_ARM_GXX
    NAMES arm-none-eabi-g++ arm-none-eabi-g++.exe
    HINTS
      "C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi"
    PATH_SUFFIXES bin
    DOC "Path to arm-none-eabi-g++."
  )
endif()

if(NOT MMIOPP_ARM_GCC)
  message(FATAL_ERROR "arm-none-eabi-gcc was not found. Set MMIOPP_ARM_GCC or install the Arm GNU Toolchain.")
endif()

if(NOT MMIOPP_ARM_GXX)
  message(FATAL_ERROR "arm-none-eabi-g++ was not found. Set MMIOPP_ARM_GXX or install the Arm GNU Toolchain.")
endif()

set(CMAKE_C_COMPILER "${MMIOPP_ARM_GCC}" CACHE FILEPATH "" FORCE)
set(CMAKE_CXX_COMPILER "${MMIOPP_ARM_GXX}" CACHE FILEPATH "" FORCE)
set(CMAKE_ASM_COMPILER "${MMIOPP_ARM_GCC}" CACHE FILEPATH "" FORCE)

get_filename_component(_mmiopp_arm_bin_dir "${MMIOPP_ARM_GCC}" DIRECTORY)
get_filename_component(_mmiopp_arm_root_dir "${_mmiopp_arm_bin_dir}" DIRECTORY)

set(CMAKE_EXECUTABLE_SUFFIX ".elf")
set(CMAKE_FIND_ROOT_PATH "${_mmiopp_arm_root_dir}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)