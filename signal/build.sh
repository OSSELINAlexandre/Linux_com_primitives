#! /bin/bash

folder=`basename "$PWD"`
echo
echo '[Builder]: "'${folder}'" project.'
echo

gcc -o signalHandler signalHandler.c 

