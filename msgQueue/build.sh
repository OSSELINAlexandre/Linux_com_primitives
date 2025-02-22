#! /bin/bash

folder=`basename "$PWD"`
echo
echo '[Builder]: "'${folder}'" project.'
echo

cc -o ClientMsgQueue ClientMsgQueue.c definition.h
cc -o ServerMsgQueue ServerMsgQueue.c definition.h

