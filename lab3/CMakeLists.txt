set(CMAKE_CXX_STANDARD 23)

include_directories(include)

add_executable(economy-simulation economy-simulation-main.cpp
        Bank/Bank.hpp
        Bank/Bank.cpp
        Bank/Actors.hpp
)
target_include_directories(economy-simulation PUBLIC include)

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

add_executable(bank-tests BankTests.cpp
        Bank/Bank.hpp
        Bank/Bank.cpp
)

target_link_libraries(bank-tests
        PRIVATE
        GTest::gtest_main
)
include(GoogleTest)

gtest_discover_tests(bank-tests)