# For autoconf < 2.63
m4_ifndef([AS_VAR_APPEND],
          AC_DEFUN([AS_VAR_APPEND], $1=$$1$2))
m4_ifndef([AS_VAR_COPY],
          [m4_define([AS_VAR_COPY],
          [AS_LITERAL_IF([$1[]$2], [$1=$$2], [eval $1=\$$2])])])


# SPICE_WARNING(warning)
# SPICE_PRINT_MESSAGES
# ----------------------
# Collect warnings and print them at the end so they are clearly visible.
# ---------------------
AC_DEFUN([SPICE_WARNING],AS_VAR_APPEND([spice_warnings],["|$1"]))
AC_DEFUN([SPICE_PRINT_MESSAGES],[
    ac_save_IFS="$IFS"
    IFS="|"
    for msg in $spice_warnings; do
        IFS="$ac_save_IFS"
        AS_VAR_IF([msg],[],,[AC_MSG_WARN([$msg]); echo >&2])
    done
    IFS="$ac_save_IFS"
])


# SPICE_CHECK_SYSDEPS()
# ---------------------
# Checks for header files and library functions needed by spice-common.
# ---------------------
AC_DEFUN([SPICE_CHECK_SYSDEPS], [
    AC_C_BIGENDIAN
    AC_FUNC_ALLOCA
    AC_CHECK_HEADERS([arpa/inet.h malloc.h netinet/in.h stddef.h stdint.h stdlib.h string.h sys/socket.h unistd.h])

    # Checks for typedefs, structures, and compiler characteristics
    AC_C_INLINE
    AC_TYPE_INT16_T
    AC_TYPE_INT32_T
    AC_TYPE_INT64_T
    AC_TYPE_INT8_T
    AC_TYPE_PID_T
    AC_TYPE_SIZE_T
    AC_TYPE_UINT16_T
    AC_TYPE_UINT32_T
    AC_TYPE_UINT64_T
    AC_TYPE_UINT8_T

    # Checks for library functions
    # do not check malloc or realloc, since that cannot be cross-compiled checked
    AC_FUNC_ERROR_AT_LINE
    AC_FUNC_FORK
    AC_CHECK_FUNCS([dup2 floor inet_ntoa memmove memset pow sqrt])
])


# SPICE_CHECK_SMARTCARD
# ---------------------
# Adds a --enable-smartcard switch in order to enable/disable smartcard
# support, and checks if the needed libraries are available. If found, it will
# return the flags to use in the SMARTCARD_CFLAGS and SMARTCARD_LIBS variables, and
# it will define a USE_SMARTCARD preprocessor symbol as well as a HAVE_SMARTCARD
# Makefile conditional.
#----------------------
AC_DEFUN([SPICE_CHECK_SMARTCARD], [
    AC_ARG_ENABLE([smartcard],
      AS_HELP_STRING([--enable-smartcard=@<:@yes/no/auto@:>@],
                     [Enable smartcard support @<:@default=auto@:>@]),
      [],
      [enable_smartcard="auto"])

    have_smartcard=no
    if test "x$enable_smartcard" != "xno"; then
      PKG_CHECK_MODULES([SMARTCARD], [libcacard >= 0.1.2], [have_smartcard=yes], [have_smartcard=no])
      if test "x$enable_smartcard" != "xauto" && test "x$have_smartcard" = "xno"; then
        AC_MSG_ERROR("Smartcard support requested but libcacard could not be found")
      fi
      if test "x$have_smartcard" = "xyes"; then
        AC_DEFINE(USE_SMARTCARD, [1], [Define if supporting smartcard proxying])
      fi
    fi
    AM_CONDITIONAL(HAVE_SMARTCARD, test "x$have_smartcard" = "xyes")
])


