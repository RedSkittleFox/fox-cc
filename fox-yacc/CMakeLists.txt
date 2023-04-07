cmake_minimum_required(VERSION 3.21)

file(GLOB_RECURSE sources 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    )

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})

add_executable(
    fox-yacc
    ${sources}
)

if(${IPO_SUPPORTED})
    set_target_properties(fox-yacc PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

set_target_properties(fox-yacc PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:fox-yacc>)

if(MSVC)
    target_compile_options(
        fox-yacc
        PRIVATE /WX # all warnings as errors 
        PRIVATE /MP # multi-processor compilation
        # PRIVATE /FR
    )
endif()
