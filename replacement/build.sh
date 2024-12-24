#! /bin/bash

folder=`basename "$PWD"`
echo
echo '[Builder]: "'${folder}'" project.'
echo

cc -o exo5.exe exo5.c
cc -o codeReplacing.exe codeReplacing.c
