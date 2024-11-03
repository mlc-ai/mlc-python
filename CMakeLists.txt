cmake_minimum_required(VERSION 3.15)

project(
  mlc
  VERSION 0.0.8
  DESCRIPTION "MLC-Python"
  LANGUAGES C CXX
)

option(MLC_BUILD_TESTS "Build tests. This option will enable a test target `mlc_tests`." OFF)
option(MLC_BUILD_PY "Build Python bindings." OFF)
option(MLC_BUILD_REGISTRY
  "Support for objects with non-static type indices. When turned on, \
  targets linked against `mlc` will allow objects that comes with non-pre-defined type indices, \
  so that the object hierarchy could expand without limitation. \
  This will require the downstream targets to link against target `mlc_registry` to be effective."
  OFF
)

include(cmake/Utils/CxxWarning.cmake)
include(cmake/Utils/Sanitizer.cmake)
include(cmake/Utils/Library.cmake)
include(cmake/Utils/AddLibbacktrace.cmake)
include(cmake/Utils/DebugSymbol.cmake)

########## Target: `dlpack_header` ##########

add_library(dlpack_header INTERFACE)
target_include_directories(dlpack_header INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/dlpack/include")

########## Target: `mlc` ##########

add_library(mlc INTERFACE)
target_link_libraries(mlc INTERFACE dlpack_header)
target_compile_features(mlc INTERFACE cxx_std_17)
target_include_directories(mlc INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

########## Target: `mlc_registry` ##########

if (MLC_BUILD_REGISTRY)
  add_library(mlc_registry_objs OBJECT
    "${CMAKE_CURRENT_SOURCE_DIR}/cpp/c_api.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/cpp/c_api_tests.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/cpp/traceback.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/cpp/traceback_win.cc"
  )
  set_target_properties(
    mlc_registry_objs PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF
    CXX_STANDARD_REQUIRED ON
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    PREFIX "lib"
  )
  add_cxx_warning(mlc_registry_objs)
  target_link_libraries(mlc_registry_objs PRIVATE dlpack_header)
  target_include_directories(mlc_registry_objs PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
  add_target_from_obj(mlc_registry mlc_registry_objs)
  if (TARGET libbacktrace)
    target_link_libraries(mlc_registry_objs PRIVATE libbacktrace)
    target_link_libraries(mlc_registry_shared PRIVATE libbacktrace)
    target_link_libraries(mlc_registry_static PRIVATE libbacktrace)
    add_debug_symbol_apple(mlc_registry_shared "lib/")
  endif ()
  install(TARGETS mlc_registry_static DESTINATION "lib/")
  install(TARGETS mlc_registry_shared DESTINATION "lib/")
endif (MLC_BUILD_REGISTRY)

########## Target: `mlc_py`  ##########

if (MLC_BUILD_PY)
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
  target_link_libraries(mlc_py PUBLIC mlc)
  install(TARGETS mlc_py DESTINATION _cython/)
endif (MLC_BUILD_PY)

########## Adding tests ##########

if (${PROJECT_NAME} STREQUAL ${CMAKE_PROJECT_NAME})
  if (MLC_BUILD_TESTS)
    enable_testing()
    message(STATUS "Enable Testing")
    include(cmake/Utils/AddGoogleTest.cmake)
    add_subdirectory(tests/cpp/)
  endif()
endif ()
