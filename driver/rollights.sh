#!/bin/bash
./loadmod.sh
exec ./raspictl.armv7l -mintimes -cpuhz 470000 rollights.hex
