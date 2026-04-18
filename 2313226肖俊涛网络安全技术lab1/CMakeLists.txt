cmake_minimum_required(VERSION 3.16)
project(DES_chat LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

add_executable(DES_chat
    main.cpp
    src/DES_Operation.cpp
    src/chat.cpp
)

target_include_directories(DES_chat PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
