#!/bin/bash
#
#  test raspictl by using the netgen simulator
#
cd `dirname $0`
cd ../modules
if [ umul.hex -ot umul.asm ]
then
    tclsh assemble.tcl umul.asm umul.hex > umul.lis
fi
rm -f simpipe
mknod simpipe p
cat simpipe | ./master.sh -sim - | ../driver/raspictl -printstate -sim simpipe -tclhex umul.hex
