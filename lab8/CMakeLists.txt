include_directories(include)

add_executable(my-task my-task-main.cpp)
target_include_directories(my-task PUBLIC common/include)

add_executable(my-awaiter my-awaiter.cpp)
target_include_directories(my-awaiter PUBLIC common/include)

add_executable(coroutine-generator coro-generator-main.cpp)
target_include_directories(coroutine-generator PUBLIC common/include)

if (MSVC)
    target_compile_options(my-task PRIVATE /W4 /wd4819)
    target_compile_options(my-awaiter PRIVATE /W4 /wd4819)
    target_compile_options(coroutine-generator PRIVATE /W4 /wd4819)
else ()
    target_compile_options(my-task PRIVATE -Wall -Wextra)
    target_compile_options(my-awaiter PRIVATE -Wall -Wextra)
    target_compile_options(coroutine-generator PRIVATE -Wall -Wextra)
endif ()