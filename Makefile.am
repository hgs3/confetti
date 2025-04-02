SUBDIRS = man examples

EXTRA_DIST = autogen.sh LICENSE CMakeLists.txt ConfettiConfig.cmake.in README.md

AUTOMAKE_OPTIONS = subdir-objects

lib_LTLIBRARIES = libconfetti.la
libconfetti_la_SOURCES = confetti.c confetti_unidata.c confetti.h
libconfetti_la_LDFLAGS = -no-undefined -version-info 0:0:0
include_HEADERS = confetti.h

noinst_PROGRAMS = parse
parse_SOURCES = examples/parse.c examples/_readstdin.c
parse_CFLAGS = -I$(top_srcdir)
parse_LDADD = libconfetti.la
parse_DEPENDENCIES = libconfetti.la

noinst_PROGRAMS = walk
walk_SOURCES = examples/walk.c examples/_readstdin.c
walk_CFLAGS = -I$(top_srcdir)
walk_LDADD = libconfetti.la
walk_DEPENDENCIES = libconfetti.la
