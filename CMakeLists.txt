cmake_minimum_required(VERSION 2.8)
project(CppServerFramework)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++20 -Wall -Wno-deprecated -Wno-unused-function")

include_directories(.)
# include_directories(/apps/sylar/include)
# link_directories(/apps/sylar/lib)

find_library(YAMLCPP yaml-cpp)
message("***", ${YAMLCPP})

set(LIB_SRC
    src/log.cpp
    src/util.cpp
    src/config.cpp
    )

add_library(sylar SHARED ${LIB_SRC})

add_executable(test tests/test.cc)
add_dependencies(test sylar)
target_link_libraries(test sylar ${YAMLCPP})

add_executable(test_config tests/test_config.cc)
add_dependencies(test_config sylar)
target_link_libraries(test_config sylar ${YAMLCPP})

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)