# SPICE_CHECK_CELT051
# -------------------
# Adds a --disable-celt051 switch in order to enable/disable CELT 0.5.1
# support, and checks if the needed libraries are available. If found, it will
# return the flags to use in the CELT051_CFLAGS and CELT051_LIBS variables, and
# it will define a HAVE_CELT051 preprocessor symbol as well as a HAVE_CELT051
# Makefile conditional.
#--------------------
AC_DEFUN([SPICE_CHECK_CELT051], [
    AC_ARG_ENABLE([celt051],
        [  --disable-celt051       Disable celt051 audio codec (enabled by default)],,
        [enable_celt051="yes"])

    if test "x$enable_celt051" = "xyes"; then
        PKG_CHECK_MODULES([CELT051], [celt051 >= 0.5.1.1], [have_celt051=yes], [have_celt051=no])
    else
        have_celt051=no
    fi

    AM_CONDITIONAL([HAVE_CELT051], [test "x$have_celt051" = "xyes"])
    AM_COND_IF([HAVE_CELT051], AC_DEFINE([HAVE_CELT051], 1, [Define if we have celt051 codec]))
])


# SPICE_CHECK_OPUS
# ----------------
# Check for the availability of Opus. If found, it will return the flags to use
# in the OPUS_CFLAGS and OPUS_LIBS variables, and it will define a
# HAVE_OPUS preprocessor symbol as well as a HAVE_OPUS Makefile conditional.
# ----------------
AC_DEFUN([SPICE_CHECK_OPUS], [
    PKG_CHECK_MODULES([OPUS], [opus >= 0.9.14], [have_opus=yes], [have_opus=no])

    AM_CONDITIONAL([HAVE_OPUS], [test "x$have_opus" = "xyes"])
    if test "x$have_opus" = "xyes" ; then
      AC_DEFINE([HAVE_OPUS], [1], [Define if we have OPUS])
    fi
])


# SPICE_CHECK_OPENGL
# ------------------
# Adds a --disable-opengl switch in order to enable/disable OpenGL
# support, and checks if the needed libraries are available. If found, it will
# return the flags to use in the GL_CFLAGS and GL_LIBS variables, and
# it will define USE_OPENGL and GL_GLEXT_PROTOTYPES preprocessor symbol as well
# as a HAVE_GL Makefile conditional.
# ------------------
AC_DEFUN([SPICE_CHECK_OPENGL], [
    AC_ARG_ENABLE([opengl],
        AS_HELP_STRING([--enable-opengl=@<:@yes/no@:>@],
                       [Enable opengl support (not recommended) @<:@default=no@:>@]),
        [],
        [enable_opengl="no"])
    AM_CONDITIONAL(HAVE_GL, test "x$enable_opengl" = "xyes")

    if test "x$enable_opengl" = "xyes"; then
        AC_CHECK_LIB(GL, glBlendFunc, GL_LIBS="$GL_LIBS -lGL", enable_opengl=no)
        AC_CHECK_LIB(GLU, gluSphere, GL_LIBS="$GL_LIBS -lGLU", enable_opengl=no)
        AC_DEFINE([USE_OPENGL], [1], [Define to build with OpenGL support])
        AC_DEFINE([GL_GLEXT_PROTOTYPES], [], [Enable GLExt prototypes])

        if test "x$enable_opengl" = "xno"; then
            AC_MSG_ERROR([GL libraries not available])
        fi
    fi
])


# SPICE_CHECK_PIXMAN
# ------------------
# Check for the availability of pixman. If found, it will return the flags to
# use in the PIXMAN_CFLAGS and PIXMAN_LIBS variables.
#-------------------
AC_DEFUN([SPICE_CHECK_PIXMAN], [
    PKG_CHECK_MODULES(PIXMAN, pixman-1 >= 0.17.7)
])


# SPICE_CHECK_GLIB2
# -----------------
# Check for the availability of glib2. If found, it will return the flags to
# use in the GLIB2_CFLAGS and GLIB2_LIBS variables.
#------------------
AC_DEFUN([SPICE_CHECK_GLIB2], [
    PKG_CHECK_MODULES(GLIB2, glib-2.0 >= 2.22 gio-2.0 >= 2.22)
])

