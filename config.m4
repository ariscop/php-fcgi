PHP_ARG_ENABLE(fcgx, whether to enable FCGX support,
[ --enable-hello   Enable Hello World support])

if test "$PHP_FCGX" = "yes"; then
  AC_DEFINE(HAVE_FCGX, 1, [Whether you have FCGX])
  AC_CHECK_LIB(fcgi, FCGX_OpenSocket)
  PHP_SUBST(FCGX_SHARED_LIBADD)
  PHP_ADD_LIBRARY(fcgi, 1, FCGX_SHARED_LIBADD)
  PHP_NEW_EXTENSION(fcgx, php_fcgx.c, $ext_shared)
fi
