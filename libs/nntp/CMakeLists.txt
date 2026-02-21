add_library(nntp
    # Vocabulary value types
    include/nntp/Article.h
    include/nntp/ArticleRange.h
    ArticleRange.cpp
    include/nntp/ArticleSpec.h
    include/nntp/MessageId.h
    MessageId.cpp
    include/nntp/Newsgroup.h
    Newsgroup.cpp

    # Conversational client API
    include/nntp/client.h
    client.cpp
)

target_include_directories(nntp PUBLIC include)
target_link_libraries(nntp PUBLIC Boost::capy)
target_folder(nntp "Libraries")
