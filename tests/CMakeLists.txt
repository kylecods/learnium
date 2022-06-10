add_executable(Test main.c)

target_link_libraries(Test PRIVATE ln_libs)

add_test(NAME tests COMMAND Test)
