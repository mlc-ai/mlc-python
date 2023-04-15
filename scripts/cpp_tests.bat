@echo off
setlocal enabledelayedexpansion

set BUILD_REGISTRY=ON
set BUILD_TYPE=RelWithDebInfo
set BUILD_DIR=build-cpp-tests/
set GTEST_COLOR=1

if exist %BUILD_DIR% rmdir /s /q %BUILD_DIR%
cmake -S . -B %BUILD_DIR% ^
    -DMLC_BUILD_REGISTRY=%BUILD_REGISTRY% ^
    -DMLC_BUILD_TESTS=ON ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
if errorlevel 1 goto :error

cmake --build %BUILD_DIR% ^
      --config %BUILD_TYPE% ^
      --target mlc_tests ^
      -j %NUMBER_OF_PROCESSORS% ^
      -- -verbosity:detailed
if errorlevel 1 goto :error

ctest -V -C %BUILD_TYPE% --test-dir %BUILD_DIR% --output-on-failure
if errorlevel 1 goto :error

rmdir /s /q %BUILD_DIR%
goto :eof

:error
echo Script failed with error #%errorlevel%.
exit /b %errorlevel%
:eof

endlocal
