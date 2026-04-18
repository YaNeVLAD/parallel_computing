set(THREADS_PREFER_PTHREAD_FLAG ON)

include_directories(include)

add_executable(streaming-tv streaming-tv-main.cpp
        include/Common.hpp
        include/VideoCapturer.hpp
        include/AudioCapturer.hpp
        include/Station.hpp
        include/Receiver.hpp

)
target_include_directories(streaming-tv PUBLIC include)

target_link_libraries(streaming-tv
        Boost::headers
        ${OpenCV_LIBS}
        Threads::Threads
        RtAudio::rtaudio
)

if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_compile_options(streaming-tv PRIVATE /W4 /wd4819)
    target_compile_definitions(streaming-tv PRIVATE WIN32_LEAN_AND_MEAN)
else ()
    target_compile_options(streaming-tv PRIVATE -Wall -Wextra)
endif ()

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
if (WIN32)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif ()
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(streaming-tv-tests StreamingTVTests.cpp
        include/Common.hpp

)

target_link_libraries(streaming-tv-tests
        PRIVATE
        GTest::gtest_main
)
include(GoogleTest)

gtest_discover_tests(streaming-tv-tests)