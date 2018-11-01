
AC_DEFUN([CHECK_GTEST], [

SAVE_CPPFLAGS="$CPPFLAGS"
SAVE_LDFLAGS="$LDFLAGS"
SAVE_LIBS="$LIBS"

AC_ARG_WITH(gtest,
  [AS_HELP_STRING(
    [--with-gtest],
    [Give path to gtest installation],
    )],
  [ gtest_path=$withval ],
  [ gtest_path="" ]
)

AH_TEMPLATE([GTEST_ENABLED], [Whether GTEST is available and GTEST stats should be built])
if test -z "$gtest_path"; then
#AC_MSG_ERROR([gtest can only be enabled by giving the install prefix --with-gtest=PREFIX, even for system default path])
AM_CONDITIONAL([HAVE_GTEST], [false])
else
AM_CONDITIONAL([HAVE_GTEST], [true])
AC_DEFINE_UNQUOTED([GTEST_ENABLED], 1, [gtest is enabled and installed])
GTEST_CPPFLAGS="-I${gtest_path}/include"
GTEST_LIBS="-lgtest"
GTEST_LDFLAGS="-L${gtest_path}/lib"
AC_SUBST([GTEST_CPPFLAGS])
AC_SUBST([GTEST_LIBS])
AC_SUBST([GTEST_LDFLAGS])
fi

])
