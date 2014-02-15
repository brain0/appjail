#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([appjail], [1], [])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB(cap,cap_set_proc,,AC_MSG_ERROR(Cannot find required library -lcap.))

# Check for headers
AC_CHECK_HEADERS([errno.h fcntl.h limits.h pwd.h sched.h stdio.h stdlib.h string.h sys/capability.h sys/mount.h sys/prctl.h sys/stat.h sys/types.h sys/wait.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_UID_T
AC_TYPE_MODE_T
AC_TYPE_PID_T

# Checks for library functions.
AC_FUNC_CHOWN
AC_FUNC_MALLOC
AC_CHECK_FUNCS([dup2 mkdir setenv clone])

AM_CONFIG_HEADER([config.h])
AC_CONFIG_FILES([Makefile])

AC_OUTPUT
