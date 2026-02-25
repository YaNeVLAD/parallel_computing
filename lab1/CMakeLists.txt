set(CMAKE_CXX_STANDARD 26)

include_directories(include)

add_executable(make-archive make-archive-main.cpp
        include/Process.hpp
        src/Process.cpp
)
target_include_directories(make-archive PUBLIC include)

add_executable(extract-files extract-files-main.cpp
        include/Process.hpp
        src/Process.cpp
)
target_include_directories(extract-files PUBLIC include)
