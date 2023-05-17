cmake_minimum_required(VERSION 3.21)

file(GLOB_RECURSE sources 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.inl"
    )

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})

add_library(
    fox-cc
    STATIC
    ${sources}
)

if(${IPO_SUPPORTED})
    set_target_properties(fox-cc PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

set_target_properties(fox-cc PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:fox-cc>)

if(MSVC)
    target_compile_options(
        fox-cc
        PRIVATE /WX # all warnings as errors 
        PRIVATE /MP # multi-processor compilation
        # PRIVATE /FR
    )
endif()

target_include_directories(
    fox-cc
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
