#!/bin/sh

PI_HOST=$1
PI_DIR=Documents/LeptonModule/software/raspberrypi_video

rsync -rv . pi@$PI_HOST:$PI_DIR
ssh pi@$PI_HOST <<EOF
 cd $PI_DIR
 qmake
 make
EOF
