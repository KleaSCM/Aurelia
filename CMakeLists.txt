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
    set(AURELIA_WARNINGS /W4 /WX)
else()
    set(AURELIA_WARNINGS
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
        -fno-exceptions 
        -fno-rtti
    )
endif()

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------
include(FetchContent)



# -----------------------------------------------------------------------------
# Aurelia Core Library
# -----------------------------------------------------------------------------
file(GLOB_RECURSE SOURCES "src/*.cpp")
list(FILTER SOURCES EXCLUDE REGEX "src/main.cpp$")
list(FILTER SOURCES EXCLUDE REGEX "src/Tools/Assembler/asm.cpp$")
file(GLOB_RECURSE HEADERS "src/*.hpp")

# We create a library so tests can link against it
add_library(AureliaLib STATIC ${SOURCES} ${HEADERS})
target_include_directories(AureliaLib PUBLIC src)
target_compile_options(AureliaLib PRIVATE ${AURELIA_WARNINGS})

# -----------------------------------------------------------------------------
# Main Executable
# -----------------------------------------------------------------------------
add_executable(Aurelia src/main.cpp)
target_link_libraries(Aurelia PRIVATE AureliaLib)
target_compile_options(Aurelia PRIVATE ${AURELIA_WARNINGS})

# -----------------------------------------------------------------------------
# Assembler Tool
# -----------------------------------------------------------------------------
add_executable(asm src/Tools/Assembler/asm.cpp)
target_link_libraries(asm PRIVATE AureliaLib)
target_compile_options(asm PRIVATE ${AURELIA_WARNINGS})


# -----------------------------------------------------------------------------
# Testing
# -----------------------------------------------------------------------------
enable_testing()
add_subdirectory(tests)
