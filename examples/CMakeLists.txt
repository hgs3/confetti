add_executable(echo echo.c readstdin.c)
target_link_libraries(echo PRIVATE confetti)
target_include_directories(echo PRIVATE ..)
