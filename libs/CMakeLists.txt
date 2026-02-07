find_package(Boost REQUIRED COMPONENTS core)

add_subdirectory(capy)
add_subdirectory(corosio)
add_subdirectory(nntp)

if(APPLE)
    target_compile_options(boost_corosio PRIVATE -fexperimental-library)
endif()

target_folder(boost_capy "Libraries/Boost")
target_folder(boost_corosio "Libraries/Boost")
target_folder(boost_capy_bench "Tools/Boost")
target_folder(boost_capy_test_suite "Tests/Boost")
target_folder(boost_capy_test_suite_main "Tests/Boost")
target_folder(boost_capy_tests "Tests/Boost")
target_folder(boost_corosio_tests "Tests/Boost")

