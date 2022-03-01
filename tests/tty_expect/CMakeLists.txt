project(tty_excpect)

# Configure lib
add_executable(tty_excpect main.cpp)
target_include_directories(tty_excpect PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(tty_excpect PRIVATE cppexpect)
add_test(NAME tty_excpect COMMAND tty_excpect)
