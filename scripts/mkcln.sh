#!/bin/sh
for make in app/*/makefile sat/*/makefile
do
  make -C `dirname $make` clean
done
echo "mkcln.sh: Entering directory '`pwd`/bin'"
cd bin/ || exit 1
echo "rm -f \`cat .gitignore\`"
rm -f `cat .gitignore`