# SPICE_CHECK_PYTHON_MODULES()
# --------------------------
# Adds a --enable-python-checks configure flags as well as checks for the
# availability of the python modules needed by the python scripts generating
# C code from spice.proto. These checks are not needed when building from
# tarballs so they are disabled by default.
#---------------------------
AC_DEFUN([SPICE_CHECK_PYTHON_MODULES], [
    AM_PATH_PYTHON
    AC_ARG_ENABLE([python-checks],
        AS_HELP_STRING([--enable-python-checks=@<:@yes/no@:>@],
                       [Enable checks for Python modules needed to build from git @<:@default=no@:>@]),
                       [],
                       [enable_python_checks="no"])
    if test "x$enable_python_checks" != "xno"; then
        AX_PYTHON_MODULE([six], [1])
        AX_PYTHON_MODULE([pyparsing], [1])
    fi
])


# SPICE_CHECK_LZ4
# ---------------
# Adds a --enable-lz4 switch in order to enable/disable LZ4 compression
# support, and checks if the needed libraries are available. If found, it will
# return the flags to use in the LZ4_CFLAGS and LZ4_LIBS variables, and
# it will define a USE_LZ4 preprocessor symbol.
# conditional.
# ---------------
AC_DEFUN([SPICE_CHECK_LZ4], [
    AC_ARG_ENABLE([lz4],
      AS_HELP_STRING([--enable-lz4=@<:@yes/no@:>@],
                     [Enable LZ4 compression support @<:@default=no@:>@]),
      [],
      [enable_lz4="no"])

    if test "x$enable_lz4" != "xno"; then
      PKG_CHECK_MODULES([LZ4], [liblz4])
      AC_DEFINE(USE_LZ4, [1], [Define to build with lz4 support])
    fi
])


# SPICE_CHECK_GSTREAMER(VAR, version, packages-to-check-for, [action-if-found, [action-if-not-found]])
# ---------------------
# Checks whether the specified GStreamer modules are present and sets the
# corresponding autoconf variables and preprocessor definitions.
# ---------------------
AC_DEFUN([SPICE_CHECK_GSTREAMER], [
    AS_VAR_PUSHDEF([have_gst],[have_]m4_tolower([$1]))dnl
    AS_VAR_PUSHDEF([gst_inspect],[GST_INSPECT_$2])dnl
    PKG_CHECK_MODULES([$1], [$3],
        [have_gst="yes"
         AC_SUBST(AS_TR_SH([[$1]_CFLAGS]))
         AC_SUBST(AS_TR_SH([[$1]_LIBS]))
         AS_VAR_APPEND([SPICE_REQUIRES], [" $3"])
         AC_DEFINE(AS_TR_SH([HAVE_$1]), [1], [Define if supporting GStreamer $2])
         AC_PATH_PROG(gst_inspect, gst-inspect-$2)
         AS_IF([test "x$gst_inspect" = x],
               SPICE_WARNING([Cannot verify that the required runtime GStreamer $2 elements are present because gst-inspect-$2 is missing]))
         $4],
        [have_gst="no"
         $5])
    AS_VAR_POPDEF([gst_inspect])dnl
    AS_VAR_POPDEF([have_gst])dnl
])

# SPICE_CHECK_GSTREAMER_ELEMENTS(gst-inspect, package, elements-to-check-for)
# ---------------------
# Checks that the specified GStreamer elements are installed. If not it
# issues a warning and sets missing_gstreamer_elements.
# ---------------------
AC_DEFUN([SPICE_CHECK_GSTREAMER_ELEMENTS], [
AS_IF([test "x$1" != x],
      [missing=""
       for element in $3
       do
           AS_VAR_PUSHDEF([cache_var],[spice_cv_prog_${1}_${element}])dnl
           AC_CACHE_CHECK([for the $element GStreamer element], cache_var,
                          [found=no
                           "$1" $element >/dev/null 2>/dev/null && found=yes
                           eval "cache_var=$found"])
           AS_VAR_COPY(res, cache_var)
           AS_IF([test "x$res" = "xno"], [missing="$missing $element"])
           AS_VAR_POPDEF([cache_var])dnl
       done
       AS_IF([test "x$missing" != x],
             [SPICE_WARNING([The$missing GStreamer element(s) are missing. You should be able to find them in the $2 package.])
              missing_gstreamer_elements="yes"],
             [test "x$missing_gstreamer_elements" = x],
             [missing_gstreamer_elements="no"])
      ])
])
