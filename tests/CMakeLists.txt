include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.4.0
)

FetchContent_MakeAvailable(Catch2)

# Discover all test files in the tests directory
file(GLOB TEST_SOURCES "*.cpp")

add_executable(AureliaTests ${TEST_SOURCES})

# Link against the main library and Catch2
target_link_libraries(AureliaTests PRIVATE AureliaLib Catch2::Catch2WithMain)
target_compile_options(AureliaTests PRIVATE ${AURELIA_WARNINGS})

include(Catch)
catch_discover_tests(AureliaTests)
