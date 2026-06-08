include_directories(common/include)

find_package(OpenCL REQUIRED)

include(FetchContent)
FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 3.0.0
)
FetchContent_MakeAvailable(SFML)

add_executable(particle-simulation particle-simulation-main.cpp
        common/src/Console.cpp

        common/src/PhysicsEngine.cpp
        common/src/RenderSystem.cpp
)
target_include_directories(particle-simulation PUBLIC common/include)

target_link_libraries(particle-simulation
        PRIVATE
        SFML::Graphics
        SFML::Window
        SFML::System
        OpenCL::OpenCL
)

if (MSVC)
    target_compile_options(particle-simulation PRIVATE /W4 /wd4819)
else ()
    target_compile_options(particle-simulation PRIVATE -Wall -Wextra)
endif ()