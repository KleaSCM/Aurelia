include(GoogleTest)

# Discover all test files in the tests directory
file(GLOB TEST_SOURCES "*.cpp")

add_executable(AureliaTests ${TEST_SOURCES})

# Link against the main library and GTest
target_link_libraries(AureliaTests PRIVATE AureliaLib GTest::gtest_main)

# Auto-discover tests
gtest_discover_tests(AureliaTests)
