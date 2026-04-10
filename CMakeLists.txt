cmake_minimum_required(VERSION 3.20)
project(ProjectArchitect VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Fetch nlohmann/json for mock JSON validation/parsing
include(FetchContent)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)

# Main executable
add_executable(architect 
    src/main.cpp 
    src/warden/WardenEngine.cpp 
    src/core/CognitiveLoop.cpp
    src/seraph/ExecutorStub.cpp
)
target_include_directories(architect PRIVATE src)
target_link_libraries(architect PRIVATE nlohmann_json::nlohmann_json)

# Optional testing
enable_testing()

# Test Cognitive Frame
add_executable(test_cognitive_frame tests/test_cognitive_frame.cpp)
target_include_directories(test_cognitive_frame PRIVATE src)
add_test(NAME test_cognitive_frame COMMAND test_cognitive_frame)

# Test Warden Engine
add_executable(test_warden tests/test_warden.cpp src/warden/WardenEngine.cpp)
target_include_directories(test_warden PRIVATE src)
target_link_libraries(test_warden PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME test_warden COMMAND test_warden)

# Test Seraph Executor
add_executable(test_executor tests/test_executor.cpp src/seraph/ExecutorStub.cpp)
target_include_directories(test_executor PRIVATE src)
add_test(NAME test_executor COMMAND test_executor)
