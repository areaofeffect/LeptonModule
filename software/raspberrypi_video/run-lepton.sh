#!/bin/sh

BASE_ID_FILE=/home/pi/.base_id
BASE_ID=`cat $BASE_ID_FILE`
LEPTON_BIN=raspberrypi_video
LEPTON_BIN_PATH=/home/pi/Documents/LeptonModule/software/raspberrypi_video/$LEPTON_BIN
OPTIONS="-l -d -m"

pkill -f $LEPTON_BIN
export DISPLAY=:0.0
nohup $LEPTON_BIN_PATH $BASE_ID $OPTIONS &
