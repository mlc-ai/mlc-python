set(gtest_force_shared_crt ON CACHE BOOL "Always use msvcrt.dll" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(BUILD_GTEST ON CACHE BOOL "" FORCE)
set(GOOGLETEST_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/googletest)

add_subdirectory(${GOOGLETEST_ROOT})
include(GoogleTest)

set_target_properties(gtest gtest_main
  PROPERTIES
    EXPORT_COMPILE_COMMANDS OFF
    EXCLUDE_FROM_ALL ON
    FOLDER 3rdparty
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Install gtest and gtest_main
install(TARGETS gtest gtest_main DESTINATION "lib/")

# Mark advanced variables
mark_as_advanced(
  BUILD_GMOCK BUILD_GTEST BUILD_SHARED_LIBS
  gmock_build_tests gtest_build_samples gtest_build_tests
  gtest_disable_pthreads gtest_force_shared_crt gtest_hide_internal_symbols
)

macro(add_googletest target_name)
  add_test(
    NAME ${target_name}
    COMMAND ${target_name}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
  target_link_libraries(${target_name} PRIVATE gtest_main)
  gtest_discover_tests(${target_name}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    DISCOVERY_MODE PRE_TEST
    PROPERTIES
      VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  )
  set_target_properties(${target_name} PROPERTIES FOLDER tests)
endmacro()
