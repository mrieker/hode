#!/bin/bash
uname=`uname -m`
set -x
exec ../driver/raspictl.$uname -nohw "$@"
