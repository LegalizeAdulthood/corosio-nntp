include(GoogleTest)

find_package(GTest CONFIG REQUIRED)

add_executable(test-nntp
    # value type tests
    Article_test.cpp
    ArticleRange_test.cpp
    ArticleSpec_test.cpp
    MessageId_test.cpp
    Newsgroup_test.cpp
    WildcardMatch_test.cpp
    WildcardMatchPattern_test.cpp

    # client API tests
    Client_test.cpp
)
target_link_libraries(test-nntp PUBLIC nntp GTest::gtest_main)
target_folder(test-nntp "Tests")

gtest_discover_tests(test-nntp)
