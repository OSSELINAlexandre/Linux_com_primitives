#! /bin/bash
folder=`basename "$PWD"`
echo
echo '[Builder]: "'${folder}'" project.'
echo

cc -o exo1.exe exo1.c
cc -o exo2.exe exo2.c
cc -o exo3.exe exo3.c

