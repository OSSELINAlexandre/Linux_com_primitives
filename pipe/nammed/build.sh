#! /bin/bash

folder=`basename "$PWD"`
echo
echo '[Builder]: "'${folder}'" project.'
echo

cc -o client.exe client.c definition.h
cc -o server.exe server.c definition.h
