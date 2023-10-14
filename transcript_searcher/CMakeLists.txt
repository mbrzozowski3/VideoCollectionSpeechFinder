cmake_minimum_required(VERSION 3.27.1)
project(TranscriptSearcher)
set(CMAKE_CXX_STANDARD 20)

set(SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
set(SOURCES ${SOURCE_DIR}/main.cpp ${SOURCE_DIR}/transcript_searcher.cpp ${SOURCE_DIR}/tf_idf_transcript_search.cpp)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/SQLiteCpp)

set(EXECUTABLE main)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(${EXECUTABLE} ${SOURCES})

target_include_directories(${EXECUTABLE} PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(${EXECUTABLE} SQLiteCpp sqlite3 pthread dl)
target_compile_definitions(${EXECUTABLE} PRIVATE PROJECT_BASE_DIR="${PROJECT_SOURCE_DIR}/../")
target_compile_options(${EXECUTABLE} PRIVATE -Wall)
