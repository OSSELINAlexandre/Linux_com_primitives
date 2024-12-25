#! /bin/bash

folder=`basename "$PWD"`
echo
echo '[Builder]: "'${folder}'" project.'
echo

cc -o exo6.exe exo6.c
