if(NOT DEFINED MMIOPP_CXX_COMPILER)
  message(FATAL_ERROR "MMIOPP_CXX_COMPILER is required.")
endif()

if(NOT DEFINED MMIOPP_SOURCE_FILE)
  message(FATAL_ERROR "MMIOPP_SOURCE_FILE is required.")
endif()

if(NOT DEFINED MMIOPP_SOURCE_DIR)
  message(FATAL_ERROR "MMIOPP_SOURCE_DIR is required.")
endif()

if(NOT DEFINED MMIOPP_BINARY_DIR)
  message(FATAL_ERROR "MMIOPP_BINARY_DIR is required.")
endif()

get_filename_component(_mmiopp_case_name "${MMIOPP_SOURCE_FILE}" NAME_WE)
set(_mmiopp_output_dir "${MMIOPP_BINARY_DIR}/compile_fail")
set(_mmiopp_object_file "${_mmiopp_output_dir}/${_mmiopp_case_name}.o")
set(_mmiopp_log_file "${_mmiopp_output_dir}/${_mmiopp_case_name}.log")

file(MAKE_DIRECTORY "${_mmiopp_output_dir}")

execute_process(
  COMMAND "${MMIOPP_CXX_COMPILER}"
          -std=c++17
          "-I${MMIOPP_SOURCE_DIR}/include"
          "-I${MMIOPP_SOURCE_DIR}/examples"
          -c "${MMIOPP_SOURCE_FILE}"
          -o "${_mmiopp_object_file}"
  RESULT_VARIABLE _mmiopp_result
  OUTPUT_VARIABLE _mmiopp_stdout
  ERROR_VARIABLE _mmiopp_stderr
)

file(WRITE "${_mmiopp_log_file}" "${_mmiopp_stdout}${_mmiopp_stderr}")

if(_mmiopp_result EQUAL 0)
  file(REMOVE "${_mmiopp_object_file}")
  message(FATAL_ERROR "Compile-fail test unexpectedly compiled: ${_mmiopp_case_name}")
endif()