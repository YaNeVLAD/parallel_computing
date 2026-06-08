include_directories(common/include)

find_package(OpenCL REQUIRED)

add_executable(particle-simulation particle-simulation-main.cpp
        common/src/Console.cpp
)
target_include_directories(particle-simulation PUBLIC common/include)

target_link_libraries(particle-simulation PRIVATE OpenCL::OpenCL)

if (MSVC)
    target_compile_options(particle-simulation PRIVATE /W4 /wd4819)
else ()
    target_compile_options(particle-simulation PRIVATE -Wall -Wextra)
endif ()