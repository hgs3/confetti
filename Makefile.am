EXTRA_DIST = autogen.sh LICENSE CMakeLists.txt ConfettiConfig.cmake.in README.md confetti.3 conf_parse.3

AUTOMAKE_OPTIONS = subdir-objects

lib_LTLIBRARIES = libconfetti.la
libconfetti_la_SOURCES = confetti.c confetti_unidata.c confetti.h
libconfetti_la_LDFLAGS = -no-undefined -version-info 0:0:0

include_HEADERS = confetti.h

man3_MANS = confetti.3 conf_parse.3
