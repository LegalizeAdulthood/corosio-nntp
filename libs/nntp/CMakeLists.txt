add_library(nntp
    include/nntp/client.h
    client.cpp
)
target_include_directories(nntp PUBLIC include)
target_folder(nntp "Libraries")
