cmake_minimum_required(VERSION 3.14)
project(search_engine VERSION 1.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(JSON_BuildTests OFF CACHE INTERNAL "")

include(FetchContent)
include(CTest)

find_package(nlohmann_json 3.11.3 QUIET)
if(NOT nlohmann_json_FOUND)
    FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
    )
    FetchContent_MakeAvailable(json)
endif()

if(BUILD_TESTING)
    find_package(GTest QUIET)
    if(NOT GTest_FOUND)
        FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        FetchContent_MakeAvailable(googletest)
    endif()
endif()


set(PROJECT_SOURCES
    src/SearchServer.cpp
    src/InvertedIndex.cpp
    src/ConverterJSON.cpp
    src/utilsSearchServer.cpp
)

set(NEWS_DEDUPE_SOURCES
    src/news_dedupe/NewsDedupeEngine.cpp
    src/news_dedupe/TextNormalizer.cpp
)

add_library(news_dedupe STATIC
    ${NEWS_DEDUPE_SOURCES}
)

target_include_directories(news_dedupe PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(search_engine
    src/main.cpp
    ${PROJECT_SOURCES}
)

target_include_directories(search_engine PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(news_dedupe_cli
    app/news_dedupe_cli.cpp
)

target_link_libraries(news_dedupe_cli PRIVATE
    news_dedupe
    nlohmann_json::nlohmann_json
)

target_link_libraries(search_engine PRIVATE
    nlohmann_json::nlohmann_json
)

configure_file(${CMAKE_SOURCE_DIR}/config.json   ${CMAKE_BINARY_DIR}/config.json   COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/requests.json ${CMAKE_BINARY_DIR}/requests.json COPYONLY)
file(COPY ${CMAKE_SOURCE_DIR}/resources DESTINATION ${CMAKE_BINARY_DIR})


if(BUILD_TESTING)
    add_executable(search_engine_tests
        tests/test_inverted_index.cpp
        tests/test_search_server.cpp
        tests/test_converter_json.cpp
        tests/test_utils_search_server.cpp
        tests/test_news_dedupe.cpp
        ${PROJECT_SOURCES}
    )

    target_include_directories(search_engine_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_link_libraries(search_engine_tests PRIVATE
        GTest::gtest_main
        news_dedupe
        nlohmann_json::nlohmann_json
    )

    include(GoogleTest)
    gtest_discover_tests(search_engine_tests)
endif()
