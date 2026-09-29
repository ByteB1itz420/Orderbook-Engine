cmake_minimum_required(VERSION 3.22)
project(orderbook_engine VERSION 0.1.0 LANGUAGES CXX)

# ---- Standard ---------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

# ---- Warnings ---------------------------------------------------------------
add_compile_options(-Wall -Wextra -Werror)

# ---- Sanitizers (option: -DLOB_SANITIZERS=ON) -------------------------------
option(LOB_SANITIZERS "Build with ASan + UBSan" OFF)
if(LOB_SANITIZERS)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

# ---- Core library -----------------------------------------------------------
add_library(lob_core
    src/book/order_book.cpp
    src/book/compact_book.cpp
    src/feed/lobster_parser.cpp
    src/replay/replay_engine.cpp
)
target_include_directories(lob_core PUBLIC src)

# ---- Replay CLI (stub for milestone 1) --------------------------------------
add_executable(replay src/replay/main.cpp)
target_link_libraries(replay PRIVATE lob_core)

# ---- Tests ------------------------------------------------------------------
include(CTest)
if(BUILD_TESTING)
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.4.0
    )
    FetchContent_MakeAvailable(Catch2)
    add_subdirectory(tests)
endif()

add_executable(lob_bench src/bench/bench.cpp)
target_link_libraries(lob_bench PRIVATE lob_core)
