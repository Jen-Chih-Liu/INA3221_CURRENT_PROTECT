cmake_minimum_required(VERSION 3.10)

project(FILESUM C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_executable(FILESUM FILESUM.c)

# 指定標頭檔搜尋路徑為上一層目錄
target_include_directories(FILESUM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
