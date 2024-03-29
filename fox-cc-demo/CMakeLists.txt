cmake_minimum_required(VERSION 3.21)

file(GLOB_RECURSE sources 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.inl"
    )

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})

add_executable(
    fox-cc-demo
    ${sources}
)

if(${IPO_SUPPORTED})
    set_target_properties(fox-cc-demo PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

set_target_properties(fox-cc-demo PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:fox-cc-demo>)

if(MSVC)
    target_compile_options(
        fox-cc-demo
        PRIVATE /WX # all warnings as errors 
        PRIVATE /MP # multi-processor compilation
        # PRIVATE /FR
    )
endif()

target_include_directories(
    fox-cc-demo
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
    fox-cc-demo
    fox-cc
)