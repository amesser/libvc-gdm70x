define([author],[Andreas Messer])
define([email],[andi@bastelmap.de])
define([libvc_gdm70x_version],[0.2.3])
define([vc_gdm70x_version],[0.2.2])

AC_INIT(libvc-gdm70x,libvc_gdm70x_version,email)
AC_CONFIG_SRCDIR(src/libvc-gdm70x.c)
AM_INIT_AUTOMAKE


AM_PROG_LIBTOOL

AC_CONFIG_HEADER(config.h)


AC_DEFINE(VC_GDM70X_VERSION,"vc_gdm70x_version",[Version of vc_gdm70x tool.])
AC_DEFINE(AUTHOR,"author",[author of the package.])
AC_DEFINE(EMAIL,"email", [email of author.])


AC_PROG_MAKE_SET
AC_PROG_INSTALL

AC_PROG_CC
AC_LANG_C

AC_HEADER_STDC

AC_CHECK_HEADERS(stdio.h errno.h getopt.h assert.h time.h termios.h fcntl.h assert.h sys/ioctl.h,,AC_MSG_ERROR([missing header file!]))
AC_CHECK_FUNCS(fprintf puts fputs malloc memset free perror tcgetattr memcpy cfsetispeed cfsetospeed tcsetattr ioctl close open read printf fwrite atof strncmp time localtime_r sprintf fputc fflush fopen fclose printf getopt_long exit,,AC_MSG_ERROR([missing function!]))

AC_SEARCH_LIBS(clock_gettime, rt,,AC_MSG_ERROR([Failed to link against clock_gettime]))

AC_CONFIG_FILES([libvc-gdm70x.pc])
AC_OUTPUT(Makefile src/Makefile)
