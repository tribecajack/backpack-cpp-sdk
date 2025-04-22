cmake_minimum_required(VERSION 3.10)
project(backpack_sdk)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set policy for Boost
cmake_policy(SET CMP0074 NEW)
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 OLD)
endif()

# Find required packages
set(Boost_USE_STATIC_LIBS ON)
set(OPENSSL_USE_STATIC_LIBS ON)
find_package(Boost 1.88.0 REQUIRED COMPONENTS system thread)
find_package(OpenSSL 3 REQUIRED COMPONENTS Crypto)
find_package(CURL REQUIRED)
find_package(nlohmann_json 3.11.2 REQUIRED)

# Create static library
add_library(${PROJECT_NAME} STATIC
    src/websocket_client.cpp
)

# Link dependencies
target_link_libraries(${PROJECT_NAME}
    PUBLIC
    Boost::system
    Boost::thread
    OpenSSL::SSL
    OpenSSL::Crypto
    CURL::libcurl
    nlohmann_json::nlohmann_json
)

# Set include directories
target_include_directories(${PROJECT_NAME}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE OPENSSL_API_COMPAT=0x30000000L)

# Examples
add_executable(websocket_example examples/websocket_example.cpp)
target_link_libraries(websocket_example PRIVATE ${PROJECT_NAME})

# Installation
install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp"
)