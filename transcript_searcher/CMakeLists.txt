cmake_minimum_required(VERSION 3.27.1)
project(TranscriptSearcher)
set(CMAKE_CXX_STANDARD 20)

# If compiling on windows, linking against the dl library needs some fidgeting
if (WIN32)
    # I'm currently using MSYS2 / mingw_w64 for GCC
    find_library(DL_LIBRARY dl HINTS C:/msys64/mingw64/lib)
# Otherwise leave the library to link as normal
ELSE()
    set(DL_LIBRARY dl)
ENDIF()
    
set(SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
set(SOURCES ${SOURCE_DIR}/main.cpp ${SOURCE_DIR}/transcript_searcher.cpp ${SOURCE_DIR}/tf_idf_transcript_search.cpp)
set(CLIENT_SOURCES ${SOURCE_DIR}/transcript_searcher_socketio_client.cpp ${SOURCE_DIR}/transcript_searcher.cpp ${SOURCE_DIR}/tf_idf_transcript_search.cpp)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/SQLiteCpp)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/socket.io-client-cpp)

set(EXECUTABLE main)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(${EXECUTABLE} ${SOURCES})

set(SOCKET_CLIENT transcript_searcher_socketio_client)
add_executable(${SOCKET_CLIENT} ${CLIENT_SOURCES})

target_include_directories(${EXECUTABLE} PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(${EXECUTABLE} SQLiteCpp sqlite3 pthread ${DL_LIBRARY})
target_compile_definitions(${EXECUTABLE} PRIVATE PROJECT_BASE_DIR="${PROJECT_SOURCE_DIR}/../")
target_compile_options(${EXECUTABLE} PRIVATE -Wall)

target_include_directories(${SOCKET_CLIENT} PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(${SOCKET_CLIENT} sioclient ws2_32 SQLiteCpp sqlite3 pthread ${DL_LIBRARY})
target_compile_definitions(${SOCKET_CLIENT} PRIVATE PROJECT_BASE_DIR="${PROJECT_SOURCE_DIR}/../")
target_compile_options(${SOCKET_CLIENT} PRIVATE -Wall)
