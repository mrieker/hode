#!/bin/bash -e
mydir=`dirname $0`
uname=`uname -m`
set -x
gcc -nostdlib -E -I../crtl/include $1.c > $1.hode.i
$mydir/compiler.$uname $1.hode.i -s $1.hode.s > /dev/null
$mydir/../asm/assemble.$uname $1.hode.s $1.hode.o > $1.lis
