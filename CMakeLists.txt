cmake_minimum_required(VERSION 3.14)
project(backpack-cpp-sdk VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Dependencies
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    include(FetchContent)
    FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz)
    FetchContent_MakeAvailable(json)
endif()

# WebSocket++ - just download and include the headers
include(FetchContent)
FetchContent_Declare(
    websocketpp
    GIT_REPOSITORY https://github.com/zaphoyd/websocketpp.git
    GIT_TAG 0.8.2
)
FetchContent_GetProperties(websocketpp)
if(NOT websocketpp_POPULATED)
    FetchContent_Populate(websocketpp)
endif()

# Define the USE_BOOST_ASIO flag for WebSocketPP
add_compile_definitions(ASIO_STANDALONE=0)

# Library sources
set(BACKPACK_SOURCES
    src/websocket_client.cpp
    src/backpack_client.cpp
    src/rest_client.cpp
    src/utils.cpp
)

# Library headers
set(BACKPACK_HEADERS
    include/backpack/websocket_client.hpp
    include/backpack/backpack_client.hpp
    include/backpack/rest_client.hpp
    include/backpack/types.hpp
    include/backpack/utils.hpp
)

# Create the library
add_library(backpack_cpp_sdk STATIC ${BACKPACK_SOURCES} ${BACKPACK_HEADERS})
target_include_directories(backpack_cpp_sdk 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${websocketpp_SOURCE_DIR}
        ${Boost_INCLUDE_DIRS}
)
target_link_libraries(backpack_cpp_sdk
    PUBLIC
        OpenSSL::SSL
        OpenSSL::Crypto
        CURL::libcurl
        nlohmann_json::nlohmann_json
        ${Boost_LIBRARIES}
)

# Examples
add_executable(websocket_example examples/websocket_example.cpp)
target_link_libraries(websocket_example PRIVATE backpack_cpp_sdk)

# Installation
include(GNUInstallDirs)
install(TARGETS backpack_cpp_sdk
    EXPORT backpack_cpp_sdk-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT backpack_cpp_sdk-targets
    FILE backpack_cpp_sdk-targets.cmake
    NAMESPACE backpack::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/backpack_cpp_sdk
)
