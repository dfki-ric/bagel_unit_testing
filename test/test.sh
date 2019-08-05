#!/bin/bash

export DYLD_LIBRARY_PATH=$MYLD_LIBRARY_PATH

bagel_unit_testing &> /dev/null
DIFF_OUT="$( diff unitLog.log unitLog_test.log )"
if [ -z "$DIFF_OUT" ]
then
    echo " -- test succeeded --"
    exit 0
else
    echo " -- test failed --"
    exit -1
fi
