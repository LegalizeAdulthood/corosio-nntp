add_executable(reader reader.cpp)
target_link_libraries(reader PUBLIC nntp)
target_folder(reader "Tools")
