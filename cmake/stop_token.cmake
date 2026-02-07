# Check if -fexperimental-library flag is needed for C++20 stop_token support
# and apply it to the specified target if required (macOS only)

function(stop_token target_name)
    if(NOT APPLE)
        return()
    endif()
    
    include(CheckCXXSourceCompiles)
    
    set(TEST_SOURCE "
        #include <stop_token>
        int main() {
            std::stop_source source;
            std::stop_token token = source.get_token();
            return 0;
        }
    ")
    
    # Test compilation without the flag
    set(CMAKE_REQUIRED_FLAGS "-std=c++20")
    check_cxx_source_compiles("${TEST_SOURCE}" CXX20_COMPILES_WITHOUT_FLAG)
    
    # Test compilation with the flag
    set(CMAKE_REQUIRED_FLAGS "-std=c++20 -fexperimental-library")
    check_cxx_source_compiles("${TEST_SOURCE}" CXX20_COMPILES_WITH_FLAG)
    
    # Reset CMAKE_REQUIRED_FLAGS
    unset(CMAKE_REQUIRED_FLAGS)
    
    # Add flag only if compilation fails without it but succeeds with it
    if(NOT CXX20_COMPILES_WITHOUT_FLAG AND CXX20_COMPILES_WITH_FLAG)
        target_compile_options(${target_name} PUBLIC -fexperimental-library)
    endif()
endfunction()
