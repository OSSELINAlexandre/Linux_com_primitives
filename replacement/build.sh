#! /bin/bash

folder=`basename "$PWD"`
echo
echo '[Builder]: "'${folder}'" project.'
echo

cc -o exo1.exe exo1.c
cc -o codeReplacing.exe codeReplacing.c
