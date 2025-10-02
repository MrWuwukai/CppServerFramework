cmake_minimum_required(VERSION 3.15)
project(CppServer)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++20 -Wall -Wno-deprecated -Wno-unused-function")

# include_directories(.)

find_library(YAMLCPP yaml-cpp)
message("***" ${YAMLCPP})

set(LIB_SRC
    src/async/fdmanager.cpp
    src/async/fiber.cpp
    src/async/hook.cpp
    src/async/iomanager.cpp
    src/async/multithread.cpp
    src/async/scheduler.cpp
    src/async/timer.cpp
    src/config.cpp
    src/log.cpp
    src/utils.cpp   
    )

add_library(CFramework SHARED ${LIB_SRC})

target_include_directories(CFramework
    PUBLIC
        # .           # 项目根目录
        src         # src 目录，可访问 src/log.h
        src/async
        src/utils
        # 如果还有别的目录，比如 include/，也可以加上
        # include
)

set(LIBS
    dl
    pthread
    ${YAMLCPP}
    )

add_executable(test tests/test_iomanager.cpp)
add_dependencies(test CFramework)
target_link_libraries(test CFramework ${LIBS})

# add_executable(test_config tests/test_config.cpp)
# add_dependencies(test_config CFramework)
# target_link_libraries(test_config CFramework ${YAMLCPP})

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)