cmake_minimum_required(VERSION 3.24)
project(IOCPServer C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 1. 컴파일러 기본 옵션 및 ASan 설정 ---
if(MSVC)
    add_compile_options(/W4 /INCREMENTAL:NO)

    # Debug 빌드에서만 ASan 활성화
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(/fsanitize=address)
        add_link_options(/fsanitize=address)
    endif()
else()
    add_compile_options(-Wall -Wextra)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-fsanitize=address)
        add_link_options(-fsanitize=address)
    endif()
endif()

# --- 2. Google Test 설정 ---
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# --- 3. 프로젝트 파일 설정 ---
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

file(GLOB_RECURSE COMMON_SRC "Common/*")
file(GLOB_RECURSE SERVER_SRC "Server/*.cpp")
file(GLOB_RECURSE TEST_SRC "Tests/*.cpp")

add_executable(server ${SERVER_SRC} ${COMMON_SRC})
add_executable(tests ${TEST_SRC} ${COMMON_SRC})

# 윈도우 필수 라이브러리 링크
target_link_libraries(server PRIVATE ws2_32)
target_link_libraries(tests PRIVATE ws2_32 GTest::gtest_main)

# --- 4. CTest 활성화 ---
enable_testing()
add_test(NAME AllTests COMMAND tests)

# --- 5. ASan DLL 복사 ---
if(MSVC)
    get_filename_component(COMPILER_DIR ${CMAKE_CXX_COMPILER} DIRECTORY)
    find_file(ASAN_DLL clang_rt.asan_dynamic-x86_64.dll PATHS ${COMPILER_DIR})

    if(ASAN_DLL)
        add_custom_command(TARGET server POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ASAN_DLL} $<TARGET_FILE_DIR:server>)
        add_custom_command(TARGET tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ASAN_DLL} $<TARGET_FILE_DIR:tests>)
    endif()
endif()
