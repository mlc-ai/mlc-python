cmake_minimum_required(VERSION 3.15)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/Utils/GitVersion.cmake)

message(STATUS "Found MLC_VERSION_GIT: ${MLC_VERSION_GIT}")

project(
  MLC
  VERSION ${MLC_VERSION_MAJOR}.${MLC_VERSION_MINOR}.${MLC_VERSION_PATCH}
  DESCRIPTION "MLC-Python"
  LANGUAGES C CXX
)

option(MLC_BUILD_TESTS "Build tests. This option will enable a test target `mlc_tests`." OFF)
option(MLC_BUILD_PY "Build Python bindings." OFF)

include(TestBigEndian)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/CPM.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/Utils/CxxUtils.cmake)
CPMAddPackage("gh:mlc-ai/mlc-backtrace@0.1.5")
CPMAddPackage("gh:potatomashed/PackageProject.cmake@1.13.1")

########## Target: `mlc` ##########

string(TIMESTAMP MLC_BUILD_TIME "%Y-%m-%dT%H:%M:%S")
TEST_BIG_ENDIAN(MLC_IS_BIG_ENDIAN)
message(STATUS "Found MLC_IS_BIG_ENDIAN: ${MLC_IS_BIG_ENDIAN}")
set_property(
  SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/cpp/c_api.cc"
  APPEND
  PROPERTY COMPILE_DEFINITIONS
  MLC_VERSION_GIT="${MLC_VERSION_GIT}"
  MLC_VERSION_MAJOR="${MLC_VERSION_MAJOR}"
  MLC_VERSION_MINOR="${MLC_VERSION_MINOR}"
  MLC_VERSION_PATCH="${MLC_VERSION_PATCH}"
  MLC_VERSION_COMMIT_NUM="${MLC_VERSION_COMMIT_NUM}"
  MLC_VERSION_COMMIT_SHA="${MLC_VERSION_COMMIT_SHA}"
  MLC_BUILD_TIME="${MLC_BUILD_TIME}"
  MLC_IS_BIG_ENDIAN="${MLC_IS_BIG_ENDIAN}"
)
set_property(
  SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/cpp/structure.cc"
  APPEND
  PROPERTY COMPILE_DEFINITIONS
  MLC_IS_BIG_ENDIAN=${MLC_IS_BIG_ENDIAN}
)

# target: `mlc_objs`
add_library(mlc_objs OBJECT
  "${CMAKE_CURRENT_SOURCE_DIR}/cpp/c_api.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/cpp/printer.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/cpp/structure.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/cpp/sym/sym.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/cpp/sym/text_format.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/cpp/sym/analyzer_const_int_bound.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/cpp/sym/analyzer_modular_set.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/cpp/sym/analyzer_rewrite_simplify.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/cpp/sym/analyzer_canonical_simplify.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/cpp/sym/analyzer_interval_set.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/cpp/sym/analyzer_transitive_comparisons.cc"
)
set_target_properties(
  mlc_objs PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF
  CXX_STANDARD_REQUIRED ON
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON
  PREFIX "lib"
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
)
add_cxx_warning(mlc_objs)
if(MSVC)
  target_compile_options(mlc_objs PRIVATE /bigobj)
endif()
target_include_directories(mlc_objs PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(mlc_objs PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/dlpack/include")
target_link_libraries(mlc_objs PUBLIC mlc::mlc_backtrace-static)
target_compile_definitions(mlc_objs PRIVATE MLC_EXPORTS)

# target: `mlc-static`
add_library(mlc-static STATIC $<TARGET_OBJECTS:mlc_objs>)
set_target_properties(
  mlc-static PROPERTIES
  OUTPUT_NAME "mlc-static"
  PREFIX "lib"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
)
target_include_directories(mlc-static PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(mlc-static PUBLIC mlc::mlc_backtrace-static)
packageProject(
  NAME mlc-static
  VERSION ${PROJECT_VERSION}
  BINARY_DIR ${PROJECT_BINARY_DIR}
  INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include
  INCLUDE_DESTINATION include
  DEPENDENCIES "mlc-backtrace 0.1.5"
  NAMESPACE mlc
  RUNTIME_DESTINATION "mlc"
  INSTALL_LIBDIR lib
  INSTALL_BINDIR lib
  CPACK YES
)

# target: `mlc-shared`
add_library(mlc-shared SHARED $<TARGET_OBJECTS:mlc_objs>)
set_target_properties(
  mlc-shared PROPERTIES
  OUTPUT_NAME "mlc"
  PREFIX "lib"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
)
target_include_directories(mlc-shared PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(mlc-shared PUBLIC mlc::mlc_backtrace-static)
add_debug_symbol_apple(mlc-shared "lib/mlc/")
packageProject(
  NAME mlc-shared
  VERSION ${PROJECT_VERSION}
  BINARY_DIR ${PROJECT_BINARY_DIR}
  INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include
  INCLUDE_DESTINATION include
  DEPENDENCIES "mlc-backtrace 0.1.5"
  NAMESPACE mlc
  RUNTIME_DESTINATION "mlc"
  INSTALL_LIBDIR lib
  INSTALL_BINDIR lib
  CPACK YES
)

# target: `mlc`
add_custom_target(mlc)
add_dependencies(mlc mlc-static mlc-shared)

########## Target: `mlc_py`  ##########

if(MLC_BUILD_PY)
  find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)
  file(GLOB _cython_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/python/mlc/_cython/*.pyx")
  set(_cython_outputs "")
  foreach(_in IN LISTS _cython_sources)
    get_filename_component(_file ${_in} NAME_WLE)
    set(_out "${CMAKE_BINARY_DIR}/_cython/${_file}_cython.cc")
    message(STATUS "Cythonize: ${_in} -> ${_out}")
    add_custom_command(
      OUTPUT "${_out}" DEPENDS "${_in}"
      COMMENT "Making `${_out}` from `${_in}"
      COMMAND Python::Interpreter -m cython "${_in}" --output-file "${_out}" --cplus
      VERBATIM
    )
    list(APPEND _cython_outputs "${_out}")
  endforeach()
  Python_add_library(mlc_py MODULE ${_cython_outputs} WITH_SOABI)
  set_target_properties(mlc_py PROPERTIES OUTPUT_NAME "core")
  target_include_directories(mlc_py PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
  target_include_directories(mlc_py PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/dlpack/include")
  install(TARGETS mlc_py DESTINATION _cython/)
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/mlc/ DESTINATION include/mlc/)
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/dlpack/include/dlpack/ DESTINATION include/dlpack/)
  install(DIRECTORY $<TARGET_PROPERTY:mlc_backtrace-static,INTERFACE_INCLUDE_DIRECTORIES>/mlc/backtrace/ DESTINATION include/mlc/backtrace/)
  install(FILES $<TARGET_FILE:mlc_backtrace-static> DESTINATION lib/mlc/)
endif(MLC_BUILD_PY)

########## Adding tests ##########

if(${PROJECT_NAME} STREQUAL ${CMAKE_PROJECT_NAME})
  if(MLC_BUILD_TESTS)
    enable_testing()
    message(STATUS "Enable Testing")
    include(cmake/Utils/AddGoogleTest.cmake)
    add_subdirectory(tests/cpp/)
  endif()
endif()
