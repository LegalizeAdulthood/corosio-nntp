add_library(nntp
    include/nntp/client.h
    client.cpp
)

target_include_directories(nntp PUBLIC include)
target_link_libraries(nntp PUBLIC Boost::capy)
target_folder(nntp "Libraries")
