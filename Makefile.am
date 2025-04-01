SUBDIRS = man examples

EXTRA_DIST = autogen.sh LICENSE CMakeLists.txt ConfettiConfig.cmake.in README.md

AUTOMAKE_OPTIONS = subdir-objects

lib_LTLIBRARIES = libconfetti.la
libconfetti_la_SOURCES = confetti.c confetti_unidata.c confetti.h
libconfetti_la_LDFLAGS = -no-undefined -version-info 0:0:0
include_HEADERS = confetti.h

noinst_PROGRAMS = echo
echo_SOURCES = examples/echo.c examples/readstdin.c
echo_CFLAGS = -I$(top_srcdir)
echo_LDADD = libconfetti.la
echo_DEPENDENCIES = libconfetti.la
