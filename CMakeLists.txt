cmake_minimum_required(VERSION 3.25)
project(Aurelia VERSION 0.1.0 LANGUAGES CXX)

# -----------------------------------------------------------------------------
# C++ Standard & Compliance
# -----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# -----------------------------------------------------------------------------
# Compiler Warnings (Strict)
# -----------------------------------------------------------------------------
if(MSVC)
    add_compile_options(/W4 /WX)
else()
            add_compile_options(
        -Wall 
        -Wextra 
        -Werror 
        -pedantic 
        -Wconversion 
        -Wshadow 
        -Wnon-virtual-dtor
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
        -fno-exceptions  # Per user style guide: No exceptions
        -fno-rtti        # Optimization (optional, removable if dynamic_cast needed)
    )
endif()

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------
include(FetchContent)

# GoogleTest
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# -----------------------------------------------------------------------------
# Aurelia Core Library
# -----------------------------------------------------------------------------
file(GLOB_RECURSE SOURCES "src/*.cpp")
list(FILTER SOURCES EXCLUDE REGEX "src/main.cpp$")
file(GLOB_RECURSE HEADERS "src/*.hpp")

# We create a library so tests can link against it
add_library(AureliaLib STATIC ${SOURCES} ${HEADERS})
target_include_directories(AureliaLib PUBLIC src)

# -----------------------------------------------------------------------------
# Main Executable
# -----------------------------------------------------------------------------
add_executable(Aurelia src/main.cpp)
target_link_libraries(Aurelia PRIVATE AureliaLib)

# -----------------------------------------------------------------------------
# Testing
# -----------------------------------------------------------------------------
enable_testing()
add_subdirectory(tests)
