dnl required version of autoconf
AC_PREREQ([2.53])

dnl package name and package version here
AC_INIT([gzdec-package],[0.10.0])


AC_ARG_WITH([gstreamer-api],
   AC_HELP_STRING([--with-gstreamer-api=VERSION],
                  [build against the specified GStreamer API version
                  @<:@default=1.0@:>@]),
   [GST_API_VERSION="$with_gstreamer_api"], [GST_API_VERSION=1.0])

case $GST_API_VERSION in
0.10)
    AC_MSG_NOTICE(Using Gstreamer API 0.10)
    ;;
1.0)
    AC_MSG_NOTICE(Using Gstreamer API 1.0)
    ;;
*)
    AC_MSG_ERROR([Unsupported GStreamer API, use 0.10 or 1.0])
    ;;
esac

AC_SUBST(GST_API_VERSION)
AC_DEFINE_UNQUOTED(GST_API_VERSION, "$GST_API_VERSION",
  [GStreamer API Version])

dnl required versions of gstreamer and plugins-base
AS_VERSION_COMPARE([$GST_API_VERSION], [0.10],
                   [],
                   [
                    GST_REQUIRED=0.10.3
                    GSTPB_REQUIRED=0.10.3
	           ],
                   [
                    GST_REQUIRED=1.0.0
                    GSTPB_REQUIRED=1.0.0
                   ])

dnl required versions of zlib
ZLIB_REQUIRED=1.2.0

AC_CONFIG_SRCDIR([src/gstgzdec.c])
AC_CONFIG_HEADERS([config.h])

dnl required version of automake
AM_INIT_AUTOMAKE([1.10])

dnl enable mainainer mode by default
AM_MAINTAINER_MODE([enable])

dnl check for tools (compiler etc.)
AC_PROG_CC

dnl required version of libtool
LT_PREREQ([2.2.6])
LT_INIT

dnl give error and exit if we don't have pkgconfig
AC_CHECK_PROG(HAVE_PKGCONFIG, pkg-config, [ ], [
  AC_MSG_ERROR([You need to have pkg-config installed!])
])

dnl Check for the required version of GStreamer core (and gst-plugins-base)
dnl This will export GST_CFLAGS and GST_LIBS variables for use in Makefile.am
dnl
dnl If you need libraries from gst-plugins-base here, also add:
dnl for libgstaudio-0.10: gstreamer-audio-0.10 >= $GST_REQUIRED
dnl for libgstvideo-0.10: gstreamer-video-0.10 >= $GST_REQUIRED
dnl for libgsttag-0.10: gstreamer-tag-0.10 >= $GST_REQUIRED
dnl for libgstpbutils-0.10: gstreamer-pbutils-0.10 >= $GST_REQUIRED
dnl for libgstfft-0.10: gstreamer-fft-0.10 >= $GST_REQUIRED
dnl for libgstinterfaces-0.10: gstreamer-interfaces-0.10 >= $GST_REQUIRED
dnl for libgstrtp-0.10: gstreamer-rtp-0.10 >= $GST_REQUIRED
dnl for libgstrtsp-0.10: gstreamer-rtsp-0.10 >= $GST_REQUIRED
dnl etc.
AS_IF([test "x$GST_API_VERSION" = x0.10],
      [
        PKG_CHECK_MODULES(GST, [
          gstreamer-0.10 >= $GST_REQUIRED
          gstreamer-base-0.10 >= $GST_REQUIRED
          gstreamer-controller-0.10 >= $GST_REQUIRED
        ], [
          AC_SUBST(GST_CFLAGS)
          AC_SUBST(GST_LIBS)
        ], [
          AC_MSG_ERROR([
            You need to install or upgrade the GStreamer development
            packages on your system. On debian-based systems these are
            libgstreamer0.10-dev and libgstreamer-plugins-base0.10-dev.
            on RPM-based systems gstreamer0.10-devel, libgstreamer0.10-devel
            or similar. The minimum version required is $GST_REQUIRED.
          ])
        ])
      ], [
        PKG_CHECK_MODULES(GST, [
          gstreamer-1.0 >= $GST_REQUIRED
          gstreamer-base-1.0 >= $GST_REQUIRED
          gstreamer-controller-1.0 >= $GST_REQUIRED
        ], [
          AC_SUBST(GST_CFLAGS)
          AC_SUBST(GST_LIBS)
        ], [
          AC_MSG_ERROR([
            You need to install or upgrade the GStreamer development
            packages on your system. On debian-based systems these are
            libgstreamer1.0-dev and libgstreamer-plugins-base1.0-dev.
            on RPM-based systems gstreamer0.10-devel, libgstreamer1.0-devel
            or similar. The minimum version required is $GST_REQUIRED.
          ])
        ])
      ])



PKG_CHECK_MODULES(ZLIB, [
  zlib >= $ZLIB_REQUIRED                       
], [
  AC_SUBST(ZLIB_CFLAGS)
  AC_SUBST(ZLIB_LIBS)
], [
  AC_MSG_ERROR([
      You need to install or upgrade libz and libz-dev (libz-devel 
      on RPM-based system). The minimum version required is $ZLIB_REQUIRED
  ])
])


dnl check if compiler understands -Wall (if yes, add -Wall to GST_CFLAGS)
AC_MSG_CHECKING([to see if compiler understands -Wall])
save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -Wall"
AS_VERSION_COMPARE([$GST_API_VERSION], [0.10],
                   [],
                   [CFLAGS="$CFLAGS -DGSTREAMER_010"],
                   [])

AC_COMPILE_IFELSE([AC_LANG_PROGRAM([ ], [ ])], [
  GST_CFLAGS="$GST_CFLAGS -Wall"
  AC_MSG_RESULT([yes])
], [
  AC_MSG_RESULT([no])
])

dnl set the plugindir where plugins should be installed (for src/Makefile.am)
if test "x${prefix}" = "x$HOME"; then
  AS_VERSION_COMPARE([$GST_API_VERSION], [0.10],
                     [],
                     [plugindir="$HOME/.gstreamer-0.10/plugins"],
                     [plugindir="$HOME/.gstreamer-1.0/plugins"])      
else
   AS_VERSION_COMPARE([$GST_API_VERSION], [0.10],
                     [],
                     [plugindir="\$(libdir)/gstreamer-0.10"],
                     [plugindir="\$(libdir)/gstreamer-1.0"])
fi
AC_SUBST(plugindir)

dnl set proper LDFLAGS for plugins
GST_PLUGIN_LDFLAGS='-module -avoid-version -export-symbols-regex [_]*\(gst_\|Gst\|GST_\).*'
AC_SUBST(GST_PLUGIN_LDFLAGS)

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT

