dnl $Id$
dnl config.m4 for extension wordsfilter

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

PHP_ARG_WITH(wordsfilter, for wordsfilter support,
dnl Make sure that the comment is aligned:
[  --with-wordsfilter             Include wordsfilter support])

dnl Otherwise use enable:

dnl PHP_ARG_ENABLE(wordsfilter, whether to enable wordsfilter support,
dnl Make sure that the comment is aligned:
dnl [  --enable-wordsfilter           Enable wordsfilter support])

if test "$PHP_WORDSFILTER" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-wordsfilter -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/wordsfilter.h"  # you most likely want to change this
  dnl if test -r $PHP_WORDSFILTER/$SEARCH_FOR; then # path given as parameter
  dnl   WORDSFILTER_DIR=$PHP_WORDSFILTER
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for wordsfilter files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       WORDSFILTER_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$WORDSFILTER_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the wordsfilter distribution])
  dnl fi

  dnl # --with-wordsfilter -> add include path
  dnl PHP_ADD_INCLUDE($WORDSFILTER_DIR/include)

  dnl # --with-wordsfilter -> check for lib and symbol presence
  dnl LIBNAME=wordsfilter # you may want to change this
  dnl LIBSYMBOL=wordsfilter # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $WORDSFILTER_DIR/$PHP_LIBDIR, WORDSFILTER_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_WORDSFILTERLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong wordsfilter lib version or lib not found])
  dnl ],[
  dnl   -L$WORDSFILTER_DIR/$PHP_LIBDIR -lm
  dnl ])
  dnl
  dnl PHP_SUBST(WORDSFILTER_SHARED_LIBADD)

  PHP_NEW_EXTENSION(wordsfilter, wordsfilter.c, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1)
fi
