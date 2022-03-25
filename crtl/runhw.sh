#!/bin/bash
#
#  Run the given .hex file on the Hode CPU hardware boardset
#
../driver/loadmod.sh
exec ../driver/raspictl.armv7l "$@"
