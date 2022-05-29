#!/bin/bash
./loadmod.sh
exec ./raspictl.armv7l -mintimes rollights.hex
