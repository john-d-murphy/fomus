#! /bin/sh

# if test -n "$1"; then
#     aclocal -I $1 >/dev/null 2>&1
# else
#     aclocal >/dev/null 2>&1
# fi
# glibtoolize >/dev/null 2>&1 || libtoolize >/dev/null 2>&1
# automake -a >/dev/null 2>&1
# if test -n "$1"; then
#     ACLOCAL="aclocal -I $1" autoreconf >/dev/null 2>&1 && { echo "bootstrap successful"; exit 0; }
# else
#     autoreconf >/dev/null 2>&1 && { echo "bootstrap successful"; exit 0; }
# fi
# echo "bootstrap failed"
# exit 1

autoreconf -fi >/dev/null 2>&1 || { autoreconf -fi; echo "bootstrap failed"; exit 1; }
echo "bootstrap successful"
exit 0
