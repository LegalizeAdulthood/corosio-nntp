add_library(nntp
    include/nntp/client.h
    client.cpp
    include/nntp/file_stream.h
    include/nntp/detail/file_ops.h
    include/nntp/detail/win_file_impl_internal.h
    include/nntp/detail/win_file_impl.h
    include/nntp/detail/win_file_service.h
    include/nntp/test/mock_file_stream.h
    file_stream.cpp
    file_ops.cpp
    win_file_impl.cpp
    win_file_service.cpp
    test/mock_file_stream.cpp
)
target_include_directories(nntp PUBLIC include)
target_include_directories(nntp PRIVATE ../corosio/src/corosio)
target_link_libraries(nntp PUBLIC Boost::corosio)
target_folder(nntp "Libraries")
