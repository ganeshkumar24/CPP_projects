cmake_minimum_required(VERSION 3.10)
project(verilog_parser VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add executable
add_executable(verilog_parser
    main.cpp
    verilog_parser.cpp
)

# Set compiler warnings
if(MSVC)
    target_compile_options(verilog_parser PRIVATE /W4)
else()
    target_compile_options(verilog_parser PRIVATE -Wall -Wextra -pedantic)
endif()

# Installation (optional)
install(TARGETS verilog_parser DESTINATION bin)