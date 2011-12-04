#! /bin/sh
echo "Generating configuration files for Pidgin-Festival, please wait...."
echo;

echo "Running libtoolize, please ignore non-fatal messages...."
echo n | libtoolize --copy --force || exit;
echo;

aclocal || exit;
autoheader || exit;
automake --add-missing --copy
autoconf || exit;
automake || exit;
