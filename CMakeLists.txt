# Host build (Mac/Linux): the SDK-free inner rings plus their unit tests.
# The sim build of the same sources is wasm/build.sh — CMake never sees the MSFS SDK,
# which enforces the dependency rule: domain/application cannot include SDK headers.
cmake_minimum_required(VERSION 3.21)
project(fs24_pilot CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(FS24_SANITIZE "Enable ASan + UBSan on the unit tests" OFF)

# Same constraint as the WASM build: no exceptions in production code.
set(FS24_WARNINGS -Wall -Wextra -Wpedantic -Werror)

# ── domain + application (pure C++, no SDK) ───────────────────────────────────
file(GLOB_RECURSE FS24_CORE_SOURCES CONFIGURE_DEPENDS src/domain/*.cpp src/application/*.cpp)
if(FS24_CORE_SOURCES)
    add_library(fs24_core STATIC ${FS24_CORE_SOURCES})
    target_include_directories(fs24_core PUBLIC src)
    target_compile_options(fs24_core PRIVATE ${FS24_WARNINGS} -fno-exceptions)
else() # header-only until the first .cpp lands (M02)
    add_library(fs24_core INTERFACE)
    target_include_directories(fs24_core INTERFACE src)
endif()

# ── tests (Catch2 amalgamated, BSL-1.0) ───────────────────────────────────────
file(GLOB FS24_TEST_SOURCES CONFIGURE_DEPENDS tests/*.cpp)
add_executable(fs24_tests vendor/catch2/catch_amalgamated.cpp ${FS24_TEST_SOURCES})
target_include_directories(fs24_tests SYSTEM PRIVATE vendor)
target_link_libraries(fs24_tests PRIVATE fs24_core)
set_source_files_properties(${FS24_TEST_SOURCES} PROPERTIES COMPILE_OPTIONS "${FS24_WARNINGS}")

if(FS24_SANITIZE)
    set(FS24_SAN -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
    if(FS24_CORE_SOURCES)
        target_compile_options(fs24_core PRIVATE ${FS24_SAN})
    endif()
    target_compile_options(fs24_tests PRIVATE ${FS24_SAN})
    target_link_options(fs24_tests PRIVATE ${FS24_SAN})
endif()

enable_testing()
add_test(NAME fs24_tests COMMAND fs24_tests)
