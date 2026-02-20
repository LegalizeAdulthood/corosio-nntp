include(GoogleTest)

find_package(GTest CONFIG REQUIRED)

add_executable(test-nntp
    nntp_test.cpp
    Article_test.cpp
    ArticleSpec_test.cpp
    MessageId_test.cpp
)
target_link_libraries(test-nntp PUBLIC nntp GTest::gtest_main)
target_folder(test-nntp "Tests")

gtest_discover_tests(test-nntp)
