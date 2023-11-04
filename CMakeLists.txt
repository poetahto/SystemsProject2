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
        src/global.hpp
        src/instruction_info.hpp
		src/logger.cpp
		src/logger.hpp
)

add_executable(disassem ${SOURCE_NAMES})

# Copies assets over to the build directory.
set(ASSET_NAMES
		${CMAKE_SOURCE_DIR}/assets/out.lst
		${CMAKE_SOURCE_DIR}/assets/test.obj
		${CMAKE_SOURCE_DIR}/assets/test.sym
		${CMAKE_SOURCE_DIR}/assets/opcode_table.csv
)

add_custom_target(copy-assets ALL
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ASSET_NAMES} ${RUNTIME_DIR}
)

add_dependencies(disassem copy-assets)
