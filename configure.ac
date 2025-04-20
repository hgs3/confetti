# Specify the project and version + the minimum automake version required.
AC_PREREQ(2.66)
AC_INIT([confetti], [1.0.0-beta.1])

# Sanity check to verify this is indeed the source directory.
AC_CONFIG_SRCDIR([confetti_unidata.c])

# Initialize automake and use "foreign" so it doesn't attmpet to look for or
# create a GNU project structure: COPYING, NEWS, etc...
AM_INIT_AUTOMAKE([foreign])

# Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
LT_INIT([win32-dll])

# Run compilation tests against a C compiler.
AC_LANG([C])

# Check for C11.
AC_MSG_CHECKING(for _Noreturn)
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM(
    [ #include <stdlib.h>
      _Noreturn void foo(void) { exit(0); } ],
    [ foo(); ]
  )],
  [AC_MSG_RESULT([yes])],
  [AC_MSG_ERROR([no])])

# Generate output files with macros expanded.
AC_CONFIG_FILES([
  Makefile
  man/Makefile
  examples/Makefile
  confetti.pc
])
AC_OUTPUT
