cmake_minimum_required(VERSION 3.20)
project(ProjectArchitect VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
include(CTest)

FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

# Warden
add_library(architect_warden STATIC
    src/warden/WardenEngine.cpp
    src/warden/MockBrainBackend.cpp
    src/warden/PromptAssembler.cpp
    src/warden/GrammarConstraints.cpp
)

target_include_directories(architect_warden
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(architect_warden
    PUBLIC
        nlohmann_json::nlohmann_json
)

# Seraph
add_library(architect_seraph STATIC
    src/seraph/ExecutorStub.cpp
)

target_include_directories(architect_seraph
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Core
add_library(architect_core STATIC
    src/core/CognitiveLoop.cpp
)

target_include_directories(architect_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(architect_core
    PUBLIC
        architect_warden
        architect_seraph
)

# Main executable
add_executable(architect
    src/main.cpp
)

target_link_libraries(architect
    PRIVATE
        architect_core
        architect_warden
        architect_seraph
)

if(BUILD_TESTING)

    add_executable(test_cognitive_frame
        tests/test_cognitive_frame.cpp
    )
    target_include_directories(test_cognitive_frame PRIVATE src)
    add_test(NAME test_cognitive_frame COMMAND test_cognitive_frame)

    add_executable(test_warden
        tests/test_warden.cpp
    )
    target_link_libraries(test_warden
        PRIVATE
            architect_warden
    )
    add_test(NAME test_warden COMMAND test_warden)

    add_executable(test_executor
        tests/test_executor.cpp
    )
    target_link_libraries(test_executor
        PRIVATE
            architect_seraph
    )
    add_test(NAME test_executor COMMAND test_executor)

endif()
