#! /bin/bash

folder=`basename "$PWD"`
echo
echo '[Builder]: "'${folder}'" project.'
echo

cc -o signalHandler signalHandler.c 

