#!/bin/bash
cd `dirname $0`
./loadmod.sh
. ./iow56sns.si
exec sudo -E ./writeallonestopc.`uname -m` "$@"
