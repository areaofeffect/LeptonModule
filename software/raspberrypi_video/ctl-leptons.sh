#!/bin/bash

LOG_SCRIPT=log-lepton.sh
LOCAL_LOG_SCRIPT_LOCATION=.
PI_LOG_SCRIPT_LOCATION=/home/pi

PI_DATA_ROOT=/home/pi/lepton-data
LOCAL_DATA_ROOT=lepton-data

for lepton in l1 
do
    case "$1" in
	start)
	    SCP_CMD="scp $LOCAL_LOG_SCRIPT_LOCATION/$LOG_SCRIPT $lepton:$PI_LOG_SCRIPT_LOCATION"
	    echo "SCP command is " $SCP_CMD
	    $SCP_CMD
	    START_CMD="ssh $lepton 'nohup $PI_LOG_SCRIPT_LOCATION/$LOG_SCRIPT < /dev/null 1>&2 &'"
	    echo "Start logging command is: " $START_CMD
	    eval $START_CMD
	    ;;
	stop)
	    STOP_CMD="ssh $lepton 'nohup pkill -f raspberrypi_video < /dev/null 1>&2 &'"
	    echo "Stop logging command is: " $STOP_CMD
	    eval $STOP_CMD
	    ;;
	collect)
	    mkdir -p $LOCAL_DATA_ROOT/$lepton
	    rsync -rv $lepton:$PI_DATA_ROOT $LOCAL_DATA_ROOT/$lepton
	    ;;
	*)
	    echo "Usage: remote-ctl-leptons.sh {start|stop|collect}"
	    ;;
    esac
done
