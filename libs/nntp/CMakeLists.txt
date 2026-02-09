add_library(nntp
    include/nntp/client.h
    client.cpp
    include/nntp/file_stream.h
    include/nntp/test/mock_file_stream.h
    file_stream.cpp
    test/mock_file_stream.cpp
)

# Platform-specific sources
if(WIN32)
    target_sources(nntp PRIVATE
        include/nntp/detail/file_ops.h
        include/nntp/detail/win_file_impl_internal.h
        include/nntp/detail/win_file_impl.h
        include/nntp/detail/win_file_service.h
        file_ops.cpp
        win_file_impl.cpp
        win_file_service.cpp
    )
elseif(UNIX AND NOT APPLE)
    # Linux
    target_sources(nntp PRIVATE
        include/nntp/detail/uring_file_ops.h
        include/nntp/detail/uring_file_impl.h
        include/nntp/detail/uring_file_service.h
        uring_file_ops.cpp
        uring_file_impl.cpp
        uring_file_service.cpp
    )

    # Link liburing for Linux
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(URING REQUIRED liburing>=2.0)
    target_link_libraries(nntp PRIVATE ${URING_LIBRARIES})
    target_include_directories(nntp PRIVATE ${URING_INCLUDE_DIRS})
elseif(APPLE)
    # macOS
    target_sources(nntp PRIVATE
        include/nntp/detail/gcd_file_ops.h
        include/nntp/detail/gcd_file_impl.h
        include/nntp/detail/gcd_file_service.h
        gcd_file_ops.cpp
        gcd_file_impl.cpp
        gcd_file_service.cpp
    )
    # No external dependencies - GCD is built-in
endif()

target_include_directories(nntp PUBLIC include)
target_include_directories(nntp PRIVATE ../corosio/src/corosio)
target_link_libraries(nntp PUBLIC Boost::corosio)
target_folder(nntp "Libraries")
