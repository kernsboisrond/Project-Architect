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

set(WASMTIME_URL "https://github.com/bytecodealliance/wasmtime/releases/download/v20.0.2/wasmtime-v20.0.2-x86_64-windows-c-api.zip")
FetchContent_Declare(
    wasmtime_c_api
    URL ${WASMTIME_URL}
)
FetchContent_MakeAvailable(wasmtime_c_api)

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
    src/seraph/ExecutionDiagnostics.cpp
    src/seraph/WasmExecutor.cpp
    src/seraph/ModuleRegistry.cpp
)

target_include_directories(architect_seraph
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    PRIVATE
        ${wasmtime_c_api_SOURCE_DIR}/include
)

target_link_libraries(architect_seraph
    PUBLIC
        nlohmann_json::nlohmann_json
        ${wasmtime_c_api_SOURCE_DIR}/lib/wasmtime.dll.lib
)

function(copy_wasmtime_dll target)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${wasmtime_c_api_SOURCE_DIR}/lib/wasmtime.dll"
        $<TARGET_FILE_DIR:${target}>
    )
endfunction()

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

copy_wasmtime_dll(architect)

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

    add_executable(test_frame_metadata
        tests/test_frame_metadata.cpp
    )
    target_link_libraries(test_frame_metadata
        PRIVATE
            architect_warden
    )
    add_test(NAME test_frame_metadata COMMAND test_frame_metadata)

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

    add_executable(test_seraph_contracts
        tests/test_seraph_contracts.cpp
    )
    target_link_libraries(test_seraph_contracts
        PRIVATE
            architect_seraph
    )
    add_test(NAME test_seraph_contracts COMMAND test_seraph_contracts)

    add_executable(test_wasm_executor
        tests/test_wasm_executor.cpp
    )
    target_link_libraries(test_wasm_executor
        PRIVATE
            architect_seraph
    )
    add_test(NAME test_wasm_executor COMMAND test_wasm_executor WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

    add_executable(test_wasm_executor_failures
        tests/test_wasm_executor_failures.cpp
    )
    target_link_libraries(test_wasm_executor_failures
        PRIVATE
            architect_seraph
    )
    add_test(NAME test_wasm_executor_failures COMMAND test_wasm_executor_failures WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

    add_executable(test_module_registry
        tests/test_module_registry.cpp
    )
    target_link_libraries(test_module_registry
        PRIVATE
            architect_seraph
    )
    add_test(NAME test_module_registry COMMAND test_module_registry WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

    copy_wasmtime_dll(test_executor)
    copy_wasmtime_dll(test_seraph_contracts)
    copy_wasmtime_dll(test_wasm_executor)
    copy_wasmtime_dll(test_wasm_executor_failures)
    copy_wasmtime_dll(test_module_registry)

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

        add_executable(test_real_backend_intents
            tests/test_real_backend_intents.cpp
        )
        target_link_libraries(test_real_backend_intents
            PRIVATE
                architect_warden
                architect_core
        )
        add_test(NAME test_real_backend_intents COMMAND test_real_backend_intents)
        set_tests_properties(test_real_backend_intents PROPERTIES TIMEOUT 120)
        copy_wasmtime_dll(test_real_backend_intents)
    endif()

endif()
