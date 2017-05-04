#!/bin/sh
die () {
  echo "*** mkone: $*" 1>&2
  exit 1
}
dchk () {
  [ -d $1 ] || die "can not find directory '$1'"
}
fchk () {
  [ -f $1 ] || die "can not find file '$1'"
}
[ $# = 2 ] || die "expected two arguments but got $#"
msg () {
  echo "[mkone.sh] $*"
}
app=$1; sat=$2
dchk app/$app
fchk app/$app/makefile
dchk sat/$sat
fchk sat/$sat/makefile
target=${app}-${sat}
msg "building SAT solver '$sat'"
make -C sat/$sat/ all
msg "building and linking application '$app' against '$sat'"
IPASIRSOLVER=$sat \
make -C app/$app/ all
[ -f app/$app/$app ] || die "could not build 'app/$app/$app'"
install -s app/$app/$app bin/$target
msg "generated target 'bin/$target'"
