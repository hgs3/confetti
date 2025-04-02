link_libraries(confetti)
include_directories(..)
add_executable(parse parse.c readstdin.c)
add_executable(walk walk.c readstdin.c)
