cmake_minimum_required(VERSION 3.10)
project(backpack_sdk)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Boost 1.88.0 REQUIRED COMPONENTS system)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
find_package(nlohmann_json 3.11.2 REQUIRED)

# Create static library
add_library(${PROJECT_NAME} STATIC
    src/websocket_client.cpp
    src/rest_client.cpp
    src/utils.cpp
)

# Link dependencies
target_link_libraries(${PROJECT_NAME}
    PUBLIC
    Boost::system
    OpenSSL::SSL
    OpenSSL::Crypto
    CURL::libcurl
    nlohmann_json::nlohmann_json
)

# Set include directories
target_include_directories(${PROJECT_NAME}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Examples
add_executable(websocket_example examples/websocket_example.cpp)
target_link_libraries(websocket_example PRIVATE ${PROJECT_NAME}) main
# Installation

install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp"
)