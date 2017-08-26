#!/bin/sh

#PI_HOST=10.105.42.245
PI_HOST=192.168.2.15
PI_DIR=Documents/LeptonModule/software/raspberrypi_video
PI_MAKELOG_DIR=/tmp
PI_MAKELOG_FNAME=remote-compile.log

rsync -rv . pi@$PI_HOST:$PI_DIR
ssh pi@$PI_HOST <<EOF
 cd $PI_DIR
 qmake
 make
EOF
#make &> $PI_MAKELOG_DIR/$PI_MAKELOG_FNAME
#scp pi@$PI_HOST:$PI_MAKELOG_DIR/$PI_MAKELOG_FNAME .
#cat ./$PI_MAKELOG_FNAME
#rm $PI_MAKELOG_FNAME
