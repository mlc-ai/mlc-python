file(GLOB _test_sources "${CMAKE_CURRENT_SOURCE_DIR}/test*.cc")
add_executable(mlc_tests ${_test_sources})
set_target_properties(
  mlc_tests PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF
  CXX_STANDARD_REQUIRED ON
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
target_compile_definitions(mlc_tests PRIVATE MLC_CPPTESTS_EXPORTS)
if(MSVC)
  add_custom_command(
    TARGET mlc_tests
    POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:mlc_tests> $<TARGET_FILE_DIR:mlc_tests>
    COMMAND_EXPAND_LISTS
  )
endif()
add_cxx_warning(mlc_tests)
add_sanitizer_address(mlc_tests)
add_sanitizer_address(mlc-shared)
target_include_directories(mlc_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/dlpack/include")
target_link_libraries(mlc_tests PRIVATE mlc-shared)
add_googletest(mlc_tests)
