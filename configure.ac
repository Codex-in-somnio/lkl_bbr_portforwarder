#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([lkl-bbr-portforwarder], [0.12], [])

AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_FILES([
  Makefile
  src/Makefile
])

# Checks for programs.
AC_PROG_CC_STDC
AC_CHECK_PROG(HAS_BASH,[bash],[yes],[no])
test "$HAS_BASH" == "no" && AC_MSG_ERROR([bash not found.])
AC_CHECK_PROG(HAS_CAT,[cat],[yes],[no])
test "$HAS_CAT" == "no" && AC_MSG_ERROR([cat not found.])

# Checks for libraries.
AC_SEARCH_LIBS([lkl_host_ops],[lkl],[],[
  AC_MSG_ERROR([Unable to find liblkl.])
])
AC_SEARCH_LIBS([pthread_create],[pthread],[],[
  AC_MSG_ERROR([Unable to find libpthread.])
])


# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h stdint.h stdlib.h string.h sys/ioctl.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset socket strerror])

AC_OUTPUT
