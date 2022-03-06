#!/bin/bash
r=`realpath $0`
d=`dirname $r`
. $d/iow56sns.si
iowkit170=$d/../iowarrior/iowkit_1.7_`uname -m`/libiowkit-1.7.0
export LIBPATH=$d/classes:$iowkit170/insdir/lib
export CLASSPATH=$d/classes:$d/jacl141/lib/tcljava1.4.1/tcljava.jar:$d/jacl141/lib/tcljava1.4.1/jacl.jar:$iowkit170/java/codemercs.jar
exec java -Djava.library.path=$LIBPATH -ea NetGen "$@"
