#!/bin/sh

PI_HOST=$1
PI_DIR=Documents/LeptonModule/software/raspberrypi_video
PI_MAKELOG_DIR=/tmp
PI_MAKELOG_FNAME=remote-compile.log

rsync -rv . pi@$PI_HOST:$PI_DIR
ssh pi@$PI_HOST <<EOF
 cd $PI_DIR
 qmake
 make
EOF
