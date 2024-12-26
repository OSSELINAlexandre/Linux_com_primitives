#! /bin/bash

folder=`basename "$PWD"`
echo
echo '[Builder]: "'${folder}'" project.'
echo

cc -o exo7.exe exo7.c 
cc -o exo8.exe exo8.c 
