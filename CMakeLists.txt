cmake_minimum_required(VERSION 3.17)

project(Lab2 
    VERSION 1.0
    DESCRIPTION "Second lab for systems class"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 11)

set(RUNTIME_DIR ${CMAKE_SOURCE_DIR}/bin/)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${RUNTIME_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${RUNTIME_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${RUNTIME_DIR})

set(SOURCE_NAMES
		src/main.cpp
		src/types.hpp
		src/logger.cpp
		src/logger.hpp
        src/string_parsing_tools.hpp
		src/string_parsing_tools.cpp
		src/object_code_parser.cpp
		src/instruction_definition_table.hpp
        src/instruction_definition_table.cpp
		src/symbol_table_parser.cpp
)

add_executable(disassem ${SOURCE_NAMES})

# Copies assets over to the build directory.
set(ASSET_NAMES
		${CMAKE_SOURCE_DIR}/assets/out.lst
		${CMAKE_SOURCE_DIR}/assets/test.obj
		${CMAKE_SOURCE_DIR}/assets/test.sym
)

add_custom_target(copy-assets ALL
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ASSET_NAMES} ${RUNTIME_DIR}
)

add_dependencies(disassem copy-assets)
