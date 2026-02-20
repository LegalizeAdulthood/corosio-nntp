include(GoogleTest)

find_package(GTest CONFIG REQUIRED)

add_executable(test-fileio
    mock_file_stream_test.cpp
    file_stream_test.cpp
)
target_link_libraries(test-fileio PUBLIC fileio GTest::gtest_main)
target_folder(test-fileio "Tests")

gtest_discover_tests(test-fileio)
