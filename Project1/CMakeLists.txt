cmake_minimum_required(VERSION 3.16)
project(black_scholes_api LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)

# JSON
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(json)

# CPR
FetchContent_Declare(
    cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 1.11.0
)
set(CPR_BUILD_TESTS OFF)
FetchContent_MakeAvailable(cpr)

# CROW (header only)
FetchContent_Declare(
    crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG v1.0+3
)
FetchContent_MakeAvailable(crow)

# Sources
set(SOURCES
    main.cpp
    black_scholes.cpp
    tests_expiry.cpp
)

add_executable(api_cpp ${SOURCES})

# Just include Crow (header-only → pas de target à linker)
target_include_directories(api_cpp PRIVATE
    ${crow_SOURCE_DIR}/include
)

# Link CPR + JSON
target_link_libraries(api_cpp
    PRIVATE
        cpr::cpr
        nlohmann_json::nlohmann_json
)

target_compile_options(api_cpp PRIVATE -O3 -Wall -Wextra)
