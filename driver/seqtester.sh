#!/bin/bash
./loadmod.sh
. ./iow56sns.si
exec ./seqtester.`uname -m` "$@"
