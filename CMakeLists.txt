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
    src/config/config.cpp
    src/HTTP/http_connection.cpp
    src/HTTP/http_parser.cpp
    src/HTTP/http_server.cpp
    src/HTTP/http_session.cpp
    src/HTTP/http.cpp
    src/HTTP/httprequest_parser.cpp
    src/HTTP/httprespond_parser.cpp
    src/HTTP/uri.cpp
    src/log/log.cpp
    src/servlet/servlet.cpp
    src/socket/async_socket.cpp
    src/socket/bytearray.cpp
    src/socket/ipaddress.cpp
    src/stream/stream.cpp
    src/stream/tcpserver.cpp
    src/utils/utils.cpp   
    )

add_library(CFramework SHARED ${LIB_SRC})

target_include_directories(CFramework
    PUBLIC
        # .           # 项目根目录
        src         # src 目录，可访问 src/log.h
        src/async
        src/config
        src/HTTP
        src/log
        src/servlet
        src/socket
        src/stream
        src/utils
)

set(LIBS
    dl
    pthread
    ${YAMLCPP}
    )

# add_executable(test tests/test_tcpserver.cpp)
# add_dependencies(test CFramework)
# target_link_libraries(test CFramework ${LIBS})

add_executable(test tests/test_httpconnection.cpp)
add_dependencies(test CFramework)
target_link_libraries(test CFramework ${LIBS})

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)