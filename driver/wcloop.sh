#!/bin/bash
./loadmod.sh
cpuhz=75000
date > wcloop.log
echo $cpuhz >> wcloop.log
nohup ./raspictl.armv7l -cpuhz $cpuhz -oddok wcloop.hex < /dev/null >> wcloop.log 2>&1 &
