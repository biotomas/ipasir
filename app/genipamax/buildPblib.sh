#!/bin/bash

die () {
  echo "*** buildPblib.sh: $*" 1>&2
  exit 1
}

msg () {
  echo "buildPblib.sh: $*"
}

checkbin () {
  $1 $2 1>/dev/null 2>/dev/null || \
  die "execution of '$1 $2' failed ('$1' required)"
}

checkbin tar --help
checkbin g++ --version
checkbin cmake --version

if [ -d pblib ]
then
  msg "Removing PBLib (consider using 'ccache' for faster rebuild)"
  rm -rf pblib
fi

msg "Unpacking PBLib"
#mkdir pblib
#tar xvf pblib.tgz -C pblib
tar xvf pblib.tar.gz

msg "Building PBLib"
cd pblib
cmake -Wno-dev .
make
cd ..
