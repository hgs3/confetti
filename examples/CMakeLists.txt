link_libraries(confetti)
include_directories(..)
add_executable(parse parse.c _readstdin.c)
add_executable(walk walk.c _readstdin.c)
