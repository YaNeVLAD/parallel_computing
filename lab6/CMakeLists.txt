include_directories(include)

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native")

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark testing" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "Disable benchmark install" FORCE)

include(FetchContent)
FetchContent_Declare(
        benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG        v1.8.3
)
FetchContent_MakeAvailable(benchmark)

add_executable(histogram-bench histogram-bench-main.cpp)
target_include_directories(histogram-bench PUBLIC common/include)

find_package(Threads REQUIRED)
target_link_libraries(histogram-bench PRIVATE benchmark::benchmark Threads::Threads)

if (MSVC)
    target_compile_options(histogram-bench PRIVATE /W4 /wd4819)
else ()
    target_compile_options(histogram-bench PRIVATE -Wall -Wextra)
endif ()