cmake_minimum_required(VERSION 3.20)
project(ProjectArchitect VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ARCHITECT_ENABLE_LLAMA "Enable llama.cpp / Gemma backend" OFF)
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
    src/warden/LlamaCppBackend.cpp
)

target_include_directories(architect_warden
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(architect_warden
    PUBLIC
        nlohmann_json::nlohmann_json
)

if(ARCHITECT_ENABLE_LLAMA)
    # Disable unnecessary llama.cpp targets to speed up build
    set(LLAMA_BUILD_TESTS OFF CACHE BOOL "Disable tests" FORCE)
    set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "Disable examples" FORCE)
    set(LLAMA_BUILD_SERVER OFF CACHE BOOL "Disable server" FORCE)
    
    FetchContent_Declare(
        llama_cpp
        GIT_REPOSITORY https://github.com/ggml-org/llama.cpp.git
        GIT_TAG        b6498 # using a pinned stable release for predictability
    )
    FetchContent_MakeAvailable(llama_cpp)

    # Work around current MSVC + C++23 build issues in llama.cpp
    if(MSVC)
        if(TARGET llama)
            set_property(TARGET llama PROPERTY CXX_STANDARD 17)
            set_property(TARGET llama PROPERTY CXX_STANDARD_REQUIRED ON)
            set_property(TARGET llama PROPERTY CXX_EXTENSIONS OFF)
        endif()

        # Some versions split pieces across ggml targets; guard if present.
        foreach(tgt ggml ggml-base ggml-cpu ggml-metal ggml-blas)
            if(TARGET ${tgt})
                set_property(TARGET ${tgt} PROPERTY CXX_STANDARD 17)
                set_property(TARGET ${tgt} PROPERTY CXX_STANDARD_REQUIRED ON)
                set_property(TARGET ${tgt} PROPERTY CXX_EXTENSIONS OFF)
            endif()
        endforeach()
    endif()
endif()

if(ARCHITECT_ENABLE_LLAMA)
    target_compile_definitions(architect_warden PUBLIC ARCHITECT_ENABLE_LLAMA=1)
    target_link_libraries(architect_warden PUBLIC llama)
else()
    target_compile_definitions(architect_warden PUBLIC ARCHITECT_ENABLE_LLAMA=0)
endif()

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

    add_executable(test_prompt_assembler
        tests/test_prompt_assembler.cpp
    )
    target_link_libraries(test_prompt_assembler
        PRIVATE
            architect_warden
    )
    add_test(NAME test_prompt_assembler COMMAND test_prompt_assembler)

    add_executable(test_executor
        tests/test_executor.cpp
    )
    target_link_libraries(test_executor
        PRIVATE
            architect_seraph
    )
    add_test(NAME test_executor COMMAND test_executor)

    if(ARCHITECT_ENABLE_LLAMA)
        add_executable(test_llama_backend_smoke
            tests/test_llama_backend_smoke.cpp
        )
        target_link_libraries(test_llama_backend_smoke
            PRIVATE
                architect_warden
        )
        add_test(NAME test_llama_backend_smoke COMMAND test_llama_backend_smoke)
        set_tests_properties(test_llama_backend_smoke PROPERTIES TIMEOUT 120)
    endif()

endif()
