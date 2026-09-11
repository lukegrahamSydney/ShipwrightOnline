cmake_minimum_required(VERSION 3.16)

project(OOTServer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

file(GLOB_RECURSE OOTSERVER_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

if(NOT OOTSERVER_SOURCES)
    message(FATAL_ERROR "No sources found under src/")
endif()

add_executable(OOTServer ${OOTSERVER_SOURCES})

target_include_directories(OOTServer PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

if(MSVC)
    target_compile_options(OOTServer PRIVATE /W4 /permissive- /utf-8 /Zc:preprocessor)
    target_compile_definitions(OOTServer PRIVATE
        _WINSOCKAPI_
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
    target_link_libraries(OOTServer PRIVATE ws2_32)
else()
    target_compile_options(OOTServer PRIVATE -Wall -Wextra)
endif()

if(WIN32 AND NOT MSVC)
    target_link_libraries(OOTServer PRIVATE ws2_32)
endif()

find_package(Threads REQUIRED)
target_link_libraries(OOTServer PRIVATE Threads::Threads)

set_target_properties(OOTServer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}"
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config.json")
    add_custom_command(TARGET OOTServer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/config.json"
            "$<TARGET_FILE_DIR:OOTServer>/config.json"
    )
endif()