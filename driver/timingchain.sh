#!/bin/bash
cd `dirname $0`
./loadmod.sh
. ./iow56sns.si
sudo -E ./timingchain.armv7l "$@"
