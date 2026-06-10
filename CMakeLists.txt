cmake_minimum_required(VERSION 3.20)

project(snl_compiler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(snlc
    src/ast.cpp
    src/diagnostic.cpp
    src/lexer.cpp
    src/main.cpp
    src/parser.cpp
    src/token.cpp
)

target_include_directories(snlc PRIVATE src)
