add_library(fileio
    include/fileio/file_stream.h
    include/fileio/test/mock_file_stream.h
    file_stream.cpp
    test/mock_file_stream.cpp
)

# Platform-specific sources
if(WIN32)
    target_sources(fileio PRIVATE
        include/fileio/detail/file_ops.h
        include/fileio/detail/win_file_impl_internal.h
        include/fileio/detail/win_file_impl.h
        include/fileio/detail/win_file_service.h
        file_ops.cpp
        win_file_impl.cpp
        win_file_service.cpp
    )
elseif(UNIX AND NOT APPLE)
    # Linux
    target_sources(fileio PRIVATE
        include/fileio/detail/uring_file_ops.h
        include/fileio/detail/uring_file_impl.h
        include/fileio/detail/uring_file_service.h
        uring_file_ops.cpp
        uring_file_impl.cpp
        uring_file_service.cpp
    )

    # Link liburing for Linux
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(URING REQUIRED liburing>=2.0)
    target_link_libraries(fileio PRIVATE ${URING_LIBRARIES})
    target_include_directories(fileio PRIVATE ${URING_INCLUDE_DIRS})
elseif(APPLE)
    # macOS
    target_sources(fileio PRIVATE
        include/fileio/detail/gcd_file_ops.h
        include/fileio/detail/gcd_file_impl.h
        include/fileio/detail/gcd_file_service.h
        gcd_file_ops.cpp
        gcd_file_impl.cpp
        gcd_file_service.cpp
    )
    # No external dependencies - GCD is built-in
endif()

target_include_directories(fileio PUBLIC include)
target_include_directories(fileio PRIVATE ../corosio/src/corosio)
target_link_libraries(fileio PUBLIC Boost::corosio)
target_folder(fileio "Libraries")
