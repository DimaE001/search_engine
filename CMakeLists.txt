cmake_minimum_required(VERSION 3.14)
project(search_engine VERSION 1.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

include(FetchContent)

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(googletest)
enable_testing()


set(PROJECT_SOURCES
    src/SearchServer.cpp
    src/InvertedIndex.cpp
    src/ConverterJSON.cpp
    src/utilsSearchServer.cpp
)

add_executable(search_engine
    src/main.cpp
    ${PROJECT_SOURCES}
)

target_include_directories(search_engine PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(search_engine_tests
    tests/test_inverted_index.cpp
    tests/test_search_server.cpp
    tests/test_converter_json.cpp
    tests/test_utils_search_server.cpp
    ${PROJECT_SOURCES}
)


target_link_libraries(search_engine PRIVATE
    nlohmann_json::nlohmann_json
)

configure_file(${CMAKE_SOURCE_DIR}/config.json   ${CMAKE_BINARY_DIR}/config.json   COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/requests.json ${CMAKE_BINARY_DIR}/requests.json COPYONLY)
file(COPY ${CMAKE_SOURCE_DIR}/resources DESTINATION ${CMAKE_BINARY_DIR})


target_include_directories(search_engine_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(search_engine_tests PRIVATE
    gtest_main
    nlohmann_json::nlohmann_json
)

include(GoogleTest)
gtest_discover_tests(search_engine_tests)

set(JSON_BuildTests OFF CACHE INTERNAL "")