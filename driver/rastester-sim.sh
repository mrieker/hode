#!/bin/bash
#
#  test rastester.cc via the netgen simulator
#
cd `dirname $0`
cd ../modules
rm -f simpipe
mknod simpipe p
cat simpipe | ./master.sh -sim - | ../driver/rastester.`uname -m` -sim simpipe
