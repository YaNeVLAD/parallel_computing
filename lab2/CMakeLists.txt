set(CMAKE_CXX_STANDARD 26)

include(FetchContent)
FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 3.0.2
        GIT_SHALLOW ON
        EXCLUDE_FROM_ALL
        SYSTEM)
FetchContent_MakeAvailable(SFML)

add_executable(life life-main.cpp
        LifeGame.hpp
        LifeIO.hpp
        Window.hpp)
target_link_libraries(life SFML::Graphics)

add_executable(gaussian-blur gaussian-blur-main.cpp)